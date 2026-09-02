#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cwctype>
#include <fcntl.h>
#include <filesystem>
#include <io.h>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// internal commands are compiled into cmd.exe itself, there is no way to enumerate them at runtime, so this list is static (Win11 cmd)
static const std::vector<std::wstring> CMD_BUILTINS = {
    L"assoc", L"break", L"call", L"cd", L"chdir", L"cls", L"color", L"copy", L"date",
    L"del", L"dir", L"dpath", L"echo", L"endlocal", L"erase", L"exit", L"for", L"ftype",
    L"goto", L"if", L"md", L"mkdir", L"mklink", L"move", L"path", L"pause", L"popd",
    L"prompt", L"pushd", L"rd", L"rem", L"ren", L"rename", L"rmdir", L"set", L"setlocal",
    L"shift", L"start", L"time", L"title", L"type", L"ver", L"verify", L"vol",
};

static const std::wstring FOOTER_SEP = L"...................................";

struct Row {
    std::wstring name;
    std::wstring path;
    bool shadowed;
};

static std::wstring lower(std::wstring s) {
    for (wchar_t& c : s) c = towlower(c);
    return s;
}

static bool iless(const std::wstring& a, const std::wstring& b) {
    return lower(a) < lower(b);
}

static std::wstring getenv_w(const wchar_t* name, const wchar_t* fallback) {
    wchar_t buf[32768];
    DWORD n = GetEnvironmentVariableW(name, buf, 32768);
    if (n == 0 || n >= 32768) return fallback;
    return std::wstring(buf, n);
}

static std::wstring expand_env(const std::wstring& s) {
    wchar_t buf[32768];
    DWORD n = ExpandEnvironmentStringsW(s.c_str(), buf, 32768);
    if (n == 0 || n > 32768) return s;
    return std::wstring(buf);
}

static std::wstring trim(const std::wstring& s, const wchar_t* junk = L" \t") {
    size_t a = s.find_first_not_of(junk);
    size_t b = s.find_last_not_of(junk);
    if (a == std::wstring::npos) return L"";
    return s.substr(a, b - a + 1);
}

static std::vector<std::wstring> split(const std::wstring& s, wchar_t sep) {
    std::vector<std::wstring> out;
    size_t start = 0;
    while (start <= s.size()) {
        size_t pos = s.find(sep, start);
        if (pos == std::wstring::npos) { out.push_back(s.substr(start)); break; }
        out.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return out;
}

// doskey writes through the pipe in the OEM codepage, so bytes are decoded with CP_OEMCP before use
static std::wstring from_oem(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_OEMCP, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_OEMCP, 0, s.data(), (int)s.size(), w.data(), n);
    return w;
}

static std::vector<std::pair<std::wstring, std::wstring>> get_doskey_macros() {
    // only macros loaded by the AutoRun registry value show up here, macros defined by hand in another live cmd window are per-session and invisible from outside
    std::vector<std::pair<std::wstring, std::wstring>> macros;
    FILE* pipe = _popen("cmd /c doskey /macros", "r");
    if (!pipe) return macros;
    char line[4096];
    while (fgets(line, sizeof(line), pipe)) {
        std::string raw(line);
        while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r')) raw.pop_back();
        std::wstring s = from_oem(raw);
        size_t eq = s.find(L'=');
        if (eq != std::wstring::npos && eq > 0) {
            macros.push_back({trim(s.substr(0, eq)), trim(s.substr(eq + 1))});
        }
    }
    _pclose(pipe);
    return macros;
}

static void scan_path(std::vector<Row>& rows, std::map<std::wstring, std::wstring>& winners) {
    std::wstring pathext = getenv_w(L"PATHEXT", L".COM;.EXE;.BAT;.CMD");
    std::map<std::wstring, int> ext_rank;
    int rank = 0;
    for (const std::wstring& e : split(pathext, L';')) {
        std::wstring ext = lower(trim(e));
        if (!ext.empty() && ext[0] == L'.' && !ext_rank.count(ext)) ext_rank[ext] = rank++;
    }

    std::wstring path = getenv_w(L"PATH", L"");
    std::set<std::wstring> seen_dirs;
    for (const std::wstring& raw : split(path, L';')) {
        std::wstring d = expand_env(trim(raw, L" \t\""));
        if (d.empty()) continue;
        // PATH often lists the same directory twice (user + system half), scanning it again would fake duplicates
        std::wstring norm = lower(d);
        std::replace(norm.begin(), norm.end(), L'/', L'\\');
        while (norm.size() > 3 && norm.back() == L'\\') norm.pop_back();
        if (!seen_dirs.insert(norm).second) continue;

        std::vector<fs::path> entries;
        std::error_code ec;
        for (fs::directory_iterator it(d, ec), end; !ec && it != end; it.increment(ec)) {
            std::error_code fec;
            if (!it->is_regular_file(fec)) continue;
            std::wstring ext = lower(it->path().extension().wstring());
            if (ext_rank.count(ext)) entries.push_back(it->path());
        }
        std::stable_sort(entries.begin(), entries.end(), [&](const fs::path& a, const fs::path& b) {
            return ext_rank[lower(a.extension().wstring())] < ext_rank[lower(b.extension().wstring())];
        });
        for (const fs::path& f : entries) {
            std::wstring stem = lower(f.stem().wstring());
            bool shadowed = winners.count(stem) > 0;
            if (!shadowed) winners[stem] = f.wstring();
            rows.push_back({f.filename().wstring(), f.wstring(), shadowed});
        }
    }
}

static std::vector<std::wstring> unique_names(const std::map<std::wstring, std::wstring>& winners, const std::vector<std::pair<std::wstring, std::wstring>>& macros) {
    std::set<std::wstring> seen;
    std::vector<std::wstring> names;
    auto add = [&](const std::wstring& n) {
        if (seen.insert(lower(n)).second) names.push_back(n);
    };
    for (const std::wstring& b : CMD_BUILTINS) add(b);
    for (const auto& m : macros) add(m.first);
    for (const auto& w : winners) add(fs::path(w.second).filename().wstring());
    std::sort(names.begin(), names.end(), iless);
    return names;
}

static std::map<std::wstring, std::vector<std::wstring>> dup_groups(const std::vector<Row>& rows) {
    std::map<std::wstring, std::vector<std::wstring>> groups;
    for (const Row& r : rows) groups[lower(r.name)].push_back(r.path);
    std::map<std::wstring, std::vector<std::wstring>> dups;
    for (const auto& g : groups) {
        if (g.second.size() > 1) dups[g.first] = g.second;
    }
    return dups;
}

static void print_names(const std::map<std::wstring, std::wstring>& winners, const std::vector<std::pair<std::wstring, std::wstring>>& macros) {
    // only the file that wins bare-name resolution is listed, a powershell.exe shadowed by an earlier powershell.bat stays hidden
    std::vector<std::wstring> names = unique_names(winners, macros);
    for (const std::wstring& n : names) std::wcout << n << L"\n";
    std::wcout << FOOTER_SEP << L"\n";
    std::wcout << names.size() << L" unique commands found\n";
}

static void print_details(const std::vector<Row>& rows, const std::map<std::wstring, std::wstring>& winners, const std::vector<std::pair<std::wstring, std::wstring>>& macros) {
    std::wcout << L"== cmd internal commands ==\n";
    for (const std::wstring& b : CMD_BUILTINS) std::wcout << b << L"\n";

    std::wcout << L"\n== doskey macros (aliases) ==\n";
    if (macros.empty()) std::wcout << L"(none registered via AutoRun)\n";
    for (const auto& m : macros) std::wcout << m.first << L"\t" << m.second << L"\n";

    std::wcout << L"\n== executables on PATH ==\n";
    std::vector<Row> sorted_rows = rows;
    std::stable_sort(sorted_rows.begin(), sorted_rows.end(), [](const Row& a, const Row& b) { return iless(a.name, b.name); });
    for (const Row& r : sorted_rows) {
        std::wcout << r.name << L"\t" << r.path << L"\t" << (r.shadowed ? L"SHADOWED" : L"") << L"\n";
    }

    std::wcout << L"\nbuiltins: " << CMD_BUILTINS.size() << L", macros: " << macros.size() << L", files on PATH: " << rows.size() << L"\n";
    std::wcout << FOOTER_SEP << L"\n";
    std::wcout << unique_names(winners, macros).size() << L" unique commands found\n";
    std::wcout << dup_groups(rows).size() << L" duplicates found\n";
}

static void print_dups(const std::vector<Row>& rows) {
    auto dups = dup_groups(rows);
    bool first = true;
    for (const auto& g : dups) {
        if (!first) std::wcout << L"\n========\n\n";
        first = false;
        std::wcout << fs::path(g.second[0]).filename().wstring() << L" [" << g.second.size() << L"]\n";
        for (const std::wstring& p : g.second) std::wcout << p << L"\n";
    }
    if (dups.empty()) std::wcout << L"(no duplicates found)\n";
    std::wcout << FOOTER_SEP << L"\n";
    std::wcout << dups.size() << L" duplicates found\n";
}

static void print_help() {
    std::wcout <<
        L"listcmd - list everything callable from Windows cmd\n"
        L"\n"
        L"usage: listcmd [--details | --dup | --help]\n"
        L"\n"
        L"  (no args)  unique callable names only: builtins, doskey macros, and the\n"
        L"             PATH file that wins bare-name resolution for each name\n"
        L"  --details  full report: builtins, macros, and every PATH file with its\n"
        L"             location and a SHADOWED tag\n"
        L"  --dup      only names present in more than one PATH directory, each with\n"
        L"             its count and every path, in PATH resolution order\n"
        L"  --help     this help\n";
}

int wmain(int argc, wchar_t* argv[]) {
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);

    bool details = false, dup = false;
    for (int i = 1; i < argc; i++) {
        std::wstring a = argv[i];
        if (a == L"--details") details = true;
        else if (a == L"--dup") dup = true;
        else if (a == L"--help" || a == L"-h") { print_help(); return 0; }
        else {
            std::wcerr << L"unknown argument: " << a << L"\ntry --help\n";
            return 2;
        }
    }
    if (details && dup) {
        std::wcerr << L"--details and --dup are mutually exclusive\n";
        return 2;
    }

    std::vector<Row> rows;
    std::map<std::wstring, std::wstring> winners;
    scan_path(rows, winners);

    if (dup) print_dups(rows);
    else if (details) print_details(rows, winners, get_doskey_macros());
    else print_names(winners, get_doskey_macros());
    return 0;
}

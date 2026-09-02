#include <windows.h>
#include <fcntl.h>
#include <filesystem>
#include <io.h>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::wstring lower(std::wstring s) {
    for (wchar_t& c : s) c = towlower(c);
    return s;
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

static std::wstring trim(const std::wstring& s, const wchar_t* junk = L" \t\"") {
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

// returns the path with the real on-disk name casing, or empty when it is not a file
static std::wstring real_file(const std::wstring& p) {
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(p.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return L"";
    FindClose(h);
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return L"";
    return (fs::path(p).parent_path() / fd.cFileName).wstring();
}

// every match for the name in the exact order cmd would try them: current directory, then each PATH directory, PATHEXT order within a directory
static std::vector<std::wstring> find_candidates(const std::wstring& name) {
    std::vector<std::wstring> exts;
    for (const std::wstring& e : split(getenv_w(L"PATHEXT", L".COM;.EXE;.BAT;.CMD"), L';')) {
        std::wstring ext = trim(e);
        if (!ext.empty() && ext[0] == L'.') exts.push_back(ext);
    }
    bool has_ext = false;
    std::wstring name_ext = lower(fs::path(name).extension().wstring());
    for (const std::wstring& e : exts) {
        if (lower(e) == name_ext) has_ext = true;
    }

    std::vector<std::wstring> dirs;
    dirs.push_back(fs::current_path().wstring());
    for (const std::wstring& raw : split(getenv_w(L"PATH", L""), L';')) {
        std::wstring d = expand_env(trim(raw));
        if (!d.empty()) dirs.push_back(d);
    }

    std::vector<std::wstring> found;
    std::set<std::wstring> seen_dirs;
    for (const std::wstring& d : dirs) {
        std::wstring norm = lower(d);
        while (norm.size() > 3 && (norm.back() == L'\\' || norm.back() == L'/')) norm.pop_back();
        if (!seen_dirs.insert(norm).second) continue;
        std::wstring base = (fs::path(d) / name).wstring();
        if (has_ext) {
            std::wstring r = real_file(base);
            if (!r.empty()) found.push_back(r);
            continue;
        }
        for (const std::wstring& e : exts) {
            std::wstring r = real_file(base + e);
            if (!r.empty()) found.push_back(r);
        }
    }
    return found;
}

// Windows command line quoting rules: wrap in quotes when needed, double backslashes that precede a quote, escape embedded quotes
static std::wstring quote_arg(const std::wstring& a) {
    if (!a.empty() && a.find_first_of(L" \t\"") == std::wstring::npos) return a;
    std::wstring out = L"\"";
    size_t backslashes = 0;
    for (wchar_t c : a) {
        if (c == L'\\') { backslashes++; continue; }
        if (c == L'"') {
            out.append(backslashes * 2 + 1, L'\\');
            out += c;
        } else {
            out.append(backslashes, L'\\');
            out += c;
        }
        backslashes = 0;
    }
    out.append(backslashes * 2, L'\\');
    out += L"\"";
    return out;
}

static int run(const std::wstring& exe, const std::vector<std::wstring>& args) {
    std::wstring ext = lower(fs::path(exe).extension().wstring());
    bool script = (ext == L".bat" || ext == L".cmd");
    std::wstring cmdline;
    std::wstring app;
    if (script) {
        // batch files are not executables, cmd has to interpret them
        app = getenv_w(L"COMSPEC", L"cmd.exe");
        cmdline = quote_arg(app) + L" /c \"" + quote_arg(exe);
        for (const std::wstring& a : args) cmdline += L" " + quote_arg(a);
        cmdline += L"\"";
    } else {
        app = exe;
        cmdline = quote_arg(exe);
        for (const std::wstring& a : args) cmdline += L" " + quote_arg(a);
    }

    // let Ctrl+C reach the child instead of killing the launcher before the child is done
    SetConsoleCtrlHandler(nullptr, TRUE);

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    std::vector<wchar_t> buf(cmdline.begin(), cmdline.end());
    buf.push_back(0);
    if (!CreateProcessW(app.c_str(), buf.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
        std::wcerr << L"runver: failed to start " << exe << L" (error " << GetLastError() << L")\n";
        return 127;
    }
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    return (int)code;
}

static void print_help() {
    std::wcout <<
        L"runver - run the Nth program of a given name found on PATH\n"
        L"\n"
        L"usage: runver <n> <program> [args...]\n"
        L"       runver list <program>\n"
        L"\n"
        L"  <n>      1-based index in cmd's search order, so 1 is what typing\n"
        L"           the bare name would run, 2 the first shadowed one, and so on\n"
        L"  list     show every match with its index, in search order\n"
        L"\n"
        L"example: runver list python\n"
        L"         runver 2 python -c \"import sys; print(sys.version)\"\n";
}

int wmain(int argc, wchar_t* argv[]) {
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);

    if (argc < 3) {
        print_help();
        return argc == 2 && (std::wstring(argv[1]) == L"--help" || std::wstring(argv[1]) == L"-h") ? 0 : 2;
    }

    std::wstring mode = argv[1];
    std::wstring program = argv[2];
    std::vector<std::wstring> found = find_candidates(program);

    if (mode == L"list") {
        if (found.empty()) {
            std::wcerr << L"runver: no match for " << program << L"\n";
            return 1;
        }
        for (size_t i = 0; i < found.size(); i++) std::wcout << (i + 1) << L"  " << found[i] << L"\n";
        return 0;
    }

    wchar_t* end = nullptr;
    long n = wcstol(mode.c_str(), &end, 10);
    if (*end != 0 || n < 1) {
        std::wcerr << L"runver: first argument must be a positive number or 'list'\n";
        return 2;
    }
    if ((size_t)n > found.size()) {
        std::wcerr << L"runver: only " << found.size() << L" match(es) for " << program << L", use 'runver list " << program << L"'\n";
        return 1;
    }

    std::vector<std::wstring> args(argv + 3, argv + argc);
    return run(found[n - 1], args);
}

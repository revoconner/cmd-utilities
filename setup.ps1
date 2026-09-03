$base = "C:\Aliases"
$folder = $base
$i = 1
while (Test-Path $folder) {
    $folder = "${base}${i}"
    $i++
}

New-Item -ItemType Directory -Path $folder | Out-Null

$repo = "https://api.github.com/repos/revoconner/cmd-utilities/contents/"
$files = Invoke-RestMethod -Uri $repo
$batFiles = $files | Where-Object { $_.name -like "*.bat" }
foreach ($f in $batFiles) {
    Invoke-WebRequest -Uri $f.download_url -OutFile "$folder\$($f.name)"
}

$userPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($userPath -notlike "*$folder*") {
    [Environment]::SetEnvironmentVariable("Path", "$userPath;$folder", "User")
}

Write-Host "Created $folder with $($batFiles.Count) batch file(s) and added to user PATH."
Write-Host "Restart your terminal for PATH changes to take effect."


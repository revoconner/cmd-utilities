@echo off
setlocal
if "%~1"=="" (
	echo Usage: vercel-rm ^<project-name^> [-y^|--yes]
	exit /b 1
)
set "PROJECT=%~1"
set "YES_ARG="
if /i "%~2"=="-y" set "YES_ARG=-Yes"
if /i "%~2"=="--yes" set "YES_ARG=-Yes"

set "TMPSCRIPT=%TEMP%\vercel-rm-%RANDOM%-%RANDOM%.ps1"
call powershell -NoProfile -ExecutionPolicy Bypass -Command "$c = Get-Content -Raw -LiteralPath '%~f0'; $ps = ($c -split '#PS_BEGIN\r?\n', 2)[1]; Set-Content -LiteralPath '%TMPSCRIPT%' -Value $ps -NoNewline -Encoding UTF8" <NUL
if not exist "%TMPSCRIPT%" (
	echo Failed to extract embedded PowerShell.
	exit /b 1
)
call powershell -NoProfile -ExecutionPolicy Bypass -File "%TMPSCRIPT%" -Project "%PROJECT%" %YES_ARG%
set "EXITCODE=%ERRORLEVEL%"
del "%TMPSCRIPT%" 2>nul
endlocal & exit /b %EXITCODE%

#PS_BEGIN
param(
	[Parameter(Mandatory = $true)][string]$Project,
	[switch]$Yes
)

$ErrorActionPreference = 'Stop'

$authPath = Join-Path $env:APPDATA 'com.vercel.cli\Data\auth.json'
if (-not (Test-Path $authPath)) {
	Write-Host "Vercel auth not found at $authPath. Run 'vercel login' first." -ForegroundColor Red
	exit 1
}
$t = (Get-Content $authPath | ConvertFrom-Json).token
if (-not $t) {
	Write-Host "No token in $authPath." -ForegroundColor Red
	exit 1
}
$h = @{ Authorization = "Bearer $t" }

$teams = @((Invoke-RestMethod 'https://api.vercel.com/v2/teams' -Headers $h).teams)
$team = if ($teams.Count -gt 0) { $teams[0] } else { $null }
$tid = if ($team) { $team.id } else { $null }
$scope = if ($team) { $team.slug } else { 'personal' }

$all = @()
$u = 0
do {
	$url = "https://api.vercel.com/v6/deployments?app=$Project&limit=100"
	if ($tid) { $url += "&teamId=$tid" }
	if ($u) { $url += "&until=$u" }
	$r = Invoke-RestMethod $url -Headers $h
	$all += $r.deployments
	$u = $r.pagination.next
} while ($u)

if ($all.Count -eq 0) {
	Write-Host "No deployments found for project '$Project' under scope '$scope'."
	exit 0
}

$keep = [System.Collections.Generic.HashSet[string]]::new()

$latestProd = $all |
	Where-Object { $_.state -eq 'READY' -and $_.target -eq 'production' } |
	Sort-Object createdAt -Descending |
	Select-Object -First 1
if ($latestProd) { [void]$keep.Add($latestProd.url) }

$all |
	Where-Object { $_.state -eq 'READY' -and $_.meta.githubCommitRef } |
	Group-Object { $_.meta.githubCommitRef } |
	ForEach-Object {
		$latest = $_.Group | Sort-Object createdAt -Descending | Select-Object -First 1
		[void]$keep.Add($latest.url)
	}

$removeUrls = @($all | Where-Object { -not $keep.Contains($_.url) } | ForEach-Object { "https://$($_.url)" })

Write-Host ""
Write-Host "Project: $Project  (scope: $scope)"
Write-Host "Total deployments: $($all.Count)"
Write-Host "Keep:   $($keep.Count)"
Write-Host "Remove: $($removeUrls.Count)"

if ($removeUrls.Count -eq 0) {
	Write-Host "Nothing to remove."
	exit 0
}

Write-Host ""
Write-Host "URLs to remove:"
$removeUrls | ForEach-Object { Write-Host "  $_" }
Write-Host ""

if (-not $Yes) {
	$resp = Read-Host "Continue? [y/N]"
	if ($resp -notmatch '^[yY]') {
		Write-Host "Aborted."
		exit 0
	}
}

& vercel rm -y @removeUrls
exit $LASTEXITCODE

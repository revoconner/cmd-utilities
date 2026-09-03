@echo off
powershell -NoProfile -Command "Set-Clipboard ([Console]::In.ReadToEnd().TrimEnd([char[]]@(13,10)))"

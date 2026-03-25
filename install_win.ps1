# Keyboard Daemon - A cross-platform layout synchronization tool
# Copyright (C) 2026 Veitangie
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

param (
    [string]$ArgsString = ""
)

if ([string]::IsNullOrWhiteSpace($ArgsString)) {
    Write-Host "=========================================="
    Write-Host " Keyboard Daemon Installer (Windows)      "
    Write-Host "=========================================="
    $ArgsString = Read-Host "Enter your layout arguments separated by space (e.g. 'en ru')"
}

$BinPath = "$env:LOCALAPPDATA\kbd\kbd.exe"
$TargetDir = Split-Path $BinPath -Parent

if (-not (Test-Path $TargetDir)) {
    New-Item -ItemType Directory -Force -Path $TargetDir | Out-Null
}

if (-not (Test-Path "kbd.exe")) {
    Write-Host "kbd.exe not found. Please compile it first using 'make win'."
    exit 1
}

Copy-Item "kbd.exe" -Destination $BinPath -Force

$RegistryPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
$Name = "KeyboardDaemon"
$Value = "`"$BinPath`" $ArgsString"

New-ItemProperty -Path $RegistryPath -Name $Name -Value $Value -PropertyType String -Force | Out-Null

Write-Host "Installed kbd.exe to $BinPath and added to startup registry: HKCU\Software\Microsoft\Windows\CurrentVersion\Run"
Start-Process -WindowStyle Hidden -FilePath $BinPath -ArgumentList $ArgsString
Write-Host "Started kbd.exe!"

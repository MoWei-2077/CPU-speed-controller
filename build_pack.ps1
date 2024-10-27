$ErrorActionPreference = 'Stop'


$id = Get-Content magisk/module.prop | Where-Object { $_ -match "id=" }
$id = $id.split('=')[1]
$version = Get-Content magisk/module.prop | Where-Object { $_ -match "version=" }
$version = $version.split('=')[1]
$versionCode = Get-Content magisk/module.prop | Where-Object { $_ -match "versionCode=" }
$versionCode = $versionCode.split('=')[1]
$zipFile = "${id}_v${version}.zip"


function log
{
    [CmdletBinding()]
    Param
    (
        [Parameter(Mandatory = $true, Position = 0)]
        [string]$LogMessage
    )
    Write-Output ("[{0}] {1}" -f (Get-Date), $LogMessage)
}

$clang = "D:\AndroidStudio\AndroidSDK\ndk\28.0.12433566\toolchains\llvm\prebuilt\windows-x86_64\bin\clang++.exe"

$sysroot = "--sysroot=D:\AndroidStudio\AndroidSDK\ndk\28.0.12433566\toolchains\llvm\prebuilt\windows-x86_64\sysroot"
$cppFlags = "--target=aarch64-linux-android31 -std=c++20 -static -s -fstrict-aliasing -Ofast -flto -funroll-loops -finline-functions -fomit-frame-pointer -Wall -Wextra -Wshadow -fno-exceptions -fno-rtti -DNDEBUG -fPIE"
log "������..."
& $clang $sysroot $cppFlags.Split(' ') -I. './Source code/main.cpp' -o ./magisk/system/bin/MW_CpuSpeedController

if (-not$?)
{
    log "����ʧ��"
    exit
}

log "����� $zipFile"
& ./7za.exe a "./out/${zipFile}" ./magisk/* | Out-Null
if (-not$?)
{
    log "���ʧ��"
    exit
}

log "CS���ȱ������"
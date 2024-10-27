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
log "构建中..."
& $clang $sysroot $cppFlags.Split(' ') -I. './Source code/main.cpp' -o ./magisk/system/bin/MW_CpuSpeedController

if (-not$?)
{
    log "构建失败"
    exit
}

log "打包中 $zipFile"
& ./7za.exe a "./out/${zipFile}" ./magisk/* | Out-Null
if (-not$?)
{
    log "打包失败"
    exit
}

log "CS调度编译完成"
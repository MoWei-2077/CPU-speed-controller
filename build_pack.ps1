$ErrorActionPreference = 'Stop'

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
& $clang $sysroot $cppFlags.Split(' ') -I. './Source code/main.cpp' -o ./build/MW_CpuSpeedController

if (-not$?)
{
    log "构建失败"
    exit
}

log "Frozen编译成功"
<#
.SYNOPSIS
    HubisVIS 장비 배포 패키지 생성 (Release x64 빌드 산출물 수집)

.DESCRIPTION
    두 가지 모드가 있다.

    [업데이트 모드] (기본)
      장비 실행 폴더에 덮어쓸 빌드 산출물만 모은다. Pylon/VC++ 는 장비 설치본을 사용한다.
      포함: HubisVIS.exe, VisionMain.dll, AlgorithmTools.dll (필수)
            ConfigSetup.exe, PlcSetup.exe, VisionSPC.exe, ReleaseNote.txt (있으면)

    [한 폴더 모드] (-Full)
      실행에 필요한 DLL 을 모두 한 폴더에 모은다. Pylon/VC++ 가 설치되지 않은 PC 에서도 이 폴더만으로 실행.
      업데이트 모드 파일 +
        HubisSDK.Library.*.dll (Release, _d 제외)
        Pylon 5 Runtime x64 전체 (PylonBase, GenApi, GigE 전송 계층 PylonGigE_v5_2_TL.dll 등)
        VC++ 런타임 app-local (Microsoft.VC141.CRT / MFC / OpenMP)
        _Install\vc_redist.x64.exe (설치형 VC++ 런타임, 선택 설치용)
        Config 폴더 (-IncludeConfig 일 때만)
      마지막에 패키지 안 모든 exe/dll 의 import 를 PE 헤더에서 읽어,
      Windows 시스템 DLL 이 아닌 의존 DLL 이 패키지 안에 모두 있는지 검사한다.

    공통 제외: *_d.dll, .pdb/.iobj/.ipdb/.lib/.exp, 레시피·이미지·로그

    검사 (하나라도 실패하면 패키지를 만들지 않는다):
      - PE 머신 타입이 x64 인지
      - Debug 런타임(MSVCP140D, VCRUNTIME140D, ucrtbased, mfc140ud, *_d.dll) 참조가 없는지
      - Pylon 참조가 v5_2 인지 (pylon 8 등 다른 버전으로 빌드된 산출물 차단)
      - (-Full) 의존 DLL 이 패키지 안에서 모두 해결되는지
    경고 (패키지는 만든다):
      - 소스 파일이 해당 바이너리보다 나중에 수정됨 (재빌드 누락 의심)
      - (-IncludeConfig) Config.ini 가 Hardware Exist=0 (시뮬레이션 설정)

.PARAMETER ProjectRoot
    P15.ANGT2301_HubisVIS_Line 폴더. 기본값은 이 스크립트의 상위 폴더.

.PARAMETER Full
    한 폴더 모드. 실행에 필요한 DLL 을 모두 포함한다.

.PARAMETER IncludeConfig
    bin\Config 폴더를 포함한다. 장비 설정(카메라 IP, Hardware Exist, PLC)을 덮어쓰므로 주의.

.PARAMETER IncludeHubisSDK
    업데이트 모드에서 HubisSDK.Library.*.dll 도 포함한다. (-Full 은 항상 포함)

.PARAMETER PylonRuntimeDir
    Pylon Runtime 폴더. 기본값 C:\Program Files\Basler\pylon 5\Runtime\x64

.PARAMETER VcRedistDir
    VC++ 재배포 폴더 (Microsoft.VC141.CRT 등이 있는 x64 폴더). 기본값은 vswhere 로 찾은 VS2017 v141.

.PARAMETER NoZip
    zip 파일을 만들지 않는다.

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\Make-DeployPackage.ps1
.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\Make-DeployPackage.ps1 -Full
.EXAMPLE
    powershell -ExecutionPolicy Bypass -File tools\Make-DeployPackage.ps1 -Full -IncludeConfig
#>
[CmdletBinding()]
param(
    [string]$ProjectRoot,
    [string]$OutputRoot,
    [switch]$Full,
    [switch]$IncludeConfig,
    [switch]$IncludeHubisSDK,
    [string]$PylonRuntimeDir = 'C:\Program Files\Basler\pylon 5\Runtime\x64',
    [string]$VcRedistDir,
    [switch]$NoZip
)

$ErrorActionPreference = 'Stop'

# PowerShell 5.1 은 param 기본값에서 $PSScriptRoot 가 비어 있으므로 본문에서 지정
if (-not $ProjectRoot) { $ProjectRoot = Split-Path -Parent $PSScriptRoot }

$BinDir = Join-Path $ProjectRoot 'bin'
if (-not $OutputRoot) { $OutputRoot = Join-Path $ProjectRoot 'Deploy' }

$RequiredFiles = @('HubisVIS.exe', 'VisionMain.dll', 'AlgorithmTools.dll')
$OptionalFiles = @('ConfigSetup.exe', 'PlcSetup.exe', 'VisionSPC.exe', 'ReleaseNote.txt')

# 바이너리별 소스 폴더 (재빌드 누락 경고용)
$SourceMap = @{
    'HubisVIS.exe'       = 'HubisVIS'
    'VisionMain.dll'     = 'VisionMain'
    'AlgorithmTools.dll' = 'AlgorithmTools'
    'ConfigSetup.exe'    = 'ConfigSetup'
    'PlcSetup.exe'       = 'PlcSetup'
    'VisionSPC.exe'      = 'VisionSPC'
}

$DebugRuntimePattern = '(?i)\b(MSVCP1\d\dD|VCRUNTIME1\d\dD|ucrtbased|mfc1\d\du?d|[A-Za-z0-9_.]+_d)\.dll'
$PylonBasePattern    = '(?i)\b(Pylon(Base|Utility|C|GUI)_v\d+_\d+)\.dll'
$ExpectedPylon       = 'v5_2'

# 시스템에 있어도 패키지 안에 있어야 하는 DLL (장비 PC 에 없을 수 있음)
$MustBundlePattern = '(?i)^(msvcp1\d\d.*|vcruntime1\d\d.*|vcomp1\d\d|mfc1\d\d.*|concrt1\d\d|vccorlib1\d\d)\.dll$'

# ---------------------------------------------------------------- PE 유틸
function Get-PeMachine([string]$Path) {
    $fs = [IO.File]::OpenRead($Path)
    try {
        $br = New-Object IO.BinaryReader($fs)
        $fs.Position = 0x3C
        $peOffset = $br.ReadInt32()
        $fs.Position = $peOffset + 4
        return $br.ReadUInt16()
    } finally { $fs.Dispose() }
}

# PE import / delay-import 테이블에서 DLL 이름 목록을 읽는다
function Get-PeImports([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    $pe = [BitConverter]::ToInt32($bytes, 0x3C)
    $numSections = [BitConverter]::ToUInt16($bytes, $pe + 6)
    $optSize = [BitConverter]::ToUInt16($bytes, $pe + 20)
    $opt = $pe + 24
    $magic = [BitConverter]::ToUInt16($bytes, $opt)
    if ($magic -eq 0x20B) { $dirBase = $opt + 112 } else { $dirBase = $opt + 96 }
    $secBase = $opt + $optSize

    $sections = @()
    for ($i = 0; $i -lt $numSections; $i++) {
        $s = $secBase + $i * 40
        $sections += [pscustomobject]@{
            VA   = [BitConverter]::ToUInt32($bytes, $s + 12)
            Size = [Math]::Max([BitConverter]::ToUInt32($bytes, $s + 8), [BitConverter]::ToUInt32($bytes, $s + 16))
            Raw  = [BitConverter]::ToUInt32($bytes, $s + 20)
        }
    }
    $toOffset = {
        param([uint32]$rva)
        foreach ($sec in $sections) {
            if ($rva -ge $sec.VA -and $rva -lt ($sec.VA + $sec.Size)) { return [int]($rva - $sec.VA + $sec.Raw) }
        }
        return -1
    }
    $readName = {
        param([int]$off)
        $end = $off
        while ($end -lt $bytes.Length -and $bytes[$end] -ne 0) { $end++ }
        return [Text.Encoding]::ASCII.GetString($bytes, $off, $end - $off)
    }

    $names = @()
    # 1: Import (descriptor 20 bytes, Name @ +12) / 13: Delay import (descriptor 32 bytes, Name @ +4)
    foreach ($d in @(@{ Index = 1; Size = 20; NameAt = 12 }, @{ Index = 13; Size = 32; NameAt = 4 })) {
        $rva = [BitConverter]::ToUInt32($bytes, $dirBase + $d.Index * 8)
        if ($rva -eq 0) { continue }
        $off = & $toOffset $rva
        if ($off -lt 0) { continue }
        for ($k = 0; $k -lt 4096; $k++) {
            $p = $off + $k * $d.Size
            if ($p + $d.Size -gt $bytes.Length) { break }
            $nameRva = [BitConverter]::ToUInt32($bytes, $p + $d.NameAt)
            if ($nameRva -eq 0) { break }
            $nameOff = & $toOffset $nameRva
            if ($nameOff -ge 0) { $names += (& $readName $nameOff) }
        }
    }
    return $names | Sort-Object -Unique
}

function Test-Binary([string]$Path) {
    $errors = @()
    $name = Split-Path -Leaf $Path

    $machine = Get-PeMachine $Path
    if ($machine -ne 0x8664) { $errors += ('{0}: x64 아님 (machine=0x{1:X4})' -f $name, $machine) }

    # 문자열 검색은 패킹된 DLL(HubisSDK) 안의 데이터까지 잡으므로 실제 import 테이블만 본다
    $imports = Get-PeImports $Path

    $debugRefs = $imports | Where-Object { $_ -match "^$DebugRuntimePattern$" }
    if ($debugRefs) { $errors += ('{0}: Debug 런타임 참조 -> {1}' -f $name, ($debugRefs -join ', ')) }

    $pylonRefs = $imports | Where-Object { $_ -match "^$PylonBasePattern$" }
    $badPylon = $pylonRefs | Where-Object { $_ -notmatch "_$ExpectedPylon\.dll$" }
    if ($badPylon) { $errors += ('{0}: Pylon {1} 이 아닌 참조 -> {2}' -f $name, $ExpectedPylon, ($badPylon -join ', ')) }

    return $errors
}

function Get-NewestSource([string]$Dir) {
    if (-not (Test-Path $Dir)) { return $null }
    Get-ChildItem $Dir -Recurse -File -Include *.cpp, *.h, *.hpp, *.rc, *.rc2 |
        Where-Object { $_.FullName -notmatch '\\(x64|Debug|Release)\\' } |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
}

function Find-VcRedistDir {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { return $null }
    $candidates = @()
    foreach ($inst in (& $vswhere -all -products * -property installationPath)) {
        $candidates += Get-ChildItem (Join-Path $inst 'VC\Redist\MSVC') -Directory -ErrorAction SilentlyContinue |
            ForEach-Object { Join-Path $_.FullName 'x64' } | Where-Object { Test-Path $_ }
    }
    # 빌드 툴셋 v141 우선, 없으면 가장 최신 VC14x
    $v141 = $candidates | Where-Object { Test-Path (Join-Path $_ 'Microsoft.VC141.CRT') } | Select-Object -First 1
    if ($v141) { return $v141 }
    return $candidates | Where-Object { Get-ChildItem $_ -Directory -Filter 'Microsoft.VC14*.CRT' } |
        Sort-Object { [version](Split-Path -Leaf (Split-Path -Parent $_)) } -Descending | Select-Object -First 1
}

# ---------------------------------------------------------------- 빌드 산출물 수집
if (-not (Test-Path $BinDir)) { throw "bin 폴더가 없습니다: $BinDir" }

$files = @()
foreach ($f in $RequiredFiles) {
    $p = Join-Path $BinDir $f
    if (-not (Test-Path $p)) { throw "필수 파일 없음: $p (Release|x64 로 빌드했는지 확인)" }
    $files += Get-Item $p
}
foreach ($f in $OptionalFiles) {
    $p = Join-Path $BinDir $f
    if (Test-Path $p) { $files += Get-Item $p }
}
if ($Full -or $IncludeHubisSDK) {
    $files += Get-ChildItem $BinDir -Filter 'HubisSDK.Library.*.dll' | Where-Object { $_.BaseName -notmatch '_d$' }
}

# ---------------------------------------------------------------- 빌드 산출물 검사
$errors = @()
$warnings = @()
foreach ($file in $files | Where-Object { $_.Extension -in '.exe', '.dll' }) {
    $errors += Test-Binary $file.FullName

    if ($SourceMap.ContainsKey($file.Name)) {
        $src = Get-NewestSource (Join-Path $ProjectRoot $SourceMap[$file.Name])
        if ($src -and $src.LastWriteTime -gt $file.LastWriteTime) {
            $warnings += ('{0} ({1:yyyy-MM-dd HH:mm}) 보다 소스 {2} ({3:yyyy-MM-dd HH:mm}) 가 최신 -> 재빌드 필요 여부 확인' -f `
                $file.Name, $file.LastWriteTime, $src.Name, $src.LastWriteTime)
        }
    }
}

# ---------------------------------------------------------------- 한 폴더 모드: 외부 런타임 확인
$vcDirs = @()
$vcRedistExe = $null
if ($Full) {
    if (-not (Test-Path (Join-Path $PylonRuntimeDir "PylonBase_$ExpectedPylon.dll"))) {
        $errors += "Pylon Runtime 에 PylonBase_$ExpectedPylon.dll 이 없음: $PylonRuntimeDir"
    }
    if (-not (Test-Path (Join-Path $PylonRuntimeDir "PylonGigE_${ExpectedPylon}_TL.dll"))) {
        $errors += "Pylon Runtime 에 GigE 전송 계층 PylonGigE_${ExpectedPylon}_TL.dll 이 없음: $PylonRuntimeDir"
    }

    if (-not $VcRedistDir) { $VcRedistDir = Find-VcRedistDir }
    if (-not $VcRedistDir -or -not (Test-Path $VcRedistDir)) {
        $errors += 'VC++ 재배포 폴더를 찾지 못함. -VcRedistDir 로 지정 (예: ...\VC\Redist\MSVC\14.16.27012\x64)'
    } else {
        $vcDirs = Get-ChildItem $VcRedistDir -Directory | Where-Object { $_.Name -match '^Microsoft\.VC14\d\.(CRT|MFC|OpenMP)$' }
        if (-not ($vcDirs | Where-Object { $_.Name -match 'CRT$' })) { $errors += "VC++ CRT 폴더 없음: $VcRedistDir" }
        if (-not ($vcDirs | Where-Object { $_.Name -match 'MFC$' })) { $errors += "VC++ MFC 폴더 없음: $VcRedistDir" }
        $exe = Join-Path (Split-Path -Parent $VcRedistDir) 'vc_redist.x64.exe'
        if (Test-Path $exe) { $vcRedistExe = $exe }
    }

    if ($IncludeConfig -and -not (Test-Path (Join-Path $BinDir 'Config'))) {
        $errors += "Config 폴더 없음: $(Join-Path $BinDir 'Config')"
    }
}

foreach ($w in $warnings) { Write-Warning $w }
if ($errors) {
    $errors | ForEach-Object { Write-Host "[FAIL] $_" -ForegroundColor Red }
    throw '검사 실패: 배포 패키지를 만들지 않았습니다.'
}

# ---------------------------------------------------------------- 복사
$stamp = Get-Date -Format 'yyyyMMdd_HHmm'
if ($Full) { $pkgName = "HubisVIS_Full_$stamp" } else { $pkgName = "HubisVIS_$stamp" }
$pkgDir = Join-Path $OutputRoot $pkgName
New-Item -ItemType Directory -Force -Path $pkgDir | Out-Null

foreach ($file in $files) { Copy-Item $file.FullName -Destination $pkgDir }

if ($Full) {
    # Pylon Runtime 전체 (전송 계층 TL 은 PylonBase 와 같은 폴더에 있어야 함)
    Get-ChildItem $PylonRuntimeDir -File | Copy-Item -Destination $pkgDir

    # VC++ 런타임 app-local (CRT, MFC, OpenMP)
    foreach ($d in $vcDirs) { Get-ChildItem $d.FullName -File -Filter *.dll | Copy-Item -Destination $pkgDir }

    if ($vcRedistExe) {
        $installDir = Join-Path $pkgDir '_Install'
        New-Item -ItemType Directory -Force -Path $installDir | Out-Null
        Copy-Item $vcRedistExe -Destination $installDir
    }
}

if ($IncludeConfig) {
    Copy-Item (Join-Path $BinDir 'Config') -Destination $pkgDir -Recurse
    $ini = Join-Path $pkgDir 'Config\Config.ini'
    if ((Test-Path $ini) -and (Select-String -Path $ini -Pattern '^\s*Hardware Exist\s*=\s*0' -Quiet)) {
        $warnings += 'Config\Config.ini 가 Hardware Exist=0 (시뮬레이션). 실장비에서는 1 로 바꾸거나 장비 Config 를 사용'
        Write-Warning $warnings[-1]
    }
}

# PLCAutoPass=1 Master 이미지 (카메라 실패 시 대체 검사용): bin\Simulation\{Before|After}\{cam}_{div}_raw.bmp
$simDir = Join-Path $BinDir 'Simulation'
if (Test-Path $simDir) {
    Copy-Item $simDir -Destination $pkgDir -Recurse
} else {
    $warnings += 'bin\Simulation (Master 이미지) 없음. PLCAutoPass=1 카메라 실패 시 대체 검사를 할 수 없음'
    Write-Warning $warnings[-1]
}

# ---------------------------------------------------------------- 한 폴더 모드: 의존 DLL 검사
$missing = @()
if ($Full) {
    $pkgNames = @{}
    Get-ChildItem $pkgDir -File | ForEach-Object { $pkgNames[$_.Name.ToLower()] = $true }
    $sys32 = Join-Path $env:WINDIR 'System32'

    foreach ($bin in Get-ChildItem $pkgDir -File | Where-Object { $_.Extension -in '.exe', '.dll' }) {
        foreach ($imp in Get-PeImports $bin.FullName) {
            $key = $imp.ToLower()
            if ($pkgNames.ContainsKey($key)) { continue }
            if ($key -match '^(api|ext)-ms-') { continue }
            if ($key -notmatch $MustBundlePattern -and (Test-Path (Join-Path $sys32 $imp))) { continue }
            $missing += "$($bin.Name) -> $imp"
        }
    }
    if ($missing) {
        $missing | ForEach-Object { Write-Host "[MISSING] $_" -ForegroundColor Red }
        Remove-Item $pkgDir -Recurse -Force
        throw '의존 DLL 검사 실패: 위 DLL 이 패키지에 없어 패키지를 삭제했습니다.'
    }
}

# ---------------------------------------------------------------- 목록 / 안내
$gitRev = ''
try { $gitRev = (git -C $ProjectRoot rev-parse --short HEAD 2>$null) } catch { }

$lines = @()
if ($Full) { $lines += "HubisVIS 배포 패키지 (한 폴더 모드) $stamp" } else { $lines += "HubisVIS 배포 패키지 (업데이트 모드) $stamp" }
$lines += "Source : $ProjectRoot"
if ($gitRev) { $lines += "Git    : $gitRev" }
if ($Full) {
    $lines += "Pylon  : $PylonRuntimeDir"
    $lines += "VC++   : $VcRedistDir"
}
$lines += ''
$lines += '[파일 목록]'
foreach ($file in Get-ChildItem $pkgDir -File -Recurse | Sort-Object FullName) {
    $rel = $file.FullName.Substring($pkgDir.Length + 1)
    $hash = (Get-FileHash $file.FullName -Algorithm SHA256).Hash.Substring(0, 16)
    $lines += ('{0,-44} {1,-14} {2,11:N0}  {3:yyyy-MM-dd HH:mm}  {4}' -f `
        $rel, $file.VersionInfo.FileVersion, $file.Length, $file.LastWriteTime, $hash)
}
$lines += ''
$lines += '[장비 적용 방법]'
if ($Full) {
    $lines += '1. HubisVIS 종료 후 작업 관리자에서 HubisVIS.exe 가 남아 있지 않은지 확인'
    $lines += '2. 기존 실행 폴더는 그대로 두고, 이 폴더를 새 위치에 통째로 복사 (예: C:\HubisVIS_new)'
    if ($IncludeConfig) {
        $lines += '3. 포함된 Config 폴더 값 확인 (Hardware Exist=1, Camera1 IP=100.100.100.4, Camera2 IP=100.100.100.2, PLC IP)'
    } else {
        $lines += '3. 기존 실행 폴더의 Config 폴더를 이 폴더에 복사 (장비 설정 유지)'
    }
    $lines += '4. 이 폴더의 HubisVIS.exe 실행. 실행 폴더에 Pylon/VC++ DLL 이 모두 있으므로 별도 설치 없이 동작'
    $lines += '   - VC++ 런타임 오류가 나면 _Install\vc_redist.x64.exe 설치'
    $lines += '   - GigE Filter/Performance Driver 는 파일 복사로 설치되지 않음. 영상 누락 시 장비에 pylon 5 설치'
    $lines += '5. 레시피는 D:\HubisVIS\Recipe 를 사용 (패키지에 포함하지 않음)'
    $lines += '6. 카메라 강제 종료 직후라면 5초 이상 기다린 뒤 실행 (GigE heartbeat 5000 ms)'
} else {
    $lines += '1. HubisVIS 종료 후 작업 관리자에서 HubisVIS.exe 가 남아 있지 않은지 확인'
    $lines += '2. 장비 실행 폴더의 기존 파일을 백업 폴더로 복사'
    $lines += '3. 이 폴더의 파일만 장비 실행 폴더에 덮어쓰기'
    $lines += '4. Pylon DLL 은 넣지 않는다 (장비에 설치된 pylon 5 Runtime 사용)'
    $lines += '   - 실행 폴더에 일부 Pylon DLL 만 있으면 GigE 전송 계층을 못 찾아 카메라 Open 실패 -> 백업 폴더로 옮긴다'
    $lines += '5. Config 폴더는 장비 것을 유지 (Hardware Exist=1, Camera1 IP=100.100.100.4, Camera2 IP=100.100.100.2)'
    $lines += '6. 카메라 강제 종료 직후라면 5초 이상 기다린 뒤 실행 (GigE heartbeat 5000 ms)'
}
if ($warnings) {
    $lines += ''
    $lines += '[경고]'
    $lines += $warnings
}

$manifest = Join-Path $pkgDir 'DEPLOY_INFO.txt'
$lines | Out-File -FilePath $manifest -Encoding UTF8

if (-not $NoZip) {
    $zip = "$pkgDir.zip"
    if (Test-Path $zip) { Remove-Item $zip -Force }
    Compress-Archive -Path (Join-Path $pkgDir '*') -DestinationPath $zip
}

$count = (Get-ChildItem $pkgDir -File -Recurse).Count
$sizeMB = [Math]::Round(((Get-ChildItem $pkgDir -File -Recurse | Measure-Object Length -Sum).Sum / 1MB), 1)
Write-Host ''
Write-Host "배포 패키지 생성 완료: $pkgDir ($count 개 파일, $sizeMB MB)" -ForegroundColor Green
if ($Full) { Write-Host '의존 DLL 검사 통과: 모든 비시스템 DLL 이 패키지 안에 있음' -ForegroundColor Green }
if (-not $NoZip) { Write-Host "ZIP: $pkgDir.zip" -ForegroundColor Green }
Write-Host "목록: $manifest"

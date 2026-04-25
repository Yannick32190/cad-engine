# =============================================================================
# CAD-ENGINE - Script de compilation Windows
# Usage : powershell -ExecutionPolicy Bypass -File build-windows.ps1
# Options :
#   -Clean    Supprime le dossier build avant de recompiler
#   -NoDist   Ne genere pas le dossier de distribution final
# =============================================================================
param(
    [switch]$Clean,
    [switch]$NoDist
)

$ErrorActionPreference = "Stop"

# -- Chemins ------------------------------------------------------------------
$CAD_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path
$MSYS2   = "C:\msys64"
$MINGW   = "$MSYS2\mingw64"
$PACMAN  = "$MSYS2\usr\bin\pacman.exe"
$CMAKE   = "$MINGW\bin\cmake.exe"
$BUILD   = "$CAD_DIR\build-mingw"

# Lire la version depuis CMakeLists.txt
$version = "0.0.0"
$cmakeContent = Get-Content "$CAD_DIR\CMakeLists.txt" -Raw -Encoding UTF8
if ($cmakeContent -match 'project\([^)]*VERSION\s+([\d.]+)') {
    $version = $matches[1]
}
$DIST = "$CAD_DIR\CAD-ENGINE-v$version-win64"

# -- Helpers ------------------------------------------------------------------
function Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }
function OK($msg)   { Write-Host "    OK  $msg" -ForegroundColor Green }
function Warn($msg) { Write-Host "    !!  $msg" -ForegroundColor Yellow }
function Fail($msg) { Write-Host "`n[ERREUR] $msg" -ForegroundColor Red; exit 1 }

# =============================================================================
# 1. Verification MSYS2
# =============================================================================
Step "Verification MSYS2..."
if (-not (Test-Path "$MSYS2\usr\bin\bash.exe")) {
    Warn "MSYS2 absent - installation via winget..."
    winget install --id MSYS2.MSYS2 --silent --accept-package-agreements --accept-source-agreements
    if (-not (Test-Path "$MSYS2\usr\bin\bash.exe")) {
        Fail "Impossible d'installer MSYS2. Installez-le depuis https://www.msys2.org/"
    }
}
OK "MSYS2 : $MSYS2"
$env:PATH = "$MINGW\bin;$MSYS2\usr\bin;$env:PATH"

# =============================================================================
# 2. Verification des paquets MSYS2
# =============================================================================
Step "Verification des dependances (Qt6, GCC, OCCT, CMake)..."

$packages = @(
    "mingw-w64-x86_64-qt6-base",
    "mingw-w64-x86_64-qt6-tools",
    "mingw-w64-x86_64-opencascade",
    "mingw-w64-x86_64-gcc",
    "mingw-w64-x86_64-cmake",
    "mingw-w64-x86_64-ninja"
)

$missing = @()
$prevEAP = $ErrorActionPreference
$ErrorActionPreference = "SilentlyContinue"
foreach ($pkg in $packages) {
    $null = & $PACMAN -Q $pkg 2>&1
    if ($LASTEXITCODE -ne 0) { $missing += $pkg }
}
$ErrorActionPreference = $prevEAP

if ($missing.Count -gt 0) {
    Warn "Paquets manquants : $($missing -join ', ')"
    Write-Host "    Mise a jour de la base de donnees pacman..." -ForegroundColor Yellow
    & $PACMAN -Sy --noconfirm 2>&1 | Out-Null
    Write-Host "    Installation en cours (peut prendre 10-20 min)..." -ForegroundColor Yellow
    & $PACMAN -S --noconfirm --needed @missing 2>&1 | Select-Object -Last 5
    if ($LASTEXITCODE -ne 0) { Fail "Echec de l'installation des dependances" }
}

if (-not (Test-Path $CMAKE)) { Fail "cmake introuvable : $CMAKE" }
OK "CMake : $(& $CMAKE --version | Select-Object -First 1)"
OK "GCC   : $(& "$MINGW\bin\gcc.exe" --version | Select-Object -First 1)"

# =============================================================================
# 3. Configuration CMake
# =============================================================================
Step "Configuration CMake..."

if ($Clean -and (Test-Path $BUILD)) {
    Remove-Item $BUILD -Recurse -Force
    OK "Dossier build supprime (-Clean)"
}
if (-not (Test-Path $BUILD)) { New-Item -ItemType Directory -Path $BUILD | Out-Null }

& $CMAKE `
    -S $CAD_DIR `
    -B $BUILD `
    -G "Ninja" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$MINGW" `
    -DBUILD_TESTS=OFF `
    -DBUILD_EXAMPLES=OFF `
    -DCMAKE_CXX_COMPILER="$MINGW\bin\g++.exe" 2>&1 | Where-Object { $_ -match "^--" } | Select-Object -Last 8

if ($LASTEXITCODE -ne 0) { Fail "Echec de la configuration CMake" }
OK "CMake configure"

# =============================================================================
# 4. Compilation
# =============================================================================
$cores = (Get-CimInstance Win32_Processor -ErrorAction SilentlyContinue).NumberOfLogicalProcessors
if (-not $cores) { $cores = 4 }

Step "Compilation sur $cores coeurs..."
$sw = [System.Diagnostics.Stopwatch]::StartNew()

& $CMAKE --build $BUILD --config Release --parallel $cores 2>&1

if ($LASTEXITCODE -ne 0) { Fail "Echec de la compilation" }

$sw.Stop()
$elapsed = $sw.Elapsed.ToString("mm\:ss")
OK "Compile en $elapsed"

if ($NoDist) {
    Write-Host "`n[OK] Build termine. Executable : $BUILD\cad-engine.exe" -ForegroundColor Green
    exit 0
}

# =============================================================================
# 5. Dossier de distribution
# =============================================================================
Step "Generation du package de distribution..."

if (Test-Path $DIST) { Remove-Item $DIST -Recurse -Force }
New-Item -ItemType Directory -Path $DIST | Out-Null

# Executable
Copy-Item "$BUILD\cad-engine.exe" $DIST

# Plugins Qt via windeployqt6
$wdq = "$MINGW\bin\windeployqt6.exe"
if (Test-Path $wdq) {
    try {
        $null = & $wdq "$DIST\cad-engine.exe" --no-translations --no-system-d3d-compiler 2>&1
    } catch { <# warnings non bloquants #> }
    OK "Plugins Qt deployes via windeployqt6"
}

# Copie des plugins Qt depuis mingw64 (garantit platforms/qwindows.dll)
$qtPluginSrc = "$MINGW\share\qt6\plugins"
if (-not (Test-Path $qtPluginSrc)) { $qtPluginSrc = "$MINGW\plugins" }
foreach ($dir in @("platforms","styles","imageformats","tls","generic","networkinformation")) {
    $src = "$qtPluginSrc\$dir"
    if (Test-Path $src) {
        Copy-Item $src $DIST -Recurse -Force
    }
}
if (Test-Path "$DIST\platforms\qwindows.dll") {
    OK "Plugin platforms/qwindows.dll present"
} else {
    Warn "ATTENTION: platforms/qwindows.dll absent - l'application ne demarrera pas !"
}

# DLL necessaires - detection automatique via ntldd
$ntldd = "$MINGW\bin\ntldd.exe"
if (-not (Test-Path $ntldd)) {
    & $PACMAN -S --noconfirm --needed mingw-w64-x86_64-ntldd 2>&1 | Out-Null
}

if (Test-Path $ntldd) {
    $raw = & $ntldd -R "$BUILD\cad-engine.exe" 2>&1
    $dllNames = $raw | ForEach-Object {
        if ($_ -match "^\s+(\S+\.dll)\s+=>" -and $_ -notmatch "Windows\\|SYSTEM32|not found") {
            $dll = $matches[1]
            if (Test-Path "$MINGW\bin\$dll") { $dll }
        }
    } | Sort-Object -Unique

    foreach ($dll in $dllNames) {
        Copy-Item "$MINGW\bin\$dll" $DIST -Force
    }
    OK "$($dllNames.Count) DLL copiees (detection automatique)"
} else {
    # Fallback liste fixe
    $fallback = @(
        "libb2-1.dll","libbrotlicommon.dll","libbrotlidec.dll","libbz2-1.dll",
        "libdouble-conversion.dll","libfreetype-6.dll","libgcc_s_seh-1.dll",
        "libglib-2.0-0.dll","libgraphite2.dll","libharfbuzz-0.dll",
        "libiconv-2.dll","libicudt78.dll","libicuin78.dll","libicuuc78.dll",
        "libintl-8.dll","libmd4c.dll","libpcre2-16-0.dll","libpcre2-8-0.dll",
        "libpng16-16.dll","libstdc++-6.dll","libtbb12.dll","libwinpthread-1.dll",
        "libzstd.dll","zlib1.dll",
        "libTKBO.dll","libTKBool.dll","libTKBRep.dll","libTKCDF.dll","libTKernel.dll",
        "libTKFillet.dll","libTKG2d.dll","libTKG3d.dll","libTKGeomAlgo.dll",
        "libTKGeomBase.dll","libTKLCAF.dll","libTKMath.dll","libTKMesh.dll",
        "libTKOffset.dll","libTKPrim.dll","libTKShHealing.dll","libTKTopAlgo.dll"
    )
    foreach ($dll in $fallback) {
        $src = "$MINGW\bin\$dll"
        if (Test-Path $src) { Copy-Item $src $DIST -Force }
    }
    OK "$($fallback.Count) DLL copiees (liste fixe)"
}


# Verification finale
if (-not (Test-Path "$DIST\cad-engine.exe")) { Fail "Executable absent du dossier dist" }

$totalMB  = [math]::Round((Get-ChildItem $DIST -Recurse | Measure-Object Length -Sum).Sum / 1MB, 0)
$fileCount = (Get-ChildItem $DIST -Recurse -File).Count

# =============================================================================
# Resume
# =============================================================================
Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host "  CAD-ENGINE v$version - Build termine !"   -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host "  Exe    : $DIST\cad-engine.exe"
Write-Host "  Dist   : $DIST"
Write-Host "  Taille : $fileCount fichiers / $totalMB MB"
Write-Host "  Duree  : $elapsed"
Write-Host "============================================" -ForegroundColor Green

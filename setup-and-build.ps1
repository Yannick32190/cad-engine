# CAD-ENGINE Windows Setup & Build Script
# Requires: Windows 10/11 64-bit, winget, internet connection
# Runs with: powershell -ExecutionPolicy Bypass -File setup-and-build.ps1

$ErrorActionPreference = "Stop"
$CAD_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path
$MSYS2_ROOT = "C:\msys64"
$MINGW = "$MSYS2_ROOT\mingw64"

function Write-Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }
function Write-OK($msg)   { Write-Host "    [OK] $msg" -ForegroundColor Green }
function Write-Fail($msg) { Write-Host "    [!!] $msg" -ForegroundColor Red; exit 1 }

# ============================================================================
# 1. Installer MSYS2 si absent
# ============================================================================
Write-Step "Verification MSYS2..."
if (-not (Test-Path "$MSYS2_ROOT\usr\bin\bash.exe")) {
    Write-Host "    MSYS2 non trouve, installation via winget..." -ForegroundColor Yellow
    winget install --id MSYS2.MSYS2 --location C:\msys64 --silent --accept-package-agreements --accept-source-agreements
    if (-not (Test-Path "$MSYS2_ROOT\usr\bin\bash.exe")) {
        Write-Fail "Echec installation MSYS2. Installez manuellement depuis https://www.msys2.org/"
    }
    Write-OK "MSYS2 installe"
} else {
    Write-OK "MSYS2 present: $MSYS2_ROOT"
}

# ============================================================================
# 2. Mettre a jour MSYS2 et installer les dependances
# ============================================================================
Write-Step "Mise a jour MSYS2 et installation des dependances (cela peut prendre 10-20 min)..."

$PACMAN = "$MSYS2_ROOT\usr\bin\pacman.exe"

# Update first
& $PACMAN -Syu --noconfirm 2>&1 | Select-Object -Last 5

# Check if Qt6 already installed
$qt6Installed = & $PACMAN -Q mingw-w64-x86_64-qt6-base 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "    Installation Qt6..." -ForegroundColor Yellow
    & $PACMAN -S --noconfirm --needed `
        mingw-w64-x86_64-qt6-base `
        mingw-w64-x86_64-qt6-tools `
        mingw-w64-x86_64-qt6-declarative
    Write-OK "Qt6 installe"
} else {
    Write-OK "Qt6 deja present: $qt6Installed"
}

# Check if OpenCASCADE already installed
$occInstalled = & $PACMAN -Q mingw-w64-x86_64-opencascade 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "    Installation OpenCASCADE (peut prendre 5-10 min)..." -ForegroundColor Yellow
    & $PACMAN -S --noconfirm --needed mingw-w64-x86_64-opencascade
    Write-OK "OpenCASCADE installe"
} else {
    Write-OK "OpenCASCADE deja present: $occInstalled"
}

# Install cmake and gcc if needed
& $PACMAN -S --noconfirm --needed `
    mingw-w64-x86_64-cmake `
    mingw-w64-x86_64-gcc `
    mingw-w64-x86_64-ninja `
    mingw-w64-x86_64-pkg-config

# ============================================================================
# 3. Verifier les chemins
# ============================================================================
Write-Step "Verification des outils..."

$CMAKE  = "$MINGW\bin\cmake.exe"
$GCC    = "$MINGW\bin\gcc.exe"
$QT6DIR = "$MINGW\lib\cmake\Qt6"

if (-not (Test-Path $CMAKE))  { Write-Fail "cmake introuvable: $CMAKE" }
if (-not (Test-Path $GCC))    { Write-Fail "gcc introuvable: $GCC" }
if (-not (Test-Path $QT6DIR)) { Write-Fail "Qt6 cmake config introuvable: $QT6DIR" }

Write-OK "cmake: $(& $CMAKE --version | Select-Object -First 1)"
Write-OK "gcc:   $(& $GCC --version | Select-Object -First 1)"
Write-OK "Qt6:   $QT6DIR"

# ============================================================================
# 4. Configurer et compiler
# ============================================================================
Write-Step "Configuration CMake..."

$BUILD_DIR = "$CAD_DIR\build-mingw"
if (Test-Path $BUILD_DIR) {
    Remove-Item $BUILD_DIR -Recurse -Force
}
New-Item -ItemType Directory -Path $BUILD_DIR | Out-Null

$env:PATH = "$MINGW\bin;$MSYS2_ROOT\usr\bin;$env:PATH"

& $CMAKE `
    -S $CAD_DIR `
    -B $BUILD_DIR `
    -G "Ninja" `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$MINGW" `
    -DBUILD_TESTS=OFF `
    -DBUILD_EXAMPLES=OFF `
    -DCMAKE_C_COMPILER="$MINGW\bin\gcc.exe" `
    -DCMAKE_CXX_COMPILER="$MINGW\bin\g++.exe"

if ($LASTEXITCODE -ne 0) { Write-Fail "Echec de la configuration CMake" }
Write-OK "CMake configure"

Write-Step "Compilation (cela peut prendre 5-15 min selon votre CPU)..."

$CORES = (Get-CimInstance Win32_Processor).NumberOfLogicalProcessors
& $CMAKE --build $BUILD_DIR --config Release --parallel $CORES

if ($LASTEXITCODE -ne 0) { Write-Fail "Echec de la compilation" }

# ============================================================================
# 5. Deployer les DLL (windeployqt)
# ============================================================================
Write-Step "Deploiement des DLL Qt..."

$EXE = "$BUILD_DIR\cad-engine.exe"
if (-not (Test-Path $EXE)) {
    Write-Fail "Executable non trouve: $EXE"
}

$WINDEPLOYQT = "$MINGW\bin\windeployqt6.exe"
if (Test-Path $WINDEPLOYQT) {
    & $WINDEPLOYQT $EXE --no-translations
    Write-OK "DLL Qt deployees"
}

# Copier les DLL OCCT manquantes
$OCC_DLLS = @("TKernel","TKMath","TKG2d","TKG3d","TKGeomBase","TKGeomAlgo","TKBRep",
              "TKTopAlgo","TKPrim","TKShHealing","TKMesh","TKBO","TKBool","TKFillet",
              "TKOffset","TKFeat","TKCDF","TKCAF","TKLCAF","TKBin","TKBinL")
foreach ($dll in $OCC_DLLS) {
    $src = "$MINGW\bin\$dll.dll"
    if (Test-Path $src) {
        Copy-Item $src $BUILD_DIR -Force
    }
}
Write-OK "DLL OCCT copiees"

# ============================================================================
# Resultat
# ============================================================================
Write-Host "`n" -NoNewline
Write-Host "============================================" -ForegroundColor Green
Write-Host " BUILD REUSSI!" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host " Executable: $EXE" -ForegroundColor White
Write-Host " Lancer:     .\build-mingw\cad-engine.exe" -ForegroundColor White
Write-Host "============================================" -ForegroundColor Green

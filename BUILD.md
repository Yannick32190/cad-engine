# CAD-ENGINE v0.8 — Guide de compilation

## Dépendances

| Composant | Version | Lien |
|-----------|---------|------|
| CMake | 3.16+ | https://cmake.org |
| Qt6 | 6.4+ | https://www.qt.io/download-qt-installer |
| OpenCASCADE | 7.9.x | https://dev.opencascade.org/release |
| OpenGL | 3.3+ | (driver GPU) |
| Compilateur C++17 | GCC 10+ / MSVC 2022 | — |

---

## Linux (Ubuntu 22.04+ / Mint 21+ / Debian 12+)

### 1. Installer les dépendances système

```bash
# Outils de build
sudo apt update
sudo apt install build-essential cmake git wget

# Qt6
sudo apt install qt6-base-dev qt6-base-dev-tools libqt6opengl6-dev \
                 qt6-tools-dev libqt6openglwidgets6

# OpenGL
sudo apt install libgl1-mesa-dev libglu1-mesa-dev libglx-dev

# Dépendances OCCT (compilation)
sudo apt install libfreetype-dev tcl-dev tk-dev libfreeimage-dev \
                 librapidjson-dev
```

### 2. Compiler OpenCASCADE 7.9.x (si pas installé)

```bash
# Télécharger les sources
wget https://git.dev.opencascade.org/repos/occt.git/snapshot/occt-V7_9_0.tar.gz
tar xzf occt-V7_9_0.tar.gz
cd occt-V7_9_0

# Compiler
mkdir build && cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DUSE_FREETYPE=ON \
    -DUSE_RAPIDJSON=ON \
    -DBUILD_MODULE_Draw=OFF \
    -DBUILD_MODULE_Visualization=OFF

make -j$(nproc)
sudo make install
sudo ldconfig
```

### 3. Compiler CAD-ENGINE

```bash
cd cad-engine
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Lancer
./cad-engine
```

### 4. Créer l'AppImage portable

```bash
chmod +x packaging/linux/build-appimage.sh
./packaging/linux/build-appimage.sh

# Résultat: CAD-ENGINE-v0.8.0-x86_64.AppImage
# (exécutable unique, aucune installation requise)
```

---

## Windows 10/11 (64-bit)

### 1. Installer les prérequis

**Visual Studio 2022 Community** (gratuit)
- https://visualstudio.microsoft.com/
- Cocher "Développement Desktop C++"

**CMake 3.16+**
- https://cmake.org/download/
- Cocher "Ajouter au PATH"

**Qt6 6.4+**
- https://www.qt.io/download-qt-installer
- Installer la version MSVC 2022 64-bit
- Définir la variable d'environnement :
```powershell
[Environment]::SetEnvironmentVariable("QT6_DIR", "C:\Qt\6.7.0\msvc2022_64", "User")
```

**OpenCASCADE 7.9.x**
- https://dev.opencascade.org/release
- Télécharger le package Windows pré-compilé ou compiler depuis les sources
- Définir :
```powershell
[Environment]::SetEnvironmentVariable("OCCT_DIR", "C:\OpenCASCADE-7.9.0", "User")
```

### 2. Compiler CAD-ENGINE

**Option A : Ligne de commande (Developer PowerShell)**
```powershell
cd cad-engine
mkdir build
cd build

cmake .. -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_PREFIX_PATH="%QT6_DIR%" `
    -DCMAKE_BUILD_TYPE=Release `
    -DBUILD_TESTS=OFF

cmake --build . --config Release --parallel
```

**Option B : Visual Studio**
1. Ouvrir VS2022 → Fichier → Ouvrir → CMake → `CMakeLists.txt`
2. Choisir preset "x64-Release"
3. Build → Build All

### 3. Créer le package portable

```powershell
# Ouvrir PowerShell en tant qu'admin (Developer PowerShell)
cd cad-engine
.\packaging\windows\build-windows.ps1

# Résultat: CAD-ENGINE-v0.8.0-win64-portable.zip
```

---

## Structure du package portable

### Linux (AppImage / tar.gz)
```
CAD-ENGINE-v0.8.0/
├── AppRun                  # Point d'entrée
├── cad-engine.desktop
├── cad-engine.png
└── usr/
    ├── bin/cad-engine       # Exécutable
    ├── lib/                 # Qt6 + OCCT + système
    └── plugins/platforms/   # Qt plugins
```

### Windows (ZIP)
```
CAD-ENGINE-v0.8.0/
├── cad-engine.exe          # Exécutable
├── README.txt
├── Qt6Core.dll             # Qt6
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── Qt6OpenGL.dll
├── Qt6OpenGLWidgets.dll
├── TKernel.dll             # OpenCASCADE
├── TKMath.dll
├── ...
├── platforms/
│   └── qwindows.dll        # Qt platform plugin
└── styles/
    └── qmodernwindowsstyle.dll
```

---

## Raccourcis clavier

| Mode 3D | Mode Sketch |
|---------|-------------|
| **E** — Extrusion | **L** — Ligne |
| **R** — Révolution | **C** — Cercle |
| **Ctrl+Z** — Annuler | **R** — Rectangle |
| **Ctrl+Y** — Rétablir | **E** — Ellipse |
| | **A** — Arc Bézier |
| | **Shift+A** — Arc Centre |
| | **P** — Polyligne |
| | **O** — Oblong |
| | **N** — Polygone |
| | **✓** / **Espace** — Finir sketch |

---

## Dépannage

**Linux : "libTKernel.so not found"**
```bash
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
sudo ldconfig
```

**Linux : AppImage ne démarre pas**
```bash
chmod +x CAD-ENGINE-*.AppImage
# Si FUSE n'est pas installé:
./CAD-ENGINE-*.AppImage --appimage-extract
./squashfs-root/AppRun
```

**Windows : "Qt6Core.dll introuvable"**
→ Vérifier que toutes les DLL sont dans le même répertoire que l'exe.
→ Relancer `windeployqt6` sur l'exe.

**Windows : écran noir / pas de rendu 3D**
→ Mettre à jour les drivers GPU (OpenGL 3.3 minimum).
→ Tester avec la variable `QT_OPENGL=desktop`.

**Les deux : extrusion échoue**
→ Vérifier que les profils forment des contours fermés.
→ Les formes imbriquées (trous) nécessitent que le profil intérieur soit entièrement à l'intérieur du profil extérieur.

# CAD-ENGINE v0.8

Modeleur CAD 3D paramétrique open-source — C++17 / Qt6 / OpenCASCADE

![C++17](https://img.shields.io/badge/C++-17-blue) ![Qt6](https://img.shields.io/badge/Qt-6.4+-green) ![OpenCASCADE](https://img.shields.io/badge/OCCT-7.9-orange) ![License](https://img.shields.io/badge/License-MIT-yellow)

---

## Installation rapide (testeurs Linux)

### Prérequis — une seule commande :

```bash
sudo apt update && sudo apt install -y \
  build-essential cmake git wget \
  qt6-base-dev qt6-base-dev-tools libqt6opengl6-dev libqt6openglwidgets6 qt6-tools-dev \
  libgl1-mesa-dev libglu1-mesa-dev libglx-dev \
  libfreetype-dev tcl-dev tk-dev libfreeimage-dev librapidjson-dev
```

### OpenCASCADE 7.9 (si pas déjà installé) :

```bash
wget https://git.dev.opencascade.org/repos/occt.git/snapshot/occt-V7_9_0.tar.gz
tar xzf occt-V7_9_0.tar.gz && cd occt-V7_9_0
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DUSE_FREETYPE=ON -DUSE_RAPIDJSON=ON -DBUILD_MODULE_Draw=OFF
make -j$(nproc)
sudo make install
sudo ldconfig
cd ../..
```

### Compiler et lancer CAD-ENGINE :

```bash
git clone https://github.com/Yannick32190/cad-engine.git
cd cad-engine
chmod +x build.sh
./build.sh
cd build
./cad-engine
```

Pour recompiler après un `git pull` :

```bash
cd build && cmake --build . -j$(nproc) && ./cad-engine
```

---

## Fonctionnalités v0.8

### Sketch 2D
- **9 outils** : ligne, cercle, rectangle, arc Bézier, arc centre, ellipse, polyligne, polygone, oblong
- **Cotations** éditables : linéaire, radiale, angulaire, diamètre
- **Contraintes** : horizontale, verticale, tangente, coïncidence, symétrie (centrage sur axe)
- **Axes de construction** automatiques (y compris sketch sur face)
- **Grille** adaptative 1-2-5 avec snap magnétique (touche G pour activer/désactiver)

### Opérations 3D
- **Extrusion** : un côté, deux côtés, symétrique — Join / Cut / Intersect
- **Révolution** : angle libre
- **Congé 3D** et **Chanfrein 3D** sur arêtes
- **Balayage** (Sweep) et **Loft**
- **Coque (Shell)** : évidement vers l'intérieur ou l'extérieur
- **Filetage** cosmétique sur faces cylindriques

### Réseaux (Patterns)
- **Réseau linéaire** : 6 directions (X±, Y±, Z±), symétrique, **2 axes pour grille**
- **Réseau circulaire** : 6 axes, angle total libre
- **Preview live** semi-transparent pendant l'édition (style Fusion 360)
- Duplique la **dernière opération** (pas le body complet) — Cut/Join respecté

### Interface
- **Sketch sur face** existante (cliquer une face → sketch dessus)
- **ViewCube** pour navigation rapide
- **Hover de face** permanent en mode 3D (vert au survol)
- **4 thèmes** : sombre, clair, bleu, graphite
- **Undo/Redo** complet en sketch
- **Sauvegarde/Ouverture** format `.cadengine` (JSON)
- **Export STL** binaire

---

## Raccourcis clavier

### Mode 3D

| Touche | Action |
|--------|--------|
| **E** | Extrusion |
| **R** | Révolution |
| **Ctrl+Z** | Annuler |
| **Ctrl+Y** | Rétablir |
| **Ctrl+S** | Enregistrer |
| **Ctrl+O** | Ouvrir |
| **Clic droit** | Rotation vue |
| **Clic milieu** | Pan |
| **Molette** | Zoom |

### Mode Sketch

| Touche | Action |
|--------|--------|
| **L** | Ligne |
| **C** | Cercle |
| **R** | Rectangle |
| **E** | Ellipse |
| **A** | Arc Bézier |
| **Shift+A** | Arc Centre |
| **P** | Polyligne |
| **O** | Oblong |
| **N** | Polygone |
| **G** | Activer/désactiver grille snap |
| **Suppr** | Supprimer entité sélectionnée |
| **Espace** | Valider et quitter le sketch |
| **Échap** | Annuler l'outil en cours |

---

## Signaler un bug

Utilisez l'onglet [Issues](https://github.com/Yannick32190/cad-engine/issues) sur GitHub pour signaler un bug ou proposer une amélioration.

Décrivez :
1. Ce que vous avez fait (étapes)
2. Ce qui s'est passé
3. Ce que vous attendiez
4. La sortie terminal s'il y a un message d'erreur

---

## Structure du projet

```
cad-engine/
├── src/
│   ├── core/               # Document, commandes undo/redo, features
│   ├── features/
│   │   ├── sketch/          # Sketch2D, entités, contraintes, cotations
│   │   ├── operations/      # Extrude, Revolve, Fillet, Chamfer, Shell, Sweep, Loft, Patterns, Thread
│   │   └── primitives/      # Box, Cylinder
│   └── ui/
│       ├── mainwindow/      # MainWindow (~6000 lignes)
│       ├── viewport/        # Viewport3D OpenGL (~7500 lignes)
│       ├── widgets/         # ViewCube
│       ├── tree/            # DocumentTree
│       ├── icons/           # IconProvider (icônes vectorielles)
│       └── theme/           # ThemeManager (4 thèmes)
├── tests/                   # Tests unitaires
├── packaging/               # AppImage Linux, NSIS Windows
├── CMakeLists.txt
├── build.sh
└── README.md
```

**25 000+ lignes** — 76 fichiers source

---

## Licence

MIT — Voir [LICENSE](LICENSE)

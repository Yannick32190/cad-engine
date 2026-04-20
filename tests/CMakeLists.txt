cmake_minimum_required(VERSION 3.16)

# Ajouter le répertoire des bibliothèques OCCT
link_directories(/usr/local/lib)

# Définir les bibliothèques OCCT dans l'ordre INVERSE des dépendances
# (les plus bas niveau en DERNIER pour le linker)
set(OCCT_LIBS
    # OCAF (Application Framework) - haut niveau
    TKBin
    TKBinL
    TKLCAF
    TKCAF
    TKCDF
    # Features avancées
    TKFeat
    TKOffset
    TKFillet
    TKBO
    TKBool
    # Mesh et healing
    TKMesh
    TKShHealing
    # Primitives
    TKPrim
    # Algorithmes topologiques
    TKTopAlgo
    TKGeomAlgo
    # Représentation B-Rep
    TKBRep
    # Géométrie de base
    TKGeomBase
    TKG3d
    TKG2d
    # Foundation - BAS NIVEAU EN DERNIER!
    TKMath
    TKernel
)

# Test 1: Validation environnement OCCT + Qt6
add_executable(test_environment test_environment.cpp)

target_include_directories(test_environment PRIVATE
    /usr/local/include/opencascade
)

target_link_libraries(test_environment
    ${OCCT_LIBS}
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    OpenGL::GL
    pthread
    dl
)

# Test 2: Création primitive simple (à venir)
# add_executable(test_primitives test_primitives.cpp)

add_test(NAME EnvironmentTest COMMAND test_environment)

# Test 2: Primitives (BoxFeature, CylinderFeature)
add_executable(test_primitives test_primitives.cpp
    ../src/core/parametric/Feature.cpp
    ../src/core/document/CADDocument.cpp
    ../src/features/primitives/BoxFeature.cpp
    ../src/features/primitives/CylinderFeature.cpp
)

target_include_directories(test_primitives PRIVATE
    /usr/local/include/opencascade
    ../src
)

target_link_libraries(test_primitives
    ${OCCT_LIBS}
    Qt6::Core
    pthread
    dl
)

add_test(NAME PrimitivesTest COMMAND test_primitives)

# Test 3: Sketch Foundation
add_executable(test_sketch_foundation test_sketch_foundation.cpp
    ../src/core/parametric/Feature.cpp
    ../src/core/document/CADDocument.cpp
    ../src/features/primitives/BoxFeature.cpp
    ../src/features/primitives/CylinderFeature.cpp
    ../src/features/sketch/SketchEntity.cpp
    ../src/features/sketch/Sketch2D.cpp
    ../src/features/sketch/Dimension.cpp
    ../src/features/sketch/SketchFeature.cpp
)

target_include_directories(test_sketch_foundation PRIVATE
    /usr/local/include/opencascade
    ../src
)

target_link_libraries(test_sketch_foundation
    ${OCCT_LIBS}
    Qt6::Core
    pthread
    dl
)

add_test(NAME SketchFoundationTest COMMAND test_sketch_foundation)

# Test 4: Viewport Sketch Mode (ray-casting)
add_executable(test_viewport_sketch test_viewport_sketch.cpp
    ../src/core/parametric/Feature.cpp
    ../src/core/document/CADDocument.cpp
    ../src/features/sketch/SketchEntity.cpp
    ../src/features/sketch/Sketch2D.cpp
    ../src/features/sketch/Dimension.cpp
    ../src/features/sketch/SketchFeature.cpp
)

target_include_directories(test_viewport_sketch PRIVATE
    /usr/local/include/opencascade
    ../src
)

target_link_libraries(test_viewport_sketch
    ${OCCT_LIBS}
    Qt6::Core
    pthread
    dl
)

add_test(NAME ViewportSketchTest COMMAND test_viewport_sketch)

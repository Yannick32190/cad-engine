cmake_minimum_required(VERSION 3.16)
project(cad-engine VERSION 0.8.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Options de build
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(PORTABLE_BUILD "Build portable package" OFF)

# ============================================================================
# Find Qt6
# ============================================================================
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL OpenGLWidgets PrintSupport)

# ============================================================================
# Find OpenCASCADE 7.9.x
# ============================================================================
set(CMAKE_PREFIX_PATH "/usr/local" ${CMAKE_PREFIX_PATH})
find_package(OpenCASCADE QUIET)

if(NOT OpenCASCADE_FOUND)
    message(STATUS "OpenCASCADE not found via find_package, manual config...")
    
    if(WIN32)
        # Windows: default OCCT install paths
        if(DEFINED ENV{OCCT_DIR})
            set(OpenCASCADE_INCLUDE_DIR "$ENV{OCCT_DIR}/inc")
            set(OpenCASCADE_LIBRARY_DIR "$ENV{OCCT_DIR}/win64/vc14/lib")
            set(OpenCASCADE_BIN_DIR "$ENV{OCCT_DIR}/win64/vc14/bin")
        else()
            set(OpenCASCADE_INCLUDE_DIR "C:/OpenCASCADE/inc")
            set(OpenCASCADE_LIBRARY_DIR "C:/OpenCASCADE/win64/vc14/lib")
            set(OpenCASCADE_BIN_DIR "C:/OpenCASCADE/win64/vc14/bin")
        endif()
    else()
        # Linux
        set(OpenCASCADE_INCLUDE_DIR "/usr/local/include/opencascade")
        set(OpenCASCADE_LIBRARY_DIR "/usr/local/lib")
    endif()
    
    include_directories(${OpenCASCADE_INCLUDE_DIR})
    link_directories(${OpenCASCADE_LIBRARY_DIR})
endif()

# ============================================================================
# Find OpenGL
# ============================================================================
find_package(OpenGL REQUIRED)

# ============================================================================
# Includes
# ============================================================================
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/features
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/viewer
    ${CMAKE_SOURCE_DIR}/src/ui
    ${Qt6Core_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6OpenGL_INCLUDE_DIRS}
    ${Qt6PrintSupport_INCLUDE_DIRS}
)

# ============================================================================
# Sources
# ============================================================================
set(CORE_SOURCES
    src/core/parametric/Feature.cpp
    src/core/document/CADDocument.cpp
)

set(FEATURES_SOURCES
    src/features/primitives/BoxFeature.cpp
    src/features/primitives/CylinderFeature.cpp
    src/features/sketch/SketchEntity.cpp
    src/features/sketch/Dimension.cpp
    src/features/sketch/GeometricConstraint.cpp
    src/features/sketch/Sketch2D.cpp
    src/features/sketch/SketchFeature.cpp
    src/features/operations/ExtrudeFeature.cpp
    src/features/operations/RevolveFeature.cpp
    src/features/operations/Fillet3DFeature.cpp
    src/features/operations/Chamfer3DFeature.cpp
    src/features/operations/SweepFeature.cpp
    src/features/operations/LoftFeature.cpp
    src/features/operations/LinearPatternFeature.cpp
    src/features/operations/CircularPatternFeature.cpp
    src/features/operations/ThreadFeature.cpp
    src/features/operations/ShellFeature.cpp
    src/features/operations/PushPullFeature.cpp
)

set(UI_SOURCES
    src/ui/viewport/Viewport3D.cpp
    src/ui/mainwindow/MainWindow.cpp
    src/ui/tree/DocumentTree.cpp
    src/ui/widgets/ViewCube.cpp
)

# ============================================================================
# Executable
# ============================================================================
if(WIN32)
    # Windows: WIN32 flag = no console window
    add_executable(cad-engine WIN32
        src/main.cpp
        ${CORE_SOURCES}
        ${FEATURES_SOURCES}
        ${UI_SOURCES}
    )
else()
    add_executable(cad-engine
        src/main.cpp
        ${CORE_SOURCES}
        ${FEATURES_SOURCES}
        ${UI_SOURCES}
    )
endif()

# ============================================================================
# OCCT Libraries
# ============================================================================
set(OCCT_LIBS
    TKBin TKBinL TKLCAF TKCAF TKCDF
    TKFeat TKOffset TKFillet TKBO TKBool
    TKMesh TKShHealing TKPrim
    TKTopAlgo TKGeomAlgo TKBRep
    TKGeomBase TKG3d TKG2d TKMath TKernel
)

# ============================================================================
# Linking
# ============================================================================
target_link_libraries(cad-engine
    ${OCCT_LIBS}
    Qt6::Core
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
    Qt6::PrintSupport
    ${OPENGL_LIBRARIES}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(cad-engine GLU pthread dl)
endif()

if(WIN32)
    target_link_libraries(cad-engine opengl32 glu32)
endif()

target_include_directories(cad-engine PRIVATE
    ${OpenCASCADE_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# ============================================================================
# Install rules (for packaging)
# ============================================================================
install(TARGETS cad-engine DESTINATION bin)

if(UNIX AND NOT APPLE)
    install(FILES packaging/linux/cad-engine.desktop DESTINATION share/applications)
    install(FILES packaging/linux/cad-engine.png DESTINATION share/icons/hicolor/256x256/apps)
endif()

# ============================================================================
# Tests
# ============================================================================
if(BUILD_TESTS)
    enable_testing()
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt)
        add_subdirectory(tests)
    endif()
endif()

# ============================================================================
# Configuration summary
# ============================================================================
message(STATUS "========================================")
message(STATUS "CAD-ENGINE v${PROJECT_VERSION}")
message(STATUS "========================================")
message(STATUS "Platform:    ${CMAKE_SYSTEM_NAME}")
message(STATUS "C++ std:     ${CMAKE_CXX_STANDARD}")
message(STATUS "Qt6:         ${Qt6_VERSION}")
message(STATUS "OCCT inc:    ${OpenCASCADE_INCLUDE_DIR}")
message(STATUS "OCCT libs:   ${OpenCASCADE_LIBRARY_DIR}")
message(STATUS "Build type:  ${CMAKE_BUILD_TYPE}")
message(STATUS "Portable:    ${PORTABLE_BUILD}")
message(STATUS "========================================")

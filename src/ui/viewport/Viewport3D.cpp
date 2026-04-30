#include "Viewport3D.h"
#include "ViewportScaling.h"
#include "ui/theme/ThemeManager.h"
#include "core/document/CADDocument.h"
#include "features/sketch/SketchFeature.h"
#include "features/sketch/Dimension.h"
#include <iostream>
#include <set>
#include <map>
#include "features/sketch/BasicConstraints.h"
#include "features/operations/ExtrudeFeature.h"
#include "features/operations/RevolveFeature.h"
#include "features/operations/Fillet3DFeature.h"
#include "features/operations/Chamfer3DFeature.h"
#include "features/operations/SweepFeature.h"
#include "features/operations/LoftFeature.h"
#include "features/operations/LinearPatternFeature.h"
#include "features/operations/CircularPatternFeature.h"
#include "features/operations/ThreadFeature.h"
#include "features/operations/ShellFeature.h"
#include "features/operations/PushPullFeature.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/glu.h>
#include <cmath>
#include <cstring>
#include <chrono>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QTimer>

// OpenCASCADE 3D tessellation
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <gp_Trsf.hxx>
#include <TopLoc_Location.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <GCPnts_TangentialDeflection.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Helper - Structure pour arc circulaire
// ============================================================================

struct ArcParams {
    gp_Pnt2d center;
    double radius;
    double startAngle;
    double endAngle;
    bool isValid;
};

// Forward declaration
ArcParams calculateArc(const gp_Pnt2d& p1, const gp_Pnt2d& p2, const gp_Pnt2d& p3);

// ============================================================================
// Viewport3D
// ============================================================================

Viewport3D::Viewport3D(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_mode(ViewMode::NORMAL_3D)
    , m_currentTool(SketchTool::None)
    , m_hasHoverPoint(false)
    , m_hasSnapPoint(false)
    , m_isDraggingEntity(false)
    , m_isDraggingDimension(false)
    , m_isDraggingVertex(false)
    , m_draggedVertexIndex(-1)
    , m_initialOffset(20.0)
    , m_cameraDistance(500.0f)
    , m_cameraAngleX(-30.0f)
    , m_cameraAngleY(45.0f)
    , m_cameraPanX(0.0f)
    , m_cameraPanY(0.0f)
    , m_sketch2DZoom(150.0f)  // Zoom initial ~300x300mm visible (pièces 200x200)
    , m_isRotating(false)
    , m_isPanning(false)
    , m_showGrid(true)
    , m_showAxes(true)
    , m_gridSnapEnabled(true)
    , m_hvSnapActive(false)
    , m_hvSnapHorizontal(false)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    
    // ViewCube overlay
    m_viewCube = new ViewCube(this);
    m_viewCube->setCameraAngles(m_cameraAngleX, m_cameraAngleY);
    connect(m_viewCube, &ViewCube::viewSelected, this, &Viewport3D::setView);
}

Viewport3D::~Viewport3D() {
}

void Viewport3D::setView(float angleX, float angleZ) {
    m_cameraAngleX = angleX;
    m_cameraAngleY = angleZ;
    m_viewCube->setCameraAngles(m_cameraAngleX, m_cameraAngleY);
    update();
}

void Viewport3D::repositionViewCube() {
    if (m_viewCube) {
        // En haut à droite avec marge
        m_viewCube->move(width() - m_viewCube->width() - 5, 5);
    }
}

void Viewport3D::resizeEvent(QResizeEvent* event) {
    QOpenGLWidget::resizeEvent(event);
    repositionViewCube();
}

// ============================================================================
// OpenGL
// ============================================================================

void Viewport3D::initializeGL() {
    initializeOpenGLFunctions();
    
    // Debug: afficher le contexte OpenGL obtenu
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (ctx) {
        auto fmt = ctx->format();
        std::cout << "[Viewport3D] OpenGL version: " << fmt.majorVersion() << "." << fmt.minorVersion() << std::endl;
        std::cout << "[Viewport3D] Profile: " << (fmt.profile() == QSurfaceFormat::CompatibilityProfile ? "Compatibility" : "Core") << std::endl;
        const char* renderer = (const char*)glGetString(GL_RENDERER);
        std::cout << "[Viewport3D] Renderer: " << (renderer ? renderer : "NULL") << std::endl;
    }
    
    glClearColor(m_bgR, m_bgG, m_bgB, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
}

void Viewport3D::updateThemeColors() {
    switch (ThemeManager::currentTheme()) {
        case ThemeManager::Light:
            m_bgR = 0.85f; m_bgG = 0.87f; m_bgB = 0.90f;  // Gris clair
            break;
        case ThemeManager::Dark:
            m_bgR = 0.18f; m_bgG = 0.18f; m_bgB = 0.22f;  // Gris foncé
            break;
        case ThemeManager::BlueDark:
            m_bgR = 0.12f; m_bgG = 0.15f; m_bgB = 0.22f;  // Bleu nuit
            break;
        case ThemeManager::Graphite:
            m_bgR = 0.22f; m_bgG = 0.22f; m_bgB = 0.22f;  // Graphite
            break;
    }
    update();
}

void Viewport3D::resizeGL(int w, int h) {
    if (w == 0 || h == 0) return;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    double aspect = (double)w / (double)h;
    if (m_mode == ViewMode::SKETCH_2D) {
        double size = m_sketch2DZoom;
        glOrtho(-size * aspect, size * aspect, -size, size, -1000.0, 1000.0);
    } else {
        gluPerspective(45.0, aspect, 1.0, 10000.0);
    }

    glMatrixMode(GL_MODELVIEW);
}

void Viewport3D::paintGL() {
    // Vider les labels de la frame précédente
    m_textLabels.clear();
    m_axisTickLabels.clear();
    m_axisLabels.clear();
    
    glClearColor(m_bgR, m_bgG, m_bgB, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set up projection matrix EVERY FRAME (zoom may have changed)
    // Utiliser les pixels physiques pour glViewport (correction DPR)
    int actualW = static_cast<int>(width()  * devicePixelRatio());
    int actualH = static_cast<int>(height() * devicePixelRatio());
    glViewport(0, 0, actualW, actualH);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    if (m_mode == ViewMode::SKETCH_2D) {
        double aspect = (double)actualW / (double)actualH;
        double size = m_sketch2DZoom;
        glOrtho(-size * aspect, size * aspect, -size, size, -1000.0, 1000.0);
    } else {
        double aspect = (double)actualW / (double)actualH;
        gluPerspective(45.0, aspect, 1.0, 20000.0);
    }
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    if (m_mode == ViewMode::NORMAL_3D) {
        setupCamera();
        renderNormalMode();
    } else {
        setupSketchCamera();
        renderSketchMode();
    }
    
    // Cacher les matrices GL pour le picking (screenTo3DRay hors paintGL)
    if (m_mode == ViewMode::NORMAL_3D) {
        glGetDoublev(GL_MODELVIEW_MATRIX, m_cachedModelview);
        glGetDoublev(GL_PROJECTION_MATRIX, m_cachedProjection);
        glGetIntegerv(GL_VIEWPORT, m_cachedViewport);
        m_glMatricesValid = true;
    }
    
    // Après le rendu OpenGL, dessiner le texte avec QPainter
    if (!m_textLabels.empty() || !m_axisLabels.empty() || !m_axisTickLabels.empty()) {
        // Récupérer matrices OpenGL pour projection AVANT QPainter
        GLdouble modelMatrix[16], projMatrix[16];
        GLint viewport[4];
        glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
        glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
        glGetIntegerv(GL_VIEWPORT, viewport);
        
        // Sauvegarder TOUS les états OpenGL
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        
        // Commencer peinture QPainter sur le widget OpenGL
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        // gluProject retourne des coords en pixels physiques ; QPainter travaille en pixels logiques
        const double dpr = devicePixelRatio();
        
        // Police adaptative au zoom
        QFont font = painter.font();
        
        // Calculer taille texte en fonction du zoom
        // ViewportScaling retourne mm, on convertit en points écran
        double textSizeMM = CADEngine::ViewportScaling::getDimensionTextSize(m_sketch2DZoom);
        // Conversion approximative : 1mm dans l'espace monde = X pixels à l'écran
        // Dépend du zoom : plus on zoome, plus 1mm = beaucoup de pixels
        double pixelsPerMM = height() / (m_sketch2DZoom * 2.0);  // Approximation
        int fontSize = static_cast<int>(textSizeMM * pixelsPerMM * 0.75);  // 0.75 = facteur points
        
        // Limites raisonnables pour l'affichage
        if (fontSize < 8) fontSize = 8;    // Min lisible
        if (fontSize > 24) fontSize = 24;  // Max raisonnable
        
        font.setPointSize(fontSize);
        font.setBold(true);
        painter.setFont(font);
        
        // Couleur texte
        painter.setPen(QColor(0, 153, 255));  // Bleu
        
        // Dessiner chaque label
        for (const auto& label : m_textLabels) {
            // Projeter coordonnées 3D vers 2D écran
            GLdouble winX, winY, winZ;
            gluProject(label.x, label.y, label.z,
                      modelMatrix, projMatrix, viewport,
                      &winX, &winY, &winZ);
            
            // OpenGL a origine en bas, Qt en haut ; diviser par DPR pour coords logiques
            int screenX = static_cast<int>(winX / dpr);
            int screenY = height() - static_cast<int>(winY / dpr);
            
            // Mesurer la taille réelle du texte
            QFontMetrics fm(painter.font());
            int textW = fm.horizontalAdvance(label.text) + 10;
            int textH = fm.height() + 2;

            // Centrer puis clamper dans les limites du widget (évite les coupures aux bords)
            int rx = qBound(2, screenX - textW/2, width() - textW - 2);
            int ry = qBound(2, screenY - textH/2, height() - textH - 2);
            QRect textRect(rx, ry, textW, textH);
            
            // Fond léger
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(0, 0, 0, 100));
            painter.drawRoundedRect(textRect, 3, 3);
            painter.setBrush(Qt::NoBrush);
            
            // Texte
            painter.setPen(QColor(0, 200, 255));
            painter.drawText(textRect, Qt::AlignCenter, label.text);
        }
        
        // ── Repères dimensionnels des axes (légers, sans fond) ──
        if (!m_axisTickLabels.empty()) {
            QFont tickFont;
            tickFont.setPixelSize(13);
            tickFont.setBold(true);
            
            QFont axisNameFont;
            axisNameFont.setPixelSize(18);
            axisNameFont.setBold(true);
            
            for (const auto& tick : m_axisTickLabels) {
                GLdouble winX, winY, winZ;
                gluProject(tick.x, tick.y, tick.z,
                          modelMatrix, projMatrix, viewport,
                          &winX, &winY, &winZ);
                
                int sx = static_cast<int>(winX / dpr);
                int sy = height() - static_cast<int>(winY / dpr);

                // Noms d'axes (X, Y, Z) = plus gros et colorés
                bool isAxisName = (tick.text.length() == 1 && tick.text[0].isLetter());
                painter.setFont(isAxisName ? axisNameFont : tickFont);
                
                if (isAxisName) {
                    // Ombre foncée pour les noms d'axes colorés
                    painter.setPen(QColor(0, 0, 0, 120));
                    painter.drawText(sx - 19, sy - 6, 40, 14, Qt::AlignCenter, tick.text);
                    QColor c = tick.color;
                    c.setAlpha(255);
                    painter.setPen(c);
                    painter.drawText(sx - 20, sy - 7, 40, 14, Qt::AlignCenter, tick.text);
                } else {
                    // Ombre foncée pour les chiffres clairs
                    painter.setPen(QColor(0, 0, 0, 140));
                    painter.drawText(sx - 19, sy - 6, 40, 14, Qt::AlignCenter, tick.text);
                    QColor c = tick.color;
                    c.setAlpha(255);
                    painter.setPen(c);
                    painter.drawText(sx - 20, sy - 7, 40, 14, Qt::AlignCenter, tick.text);
                }
            }
        }
        
        // ── Labels des axes 3D (X, Y, Z) ──
        if (!m_axisLabels.empty()) {
            QFont axisFont = painter.font();
            axisFont.setPointSize(13);
            axisFont.setBold(true);
            painter.setFont(axisFont);
            
            for (const auto& al : m_axisLabels) {
                GLdouble winX, winY, winZ;
                gluProject(al.x, al.y, al.z,
                          modelMatrix, projMatrix, viewport,
                          &winX, &winY, &winZ);
                
                // Vérifier que le label est devant la caméra
                if (winZ > 0 && winZ < 1.0) {
                    int sx = static_cast<int>(winX / dpr);
                    int sy = height() - static_cast<int>(winY / dpr);
                    
                    // Ombre pour lisibilité
                    painter.setPen(QColor(0, 0, 0, 120));
                    painter.drawText(sx - 9, sy - 9, 20, 20, Qt::AlignCenter, al.text);
                    
                    // Texte coloré
                    painter.setPen(al.color);
                    painter.drawText(sx - 10, sy - 10, 20, 20, Qt::AlignCenter, al.text);
                }
            }
        }
        
        painter.end();
        
        // Restaurer états OpenGL
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        glPopAttrib();
    }
}

// ============================================================================
// Mise à jour dimensions après drag entités
// ============================================================================

void Viewport3D::updateDimensionsForLine(const gp_Pnt2d& oldStart, const gp_Pnt2d& oldEnd,
                                          const gp_Pnt2d& newStart, const gp_Pnt2d& newEnd) {
    if (!m_activeSketch) return;
    
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return;
    
    double tolerance = 1.0;  // 1mm de tolérance
    
    // Parcourir toutes les dimensions
    for (const auto& dim : sketch2D->getDimensions()) {
        if (dim->getType() == CADEngine::DimensionType::Linear) {
            auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
            if (!linearDim) continue;
            
            gp_Pnt2d p1 = linearDim->getPoint1();
            gp_Pnt2d p2 = linearDim->getPoint2();
            
            // Vérifier si cette dimension correspond à notre ligne
            bool matchStart1 = (p1.Distance(oldStart) < tolerance);
            bool matchEnd1 = (p2.Distance(oldEnd) < tolerance);
            bool matchStart2 = (p1.Distance(oldEnd) < tolerance);
            bool matchEnd2 = (p2.Distance(oldStart) < tolerance);
            
            if (matchStart1 && matchEnd1) {
                // p1=start, p2=end
                linearDim->setPoint1(newStart);
                linearDim->setPoint2(newEnd);
            }
            else if (matchStart2 && matchEnd2) {
                // p1=end, p2=start
                linearDim->setPoint1(newEnd);
                linearDim->setPoint2(newStart);
            }
        }
        else if (dim->getType() == CADEngine::DimensionType::Angular) {
            // Rafraîchir cotations angulaires liées à la ligne draggée
            auto angularDim = std::dynamic_pointer_cast<CADEngine::AngularDimension>(dim);
            if (angularDim && m_draggedEntity) {
                if (angularDim->getSourceEntity1() == m_draggedEntity ||
                    angularDim->getSourceEntity2() == m_draggedEntity) {
                    angularDim->refreshFromSources();
                }
            }
        }
    }
}

void Viewport3D::updateDimensionsForCircle(const gp_Pnt2d& oldCenter, const gp_Pnt2d& newCenter) {
    if (!m_activeSketch) return;
    
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return;
    
    double tolerance = 1.0;  // 1mm de tolérance
    double deltaX = newCenter.X() - oldCenter.X();
    double deltaY = newCenter.Y() - oldCenter.Y();
    
    // Parcourir toutes les dimensions
    for (const auto& dim : sketch2D->getDimensions()) {
        if (dim->getType() == CADEngine::DimensionType::Diametral) {
            auto diametralDim = std::dynamic_pointer_cast<CADEngine::DiametralDimension>(dim);
            if (!diametralDim) continue;
            
            gp_Pnt2d center = diametralDim->getCenter();
            
            // Vérifier si cette dimension correspond à notre cercle
            if (center.Distance(oldCenter) < tolerance) {
                // Déplacer le centre de la dimension
                diametralDim->setCenter(newCenter);
            }
        }
    }
}

void Viewport3D::updateDimensionsForRectangle(const gp_Pnt2d& oldCorner, double width, double height,
                                                const gp_Pnt2d& newCorner) {
    if (!m_activeSketch) return;
    
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return;
    
    // Anciens coins
    gp_Pnt2d oldP1 = oldCorner;
    gp_Pnt2d oldP2(oldCorner.X() + width, oldCorner.Y());
    gp_Pnt2d oldP3(oldCorner.X() + width, oldCorner.Y() + height);
    gp_Pnt2d oldP4(oldCorner.X(), oldCorner.Y() + height);
    
    // Nouveaux coins
    gp_Pnt2d newP1 = newCorner;
    gp_Pnt2d newP2(newCorner.X() + width, newCorner.Y());
    gp_Pnt2d newP3(newCorner.X() + width, newCorner.Y() + height);
    gp_Pnt2d newP4(newCorner.X(), newCorner.Y() + height);
    
    double tolerance = 15.0;  // Tolérance large pour détecter les côtés
    
    // Parcourir dimensions et mettre à jour les côtés
    for (const auto& dim : sketch2D->getDimensions()) {
        if (dim->getType() == CADEngine::DimensionType::Linear) {
            auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
            if (!linearDim) continue;
            
            gp_Pnt2d p1 = linearDim->getPoint1();
            gp_Pnt2d p2 = linearDim->getPoint2();
            
            // Calculer direction de la dimension
            double dimDx = p2.X() - p1.X();
            double dimDy = p2.Y() - p1.Y();
            double dimLen = std::sqrt(dimDx*dimDx + dimDy*dimDy);
            
            if (dimLen < 1e-6) continue;
            
            // Vérifier chaque côté du rectangle PAR PROXIMITÉ
            // Côté bas : horizontal, Y proche de oldP1.Y
            if (std::abs(dimDy) < 0.1 * dimLen) {  // Quasi-horizontal
                double distY1 = std::abs(p1.Y() - oldP1.Y());
                double distY2 = std::abs(p2.Y() - oldP1.Y());
                if (distY1 < tolerance && distY2 < tolerance) {
                    // C'est le côté bas !
                    linearDim->setPoint1(newP1);
                    linearDim->setPoint2(newP2);
                    continue;
                }
                // Côté haut
                distY1 = std::abs(p1.Y() - oldP3.Y());
                distY2 = std::abs(p2.Y() - oldP3.Y());
                if (distY1 < tolerance && distY2 < tolerance) {
                    linearDim->setPoint1(newP3);
                    linearDim->setPoint2(newP4);
                    continue;
                }
            }
            // Côtés verticaux
            else if (std::abs(dimDx) < 0.1 * dimLen) {  // Quasi-vertical
                double distX1 = std::abs(p1.X() - oldP2.X());
                double distX2 = std::abs(p2.X() - oldP2.X());
                if (distX1 < tolerance && distX2 < tolerance) {
                    // C'est le côté droit !
                    linearDim->setPoint1(newP2);
                    linearDim->setPoint2(newP3);
                    continue;
                }
                // Côté gauche
                distX1 = std::abs(p1.X() - oldP1.X());
                distX2 = std::abs(p2.X() - oldP1.X());
                if (distX1 < tolerance && distX2 < tolerance) {
                    linearDim->setPoint1(newP4);
                    linearDim->setPoint2(newP1);
                    continue;
                }
            }
        }
    }
}

// ============================================================================
// Caméra
// ============================================================================

void Viewport3D::setupCamera() {
    // Caméra orbitale standard
    glTranslatef(m_cameraPanX, m_cameraPanY, -m_cameraDistance);
    glRotatef(m_cameraAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(m_cameraAngleY, 0.0f, 0.0f, 1.0f);
}

gp_Dir Viewport3D::getCameraViewDirection() const {
    // Calculer la direction de vue depuis les angles caméra
    double ax = m_cameraAngleX * M_PI / 180.0;
    double ay = m_cameraAngleY * M_PI / 180.0;
    
    // Position caméra en coordonnées monde (inverse des rotations GL)
    double eyeX = sin(ax) * sin(ay);
    double eyeY = sin(ax) * cos(ay);
    double eyeZ = cos(ax);
    
    // Direction de vue = de la caméra vers l'origine = -eye
    double vx = -eyeX, vy = -eyeY, vz = -eyeZ;
    double mag = sqrt(vx*vx + vy*vy + vz*vz);
    if (mag < 1e-10) return gp_Dir(0, 0, -1);
    return gp_Dir(vx/mag, vy/mag, vz/mag);
}

void Viewport3D::setupSketchCamera() {
    if (!m_activeSketch) return;
    
    // CRITIQUE : Recalculer la projection avec le zoom actuel !
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    double aspect = (double)width() / (double)height();
    double size = m_sketch2DZoom;  // Utiliser le zoom dynamique
    glOrtho(-size * aspect, size * aspect, -size, size, -1000.0, 1000.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Vue de dessus simple - la projection orthogonale gère le scale
    gluLookAt(
        0.0, 0.0, 500.0,      // Caméra au-dessus
        0.0, 0.0, 0.0,        // Regarde l'origine
        0.0, 1.0, 0.0         // Up = Y
    );
    
    // Pan seulement
    glTranslatef(-m_cameraPanX, -m_cameraPanY, 0.0f);
}

// ============================================================================
// Rendu
// ============================================================================

void Viewport3D::renderNormalMode() {
    if (m_showGrid) renderGrid();
    if (m_showAxes) renderAxes();
    
    // Enable lighting for solid rendering
    setupLighting();
    
    renderAllFeatures();  // Rendu des features du document
    
    // Disable lighting after solid rendering
    glDisable(GL_LIGHTING);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
}

void Viewport3D::renderSketchMode() {
    // Mode sketch : pas de grille/axes 3D (perturbant)
    
    // Ordre important : 
    renderSketchGrid();           // 1. Grille (dessous)
    renderSketchReferenceBody();  // 2. Ghost wireframe du body 3D (pour repères)
    renderSketchPlane();          // 3. Plan transparent
    renderSketchAxes();           // 4. Axes X/Y PAR-DESSUS le plan
    renderSketchEntities();       // 5. Entités
    renderTempEntity();           // 6. Preview
}

void Viewport3D::renderSketchAxes() {
    if (!m_activeSketch) return;
    
    int gridSize = CADEngine::ViewportScaling::getGridDisplaySize(m_sketch2DZoom);
    float axisZ = 0.05f;
    
    // Déterminer les labels et couleurs selon le plan du sketch
    std::string planeType = "XY";
    try { planeType = m_activeSketch->getString("plane_type"); } catch(...) {}
    
    QString labelH = "X", labelV = "Y";
    float colorH[3] = {0.85f, 0.15f, 0.15f};
    float colorV[3] = {0.15f, 0.70f, 0.15f};
    
    if (planeType == "YZ") {
        labelH = "Y"; labelV = "Z";
        colorH[0] = 0.15f; colorH[1] = 0.70f; colorH[2] = 0.15f;
        colorV[0] = 0.30f; colorV[1] = 0.50f; colorV[2] = 0.90f;
    } else if (planeType == "ZX") {
        labelH = "X"; labelV = "Z";
        colorH[0] = 0.85f; colorH[1] = 0.15f; colorH[2] = 0.15f;
        colorV[0] = 0.30f; colorV[1] = 0.50f; colorV[2] = 0.90f;
    }
    
    glLineWidth(2.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBegin(GL_LINES);
    // Axe horizontal
    glColor4f(colorH[0], colorH[1], colorH[2], 0.9f);
    glVertex3f((float)-gridSize, 0, axisZ);
    glVertex3f((float)gridSize, 0, axisZ);
    // Axe vertical
    glColor4f(colorV[0], colorV[1], colorV[2], 0.9f);
    glVertex3f(0, (float)-gridSize, axisZ);
    glVertex3f(0, (float)gridSize, axisZ);
    glEnd();
    
    // ── Repères dimensionnels (tick marks + valeurs) ──
    // Utiliser le même step que la grille → toujours entier
    double gridStep = CADEngine::ViewportScaling::getGridDisplayStep(m_sketch2DZoom);
    
    // Tick interval = step de la grille (toujours entier : 1, 2, 5, 10, 25, 50...)
    double tickInterval = gridStep;
    
    // Label interval = multiple du tick pour lisibilité
    // Calculer combien de pixels entre chaque tick
    double pixelsPerTick = (height() / (m_sketch2DZoom * 2.0)) * tickInterval;
    
    // Labels sur les ticks, mais pas tous si trop denses
    double labelInterval = tickInterval;
    if (pixelsPerTick < 25) {
        // Très dense → labels tous les 5 ticks
        labelInterval = tickInterval * 5;
    } else if (pixelsPerTick < 50) {
        // Dense → labels tous les 2 ticks  
        labelInterval = tickInterval * 2;
    }
    // Arrondir labelInterval au nombre entier supérieur
    if (labelInterval < 1.0) labelInterval = 1.0;
    labelInterval = std::ceil(labelInterval);
    
    float tickLen = m_sketch2DZoom * 0.015f;
    float visRange = m_sketch2DZoom * 1.1f;
    int maxLabels = 10;
    
    glLineWidth(1.0f);
    
    // Petits ticks sur l'axe horizontal (à chaque gridStep)
    glColor4f(colorH[0], colorH[1], colorH[2], 0.5f);
    glBegin(GL_LINES);
    for (double x = tickInterval; x < visRange; x += tickInterval) {
        glVertex3f((float)x, -tickLen, axisZ);
        glVertex3f((float)x, tickLen, axisZ);
        glVertex3f((float)-x, -tickLen, axisZ);
        glVertex3f((float)-x, tickLen, axisZ);
    }
    glEnd();
    
    // Petits ticks sur l'axe vertical
    glColor4f(colorV[0], colorV[1], colorV[2], 0.5f);
    glBegin(GL_LINES);
    for (double y = tickInterval; y < visRange; y += tickInterval) {
        glVertex3f(-tickLen, (float)y, axisZ);
        glVertex3f(tickLen, (float)y, axisZ);
        glVertex3f(-tickLen, (float)-y, axisZ);
        glVertex3f(tickLen, (float)-y, axisZ);
    }
    glEnd();
    
    // Valeurs numériques — uniquement aux intervalles de label, nombres ronds
    QColor tickColor(220, 225, 235);
    QColor colorHQ(colorH[0]*255, colorH[1]*255, colorH[2]*255);
    QColor colorVQ(colorV[0]*255, colorV[1]*255, colorV[2]*255);
    
    // Formateur : entier si rond, sinon 1 décimale
    auto fmt = [](double val) -> QString {
        if (std::abs(val - std::round(val)) < 0.01)
            return QString::number((int)std::round(val));
        return QString::number(val, 'f', 1);
    };
    
    int labelCount = 0;
    for (double v = labelInterval; v < visRange && labelCount < maxLabels; v += labelInterval) {
        m_axisTickLabels.push_back({v, -tickLen * 4.0, 0.2, fmt(v), tickColor});
        m_axisTickLabels.push_back({-v, -tickLen * 4.0, 0.2, fmt(-v), tickColor});
        m_axisTickLabels.push_back({tickLen * 3.0, v, 0.2, fmt(v), tickColor});
        m_axisTickLabels.push_back({tickLen * 3.0, -v, 0.2, fmt(-v), tickColor});
        labelCount++;
    }
    
    // Labels des axes aux bords du viewport (colorés)
    float labelOffset = m_sketch2DZoom * 0.85f;
    m_axisTickLabels.push_back({(double)labelOffset, -m_sketch2DZoom * 0.03, 0.3, labelH, colorHQ});
    m_axisTickLabels.push_back({m_sketch2DZoom * 0.02, (double)labelOffset, 0.3, labelV, colorVQ});
    
    // Point d'origine
    glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    glVertex3f(0, 0, axisZ + 0.01f);
    glEnd();
    glPointSize(1.0f);
    
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}

void Viewport3D::renderGrid() {
    // ── Grille style Fusion 360 : lignes majeures sobres avec fondu aux bords ──
    int gridExtent = 300;
    int majorStep = 50;
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = -gridExtent; i <= gridExtent; i += majorStep) {
        if (i == 0) continue;  // L'axe central est dessiné par renderAxes
        
        // Fondu : alpha décroît vers les bords
        float dist = std::abs((float)i) / (float)gridExtent;
        float alpha = 0.35f * (1.0f - dist * dist);  // Quadratique
        
        glColor4f(0.45f, 0.45f, 0.45f, alpha);
        glVertex3f((float)i, (float)-gridExtent, 0.0f);
        glColor4f(0.45f, 0.45f, 0.45f, 0.0f);
        glVertex3f((float)i, (float)-gridExtent, 0.0f);  // redraw start
        
        // Chaque ligne : fondu aux deux extrémités
        glColor4f(0.45f, 0.45f, 0.45f, 0.0f);
        glVertex3f((float)i, (float)-gridExtent, 0.0f);
        glColor4f(0.45f, 0.45f, 0.45f, alpha);
        glVertex3f((float)i, (float)(-gridExtent / 2), 0.0f);
        
        glColor4f(0.45f, 0.45f, 0.45f, alpha);
        glVertex3f((float)i, (float)(-gridExtent / 2), 0.0f);
        glColor4f(0.45f, 0.45f, 0.45f, alpha);
        glVertex3f((float)i, (float)(gridExtent / 2), 0.0f);
        
        glColor4f(0.45f, 0.45f, 0.45f, alpha);
        glVertex3f((float)i, (float)(gridExtent / 2), 0.0f);
        glColor4f(0.45f, 0.45f, 0.45f, 0.0f);
        glVertex3f((float)i, (float)gridExtent, 0.0f);
        
        // Même chose pour l'autre direction
        glColor4f(0.45f, 0.45f, 0.45f, 0.0f);
        glVertex3f((float)-gridExtent, (float)i, 0.0f);
        glColor4f(0.45f, 0.45f, 0.45f, alpha);
        glVertex3f((float)(-gridExtent / 2), (float)i, 0.0f);
        
        glColor4f(0.45f, 0.45f, 0.45f, alpha);
        glVertex3f((float)(-gridExtent / 2), (float)i, 0.0f);
        glColor4f(0.45f, 0.45f, 0.45f, alpha);
        glVertex3f((float)(gridExtent / 2), (float)i, 0.0f);
        
        glColor4f(0.45f, 0.45f, 0.45f, alpha);
        glVertex3f((float)(gridExtent / 2), (float)i, 0.0f);
        glColor4f(0.45f, 0.45f, 0.45f, 0.0f);
        glVertex3f((float)gridExtent, (float)i, 0.0f);
    }
    glEnd();
    
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Viewport3D::renderAxes() {
    // ── Style Fusion 360 / SolidWorks ──
    // Proportions calibrées pour camera distance 500
    float axisLen   = 60.0f;
    float negLen    = 30.0f;
    float arrowLen  = 8.0f;
    float arrowR    = 2.2f;
    int arrowSegs   = 12;
    
    glDisable(GL_LIGHTING);
    
    // ══════════════════════════════════════════════
    // 1. Partie négative des axes (tirets discrets)
    // ══════════════════════════════════════════════
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(2, 0xCCCC);
    glLineWidth(1.0f);
    
    glBegin(GL_LINES);
    glColor3f(0.45f, 0.18f, 0.18f);
    glVertex3f(0, 0, 0);  glVertex3f(-negLen, 0, 0);
    glColor3f(0.18f, 0.4f, 0.18f);
    glVertex3f(0, 0, 0);  glVertex3f(0, -negLen, 0);
    glColor3f(0.18f, 0.18f, 0.45f);
    glVertex3f(0, 0, 0);  glVertex3f(0, 0, -negLen);
    glEnd();
    
    glDisable(GL_LINE_STIPPLE);
    
    // ══════════════════════════════════════════════
    // 3. Partie positive des axes (trait solide)
    // ══════════════════════════════════════════════
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glColor3f(0.85f, 0.20f, 0.20f);
    glVertex3f(0, 0, 0);  glVertex3f(axisLen, 0, 0);
    glColor3f(0.20f, 0.72f, 0.20f);
    glVertex3f(0, 0, 0);  glVertex3f(0, axisLen, 0);
    glColor3f(0.20f, 0.40f, 0.85f);
    glVertex3f(0, 0, 0);  glVertex3f(0, 0, axisLen);
    glEnd();
    glLineWidth(1.0f);
    
    // ══════════════════════════════════════════════
    // 4. Flèches coniques
    // ══════════════════════════════════════════════
    auto drawCone = [&](float tipX, float tipY, float tipZ,
                         float dX, float dY, float dZ,
                         float r, float g, float b) {
        float bx = tipX - arrowLen * dX;
        float by = tipY - arrowLen * dY;
        float bz = tipZ - arrowLen * dZ;
        
        // Base orthonormale
        float ux = 0, uy = 0, uz = 1;
        if (std::abs(dZ) > 0.9f) { ux = 1; uy = 0; uz = 0; }
        float p1x = dY*uz - dZ*uy, p1y = dZ*ux - dX*uz, p1z = dX*uy - dY*ux;
        float len = std::sqrt(p1x*p1x + p1y*p1y + p1z*p1z);
        if (len > 0) { p1x/=len; p1y/=len; p1z/=len; }
        float p2x = dY*p1z - dZ*p1y, p2y = dZ*p1x - dX*p1z, p2z = dX*p1y - dY*p1x;
        
        glColor3f(r, g, b);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(tipX, tipY, tipZ);
        for (int i = 0; i <= arrowSegs; i++) {
            float a = 2.0f * M_PI * i / arrowSegs;
            float ca = std::cos(a) * arrowR, sa = std::sin(a) * arrowR;
            glVertex3f(bx + ca*p1x + sa*p2x, by + ca*p1y + sa*p2y, bz + ca*p1z + sa*p2z);
        }
        glEnd();
        // Disque base
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(bx, by, bz);
        for (int i = arrowSegs; i >= 0; i--) {
            float a = 2.0f * M_PI * i / arrowSegs;
            float ca = std::cos(a) * arrowR, sa = std::sin(a) * arrowR;
            glVertex3f(bx + ca*p1x + sa*p2x, by + ca*p1y + sa*p2y, bz + ca*p1z + sa*p2z);
        }
        glEnd();
    };
    
    drawCone(axisLen, 0, 0,  1,0,0,  0.85f, 0.20f, 0.20f);
    drawCone(0, axisLen, 0,  0,1,0,  0.20f, 0.72f, 0.20f);
    drawCone(0, 0, axisLen,  0,0,1,  0.20f, 0.40f, 0.85f);
    
    // ══════════════════════════════════════════════
    // 5. Point d'origine (petite sphère)
    // ══════════════════════════════════════════════
    glColor3f(0.85f, 0.85f, 0.85f);
    float oR = 1.8f;
    int oS = 8;
    for (int i = 0; i < oS; i++) {
        float lat1 = M_PI * (-0.5f + (float)i / oS);
        float lat2 = M_PI * (-0.5f + (float)(i+1) / oS);
        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= oS; j++) {
            float lng = 2*M_PI*(float)j/oS;
            glVertex3f(oR*cos(lat2)*cos(lng), oR*cos(lat2)*sin(lng), oR*sin(lat2));
            glVertex3f(oR*cos(lat1)*cos(lng), oR*cos(lat1)*sin(lng), oR*sin(lat1));
        }
        glEnd();
    }
    
    // ══════════════════════════════════════════════
    // 6. Labels X, Y, Z (rendu QPainter overlay)
    // ══════════════════════════════════════════════
    m_axisLabels.clear();
    m_axisLabels.push_back({axisLen + 10, 0, 0, "X", QColor(210, 55, 55)});
    m_axisLabels.push_back({0, axisLen + 10, 0, "Y", QColor(50, 185, 50)});
    m_axisLabels.push_back({0, 0, axisLen + 10, "Z", QColor(55, 100, 210)});
}

void Viewport3D::renderSketchReferenceBody() {
    if (m_sketchRefBody.IsNull() || !m_activeSketch) return;
    
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return;
    
    gp_Pln plane = sketch2D->getPlane();
    gp_Pnt origin = plane.Location();
    gp_Dir xDir = plane.XAxis().Direction();
    gp_Dir yDir = plane.YAxis().Direction();
    gp_Dir nDir = plane.Axis().Direction();
    
    // Helper: project 3D point to sketch local coords
    auto projPt = [&](const gp_Pnt& p, float& u, float& v, float& w) {
        gp_Vec vec(origin, p);
        u = (float)vec.Dot(gp_Vec(xDir));
        v = (float)vec.Dot(gp_Vec(yDir));
        w = (float)vec.Dot(gp_Vec(nDir));
    };
    
    // CRITICAL: Disable depth test — ghost body must NEVER occlude sketch entities
    glDisable(GL_DEPTH_TEST);
    
    // ---- 1. Render faces as faint fill (depth context) ----
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.5f, 0.55f, 0.65f, 0.12f);
    
    TopExp_Explorer faceExp(m_sketchRefBody, TopAbs_FACE);
    for (; faceExp.More(); faceExp.Next()) {
        TopoDS_Face face = TopoDS::Face(faceExp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) continue;
        
        gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : loc.Transformation();
        bool hasTrsf = !loc.IsIdentity();
        
        glBegin(GL_TRIANGLES);
        for (int i = 1; i <= tri->NbTriangles(); i++) {
            int n1, n2, n3;
            tri->Triangle(i).Get(n1, n2, n3);
            gp_Pnt p1 = tri->Node(n1), p2 = tri->Node(n2), p3 = tri->Node(n3);
            if (hasTrsf) { p1.Transform(trsf); p2.Transform(trsf); p3.Transform(trsf); }
            float u1,v1,w1, u2,v2,w2, u3,v3,w3;
            projPt(p1,u1,v1,w1); projPt(p2,u2,v2,w2); projPt(p3,u3,v3,w3);
            // Push ALL geometry behind sketch plane (z = -2 always)
            glVertex3f(u1, v1, -2.0f);
            glVertex3f(u2, v2, -2.0f);
            glVertex3f(u3, v3, -2.0f);
        }
        glEnd();
    }
    glDisable(GL_BLEND);
    
    // ---- 2. Render edges as wireframe + collect on-plane snap points ----
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    m_refBodySnapPoints.clear();
    
    TopExp_Explorer edgeExp(m_sketchRefBody, TopAbs_EDGE);
    for (; edgeExp.More(); edgeExp.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(edgeExp.Current());
        if (edge.IsNull()) continue;
        
        try {
            BRepAdaptor_Curve curve(edge);
            GCPnts_TangentialDeflection disc(curve, 0.1, 0.05);
            if (disc.NbPoints() < 2) continue;
            
            bool onPlane = true;
            double maxDist = 0.0;
            std::vector<std::pair<float,float>> projected;
            
            for (int i = 1; i <= disc.NbPoints(); i++) {
                gp_Pnt p = disc.Value(i);
                float u, v, w;
                projPt(p, u, v, w);
                projected.push_back({u, v});
                if (std::abs(w) > 0.5) onPlane = false;
                if (std::abs(w) > maxDist) maxDist = std::abs(w);
            }
            
            // Edges on sketch plane: bright orange (face contour)
            if (onPlane) {
                glColor4f(1.0f, 0.55f, 0.1f, 0.95f);
                glLineWidth(2.5f);
                
                // Collect endpoints as snap points
                gp_Pnt pFirst = disc.Value(1);
                gp_Pnt pLast = disc.Value(disc.NbPoints());
                float uf, vf, wf, ul, vl, wl;
                projPt(pFirst, uf, vf, wf);
                projPt(pLast, ul, vl, wl);
                m_refBodySnapPoints.push_back(gp_Pnt2d(uf, vf));
                m_refBodySnapPoints.push_back(gp_Pnt2d(ul, vl));
                
                // Also add midpoint for snapping
                if (disc.NbPoints() >= 3) {
                    int mid = disc.NbPoints() / 2 + 1;
                    gp_Pnt pMid = disc.Value(mid);
                    float um, vm, wm;
                    projPt(pMid, um, vm, wm);
                    m_refBodySnapPoints.push_back(gp_Pnt2d(um, vm));
                }
            } else if (maxDist < 50.0) {
                glColor4f(0.55f, 0.58f, 0.65f, 0.55f);
                glLineWidth(1.5f);
            } else {
                glColor4f(0.5f, 0.52f, 0.58f, 0.3f);
                glLineWidth(1.0f);
            }
            
            // All edges rendered at z=-1 (behind sketch plane)
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i < (int)projected.size(); i++) {
                glVertex3f(projected[i].first, projected[i].second, -1.0f);
            }
            glEnd();
            
        } catch (...) {}
    }
    
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    
    // ---- 3. Draw snap point markers on sketch plane edges ----
    if (!m_refBodySnapPoints.empty()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1.0f, 0.55f, 0.1f, 0.7f);
        glPointSize(6.0f);
        glBegin(GL_POINTS);
        for (const auto& sp : m_refBodySnapPoints) {
            glVertex3f((float)sp.X(), (float)sp.Y(), -0.5f);
        }
        glEnd();
        glPointSize(1.0f);
        glDisable(GL_BLEND);
    }
    
    // Re-enable depth test for subsequent rendering (sketch entities, etc.)
    glEnable(GL_DEPTH_TEST);
}

void Viewport3D::renderSketchPlane() {
    if (!m_activeSketch) return;
    
    // Rectangle bleu semi-transparent adapté au zoom
    float size = m_sketch2DZoom * 2.0f;  // 2× la zone visible
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.5f, 0.7f, 1.0f, 0.15f);  // Bleu très transparent
    
    glBegin(GL_QUADS);
    glVertex3f(-size, -size, 0.0f);
    glVertex3f(size, -size, 0.0f);
    glVertex3f(size, size, 0.0f);
    glVertex3f(-size, size, 0.0f);
    glEnd();
    
    glDisable(GL_BLEND);
}

void Viewport3D::renderSketchGrid() {
    if (!m_activeSketch) return;
    
    // Grille adaptative avec système centralisé
    int gridSize = CADEngine::ViewportScaling::getGridDisplaySize(m_sketch2DZoom);
    double step = CADEngine::ViewportScaling::getGridDisplayStep(m_sketch2DZoom);
    
    // Séquence entière pour monter au palier suivant si trop de lignes
    static const double roundSteps[] = {1, 2, 5, 10, 25, 50, 100, 250, 500, 1000};
    int maxLines = 200;
    int numLines = static_cast<int>((2.0 * gridSize) / step);
    if (numLines > maxLines) {
        // Trouver le prochain palier rond qui donne <= maxLines lignes
        for (double rs : roundSteps) {
            if (rs > step && (2.0 * gridSize) / rs <= maxLines) {
                step = rs;
                break;
            }
        }
    }
    
    // Grille — atténuée si snap désactivé
    float gridAlpha = m_gridSnapEnabled ? 1.0f : 0.35f;
    glColor4f(0.5f, 0.5f, 0.5f, gridAlpha);
    glLineWidth(1.0f);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBegin(GL_LINES);
    
    double startPos = std::floor(-(double)gridSize / step) * step;
    
    for (double i = startPos; i <= gridSize; i += step) {
        glVertex3f(-gridSize, i, 0);
        glVertex3f(gridSize, i, 0);
        glVertex3f(i, -gridSize, 0);
        glVertex3f(i, gridSize, 0);
    }
    
    glEnd();
    
    glDisable(GL_BLEND);
    
    // Indicateur snap état (coin bas-gauche du viewport sketch)
    double snapGrid = CADEngine::ViewportScaling::getSnapGridSize(m_sketch2DZoom);
    double labelX = -m_sketch2DZoom * ((double)width() / (double)height()) + m_sketch2DZoom * 0.05;
    double labelY = -m_sketch2DZoom + m_sketch2DZoom * 0.05;
    if (m_gridSnapEnabled) {
        // Affichage propre : "1mm" ou "0.5mm" ou "0.1mm"
        QString snapStr;
        if (snapGrid >= 1.0 && std::abs(snapGrid - std::round(snapGrid)) < 0.01)
            snapStr = QString("%1mm").arg((int)std::round(snapGrid));
        else
            snapStr = QString("%1mm").arg(snapGrid, 0, 'f', 1);
        renderText(labelX, labelY, 0.3, snapStr + " [G]");
    } else {
        renderText(labelX, labelY, 0.3, QString("Libre 0.1mm [G]"));
    }
    
    // Indicateur de snap entité — losange orange quand accroché à un point
    if (m_hasSnapPoint) {
        double sz = m_sketch2DZoom * 0.015;  // Taille adaptative
        glColor4f(1.0f, 0.6f, 0.0f, 1.0f);  // Orange
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex3d(m_snapPoint.X(), m_snapPoint.Y() + sz, 0);
        glVertex3d(m_snapPoint.X() + sz, m_snapPoint.Y(), 0);
        glVertex3d(m_snapPoint.X(), m_snapPoint.Y() - sz, 0);
        glVertex3d(m_snapPoint.X() - sz, m_snapPoint.Y(), 0);
        glEnd();
        // Croix au centre
        glBegin(GL_LINES);
        glVertex3d(m_snapPoint.X() - sz*0.5, m_snapPoint.Y(), 0);
        glVertex3d(m_snapPoint.X() + sz*0.5, m_snapPoint.Y(), 0);
        glVertex3d(m_snapPoint.X(), m_snapPoint.Y() - sz*0.5, 0);
        glVertex3d(m_snapPoint.X(), m_snapPoint.Y() + sz*0.5, 0);
        glEnd();
        glLineWidth(1.0f);
    }
}

// Suite dans partie 2...
// Suite de Viewport3D.cpp - Partie 2

void Viewport3D::renderSketchEntities() {
    if (!m_activeSketch) return;
    
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return;
    
    glColor3f(0.0f, 0.0f, 0.0f);  // Noir
    glLineWidth(3.0f);
    
    for (const auto& entity : sketch2D->getEntities()) {
        // Construction geometry: pointillés, couleur différente
        if (entity->isConstruction()) {
            if (entity->getType() == CADEngine::SketchEntityType::Line) {
                auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
                if (line) {
                    glEnable(GL_LINE_STIPPLE);
                    glLineStipple(2, 0x00FF);  // Pointillés
                    glLineWidth(1.5f);
                    
                    // Couleur selon orientation (horizontal/vertical)
                    double dx = std::abs(line->getEnd().X() - line->getStart().X());
                    double dy = std::abs(line->getEnd().Y() - line->getStart().Y());
                    if (dx > dy) {
                        // Horizontal
                        std::string pt = "XY";
                        try { pt = m_activeSketch->getString("plane_type"); } catch(...) {}
                        if (pt == "YZ")
                            glColor4f(0.15f, 0.70f, 0.15f, 0.6f);
                        else
                            glColor4f(0.85f, 0.15f, 0.15f, 0.6f);
                    } else {
                        // Vertical
                        std::string pt = "XY";
                        try { pt = m_activeSketch->getString("plane_type"); } catch(...) {}
                        if (pt == "XY")
                            glColor4f(0.15f, 0.70f, 0.15f, 0.6f);
                        else
                            glColor4f(0.30f, 0.50f, 0.90f, 0.6f);
                    }
                    
                    float extent = m_sketch2DZoom * 2.0f;
                    gp_Pnt2d s = line->getStart(), e = line->getEnd();
                    float sx = std::max((float)s.X(), -extent), sy = std::max((float)s.Y(), -extent);
                    float ex = std::min((float)e.X(), extent), ey = std::min((float)e.Y(), extent);
                    
                    glBegin(GL_LINES);
                    glVertex3f(sx, sy, 0.08f);
                    glVertex3f(ex, ey, 0.08f);
                    glEnd();
                    
                    glDisable(GL_LINE_STIPPLE);
                    glLineWidth(3.0f);
                    glColor3f(0.0f, 0.0f, 0.0f);
                }
            }
            continue;
        }
        
        if (entity->getType() == CADEngine::SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
            if (line) {
                gp_Pnt2d start2D = line->getStart();
                gp_Pnt2d end2D = line->getEnd();
                
                glBegin(GL_LINES);
                glVertex3f(start2D.X(), start2D.Y(), 0.1f);
                glVertex3f(end2D.X(), end2D.Y(), 0.1f);
                glEnd();
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
            if (circle) {
                gp_Pnt2d center2D = circle->getCenter();
                double radius = circle->getRadius();
                
                // Dessiner le cercle
                glBegin(GL_LINE_LOOP);
                int segments = 64;
                for (int i = 0; i < segments; ++i) {
                    double angle = 2.0 * M_PI * i / segments;
                    float x = center2D.X() + radius * cos(angle);
                    float y = center2D.Y() + radius * sin(angle);
                    glVertex3f(x, y, 0.1f);
                }
                glEnd();
                
                // Dessiner croix au centre (taille fixe 5mm)
                float crossSize = 5.0f;
                glBegin(GL_LINES);
                // Ligne horizontale
                glVertex3f(center2D.X() - crossSize, center2D.Y(), 0.12f);
                glVertex3f(center2D.X() + crossSize, center2D.Y(), 0.12f);
                // Ligne verticale
                glVertex3f(center2D.X(), center2D.Y() - crossSize, 0.12f);
                glVertex3f(center2D.X(), center2D.Y() + crossSize, 0.12f);
                glEnd();
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
            if (rect) {
                auto corners = rect->getKeyPoints();
                
                glBegin(GL_LINE_LOOP);
                for (const auto& corner : corners) {
                    glVertex3f(corner.X(), corner.Y(), 0.1f);
                }
                glEnd();
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Arc) {
            auto arc = std::dynamic_pointer_cast<CADEngine::SketchArc>(entity);
            if (arc) {
                gp_Pnt2d p1 = arc->getStart();
                gp_Pnt2d p2 = arc->getMid();
                gp_Pnt2d p3 = arc->getEnd();
                
                if (arc->isBezier()) {
                    // Bézier quadratique : p2 est un point de contrôle (pas sur la courbe)
                    int segments = 32;
                    glBegin(GL_LINE_STRIP);
                    for (int i = 0; i <= segments; ++i) {
                        double t = (double)i / segments;
                        double u = 1.0 - t;
                        // B(t) = (1-t)²·P1 + 2(1-t)t·P2 + t²·P3
                        double bx = u*u*p1.X() + 2*u*t*p2.X() + t*t*p3.X();
                        double by = u*u*p1.Y() + 2*u*t*p2.Y() + t*t*p3.Y();
                        glVertex3f(bx, by, 0.1f);
                    }
                    glEnd();
                } else {
                    // Arc circulaire : p2 est un point SUR l'arc
                    double ax = p2.X() - p1.X(), ay = p2.Y() - p1.Y();
                    double bx = p3.X() - p2.X(), by = p3.Y() - p2.Y();
                    double det = 2.0 * (ax * by - ay * bx);
                    
                    if (std::abs(det) > 1e-6) {
                        double aMag = ax*ax + ay*ay;
                        double bMag = bx*bx + by*by;
                        double cx = p1.X() + (aMag * by - bMag * ay) / det;
                        double cy = p1.Y() + (bMag * ax - aMag * bx) / det;
                        double radius = std::sqrt((p1.X()-cx)*(p1.X()-cx) + (p1.Y()-cy)*(p1.Y()-cy));
                        
                        double a1 = std::atan2(p1.Y()-cy, p1.X()-cx);
                        double a2 = std::atan2(p2.Y()-cy, p2.X()-cx);
                        double a3 = std::atan2(p3.Y()-cy, p3.X()-cx);
                        
                        double sweep13 = a3 - a1;
                        while (sweep13 < -M_PI) sweep13 += 2*M_PI;
                        while (sweep13 > M_PI) sweep13 -= 2*M_PI;
                        double sweep12 = a2 - a1;
                        while (sweep12 < -M_PI) sweep12 += 2*M_PI;
                        while (sweep12 > M_PI) sweep12 -= 2*M_PI;
                        
                        double totalSweep;
                        if ((sweep13 > 0 && sweep12 > 0 && sweep12 < sweep13) ||
                            (sweep13 < 0 && sweep12 < 0 && sweep12 > sweep13)) {
                            totalSweep = sweep13;
                        } else {
                            totalSweep = (sweep13 > 0) ? sweep13 - 2*M_PI : sweep13 + 2*M_PI;
                        }
                        
                        int segments = std::max(16, (int)(std::abs(totalSweep) * 32.0 / M_PI));
                        glBegin(GL_LINE_STRIP);
                        for (int i = 0; i <= segments; ++i) {
                            double a = a1 + totalSweep * i / segments;
                            glVertex3f(cx + radius * std::cos(a), cy + radius * std::sin(a), 0.1f);
                        }
                        glEnd();
                    } else {
                        glBegin(GL_LINES);
                        glVertex3f(p1.X(), p1.Y(), 0.1f);
                        glVertex3f(p3.X(), p3.Y(), 0.1f);
                        glEnd();
                    }
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
            if (polyline) {
                auto points = polyline->getPoints();
                const auto& filletArcs = polyline->getFilletArcs();

                if (filletArcs.empty()) {
                    glBegin(GL_LINE_STRIP);
                    for (const auto& pt : points)
                        glVertex3f(pt.X(), pt.Y(), 0.1f);
                    glEnd();
                } else {
                    // Build arc lookup
                    std::map<int, const CADEngine::SketchPolyline::FilletArc*> arcMap;
                    for (const auto& fa : filletArcs) arcMap[fa.startIdx] = &fa;

                    int np = (int)points.size();
                    int idx = 0;
                    while (idx < np) {
                        auto it = arcMap.find(idx);
                        if (it != arcMap.end()) {
                            const auto& fa = *it->second;
                            int endIdx = (fa.endIdx < np) ? fa.endIdx : np - 1;
                            glBegin(GL_LINE_STRIP);
                            glVertex3f(points[idx].X(), points[idx].Y(), 0.1f);
                            const int tessN = 16;
                            for (int k = 1; k < tessN; k++) {
                                double a = fa.startAngle + (double)k / tessN * fa.angleDiff;
                                glVertex3f((float)(fa.cx + fa.radius * std::cos(a)),
                                           (float)(fa.cy + fa.radius * std::sin(a)), 0.1f);
                            }
                            glVertex3f(points[endIdx].X(), points[endIdx].Y(), 0.1f);
                            glEnd();
                            idx = endIdx;
                        } else {
                            if (idx + 1 < np) {
                                glBegin(GL_LINE_STRIP);
                                glVertex3f(points[idx].X(), points[idx].Y(), 0.1f);
                                glVertex3f(points[idx+1].X(), points[idx+1].Y(), 0.1f);
                                glEnd();
                            }
                            idx++;
                        }
                    }
                }

                // Draw draggable vertices
                glPointSize(7.0f);
                glColor3f(0.2f, 0.2f, 0.8f);
                glBegin(GL_POINTS);
                for (const auto& pt : points)
                    glVertex3f(pt.X(), pt.Y(), 0.15f);
                glEnd();
                glColor3f(0.0f, 0.0f, 0.0f);
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Ellipse) {
            auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
            if (ellipse) {
                gp_Pnt2d center = ellipse->getCenter();
                double a = ellipse->getMajorRadius();
                double b = ellipse->getMinorRadius();
                double rot = ellipse->getRotation();
                double cosR = std::cos(rot), sinR = std::sin(rot);
                
                glBegin(GL_LINE_LOOP);
                int segments = 64;
                for (int i = 0; i < segments; ++i) {
                    double t = 2.0 * M_PI * i / segments;
                    double lx = a * std::cos(t);
                    double ly = b * std::sin(t);
                    float x = center.X() + lx * cosR - ly * sinR;
                    float y = center.Y() + lx * sinR + ly * cosR;
                    glVertex3f(x, y, 0.1f);
                }
                glEnd();
                
                // Croix au centre
                float crossSize = 5.0f;
                glBegin(GL_LINES);
                glVertex3f(center.X() - crossSize, center.Y(), 0.12f);
                glVertex3f(center.X() + crossSize, center.Y(), 0.12f);
                glVertex3f(center.X(), center.Y() - crossSize, 0.12f);
                glVertex3f(center.X(), center.Y() + crossSize, 0.12f);
                glEnd();
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
            auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
            if (polygon) {
                auto verts = polygon->getVertices();
                
                glBegin(GL_LINE_LOOP);
                for (const auto& v : verts) {
                    glVertex3f(v.X(), v.Y(), 0.1f);
                }
                glEnd();
                
                // Croix au centre
                gp_Pnt2d center = polygon->getCenter();
                float crossSize = 5.0f;
                glBegin(GL_LINES);
                glVertex3f(center.X() - crossSize, center.Y(), 0.12f);
                glVertex3f(center.X() + crossSize, center.Y(), 0.12f);
                glVertex3f(center.X(), center.Y() - crossSize, 0.12f);
                glVertex3f(center.X(), center.Y() + crossSize, 0.12f);
                glEnd();
                
                // Vertices
                glPointSize(5.0f);
                glColor3f(0.2f, 0.2f, 0.8f);
                glBegin(GL_POINTS);
                for (const auto& v : verts) {
                    glVertex3f(v.X(), v.Y(), 0.15f);
                }
                glEnd();
                glColor3f(0.0f, 0.0f, 0.0f);
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
            auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
            if (oblong) {
                gp_Pnt2d center = oblong->getCenter();
                double L = oblong->getLength();
                double W = oblong->getWidth();
                double rot = oblong->getRotation();
                double r = W / 2.0;
                double halfS = (L - W) / 2.0;  // Demi-longueur de la partie droite
                
                double cosR = std::cos(rot), sinR = std::sin(rot);
                double axX = cosR, axY = sinR;      // Axe principal
                double prX = -sinR, prY = cosR;     // Perpendiculaire
                
                // 4 coins de la partie droite
                double p1x = center.X() - halfS*axX + r*prX;
                double p1y = center.Y() - halfS*axY + r*prY;
                double p2x = center.X() + halfS*axX + r*prX;
                double p2y = center.Y() + halfS*axY + r*prY;
                double p3x = center.X() + halfS*axX - r*prX;
                double p3y = center.Y() + halfS*axY - r*prY;
                double p4x = center.X() - halfS*axX - r*prX;
                double p4y = center.Y() - halfS*axY - r*prY;
                
                // Ligne haut
                glBegin(GL_LINES);
                glVertex3f(p1x, p1y, 0.1f);
                glVertex3f(p2x, p2y, 0.1f);
                glEnd();
                
                // Demi-cercle droit
                double c2x = center.X() + halfS*axX;
                double c2y = center.Y() + halfS*axY;
                double startAngle = std::atan2(prY, prX);
                glBegin(GL_LINE_STRIP);
                for (int i = 0; i <= 32; ++i) {
                    double a = startAngle - M_PI * i / 32.0;
                    glVertex3f(c2x + r*std::cos(a), c2y + r*std::sin(a), 0.1f);
                }
                glEnd();
                
                // Ligne bas
                glBegin(GL_LINES);
                glVertex3f(p3x, p3y, 0.1f);
                glVertex3f(p4x, p4y, 0.1f);
                glEnd();
                
                // Demi-cercle gauche
                double c1x = center.X() - halfS*axX;
                double c1y = center.Y() - halfS*axY;
                glBegin(GL_LINE_STRIP);
                for (int i = 0; i <= 32; ++i) {
                    double a = startAngle + M_PI - M_PI * i / 32.0;
                    glVertex3f(c1x + r*std::cos(a), c1y + r*std::sin(a), 0.1f);
                }
                glEnd();
                
                // Croix au centre
                float crossSize = 5.0f;
                glBegin(GL_LINES);
                glVertex3f(center.X() - crossSize, center.Y(), 0.12f);
                glVertex3f(center.X() + crossSize, center.Y(), 0.12f);
                glVertex3f(center.X(), center.Y() - crossSize, 0.12f);
                glVertex3f(center.X(), center.Y() + crossSize, 0.12f);
                glEnd();
            }
        }
    }
    
    // Rendu des symboles de contraintes géométriques
    {
        auto constraints = sketch2D->getConstraints();
        for (const auto& constraint : constraints) {
            if (!constraint || !constraint->isActive()) continue;
            
            std::string symbol = constraint->getSymbol();
            gp_Pnt2d symbolPos(0, 0);
            bool hasPos = false;
            
            if (constraint->getType() == CADEngine::ConstraintType::Horizontal ||
                constraint->getType() == CADEngine::ConstraintType::Vertical) {
                // Symbole au milieu du segment (ligne ou segment polyline)
                auto hc = std::dynamic_pointer_cast<CADEngine::HorizontalConstraint>(constraint);
                auto vc = std::dynamic_pointer_cast<CADEngine::VerticalConstraint>(constraint);
                
                gp_Pnt2d segStart(0,0), segEnd(0,0);
                bool valid = false;
                
                if (hc) { segStart = hc->getSegStart(); segEnd = hc->getSegEnd(); valid = true; }
                if (vc) { segStart = vc->getSegStart(); segEnd = vc->getSegEnd(); valid = true; }
                
                if (valid) {
                    double offset = CADEngine::ViewportScaling::getDimensionTextSize(m_sketch2DZoom) * 0.5;
                    symbolPos = gp_Pnt2d(
                        (segStart.X() + segEnd.X()) / 2.0,
                        (segStart.Y() + segEnd.Y()) / 2.0 + offset
                    );
                    hasPos = true;
                }
            }
            else if (constraint->getType() == CADEngine::ConstraintType::Coincident) {
                auto cc = std::dynamic_pointer_cast<CADEngine::CoincidentConstraint>(constraint);
                if (cc) {
                    gp_Pnt2d p = cc->getPoint(cc->getEntity1(), cc->getVertexIndex1());
                    symbolPos = p;
                    hasPos = true;
                    
                    // Dessiner point vert sur les points coïncidents
                    glPointSize(10.0f);
                    glColor3f(0.0f, 0.8f, 0.0f);  // Vert
                    glBegin(GL_POINTS);
                    glVertex3f(p.X(), p.Y(), 0.3f);
                    glEnd();
                    glColor3f(0.0f, 0.0f, 0.0f);
                }
            }
            else if (constraint->getType() == CADEngine::ConstraintType::Parallel) {
                auto pc = std::dynamic_pointer_cast<CADEngine::ParallelConstraint>(constraint);
                if (pc) {
                    // Symbole au milieu du segment 2 (celui contraint)
                    double offset = CADEngine::ViewportScaling::getDimensionTextSize(m_sketch2DZoom) * 0.5;
                    symbolPos = gp_Pnt2d(
                        (pc->getSeg2Start().X() + pc->getSeg2End().X()) / 2.0,
                        (pc->getSeg2Start().Y() + pc->getSeg2End().Y()) / 2.0 + offset
                    );
                    hasPos = true;
                }
            }
            else if (constraint->getType() == CADEngine::ConstraintType::Perpendicular) {
                auto pc = std::dynamic_pointer_cast<CADEngine::PerpendicularConstraint>(constraint);
                if (pc) {
                    double offset = CADEngine::ViewportScaling::getDimensionTextSize(m_sketch2DZoom) * 0.5;
                    symbolPos = gp_Pnt2d(
                        (pc->getSeg2Start().X() + pc->getSeg2End().X()) / 2.0,
                        (pc->getSeg2Start().Y() + pc->getSeg2End().Y()) / 2.0 + offset
                    );
                    hasPos = true;
                }
            }
            else if (constraint->getType() == CADEngine::ConstraintType::Fix) {
                auto lc = std::dynamic_pointer_cast<CADEngine::FixConstraint>(constraint);
                if (lc) {
                    gp_Pnt2d p = lc->getLockedPosition();
                    double offset = CADEngine::ViewportScaling::getDimensionTextSize(m_sketch2DZoom) * 0.4;
                    symbolPos = gp_Pnt2d(p.X() + offset, p.Y() + offset);
                    hasPos = true;
                    
                    // Petit carré rouge sur le point verrouillé
                    float sz = (float)(offset * 0.5);
                    glColor3f(0.9f, 0.1f, 0.1f);
                    glBegin(GL_LINE_LOOP);
                    glVertex3f(p.X() - sz, p.Y() - sz, 0.35f);
                    glVertex3f(p.X() + sz, p.Y() - sz, 0.35f);
                    glVertex3f(p.X() + sz, p.Y() + sz, 0.35f);
                    glVertex3f(p.X() - sz, p.Y() + sz, 0.35f);
                    glEnd();
                    glColor3f(0.0f, 0.0f, 0.0f);
                }
            }
            else if (constraint->getType() == CADEngine::ConstraintType::Symmetric) {
                auto sc = std::dynamic_pointer_cast<CADEngine::SymmetricConstraint>(constraint);
                if (sc) {
                    gp_Pnt2d p1 = sc->getPoint1();
                    gp_Pnt2d p2 = sc->getPoint2();
                    symbolPos = gp_Pnt2d((p1.X() + p2.X()) / 2.0,
                                          (p1.Y() + p2.Y()) / 2.0);
                    hasPos = true;
                    
                    // Ligne pointillée entre les deux points symétriques
                    glEnable(GL_LINE_STIPPLE);
                    glLineStipple(2, 0xAAAA);
                    glColor3f(0.6f, 0.0f, 0.8f);  // Violet
                    glBegin(GL_LINES);
                    glVertex3f(p1.X(), p1.Y(), 0.3f);
                    glVertex3f(p2.X(), p2.Y(), 0.3f);
                    glEnd();
                    glDisable(GL_LINE_STIPPLE);
                    
                    // Petits losanges sur les deux points
                    float d = (float)(CADEngine::ViewportScaling::getDimensionTextSize(m_sketch2DZoom) * 0.25);
                    glColor3f(0.6f, 0.0f, 0.8f);
                    for (const auto& pp : {p1, p2}) {
                        glBegin(GL_LINE_LOOP);
                        glVertex3f(pp.X(), pp.Y() - d, 0.35f);
                        glVertex3f(pp.X() + d, pp.Y(), 0.35f);
                        glVertex3f(pp.X(), pp.Y() + d, 0.35f);
                        glVertex3f(pp.X() - d, pp.Y(), 0.35f);
                        glEnd();
                    }
                    glColor3f(0.0f, 0.0f, 0.0f);
                }
            }
            else if (constraint->getType() == CADEngine::ConstraintType::CenterOnAxis) {
                auto coa = std::dynamic_pointer_cast<CADEngine::CenterOnAxisConstraint>(constraint);
                if (coa) {
                    gp_Pnt2d center = coa->getEntityCenter();
                    gp_Pnt2d proj = coa->projectOnAxis(center);
                    
                    // Direction de l'axe (normalisée)
                    gp_Pnt2d axA = coa->getAxisLine()->getStart();
                    gp_Pnt2d axB = coa->getAxisLine()->getEnd();
                    double adx = axB.X()-axA.X(), ady = axB.Y()-axA.Y();
                    double alen = std::sqrt(adx*adx+ady*ady);
                    if (alen > 1e-6) { adx /= alen; ady /= alen; }
                    
                    // Calculer le demi-gabarit de l'entité le long de l'axe
                    double halfExtent = 0;
                    auto entity = coa->getEntity();
                    if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
                        auto r = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
                        if (r) {
                            // Projeter les 4 coins sur l'axe, trouver max
                            gp_Pnt2d c = r->getCorner();
                            double w = r->getWidth(), h = r->getHeight();
                            gp_Pnt2d corners[4] = {c, gp_Pnt2d(c.X()+w,c.Y()), gp_Pnt2d(c.X()+w,c.Y()+h), gp_Pnt2d(c.X(),c.Y()+h)};
                            for (int i = 0; i < 4; i++) {
                                double d = std::abs((corners[i].X()-proj.X())*adx + (corners[i].Y()-proj.Y())*ady);
                                if (d > halfExtent) halfExtent = d;
                            }
                        }
                    } else if (entity->getType() == CADEngine::SketchEntityType::Circle) {
                        auto ci = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
                        if (ci) halfExtent = ci->getRadius();
                    } else {
                        halfExtent = 15.0; // fallback
                    }
                    
                    // Position du symbole : au bord de l'entité + petit offset
                    float offset = halfExtent + 8.0;
                    gp_Pnt2d symPt(proj.X() + adx * offset, proj.Y() + ady * offset);
                    symbolPos = symPt;
                    hasPos = true;
                    
                    float d = (float)(CADEngine::ViewportScaling::getDimensionTextSize(m_sketch2DZoom) * 0.3);
                    
                    // Symbole de symétrie : deux triangles miroir + ligne médiane
                    glColor3f(0.1f, 0.6f, 0.9f);
                    glLineWidth(2.0f);
                    glBegin(GL_LINE_LOOP);
                    glVertex3f(symPt.X()-d*0.3f, symPt.Y()-d, 0.35f);
                    glVertex3f(symPt.X()-d*1.2f, symPt.Y(), 0.35f);
                    glVertex3f(symPt.X()-d*0.3f, symPt.Y()+d, 0.35f);
                    glEnd();
                    glBegin(GL_LINE_LOOP);
                    glVertex3f(symPt.X()+d*0.3f, symPt.Y()-d, 0.35f);
                    glVertex3f(symPt.X()+d*1.2f, symPt.Y(), 0.35f);
                    glVertex3f(symPt.X()+d*0.3f, symPt.Y()+d, 0.35f);
                    glEnd();
                    glBegin(GL_LINES);
                    glVertex3f(symPt.X(), symPt.Y()-d*1.3f, 0.35f);
                    glVertex3f(symPt.X(), symPt.Y()+d*1.3f, 0.35f);
                    glEnd();
                    glLineWidth(3.0f);
                    glColor3f(0.0f, 0.0f, 0.0f);
                    
                    symbol = "\xE2\x87\x94";  // ⇔
                }
            }
            
            if (hasPos) {
                renderText(symbolPos.X(), symbolPos.Y(), 0.4, 
                          QString::fromStdString(symbol));
            }
        }
    }
    
    // Highlight extrémités au survol
    if (!m_hoveredEndpoints.empty()) {
        glPointSize(10.0f);
        glColor3f(0.0f, 1.0f, 1.0f);  // Cyan vif
        glBegin(GL_POINTS);
        for (const auto& pt : m_hoveredEndpoints) {
            glVertex3f(pt.X(), pt.Y(), 0.3f);  // Z élevé pour être visible
        }
        glEnd();
    }
    
    // Highlight premier point sélectionné pour contrainte coïncidente
    if (m_currentTool == SketchTool::ConstraintCoincident && m_firstEndpointForCoincident.entity) {
        glPointSize(14.0f);
        glColor3f(0.0f, 1.0f, 0.0f);  // Vert vif
        glBegin(GL_POINTS);
        glVertex3f(m_firstEndpointForCoincident.point.X(), 
                   m_firstEndpointForCoincident.point.Y(), 0.35f);
        glEnd();
        
        // Cercle autour du point sélectionné
        double r = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom) * 0.5;
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 24; i++) {
            double a = 2.0 * M_PI * i / 24.0;
            glVertex3f(m_firstEndpointForCoincident.point.X() + r * cos(a),
                       m_firstEndpointForCoincident.point.Y() + r * sin(a), 0.35f);
        }
        glEnd();
    }
    
    // Highlight premier segment sélectionné pour contrainte parallèle/perpendiculaire
    if ((m_currentTool == SketchTool::ConstraintParallel || 
         m_currentTool == SketchTool::ConstraintPerpendicular) && m_firstEntityForConstraint) {
        glLineWidth(4.0f);
        glColor3f(0.0f, 1.0f, 0.0f);  // Vert vif
        
        gp_Pnt2d p1(0,0), p2(0,0);
        bool valid = false;
        
        if (m_firstEntityForConstraint->getType() == CADEngine::SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(m_firstEntityForConstraint);
            if (line) { p1 = line->getStart(); p2 = line->getEnd(); valid = true; }
        } else if (m_firstEntityForConstraint->getType() == CADEngine::SketchEntityType::Polyline) {
            auto poly = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(m_firstEntityForConstraint);
            if (poly && m_firstSegmentIndexForConstraint >= 0) {
                auto pts = poly->getPoints();
                int idx = m_firstSegmentIndexForConstraint;
                if (idx + 1 < (int)pts.size()) { p1 = pts[idx]; p2 = pts[idx+1]; valid = true; }
            }
        }
        
        if (valid) {
            glBegin(GL_LINES);
            glVertex3f(p1.X(), p1.Y(), 0.35f);
            glVertex3f(p2.X(), p2.Y(), 0.35f);
            glEnd();
        }
        glColor3f(0.0f, 0.0f, 0.0f);
    }
    
    // Highlight points sélectionnés pour contrainte symétrique
    if (m_currentTool == SketchTool::ConstraintSymmetric && m_symmetricState.step > 0) {
        // Highlight de l'entité sélectionnée (step 1 = en attente de l'axe)
        if (m_symmetricState.entity1) {
            glColor4f(0.6f, 0.0f, 0.8f, 0.4f);  // Violet semi-transparent
            glLineWidth(4.0f);
            // Redessiner l'entité source en surbrillance
            auto src = m_symmetricState.entity1;
            if (src->getType() == CADEngine::SketchEntityType::Rectangle) {
                auto r = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(src);
                if (r) {
                    gp_Pnt2d c = r->getCorner();
                    double w = r->getWidth(), h = r->getHeight();
                    glBegin(GL_LINE_LOOP);
                    glVertex3f(c.X(), c.Y(), 0.35f);
                    glVertex3f(c.X()+w, c.Y(), 0.35f);
                    glVertex3f(c.X()+w, c.Y()+h, 0.35f);
                    glVertex3f(c.X(), c.Y()+h, 0.35f);
                    glEnd();
                }
            } else if (src->getType() == CADEngine::SketchEntityType::Circle) {
                auto ci = std::dynamic_pointer_cast<CADEngine::SketchCircle>(src);
                if (ci) {
                    glBegin(GL_LINE_LOOP);
                    for (int i = 0; i < 48; i++) {
                        double a = 2.0*M_PI*i/48.0;
                        glVertex3f(ci->getCenter().X()+ci->getRadius()*cos(a),
                                   ci->getCenter().Y()+ci->getRadius()*sin(a), 0.35f);
                    }
                    glEnd();
                }
            } else if (src->getType() == CADEngine::SketchEntityType::Line) {
                auto l = std::dynamic_pointer_cast<CADEngine::SketchLine>(src);
                if (l) {
                    glBegin(GL_LINES);
                    glVertex3f(l->getStart().X(), l->getStart().Y(), 0.35f);
                    glVertex3f(l->getEnd().X(), l->getEnd().Y(), 0.35f);
                    glEnd();
                }
            }
            glLineWidth(3.0f);
        }
        glColor3f(0.0f, 0.0f, 0.0f);
    }
    
    // Highlight premier point sélectionné pour cotation point-à-point
    if (m_currentTool == SketchTool::Dimension && m_hasDimensionFirstPoint) {
        glPointSize(14.0f);
        glColor3f(0.0f, 0.9f, 0.3f);  // Vert vif
        glBegin(GL_POINTS);
        glVertex3f(m_firstDimensionEndpoint.point.X(), 
                   m_firstDimensionEndpoint.point.Y(), 0.35f);
        glEnd();
        
        // Petit cercle autour du point pour le rendre plus visible
        float r = CADEngine::ViewportScaling::getDimensionTextSize(m_sketch2DZoom) * 0.35f;
        glColor3f(0.0f, 0.8f, 0.2f);
        glLineWidth(1.5f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 16; i++) {
            float a = i * 2.0f * M_PI / 16.0f;
            glVertex3f(m_firstDimensionEndpoint.point.X() + r * cos(a),
                       m_firstDimensionEndpoint.point.Y() + r * sin(a), 0.35f);
        }
        glEnd();
        glColor3f(0.0f, 0.0f, 0.0f);
    }
    
    // Highlight de la 1ère ligne sélectionnée en mode Ctrl+clic (distance entre lignes)
    if (m_currentTool == SketchTool::Dimension && m_firstLineForDistance.entity) {
        glLineWidth(3.0f);
        glColor3f(0.0f, 0.9f, 0.3f);  // Vert vif
        glBegin(GL_LINES);
        glVertex3f(m_firstLineForDistance.startPt.X(), m_firstLineForDistance.startPt.Y(), 0.35f);
        glVertex3f(m_firstLineForDistance.endPt.X(), m_firstLineForDistance.endPt.Y(), 0.35f);
        glEnd();
        glLineWidth(1.0f);
        glColor3f(0.0f, 0.0f, 0.0f);
    }
    
    // Highlight de la 1ère ligne sélectionnée en mode Shift+clic (angle)
    if (m_currentTool == SketchTool::Dimension && m_firstSegForAngle.entity) {
        glLineWidth(3.0f);
        glColor3f(0.9f, 0.6f, 0.0f);  // Orange
        glBegin(GL_LINES);
        glVertex3f(m_firstSegForAngle.startPt.X(), m_firstSegForAngle.startPt.Y(), 0.35f);
        glVertex3f(m_firstSegForAngle.endPt.X(), m_firstSegForAngle.endPt.Y(), 0.35f);
        glEnd();
        glLineWidth(1.0f);
        glColor3f(0.0f, 0.0f, 0.0f);
    }
    
    // Highlight vertex en cours de drag
    if (m_isDraggingVertex && m_draggedEntity && 
        m_draggedEntity->getType() == CADEngine::SketchEntityType::Polyline) {
        auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(m_draggedEntity);
        if (polyline && m_draggedVertexIndex >= 0) {
            auto points = polyline->getPoints();
            if (m_draggedVertexIndex < (int)points.size()) {
                glPointSize(12.0f);
                glColor3f(1.0f, 1.0f, 0.0f);  // Jaune vif
                glBegin(GL_POINTS);
                glVertex3f(points[m_draggedVertexIndex].X(), 
                           points[m_draggedVertexIndex].Y(), 0.35f);
                glEnd();
            }
        }
    }
    
    glLineWidth(1.0f);
}

// ============================================================================
// Helper : Affichage angle + longueur pendant l'édition
// ============================================================================

// Dessine un arc d'angle et affiche la valeur entre deux directions
// origin: point d'intersection des deux segments
// refAngle: angle (rad) du segment de référence (précédent ou axe X)
// curAngle: angle (rad) du segment courant
// arcRadius: rayon de l'arc indicateur (en unités monde)
void Viewport3D::renderAngleIndicator(const gp_Pnt2d& origin, 
                                       double refAngle, double curAngle,
                                       double arcRadius) {
    // ── Angle intérieur au sommet ──
    // refAngle = direction du segment PRÉCÉDENT (A→B)  
    // curAngle = direction du segment COURANT (B→C)
    // Angle intérieur = angle entre vecteurs B→A et B→C
    double baAngle = refAngle + M_PI;  // Direction B→A (inverse du segment précédent)
    
    // Angle de B→A vers B→C (sens trigonométrique, normalisé)
    double sweep = curAngle - baAngle;
    while (sweep > M_PI)  sweep -= 2.0 * M_PI;
    while (sweep < -M_PI) sweep += 2.0 * M_PI;
    
    double interiorDeg = std::abs(sweep) * 180.0 / M_PI;
    
    // ── Prolongement du segment précédent en pointillé ──
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(2, 0xCCCC);
    glColor4f(0.6f, 0.6f, 0.6f, 0.6f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex3f(origin.X(), origin.Y(), 0.25f);
    glVertex3f(origin.X() + arcRadius * 1.5 * cos(refAngle),
               origin.Y() + arcRadius * 1.5 * sin(refAngle), 0.25f);
    glEnd();
    glDisable(GL_LINE_STIPPLE);
    
    // ── Arc d'angle (de B→A vers B→C) ──
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.85f, 1.0f, 0.8f);
    glLineWidth(1.5f);
    
    int segs = std::max(12, (int)(std::abs(sweep) * 20.0 / M_PI));
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= segs; ++i) {
        double a = baAngle + sweep * i / segs;
        glVertex3f(origin.X() + arcRadius * cos(a),
                   origin.Y() + arcRadius * sin(a), 0.25f);
    }
    glEnd();
    
    // Marques aux extrémités
    float tick = arcRadius * 0.15f;
    glBegin(GL_LINES);
    glVertex3f(origin.X() + (arcRadius - tick) * cos(baAngle),
               origin.Y() + (arcRadius - tick) * sin(baAngle), 0.25f);
    glVertex3f(origin.X() + (arcRadius + tick) * cos(baAngle),
               origin.Y() + (arcRadius + tick) * sin(baAngle), 0.25f);
    glVertex3f(origin.X() + (arcRadius - tick) * cos(curAngle),
               origin.Y() + (arcRadius - tick) * sin(curAngle), 0.25f);
    glVertex3f(origin.X() + (arcRadius + tick) * cos(curAngle),
               origin.Y() + (arcRadius + tick) * sin(curAngle), 0.25f);
    glEnd();
    
    glDisable(GL_BLEND);
    
    // ── Texte angle au milieu de l'arc ──
    double midA = baAngle + sweep / 2.0;
    renderText(origin.X() + arcRadius * 1.5 * cos(midA),
               origin.Y() + arcRadius * 1.5 * sin(midA), 0.3,
               QString("%1%2").arg(interiorDeg, 0, 'f', 1).arg(QChar(0x00B0)));
    
    // Restaurer
    glColor3f(1.0f, 0.5f, 0.0f);
    glLineWidth(2.0f);
}

void Viewport3D::renderTempEntity() {
    if (!m_activeSketch) return;
    // NE PAS return si m_tempPoints est vide - on veut quand même changer les états OpenGL !
    
    glLineWidth(2.0f);
    glColor3f(1.0f, 0.5f, 0.0f);  // Orange
    
    // ── Guide visuel snap H/V ──
    if (m_hvSnapActive) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(3, 0xF0F0);
        glLineWidth(1.0f);
        glColor4f(0.0f, 0.9f, 0.4f, 0.7f);  // Vert clair
        
        float guideLen = m_sketch2DZoom * 0.6f;
        glBegin(GL_LINES);
        if (m_hvSnapHorizontal) {
            glVertex3f(m_hvSnapOrigin.X() - guideLen, m_hvSnapOrigin.Y(), 0.15f);
            glVertex3f(m_hvSnapOrigin.X() + guideLen, m_hvSnapOrigin.Y(), 0.15f);
        } else {
            glVertex3f(m_hvSnapOrigin.X(), m_hvSnapOrigin.Y() - guideLen, 0.15f);
            glVertex3f(m_hvSnapOrigin.X(), m_hvSnapOrigin.Y() + guideLen, 0.15f);
        }
        glEnd();
        
        glDisable(GL_LINE_STIPPLE);
        glLineWidth(2.0f);
        glColor3f(1.0f, 0.5f, 0.0f);  // Retour orange
    }
    
    // Toujours afficher les points déjà placés
    if (!m_tempPoints.empty()) {
        glPointSize(8.0f);
        glBegin(GL_POINTS);
        for (const auto& pt : m_tempPoints) {
            glVertex3f(pt.X(), pt.Y(), 0.2f);
        }
        glEnd();
    }
    
    // Preview selon l'outil et le nombre de points
    if (m_currentTool == SketchTool::Line && m_tempPoints.size() >= 2) {
        // Ligne preview
        glBegin(GL_LINES);
        glVertex3f(m_tempPoints[0].X(), m_tempPoints[0].Y(), 0.2f);
        glVertex3f(m_tempPoints[1].X(), m_tempPoints[1].Y(), 0.2f);
        glEnd();
        
        // Longueur
        double length = m_tempPoints[0].Distance(m_tempPoints[1]);
        gp_Pnt2d midPt((m_tempPoints[0].X() + m_tempPoints[1].X()) / 2.0,
                       (m_tempPoints[0].Y() + m_tempPoints[1].Y()) / 2.0);
        
        // Angle du segment courant par rapport à l'axe X
        double dx = m_tempPoints[1].X() - m_tempPoints[0].X();
        double dy = m_tempPoints[1].Y() - m_tempPoints[0].Y();
        double curAngle = std::atan2(dy, dx);
        
        // Chercher le segment précédent dans le sketch pour l'angle relatif
        double refAngle = 0.0;  // Par défaut: axe X (horizontal)
        bool hasPrevSegment = false;
        
        if (m_activeSketch && m_activeSketch->getSketch2D()) {
            auto entities = m_activeSketch->getSketch2D()->getEntities();
            // Chercher la dernière ligne dont un endpoint == notre point de départ
            for (int i = (int)entities.size() - 1; i >= 0; --i) {
                if (entities[i]->getType() == CADEngine::SketchEntityType::Line) {
                    auto prevLine = std::dynamic_pointer_cast<CADEngine::SketchLine>(entities[i]);
                    if (!prevLine) continue;
                    gp_Pnt2d ps = prevLine->getStart();
                    gp_Pnt2d pe = prevLine->getEnd();
                    double tol = 0.5;
                    
                    if (m_tempPoints[0].Distance(pe) < tol) {
                        // Ce segment finit où le nôtre commence → direction = start→end
                        refAngle = std::atan2(pe.Y() - ps.Y(), pe.X() - ps.X());
                        hasPrevSegment = true;
                        break;
                    } else if (m_tempPoints[0].Distance(ps) < tol) {
                        // Ce segment commence où le nôtre commence → direction inversée
                        refAngle = std::atan2(ps.Y() - pe.Y(), ps.X() - pe.X());
                        hasPrevSegment = true;
                        break;
                    }
                }
            }
        }
        
        // Afficher longueur au milieu du segment
        renderText(midPt.X(), midPt.Y(), 0.3, QString("L=%1").arg(length, 0, 'f', 1));
        
        // Arc d'angle (rayon adapté au zoom)
        if (length > 1.0) {
            double arcR = std::min(length * 0.3, m_sketch2DZoom * 0.08);
            renderAngleIndicator(m_tempPoints[0], refAngle, curAngle, arcR);
        }
    }
    else if (m_currentTool == SketchTool::Circle && m_tempPoints.size() >= 2) {
        // Cercle preview
        gp_Pnt2d center = m_tempPoints[0];
        double rawRadius = center.Distance(m_tempPoints[1]);
        
        // Snap rayon pour le preview (même logique que le finish)
        double snapGrid = CADEngine::ViewportScaling::getSnapGridSize(m_sketch2DZoom);
        double radius = std::round(rawRadius / snapGrid) * snapGrid;
        if (radius < snapGrid) radius = rawRadius;  // Pas de cercle nul
        
        glBegin(GL_LINE_LOOP);
        int segments = 64;
        for (int i = 0; i < segments; ++i) {
            double angle = 2.0 * M_PI * i / segments;
            float x = center.X() + radius * cos(angle);
            float y = center.Y() + radius * sin(angle);
            glVertex3f(x, y, 0.2f);
        }
        glEnd();
        
        // Rayon preview
        glBegin(GL_LINES);
        glVertex3f(center.X(), center.Y(), 0.2f);
        glVertex3f(m_tempPoints[1].X(), m_tempPoints[1].Y(), 0.2f);
        glEnd();
        
        // Dimension — afficher diamètre (plus utile que rayon)
        renderText(m_tempPoints[1].X(), m_tempPoints[1].Y(), 0.3, 
                   QString("\xC3\x98%1").arg(radius * 2.0, 0, 'f', 2));
    }
    else if (m_currentTool == SketchTool::Rectangle && m_tempPoints.size() >= 2) {
        // Rectangle preview
        double x1 = std::min(m_tempPoints[0].X(), m_tempPoints[1].X());
        double y1 = std::min(m_tempPoints[0].Y(), m_tempPoints[1].Y());
        double x2 = std::max(m_tempPoints[0].X(), m_tempPoints[1].X());
        double y2 = std::max(m_tempPoints[0].Y(), m_tempPoints[1].Y());
        
        glBegin(GL_LINE_LOOP);
        glVertex3f(x1, y1, 0.2f);
        glVertex3f(x2, y1, 0.2f);
        glVertex3f(x2, y2, 0.2f);
        glVertex3f(x1, y2, 0.2f);
        glEnd();
        
        // Dimensions
        double width = x2 - x1;
        double height = y2 - y1;
        renderText((x1 + x2) / 2, y1 - 10, 0.3, QString("%1").arg(width, 0, 'f', 1));
        renderText(x2 + 10, (y1 + y2) / 2, 0.3, QString("%1").arg(height, 0, 'f', 1));
    }
    else if (m_currentTool == SketchTool::Arc && m_tempPoints.size() >= 1) {
        // Arc Bézier preview selon nombre de points
        if (m_tempPoints.size() == 1) {
            // 1 point : ligne jusqu'au hover
            glBegin(GL_LINES);
            glVertex3f(m_tempPoints[0].X(), m_tempPoints[0].Y(), 0.2f);
            if (m_hasHoverPoint) {
                glVertex3f(m_hoverPoint.X(), m_hoverPoint.Y(), 0.2f);
            }
            glEnd();
            
            renderText(m_tempPoints[0].X(), m_tempPoints[0].Y(), 0.3, "Arc Bézier - Point de fin");
        }
        else if (m_tempPoints.size() == 2) {
            // 2 points (start + end) : Bézier avec point de contrôle ajustable par hover
            gp_Pnt2d p1 = m_tempPoints[0];
            gp_Pnt2d p3 = m_tempPoints[1];
            
            if (m_hasHoverPoint) {
                // Le hover EST le point de contrôle Bézier directement
                gp_Pnt2d p2 = m_hoverPoint;
                
                // Dessiner la courbe Bézier quadratique : B(t) = (1-t)²P1 + 2(1-t)tP2 + t²P3
                int segs = 32;
                glBegin(GL_LINE_STRIP);
                for (int i = 0; i <= segs; ++i) {
                    double t = (double)i / segs;
                    double u = 1.0 - t;
                    double bx = u*u*p1.X() + 2*u*t*p2.X() + t*t*p3.X();
                    double by = u*u*p1.Y() + 2*u*t*p2.Y() + t*t*p3.Y();
                    glVertex3f(bx, by, 0.2f);
                }
                glEnd();
                
                // Lignes de contrôle (tangentes)
                glEnable(GL_LINE_STIPPLE);
                glLineStipple(2, 0xAAAA);
                glBegin(GL_LINE_STRIP);
                glVertex3f(p1.X(), p1.Y(), 0.2f);
                glVertex3f(p2.X(), p2.Y(), 0.2f);
                glVertex3f(p3.X(), p3.Y(), 0.2f);
                glEnd();
                glDisable(GL_LINE_STIPPLE);
                
                // Point de contrôle visible
                glPointSize(6.0f);
                glColor3f(0.0f, 1.0f, 0.0f);  // Vert
                glBegin(GL_POINTS);
                glVertex3f(p2.X(), p2.Y(), 0.2f);
                glEnd();
                glColor3f(1.0f, 0.5f, 0.0f);  // Retour orange
                
                renderText(p2.X(), p2.Y() + 5, 0.3, "Ctrl point");
            } else {
                // Pas de hover → ligne droite
                glBegin(GL_LINES);
                glVertex3f(p1.X(), p1.Y(), 0.2f);
                glVertex3f(p3.X(), p3.Y(), 0.2f);
                glEnd();
            }
        }
    }
    else if (m_currentTool == SketchTool::Polyline && m_tempPoints.size() >= 1) {
        // Polyligne preview - afficher tous les segments + hover
        glBegin(GL_LINE_STRIP);
        for (const auto& pt : m_tempPoints) {
            glVertex3f(pt.X(), pt.Y(), 0.2f);
        }
        if (m_hasHoverPoint) {
            glVertex3f(m_hoverPoint.X(), m_hoverPoint.Y(), 0.2f);
        }
        glEnd();
        
        // ── Longueur + angle du segment en cours ──
        gp_Pnt2d lastPt = m_tempPoints.back();
        gp_Pnt2d endPt = m_hasHoverPoint ? m_hoverPoint : lastPt;
        double segLen = lastPt.Distance(endPt);
        
        if (m_hasHoverPoint && segLen > 0.5) {
            // Longueur au milieu du segment courant
            gp_Pnt2d mid((lastPt.X() + endPt.X()) / 2.0,
                         (lastPt.Y() + endPt.Y()) / 2.0);
            renderText(mid.X(), mid.Y(), 0.3, QString("L=%1").arg(segLen, 0, 'f', 1));
            
            // Angle du segment courant
            double curAngle = std::atan2(endPt.Y() - lastPt.Y(),
                                         endPt.X() - lastPt.X());
            
            // Référence: segment précédent ou axe X
            double refAngle = 0.0;
            if (m_tempPoints.size() >= 2) {
                gp_Pnt2d prevPt = m_tempPoints[m_tempPoints.size() - 2];
                refAngle = std::atan2(lastPt.Y() - prevPt.Y(),
                                      lastPt.X() - prevPt.X());
            }
            
            // Arc d'angle
            double arcR = std::min(segLen * 0.3, m_sketch2DZoom * 0.08);
            renderAngleIndicator(lastPt, refAngle, curAngle, arcR);
        }
        
        renderText(m_tempPoints[0].X(), m_tempPoints[0].Y() + m_sketch2DZoom * 0.04, 0.3, 
                   QString("Polyline (%1 pts)").arg(m_tempPoints.size()));
    }
    else if (m_currentTool == SketchTool::Ellipse) {
        if (m_tempPoints.size() >= 2) {
            gp_Pnt2d center = m_tempPoints[0];
            double dx = m_tempPoints[1].X() - center.X();
            double dy = m_tempPoints[1].Y() - center.Y();
            double majorR = std::sqrt(dx*dx + dy*dy);
            double rot = std::atan2(dy, dx);
            
            double minorR = majorR * 0.6;  // Ratio par défaut
            if (m_tempPoints.size() >= 3) {
                double perpX = -std::sin(rot), perpY = std::cos(rot);
                double px = m_tempPoints[2].X() - center.X();
                double py = m_tempPoints[2].Y() - center.Y();
                minorR = std::abs(px * perpX + py * perpY);
                if (minorR < 1.0) minorR = 1.0;
                if (minorR > majorR) minorR = majorR;
            }
            
            double cosR = std::cos(rot), sinR = std::sin(rot);
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 64; ++i) {
                double t = 2.0 * M_PI * i / 64;
                double lx = majorR * std::cos(t);
                double ly = minorR * std::sin(t);
                glVertex3f(center.X() + lx*cosR - ly*sinR,
                           center.Y() + lx*sinR + ly*cosR, 0.2f);
            }
            glEnd();
            
            // Axes
            glBegin(GL_LINES);
            glVertex3f(center.X(), center.Y(), 0.2f);
            glVertex3f(m_tempPoints[1].X(), m_tempPoints[1].Y(), 0.2f);
            glEnd();
            
            renderText(center.X(), center.Y() - 10, 0.3,
                       QString("a=%1 b=%2").arg(majorR, 0, 'f', 1).arg(minorR, 0, 'f', 1));
        }
    }
    else if (m_currentTool == SketchTool::Polygon && m_tempPoints.size() >= 1) {
        gp_Pnt2d center = m_tempPoints[0];
        double radius = 0, rotation = 0;
        
        if (m_tempPoints.size() >= 2) {
            double dx = m_tempPoints[1].X() - center.X();
            double dy = m_tempPoints[1].Y() - center.Y();
            radius = std::sqrt(dx*dx + dy*dy);
            rotation = std::atan2(dy, dx);
        }
        
        if (radius > 0.5) {
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < m_polygonSides; ++i) {
                double angle = rotation + 2.0 * M_PI * i / m_polygonSides;
                glVertex3f(center.X() + radius * std::cos(angle),
                           center.Y() + radius * std::sin(angle), 0.2f);
            }
            glEnd();
            
            // Rayon
            glBegin(GL_LINES);
            glVertex3f(center.X(), center.Y(), 0.2f);
            glVertex3f(m_tempPoints[1].X(), m_tempPoints[1].Y(), 0.2f);
            glEnd();
            
            renderText(center.X(), center.Y() - 10, 0.3,
                       QString("%1 côtés R=%2").arg(m_polygonSides).arg(radius, 0, 'f', 1));
        }
    }
    else if (m_currentTool == SketchTool::ArcCenter && m_tempPoints.size() >= 1) {
        gp_Pnt2d center = m_tempPoints[0];
        
        if (m_tempPoints.size() == 1 && m_hasHoverPoint) {
            // Preview: cercle complet en pointillé + rayon
            double radius = center.Distance(m_hoverPoint);
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(2, 0xAAAA);
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 64; ++i) {
                double angle = 2.0 * M_PI * i / 64;
                glVertex3f(center.X() + radius * std::cos(angle),
                           center.Y() + radius * std::sin(angle), 0.2f);
            }
            glEnd();
            glDisable(GL_LINE_STIPPLE);
            
            glBegin(GL_LINES);
            glVertex3f(center.X(), center.Y(), 0.2f);
            glVertex3f(m_hoverPoint.X(), m_hoverPoint.Y(), 0.2f);
            glEnd();
            renderText(m_hoverPoint.X(), m_hoverPoint.Y(), 0.3,
                       QString("R=%1").arg(radius, 0, 'f', 1));
        }
        else if (m_tempPoints.size() == 2) {
            gp_Pnt2d startPt = m_tempPoints[1];
            double radius = center.Distance(startPt);
            double startAngle = std::atan2(startPt.Y() - center.Y(), startPt.X() - center.X());
            
            // Cercle complet en pointillé
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(2, 0xAAAA);
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 64; ++i) {
                double angle = 2.0 * M_PI * i / 64;
                glVertex3f(center.X() + radius * std::cos(angle),
                           center.Y() + radius * std::sin(angle), 0.2f);
            }
            glEnd();
            glDisable(GL_LINE_STIPPLE);
            
            // Arc preview jusqu'au hover (utilise le sweep accumulé)
            if (m_hasHoverPoint && m_arcCenterTracking) {
                double sweep = m_arcCenterAccumSweep;
                if (std::abs(sweep) < 0.01) sweep = 0.01;  // Minimum visible
                
                glLineWidth(3.0f);
                glColor3f(0.0f, 1.0f, 0.5f);  // Vert clair
                glBegin(GL_LINE_STRIP);
                int segs = std::max(16, (int)(std::abs(sweep) * 32 / (2.0 * M_PI)));
                for (int i = 0; i <= segs; ++i) {
                    double a = startAngle + sweep * i / segs;
                    glVertex3f(center.X() + radius * std::cos(a),
                               center.Y() + radius * std::sin(a), 0.2f);
                }
                glEnd();
                glColor3f(1.0f, 0.5f, 0.0f);
                glLineWidth(2.0f);
                
                double angleDeg = std::abs(sweep) * 180.0 / M_PI;
                renderText(center.X(), center.Y() - 10, 0.3,
                           QString("Arc %1°").arg(angleDeg, 0, 'f', 1));
            }
            
            // Rayons
            glBegin(GL_LINES);
            glVertex3f(center.X(), center.Y(), 0.2f);
            glVertex3f(startPt.X(), startPt.Y(), 0.2f);
            glEnd();
        }
    }
    else if (m_currentTool == SketchTool::Oblong && m_tempPoints.size() >= 1) {
        gp_Pnt2d center = m_tempPoints[0];
        gp_Pnt2d endPt = m_hasHoverPoint ? m_hoverPoint : 
                          (m_tempPoints.size() >= 2 ? m_tempPoints[1] : center);
        
        double dx = endPt.X() - center.X();
        double dy = endPt.Y() - center.Y();
        double halfL = std::sqrt(dx*dx + dy*dy);
        double rot = std::atan2(dy, dx);
        
        if (halfL > 0.5) {
            double length = halfL * 2.0;
            double width;
            
            if (m_tempPoints.size() >= 2 && m_hasHoverPoint) {
                // 3ème point: largeur depuis hover
                double perpX = -std::sin(rot), perpY = std::cos(rot);
                double px = m_hoverPoint.X() - center.X();
                double py = m_hoverPoint.Y() - center.Y();
                width = std::abs(px * perpX + py * perpY) * 2.0;
                if (width < 2.0) width = 2.0;
                if (width > length) width = length;
            } else {
                width = length / 3.0;
            }
            
            double r = width / 2.0;
            double halfS = (length - width) / 2.0;
            double cosR = std::cos(rot), sinR = std::sin(rot);
            double axX = cosR, axY = sinR;
            double prX = -sinR, prY = cosR;
            
            double p1x = center.X() - halfS*axX + r*prX;
            double p1y = center.Y() - halfS*axY + r*prY;
            double p2x = center.X() + halfS*axX + r*prX;
            double p2y = center.Y() + halfS*axY + r*prY;
            double p3x = center.X() + halfS*axX - r*prX;
            double p3y = center.Y() + halfS*axY - r*prY;
            double p4x = center.X() - halfS*axX - r*prX;
            double p4y = center.Y() - halfS*axY - r*prY;
            
            // Ligne haut
            glBegin(GL_LINES);
            glVertex3f(p1x, p1y, 0.2f); glVertex3f(p2x, p2y, 0.2f);
            glEnd();
            // Demi-cercle droit
            double c2x = center.X() + halfS*axX, c2y = center.Y() + halfS*axY;
            double sa = std::atan2(prY, prX);
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= 32; ++i) {
                double a = sa - M_PI * i / 32.0;
                glVertex3f(c2x + r*std::cos(a), c2y + r*std::sin(a), 0.2f);
            }
            glEnd();
            // Ligne bas
            glBegin(GL_LINES);
            glVertex3f(p3x, p3y, 0.2f); glVertex3f(p4x, p4y, 0.2f);
            glEnd();
            // Demi-cercle gauche
            double c1x = center.X() - halfS*axX, c1y = center.Y() - halfS*axY;
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= 32; ++i) {
                double a = sa + M_PI - M_PI * i / 32.0;
                glVertex3f(c1x + r*std::cos(a), c1y + r*std::sin(a), 0.2f);
            }
            glEnd();
            
            // Axe central pointillé
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(2, 0xAAAA);
            glBegin(GL_LINES);
            glVertex3f(center.X(), center.Y(), 0.2f);
            glVertex3f(endPt.X(), endPt.Y(), 0.2f);
            glEnd();
            glDisable(GL_LINE_STIPPLE);
            
            renderText(center.X(), center.Y() - 10, 0.3,
                       QString("L=%1 l=%2").arg(length, 0, 'f', 1).arg(width, 0, 'f', 1));
        }
    }
    
    // Rendu des dimensions (cotations)
    if (m_activeSketch) {
        auto sketch2D = m_activeSketch->getSketch2D();
        if (sketch2D) {
            // Cotations manuelles (bleu)
            renderDimensions(sketch2D->getDimensions());
            
            // Cotations automatiques (vert)
            sketch2D->regenerateAutoDimensions();
            auto autoDims = sketch2D->getAutoDimensions();
            if (!autoDims.empty()) {
                renderAutoDimensions(autoDims);
            }
        }
    }
    
    glLineWidth(1.0f);
}


// ============================================================================
// Rendu texte simplifié (sans Qt - juste pour debug)
// ============================================================================

void Viewport3D::renderDimensions(const std::vector<std::shared_ptr<CADEngine::Dimension>>& dimensions) {
    if (dimensions.empty()) {
        return;
    }
    
    // Désactiver depth test
    glDisable(GL_DEPTH_TEST);
    
    // BLEU pour les cotations
    glColor3f(0.0f, 0.6f, 1.0f);
    glLineWidth(2.0f);
    
    for (const auto& dim : dimensions) {
        if (!dim || !dim->isVisible()) continue;
        
        if (dim->getType() == CADEngine::DimensionType::Linear) {
            auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
            if (linearDim) {
                renderLinearDimension(linearDim);
            }
        }
        else if (dim->getType() == CADEngine::DimensionType::Diametral) {
            auto diametralDim = std::dynamic_pointer_cast<CADEngine::DiametralDimension>(dim);
            if (diametralDim) {
                renderDiametralDimension(diametralDim);
            }
        }
        else if (dim->getType() == CADEngine::DimensionType::Angular) {
            auto angularDim = std::dynamic_pointer_cast<CADEngine::AngularDimension>(dim);
            if (angularDim) {
                renderAngularDimension(angularDim);
            }
        }
    }
    
    // Réactiver depth test
    glEnable(GL_DEPTH_TEST);
}

void Viewport3D::renderLinearDimension(std::shared_ptr<CADEngine::LinearDimension> dim) {
    gp_Pnt2d p1 = dim->getPoint1();
    gp_Pnt2d p2 = dim->getPoint2();
    
    float z = 0.25f;  // Z plus élevé pour être au-dessus des entités (0.1f) et du preview (0.2f)
    
    // Lignes d'attache (extension lines)
    gp_Pnt2d ext1Start = dim->getExtensionLine1Start();
    gp_Pnt2d ext1End = dim->getExtensionLine1End();
    gp_Pnt2d ext2Start = dim->getExtensionLine2Start();
    gp_Pnt2d ext2End = dim->getExtensionLine2End();
    
    glBegin(GL_LINES);
    // Extension line 1
    glVertex3f(ext1Start.X(), ext1Start.Y(), z);
    glVertex3f(ext1End.X(), ext1End.Y(), z);
    // Extension line 2
    glVertex3f(ext2Start.X(), ext2Start.Y(), z);
    glVertex3f(ext2End.X(), ext2End.Y(), z);
    glEnd();
    
    // Ligne de cote (dimension line)
    gp_Pnt2d dimLineStart = dim->getDimensionLineStart();
    gp_Pnt2d dimLineEnd = dim->getDimensionLineEnd();
    
    glBegin(GL_LINES);
    glVertex3f(dimLineStart.X(), dimLineStart.Y(), z);
    glVertex3f(dimLineEnd.X(), dimLineEnd.Y(), z);
    glEnd();
    
    // Flèches adaptatives au zoom
    double arrowSize = CADEngine::ViewportScaling::getDimensionArrowSize(m_sketch2DZoom);
    double dx = dimLineEnd.X() - dimLineStart.X();
    double dy = dimLineEnd.Y() - dimLineStart.Y();
    double len = std::sqrt(dx*dx + dy*dy);
    
    if (len > 1e-6) {
        // Vecteurs directeur et perpendiculaire
        double dirX = dx / len;
        double dirY = dy / len;
        double perpX = -dirY;
        double perpY = dirX;
        
        // Flèche début
        glBegin(GL_TRIANGLES);
        glVertex3f(dimLineStart.X(), dimLineStart.Y(), z);
        glVertex3f(dimLineStart.X() + dirX * arrowSize - perpX * arrowSize/2, 
                   dimLineStart.Y() + dirY * arrowSize - perpY * arrowSize/2, z);
        glVertex3f(dimLineStart.X() + dirX * arrowSize + perpX * arrowSize/2, 
                   dimLineStart.Y() + dirY * arrowSize + perpY * arrowSize/2, z);
        glEnd();
        
        // Flèche fin
        glBegin(GL_TRIANGLES);
        glVertex3f(dimLineEnd.X(), dimLineEnd.Y(), z);
        glVertex3f(dimLineEnd.X() - dirX * arrowSize - perpX * arrowSize/2, 
                   dimLineEnd.Y() - dirY * arrowSize - perpY * arrowSize/2, z);
        glVertex3f(dimLineEnd.X() - dirX * arrowSize + perpX * arrowSize/2, 
                   dimLineEnd.Y() - dirY * arrowSize + perpY * arrowSize/2, z);
        glEnd();
    }
    
    // Texte
    gp_Pnt2d textPos = dim->getTextPosition();
    renderText(textPos.X(), textPos.Y(), z + 0.01f, QString::fromStdString(dim->getText()));
}

void Viewport3D::renderDiametralDimension(std::shared_ptr<CADEngine::DiametralDimension> dim) {
    gp_Pnt2d center = dim->getCenter();
    double diameter = dim->getDiameter();
    double radius = diameter / 2.0;
    double angle = dim->getAngle();  // Angle de la ligne de cote (défaut 45°)
    
    float z = 0.25f;
    
    // Calculer les points
    double angleRad = angle * M_PI / 180.0;
    double textOffset = 20.0;  // Même offset que dans Dimension.cpp
    
    // Point de départ : bord du cercle (côté opposé au texte)
    gp_Pnt2d start(center.X() - radius * cos(angleRad), 
                   center.Y() - radius * sin(angleRad));
    
    // Point d'arrivée : juste avant le texte
    gp_Pnt2d end(center.X() + (radius + textOffset - 5.0) * cos(angleRad), 
                 center.Y() + (radius + textOffset - 5.0) * sin(angleRad));
    
    // Ligne de cote (du bord du cercle jusqu'au texte)
    glBegin(GL_LINES);
    glVertex3f(start.X(), start.Y(), z);
    glVertex3f(end.X(), end.Y(), z);
    glEnd();
    
    // Flèche à la fin seulement (pointe vers le texte)
    double arrowSize = 5.0;
    double dirX = cos(angleRad);
    double dirY = sin(angleRad);
    double perpX = -dirY;
    double perpY = dirX;
    
    glBegin(GL_TRIANGLES);
    glVertex3f(end.X(), end.Y(), z);
    glVertex3f(end.X() - dirX * arrowSize - perpX * arrowSize/2, 
               end.Y() - dirY * arrowSize - perpY * arrowSize/2, z);
    glVertex3f(end.X() - dirX * arrowSize + perpX * arrowSize/2, 
               end.Y() - dirY * arrowSize + perpY * arrowSize/2, z);
    glEnd();
    
    // Texte
    gp_Pnt2d textPos = dim->getTextPosition();
    renderText(textPos.X(), textPos.Y(), z + 0.01f, QString::fromStdString(dim->getText()));
}

void Viewport3D::renderAngularDimension(std::shared_ptr<CADEngine::AngularDimension> dim) {
    if (!dim) return;
    
    gp_Pnt2d center = dim->getCenter();
    double radius = dim->getRadius();
    float z = 0.05f;
    
    double startAngle = dim->getStartAngle();
    double sweep = dim->getSweep();
    
    if (sweep < 1e-6) return;  // Pas d'angle à dessiner
    
    // Arc de cotation (sens antihoraire depuis startAngle sur sweep radians)
    glColor3f(0.0f, 0.7f, 1.0f);  // Bleu clair
    glBegin(GL_LINE_STRIP);
    int segments = 50;
    for (int i = 0; i <= segments; i++) {
        double t = (double)i / segments;
        double angle = startAngle + t * sweep;
        double x = center.X() + radius * std::cos(angle);
        double y = center.Y() + radius * std::sin(angle);
        glVertex3f(x, y, z);
    }
    glEnd();
    
    // Lignes de repère (du point vers l'arc)
    gp_Pnt2d point1 = dim->getPoint1();
    gp_Pnt2d point2 = dim->getPoint2();
    
    double dist1 = center.Distance(point1);
    double dist2 = center.Distance(point2);
    double a1 = dim->getAngle1();
    double a2 = dim->getAngle2();
    
    // Extension line 1
    if (dist1 < radius) {
        glBegin(GL_LINES);
        glVertex3f(point1.X(), point1.Y(), z);
        glVertex3f(center.X() + radius * std::cos(a1), 
                   center.Y() + radius * std::sin(a1), z);
        glEnd();
    }
    
    // Extension line 2
    if (dist2 < radius) {
        glBegin(GL_LINES);
        glVertex3f(point2.X(), point2.Y(), z);
        glVertex3f(center.X() + radius * std::cos(a2), 
                   center.Y() + radius * std::sin(a2), z);
        glEnd();
    }
    
    // Flèches aux extrémités de l'arc
    double arrowSize = CADEngine::ViewportScaling::getDimensionTextSize(m_sketch2DZoom) * 0.4;
    
    // Flèche début (tangente à l'arc au point startAngle)
    double arcStart_x = center.X() + radius * std::cos(startAngle);
    double arcStart_y = center.Y() + radius * std::sin(startAngle);
    // Tangente au cercle = perpendiculaire au rayon, vers l'intérieur de l'arc
    double tanStartX = -std::sin(startAngle);
    double tanStartY = std::cos(startAngle);
    
    glBegin(GL_TRIANGLES);
    glVertex3f(arcStart_x, arcStart_y, z);
    glVertex3f(arcStart_x + arrowSize * tanStartX - arrowSize * 0.3 * std::cos(startAngle),
               arcStart_y + arrowSize * tanStartY - arrowSize * 0.3 * std::sin(startAngle), z);
    glVertex3f(arcStart_x + arrowSize * tanStartX + arrowSize * 0.3 * std::cos(startAngle),
               arcStart_y + arrowSize * tanStartY + arrowSize * 0.3 * std::sin(startAngle), z);
    glEnd();
    
    // Flèche fin (tangente à l'arc au point startAngle + sweep)
    double endAngle = startAngle + sweep;
    double arcEnd_x = center.X() + radius * std::cos(endAngle);
    double arcEnd_y = center.Y() + radius * std::sin(endAngle);
    double tanEndX = std::sin(endAngle);   // Direction opposée (pointe vers le début)
    double tanEndY = -std::cos(endAngle);
    
    glBegin(GL_TRIANGLES);
    glVertex3f(arcEnd_x, arcEnd_y, z);
    glVertex3f(arcEnd_x + arrowSize * tanEndX - arrowSize * 0.3 * std::cos(endAngle),
               arcEnd_y + arrowSize * tanEndY - arrowSize * 0.3 * std::sin(endAngle), z);
    glVertex3f(arcEnd_x + arrowSize * tanEndX + arrowSize * 0.3 * std::cos(endAngle),
               arcEnd_y + arrowSize * tanEndY + arrowSize * 0.3 * std::sin(endAngle), z);
    glEnd();
    
    // Texte
    gp_Pnt2d textPos = dim->getTextPosition();
    renderText(textPos.X(), textPos.Y(), z + 0.01f, QString::fromStdString(dim->getText()));
}

// ============================================================================
// Rendu cotation radiale (R...)
// ============================================================================

void Viewport3D::renderRadialDimension(std::shared_ptr<CADEngine::RadialDimension> dim) {
    if (!dim) return;
    
    gp_Pnt2d center = dim->getCenter();
    gp_Pnt2d arrow = dim->getArrowPoint();
    float z = 0.25f;
    
    // Ligne du centre vers le point flèche
    glBegin(GL_LINES);
    glVertex3f(center.X(), center.Y(), z);
    glVertex3f(arrow.X(), arrow.Y(), z);
    glEnd();
    
    // Flèche
    double arrowSize = CADEngine::ViewportScaling::getDimensionArrowSize(m_sketch2DZoom);
    double dx = arrow.X() - center.X();
    double dy = arrow.Y() - center.Y();
    double len = std::sqrt(dx*dx + dy*dy);
    
    if (len > 1e-6) {
        double dirX = dx / len;
        double dirY = dy / len;
        double perpX = -dirY;
        double perpY = dirX;
        
        glBegin(GL_TRIANGLES);
        glVertex3f(arrow.X(), arrow.Y(), z);
        glVertex3f(arrow.X() - dirX * arrowSize - perpX * arrowSize/2,
                   arrow.Y() - dirY * arrowSize - perpY * arrowSize/2, z);
        glVertex3f(arrow.X() - dirX * arrowSize + perpX * arrowSize/2,
                   arrow.Y() - dirY * arrowSize + perpY * arrowSize/2, z);
        glEnd();
    }
    
    // Texte
    gp_Pnt2d textPos = dim->getTextPosition();
    renderText(textPos.X(), textPos.Y(), z + 0.01f, QString::fromStdString(dim->getText()));
}

// ============================================================================
// Rendu auto-cotations (vert, plus fin)
// ============================================================================

void Viewport3D::renderAutoDimensions(const std::vector<std::shared_ptr<CADEngine::Dimension>>& dimensions) {
    if (dimensions.empty()) return;
    
    glDisable(GL_DEPTH_TEST);
    glLineWidth(1.5f);
    
    for (const auto& dim : dimensions) {
        if (!dim || !dim->isVisible()) continue;
        
        // Vert pour auto-cotations (reset avant chaque car angular peut changer)
        glColor3f(0.1f, 0.65f, 0.1f);
        
        if (dim->getType() == CADEngine::DimensionType::Linear) {
            auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
            if (linearDim) renderLinearDimension(linearDim);
        }
        else if (dim->getType() == CADEngine::DimensionType::Diametral) {
            auto diametralDim = std::dynamic_pointer_cast<CADEngine::DiametralDimension>(dim);
            if (diametralDim) renderDiametralDimension(diametralDim);
        }
        else if (dim->getType() == CADEngine::DimensionType::Angular) {
            auto angularDim = std::dynamic_pointer_cast<CADEngine::AngularDimension>(dim);
            if (angularDim) renderAngularDimension(angularDim);
        }
        else if (dim->getType() == CADEngine::DimensionType::Radial) {
            auto radialDim = std::dynamic_pointer_cast<CADEngine::RadialDimension>(dim);
            if (radialDim) renderRadialDimension(radialDim);
        }
    }
    
    glEnable(GL_DEPTH_TEST);
}

void Viewport3D::renderText(double x, double y, double z, const QString& text) {
    // Stocker le label pour affichage après le rendu OpenGL
    m_textLabels.push_back({x, y, z, text});
}

// ============================================================================
// Détection clic sur dimensions
// ============================================================================

std::shared_ptr<CADEngine::SketchEntity> Viewport3D::findEntityAtPoint(const gp_Pnt2d& pt) {
    if (!m_activeSketch) return nullptr;
    
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return nullptr;
    
    double tolerance = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom);
    
    // Lambda pour tester si un point est sur une entité
    auto testEntity = [&](const std::shared_ptr<CADEngine::SketchEntity>& entity) -> bool {
        
        if (entity->getType() == CADEngine::SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
            if (!line) return false;
            gp_Pnt2d start = line->getStart(), end = line->getEnd();
            double dx = end.X()-start.X(), dy = end.Y()-start.Y();
            double len2 = dx*dx+dy*dy;
            if (len2 < 1e-6) return false;
            double t = ((pt.X()-start.X())*dx + (pt.Y()-start.Y())*dy) / len2;
            t = std::max(0.0, std::min(1.0, t));
            double px = start.X()+t*dx, py = start.Y()+t*dy;
            return std::sqrt((pt.X()-px)*(pt.X()-px)+(pt.Y()-py)*(pt.Y()-py)) < tolerance;
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
            if (!circle) return false;
            double d = pt.Distance(circle->getCenter());
            return (d < tolerance) || (std::abs(d - circle->getRadius()) < tolerance);
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
            if (!rect) return false;
            gp_Pnt2d c = rect->getCorner();
            double w = rect->getWidth(), h = rect->getHeight();
            // Test si clic à l'intérieur du rectangle
            if (pt.X() >= c.X() && pt.X() <= c.X()+w &&
                pt.Y() >= c.Y() && pt.Y() <= c.Y()+h) return true;
            // Test sur les 4 bords
            gp_Pnt2d corners[4] = {c, gp_Pnt2d(c.X()+w,c.Y()), gp_Pnt2d(c.X()+w,c.Y()+h), gp_Pnt2d(c.X(),c.Y()+h)};
            for (int i = 0; i < 4; i++) {
                gp_Pnt2d s = corners[i], e = corners[(i+1)%4];
                double dx = e.X()-s.X(), dy = e.Y()-s.Y();
                double len2 = dx*dx+dy*dy;
                if (len2 < 1e-6) continue;
                double t = ((pt.X()-s.X())*dx + (pt.Y()-s.Y())*dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                double px = s.X()+t*dx, py = s.Y()+t*dy;
                if (std::sqrt((pt.X()-px)*(pt.X()-px)+(pt.Y()-py)*(pt.Y()-py)) < tolerance) return true;
            }
            return false;
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
            if (!polyline) return false;
            auto points = polyline->getPoints();
            for (size_t i = 0; i+1 < points.size(); i++) {
                gp_Pnt2d s = points[i], e = points[i+1];
                double dx = e.X()-s.X(), dy = e.Y()-s.Y();
                double len2 = dx*dx+dy*dy;
                if (len2 < 1e-6) continue;
                double t = ((pt.X()-s.X())*dx + (pt.Y()-s.Y())*dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                double px = s.X()+t*dx, py = s.Y()+t*dy;
                if (std::sqrt((pt.X()-px)*(pt.X()-px)+(pt.Y()-py)*(pt.Y()-py)) < tolerance) return true;
            }
            return false;
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Arc) {
            auto arc = std::dynamic_pointer_cast<CADEngine::SketchArc>(entity);
            if (!arc) return false;
            gp_Pnt2d start = arc->getStart(), mid = arc->getMid(), end = arc->getEnd();
            if (arc->isBezier()) {
                double minDist = 1e12;
                for (int i = 0; i <= 32; ++i) {
                    double t = (double)i/32.0, u = 1.0-t;
                    double bx = u*u*start.X()+2*u*t*mid.X()+t*t*end.X();
                    double by = u*u*start.Y()+2*u*t*mid.Y()+t*t*end.Y();
                    double d = std::sqrt((pt.X()-bx)*(pt.X()-bx)+(pt.Y()-by)*(pt.Y()-by));
                    if (d < minDist) minDist = d;
                }
                return minDist < tolerance;
            } else {
                double ax2 = mid.X()-start.X(), ay2 = mid.Y()-start.Y();
                double bx2 = end.X()-mid.X(), by2 = end.Y()-mid.Y();
                double d2 = 2*(ax2*by2-ay2*bx2);
                if (std::abs(d2) < 1e-6) return false;
                double aM = ax2*ax2+ay2*ay2, bM = bx2*bx2+by2*by2;
                double cx = start.X()+(aM*by2-bM*ay2)/d2;
                double cy = start.Y()+(bM*ax2-aM*bx2)/d2;
                double r = std::sqrt((start.X()-cx)*(start.X()-cx)+(start.Y()-cy)*(start.Y()-cy));
                double dc = std::sqrt((pt.X()-cx)*(pt.X()-cx)+(pt.Y()-cy)*(pt.Y()-cy));
                return std::abs(dc-r) < tolerance;
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Ellipse) {
            auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
            if (!ellipse) return false;
            gp_Pnt2d center = ellipse->getCenter();
            if (pt.Distance(center) < tolerance) return true;
            double a = ellipse->getMajorRadius(), b = ellipse->getMinorRadius(), rot = ellipse->getRotation();
            double dx = pt.X()-center.X(), dy = pt.Y()-center.Y();
            double cosR = std::cos(-rot), sinR = std::sin(-rot);
            double lx = dx*cosR-dy*sinR, ly = dx*sinR+dy*cosR;
            double normalized = (lx*lx)/(a*a)+(ly*ly)/(b*b);
            return std::abs(std::sqrt(normalized)-1.0)*std::min(a,b) < tolerance;
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
            auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
            if (!polygon) return false;
            if (pt.Distance(polygon->getCenter()) < tolerance) return true;
            auto verts = polygon->getVertices();
            for (size_t i = 0; i < verts.size(); i++) {
                gp_Pnt2d s = verts[i], e = verts[(i+1)%verts.size()];
                double dx = e.X()-s.X(), dy = e.Y()-s.Y();
                double len2 = dx*dx+dy*dy;
                if (len2 < 1e-6) continue;
                double t = ((pt.X()-s.X())*dx+(pt.Y()-s.Y())*dy)/len2;
                t = std::max(0.0, std::min(1.0, t));
                double px = s.X()+t*dx, py = s.Y()+t*dy;
                if (std::sqrt((pt.X()-px)*(pt.X()-px)+(pt.Y()-py)*(pt.Y()-py)) < tolerance) return true;
            }
            return false;
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
            auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
            if (!oblong) return false;
            if (pt.Distance(oblong->getCenter()) < tolerance) return true;
            gp_Pnt2d center = oblong->getCenter();
            double rot = oblong->getRotation();
            double cosR = std::cos(-rot), sinR = std::sin(-rot);
            double lx = (pt.X()-center.X())*cosR-(pt.Y()-center.Y())*sinR;
            double ly = (pt.X()-center.X())*sinR+(pt.Y()-center.Y())*cosR;
            double halfS = (oblong->getLength()-oblong->getWidth())/2.0;
            double r = oblong->getWidth()/2.0;
            if (std::abs(lx) <= halfS && std::abs(std::abs(ly)-r) < tolerance) return true;
            if (lx > halfS) { double d = std::sqrt((lx-halfS)*(lx-halfS)+ly*ly); if (std::abs(d-r) < tolerance) return true; }
            if (lx < -halfS) { double d = std::sqrt((lx+halfS)*(lx+halfS)+ly*ly); if (std::abs(d-r) < tolerance) return true; }
            return false;
        }
        return false;
    };
    
    // PASSE 1 : entités normales (priorité haute)
    for (const auto& entity : sketch2D->getEntities()) {
        if (entity->isConstruction()) continue;
        if (testEntity(entity)) return entity;
    }
    
    // PASSE 2 : entités de construction (axes) — seulement si aucune entité normale trouvée
    for (const auto& entity : sketch2D->getEntities()) {
        if (!entity->isConstruction()) continue;
        if (testEntity(entity)) return entity;
    }
    
    return nullptr;
}

void Viewport3D::handleAutoDimension(std::shared_ptr<CADEngine::SketchEntity> entity, const gp_Pnt2d& clickPoint) {
    if (!entity || !m_activeSketch) return;
    
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return;
    
    std::shared_ptr<CADEngine::Dimension> dimension;
    
    // === LIGNES → cotation linéaire ===
    if (entity->getType() == CADEngine::SketchEntityType::Line) {
        auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
        if (line) {
            auto linDim = std::make_shared<CADEngine::LinearDimension>(
                line->getStart(), line->getEnd()
            );
            // Attacher les sources pour refresh au drag
            linDim->setSourceEntity1(entity, -1);  // start
            linDim->setSourceEntity2(entity, -2);  // end
            dimension = linDim;
            emit statusMessage(QString("Cotation créée: %1 mm")
                .arg(linDim->getValue(), 0, 'f', 1));
        }
    }
    else if (entity->getType() == CADEngine::SketchEntityType::Circle) {
        // Cercle → Cotation diamètre
        auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
        if (circle) {
            double diameter = circle->getRadius() * 2.0;  // IMPORTANT: diamètre !
            dimension = std::make_shared<CADEngine::DiametralDimension>(
                circle->getCenter(), diameter
            );
            emit statusMessage(QString("Cotation diamètre créée: Ø%1 mm")
                .arg(diameter, 0, 'f', 1));
        }
    }
    else if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
        // Polyligne → Cotation du segment le plus proche du clic
        auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
        if (polyline) {
            auto points = polyline->getPoints();
            if (points.size() >= 2) {
                double minDist = 1e10;
                size_t closestSegment = 0;
                
                // Trouver segment le plus proche du point de clic
                for (size_t i = 0; i < points.size() - 1; i++) {
                    gp_Pnt2d p1 = points[i];
                    gp_Pnt2d p2 = points[i + 1];
                    
                    // Distance point-segment
                    double dx = p2.X() - p1.X();
                    double dy = p2.Y() - p1.Y();
                    double len2 = dx*dx + dy*dy;
                    
                    if (len2 < 1e-6) continue;
                    
                    double t = ((clickPoint.X() - p1.X()) * dx + (clickPoint.Y() - p1.Y()) * dy) / len2;
                    t = std::max(0.0, std::min(1.0, t));
                    
                    double projX = p1.X() + t * dx;
                    double projY = p1.Y() + t * dy;
                    
                    double dist = std::sqrt((clickPoint.X() - projX) * (clickPoint.X() - projX) +
                                           (clickPoint.Y() - projY) * (clickPoint.Y() - projY));
                    
                    if (dist < minDist) {
                        minDist = dist;
                        closestSegment = i;
                    }
                }
                
                // Créer dimension pour ce segment
                gp_Pnt2d segStart = points[closestSegment];
                gp_Pnt2d segEnd = points[closestSegment + 1];
                
                dimension = std::make_shared<CADEngine::LinearDimension>(segStart, segEnd);
                
                // STOCKER l'ID de la polyligne et l'index du segment
                dimension->setEntityId(polyline->getId());
                dimension->setSegmentIndex(closestSegment);
                
                // Attacher sources pour refresh au drag
                auto linDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dimension);
                if (linDim) {
                    linDim->setSourceEntity1(entity, (int)closestSegment);
                    linDim->setSourceEntity2(entity, (int)closestSegment + 1);
                }
                
                emit statusMessage(QString("Cotation segment %1: %2 mm")
                    .arg(closestSegment + 1)
                    .arg(dimension->getValue(), 0, 'f', 1));
            }
        }
    }
    else if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
        // Rectangle → Cotation du côté le plus proche du clic
        auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
        if (rect) {
            gp_Pnt2d corner = rect->getCorner();
            double width = rect->getWidth();
            double height = rect->getHeight();
            
            // Les 4 coins
            gp_Pnt2d p1 = corner;
            gp_Pnt2d p2(corner.X() + width, corner.Y());
            gp_Pnt2d p3(corner.X() + width, corner.Y() + height);
            gp_Pnt2d p4(corner.X(), corner.Y() + height);
            
            // Les 4 côtés
            std::vector<std::pair<gp_Pnt2d, gp_Pnt2d>> sides = {
                {p1, p2}, {p2, p3}, {p3, p4}, {p4, p1}
            };
            
            // Trouver le côté le plus proche du clic
            double minDist = 1e10;
            int closestSide = 0;
            
            for (size_t i = 0; i < sides.size(); i++) {
                gp_Pnt2d start = sides[i].first;
                gp_Pnt2d end = sides[i].second;
                
                double dx = end.X() - start.X();
                double dy = end.Y() - start.Y();
                double len2 = dx*dx + dy*dy;
                
                if (len2 < 1e-6) continue;
                
                double t = ((clickPoint.X() - start.X()) * dx + (clickPoint.Y() - start.Y()) * dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                
                double projX = start.X() + t * dx;
                double projY = start.Y() + t * dy;
                
                double dist = std::sqrt((clickPoint.X() - projX) * (clickPoint.X() - projX) +
                                       (clickPoint.Y() - projY) * (clickPoint.Y() - projY));
                
                if (dist < minDist) {
                    minDist = dist;
                    closestSide = i;
                }
            }
            
            // Créer dimension pour ce côté
            auto linDim = std::make_shared<CADEngine::LinearDimension>(
                sides[closestSide].first, 
                sides[closestSide].second
            );
            // Attacher sources : coins du rectangle (indices 0-3, cyclique)
            int vtxA = closestSide;
            int vtxB = (closestSide + 1) % 4;
            linDim->setSourceEntity1(entity, vtxA);
            linDim->setSourceEntity2(entity, vtxB);
            dimension = linDim;
            
            const char* sideNames[] = {"bas", "droit", "haut", "gauche"};
            emit statusMessage(QString("Cotation côté %1: %2 mm")
                .arg(sideNames[closestSide])
                .arg(dimension->getValue(), 0, 'f', 1));
        }
    }
    else if (entity->getType() == CADEngine::SketchEntityType::Ellipse) {
        // Ellipse → Cotation du grand axe (diamètre)
        auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
        if (ellipse) {
            double majorDiam = ellipse->getMajorRadius() * 2.0;
            dimension = std::make_shared<CADEngine::DiametralDimension>(
                ellipse->getCenter(), majorDiam
            );
            // Orienter le texte selon la rotation de l'ellipse
            auto diamDim = std::dynamic_pointer_cast<CADEngine::DiametralDimension>(dimension);
            if (diamDim) {
                diamDim->setAngle(ellipse->getRotation() * 180.0 / M_PI);
            }
            emit statusMessage(QString("Cotation ellipse: Ø%1 mm (grand axe)")
                .arg(majorDiam, 0, 'f', 1));
        }
    }
    else if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
        // Polygone → Cotation du côté le plus proche du clic
        auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
        if (polygon) {
            auto verts = polygon->getVertices();
            double minDist = 1e10;
            int closestSide = 0;
            
            for (size_t i = 0; i < verts.size(); i++) {
                gp_Pnt2d p1 = verts[i];
                gp_Pnt2d p2 = verts[(i + 1) % verts.size()];
                
                double dx = p2.X() - p1.X();
                double dy = p2.Y() - p1.Y();
                double len2 = dx*dx + dy*dy;
                if (len2 < 1e-6) continue;
                
                double t = ((clickPoint.X() - p1.X()) * dx + (clickPoint.Y() - p1.Y()) * dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                double projX = p1.X() + t * dx;
                double projY = p1.Y() + t * dy;
                double dist = std::sqrt((clickPoint.X()-projX)*(clickPoint.X()-projX) +
                                       (clickPoint.Y()-projY)*(clickPoint.Y()-projY));
                if (dist < minDist) { minDist = dist; closestSide = (int)i; }
            }
            
            gp_Pnt2d sideStart = verts[closestSide];
            gp_Pnt2d sideEnd = verts[(closestSide + 1) % verts.size()];
            auto linDim = std::make_shared<CADEngine::LinearDimension>(sideStart, sideEnd);
            dimension = linDim;
            
            emit statusMessage(QString("Cotation côté %1/%2: %3 mm")
                .arg(closestSide + 1).arg(polygon->getSides())
                .arg(dimension->getValue(), 0, 'f', 1));
        }
    }
    else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
        // Oblong → Cotation de la longueur totale
        auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
        if (oblong) {
            auto kp = oblong->getKeyPoints();
            // kp[1] = extrémité droite, kp[2] = extrémité gauche
            if (kp.size() >= 3) {
                auto linDim = std::make_shared<CADEngine::LinearDimension>(kp[1], kp[2]);
                dimension = linDim;
                emit statusMessage(QString("Cotation oblong: L=%1 mm")
                    .arg(oblong->getLength(), 0, 'f', 1));
            }
        }
    }
    
    if (dimension) {
        emit dimensionCreated(dimension, sketch2D);
        emit sketchEntityAdded();
        m_tempPoints.clear();  // Réinitialiser
        update();
    }
}

// ============================================================================
// Gestion des contraintes géométriques
// ============================================================================

void Viewport3D::handleConstraintClick(const gp_Pnt2d& pt) {
    if (!m_activeSketch) return;
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return;
    
    // Trouver l'entité cliquée
    auto clickedEntity = findEntityAtPoint(pt);
    
    // CONTRAINTES 1 ENTITÉ : Horizontal, Vertical
    if (m_currentTool == SketchTool::ConstraintHorizontal) {
        if (!clickedEntity) {
            emit statusMessage("Cliquez sur une ligne ou un segment de polyline !");
            return;
        }
        
        // === CAS 1 : SketchLine ===
        if (clickedEntity->getType() == CADEngine::SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(clickedEntity);
            
            // Supprimer contraintes conflictuelles sur cette ligne
            auto constraints = sketch2D->getConstraints();
            for (const auto& c : constraints) {
                if (c->getType() == CADEngine::ConstraintType::Vertical) {
                    auto vc = std::dynamic_pointer_cast<CADEngine::VerticalConstraint>(c);
                    if (vc && vc->matchesLine(line)) {
                        sketch2D->removeConstraint(c);
                        emit statusMessage("Contrainte verticale supprimée");
                        break;
                    }
                } else if (c->getType() == CADEngine::ConstraintType::Horizontal) {
                    auto hc = std::dynamic_pointer_cast<CADEngine::HorizontalConstraint>(c);
                    if (hc && hc->matchesLine(line)) {
                        emit statusMessage("Contrainte horizontale déjà présente - ignorée");
                        setSketchTool(SketchTool::None);
                        return;
                    }
                }
            }
            
            auto constraint = std::make_shared<CADEngine::HorizontalConstraint>(line);
            sketch2D->addConstraint(constraint);
            sketch2D->solveConstraints();
            update();
            emit statusMessage("Contrainte horizontale appliquée !");
            setSketchTool(SketchTool::None);
            return;
        }
        
        // === CAS 2 : Polyline → trouver le segment cliqué ===
        if (clickedEntity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(clickedEntity);
            auto points = polyline->getPoints();
            
            // Trouver le segment le plus proche du clic
            double minDist = 1e10;
            int closestSeg = -1;
            for (size_t i = 0; i < points.size() - 1; i++) {
                gp_Pnt2d segStart = points[i];
                gp_Pnt2d segEnd = points[i + 1];
                double dx = segEnd.X() - segStart.X();
                double dy = segEnd.Y() - segStart.Y();
                double len2 = dx*dx + dy*dy;
                if (len2 < 1e-6) continue;
                double t = ((pt.X() - segStart.X()) * dx + (pt.Y() - segStart.Y()) * dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                double projX = segStart.X() + t * dx;
                double projY = segStart.Y() + t * dy;
                double dist = std::sqrt((pt.X()-projX)*(pt.X()-projX) + (pt.Y()-projY)*(pt.Y()-projY));
                if (dist < minDist) { minDist = dist; closestSeg = (int)i; }
            }
            
            if (closestSeg < 0) {
                emit statusMessage("Segment introuvable !");
                return;
            }
            
            // Vérifier contraintes conflictuelles sur ce segment
            auto constraints = sketch2D->getConstraints();
            for (const auto& c : constraints) {
                if (c->getType() == CADEngine::ConstraintType::Vertical) {
                    auto vc = std::dynamic_pointer_cast<CADEngine::VerticalConstraint>(c);
                    if (vc && vc->matchesPolylineSegment(polyline, closestSeg)) {
                        sketch2D->removeConstraint(c);
                        emit statusMessage("Contrainte verticale supprimée sur segment");
                        break;
                    }
                } else if (c->getType() == CADEngine::ConstraintType::Horizontal) {
                    auto hc = std::dynamic_pointer_cast<CADEngine::HorizontalConstraint>(c);
                    if (hc && hc->matchesPolylineSegment(polyline, closestSeg)) {
                        emit statusMessage("Contrainte horizontale déjà présente sur ce segment");
                        setSketchTool(SketchTool::None);
                        return;
                    }
                }
            }
            
            auto constraint = std::make_shared<CADEngine::HorizontalConstraint>(polyline, closestSeg);
            sketch2D->addConstraint(constraint);
            sketch2D->solveConstraints();
            update();
            emit statusMessage(QString("Contrainte horizontale appliquée au segment %1 !").arg(closestSeg));
            setSketchTool(SketchTool::None);
            return;
        }
        
        emit statusMessage("Cliquez sur une ligne ou un segment de polyline !");
        return;
    }
    
    if (m_currentTool == SketchTool::ConstraintVertical) {
        if (!clickedEntity) {
            emit statusMessage("Cliquez sur une ligne ou un segment de polyline !");
            return;
        }
        
        // === CAS 1 : SketchLine ===
        if (clickedEntity->getType() == CADEngine::SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(clickedEntity);
            
            auto constraints = sketch2D->getConstraints();
            for (const auto& c : constraints) {
                if (c->getType() == CADEngine::ConstraintType::Horizontal) {
                    auto hc = std::dynamic_pointer_cast<CADEngine::HorizontalConstraint>(c);
                    if (hc && hc->matchesLine(line)) {
                        sketch2D->removeConstraint(c);
                        emit statusMessage("Contrainte horizontale supprimée");
                        break;
                    }
                } else if (c->getType() == CADEngine::ConstraintType::Vertical) {
                    auto vc = std::dynamic_pointer_cast<CADEngine::VerticalConstraint>(c);
                    if (vc && vc->matchesLine(line)) {
                        emit statusMessage("Contrainte verticale déjà présente - ignorée");
                        setSketchTool(SketchTool::None);
                        return;
                    }
                }
            }
            
            auto constraint = std::make_shared<CADEngine::VerticalConstraint>(line);
            sketch2D->addConstraint(constraint);
            sketch2D->solveConstraints();
            update();
            emit statusMessage("Contrainte verticale appliquée !");
            setSketchTool(SketchTool::None);
            return;
        }
        
        // === CAS 2 : Polyline → trouver le segment cliqué ===
        if (clickedEntity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(clickedEntity);
            auto points = polyline->getPoints();
            
            double minDist = 1e10;
            int closestSeg = -1;
            for (size_t i = 0; i < points.size() - 1; i++) {
                gp_Pnt2d segStart = points[i];
                gp_Pnt2d segEnd = points[i + 1];
                double dx = segEnd.X() - segStart.X();
                double dy = segEnd.Y() - segStart.Y();
                double len2 = dx*dx + dy*dy;
                if (len2 < 1e-6) continue;
                double t = ((pt.X() - segStart.X()) * dx + (pt.Y() - segStart.Y()) * dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                double projX = segStart.X() + t * dx;
                double projY = segStart.Y() + t * dy;
                double dist = std::sqrt((pt.X()-projX)*(pt.X()-projX) + (pt.Y()-projY)*(pt.Y()-projY));
                if (dist < minDist) { minDist = dist; closestSeg = (int)i; }
            }
            
            if (closestSeg < 0) {
                emit statusMessage("Segment introuvable !");
                return;
            }
            
            auto constraints = sketch2D->getConstraints();
            for (const auto& c : constraints) {
                if (c->getType() == CADEngine::ConstraintType::Horizontal) {
                    auto hc = std::dynamic_pointer_cast<CADEngine::HorizontalConstraint>(c);
                    if (hc && hc->matchesPolylineSegment(polyline, closestSeg)) {
                        sketch2D->removeConstraint(c);
                        emit statusMessage("Contrainte horizontale supprimée sur segment");
                        break;
                    }
                } else if (c->getType() == CADEngine::ConstraintType::Vertical) {
                    auto vc = std::dynamic_pointer_cast<CADEngine::VerticalConstraint>(c);
                    if (vc && vc->matchesPolylineSegment(polyline, closestSeg)) {
                        emit statusMessage("Contrainte verticale déjà présente sur ce segment");
                        setSketchTool(SketchTool::None);
                        return;
                    }
                }
            }
            
            auto constraint = std::make_shared<CADEngine::VerticalConstraint>(polyline, closestSeg);
            sketch2D->addConstraint(constraint);
            sketch2D->solveConstraints();
            update();
            emit statusMessage(QString("Contrainte verticale appliquée au segment %1 !").arg(closestSeg));
            setSketchTool(SketchTool::None);
            return;
        }
        
        emit statusMessage("Cliquez sur une ligne ou un segment de polyline !");
        return;
    }
    
    // CONTRAINTES 2 ENTITÉS : Parallel, Perpendicular, Concentric
    if (m_currentTool == SketchTool::ConstraintParallel ||
        m_currentTool == SketchTool::ConstraintPerpendicular) {
        
        if (!clickedEntity || 
            (clickedEntity->getType() != CADEngine::SketchEntityType::Line &&
             clickedEntity->getType() != CADEngine::SketchEntityType::Polyline)) {
            emit statusMessage("Cliquez sur une ligne ou un segment de polyline !");
            return;
        }
        
        // Déterminer le segment cliqué (index = -1 pour une ligne simple)
        int clickedSegIdx = -1;
        if (clickedEntity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(clickedEntity);
            auto points = polyline->getPoints();
            double minDist = 1e10;
            for (size_t i = 0; i < points.size() - 1; i++) {
                double dx = points[i+1].X() - points[i].X();
                double dy = points[i+1].Y() - points[i].Y();
                double len2 = dx*dx + dy*dy;
                if (len2 < 1e-6) continue;
                double t = ((pt.X() - points[i].X()) * dx + (pt.Y() - points[i].Y()) * dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                double px = points[i].X() + t * dx;
                double py = points[i].Y() + t * dy;
                double dist = std::sqrt((pt.X()-px)*(pt.X()-px) + (pt.Y()-py)*(pt.Y()-py));
                if (dist < minDist) { minDist = dist; clickedSegIdx = (int)i; }
            }
            if (clickedSegIdx < 0) {
                emit statusMessage("Segment introuvable !");
                return;
            }
        }
        
        if (!m_firstEntityForConstraint) {
            // Premier clic - stocker entité + index segment
            m_firstEntityForConstraint = clickedEntity;
            m_firstSegmentIndexForConstraint = clickedSegIdx;
            
            QString desc = (clickedEntity->getType() == CADEngine::SketchEntityType::Polyline)
                ? QString("Segment %1 polyline").arg(clickedSegIdx) : "Ligne";
            emit statusMessage(QString("%1 sélectionné(e) - cliquez sur le 2ème segment").arg(desc));
        } else {
            // Deuxième clic - créer la contrainte
            auto entity1 = m_firstEntityForConstraint;
            int segIdx1 = m_firstSegmentIndexForConstraint;
            auto entity2 = clickedEntity;
            int segIdx2 = clickedSegIdx;
            
            // Récupérer les shared_ptr typés
            auto line1 = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity1);
            auto poly1 = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity1);
            auto line2 = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity2);
            auto poly2 = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity2);
            
            if (m_currentTool == SketchTool::ConstraintParallel) {
                std::shared_ptr<CADEngine::ParallelConstraint> constraint;
                if (line1 && line2)       constraint = std::make_shared<CADEngine::ParallelConstraint>(line1, line2);
                else if (line1 && poly2)  constraint = std::make_shared<CADEngine::ParallelConstraint>(line1, poly2, segIdx2);
                else if (poly1 && line2)  constraint = std::make_shared<CADEngine::ParallelConstraint>(poly1, segIdx1, line2);
                else if (poly1 && poly2)  constraint = std::make_shared<CADEngine::ParallelConstraint>(poly1, segIdx1, poly2, segIdx2);
                
                if (constraint) {
                    sketch2D->addConstraint(constraint);
                    emit statusMessage("Contrainte parallèle appliquée !");
                }
            } else {
                std::shared_ptr<CADEngine::PerpendicularConstraint> constraint;
                if (line1 && line2)       constraint = std::make_shared<CADEngine::PerpendicularConstraint>(line1, line2);
                else if (line1 && poly2)  constraint = std::make_shared<CADEngine::PerpendicularConstraint>(line1, poly2, segIdx2);
                else if (poly1 && line2)  constraint = std::make_shared<CADEngine::PerpendicularConstraint>(poly1, segIdx1, line2);
                else if (poly1 && poly2)  constraint = std::make_shared<CADEngine::PerpendicularConstraint>(poly1, segIdx1, poly2, segIdx2);
                
                if (constraint) {
                    sketch2D->addConstraint(constraint);
                    emit statusMessage("Contrainte perpendiculaire appliquée !");
                }
            }
            
            sketch2D->solveConstraints();
            update();
            m_firstEntityForConstraint.reset();
            m_firstSegmentIndexForConstraint = -1;
            setSketchTool(SketchTool::None);
        }
        return;
    }
    
    if (m_currentTool == SketchTool::ConstraintConcentric) {
        if (!clickedEntity || clickedEntity->getType() != CADEngine::SketchEntityType::Circle) {
            emit statusMessage("Veuillez cliquer sur un cercle !");
            return;
        }
        
        if (!m_firstEntityForConstraint) {
            m_firstEntityForConstraint = clickedEntity;
            emit statusMessage("Premier cercle sélectionné - cliquez sur le deuxième cercle");
        } else {
            auto circle1 = std::dynamic_pointer_cast<CADEngine::SketchCircle>(m_firstEntityForConstraint);
            auto circle2 = std::dynamic_pointer_cast<CADEngine::SketchCircle>(clickedEntity);
            
            auto constraint = std::make_shared<CADEngine::ConcentricConstraint>(circle1, circle2);
            sketch2D->addConstraint(constraint);
            sketch2D->solveConstraints();
            update();
            emit statusMessage("Contrainte concentrique appliquée !");
            m_firstEntityForConstraint.reset();
            setSketchTool(SketchTool::None);
        }
        return;
    }
    
    // CONTRAINTE COINCIDENT (2 clics sur extrémités/centres/vertices)
    if (m_currentTool == SketchTool::ConstraintCoincident) {
        double tolerance = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom);
        auto endpoint = findNearestEndpoint(pt, tolerance);
        
        if (!endpoint.entity) {
            emit statusMessage("Cliquez sur une extrémité, un sommet ou un centre !");
            return;
        }
        
        if (!m_firstEndpointForCoincident.entity) {
            // Premier clic - stocker l'endpoint
            m_firstEndpointForCoincident = endpoint;
            
            // Feedback visuel : quel type de point ?
            QString pointDesc;
            if (endpoint.entity->getType() == CADEngine::SketchEntityType::Circle)
                pointDesc = "Centre cercle";
            else if (endpoint.entity->getType() == CADEngine::SketchEntityType::Polyline)
                pointDesc = QString("Vertex %1 polyline").arg(endpoint.vertexIndex);
            else if (endpoint.vertexIndex == -1)
                pointDesc = "Extrémité début";
            else if (endpoint.vertexIndex == -2)
                pointDesc = "Extrémité fin";
            else
                pointDesc = QString("Point %1").arg(endpoint.vertexIndex);
            
            emit statusMessage(QString("%1 sélectionné (%2, %3) - Cliquez sur le 2ème point")
                .arg(pointDesc)
                .arg(endpoint.point.X(), 0, 'f', 1)
                .arg(endpoint.point.Y(), 0, 'f', 1));
        } else {
            // Deuxième clic - créer la contrainte
            
            // Vérifier qu'on ne clique pas sur le même point
            if (endpoint.entity == m_firstEndpointForCoincident.entity 
                && endpoint.vertexIndex == m_firstEndpointForCoincident.vertexIndex) {
                emit statusMessage("Sélectionnez un point différent !");
                return;
            }
            
            auto constraint = std::make_shared<CADEngine::CoincidentConstraint>(
                m_firstEndpointForCoincident.entity, m_firstEndpointForCoincident.vertexIndex,
                endpoint.entity, endpoint.vertexIndex
            );
            
            sketch2D->addConstraint(constraint);
            sketch2D->solveConstraints();
            
            // Mettre à jour les dimensions liées aux entités modifiées
            update();
            
            emit statusMessage(QString("Contrainte coïncidente appliquée ! (%1, %2)")
                .arg(m_firstEndpointForCoincident.point.X(), 0, 'f', 1)
                .arg(m_firstEndpointForCoincident.point.Y(), 0, 'f', 1));
            
            // Reset
            m_firstEndpointForCoincident = EndpointInfo();
            m_firstEndpointForCoincident.entity = nullptr;
            m_firstEndpointForCoincident.vertexIndex = -99;
            setSketchTool(SketchTool::None);
        }
        return;
    }
    
    // ================================================================
    // CONTRAINTE FIX (Verrouillage) - 1 clic sur un vertex/endpoint
    // ================================================================
    if (m_currentTool == SketchTool::ConstraintFix) {
        double tolerance = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom);
        auto endpoint = findNearestEndpoint(pt, tolerance);
        
        if (!endpoint.entity) {
            emit statusMessage("Cliquez sur un point (extrémité, vertex, centre) !");
            return;
        }
        
        // Vérifier si déjà verrouillé → toggle
        auto constraints = sketch2D->getConstraints();
        for (const auto& c : constraints) {
            if (c->getType() == CADEngine::ConstraintType::Fix) {
                auto lc = std::dynamic_pointer_cast<CADEngine::FixConstraint>(c);
                if (lc && lc->matchesVertex(endpoint.entity, endpoint.vertexIndex)) {
                    sketch2D->removeConstraint(c);
                    update();
                    emit statusMessage("Point déverrouillé !");
                    setSketchTool(SketchTool::None);
                    return;
                }
            }
        }
        
        auto constraint = std::make_shared<CADEngine::FixConstraint>(
            endpoint.entity, endpoint.vertexIndex, endpoint.point
        );
        sketch2D->addConstraint(constraint);
        update();
        
        emit statusMessage(QString("Point verrouillé à (%1, %2)")
            .arg(endpoint.point.X(), 0, 'f', 1).arg(endpoint.point.Y(), 0, 'f', 1));
        setSketchTool(SketchTool::None);
        return;
    }
    
    // ================================================================
    // CONTRAINTE SYMÉTRIQUE - 3 clics (point1, point2, axe)
    // ================================================================
    if (m_currentTool == SketchTool::ConstraintSymmetric) {
        double tolerance = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom);
        
        if (m_symmetricState.step == 0) {
            // Étape 1 : sélectionner l'entité à mettre en miroir
            if (!clickedEntity || clickedEntity->isConstruction()) {
                emit statusMessage("Cliquez sur l'entité à mettre en miroir !");
                return;
            }
            m_symmetricState.entity1 = clickedEntity;
            m_symmetricState.step = 1;
            emit statusMessage("Entité sélectionnée — Cliquez sur l'axe de symétrie");
        }
        else if (m_symmetricState.step == 1) {
            // Étape 2 : sélectionner l'axe → créer la copie miroir
            std::shared_ptr<CADEngine::SketchLine> axisLine;
            double bestDist = tolerance * 2.0;
            
            for (const auto& entity : sketch2D->getEntities()) {
                if (entity->getType() != CADEngine::SketchEntityType::Line) continue;
                auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
                if (!line) continue;
                gp_Pnt2d s = line->getStart(), e = line->getEnd();
                double dx = e.X()-s.X(), dy = e.Y()-s.Y();
                double len2 = dx*dx+dy*dy;
                if (len2 < 1e-6) continue;
                double t = ((pt.X()-s.X())*dx + (pt.Y()-s.Y())*dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                double px = s.X()+t*dx, py = s.Y()+t*dy;
                double dist = std::sqrt((pt.X()-px)*(pt.X()-px)+(pt.Y()-py)*(pt.Y()-py));
                if (dist < bestDist) { bestDist = dist; axisLine = line; }
            }
            
            if (!axisLine) {
                emit statusMessage("Cliquez sur un axe ou une ligne de construction !");
                return;
            }
            
            auto src = m_symmetricState.entity1;
            
            // Projeter le centre de l'entité sur l'axe
            auto projectOnAxis = [&](const gp_Pnt2d& center) -> gp_Pnt2d {
                gp_Pnt2d a = axisLine->getStart(), b = axisLine->getEnd();
                double dx = b.X()-a.X(), dy = b.Y()-a.Y();
                double len2 = dx*dx+dy*dy;
                if (len2 < 1e-12) return center;
                double t = ((center.X()-a.X())*dx + (center.Y()-a.Y())*dy) / len2;
                return gp_Pnt2d(a.X()+t*dx, a.Y()+t*dy);
            };
            
            if (src->getType() == CADEngine::SketchEntityType::Rectangle) {
                auto r = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(src);
                if (r) {
                    gp_Pnt2d c = r->getCorner();
                    double w = r->getWidth(), h = r->getHeight();
                    gp_Pnt2d center(c.X()+w/2, c.Y()+h/2);
                    gp_Pnt2d proj = projectOnAxis(center);
                    // Déplacer pour centrer sur l'axe
                    r->setCorner(gp_Pnt2d(proj.X()-w/2, proj.Y()-h/2));
                }
            }
            else if (src->getType() == CADEngine::SketchEntityType::Circle) {
                auto ci = std::dynamic_pointer_cast<CADEngine::SketchCircle>(src);
                if (ci) ci->setCenter(projectOnAxis(ci->getCenter()));
            }
            else if (src->getType() == CADEngine::SketchEntityType::Line) {
                auto l = std::dynamic_pointer_cast<CADEngine::SketchLine>(src);
                if (l) {
                    gp_Pnt2d mid((l->getStart().X()+l->getEnd().X())/2,
                                 (l->getStart().Y()+l->getEnd().Y())/2);
                    gp_Pnt2d proj = projectOnAxis(mid);
                    double dx = proj.X()-mid.X(), dy = proj.Y()-mid.Y();
                    l->setStart(gp_Pnt2d(l->getStart().X()+dx, l->getStart().Y()+dy));
                    l->setEnd(gp_Pnt2d(l->getEnd().X()+dx, l->getEnd().Y()+dy));
                }
            }
            else if (src->getType() == CADEngine::SketchEntityType::Ellipse) {
                auto e = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(src);
                if (e) e->setCenter(projectOnAxis(e->getCenter()));
            }
            else if (src->getType() == CADEngine::SketchEntityType::Polygon) {
                auto p = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(src);
                if (p) p->setCenter(projectOnAxis(p->getCenter()));
            }
            else if (src->getType() == CADEngine::SketchEntityType::Oblong) {
                auto o = std::dynamic_pointer_cast<CADEngine::SketchOblong>(src);
                if (o) o->setCenter(projectOnAxis(o->getCenter()));
            }
            else if (src->getType() == CADEngine::SketchEntityType::Polyline) {
                auto pl = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(src);
                if (pl) {
                    auto pts = pl->getPoints();
                    // Centre = barycentre
                    double cx = 0, cy = 0;
                    for (const auto& p : pts) { cx += p.X(); cy += p.Y(); }
                    cx /= pts.size(); cy /= pts.size();
                    gp_Pnt2d proj = projectOnAxis(gp_Pnt2d(cx, cy));
                    double dx = proj.X()-cx, dy = proj.Y()-cy;
                    for (auto& p : pts) p = gp_Pnt2d(p.X()+dx, p.Y()+dy);
                    pl->setPoints(pts);
                }
            }
            
            sketch2D->regenerateAutoDimensions();
            
            // Créer la contrainte persistante
            auto constraint = std::make_shared<CADEngine::CenterOnAxisConstraint>(src, axisLine);
            sketch2D->addConstraint(constraint);
            
            update();
            emit sketchEntityAdded();
            emit statusMessage("Entité centrée sur l'axe (contrainte active) !");
            m_symmetricState = SymmetricState();
            setSketchTool(SketchTool::None);
        }
        return;
    }
    
    // ================================================================
    // CONGÉ (Fillet) - clic sur un angle (polyline, rectangle, jonction lignes)
    // ================================================================
    if (m_currentTool == SketchTool::Fillet) {
        double tolerance = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom);
        auto sketch2D = m_activeSketch->getSketch2D();

        // 1. Vertex intérieur de polyline (comportement existant)
        if (clickedEntity && clickedEntity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(clickedEntity);
            auto points = polyline->getPoints();
            if (points.size() >= 3) {
                int closestVertex = -1;
                double minDist = tolerance;
                for (int i = 1; i < (int)points.size() - 1; i++) {
                    double dist = pt.Distance(points[i]);
                    if (dist < minDist) { minDist = dist; closestVertex = i; }
                }
                if (closestVertex >= 0) {
                    emit filletRequested(polyline, closestVertex);
                    return;
                }
            }
        }

        // 2. Coin de rectangle
        for (const auto& entity : sketch2D->getEntities()) {
            if (entity->isConstruction()) continue;
            if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
                auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
                auto corners = rect->getKeyPoints();
                for (int i = 0; i < 4; i++) {
                    if (pt.Distance(corners[i]) < tolerance) {
                        emit filletRectCornerRequested(rect, i);
                        return;
                    }
                }
            }
        }

        // 3. Jonction de deux lignes partageant un endpoint
        std::vector<std::pair<std::shared_ptr<CADEngine::SketchLine>, bool>> nearLines;
        for (const auto& entity : sketch2D->getEntities()) {
            if (entity->isConstruction()) continue;
            if (entity->getType() == CADEngine::SketchEntityType::Line) {
                auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
                if (pt.Distance(line->getStart()) < tolerance)
                    nearLines.push_back({line, true});
                else if (pt.Distance(line->getEnd()) < tolerance)
                    nearLines.push_back({line, false});
            }
        }
        if (nearLines.size() >= 2) {
            emit filletLineCornerRequested(nearLines[0].first, nearLines[0].second,
                                           nearLines[1].first, nearLines[1].second);
            return;
        }

        emit statusMessage("Cliquez sur un angle : coin de rectangle, vertex de polyline ou jonction de lignes");
        return;
    }
}

std::shared_ptr<CADEngine::Dimension> Viewport3D::findDimensionAtPoint(const gp_Pnt2d& pt) {
    if (!m_activeSketch) return nullptr;
    
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return nullptr;
    
    // Tolérance centralisée
    double tolerance = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom);
    
    for (const auto& dim : sketch2D->getDimensions()) {
        if (dim->getType() == CADEngine::DimensionType::Linear) {
            auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
            if (!linearDim) continue;
            
            // Vérifier si clic proche de la ligne de cote
            gp_Pnt2d dimStart = linearDim->getDimensionLineStart();
            gp_Pnt2d dimEnd = linearDim->getDimensionLineEnd();
            
            // Distance point-segment
            double dx = dimEnd.X() - dimStart.X();
            double dy = dimEnd.Y() - dimStart.Y();
            double len2 = dx*dx + dy*dy;
            
            if (len2 < 1e-6) continue;
            
            // Projection du point sur le segment
            double t = ((pt.X() - dimStart.X()) * dx + (pt.Y() - dimStart.Y()) * dy) / len2;
            t = std::max(0.0, std::min(1.0, t));
            
            double projX = dimStart.X() + t * dx;
            double projY = dimStart.Y() + t * dy;
            
            double dist = std::sqrt((pt.X() - projX) * (pt.X() - projX) + 
                                   (pt.Y() - projY) * (pt.Y() - projY));
            
            if (dist < tolerance) {
                return dim;
            }
            
            // Vérifier aussi si clic sur le texte
            gp_Pnt2d textPos = linearDim->getTextPosition();
            double textDist = std::sqrt((pt.X() - textPos.X()) * (pt.X() - textPos.X()) +
                                       (pt.Y() - textPos.Y()) * (pt.Y() - textPos.Y()));
            
            if (textDist < tolerance) {
                return dim;
            }
        }
        else if (dim->getType() == CADEngine::DimensionType::Diametral) {
            auto diametralDim = std::dynamic_pointer_cast<CADEngine::DiametralDimension>(dim);
            if (!diametralDim) continue;
            
            gp_Pnt2d center = diametralDim->getCenter();
            double diameter = diametralDim->getDiameter();
            double radius = diameter / 2.0;
            double angle = diametralDim->getAngle();
            double angleRad = angle * M_PI / 180.0;
            
            // Points de la ligne diamétrale
            gp_Pnt2d start(center.X() - radius * cos(angleRad), 
                          center.Y() - radius * sin(angleRad));
            gp_Pnt2d end(center.X() + radius * cos(angleRad), 
                        center.Y() + radius * sin(angleRad));
            
            // Distance point-segment
            double dx = end.X() - start.X();
            double dy = end.Y() - start.Y();
            double len2 = dx*dx + dy*dy;
            
            if (len2 > 1e-6) {
                double t = ((pt.X() - start.X()) * dx + (pt.Y() - start.Y()) * dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                
                double projX = start.X() + t * dx;
                double projY = start.Y() + t * dy;
                
                double dist = std::sqrt((pt.X() - projX) * (pt.X() - projX) + 
                                       (pt.Y() - projY) * (pt.Y() - projY));
                
                if (dist < tolerance) {
                    return dim;
                }
            }
            
            // Vérifier aussi si clic sur le texte
            gp_Pnt2d textPos = diametralDim->getTextPosition();
            double textDist = std::sqrt((pt.X() - textPos.X()) * (pt.X() - textPos.X()) +
                                       (pt.Y() - textPos.Y()) * (pt.Y() - textPos.Y()));
            
            if (textDist < tolerance) {
                return dim;
            }
        }
        else if (dim->getType() == CADEngine::DimensionType::Angular) {
            auto angularDim = std::dynamic_pointer_cast<CADEngine::AngularDimension>(dim);
            if (!angularDim) continue;
            
            // Vérifier clic sur le texte (le plus simple)
            gp_Pnt2d textPos = angularDim->getTextPosition();
            double textDist = std::sqrt((pt.X() - textPos.X()) * (pt.X() - textPos.X()) +
                                       (pt.Y() - textPos.Y()) * (pt.Y() - textPos.Y()));
            
            if (textDist < tolerance * 2) {  // Tolérance plus large pour le texte
                return dim;
            }
            
            // Vérifier aussi clic sur l'arc de cotation
            gp_Pnt2d center = angularDim->getCenter();
            double radius = angularDim->getRadius();
            
            double distToCenter = std::sqrt((pt.X() - center.X()) * (pt.X() - center.X()) +
                                           (pt.Y() - center.Y()) * (pt.Y() - center.Y()));
            
            // Si le clic est proche du rayon de l'arc (tolérance ±10mm)
            if (std::abs(distToCenter - radius) < tolerance) {
                return dim;
            }
        }
    }
    
    // Chercher aussi dans les auto-dimensions (éditables)
    auto autoDims = sketch2D->getAutoDimensions();
    for (const auto& dim : autoDims) {
        gp_Pnt2d textPos = dim->getTextPosition();
        double textDist = std::sqrt((pt.X() - textPos.X()) * (pt.X() - textPos.X()) +
                                   (pt.Y() - textPos.Y()) * (pt.Y() - textPos.Y()));
        if (textDist < tolerance * 2) {
            return dim;
        }
        
        // Vérifier aussi la ligne/arc de cotation
        if (dim->getType() == CADEngine::DimensionType::Linear) {
            auto ld = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
            if (!ld) continue;
            gp_Pnt2d dimStart = ld->getDimensionLineStart();
            gp_Pnt2d dimEnd = ld->getDimensionLineEnd();
            double dx = dimEnd.X() - dimStart.X();
            double dy = dimEnd.Y() - dimStart.Y();
            double len2 = dx*dx + dy*dy;
            if (len2 < 1e-6) continue;
            double t = ((pt.X() - dimStart.X()) * dx + (pt.Y() - dimStart.Y()) * dy) / len2;
            t = std::max(0.0, std::min(1.0, t));
            double projX = dimStart.X() + t * dx;
            double projY = dimStart.Y() + t * dy;
            double dist = std::sqrt((pt.X() - projX) * (pt.X() - projX) + 
                                   (pt.Y() - projY) * (pt.Y() - projY));
            if (dist < tolerance) return dim;
        }
        else if (dim->getType() == CADEngine::DimensionType::Diametral) {
            auto dd = std::dynamic_pointer_cast<CADEngine::DiametralDimension>(dim);
            if (!dd) continue;
            double diameter = dd->getDiameter();
            double radius = diameter / 2.0;
            double angle = dd->getAngle() * M_PI / 180.0;
            gp_Pnt2d center = dd->getCenter();
            gp_Pnt2d start(center.X() - radius * cos(angle), center.Y() - radius * sin(angle));
            gp_Pnt2d end(center.X() + radius * cos(angle), center.Y() + radius * sin(angle));
            double dx = end.X() - start.X(), dy = end.Y() - start.Y();
            double len2 = dx*dx + dy*dy;
            if (len2 > 1e-6) {
                double t = ((pt.X() - start.X()) * dx + (pt.Y() - start.Y()) * dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                double projX = start.X() + t * dx, projY = start.Y() + t * dy;
                double dist = std::sqrt((pt.X()-projX)*(pt.X()-projX) + (pt.Y()-projY)*(pt.Y()-projY));
                if (dist < tolerance) return dim;
            }
        }
        else if (dim->getType() == CADEngine::DimensionType::Radial) {
            auto rd = std::dynamic_pointer_cast<CADEngine::RadialDimension>(dim);
            if (!rd) continue;
            gp_Pnt2d center = rd->getCenter();
            gp_Pnt2d arrow = rd->getArrowPoint();
            double dx = arrow.X() - center.X(), dy = arrow.Y() - center.Y();
            double len2 = dx*dx + dy*dy;
            if (len2 > 1e-6) {
                double t = ((pt.X() - center.X()) * dx + (pt.Y() - center.Y()) * dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                double projX = center.X() + t * dx, projY = center.Y() + t * dy;
                double dist = std::sqrt((pt.X()-projX)*(pt.X()-projX) + (pt.Y()-projY)*(pt.Y()-projY));
                if (dist < tolerance) return dim;
            }
        }
    }
    
    return nullptr;
}

// ============================================================================
// Ray-casting
// ============================================================================

gp_Pnt2d Viewport3D::screenToSketch(int screenX, int screenY) {
    if (!m_activeSketch) return gp_Pnt2d(0, 0);
    
    // Direct mathematical conversion — no GL state dependency
    // Matches exactly what setupSketchCamera does:
    //   glOrtho(-size*aspect, size*aspect, -size, size, ...)
    //   gluLookAt(0,0,500, 0,0,0, 0,1,0)
    //   glTranslatef(-panX, -panY, 0)
    
    double aspect = (double)width() / (double)height();
    double size = m_sketch2DZoom;
    
    // Normalized screen coords [-1, 1]
    double ndcX = (2.0 * screenX / width()) - 1.0;
    double ndcY = 1.0 - (2.0 * screenY / height());  // Y flipped (Qt top-left origin)
    
    // NDC → world (ortho projection inverse + pan)
    double worldX = ndcX * size * aspect + m_cameraPanX;
    double worldY = ndcY * size + m_cameraPanY;
    
    return gp_Pnt2d(worldX, worldY);
}

gp_Pnt2d Viewport3D::snapToGrid(const gp_Pnt2d& pt, double gridSize) {
    if (!m_gridSnapEnabled) {
        // Snap désactivé → arrondir à 0.1mm seulement (précision flottante)
        double x = std::round(pt.X() * 10.0) / 10.0;
        double y = std::round(pt.Y() * 10.0) / 10.0;
        return gp_Pnt2d(x, y);
    }
    // Utiliser la grille adaptative au zoom au lieu du gridSize fixe
    double adaptiveGrid = CADEngine::ViewportScaling::getSnapGridSize(m_sketch2DZoom);
    double x = std::round(pt.X() / adaptiveGrid) * adaptiveGrid;
    double y = std::round(pt.Y() / adaptiveGrid) * adaptiveGrid;
    return gp_Pnt2d(x, y);
}

gp_Pnt2d Viewport3D::snapToHV(const gp_Pnt2d& origin, const gp_Pnt2d& pt, double angleTolDeg) {
    double dx = pt.X() - origin.X();
    double dy = pt.Y() - origin.Y();
    double dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist < 0.5) {
        m_hvSnapActive = false;
        return pt;
    }
    
    double angle = std::atan2(std::abs(dy), std::abs(dx));  // Angle absolu dans [0, π/2]
    double tolRad = angleTolDeg * M_PI / 180.0;
    
    if (angle < tolRad) {
        // Proche de l'horizontale → forcer Y = origin.Y
        m_hvSnapActive = true;
        m_hvSnapHorizontal = true;
        m_hvSnapOrigin = origin;
        return gp_Pnt2d(pt.X(), origin.Y());
    }
    else if (angle > (M_PI / 2.0 - tolRad)) {
        // Proche de la verticale → forcer X = origin.X
        m_hvSnapActive = true;
        m_hvSnapHorizontal = false;
        m_hvSnapOrigin = origin;
        return gp_Pnt2d(origin.X(), pt.Y());
    }
    
    m_hvSnapActive = false;
    return pt;
}

gp_Pnt2d Viewport3D::snapToEntities(const gp_Pnt2d& pt, double tolerance,
                                     std::shared_ptr<CADEngine::SketchEntity> excludeEntity) {
    if (!m_activeSketch) return pt;
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return pt;
    
    // Tolérance adaptative centralisée
    double adaptiveTolerance = CADEngine::ViewportScaling::getSnapTolerance(m_sketch2DZoom);
    
    double closestDist = adaptiveTolerance;
    gp_Pnt2d closestPoint = pt;
    bool found = false;
    m_hasSnapPoint = false;  // Reset snap indicator
    
    // Parcourir toutes les entités pour trouver les points d'accrochage
    for (const auto& entity : sketch2D->getEntities()) {
        // Exclure l'entité en cours de drag
        if (excludeEntity && entity == excludeEntity) continue;
        
        if (entity->getType() == CADEngine::SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
            if (!line) continue;
            
            gp_Pnt2d start = line->getStart();
            gp_Pnt2d end = line->getEnd();
            
            // Construction lines: snap perpendiculaire sur la ligne entière
            if (entity->isConstruction()) {
                // Snap à l'origine (intersection des axes) — PRIORITÉ ABSOLUE
                // Tolérance = max(adaptiveTolerance * 3, gridStep) pour ne jamais rater l'origine
                double gridStep = CADEngine::ViewportScaling::getSnapGridSize(m_sketch2DZoom);
                double originTolerance = std::max(adaptiveTolerance * 3.0, gridStep * 1.5);
                double distOrigin = pt.Distance(gp_Pnt2d(0, 0));
                if (distOrigin < originTolerance) {
                    // Origine trouvée → retour immédiat, pas de projection d'axe
                    m_hasSnapPoint = true;
                    m_snapPoint = gp_Pnt2d(0, 0);
                    return gp_Pnt2d(0, 0);
                }
                // Projection perpendiculaire sur l'axe (seulement si loin de l'origine)
                double dx = end.X() - start.X();
                double dy = end.Y() - start.Y();
                double len2 = dx*dx + dy*dy;
                if (len2 > 1e-12) {
                    double t = ((pt.X()-start.X())*dx + (pt.Y()-start.Y())*dy) / len2;
                    // Pas de clamping à [0,1] — ligne infinie pour les axes
                    gp_Pnt2d proj(start.X() + t*dx, start.Y() + t*dy);
                    double distProj = pt.Distance(proj);
                    if (distProj < closestDist) {
                        closestDist = distProj;
                        closestPoint = proj;
                        found = true;
                    }
                }
                continue;
            }
            
            // Snap aux extrémités de la ligne
            double distStart = pt.Distance(start);
            double distEnd = pt.Distance(end);
            
            if (distStart < closestDist) {
                closestDist = distStart;
                closestPoint = start;
                found = true;
            }
            if (distEnd < closestDist) {
                closestDist = distEnd;
                closestPoint = end;
                found = true;
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
            if (!circle) continue;
            
            // Snap au centre du cercle
            gp_Pnt2d center = circle->getCenter();
            double distCenter = pt.Distance(center);
            
            if (distCenter < closestDist) {
                closestDist = distCenter;
                closestPoint = center;
                found = true;
            }
            
            // Snap aux 4 points cardinaux (haut, bas, gauche, droite)
            double radius = circle->getRadius();
            gp_Pnt2d top(center.X(), center.Y() + radius);
            gp_Pnt2d bottom(center.X(), center.Y() - radius);
            gp_Pnt2d left(center.X() - radius, center.Y());
            gp_Pnt2d right(center.X() + radius, center.Y());
            
            for (const auto& cardinalPt : {top, bottom, left, right}) {
                double dist = pt.Distance(cardinalPt);
                if (dist < closestDist) {
                    closestDist = dist;
                    closestPoint = cardinalPt;
                    found = true;
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
            if (!rect) continue;
            
            // Snap aux 4 coins du rectangle
            gp_Pnt2d corner = rect->getCorner();
            double w = rect->getWidth();
            double h = rect->getHeight();
            
            std::vector<gp_Pnt2d> corners = {
                corner,
                gp_Pnt2d(corner.X() + w, corner.Y()),
                gp_Pnt2d(corner.X() + w, corner.Y() + h),
                gp_Pnt2d(corner.X(), corner.Y() + h)
            };
            
            for (const auto& c : corners) {
                double dist = pt.Distance(c);
                if (dist < closestDist) {
                    closestDist = dist;
                    closestPoint = c;
                    found = true;
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
            if (!polyline) continue;
            
            // Snap à tous les points de la polyligne
            for (const auto& p : polyline->getPoints()) {
                double dist = pt.Distance(p);
                if (dist < closestDist) {
                    closestDist = dist;
                    closestPoint = p;
                    found = true;
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Arc) {
            auto arc = std::dynamic_pointer_cast<CADEngine::SketchArc>(entity);
            if (!arc) continue;
            
            // Snap aux extrémités de l'arc
            gp_Pnt2d start = arc->getStart();
            gp_Pnt2d end = arc->getEnd();
            
            double distStart = pt.Distance(start);
            double distEnd = pt.Distance(end);
            
            if (distStart < closestDist) {
                closestDist = distStart;
                closestPoint = start;
                found = true;
            }
            if (distEnd < closestDist) {
                closestDist = distEnd;
                closestPoint = end;
                found = true;
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Ellipse) {
            auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
            if (!ellipse) continue;
            
            // Snap au centre et aux extrémités des axes
            auto kp = ellipse->getKeyPoints();
            for (const auto& p : kp) {
                double dist = pt.Distance(p);
                if (dist < closestDist) {
                    closestDist = dist;
                    closestPoint = p;
                    found = true;
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
            auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
            if (!polygon) continue;
            
            // Snap au centre
            double distCenter = pt.Distance(polygon->getCenter());
            if (distCenter < closestDist) {
                closestDist = distCenter;
                closestPoint = polygon->getCenter();
                found = true;
            }
            // Snap aux vertices
            for (const auto& v : polygon->getVertices()) {
                double dist = pt.Distance(v);
                if (dist < closestDist) {
                    closestDist = dist;
                    closestPoint = v;
                    found = true;
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
            auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
            if (!oblong) continue;
            
            // Snap au centre et aux keypoints
            for (const auto& p : oblong->getKeyPoints()) {
                double dist = pt.Distance(p);
                if (dist < closestDist) {
                    closestDist = dist;
                    closestPoint = p;
                    found = true;
                }
            }
        }
    }
    
    // Snap to reference body points (edges/vertices on sketch plane)
    for (const auto& sp : m_refBodySnapPoints) {
        double dist = pt.Distance(sp);
        if (dist < closestDist) {
            closestDist = dist;
            closestPoint = sp;
            found = true;
        }
    }
    
    // Si on a trouvé un point d'accrochage proche, le retourner
    if (found) {
        m_hasSnapPoint = true;
        m_snapPoint = closestPoint;
        return closestPoint;
    }
    
    // Grille de snap ultra-précise avec système centralisé
    double gridSize = CADEngine::ViewportScaling::getSnapGridSize(m_sketch2DZoom);
    
    return snapToGrid(pt, gridSize);
}

// ============================================================================
// Modes
// ============================================================================

void Viewport3D::setMode(ViewMode mode) {
    if (m_mode != mode) {
        m_mode = mode;
        m_currentTool = SketchTool::None;
        m_tempPoints.clear();
        
        // Reconfigurer la projection
        makeCurrent();
        resizeGL(width(), height());
        
        // Ensure viewport has focus for wheel/key events
        setFocus();
        
        emit modeChanged(mode);
        update();
    }
}

void Viewport3D::enterSketchMode(std::shared_ptr<CADEngine::SketchFeature> sketch, double centerU, double centerV, double autoZoom) {
    m_activeSketch = sketch;
    setMode(ViewMode::SKETCH_2D);
    
    // Save current body as reference for ghost wireframe rendering
    m_sketchRefBody = m_currentBody;
    
    // Ensure reference body is tessellated for edge rendering
    if (!m_sketchRefBody.IsNull()) {
        try {
            BRepMesh_IncrementalMesh mesher(m_sketchRefBody, 0.1, Standard_False, 0.3, Standard_True);
            mesher.Perform();
        } catch (...) {}
    }
    
    // Pan camera to face center in local sketch coordinates
    m_cameraPanX = (float)centerU;
    m_cameraPanY = (float)centerV;
    
    // Auto-zoom to fit face if specified, otherwise use default
    if (autoZoom > 0.0) {
        m_sketch2DZoom = (float)autoZoom;
    } else {
        m_sketch2DZoom = 300.0f; // default for XY/YZ/XZ planes
    }
    m_cameraDistance = 500.0f;
    
    // Log sketch plane axes for diagnostics
    if (sketch->getSketch2D()) {
        std::cout << "[enterSketchMode] Pan: u=" << centerU << " v=" << centerV 
                  << " zoom=" << m_sketch2DZoom << std::endl;
    } else {
        std::cout << "[enterSketchMode] Pan: u=" << centerU << " v=" << centerV 
                  << " zoom=" << m_sketch2DZoom << std::endl;
    }
    
    if (m_viewCube) m_viewCube->hide();
    
    emit statusMessage("Mode Sketch activé - Sélectionnez un outil (L/C/R)");
    update();
}

void Viewport3D::exitSketchMode() {
    m_activeSketch = nullptr;
    m_sketchRefBody = TopoDS_Shape();  // Clear ghost body
    m_refBodySnapPoints.clear();
    
    // Restore 3D camera defaults — sketch mode may have modified pan/distance
    m_cameraPanX = 0.0f;
    m_cameraPanY = 0.0f;
    m_cameraDistance = 500.0f;
    
    setMode(ViewMode::NORMAL_3D);
    m_currentTool = SketchTool::None;
    m_tempPoints.clear();
    
    if (m_viewCube) {
        m_viewCube->setCameraAngles(m_cameraAngleX, m_cameraAngleY);
        m_viewCube->show();
    }
    
    emit statusMessage("Mode 3D");
}

void Viewport3D::setSketchTool(SketchTool tool) {
    m_currentTool = tool;
    m_tempPoints.clear();
    
    // Reset états de sélection multi-clics
    m_firstEntityForConstraint.reset();
    m_firstSegmentIndexForConstraint = -1;
    m_firstSegForAngle = AngleSegmentInfo();
    m_firstLineForDistance = AngleSegmentInfo();
    m_arcCenterTracking = false;
    m_arcCenterAccumSweep = 0.0;
    m_firstEndpointForCoincident = EndpointInfo();
    m_firstEndpointForCoincident.entity = nullptr;
    m_firstEndpointForCoincident.vertexIndex = -99;
    m_symmetricState = SymmetricState();
    m_hasDimensionFirstPoint = false;
    m_firstDimensionEndpoint = EndpointInfo();
    m_firstDimensionEndpoint.entity = nullptr;
    m_firstDimensionEndpoint.vertexIndex = -99;
    
    QString toolName;
    switch (tool) {
        case SketchTool::Line: toolName = "Line"; break;
        case SketchTool::Circle: toolName = "Circle"; break;
        case SketchTool::Rectangle: toolName = "Rectangle"; break;
        case SketchTool::Arc: toolName = "Arc Bézier"; break;
        case SketchTool::Polyline: toolName = "Polyline"; break;
        case SketchTool::Ellipse: toolName = "Ellipse (centre → rayon)"; break;
        case SketchTool::Polygon: toolName = QString("Polygone (%1 côtés)").arg(m_polygonSides); break;
        case SketchTool::ArcCenter: toolName = "Arc Centre (centre → début → fin)"; break;
        case SketchTool::Oblong: toolName = "Oblong (centre → longueur → largeur)"; break;
        case SketchTool::Dimension: toolName = "Cotation (Shift=angle, Ctrl=distance entre lignes)"; break;
        case SketchTool::ConstraintCoincident: toolName = "Coïncident (cliquez sur 2 points)"; break;
        case SketchTool::ConstraintHorizontal: toolName = "Horizontal"; break;
        case SketchTool::ConstraintVertical: toolName = "Vertical"; break;
        case SketchTool::ConstraintParallel: toolName = "Parallèle"; break;
        case SketchTool::ConstraintPerpendicular: toolName = "Perpendiculaire"; break;
        case SketchTool::ConstraintConcentric: toolName = "Concentrique"; break;
        case SketchTool::ConstraintSymmetric: toolName = "Centrer sur axe"; break;
        case SketchTool::ConstraintFix: toolName = "Verrouiller point"; break;
        case SketchTool::Fillet: toolName = "Congé"; break;
        default: toolName = "Sélection"; break;
    }
    
    emit statusMessage("Outil: " + toolName);
    emit toolChanged(tool);  // ← Notifier le changement d'outil
}

// ============================================================================
// Outils de dessin
// ============================================================================

void Viewport3D::handleLineToolFinish() {
    if (m_tempPoints.size() < 2) return;
    
    auto line = std::make_shared<CADEngine::SketchLine>(m_tempPoints[0], m_tempPoints[1]);
    
    // Émettre signal pour que MainWindow gère l'ajout via commande (undo/redo)
    emit entityCreated(line, m_activeSketch->getSketch2D());
    
    double length = m_tempPoints[0].Distance(m_tempPoints[1]);
    
    emit statusMessage(QString("Ligne créée: L=%1 mm")
        .arg(length, 0, 'f', 1));
    emit sketchEntityAdded();
}

void Viewport3D::handleCircleToolFinish() {
    if (m_tempPoints.size() < 2) return;
    
    gp_Pnt2d center = m_tempPoints[0];
    double radius = center.Distance(m_tempPoints[1]);
    
    // Snap rayon à la grille adaptative (même précision que le positionnement)
    double snapGrid = CADEngine::ViewportScaling::getSnapGridSize(m_sketch2DZoom);
    radius = std::round(radius / snapGrid) * snapGrid;
    
    auto circle = std::make_shared<CADEngine::SketchCircle>(center, radius);
    emit entityCreated(circle, m_activeSketch->getSketch2D());
    
    emit statusMessage(QString("Cercle créé: Ø%1 mm").arg(radius * 2.0, 0, 'f', 2));
    emit sketchEntityAdded();
}

void Viewport3D::handleRectangleToolFinish() {
    if (m_tempPoints.size() < 2) return;
    
    gp_Pnt2d corner1 = m_tempPoints[0];
    gp_Pnt2d corner2 = m_tempPoints[1];
    
    double width = std::abs(corner2.X() - corner1.X());
    double height = std::abs(corner2.Y() - corner1.Y());
    
    gp_Pnt2d corner(std::min(corner1.X(), corner2.X()), 
                    std::min(corner1.Y(), corner2.Y()));
    
    auto rect = std::make_shared<CADEngine::SketchRectangle>(corner, width, height);
    emit entityCreated(rect, m_activeSketch->getSketch2D());
    
    emit statusMessage(QString("Rectangle créé: %1×%2 mm")
        .arg(width, 0, 'f', 1).arg(height, 0, 'f', 1));
    emit sketchEntityAdded();
}

void Viewport3D::handleArcToolFinish() {
    qDebug() << "[handleArcToolFinish] Points:" << m_tempPoints.size();
    if (m_tempPoints.size() < 3) return;
    
    gp_Pnt2d start = m_tempPoints[0];
    gp_Pnt2d end = m_tempPoints[1];    // Point 2 = fin
    gp_Pnt2d mid = m_tempPoints[2];    // Point 3 = point de contrôle Bézier
    
    qDebug() << "  Création Arc Bézier:" << start.X() << start.Y() << "ctrl" << mid.X() << mid.Y() << "end" << end.X() << end.Y();
    
    auto arc = std::make_shared<CADEngine::SketchArc>(start, end, mid, true);  // Bézier
    emit entityCreated(arc, m_activeSketch->getSketch2D());
    
    emit statusMessage("Arc Bézier créé (3 points)");
    emit sketchEntityAdded();
}

void Viewport3D::handlePolylineToolFinish() {
    qDebug() << "[handlePolylineToolFinish] Points:" << m_tempPoints.size();
    if (m_tempPoints.size() < 2) return;
    
    auto polyline = std::make_shared<CADEngine::SketchPolyline>(m_tempPoints);
    emit entityCreated(polyline, m_activeSketch->getSketch2D());
    
    qDebug() << "  Création Polyligne:" << m_tempPoints.size() << "points";
    
    emit statusMessage(QString("Polyligne créée: %1 points").arg(m_tempPoints.size()));
    emit sketchEntityAdded();
}

void Viewport3D::handleEllipseToolFinish() {
    if (m_tempPoints.size() < 2) return;
    
    gp_Pnt2d center = m_tempPoints[0];
    double dx = m_tempPoints[1].X() - center.X();
    double dy = m_tempPoints[1].Y() - center.Y();
    double majorRadius = std::sqrt(dx*dx + dy*dy);
    double rotation = std::atan2(dy, dx);
    
    // Si on a un 3ème point, l'utiliser pour le petit axe
    double minorRadius = majorRadius * 0.6;  // ratio par défaut
    if (m_tempPoints.size() >= 3) {
        // Projeter le 3ème point sur l'axe perpendiculaire
        double perpX = -std::sin(rotation);
        double perpY = std::cos(rotation);
        double px = m_tempPoints[2].X() - center.X();
        double py = m_tempPoints[2].Y() - center.Y();
        minorRadius = std::abs(px * perpX + py * perpY);
        if (minorRadius < 1.0) minorRadius = 1.0;
    }
    
    // S'assurer que major >= minor
    if (minorRadius > majorRadius) std::swap(majorRadius, minorRadius);
    
    auto ellipse = std::make_shared<CADEngine::SketchEllipse>(center, majorRadius, minorRadius, rotation);
    emit entityCreated(ellipse, m_activeSketch->getSketch2D());
    
    emit statusMessage(QString("Ellipse créée: a=%1 b=%2 mm")
        .arg(majorRadius, 0, 'f', 1).arg(minorRadius, 0, 'f', 1));
    emit sketchEntityAdded();
}

void Viewport3D::handlePolygonToolFinish() {
    if (m_tempPoints.size() < 2) return;
    
    gp_Pnt2d center = m_tempPoints[0];
    double dx = m_tempPoints[1].X() - center.X();
    double dy = m_tempPoints[1].Y() - center.Y();
    double radius = std::sqrt(dx*dx + dy*dy);
    double rotation = std::atan2(dy, dx);
    
    auto polygon = std::make_shared<CADEngine::SketchPolygon>(center, radius, m_polygonSides, rotation);
    emit entityCreated(polygon, m_activeSketch->getSketch2D());
    
    emit statusMessage(QString("Polygone créé: %1 côtés, R=%2 mm")
        .arg(m_polygonSides).arg(radius, 0, 'f', 1));
    emit sketchEntityAdded();
}

void Viewport3D::handleArcCenterToolFinish() {
    if (m_tempPoints.size() < 3) return;
    
    gp_Pnt2d center = m_tempPoints[0];
    gp_Pnt2d startPt = m_tempPoints[1];
    
    double radius = center.Distance(startPt);
    double startAngle = std::atan2(startPt.Y() - center.Y(), startPt.X() - center.X());
    
    // Utiliser le sweep accumulé par le suivi continu de la souris
    double sweep = m_arcCenterAccumSweep;
    if (std::abs(sweep) < 0.05) return;  // Arc trop petit
    
    // Limiter à ±360°
    if (sweep > 2.0 * M_PI) sweep = 2.0 * M_PI - 0.01;
    if (sweep < -2.0 * M_PI) sweep = -2.0 * M_PI + 0.01;
    
    double endAngle = startAngle + sweep;
    gp_Pnt2d endPt(center.X() + radius * std::cos(endAngle),
                    center.Y() + radius * std::sin(endAngle));
    
    // Point milieu SUR l'arc
    double midAngle = startAngle + sweep / 2.0;
    gp_Pnt2d midPt(center.X() + radius * std::cos(midAngle),
                    center.Y() + radius * std::sin(midAngle));
    
    auto arc = std::make_shared<CADEngine::SketchArc>(startPt, endPt, midPt);
    emit entityCreated(arc, m_activeSketch->getSketch2D());
    
    double angleDeg = std::abs(sweep) * 180.0 / M_PI;
    emit statusMessage(QString("Arc créé: R=%1 mm, angle=%2°")
        .arg(radius, 0, 'f', 1).arg(angleDeg, 0, 'f', 1));
    emit sketchEntityAdded();
}

void Viewport3D::handleOblongToolFinish() {
    if (m_tempPoints.size() < 2) return;
    
    gp_Pnt2d center = m_tempPoints[0];
    double dx = m_tempPoints[1].X() - center.X();
    double dy = m_tempPoints[1].Y() - center.Y();
    double halfLength = std::sqrt(dx*dx + dy*dy);
    double rotation = std::atan2(dy, dx);
    double length = halfLength * 2.0;
    
    // Largeur depuis 3ème point ou auto
    double width;
    if (m_tempPoints.size() >= 3) {
        // Projeter le 3ème point sur l'axe perpendiculaire
        double perpX = -std::sin(rotation);
        double perpY = std::cos(rotation);
        double px = m_tempPoints[2].X() - center.X();
        double py = m_tempPoints[2].Y() - center.Y();
        width = std::abs(px * perpX + py * perpY) * 2.0;
        if (width < 2.0) width = 2.0;
    } else {
        // Auto: largeur = longueur / 3
        width = length / 3.0;
    }
    
    // S'assurer que width <= length
    if (width > length) width = length;
    
    auto oblong = std::make_shared<CADEngine::SketchOblong>(center, length, width, rotation);
    emit entityCreated(oblong, m_activeSketch->getSketch2D());
    
    emit statusMessage(QString("Oblong créé: L=%1 × l=%2 mm")
        .arg(length, 0, 'f', 1).arg(width, 0, 'f', 1));
    emit sketchEntityAdded();
}

void Viewport3D::handleDimensionToolFinish() {
    if (m_tempPoints.size() < 2) return;
    
    gp_Pnt2d point1 = m_tempPoints[0];
    gp_Pnt2d point2 = m_tempPoints[1];
    
    // Créer cotation linéaire
    auto dimension = std::make_shared<CADEngine::LinearDimension>(point1, point2);
    
    // Émettre signal pour que MainWindow gère l'ajout via commande
    auto sketch2D = m_activeSketch->getSketch2D();
    if (sketch2D) {
        emit dimensionCreated(dimension, sketch2D);
        
        double distance = dimension->getValue();
        emit statusMessage(QString("Cotation créée: %1 mm").arg(distance, 0, 'f', 1));
        emit sketchEntityAdded();
        
        update();
    }
}

// Suite dans partie 3 (Events souris)...
// Suite de Viewport3D.cpp - Partie 3

// ============================================================================
// Events souris
// ============================================================================

// PATCH pour Viewport3D.cpp
// Remplacer la fonction mousePressEvent (ligne 509-545)

void Viewport3D::mousePressEvent(QMouseEvent* event) {
    m_lastMousePos = event->pos();
    setFocus();  // Ensure widget has focus for wheel events
    
    if (m_mode == ViewMode::SKETCH_2D) {
        // Mode sketch - CLIC DROIT pour menu contextuel (suppression)
        if (event->button() == Qt::RightButton && m_currentTool == SketchTool::None) {
            gp_Pnt2d pt = screenToSketch(event->pos().x(), event->pos().y());
            
            // Vérifier clic sur dimension
            auto clickedDim = findDimensionAtPoint(pt);
            if (clickedDim) {
                emit dimensionRightClicked(clickedDim, event->globalPosition().toPoint());
                return;
            }
            
            // Vérifier clic sur symbole de contrainte
            if (m_activeSketch && m_activeSketch->getSketch2D()) {
                auto sketch2D = m_activeSketch->getSketch2D();
                double tolerance = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom) * 3.0;
                
                for (const auto& c : sketch2D->getConstraints()) {
                    gp_Pnt2d symPos(0, 0);
                    bool found = false;
                    
                    if (c->getType() == CADEngine::ConstraintType::CenterOnAxis) {
                        auto coa = std::dynamic_pointer_cast<CADEngine::CenterOnAxisConstraint>(c);
                        if (coa) {
                            gp_Pnt2d center = coa->getEntityCenter();
                            gp_Pnt2d proj = coa->projectOnAxis(center);
                            // Même calcul que le rendu : au bord de l'entité
                            gp_Pnt2d axA = coa->getAxisLine()->getStart();
                            gp_Pnt2d axB = coa->getAxisLine()->getEnd();
                            double adx = axB.X()-axA.X(), ady = axB.Y()-axA.Y();
                            double al = std::sqrt(adx*adx+ady*ady);
                            if (al > 1e-6) { adx /= al; ady /= al; }
                            double halfExtent = 15.0;
                            auto ent = coa->getEntity();
                            if (ent->getType() == CADEngine::SketchEntityType::Rectangle) {
                                auto r = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(ent);
                                if (r) {
                                    gp_Pnt2d co = r->getCorner();
                                    double w = r->getWidth(), h = r->getHeight();
                                    gp_Pnt2d corners[4] = {co, gp_Pnt2d(co.X()+w,co.Y()), gp_Pnt2d(co.X()+w,co.Y()+h), gp_Pnt2d(co.X(),co.Y()+h)};
                                    halfExtent = 0;
                                    for (int i = 0; i < 4; i++) {
                                        double d = std::abs((corners[i].X()-proj.X())*adx + (corners[i].Y()-proj.Y())*ady);
                                        if (d > halfExtent) halfExtent = d;
                                    }
                                }
                            } else if (ent->getType() == CADEngine::SketchEntityType::Circle) {
                                auto ci = std::dynamic_pointer_cast<CADEngine::SketchCircle>(ent);
                                if (ci) halfExtent = ci->getRadius();
                            }
                            symPos = gp_Pnt2d(proj.X() + adx*(halfExtent+8), proj.Y() + ady*(halfExtent+8));
                            found = true;
                        }
                    } else if (c->getType() == CADEngine::ConstraintType::Symmetric) {
                        auto sc = std::dynamic_pointer_cast<CADEngine::SymmetricConstraint>(c);
                        if (sc) {
                            gp_Pnt2d p1 = sc->getPoint1(), p2 = sc->getPoint2();
                            symPos = gp_Pnt2d((p1.X()+p2.X())/2, (p1.Y()+p2.Y())/2);
                            found = true;
                        }
                    }
                    // Autres types : position du symbole basée sur les entités
                    
                    if (found && pt.Distance(symPos) < tolerance) {
                        // Menu contextuel pour supprimer la contrainte
                        QMenu menu;
                        QAction* deleteAction = menu.addAction(QString("Supprimer %1")
                            .arg(QString::fromStdString(c->getDescription())));
                        QAction* result = menu.exec(event->globalPosition().toPoint());
                        if (result == deleteAction) {
                            sketch2D->removeConstraint(c);
                            sketch2D->regenerateAutoDimensions();
                            update();
                            emit statusMessage("Contrainte supprimée");
                        }
                        return;
                    }
                }
            }
            
            // Vérifier clic sur entité
            auto clickedEntity = findEntityAtPoint(pt);
            if (clickedEntity) {
                // Si polyline, trouver le segment cliqué
                m_lastRightClickedSegmentIndex = -1;
                if (clickedEntity->getType() == CADEngine::SketchEntityType::Polyline) {
                    auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(clickedEntity);
                    auto points = polyline->getPoints();
                    double minDist = 1e10;
                    for (size_t i = 0; i < points.size() - 1; i++) {
                        double dx = points[i+1].X() - points[i].X();
                        double dy = points[i+1].Y() - points[i].Y();
                        double len2 = dx*dx + dy*dy;
                        if (len2 < 1e-6) continue;
                        double t = ((pt.X() - points[i].X()) * dx + (pt.Y() - points[i].Y()) * dy) / len2;
                        t = std::max(0.0, std::min(1.0, t));
                        double px = points[i].X() + t * dx;
                        double py = points[i].Y() + t * dy;
                        double dist = std::sqrt((pt.X()-px)*(pt.X()-px) + (pt.Y()-py)*(pt.Y()-py));
                        if (dist < minDist) { minDist = dist; m_lastRightClickedSegmentIndex = (int)i; }
                    }
                }
                emit entityRightClicked(clickedEntity, event->globalPosition().toPoint());
                return;
            }
        }
        
        // Mode sketch - vérifier d'abord si clic sur dimension (sans outil actif)
        if (event->button() == Qt::LeftButton && m_currentTool == SketchTool::None) {
            gp_Pnt2d pt = screenToSketch(event->pos().x(), event->pos().y());
            
            // PRIORITÉ 0 : CTRL+clic → vérifier dimensions EN PREMIER (avant entités)
            // Car les auto-dimensions se superposent aux entités sources
            if (event->modifiers() & Qt::ControlModifier) {
                auto ctrlDim = findDimensionAtPoint(pt);
                if (ctrlDim) {
                    m_isDraggingDimension = true;
                    m_draggedDimension = ctrlDim;
                    m_dragStartPoint = pt;
                    
                    if (ctrlDim->getType() == CADEngine::DimensionType::Linear) {
                        auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(ctrlDim);
                        if (linearDim) {
                            m_initialOffset = linearDim->getOffset();
                        }
                    }
                    
                    emit statusMessage(QString("Déplacer cotation %1")
                        .arg(ctrlDim->isAutomatic() ? "(auto)" : "(manuelle)"));
                    return;
                }
            }
            
            // PRIORITÉ 1 : Vérifier d'abord si clic sur un VERTEX de polyline (pour drag vertex)
            auto clickedEntity = findEntityAtPoint(pt);
            if (clickedEntity && clickedEntity->getType() == CADEngine::SketchEntityType::Polyline) {
                auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(clickedEntity);
                double tolerance = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom);
                int vertexIdx = findPolylineVertexAtPoint(polyline, pt, tolerance);
                
                if (vertexIdx >= 0) {
                    // Clic sur un vertex → drag de ce vertex uniquement
                    m_isDraggingVertex = true;
                    m_isDraggingEntity = true;
                    m_draggedEntity = clickedEntity;
                    m_draggedVertexIndex = vertexIdx;
                    m_dragStartPoint = pt;
                    emit statusMessage(QString("Vertex %1 sélectionné - Déplacer").arg(vertexIdx));
                    return;
                }
                
                // Pas sur un vertex → drag de toute la polyline
                m_isDraggingVertex = false;
                m_isDraggingEntity = true;
                m_draggedEntity = clickedEntity;
                m_dragStartPoint = pt;
                return;
            }
            
            // PRIORITÉ 2 : Clic sur autre entité (pour drag entier)
            if (clickedEntity && !clickedEntity->isConstruction()) {
                m_isDraggingVertex = false;
                m_isDraggingEntity = true;
                m_draggedEntity = clickedEntity;
                m_dragStartPoint = pt;
                return;
            }
            
            // PRIORITÉ 2 : Sinon vérifier clic sur dimension (pour éditer)
            auto clickedDim = findDimensionAtPoint(pt);
            if (clickedDim) {
                // Ctrl+Clic = Drag offset, Clic simple = Éditer valeur
                if (event->modifiers() & Qt::ControlModifier) {
                    // Mode drag dimension (changer offset)
                    m_isDraggingDimension = true;
                    m_draggedDimension = clickedDim;
                    m_dragStartPoint = pt;
                    
                    // Sauvegarder offset initial
                    if (clickedDim->getType() == CADEngine::DimensionType::Linear) {
                        auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(clickedDim);
                        if (linearDim) {
                            m_initialOffset = linearDim->getOffset();
                        }
                    }
                    
                    emit statusMessage("Déplacer pour ajuster l'offset de la cotation");
                    return;
                } else {
                    // Clic simple = Éditer valeur
                    emit dimensionClicked(clickedDim);
                    return;
                }
            }
        }
        
        // ===== MODE CONTRAINTE =====
        if (event->button() == Qt::LeftButton && 
            (m_currentTool == SketchTool::ConstraintHorizontal ||
             m_currentTool == SketchTool::ConstraintVertical ||
             m_currentTool == SketchTool::ConstraintParallel ||
             m_currentTool == SketchTool::ConstraintPerpendicular ||
             m_currentTool == SketchTool::ConstraintCoincident ||
             m_currentTool == SketchTool::ConstraintConcentric ||
             m_currentTool == SketchTool::ConstraintSymmetric ||
             m_currentTool == SketchTool::ConstraintFix ||
             m_currentTool == SketchTool::Fillet)) {
            
            gp_Pnt2d pt = screenToSketch(event->pos().x(), event->pos().y());
            handleConstraintClick(pt);
            return;
        }
        
        // Mode sketch
        if (event->button() == Qt::LeftButton && m_currentTool != SketchTool::None) {
            gp_Pnt2d pt = screenToSketch(event->pos().x(), event->pos().y());
            gp_Pnt2d snapped = snapToEntities(pt);  // Snap entité + grille fallback
            
            // Arc et Polyline = Mode CLICS (pas de drag)
            // Arc / Polyline / Dimension / Ellipse / ArcCenter ont des modes spéciaux
            if (m_currentTool == SketchTool::Arc || m_currentTool == SketchTool::Polyline || 
                m_currentTool == SketchTool::Dimension || m_currentTool == SketchTool::Ellipse ||
                m_currentTool == SketchTool::ArcCenter || m_currentTool == SketchTool::Oblong) {
                // Arc : mode spécial 2 points + courbure
                if (m_currentTool == SketchTool::Arc) {
                    if (m_tempPoints.size() < 2) {
                        // Clic 1 ou 2 : ajouter point
                        m_tempPoints.push_back(snapped);
                        m_hasHoverPoint = false;
                        qDebug() << "[Press] Arc - Point" << m_tempPoints.size();
                    } else {
                        // Clic 3 : valider avec la courbure actuelle (via hover)
                        if (m_hasHoverPoint && m_tempPoints.size() == 2) {
                            gp_Pnt2d p1 = m_tempPoints[0];
                            gp_Pnt2d p3 = m_tempPoints[1];
                            
                            // Calculer point milieu basé sur hover
                            gp_Pnt2d lineCenter((p1.X() + p3.X()) / 2.0, (p1.Y() + p3.Y()) / 2.0);
                            
                            double dx = p3.X() - p1.X();
                            double dy = p3.Y() - p1.Y();
                            double lineLength = std::sqrt(dx*dx + dy*dy);
                            
                            if (lineLength > 1e-6) {
                                double perpX = -dy / lineLength;
                                double perpY = dx / lineLength;
                                
                                double hoverDx = m_hoverPoint.X() - lineCenter.X();
                                double hoverDy = m_hoverPoint.Y() - lineCenter.Y();
                                double distanceFromLine = hoverDx * perpX + hoverDy * perpY;
                                
                                // APPROCHE FUSION: distance = hauteur directe !
                                double arcHeight = distanceFromLine;
                                
                                
                                // Point milieu de l'arc
                                gp_Pnt2d p2(lineCenter.X() + arcHeight * perpX,
                                            lineCenter.Y() + arcHeight * perpY);
                                
                                
                                // Ajouter p2 et créer l'arc
                                m_tempPoints.push_back(p2);
                                handleArcToolFinish();
                                m_tempPoints.clear();
                                m_hasHoverPoint = false;
                                
                                qDebug() << "[Press] Arc - Validé hauteur:" << arcHeight;
                            }
                        }
                    }
                } else if (m_currentTool == SketchTool::Dimension) {
                    double tolerance = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom);
                    auto clickedEntity = findEntityAtPoint(snapped);
                    
                    // === SHIFT + clic → Mode cotation angulaire (inchangé) ===
                    if ((event->modifiers() & Qt::ShiftModifier) && clickedEntity &&
                        (clickedEntity->getType() == CADEngine::SketchEntityType::Line ||
                         clickedEntity->getType() == CADEngine::SketchEntityType::Polyline)) {
                        
                        AngleSegmentInfo segInfo;
                        segInfo.entity = clickedEntity;
                        
                        if (clickedEntity->getType() == CADEngine::SketchEntityType::Line) {
                            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(clickedEntity);
                            segInfo.segmentIndex = -1;
                            segInfo.startPt = line->getStart();
                            segInfo.endPt = line->getEnd();
                        } else {
                            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(clickedEntity);
                            auto points = polyline->getPoints();
                            double minDist = 1e10;
                            int closestSeg = -1;
                            for (size_t i = 0; i < points.size() - 1; i++) {
                                double dx = points[i+1].X() - points[i].X();
                                double dy = points[i+1].Y() - points[i].Y();
                                double len2 = dx*dx + dy*dy;
                                if (len2 < 1e-6) continue;
                                double t = ((snapped.X() - points[i].X()) * dx + (snapped.Y() - points[i].Y()) * dy) / len2;
                                t = std::max(0.0, std::min(1.0, t));
                                double px = points[i].X() + t * dx, py = points[i].Y() + t * dy;
                                double dist = std::sqrt((snapped.X()-px)*(snapped.X()-px) + (snapped.Y()-py)*(snapped.Y()-py));
                                if (dist < minDist) { minDist = dist; closestSeg = (int)i; }
                            }
                            if (closestSeg < 0) { emit statusMessage("Segment introuvable !"); return; }
                            segInfo.segmentIndex = closestSeg;
                            segInfo.startPt = points[closestSeg];
                            segInfo.endPt = points[closestSeg + 1];
                        }
                        
                        if (!m_firstSegForAngle.entity) {
                            m_firstSegForAngle = segInfo;
                            emit statusMessage("1er segment sélectionné - Shift+Clic sur le 2ème segment");
                        } else {
                            gp_Pnt2d vertex, pt1, pt2;
                            bool found = false;
                            gp_Pnt2d a1 = m_firstSegForAngle.startPt;
                            gp_Pnt2d a2 = m_firstSegForAngle.endPt;
                            gp_Pnt2d b1 = segInfo.startPt;
                            gp_Pnt2d b2 = segInfo.endPt;
                            
                            double tol = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom);
                            if (a1.Distance(b1) < tol) { vertex = a1; pt1 = a2; pt2 = b2; found = true; }
                            else if (a1.Distance(b2) < tol) { vertex = a1; pt1 = a2; pt2 = b1; found = true; }
                            else if (a2.Distance(b1) < tol) { vertex = a2; pt1 = a1; pt2 = b2; found = true; }
                            else if (a2.Distance(b2) < tol) { vertex = a2; pt1 = a1; pt2 = b1; found = true; }
                            
                            if (found) {
                                auto angDim = std::make_shared<CADEngine::AngularDimension>(vertex, pt1, pt2);
                                angDim->setSourceEntity1(m_firstSegForAngle.entity, m_firstSegForAngle.segmentIndex);
                                angDim->setSourceEntity2(segInfo.entity, segInfo.segmentIndex);
                                emit statusMessage(QString("Cotation angle: %1°").arg(angDim->getValue(), 0, 'f', 1));
                                emit dimensionCreated(angDim, m_activeSketch->getSketch2D());
                            } else {
                                emit statusMessage("Les 2 segments n'ont pas de point commun !");
                            }
                            m_firstSegForAngle = AngleSegmentInfo();
                            update();
                        }
                    }
                    // === CTRL + clic → Distance perpendiculaire entre 2 lignes ===
                    else if ((event->modifiers() & Qt::ControlModifier) && clickedEntity &&
                        (clickedEntity->getType() == CADEngine::SketchEntityType::Line ||
                         clickedEntity->getType() == CADEngine::SketchEntityType::Polyline)) {
                        
                        AngleSegmentInfo segInfo;
                        segInfo.entity = clickedEntity;
                        
                        if (clickedEntity->getType() == CADEngine::SketchEntityType::Line) {
                            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(clickedEntity);
                            segInfo.segmentIndex = -1;
                            segInfo.startPt = line->getStart();
                            segInfo.endPt = line->getEnd();
                        } else {
                            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(clickedEntity);
                            auto points = polyline->getPoints();
                            double minDist = 1e10;
                            int closestSeg = -1;
                            for (size_t i = 0; i < points.size() - 1; i++) {
                                double dx = points[i+1].X() - points[i].X();
                                double dy = points[i+1].Y() - points[i].Y();
                                double len2 = dx*dx + dy*dy;
                                if (len2 < 1e-6) continue;
                                double t = ((snapped.X() - points[i].X()) * dx + (snapped.Y() - points[i].Y()) * dy) / len2;
                                t = std::max(0.0, std::min(1.0, t));
                                double px = points[i].X() + t * dx, py = points[i].Y() + t * dy;
                                double dist = std::sqrt((snapped.X()-px)*(snapped.X()-px) + (snapped.Y()-py)*(snapped.Y()-py));
                                if (dist < minDist) { minDist = dist; closestSeg = (int)i; }
                            }
                            if (closestSeg < 0) { emit statusMessage("Segment introuvable !"); return; }
                            segInfo.segmentIndex = closestSeg;
                            segInfo.startPt = points[closestSeg];
                            segInfo.endPt = points[closestSeg + 1];
                        }
                        
                        if (!m_firstLineForDistance.entity) {
                            // 1ère ligne sélectionnée
                            m_firstLineForDistance = segInfo;
                            emit statusMessage("Ligne 1 sélectionnée — Ctrl+Clic sur la 2ème ligne (distance perpendiculaire)");
                        } else {
                            // 2ème ligne → calculer distance perpendiculaire
                            gp_Pnt2d a1 = m_firstLineForDistance.startPt;
                            gp_Pnt2d a2 = m_firstLineForDistance.endPt;
                            gp_Pnt2d b1 = segInfo.startPt;
                            gp_Pnt2d b2 = segInfo.endPt;
                            
                            // Direction de la ligne 1 (référence pour la perpendiculaire)
                            double adx = a2.X() - a1.X();
                            double ady = a2.Y() - a1.Y();
                            double alen = std::sqrt(adx*adx + ady*ady);
                            
                            if (alen > 1e-6) {
                                // Normale de la ligne 1
                                double nx = -ady / alen;
                                double ny = adx / alen;
                                
                                // Distance perpendiculaire = projection de (b_milieu - a_milieu) sur la normale
                                double aMidX = (a1.X() + a2.X()) / 2.0;
                                double aMidY = (a1.Y() + a2.Y()) / 2.0;
                                double bMidX = (b1.X() + b2.X()) / 2.0;
                                double bMidY = (b1.Y() + b2.Y()) / 2.0;
                                
                                double perpDist = std::abs((bMidX - aMidX) * nx + (bMidY - aMidY) * ny);
                                
                                // Points de la cotation: pied de la perpendiculaire sur chaque ligne
                                // Point sur ligne 1 = milieu ligne 1
                                gp_Pnt2d dimP1(aMidX, aMidY);
                                // Point sur ligne 2 = projection perpendiculaire du dimP1 sur ligne 2,
                                // ou bien dimP1 + perpDist * normale
                                double sign = ((bMidX - aMidX) * nx + (bMidY - aMidY) * ny) >= 0 ? 1.0 : -1.0;
                                gp_Pnt2d dimP2(aMidX + sign * perpDist * nx, aMidY + sign * perpDist * ny);
                                
                                if (perpDist > 0.5) {
                                    auto dim = std::make_shared<CADEngine::LinearDimension>(dimP1, dimP2);
                                    
                                    // Stocker les sources: -4 = SketchLine, -(100+segIdx) = polyline segment
                                    int vtx1 = m_firstLineForDistance.segmentIndex >= 0 ? 
                                        -(100 + m_firstLineForDistance.segmentIndex) : -4;
                                    int vtx2 = segInfo.segmentIndex >= 0 ?
                                        -(100 + segInfo.segmentIndex) : -4;
                                    dim->setSourceEntity1(m_firstLineForDistance.entity, vtx1);
                                    dim->setSourceEntity2(segInfo.entity, vtx2);
                                    
                                    emit dimensionCreated(dim, m_activeSketch->getSketch2D());
                                    emit statusMessage(QString("Distance entre lignes: %1 mm (perpendiculaire)")
                                        .arg(perpDist, 0, 'f', 1));
                                } else {
                                    emit statusMessage("Lignes trop proches (distance ≈ 0)");
                                }
                            }
                            
                            m_firstLineForDistance = AngleSegmentInfo();
                            update();
                        }
                    }
                    // === Mode normal : cotation point-à-point ou auto ===
                    else {
                        // Chercher un endpoint/centre proche du clic
                        auto endpoint = findNearestEndpoint(snapped, tolerance);
                        
                        if (m_hasDimensionFirstPoint) {
                            // *** 2ème clic → créer la cotation ***
                            gp_Pnt2d p2;
                            std::shared_ptr<CADEngine::SketchEntity> srcEntity2 = nullptr;
                            int srcVtx2 = -99;
                            
                            if (endpoint.entity) {
                                // Accroché à un endpoint
                                p2 = endpoint.point;
                                srcEntity2 = endpoint.entity;
                                srcVtx2 = endpoint.vertexIndex;
                            } else {
                                // Clic dans le vide → utiliser le point snappé
                                p2 = snapped;
                            }
                            
                            // Vérifier qu'on ne crée pas une cotation de longueur 0
                            if (m_firstDimensionEndpoint.point.Distance(p2) > 0.5) {
                                auto dim = std::make_shared<CADEngine::LinearDimension>(
                                    m_firstDimensionEndpoint.point, p2);
                                
                                // Stocker les sources pour refresh au drag
                                if (m_firstDimensionEndpoint.entity) {
                                    dim->setSourceEntity1(m_firstDimensionEndpoint.entity,
                                                          m_firstDimensionEndpoint.vertexIndex);
                                }
                                if (srcEntity2) {
                                    dim->setSourceEntity2(srcEntity2, srcVtx2);
                                }
                                
                                emit dimensionCreated(dim, m_activeSketch->getSketch2D());
                                emit statusMessage(QString("Cotation créée: %1 mm")
                                    .arg(dim->getValue(), 0, 'f', 1));
                            }
                            
                            // Reset
                            m_hasDimensionFirstPoint = false;
                            m_firstDimensionEndpoint = EndpointInfo();
                            m_firstDimensionEndpoint.entity = nullptr;
                            m_firstDimensionEndpoint.vertexIndex = -99;
                            update();
                        }
                        else if (endpoint.entity) {
                            // *** 1er clic sur un endpoint → stocker ***
                            m_firstDimensionEndpoint = endpoint;
                            m_hasDimensionFirstPoint = true;
                            emit statusMessage(QString("Point 1 sélectionné (%1, %2) — Cliquez sur le 2ème point")
                                .arg(endpoint.point.X(), 0, 'f', 1)
                                .arg(endpoint.point.Y(), 0, 'f', 1));
                            update();
                        }
                        else if (clickedEntity) {
                            // *** Clic sur le corps d'une entité (pas près d'un endpoint) ***
                            // → Auto-dimension classique (longueur segment, diamètre cercle...)
                            handleAutoDimension(clickedEntity, snapped);
                        }
                        else {
                            // *** Clic dans le vide ***
                            if (!m_hasDimensionFirstPoint) {
                                // Collecter comme point manuel
                                m_tempPoints.push_back(snapped);
                                m_hasHoverPoint = false;
                                if (m_tempPoints.size() >= 2) {
                                    handleDimensionToolFinish();
                                    m_tempPoints.clear();
                                }
                            }
                        }
                    }
                } else if (m_currentTool == SketchTool::Ellipse) {
                    // Ellipse: clic 1=centre, clic 2=grand axe, clic 3=petit axe
                    m_tempPoints.push_back(snapped);
                    m_hasHoverPoint = false;
                    if (m_tempPoints.size() == 1)
                        emit statusMessage("Ellipse - Cliquez le point du grand axe");
                    else if (m_tempPoints.size() == 2)
                        emit statusMessage("Ellipse - Cliquez le point du petit axe (ou clic droit pour 60%)");
                    if (m_tempPoints.size() >= 3) {
                        handleEllipseToolFinish();
                        m_tempPoints.clear();
                    }
                } else if (m_currentTool == SketchTool::ArcCenter) {
                    // Arc Centre: clic 1=centre, clic 2=début, clic 3=fin
                    m_tempPoints.push_back(snapped);
                    m_hasHoverPoint = false;
                    if (m_tempPoints.size() == 1) {
                        emit statusMessage("Arc Centre - Cliquez le point de départ");
                    }
                    else if (m_tempPoints.size() == 2) {
                        emit statusMessage("Arc Centre - Déplacez et cliquez le point de fin");
                        // Démarrer le suivi d'angle accumulé
                        gp_Pnt2d center = m_tempPoints[0];
                        gp_Pnt2d startPt = m_tempPoints[1];
                        m_arcCenterLastAngle = std::atan2(startPt.Y() - center.Y(), startPt.X() - center.X());
                        m_arcCenterAccumSweep = 0.0;
                        m_arcCenterTracking = true;
                    }
                    if (m_tempPoints.size() >= 3) {
                        m_arcCenterTracking = false;
                        handleArcCenterToolFinish();
                        m_tempPoints.clear();
                        m_hasHoverPoint = false;
                    }
                } else if (m_currentTool == SketchTool::Oblong) {
                    // Oblong: clic 1=centre, clic 2=longueur+orientation, clic 3=largeur
                    if (!m_tempPoints.empty()) {
                        snapped = snapToHV(m_tempPoints.back(), snapped, 5.0);
                    }
                    m_tempPoints.push_back(snapped);
                    m_hasHoverPoint = false;
                    if (m_tempPoints.size() == 1)
                        emit statusMessage("Oblong - Cliquez l'extrémité (longueur + orientation)");
                    else if (m_tempPoints.size() == 2)
                        emit statusMessage("Oblong - Cliquez pour la largeur (ou clic droit = auto)");
                    if (m_tempPoints.size() >= 3) {
                        handleOblongToolFinish();
                        m_tempPoints.clear();
                        m_hasHoverPoint = false;
                    }
                } else {
                    // Polyline : ajouter point avec snap H/V
                    if (!m_tempPoints.empty()) {
                        snapped = snapToHV(m_tempPoints.back(), snapped, 5.0);
                    }
                    m_tempPoints.push_back(snapped);
                    m_hasHoverPoint = false;
                    qDebug() << "[Press] Polyline - Point" << m_tempPoints.size();
                }
                
                update();
            } else {
                // Autres outils = Mode DRAG (comportement actuel)
                m_tempPoints.clear();
                m_tempPoints.push_back(snapped);
                update();
            }
        }
        // Clic droit en mode Polyline = Finir
        else if (event->button() == Qt::RightButton && m_currentTool == SketchTool::Polyline && m_tempPoints.size() >= 2) {
            qDebug() << "[Press] Clic droit - Finir Polyline";
            handlePolylineToolFinish();
            m_tempPoints.clear();
            m_hasHoverPoint = false;
            update();
        }
        // Clic droit en mode Ellipse avec 2 points = finir avec ratio 60%
        else if (event->button() == Qt::RightButton && m_currentTool == SketchTool::Ellipse && m_tempPoints.size() == 2) {
            handleEllipseToolFinish();
            m_tempPoints.clear();
            m_hasHoverPoint = false;
            update();
        }
        // Clic droit en mode Oblong avec 2 points = finir avec largeur = longueur/3
        else if (event->button() == Qt::RightButton && m_currentTool == SketchTool::Oblong && m_tempPoints.size() == 2) {
            handleOblongToolFinish();
            m_tempPoints.clear();
            m_hasHoverPoint = false;
            update();
        }
        else if (event->button() == Qt::MiddleButton) {
            m_isPanning = true;
        }
    }
    else {
        // Mode 3D - navigation & interaction
        if (event->button() == Qt::LeftButton) {
            // Profile selection mode (Fusion 360 style) — highest priority
            if (m_profileSelectionMode) {
                makeCurrent();
                int idx = pickProfileRegionAtScreen(event->pos().x(), event->pos().y());
                if (idx >= 0 && idx < (int)m_profileRegions.size()) {
                    // Toggle selection
                    if (m_selectedProfileIndices.count(idx)) {
                        m_selectedProfileIndices.erase(idx);
                    } else {
                        m_selectedProfileIndices.insert(idx);
                    }
                    
                    int count = (int)m_selectedProfileIndices.size();
                    if (count > 0) {
                        emit statusMessage(QString("%1 profil%2 sélectionné%3 — Entrée pour confirmer, Echap pour annuler")
                            .arg(count).arg(count > 1 ? "s" : "").arg(count > 1 ? "s" : ""));
                    } else {
                        emit statusMessage(QString("Cliquez un profil pour le sélectionner (%1 régions)")
                            .arg(m_profileRegions.size()));
                    }
                    update();
                    return;
                }
            }
            // Face selection mode (Sketch on Face)
            else if (m_faceSelectionMode && !m_currentBody.IsNull()) {
                makeCurrent();
                TopoDS_Face face = pickFaceAtScreen(event->pos().x(), event->pos().y());
                if (!face.IsNull()) {
                    m_selectedFace = face;
                    m_hasSelectedFace = true;
                    // Don't exit face selection mode here — let the handler decide
                    // (allows retry if face is non-planar)
                    emit faceSelected(face);
                    update();
                    return;
                }
            }
            // Edge selection mode (fillet/chamfer)
            else if (m_selectingEdges && !m_currentBody.IsNull()) {
                makeCurrent();
                TopoDS_Edge edge = pickEdgeAtScreen(event->pos().x(), event->pos().y());
                if (!edge.IsNull()) {
                    // Toggle edge selection
                    bool found = false;
                    for (auto it = m_selectedEdges3D.begin(); it != m_selectedEdges3D.end(); ++it) {
                        if (it->IsEqual(edge)) {
                            m_selectedEdges3D.erase(it);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        m_selectedEdges3D.push_back(edge);
                    }
                    emit edgeSelected(edge);
                    emit statusMessage(QString("%1 arête(s) sélectionnée(s)").arg(m_selectedEdges3D.size()));
                    update();
                    return;
                }
            }
            // Face click (for sketch on face)
            else if (!m_currentBody.IsNull()) {
                makeCurrent();
                TopoDS_Face face = pickFaceAtScreen(event->pos().x(), event->pos().y());
                if (!face.IsNull()) {
                    m_selectedFace = face;
                    m_hasSelectedFace = true;
                    emit faceClicked(face);
                }
            }
        }
        else if (event->button() == Qt::RightButton) {
            m_isRotating = true;
        }
        else if (event->button() == Qt::MiddleButton) {
            m_isPanning = true;
        }
    }
}

void Viewport3D::mouseMoveEvent(QMouseEvent* event) {
    int dx = (int)event->position().x() - m_lastMousePos.x();
    int dy = (int)event->position().y() - m_lastMousePos.y();
    
    // TOUJOURS détecter extrémités au survol (mode Sketch uniquement)
    if (m_mode == ViewMode::SKETCH_2D && m_activeSketch) {
        gp_Pnt2d mousePt = screenToSketch(event->pos().x(), event->pos().y());
        double tolerance = CADEngine::ViewportScaling::getClickTolerance(m_sketch2DZoom);
        
        m_hoveredEndpoints.clear();
        
        auto sketch2D = m_activeSketch->getSketch2D();
        if (sketch2D) {
            for (const auto& entity : sketch2D->getEntities()) {
                auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
                if (line) {
                    // Vérifier proximité des extrémités
                    double dist1 = mousePt.Distance(line->getStart());
                    double dist2 = mousePt.Distance(line->getEnd());
                    
                    if (dist1 < tolerance) {
                        m_hoveredEndpoints.push_back(line->getStart());
                    }
                    if (dist2 < tolerance) {
                        m_hoveredEndpoints.push_back(line->getEnd());
                    }
                }
                
                // Support rectangle
                auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
                if (rect) {
                    auto corners = rect->getKeyPoints();
                    for (const auto& corner : corners) {
                        if (mousePt.Distance(corner) < tolerance) {
                            m_hoveredEndpoints.push_back(corner);
                        }
                    }
                }
                
                // Support arc
                auto arc = std::dynamic_pointer_cast<CADEngine::SketchArc>(entity);
                if (arc) {
                    double dist1 = mousePt.Distance(arc->getStart());
                    double dist2 = mousePt.Distance(arc->getEnd());
                    
                    if (dist1 < tolerance) {
                        m_hoveredEndpoints.push_back(arc->getStart());
                    }
                    if (dist2 < tolerance) {
                        m_hoveredEndpoints.push_back(arc->getEnd());
                    }
                }
                
                // Support cercle (centre)
                auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
                if (circle) {
                    if (mousePt.Distance(circle->getCenter()) < tolerance) {
                        m_hoveredEndpoints.push_back(circle->getCenter());
                    }
                }
                
                // Support polyline - TOUS les points !
                auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
                if (polyline) {
                    auto points = polyline->getPoints();
                    for (const auto& pt : points) {
                        if (mousePt.Distance(pt) < tolerance) {
                            m_hoveredEndpoints.push_back(pt);
                        }
                    }
                }
                
                // Support ellipse (centre + extrémités axes)
                auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
                if (ellipse) {
                    auto kp = ellipse->getKeyPoints();
                    for (const auto& pt : kp) {
                        if (mousePt.Distance(pt) < tolerance) {
                            m_hoveredEndpoints.push_back(pt);
                        }
                    }
                }
                
                // Support polygone (centre + vertices)
                auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
                if (polygon) {
                    if (mousePt.Distance(polygon->getCenter()) < tolerance) {
                        m_hoveredEndpoints.push_back(polygon->getCenter());
                    }
                    for (const auto& v : polygon->getVertices()) {
                        if (mousePt.Distance(v) < tolerance) {
                            m_hoveredEndpoints.push_back(v);
                        }
                    }
                }
                
                // Support oblong (centre + keypoints)
                auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
                if (oblong) {
                    for (const auto& kp : oblong->getKeyPoints()) {
                        if (mousePt.Distance(kp) < tolerance) {
                            m_hoveredEndpoints.push_back(kp);
                        }
                    }
                }
            }
        }
        
        update();  // Redessiner pour afficher les points
    }
    
    // Mode Sketch - Drag dimension (changer offset)
    if (m_mode == ViewMode::SKETCH_2D && m_isDraggingDimension && m_draggedDimension) {
        gp_Pnt2d currentPt = screenToSketch(event->pos().x(), event->pos().y());
        
        // Calculer nouveau offset basé sur la distance perpendiculaire
        if (m_draggedDimension->getType() == CADEngine::DimensionType::Linear) {
            auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(m_draggedDimension);
            if (linearDim) {
                gp_Pnt2d p1 = linearDim->getPoint1();
                gp_Pnt2d p2 = linearDim->getPoint2();
                
                // Vecteur de la ligne
                double dx_line = p2.X() - p1.X();
                double dy_line = p2.Y() - p1.Y();
                double len = std::sqrt(dx_line*dx_line + dy_line*dy_line);
                
                if (len > 1e-6) {
                    // Vecteur perpendiculaire normalisé
                    double perpX = -dy_line / len;
                    double perpY = dx_line / len;
                    
                    // Milieu de la ligne
                    double mx = (p1.X() + p2.X()) / 2.0;
                    double my = (p1.Y() + p2.Y()) / 2.0;
                    
                    // Distance du curseur au milieu, projetée sur la perpendiculaire
                    double vec_x = currentPt.X() - mx;
                    double vec_y = currentPt.Y() - my;
                    double newOffset = vec_x * perpX + vec_y * perpY;
                    
                    // Appliquer nouvel offset
                    linearDim->setOffset(newOffset);
                    
                    // Persister l'offset pour les auto-cotations (survit à la régénération)
                    if (m_draggedDimension->isAutomatic() && m_draggedDimension->getAutoSourceEntity()) {
                        auto sketch2D = m_activeSketch->getSketch2D();
                        if (sketch2D) {
                            sketch2D->setAutoDimOffset(
                                m_draggedDimension->getAutoSourceEntity()->getId(),
                                m_draggedDimension->getAutoSourceProperty(),
                                newOffset);
                        }
                    }
                    
                    emit statusMessage(QString("Offset: %1 mm").arg(newOffset, 0, 'f', 1));
                }
            }
        }
        else if (m_draggedDimension->getType() == CADEngine::DimensionType::Diametral) {
            auto dd = std::dynamic_pointer_cast<CADEngine::DiametralDimension>(m_draggedDimension);
            if (dd) {
                // Changer l'angle de la ligne de cote (tourner autour du centre)
                double dx = currentPt.X() - dd->getCenter().X();
                double dy = currentPt.Y() - dd->getCenter().Y();
                double newAngle = std::atan2(dy, dx) * 180.0 / M_PI;
                dd->setAngle(newAngle);
                
                // Persister l'angle pour les auto-cotations
                if (m_draggedDimension->isAutomatic() && m_draggedDimension->getAutoSourceEntity()) {
                    auto sketch2D = m_activeSketch->getSketch2D();
                    if (sketch2D) {
                        sketch2D->setAutoDimAngle(
                            m_draggedDimension->getAutoSourceEntity()->getId(),
                            m_draggedDimension->getAutoSourceProperty(),
                            newAngle);
                    }
                }
                
                emit statusMessage(QString("Angle cotation: %1°").arg(newAngle, 0, 'f', 1));
            }
        }
        else if (m_draggedDimension->getType() == CADEngine::DimensionType::Radial) {
            auto rd = std::dynamic_pointer_cast<CADEngine::RadialDimension>(m_draggedDimension);
            if (rd) {
                // Déplacer le point de flèche le long du cercle
                double dx = currentPt.X() - rd->getCenter().X();
                double dy = currentPt.Y() - rd->getCenter().Y();
                double angle = std::atan2(dy, dx);
                double r = rd->getRadius();
                rd->setArrowPoint(gp_Pnt2d(rd->getCenter().X() + r * std::cos(angle),
                                           rd->getCenter().Y() + r * std::sin(angle)));
                rd->setTextPosition(gp_Pnt2d(
                    (rd->getCenter().X() + rd->getArrowPoint().X()) / 2.0,
                    (rd->getCenter().Y() + rd->getArrowPoint().Y()) / 2.0));
                emit statusMessage(QString("Position cotation: %1°").arg(angle * 180.0 / M_PI, 0, 'f', 1));
                
                // Persister l'angle pour les auto-cotations
                if (m_draggedDimension->isAutomatic() && m_draggedDimension->getAutoSourceEntity()) {
                    auto sketch2D = m_activeSketch->getSketch2D();
                    if (sketch2D) {
                        sketch2D->setAutoDimAngle(
                            m_draggedDimension->getAutoSourceEntity()->getId(),
                            m_draggedDimension->getAutoSourceProperty(),
                            angle * 180.0 / M_PI);
                    }
                }
            }
        }
        
        update();
        return;
    }
    
    // Mode Sketch - Drag entité en mode sélection
    if (m_mode == ViewMode::SKETCH_2D && m_isDraggingEntity && m_draggedEntity) {
        gp_Pnt2d currentPt = screenToSketch(event->pos().x(), event->pos().y());
        // Snap aux autres entités SAUF celle qu'on drag (évite le tremblement)
        gp_Pnt2d snapped = snapToEntities(currentPt, 15.0, m_draggedEntity);
        
        double deltaX = snapped.X() - m_dragStartPoint.X();
        double deltaY = snapped.Y() - m_dragStartPoint.Y();
        
        // Déplacer l'entité
        if (m_draggedEntity->getType() == CADEngine::SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(m_draggedEntity);
            if (line) {
                gp_Pnt2d oldStart = line->getStart();
                gp_Pnt2d oldEnd = line->getEnd();
                gp_Pnt2d newStart(oldStart.X() + deltaX, oldStart.Y() + deltaY);
                gp_Pnt2d newEnd(oldEnd.X() + deltaX, oldEnd.Y() + deltaY);
                
                line->setStart(newStart);
                line->setEnd(newEnd);
                
                // Mettre à jour les dimensions associées
                updateDimensionsForLine(oldStart, oldEnd, newStart, newEnd);
            }
        }
        else if (m_draggedEntity->getType() == CADEngine::SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(m_draggedEntity);
            if (circle) {
                gp_Pnt2d oldCenter = circle->getCenter();
                gp_Pnt2d newCenter(oldCenter.X() + deltaX, oldCenter.Y() + deltaY);
                
                circle->setCenter(newCenter);
                
                // Mettre à jour les dimensions associées (diamètre)
                updateDimensionsForCircle(oldCenter, newCenter);
            }
        }
        else if (m_draggedEntity->getType() == CADEngine::SketchEntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(m_draggedEntity);
            if (rect) {
                gp_Pnt2d oldCorner = rect->getCorner();
                double width = rect->getWidth();
                double height = rect->getHeight();
                gp_Pnt2d newCorner(oldCorner.X() + deltaX, oldCorner.Y() + deltaY);
                
                rect->setCorner(newCorner);
                
                // Mettre à jour les dimensions associées
                updateDimensionsForRectangle(oldCorner, width, height, newCorner);
            }
        }
        else if (m_draggedEntity->getType() == CADEngine::SketchEntityType::Arc) {
            auto arc = std::dynamic_pointer_cast<CADEngine::SketchArc>(m_draggedEntity);
            if (arc) {
                // Arc n'a pas de setters - TODO: ajouter dans SketchArc.h
                // Pour l'instant, on ne peut pas déplacer les arcs
                qDebug() << "[Drag] Arc: déplacement non implémenté (pas de setters)";
            }
        }
        else if (m_draggedEntity->getType() == CADEngine::SketchEntityType::Ellipse) {
            auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(m_draggedEntity);
            if (ellipse) {
                gp_Pnt2d oldCenter = ellipse->getCenter();
                gp_Pnt2d newCenter(oldCenter.X() + deltaX, oldCenter.Y() + deltaY);
                ellipse->setCenter(newCenter);
                
                emit statusMessage(QString("Ellipse → (%1, %2)")
                    .arg(newCenter.X(), 0, 'f', 1).arg(newCenter.Y(), 0, 'f', 1));
            }
        }
        else if (m_draggedEntity->getType() == CADEngine::SketchEntityType::Polygon) {
            auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(m_draggedEntity);
            if (polygon) {
                gp_Pnt2d oldCenter = polygon->getCenter();
                gp_Pnt2d newCenter(oldCenter.X() + deltaX, oldCenter.Y() + deltaY);
                polygon->setCenter(newCenter);
                
                emit statusMessage(QString("Polygone → (%1, %2)")
                    .arg(newCenter.X(), 0, 'f', 1).arg(newCenter.Y(), 0, 'f', 1));
            }
        }
        else if (m_draggedEntity->getType() == CADEngine::SketchEntityType::Oblong) {
            auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(m_draggedEntity);
            if (oblong) {
                gp_Pnt2d oldCenter = oblong->getCenter();
                gp_Pnt2d newCenter(oldCenter.X() + deltaX, oldCenter.Y() + deltaY);
                oblong->setCenter(newCenter);
                
                emit statusMessage(QString("Oblong → (%1, %2)")
                    .arg(newCenter.X(), 0, 'f', 1).arg(newCenter.Y(), 0, 'f', 1));
            }
        }
        else if (m_draggedEntity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(m_draggedEntity);
            if (polyline) {
                auto points = polyline->getPoints();
                
                if (m_isDraggingVertex && m_draggedVertexIndex >= 0 
                    && m_draggedVertexIndex < (int)points.size()) {
                    // MODE VERTEX : déplacer uniquement ce sommet
                    gp_Pnt2d newPos(points[m_draggedVertexIndex].X() + deltaX,
                                     points[m_draggedVertexIndex].Y() + deltaY);
                    points[m_draggedVertexIndex] = newPos;
                    polyline->setPoints(points);
                    
                    // Mettre à jour les dimensions des segments adjacents
                    updateDimensionsForPolylineVertex(polyline, m_draggedVertexIndex);
                    
                    emit statusMessage(QString("Vertex %1 → (%2, %3)")
                        .arg(m_draggedVertexIndex)
                        .arg(newPos.X(), 0, 'f', 2)
                        .arg(newPos.Y(), 0, 'f', 2));
                } else {
                    // MODE ENTITÉ : translation de toute la polyline
                    for (size_t i = 0; i < points.size(); i++) {
                        points[i] = gp_Pnt2d(points[i].X() + deltaX, 
                                              points[i].Y() + deltaY);
                    }
                    polyline->setPoints(points);
                    
                    // Mettre à jour toutes les dimensions liées
                    int polylineId = polyline->getId();
                    auto sketch2D = m_activeSketch->getSketch2D();
                    if (sketch2D) {
                        for (const auto& dim : sketch2D->getDimensions()) {
                            if (dim->getType() != CADEngine::DimensionType::Linear) continue;
                            auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
                            if (!linearDim) continue;
                            
                            if (linearDim->getEntityId() == polylineId) {
                                int segIdx = linearDim->getSegmentIndex();
                                if (segIdx >= 0 && segIdx < (int)points.size() - 1) {
                                    linearDim->setPoint1(points[segIdx]);
                                    linearDim->setPoint2(points[segIdx + 1]);
                                }
                            }
                        }
                        // Rafraîchir cotations angulaires liées à cette polyline
                        for (const auto& dim : sketch2D->getDimensions()) {
                            if (dim->getType() != CADEngine::DimensionType::Angular) continue;
                            auto angularDim = std::dynamic_pointer_cast<CADEngine::AngularDimension>(dim);
                            if (!angularDim) continue;
                            if (angularDim->getSourceEntity1() == polyline ||
                                angularDim->getSourceEntity2() == polyline) {
                                angularDim->refreshFromSources();
                            }
                        }
                    }
                }
            }
        }
        
        // === Rafraîchir toutes les cotations cross-entity (point-à-point) ===
        {
            auto sketch2D = m_activeSketch->getSketch2D();
            if (sketch2D) {
                for (const auto& dim : sketch2D->getDimensions()) {
                    if (dim->getType() != CADEngine::DimensionType::Linear) continue;
                    auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
                    if (!linearDim || !linearDim->hasSources()) continue;
                    
                    // Vérifier si une des sources est l'entité draggée
                    if (linearDim->getSourceEntity1() == m_draggedEntity ||
                        linearDim->getSourceEntity2() == m_draggedEntity) {
                        linearDim->refreshFromSources();
                    }
                }
            }
        }
        
        m_dragStartPoint = snapped;  // Update point de départ pour le prochain delta
        
        // Ré-appliquer les contraintes pendant le drag (symétrie temps réel)
        if (m_activeSketch && m_activeSketch->getSketch2D()) {
            m_activeSketch->getSketch2D()->solveConstraints();
        }
        
        update();
        return;
    }
    
    // Mode Sketch avec outil actif et point de départ
    if (m_mode == ViewMode::SKETCH_2D && m_currentTool != SketchTool::None && !m_tempPoints.empty()) {
        // Arc et Polyline = Preview du prochain point au survol
        if (m_currentTool == SketchTool::Arc || m_currentTool == SketchTool::Polyline ||
            m_currentTool == SketchTool::ArcCenter || m_currentTool == SketchTool::Ellipse ||
            m_currentTool == SketchTool::Oblong) {
            // Stocker point de survol pour preview
            gp_Pnt2d hoverPt = screenToSketch(event->pos().x(), event->pos().y());
            
            // PAS de snap pour Arc (mouvement fluide de courbure)
            // Snap seulement pour Polyline/Oblong/Ellipse
            if (m_currentTool == SketchTool::Arc || m_currentTool == SketchTool::ArcCenter) {
                m_hoverPoint = hoverPt;  // Pas de snap = mouvement fluide !
                m_hvSnapActive = false;
            } else {
                // Snap entité d'abord (origine, centres, extrémités)
                gp_Pnt2d entitySnapped = snapToEntities(hoverPt);
                if (entitySnapped.Distance(hoverPt) > 1e-6) {
                    m_hoverPoint = entitySnapped;  // Snap entité trouvé
                } else {
                    m_hoverPoint = snapToGrid(hoverPt, 10.0);  // Fallback grille
                }
                // Snap H/V automatique si on a un point d'origine
                if (!m_tempPoints.empty()) {
                    m_hoverPoint = snapToHV(m_tempPoints.back(), m_hoverPoint, 5.0);
                } else {
                    m_hvSnapActive = false;
                }
            }
            
            m_hasHoverPoint = true;
            
            // Suivi d'angle accumulé pour Arc Centre
            if (m_arcCenterTracking && m_currentTool == SketchTool::ArcCenter && m_tempPoints.size() == 2) {
                gp_Pnt2d center = m_tempPoints[0];
                double currentAngle = std::atan2(m_hoverPoint.Y() - center.Y(), 
                                                  m_hoverPoint.X() - center.X());
                double delta = currentAngle - m_arcCenterLastAngle;
                // Normaliser delta entre -π et +π pour gérer le wrap-around de atan2
                while (delta > M_PI) delta -= 2.0 * M_PI;
                while (delta < -M_PI) delta += 2.0 * M_PI;
                m_arcCenterAccumSweep += delta;
                m_arcCenterLastAngle = currentAngle;
            }
            
            update();
        } else {
            // Autres outils = Mode DRAG, update pendant le mouvement
            gp_Pnt2d currentPt = screenToSketch(event->pos().x(), event->pos().y());
            gp_Pnt2d snapped = snapToEntities(currentPt);  // Snap entité + grille fallback
            
            // Snap H/V pour outil Ligne
            if (m_currentTool == SketchTool::Line && !m_tempPoints.empty()) {
                snapped = snapToHV(m_tempPoints[0], snapped, 5.0);
            } else {
                m_hvSnapActive = false;
            }
            
            // Stocker le point actuel pour le preview
            if (m_tempPoints.size() == 2) {
                m_tempPoints[1] = snapped;  // Update le 2ème point
            } else {
                m_tempPoints.push_back(snapped);  // Ajouter le 2ème point
            }
            
            update();  // Redessiner le preview
        }
    }
    // Navigation 3D
    else if (m_mode == ViewMode::NORMAL_3D) {
        if (m_isRotating) {
            rotateView(dx, dy);
        }
        // Profile selection mode (Fusion 360 style)
        else if (m_profileSelectionMode) {
            makeCurrent();
            int idx = pickProfileRegionAtScreen(event->pos().x(), event->pos().y());
            if (idx != m_hoveredRegionIndex) {
                m_hoveredRegionIndex = idx;
                int selCount = (int)m_selectedProfileIndices.size();
                if (idx >= 0) {
                    bool isSel = m_selectedProfileIndices.count(idx) > 0;
                    emit statusMessage(QString("Profil: %1 %2 — %3 sélectionné(s) — Entrée=confirmer")
                        .arg(QString::fromStdString(m_profileRegions[idx].description))
                        .arg(isSel ? "✓" : "")
                        .arg(selCount));
                } else if (selCount > 0) {
                    emit statusMessage(QString("%1 profil(s) sélectionné(s) — Entrée=confirmer, Echap=annuler")
                        .arg(selCount));
                }
                update();
            }
        }
        // Face/edge hover highlight — TOUJOURS actif en 3D quand il y a un body
        else if (!m_currentBody.IsNull() && !m_isRotating && !m_isPanning) {
            // Throttle : pick max toutes les 50ms pour éviter les ralentissements
            static auto lastPickTime = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPickTime).count() >= 50) {
                lastPickTime = now;
                makeCurrent();
                // Face hover (toujours actif pour pré-sélection)
                {
                    TopoDS_Face face = pickFaceAtScreen(event->pos().x(), event->pos().y());
                    bool changed = false;
                    if (face.IsNull() && !m_hoveredFace.IsNull()) { m_hoveredFace = TopoDS_Face(); changed = true; }
                    else if (!face.IsNull() && (m_hoveredFace.IsNull() || !face.IsEqual(m_hoveredFace))) { m_hoveredFace = face; changed = true; }
                    if (changed) update();
                }
                // Edge hover (uniquement en mode sélection d'arêtes)
                if (m_selectingEdges) {
                    TopoDS_Edge edge = pickEdgeAtScreen(event->pos().x(), event->pos().y());
                    bool changed = false;
                    if (edge.IsNull() && !m_hoveredEdge.IsNull()) { m_hoveredEdge = TopoDS_Edge(); changed = true; }
                    else if (!edge.IsNull() && (m_hoveredEdge.IsNull() || !edge.IsEqual(m_hoveredEdge))) { m_hoveredEdge = edge; changed = true; }
                    if (changed) update();
                }
            }
        }
    }
    
    // Pan (les deux modes)
    if (m_isPanning) {
        panView(dx, dy);
    }
    
    m_lastMousePos = event->pos();
}

void Viewport3D::mouseReleaseEvent(QMouseEvent* event) {
    // Arrêter drag dimension
    if (m_isDraggingDimension && event->button() == Qt::LeftButton) {
        m_isDraggingDimension = false;
        m_draggedDimension.reset();
        emit sketchEntityAdded();  // Notifier modification
        emit statusMessage("Offset de cotation ajusté");
        return;
    }
    
    // Arrêter drag entité
    if (m_isDraggingEntity && event->button() == Qt::LeftButton) {
        m_isDraggingEntity = false;
        m_isDraggingVertex = false;
        m_draggedVertexIndex = -1;
        m_draggedEntity.reset();
        
        // Ré-appliquer les contraintes après le drag
        if (m_activeSketch && m_activeSketch->getSketch2D()) {
            m_activeSketch->getSketch2D()->solveConstraints();
            m_activeSketch->getSketch2D()->regenerateAutoDimensions();
        }
        
        emit sketchEntityAdded();  // Notifier modification
        return;
    }
    
    // Arc, Polyline, Dimension, Ellipse, ArcCenter = Mode CLICS, tout se passe au Press
    // Ne rien faire au Release pour eux
    if (m_currentTool == SketchTool::Arc || m_currentTool == SketchTool::Polyline || 
        m_currentTool == SketchTool::Dimension || m_currentTool == SketchTool::Ellipse ||
        m_currentTool == SketchTool::ArcCenter || m_currentTool == SketchTool::Oblong) {
        // Navigation
        if (event->button() == Qt::LeftButton) {
            m_isRotating = false;
        }
        else if (event->button() == Qt::RightButton) {
            m_isRotating = false;
        }
        else if (event->button() == Qt::MiddleButton) {
            m_isPanning = false;
        }
        return;  // Ne pas traiter le release pour Arc/Polyline
    }
    
    // Mode sketch - créer l'entité au relâché (DRAG tools uniquement)
    if (m_mode == ViewMode::SKETCH_2D && event->button() == Qt::LeftButton 
        && m_currentTool != SketchTool::None && !m_tempPoints.empty()) {
        
        qDebug() << "[Release] TempPoints avant ajout:" << m_tempPoints.size();
        
        // Si on n'a qu'un seul point (pas de mouvement), ajouter le point au release
        if (m_tempPoints.size() == 1) {
            gp_Pnt2d releasePt = screenToSketch(event->pos().x(), event->pos().y());
            gp_Pnt2d snapped = snapToGrid(releasePt, 10.0);
            // Snap H/V pour l'outil Ligne
            if (m_currentTool == SketchTool::Line) {
                snapped = snapToHV(m_tempPoints[0], snapped, 5.0);
            }
            m_tempPoints.push_back(snapped);
            qDebug() << "[Release] Point ajouté, total:" << m_tempPoints.size();
        }
        
        // Créer l'entité (outils DRAG uniquement)
        bool shouldFinish = false;
        
        if (m_tempPoints.size() >= 2) {
            shouldFinish = true;
        }
        
        // Créer l'entité si nécessaire
        if (shouldFinish) {
            switch (m_currentTool) {
                case SketchTool::Line:
                    handleLineToolFinish();
                    break;
                case SketchTool::Circle:
                    handleCircleToolFinish();
                    break;
                case SketchTool::Rectangle:
                    handleRectangleToolFinish();
                    break;
                case SketchTool::Polygon:
                    handlePolygonToolFinish();
                    break;
                case SketchTool::Arc:
                    handleArcToolFinish();
                    break;
                case SketchTool::Polyline:
                    handlePolylineToolFinish();
                    break;
                case SketchTool::Dimension:
                    handleDimensionToolFinish();
                    break;
                default:
                    break;
            }
            
            m_tempPoints.clear();
        }
        
        update();
    }
    
    // Navigation
    if (event->button() == Qt::LeftButton) {
        m_isRotating = false;
    }
    else if (event->button() == Qt::RightButton) {
        m_isRotating = false;
    }
    else if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
    }
}

void Viewport3D::wheelEvent(QWheelEvent* event) {
    zoomView(event->angleDelta().y());
    event->accept();  // Prevent parent widgets from consuming the event
}

// ============================================================================
// Polyline vertex drag helpers
// ============================================================================

int Viewport3D::findPolylineVertexAtPoint(std::shared_ptr<CADEngine::SketchPolyline> polyline,
                                           const gp_Pnt2d& pt, double tolerance) {
    if (!polyline) return -1;
    
    auto points = polyline->getPoints();
    double minDist = tolerance;
    int closestVertex = -1;
    
    for (size_t i = 0; i < points.size(); i++) {
        double dist = pt.Distance(points[i]);
        if (dist < minDist) {
            minDist = dist;
            closestVertex = static_cast<int>(i);
        }
    }
    
    return closestVertex;
}

void Viewport3D::updateDimensionsForPolylineVertex(std::shared_ptr<CADEngine::SketchPolyline> polyline,
                                                     int vertexIndex) {
    if (!polyline || !m_activeSketch) return;
    
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return;
    
    auto points = polyline->getPoints();
    int polylineId = polyline->getId();
    
    // Mettre à jour les dimensions linéaires liées aux segments adjacents au vertex
    for (const auto& dim : sketch2D->getDimensions()) {
        if (dim->getType() != CADEngine::DimensionType::Linear) continue;
        
        auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
        if (!linearDim) continue;
        
        // Dimension appartient à cette polyligne ?
        if (linearDim->getEntityId() == polylineId) {
            int segIdx = linearDim->getSegmentIndex();
            
            // Segment avant le vertex (segIdx == vertexIndex - 1)
            // Segment après le vertex (segIdx == vertexIndex)
            if (segIdx == vertexIndex || segIdx == vertexIndex - 1) {
                if (segIdx >= 0 && segIdx < (int)points.size() - 1) {
                    linearDim->setPoint1(points[segIdx]);
                    linearDim->setPoint2(points[segIdx + 1]);
                }
            }
        }
        
        // Aussi vérifier par proximité de points (dimensions sans entityId)
        if (linearDim->getEntityId() == -1) {
            double tolerance = 1.0;
            gp_Pnt2d p1 = linearDim->getPoint1();
            gp_Pnt2d p2 = linearDim->getPoint2();
            
            // Segment avant le vertex
            if (vertexIndex > 0) {
                gp_Pnt2d segStart = points[vertexIndex - 1];
                gp_Pnt2d segEnd = points[vertexIndex];
                
                if ((p1.Distance(segStart) < tolerance && p2.Distance(segEnd) < tolerance) ||
                    (p1.Distance(segEnd) < tolerance && p2.Distance(segStart) < tolerance)) {
                    linearDim->setPoint1(segStart);
                    linearDim->setPoint2(segEnd);
                }
            }
            // Segment après le vertex
            if (vertexIndex < (int)points.size() - 1) {
                gp_Pnt2d segStart = points[vertexIndex];
                gp_Pnt2d segEnd = points[vertexIndex + 1];
                
                if ((p1.Distance(segStart) < tolerance && p2.Distance(segEnd) < tolerance) ||
                    (p1.Distance(segEnd) < tolerance && p2.Distance(segStart) < tolerance)) {
                    linearDim->setPoint1(segStart);
                    linearDim->setPoint2(segEnd);
                }
            }
        }
    }
    
    // Mettre à jour les cotations angulaires liées à cette polyline
    for (const auto& dim : sketch2D->getDimensions()) {
        if (dim->getType() != CADEngine::DimensionType::Angular) continue;
        
        auto angularDim = std::dynamic_pointer_cast<CADEngine::AngularDimension>(dim);
        if (!angularDim) continue;
        
        // Si cette cotation référence notre polyline, rafraîchir depuis les sources
        if (angularDim->getSourceEntity1() == polyline || 
            angularDim->getSourceEntity2() == polyline) {
            angularDim->refreshFromSources();
        }
    }
}

// ============================================================================
// Find nearest endpoint of any entity (for Coincident constraint)
// ============================================================================

Viewport3D::EndpointInfo Viewport3D::findNearestEndpoint(const gp_Pnt2d& pt, double tolerance) {
    EndpointInfo result;
    result.vertexIndex = -99;  // Invalid
    result.entity = nullptr;
    
    if (!m_activeSketch) return result;
    auto sketch2D = m_activeSketch->getSketch2D();
    if (!sketch2D) return result;
    
    double minDist = tolerance;
    
    for (const auto& entity : sketch2D->getEntities()) {
        // Skip construction entities (axes) — pas de snap points dessus
        if (entity->isConstruction()) continue;
        
        if (entity->getType() == CADEngine::SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
            if (!line) continue;
            
            double distStart = pt.Distance(line->getStart());
            double distEnd = pt.Distance(line->getEnd());
            
            if (distStart < minDist) {
                minDist = distStart;
                result.point = line->getStart();
                result.entity = entity;
                result.vertexIndex = -1;  // start
            }
            if (distEnd < minDist) {
                minDist = distEnd;
                result.point = line->getEnd();
                result.entity = entity;
                result.vertexIndex = -2;  // end
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
            if (!circle) continue;
            
            double distCenter = pt.Distance(circle->getCenter());
            if (distCenter < minDist) {
                minDist = distCenter;
                result.point = circle->getCenter();
                result.entity = entity;
                result.vertexIndex = -3;  // center
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Arc) {
            auto arc = std::dynamic_pointer_cast<CADEngine::SketchArc>(entity);
            if (!arc) continue;
            
            double distStart = pt.Distance(arc->getStart());
            double distEnd = pt.Distance(arc->getEnd());
            
            if (distStart < minDist) {
                minDist = distStart;
                result.point = arc->getStart();
                result.entity = entity;
                result.vertexIndex = -1;  // start
            }
            if (distEnd < minDist) {
                minDist = distEnd;
                result.point = arc->getEnd();
                result.entity = entity;
                result.vertexIndex = -2;  // end
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
            if (!rect) continue;
            
            // Centre du rectangle
            gp_Pnt2d center(rect->getCorner().X() + rect->getWidth()/2.0,
                            rect->getCorner().Y() + rect->getHeight()/2.0);
            double distCenter = pt.Distance(center);
            if (distCenter < minDist) {
                minDist = distCenter;
                result.point = center;
                result.entity = entity;
                result.vertexIndex = -3;  // center
            }
            
            // 4 coins
            auto corners = rect->getKeyPoints();
            for (size_t i = 0; i < corners.size(); i++) {
                double dist = pt.Distance(corners[i]);
                if (dist < minDist) {
                    minDist = dist;
                    result.point = corners[i];
                    result.entity = entity;
                    result.vertexIndex = static_cast<int>(i);  // corner index 0-3
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
            if (!polyline) continue;
            
            auto points = polyline->getPoints();
            for (size_t i = 0; i < points.size(); i++) {
                double dist = pt.Distance(points[i]);
                if (dist < minDist) {
                    minDist = dist;
                    result.point = points[i];
                    result.entity = entity;
                    result.vertexIndex = static_cast<int>(i);  // vertex index
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Ellipse) {
            auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
            if (!ellipse) continue;
            
            double distCenter = pt.Distance(ellipse->getCenter());
            if (distCenter < minDist) {
                minDist = distCenter;
                result.point = ellipse->getCenter();
                result.entity = entity;
                result.vertexIndex = -3;  // center
            }
            // Snap aux extrémités des axes
            auto kp = ellipse->getKeyPoints();
            for (size_t i = 1; i < kp.size(); i++) {  // skip center (index 0)
                double dist = pt.Distance(kp[i]);
                if (dist < minDist) {
                    minDist = dist;
                    result.point = kp[i];
                    result.entity = entity;
                    result.vertexIndex = static_cast<int>(i);
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
            auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
            if (!polygon) continue;
            
            // Centre
            double distCenter = pt.Distance(polygon->getCenter());
            if (distCenter < minDist) {
                minDist = distCenter;
                result.point = polygon->getCenter();
                result.entity = entity;
                result.vertexIndex = -3;
            }
            // Vertices
            auto verts = polygon->getVertices();
            for (size_t i = 0; i < verts.size(); i++) {
                double dist = pt.Distance(verts[i]);
                if (dist < minDist) {
                    minDist = dist;
                    result.point = verts[i];
                    result.entity = entity;
                    result.vertexIndex = static_cast<int>(i);
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
            auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
            if (!oblong) continue;
            
            // Centre
            double distCenter = pt.Distance(oblong->getCenter());
            if (distCenter < minDist) {
                minDist = distCenter;
                result.point = oblong->getCenter();
                result.entity = entity;
                result.vertexIndex = -3;
            }
            // Keypoints (extrémités, centres arcs, etc.)
            auto kp = oblong->getKeyPoints();
            for (size_t i = 1; i < kp.size(); i++) {  // skip center (index 0)
                double dist = pt.Distance(kp[i]);
                if (dist < minDist) {
                    minDist = dist;
                    result.point = kp[i];
                    result.entity = entity;
                    result.vertexIndex = static_cast<int>(i);
                }
            }
        }
    }
    
    return result;
}

// ============================================================================
// Navigation 3D
// ============================================================================

void Viewport3D::rotateView(int dx, int dy) {
    m_cameraAngleY += dx * 0.5f;
    m_cameraAngleX += dy * 0.5f;
    
    // Allow full orbit — clamp to ±179° to avoid exact poles (gimbal lock)
    if (m_cameraAngleX > 179.0f) m_cameraAngleX = 179.0f;
    if (m_cameraAngleX < -179.0f) m_cameraAngleX = -179.0f;
    
    // Synchroniser le ViewCube
    if (m_viewCube) m_viewCube->setCameraAngles(m_cameraAngleX, m_cameraAngleY);
    
    update();
}

void Viewport3D::panView(int dx, int dy) {
    if (m_mode == ViewMode::SKETCH_2D) {
        // Mode sketch : pan proportionnel centralisé
        float panSpeed = CADEngine::ViewportScaling::getPanSpeed(m_sketch2DZoom);
        
        // Mouvement intuitif (droite→droite, haut→haut)
        m_cameraPanX -= dx * panSpeed;
        m_cameraPanY += dy * panSpeed;
    } else {
        // Mode 3D : comportement standard
        float panSpeed = 0.5f;
        m_cameraPanX += dx * panSpeed;
        m_cameraPanY -= dy * panSpeed;
    }
    
    update();
}

void Viewport3D::zoomView(int delta) {
    
    if (m_mode == ViewMode::SKETCH_2D) {
        // Mode Sketch : zoom PROPORTIONNEL (pas linéaire)
        float zoomFactor = 1.0f + (delta / 1200.0f);  // ~10% par cran de molette
        m_sketch2DZoom /= zoomFactor;
        
        // LIMITES ULTIMES - grille optimisée = pas de crash
        if (m_sketch2DZoom < 1.0f) m_sketch2DZoom = 1.0f;        // 1mm minimum - ULTIME !
        if (m_sketch2DZoom > 5000.0f) m_sketch2DZoom = 5000.0f;  // 5000mm max - vue d'ensemble
        
    } else {
        // Mode 3D : zoom PROPORTIONNEL à la distance (comme Fusion 360)
        float zoomFactor = 1.0f + (delta / 1200.0f);  // ~10% par cran de molette
        m_cameraDistance /= zoomFactor;
        
        
        // Limiter le zoom 3D
        if (m_cameraDistance < 10.0f) m_cameraDistance = 10.0f;
        if (m_cameraDistance > 10000.0f) m_cameraDistance = 10000.0f;
    }
    
    update();
}

// ============================================================================
// Rendu de toutes les features du document
// ============================================================================

void Viewport3D::renderAllFeatures() {
    if (!m_document) return;
    
    auto features = m_document->getAllFeatures();
    
    // ================================================================
    // Pass 1: Accumulate body — only render the FINAL accumulated body
    // This prevents z-fighting when Extrude→Fillet→Chamfer stack up
    // ================================================================
    TopoDS_Shape lastBody;
    int lastBodyFeatureId = -1;
    
    for (const auto& feature : features) {
        if (!feature || !feature->isVisible()) continue;
        CADEngine::FeatureType type = feature->getType();
        
        if (type == CADEngine::FeatureType::Extrude) {
            auto extrude = std::dynamic_pointer_cast<CADEngine::ExtrudeFeature>(feature);
            if (extrude && extrude->isValid()) {
                lastBody = extrude->getResultShape();
                lastBodyFeatureId = feature->getId();
            }
        }
        else if (type == CADEngine::FeatureType::Revolve) {
            auto revolve = std::dynamic_pointer_cast<CADEngine::RevolveFeature>(feature);
            if (revolve && revolve->isValid()) {
                lastBody = revolve->getResultShape();
                lastBodyFeatureId = feature->getId();
            }
        }
        else if (type == CADEngine::FeatureType::Fillet) {
            auto fillet = std::dynamic_pointer_cast<CADEngine::Fillet3DFeature>(feature);
            if (fillet && fillet->isValid()) {
                lastBody = fillet->getResultShape();
                lastBodyFeatureId = feature->getId();
            }
        }
        else if (type == CADEngine::FeatureType::Chamfer) {
            auto chamfer = std::dynamic_pointer_cast<CADEngine::Chamfer3DFeature>(feature);
            if (chamfer && chamfer->isValid()) {
                lastBody = chamfer->getResultShape();
                lastBodyFeatureId = feature->getId();
            }
        }
        // New feature types — all use getResultShape pattern
        else if (feature->getTypeName() == "Sweep") {
            auto sweep = std::dynamic_pointer_cast<CADEngine::SweepFeature>(feature);
            if (sweep && sweep->isValid()) {
                lastBody = sweep->getResultShape();
                lastBodyFeatureId = feature->getId();
            }
        }
        else if (feature->getTypeName() == "Loft") {
            auto loft = std::dynamic_pointer_cast<CADEngine::LoftFeature>(feature);
            if (loft && loft->isValid()) {
                lastBody = loft->getResultShape();
                lastBodyFeatureId = feature->getId();
            }
        }
        else if (type == CADEngine::FeatureType::LinearPattern) {
            auto lp = std::dynamic_pointer_cast<CADEngine::LinearPatternFeature>(feature);
            if (lp && lp->isValid()) {
                lastBody = lp->getResultShape();
                lastBodyFeatureId = feature->getId();
            }
        }
        else if (type == CADEngine::FeatureType::CircularPattern) {
            auto cp = std::dynamic_pointer_cast<CADEngine::CircularPatternFeature>(feature);
            if (cp && cp->isValid()) {
                lastBody = cp->getResultShape();
                lastBodyFeatureId = feature->getId();
            }
        }
        else if (feature->getTypeName() == "Thread") {
            auto thr = std::dynamic_pointer_cast<CADEngine::ThreadFeature>(feature);
            if (thr && thr->isValid()) {
                lastBody = thr->getResultShape();
                lastBodyFeatureId = feature->getId();
            }
        }
        else if (feature->getTypeName() == "Shell") {
            auto shell = std::dynamic_pointer_cast<CADEngine::ShellFeature>(feature);
            if (shell && shell->isValid()) {
                lastBody = shell->getResultShape();
                lastBodyFeatureId = feature->getId();
            }
        }
        else if (feature->getTypeName() == "PushPull") {
            auto pp = std::dynamic_pointer_cast<CADEngine::PushPullFeature>(feature);
            if (pp && pp->isValid()) {
                lastBody = pp->getResultShape();
                lastBodyFeatureId = feature->getId();
            }
        }
        else if (type == CADEngine::FeatureType::Box || type == CADEngine::FeatureType::Cylinder) {
            if (feature->isValid()) {
                lastBody = feature->getShape();
                lastBodyFeatureId = feature->getId();
            }
        }
    }
    
    // ================================================================
    // Pass 2: Render sketches (only unused ones) and the final body
    // ================================================================
    for (const auto& feature : features) {
        if (!feature || !feature->isVisible()) continue;
        CADEngine::FeatureType type = feature->getType();
        
        if (type == CADEngine::FeatureType::Sketch) {
            auto sketch = std::dynamic_pointer_cast<CADEngine::SketchFeature>(feature);
            if (sketch && sketch->getEntityCount() > 0) {
                // Don't render sketch if used by an extrude/revolve
                bool hasChild = false;
                for (const auto& f2 : features) {
                    if (!f2) continue;
                    auto ext = std::dynamic_pointer_cast<CADEngine::ExtrudeFeature>(f2);
                    if (ext && ext->getSketchFeature() == sketch) { hasChild = true; break; }
                    auto rev = std::dynamic_pointer_cast<CADEngine::RevolveFeature>(f2);
                    if (rev && rev->getSketchFeature() == sketch) { hasChild = true; break; }
                }
                if (!hasChild) {
                    glDisable(GL_LIGHTING);
                    renderSketchIn3D(sketch);
                    
                    // Render closed profiles with semi-transparent fill
                    auto sketch2D = sketch->getSketch2D();
                    if (sketch2D) {
                        auto profiles = sketch2D->detectClosedProfiles();
                        if (!profiles.empty()) {
                            glEnable(GL_BLEND);
                            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                            glDepthFunc(GL_LEQUAL);
                            
                            // Profile colors (alternate)
                            float colors[][4] = {
                                {0.2f, 0.5f, 0.9f, 0.2f},   // Blue
                                {0.2f, 0.9f, 0.5f, 0.2f},   // Green
                                {0.9f, 0.5f, 0.2f, 0.2f},   // Orange
                                {0.7f, 0.2f, 0.9f, 0.2f},   // Purple
                            };
                            
                            for (size_t pi = 0; pi < profiles.size(); pi++) {
                                const auto& prof = profiles[pi];
                                if (prof.face.IsNull()) continue;
                                
                                try {
                                    BRepMesh_IncrementalMesh mesher(prof.face, 0.1);
                                    mesher.Perform();
                                    
                                    TopLoc_Location loc;
                                    Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(prof.face, loc);
                                    if (!tri.IsNull()) {
                                        int ci = pi % 4;
                                        glColor4f(colors[ci][0], colors[ci][1], colors[ci][2], colors[ci][3]);
                                        
                                        gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : loc.Transformation();
                                        bool hasTrsf = !loc.IsIdentity();
                                        
                                        glBegin(GL_TRIANGLES);
                                        for (int it = 1; it <= tri->NbTriangles(); it++) {
                                            int n1, n2, n3;
                                            tri->Triangle(it).Get(n1, n2, n3);
                                            gp_Pnt p1 = tri->Node(n1), p2 = tri->Node(n2), p3 = tri->Node(n3);
                                            if (hasTrsf) { p1.Transform(trsf); p2.Transform(trsf); p3.Transform(trsf); }
                                            glVertex3d(p1.X(), p1.Y(), p1.Z());
                                            glVertex3d(p2.X(), p2.Y(), p2.Z());
                                            glVertex3d(p3.X(), p3.Y(), p3.Z());
                                        }
                                        glEnd();
                                    }
                                } catch (...) {}
                            }
                            
                            glDepthFunc(GL_LESS);
                            glDisable(GL_BLEND);
                        }
                    }
                    
                    setupLighting();
                }
            }
        }
    }
    
    // Render only the final accumulated body (no z-fighting!)
    if (!lastBody.IsNull() && lastBodyFeatureId >= 0) {
        renderSolidShape(lastBody, lastBodyFeatureId, 0.6f, 0.65f, 0.72f);
    }
    
    // Update currentBody reference — clear if document has no 3D body
    if (!lastBody.IsNull()) {
        m_currentBody = lastBody;
    } else {
        m_currentBody = TopoDS_Shape();
        m_hoveredFace = TopoDS_Face();
        m_tessCache.clear();
    }
    
    // ================================================================
    // Pattern preview — fantômes semi-transparents (Fusion 360 style)
    // ================================================================
    if (!m_patternPreviewShapes.empty()) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);
        
        for (const auto& previewShape : m_patternPreviewShapes) {
            if (previewShape.IsNull()) continue;
            
            // Faces semi-transparentes
            if (m_patternPreviewIsCut)
                glColor4f(1.0f, 0.3f, 0.2f, 0.25f);   // Rouge pour Cut
            else
                glColor4f(0.2f, 0.6f, 1.0f, 0.25f);    // Bleu pour Join
            
            TopExp_Explorer faceExp(previewShape, TopAbs_FACE);
            for (; faceExp.More(); faceExp.Next()) {
                TopoDS_Face face = TopoDS::Face(faceExp.Current());
                TopLoc_Location loc;
                Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
                if (tri.IsNull()) continue;
                
                gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : loc.Transformation();
                bool hasTrsf = !loc.IsIdentity();
                
                glBegin(GL_TRIANGLES);
                for (int i = 1; i <= tri->NbTriangles(); i++) {
                    int n1, n2, n3;
                    tri->Triangle(i).Get(n1, n2, n3);
                    gp_Pnt p1 = tri->Node(n1), p2 = tri->Node(n2), p3 = tri->Node(n3);
                    if (hasTrsf) { p1.Transform(trsf); p2.Transform(trsf); p3.Transform(trsf); }
                    glVertex3d(p1.X(), p1.Y(), p1.Z());
                    glVertex3d(p2.X(), p2.Y(), p2.Z());
                    glVertex3d(p3.X(), p3.Y(), p3.Z());
                }
                glEnd();
            }
            
            // Arêtes wireframe
            if (m_patternPreviewIsCut)
                glColor4f(1.0f, 0.4f, 0.3f, 0.6f);
            else
                glColor4f(0.3f, 0.7f, 1.0f, 0.6f);
            glLineWidth(1.5f);
            
            TopExp_Explorer edgeExp(previewShape, TopAbs_EDGE);
            for (; edgeExp.More(); edgeExp.Next()) {
                try {
                    BRepAdaptor_Curve curve(TopoDS::Edge(edgeExp.Current()));
                    GCPnts_TangentialDeflection discretizer(curve, 0.5, 0.1);
                    glBegin(GL_LINE_STRIP);
                    for (int i = 1; i <= discretizer.NbPoints(); i++) {
                        gp_Pnt p = discretizer.Value(i);
                        glVertex3d(p.X(), p.Y(), p.Z());
                    }
                    glEnd();
                } catch (...) {}
            }
        }
        
        glDepthFunc(GL_LESS);
        glLineWidth(1.0f);
        glDisable(GL_BLEND);
        setupLighting();
    }
    
    // ================================================================
    // Render face highlight on hover (toujours actif en 3D)
    // ================================================================
    if (!m_hoveredFace.IsNull()) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        // Couleur selon le mode : bleu en sélection active, vert subtil sinon
        if (m_faceSelectionMode)
            glColor4f(0.2f, 0.6f, 1.0f, 0.35f);   // Bleu — mode sélection
        else
            glColor4f(0.2f, 0.8f, 0.3f, 0.20f);    // Vert subtil — survol normal
        
        glDepthFunc(GL_LEQUAL);
        
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(m_hoveredFace, loc);
        if (!tri.IsNull()) {
            gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : loc.Transformation();
            bool hasTrsf = !loc.IsIdentity();
            
            glBegin(GL_TRIANGLES);
            for (int i = 1; i <= tri->NbTriangles(); i++) {
                int n1, n2, n3;
                tri->Triangle(i).Get(n1, n2, n3);
                gp_Pnt p1 = tri->Node(n1), p2 = tri->Node(n2), p3 = tri->Node(n3);
                if (hasTrsf) { p1.Transform(trsf); p2.Transform(trsf); p3.Transform(trsf); }
                glVertex3d(p1.X(), p1.Y(), p1.Z());
                glVertex3d(p2.X(), p2.Y(), p2.Z());
                glVertex3d(p3.X(), p3.Y(), p3.Z());
            }
            glEnd();
        }
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);
        setupLighting();
    }
    
    // Render hovered edge highlight
    if ((m_selectingEdges || m_faceSelectionMode) && !m_hoveredEdge.IsNull()) {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.8f, 0.0f);  // Yellow highlight
        glLineWidth(4.0f);
        try {
            BRepAdaptor_Curve curve(m_hoveredEdge);
            GCPnts_TangentialDeflection discretizer(curve, 0.5, 0.1);
            glBegin(GL_LINE_STRIP);
            for (int i = 1; i <= discretizer.NbPoints(); i++) {
                gp_Pnt p = discretizer.Value(i);
                glVertex3d(p.X(), p.Y(), p.Z());
            }
            glEnd();
        } catch (...) {}
        glLineWidth(1.0f);
        setupLighting();
    }
    
    // Render selected edges highlight (for fillet/chamfer)
    if (!m_selectedEdges3D.empty()) {
        glDisable(GL_LIGHTING);
        glColor3f(0.0f, 1.0f, 0.4f);  // Green = selected
        glLineWidth(4.0f);
        for (const auto& edge : m_selectedEdges3D) {
            if (edge.IsNull()) continue;
            try {
                BRepAdaptor_Curve curve(edge);
                GCPnts_TangentialDeflection discretizer(curve, 0.5, 0.1);
                glBegin(GL_LINE_STRIP);
                for (int i = 1; i <= discretizer.NbPoints(); i++) {
                    gp_Pnt p = discretizer.Value(i);
                    glVertex3d(p.X(), p.Y(), p.Z());
                }
                glEnd();
            } catch (...) {}
        }
        glLineWidth(1.0f);
        setupLighting();
    }
    
    // Render hovered edge highlight (yellow) during edge selection
    if (m_selectingEdges && !m_hoveredEdge.IsNull()) {
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 0.0f);  // Yellow = hover
        glLineWidth(5.0f);
        try {
            BRepAdaptor_Curve curve(m_hoveredEdge);
            GCPnts_TangentialDeflection discretizer(curve, 0.5, 0.1);
            glBegin(GL_LINE_STRIP);
            for (int i = 1; i <= discretizer.NbPoints(); i++) {
                gp_Pnt p = discretizer.Value(i);
                glVertex3d(p.X(), p.Y(), p.Z());
            }
            glEnd();
        } catch (...) {}
        glLineWidth(1.0f);
        setupLighting();
    }
    // Render selected face highlight
    if (m_hasSelectedFace && !m_selectedFace.IsNull()) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 1.0f, 0.4f, 0.3f);  // Green transparent = confirmed selection
        glDepthFunc(GL_LEQUAL);
        
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(m_selectedFace, loc);
        if (!tri.IsNull()) {
            gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : loc.Transformation();
            bool hasTrsf = !loc.IsIdentity();
            
            glBegin(GL_TRIANGLES);
            for (int i = 1; i <= tri->NbTriangles(); i++) {
                int n1, n2, n3;
                tri->Triangle(i).Get(n1, n2, n3);
                gp_Pnt p1 = tri->Node(n1), p2 = tri->Node(n2), p3 = tri->Node(n3);
                if (hasTrsf) { p1.Transform(trsf); p2.Transform(trsf); p3.Transform(trsf); }
                glVertex3d(p1.X(), p1.Y(), p1.Z());
                glVertex3d(p2.X(), p2.Y(), p2.Z());
                glVertex3d(p3.X(), p3.Y(), p3.Z());
            }
            glEnd();
        }
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);
        setupLighting();
    }
    
    // ================================================================
    // Render profile regions in profile selection mode (Fusion 360)
    // ================================================================
    if (m_profileSelectionMode && !m_profileRegions.empty()) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);
        
        for (int ri = 0; ri < (int)m_profileRegions.size(); ri++) {
            const auto& region = m_profileRegions[ri];
            if (region.face.IsNull()) continue;
            
            TopLoc_Location loc;
            Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(region.face, loc);
            if (tri.IsNull()) continue;
            
            gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : loc.Transformation();
            bool hasTrsf = !loc.IsIdentity();
            
            // Selected = green, Hovered = bright blue, others = dim
            bool isSelected = m_selectedProfileIndices.count(ri) > 0;
            if (isSelected)
                glColor4f(0.2f, 0.9f, 0.3f, 0.45f);   // Green = selected
            else if (ri == m_hoveredRegionIndex)
                glColor4f(0.2f, 0.6f, 1.0f, 0.45f);  // Bright blue = hovered
            else
                glColor4f(0.3f, 0.5f, 0.7f, 0.15f);   // Dim blue = available
            
            glBegin(GL_TRIANGLES);
            for (int it = 1; it <= tri->NbTriangles(); it++) {
                int n1, n2, n3;
                tri->Triangle(it).Get(n1, n2, n3);
                gp_Pnt p1 = tri->Node(n1), p2 = tri->Node(n2), p3 = tri->Node(n3);
                if (hasTrsf) { p1.Transform(trsf); p2.Transform(trsf); p3.Transform(trsf); }
                glVertex3d(p1.X(), p1.Y(), p1.Z());
                glVertex3d(p2.X(), p2.Y(), p2.Z());
                glVertex3d(p3.X(), p3.Y(), p3.Z());
            }
            glEnd();
            
            // Draw outline for hovered or selected region
            if (ri == m_hoveredRegionIndex || isSelected) {
                if (isSelected)
                    glColor4f(0.2f, 1.0f, 0.4f, 0.9f);  // Green outline
                else
                    glColor4f(0.2f, 0.8f, 1.0f, 0.9f);  // Blue outline
                glLineWidth(3.0f);
                TopExp_Explorer edgeExp(region.face, TopAbs_EDGE);
                for (; edgeExp.More(); edgeExp.Next()) {
                    TopoDS_Edge edge = TopoDS::Edge(edgeExp.Current());
                    try {
                        BRepAdaptor_Curve curve(edge);
                        GCPnts_TangentialDeflection disc(curve, 0.1, 0.05);
                        glBegin(GL_LINE_STRIP);
                        for (int i = 1; i <= disc.NbPoints(); i++) {
                            gp_Pnt p = disc.Value(i);
                            glVertex3d(p.X(), p.Y(), p.Z());
                        }
                        glEnd();
                    } catch (...) {}
                }
                glLineWidth(1.0f);
            }
        }
        
        glDepthFunc(GL_LESS);
        glDisable(GL_BLEND);
        setupLighting();
    }
}

void Viewport3D::renderSketchIn3D(std::shared_ptr<CADEngine::SketchFeature> sketch) {
    if (!sketch) {
        return;
    }
    
    auto sketch2D = sketch->getSketch2D();
    if (!sketch2D) {
        return;
    }
    
    glPushMatrix();
    
    // Désactiver depth test pour que le sketch soit toujours visible
    glDisable(GL_DEPTH_TEST);
    
    // Transformation selon le plan du sketch
    // Les entités 2D sont en coordonnées locales (u,v) → on les transforme en 3D via le plan
    gp_Pln plane = sketch2D->getPlane();
    gp_Pnt origin = plane.Location();
    gp_Dir xDir = plane.XAxis().Direction();
    gp_Dir yDir = plane.YAxis().Direction();
    gp_Dir nDir = plane.Axis().Direction();
    
    {
        // Matrice OpenGL column-major : transforme (x,y,0) local → 3D
        double m[16] = {
            xDir.X(), xDir.Y(), xDir.Z(), 0.0,
            yDir.X(), yDir.Y(), yDir.Z(), 0.0,
            nDir.X(), nDir.Y(), nDir.Z(), 0.0,
            origin.X(), origin.Y(), origin.Z(), 1.0
        };
        glMultMatrixd(m);
    }
    
    // Couleur du sketch (gris clair)
    glColor3f(0.7f, 0.7f, 0.7f);
    glLineWidth(2.0f);
    
    // Dessiner toutes les entités
    auto entities = sketch2D->getEntities();
    
    for (const auto& entity : entities) {
        if (!entity) continue;
        // Skip construction geometry in 3D view
        if (entity->isConstruction()) continue;
        
        
        if (entity->getType() == CADEngine::SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
            if (line) {
                gp_Pnt2d p1 = line->getStart();
                gp_Pnt2d p2 = line->getEnd();
                
                
                glBegin(GL_LINES);
                glVertex3f(p1.X(), p1.Y(), 0.0f);
                glVertex3f(p2.X(), p2.Y(), 0.0f);
                glEnd();
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
            if (circle) {
                gp_Pnt2d center = circle->getCenter();
                double radius = circle->getRadius();
                
                
                glBegin(GL_LINE_LOOP);
                for (int j = 0; j < 64; j++) {
                    double angle = 2.0 * M_PI * j / 64.0;
                    double x = center.X() + radius * cos(angle);
                    double y = center.Y() + radius * sin(angle);
                    glVertex3f(x, y, 0.0f);
                }
                glEnd();
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
            if (rect) {
                auto corners = rect->getKeyPoints();
                
                
                if (corners.size() >= 4) {
                    glBegin(GL_LINE_LOOP);
                    for (const auto& corner : corners) {
                        glVertex3f(corner.X(), corner.Y(), 0.0f);
                    }
                    glEnd();
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Arc) {
            auto arc = std::dynamic_pointer_cast<CADEngine::SketchArc>(entity);
            if (arc) {
                gp_Pnt2d p1 = arc->getStart();
                gp_Pnt2d p2 = arc->getMid();
                gp_Pnt2d p3 = arc->getEnd();
                
                if (arc->isBezier()) {
                    // Bézier quadratique : p2 est point de contrôle
                    int segs = 32;
                    glBegin(GL_LINE_STRIP);
                    for (int i = 0; i <= segs; ++i) {
                        double t = (double)i / segs;
                        double u = 1.0 - t;
                        double bx = u*u*p1.X() + 2*u*t*p2.X() + t*t*p3.X();
                        double by = u*u*p1.Y() + 2*u*t*p2.Y() + t*t*p3.Y();
                        glVertex3f(bx, by, 0.0f);
                    }
                    glEnd();
                } else {
                    // Arc circulaire via 3 points
                    double ax = p2.X()-p1.X(), ay = p2.Y()-p1.Y();
                    double bx = p3.X()-p2.X(), by = p3.Y()-p2.Y();
                    double det = 2.0*(ax*by - ay*bx);
                    
                    if (std::abs(det) > 1e-6) {
                        double aMag = ax*ax+ay*ay, bMag = bx*bx+by*by;
                        double cx = p1.X() + (aMag*by - bMag*ay)/det;
                        double cy = p1.Y() + (bMag*ax - aMag*bx)/det;
                        double r = std::sqrt((p1.X()-cx)*(p1.X()-cx)+(p1.Y()-cy)*(p1.Y()-cy));
                        
                        double a1 = std::atan2(p1.Y()-cy, p1.X()-cx);
                        double a2 = std::atan2(p2.Y()-cy, p2.X()-cx);
                        double a3 = std::atan2(p3.Y()-cy, p3.X()-cx);
                        
                        double sw13 = a3-a1; while(sw13<-M_PI) sw13+=2*M_PI; while(sw13>M_PI) sw13-=2*M_PI;
                        double sw12 = a2-a1; while(sw12<-M_PI) sw12+=2*M_PI; while(sw12>M_PI) sw12-=2*M_PI;
                        double totalSw;
                        if ((sw13>0 && sw12>0 && sw12<sw13)||(sw13<0 && sw12<0 && sw12>sw13))
                            totalSw = sw13;
                        else
                            totalSw = (sw13>0) ? sw13-2*M_PI : sw13+2*M_PI;
                        
                        int segs = std::max(16, (int)(std::abs(totalSw)*32.0/M_PI));
                        glBegin(GL_LINE_STRIP);
                        for (int i=0; i<=segs; ++i) {
                            double a = a1 + totalSw*i/segs;
                            glVertex3f(cx+r*std::cos(a), cy+r*std::sin(a), 0.0f);
                        }
                        glEnd();
                    } else {
                        glBegin(GL_LINES);
                        glVertex3f(p1.X(), p1.Y(), 0.0f);
                        glVertex3f(p3.X(), p3.Y(), 0.0f);
                        glEnd();
                    }
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
            if (polyline) {
                auto points = polyline->getPoints();
                const auto& filletArcs3D = polyline->getFilletArcs();
                if (filletArcs3D.empty()) {
                    glBegin(GL_LINE_STRIP);
                    for (const auto& pt : points)
                        glVertex3f(pt.X(), pt.Y(), 0.0f);
                    glEnd();
                } else {
                    std::map<int, const CADEngine::SketchPolyline::FilletArc*> arcMap3D;
                    for (const auto& fa : filletArcs3D) arcMap3D[fa.startIdx] = &fa;
                    int np3 = (int)points.size();
                    int idx3 = 0;
                    while (idx3 < np3) {
                        auto it = arcMap3D.find(idx3);
                        if (it != arcMap3D.end()) {
                            const auto& fa = *it->second;
                            int endIdx3 = (fa.endIdx < np3) ? fa.endIdx : np3 - 1;
                            glBegin(GL_LINE_STRIP);
                            glVertex3f(points[idx3].X(), points[idx3].Y(), 0.0f);
                            const int tessN3 = 16;
                            for (int k = 1; k < tessN3; k++) {
                                double a = fa.startAngle + (double)k / tessN3 * fa.angleDiff;
                                glVertex3f((float)(fa.cx + fa.radius * std::cos(a)),
                                           (float)(fa.cy + fa.radius * std::sin(a)), 0.0f);
                            }
                            glVertex3f(points[endIdx3].X(), points[endIdx3].Y(), 0.0f);
                            glEnd();
                            idx3 = endIdx3;
                        } else {
                            if (idx3 + 1 < np3) {
                                glBegin(GL_LINE_STRIP);
                                glVertex3f(points[idx3].X(), points[idx3].Y(), 0.0f);
                                glVertex3f(points[idx3+1].X(), points[idx3+1].Y(), 0.0f);
                                glEnd();
                            }
                            idx3++;
                        }
                    }
                }
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Ellipse) {
            auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
            if (ellipse) {
                gp_Pnt2d center = ellipse->getCenter();
                double a = ellipse->getMajorRadius();
                double b = ellipse->getMinorRadius();
                double rot = ellipse->getRotation();
                double cosR = std::cos(rot), sinR = std::sin(rot);
                
                glBegin(GL_LINE_LOOP);
                for (int i = 0; i < 64; ++i) {
                    double t = 2.0 * M_PI * i / 64;
                    double lx = a * std::cos(t);
                    double ly = b * std::sin(t);
                    glVertex3f(center.X() + lx*cosR - ly*sinR,
                               center.Y() + lx*sinR + ly*cosR, 0.0f);
                }
                glEnd();
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
            auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
            if (polygon) {
                auto verts = polygon->getVertices();
                glBegin(GL_LINE_LOOP);
                for (const auto& v : verts) {
                    glVertex3f(v.X(), v.Y(), 0.0f);
                }
                glEnd();
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
            auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
            if (oblong) {
                gp_Pnt2d center = oblong->getCenter();
                double L = oblong->getLength(), W = oblong->getWidth();
                double rot = oblong->getRotation(), r = W/2.0;
                double halfS = (L - W) / 2.0;
                double cosR = std::cos(rot), sinR = std::sin(rot);
                double axX = cosR, axY = sinR, prX = -sinR, prY = cosR;
                
                float p1x = center.X()-halfS*axX+r*prX, p1y = center.Y()-halfS*axY+r*prY;
                float p2x = center.X()+halfS*axX+r*prX, p2y = center.Y()+halfS*axY+r*prY;
                float p3x = center.X()+halfS*axX-r*prX, p3y = center.Y()+halfS*axY-r*prY;
                float p4x = center.X()-halfS*axX-r*prX, p4y = center.Y()-halfS*axY-r*prY;
                
                glBegin(GL_LINES); glVertex3f(p1x,p1y,0); glVertex3f(p2x,p2y,0); glEnd();
                double c2x = center.X()+halfS*axX, c2y = center.Y()+halfS*axY;
                double sa = std::atan2(prY, prX);
                glBegin(GL_LINE_STRIP);
                for (int i=0; i<=32; ++i) { double a=sa-M_PI*i/32.0; glVertex3f(c2x+r*cos(a),c2y+r*sin(a),0); }
                glEnd();
                glBegin(GL_LINES); glVertex3f(p3x,p3y,0); glVertex3f(p4x,p4y,0); glEnd();
                double c1x = center.X()-halfS*axX, c1y = center.Y()-halfS*axY;
                glBegin(GL_LINE_STRIP);
                for (int i=0; i<=32; ++i) { double a=sa+M_PI-M_PI*i/32.0; glVertex3f(c1x+r*cos(a),c1y+r*sin(a),0); }
                glEnd();
            }
        }
    }
    
    // NE PAS rendre les dimensions ici - elles sont rendues pendant l'édition seulement
    // (Si on veut les afficher après fermeture, décommenter ci-dessous)
    /*
    auto dimensions = sketch2D->getDimensions();
    if (!dimensions.empty()) {
        renderDimensions(dimensions);
    }
    */
    
    
    glEnable(GL_DEPTH_TEST);  // Réactiver
    glPopMatrix();
}

// ============================================================================
// Keyboard Events  
// ============================================================================

void Viewport3D::keyPressEvent(QKeyEvent* event) {
    if (m_mode == ViewMode::SKETCH_2D) {
        // Touche Entrée ou Espace = finir Polyline
        if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space)
            && m_currentTool == SketchTool::Polyline && m_tempPoints.size() >= 2) {
            qDebug() << "[Key] Entrée - Finish Polyline avec" << m_tempPoints.size() << "points";
            handlePolylineToolFinish();
            m_tempPoints.clear();
            update();
        }
        // Polygon: +/- pour changer le nombre de côtés
        else if (m_currentTool == SketchTool::Polygon && 
                 (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal ||
                  event->key() == Qt::Key_Up)) {
            m_polygonSides = std::min(32, m_polygonSides + 1);
            emit statusMessage(QString("Polygone: %1 côtés").arg(m_polygonSides));
            update();
        }
        else if (m_currentTool == SketchTool::Polygon && 
                 (event->key() == Qt::Key_Minus || event->key() == Qt::Key_Down)) {
            m_polygonSides = std::max(3, m_polygonSides - 1);
            emit statusMessage(QString("Polygone: %1 côtés").arg(m_polygonSides));
            update();
        }
        // Polygon: chiffres directs 3-9
        else if (m_currentTool == SketchTool::Polygon && 
                 event->key() >= Qt::Key_3 && event->key() <= Qt::Key_9) {
            m_polygonSides = event->key() - Qt::Key_0;
            emit statusMessage(QString("Polygone: %1 côtés").arg(m_polygonSides));
            update();
        }
        // Touche Echap = annuler et désactiver outil
        else if (event->key() == Qt::Key_Escape) {
            qDebug() << "[Key] Echap - Annuler et désactiver outil";
            m_tempPoints.clear();
            m_hasHoverPoint = false;
            m_hasDimensionFirstPoint = false;
            m_currentTool = SketchTool::None;
            emit toolChanged(SketchTool::None);
            emit statusMessage("Outil désactivé - Mode sélection");
            update();
        }
    }
    // 3D mode key handling
    else if (m_mode == ViewMode::NORMAL_3D) {
        if (event->key() == Qt::Key_Escape) {
            if (m_profileSelectionMode) {
                stopProfileSelection();
                emit profileSelectionCancelled();
                emit statusMessage("Sélection profil annulée");
            }
            else if (m_faceSelectionMode) {
                stopFaceSelection();
                emit faceSelectionCancelled();
                emit statusMessage("Sélection face annulée");
            }
            else if (m_selectingEdges) {
                stopEdgeSelection();
                m_selectedEdges3D.clear();
                emit edgeSelectionCancelled();
                emit statusMessage("Sélection arêtes annulée");
            }
            update();
        }
        // Enter = confirm edge selection
        else if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
                 && m_selectingEdges && !m_selectedEdges3D.empty()) {
            stopEdgeSelection();
            emit edgeSelectionConfirmed();
        }
        // Enter = confirm multi-profile selection
        else if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
                 && m_profileSelectionMode && !m_selectedProfileIndices.empty()) {
            confirmProfileSelection();
        }
    }
    
    // ── Toggle snap grille (G) — fonctionne dans tous les modes ──
    if (event->key() == Qt::Key_G && !(event->modifiers() & Qt::ControlModifier)) {
        m_gridSnapEnabled = !m_gridSnapEnabled;
        double gridSize = CADEngine::ViewportScaling::getSnapGridSize(m_sketch2DZoom);
        if (m_gridSnapEnabled) {
            emit statusMessage(QString("Ancrage grille : ON (pas = %1 mm)")
                .arg(gridSize, 0, 'f', 2));
        } else {
            emit statusMessage("Ancrage grille : OFF (précision libre 0.1mm)");
        }
        update();
    }
    
    QOpenGLWidget::keyPressEvent(event);
}

// ============================================================================
// Helper - Calculer arc circulaire à partir de 3 points
// ============================================================================

ArcParams calculateArc(const gp_Pnt2d& p1, const gp_Pnt2d& p2, const gp_Pnt2d& p3) {
    ArcParams result;
    result.isValid = false;
    
    // Vérifier que les points ne sont pas alignés
    double dx1 = p2.X() - p1.X();
    double dy1 = p2.Y() - p1.Y();
    double dx2 = p3.X() - p2.X();
    double dy2 = p3.Y() - p2.Y();
    
    double cross = dx1 * dy2 - dy1 * dx2;
    if (std::abs(cross) < 1e-6) {
        return result;  // Points alignés
    }
    
    // Calculer centre de l'arc (intersection des médiatrices)
    double ma = dy1 / dx1;
    double mb = dy2 / dx2;
    
    if (std::abs(dx1) < 1e-6) {
        // Ligne 1 verticale
        double cx = (p1.X() + p2.X()) / 2.0;
        double midx2 = (p2.X() + p3.X()) / 2.0;
        double midy2 = (p2.Y() + p3.Y()) / 2.0;
        double cy = midy2 - mb * (midx2 - cx);
        result.center = gp_Pnt2d(cx, cy);
    } else if (std::abs(dx2) < 1e-6) {
        // Ligne 2 verticale
        double cx = (p2.X() + p3.X()) / 2.0;
        double midx1 = (p1.X() + p2.X()) / 2.0;
        double midy1 = (p1.Y() + p2.Y()) / 2.0;
        double cy = midy1 - ma * (midx1 - cx);
        result.center = gp_Pnt2d(cx, cy);
    } else {
        // Cas général
        double midx1 = (p1.X() + p2.X()) / 2.0;
        double midy1 = (p1.Y() + p2.Y()) / 2.0;
        double midx2 = (p2.X() + p3.X()) / 2.0;
        double midy2 = (p2.Y() + p3.Y()) / 2.0;
        
        double cx = (midy2 - midy1 + ma * midx1 - mb * midx2) / (ma - mb);
        double cy = midy1 - ma * (midx1 - cx);
        
        result.center = gp_Pnt2d(cx, cy);
    }
    
    // Calculer rayon
    result.radius = result.center.Distance(p1);
    
    // Si le rayon est très grand par rapport à la distance p1-p3,
    // l'arc est trop plat et ressemble à un demi-cercle
    // → Limiter ou invalider
    double chordLength = p1.Distance(p3);
    if (result.radius > chordLength * 20) {
        // Arc trop plat - retourner invalide pour dessiner une ligne droite
        result.isValid = false;
        return result;
    }
    
    // Calculer angles
    result.startAngle = std::atan2(p1.Y() - result.center.Y(), p1.X() - result.center.X());
    double midAngle = std::atan2(p2.Y() - result.center.Y(), p2.X() - result.center.X());
    result.endAngle = std::atan2(p3.Y() - result.center.Y(), p3.X() - result.center.X());
    
    // Utiliser produit vectoriel pour déterminer le sens
    // Vecteur start->mid et start->end depuis le centre
    double v1x = p2.X() - p1.X();
    double v1y = p2.Y() - p1.Y();
    double v2x = p3.X() - p1.X();
    double v2y = p3.Y() - p1.Y();
    
    // Produit vectoriel (positif = antihoraire, négatif = horaire)
    double crossProduct = v1x * v2y - v1y * v2x;
    
    // Calculer l'angle à parcourir
    double angleSpan = result.endAngle - result.startAngle;
    
    // Normaliser dans [-π, π]
    while (angleSpan > M_PI) angleSpan -= 2 * M_PI;
    while (angleSpan < -M_PI) angleSpan += 2 * M_PI;
    
    // Vérifier que le sens correspond au produit vectoriel
    if (crossProduct > 0) {
        // Sens antihoraire demandé
        if (angleSpan < 0) {
            result.endAngle += 2 * M_PI;
        }
    } else {
        // Sens horaire demandé
        if (angleSpan > 0) {
            result.endAngle -= 2 * M_PI;
        }
    }
    
    result.isValid = true;
    return result;
}

// ============================================================================
// 3D Solid Rendering - Lighting
// ============================================================================

void Viewport3D::setupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    // Light 0 - key light (above-right-front)
    GLfloat light0_pos[] = { 300.0f, 500.0f, 400.0f, 0.0f };  // Directional
    GLfloat light0_amb[] = { 0.15f, 0.15f, 0.17f, 1.0f };
    GLfloat light0_dif[] = { 0.75f, 0.73f, 0.70f, 1.0f };
    GLfloat light0_spec[] = { 0.4f, 0.4f, 0.4f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_dif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_spec);
    
    // Light 1 - fill light (below-left)
    GLfloat light1_pos[] = { -200.0f, -300.0f, 200.0f, 0.0f };
    GLfloat light1_amb[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat light1_dif[] = { 0.25f, 0.27f, 0.30f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, light1_pos);
    glLightfv(GL_LIGHT1, GL_AMBIENT, light1_amb);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_dif);
    
    // Material specular
    GLfloat matSpec[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat matShin[] = { 40.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShin);
}

// ============================================================================
// 3D Solid Rendering - Tessellation
// ============================================================================

void Viewport3D::tessellateShape(const TopoDS_Shape& shape, TessCache& cache) {
    cache.triangles.clear();
    cache.valid = false;
    
    if (shape.IsNull()) return;
    
    // Compute mesh with better quality
    try {
        // deflection=0.1 (finer), relative=false, angular=0.3, parallel=true
        BRepMesh_IncrementalMesh mesher(shape, 0.1, Standard_False, 0.3, Standard_True);
        mesher.Perform();
    } catch (...) {
        return;
    }
    
    // Extract triangles from all faces
    TopExp_Explorer explorer(shape, TopAbs_FACE);
    for (; explorer.More(); explorer.Next()) {
        TopoDS_Face face = TopoDS::Face(explorer.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) continue;
        
        gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : loc.Transformation();
        bool hasTransform = !loc.IsIdentity();
        
        int nbTri = tri->NbTriangles();
        bool reversed = (face.Orientation() == TopAbs_REVERSED);
        
        // Check if per-vertex normals are available
        bool hasNormals = tri->HasNormals();
        
        for (int iTri = 1; iTri <= nbTri; iTri++) {
            int n1, n2, n3;
            tri->Triangle(iTri).Get(n1, n2, n3);
            
            // Swap winding for reversed faces — this fixes the normal direction
            if (reversed) std::swap(n2, n3);
            
            gp_Pnt p1 = tri->Node(n1);
            gp_Pnt p2 = tri->Node(n2);
            gp_Pnt p3 = tri->Node(n3);
            
            if (hasTransform) {
                p1.Transform(trsf);
                p2.Transform(trsf);
                p3.Transform(trsf);
            }
            
            // Compute normals per vertex
            float nx1, ny1, nz1, nx2, ny2, nz2, nx3, ny3, nz3;
            
            if (hasNormals) {
                // Use smooth per-vertex normals from tessellation
                gp_Dir dn1 = tri->Normal(n1);
                gp_Dir dn2 = tri->Normal(n2);
                gp_Dir dn3 = tri->Normal(n3);
                if (reversed) { dn1.Reverse(); dn2.Reverse(); dn3.Reverse(); }
                if (hasTransform) { dn1.Transform(trsf); dn2.Transform(trsf); dn3.Transform(trsf); }
                nx1 = (float)dn1.X(); ny1 = (float)dn1.Y(); nz1 = (float)dn1.Z();
                nx2 = (float)dn2.X(); ny2 = (float)dn2.Y(); nz2 = (float)dn2.Z();
                nx3 = (float)dn3.X(); ny3 = (float)dn3.Y(); nz3 = (float)dn3.Z();
            } else {
                // Flat shading: compute face normal from cross product
                gp_XYZ v1 = p2.XYZ() - p1.XYZ();
                gp_XYZ v2 = p3.XYZ() - p1.XYZ();
                gp_XYZ fn = v1.Crossed(v2);
                double mag = fn.Modulus();
                if (mag < 1e-12) continue;  // Degenerate triangle
                fn.Divide(mag);
                // No extra reversal — winding swap already handles orientation
                nx1 = nx2 = nx3 = (float)fn.X();
                ny1 = ny2 = ny3 = (float)fn.Y();
                nz1 = nz2 = nz3 = (float)fn.Z();
            }
            
            TessVertex tv;
            tv.x = (float)p1.X(); tv.y = (float)p1.Y(); tv.z = (float)p1.Z();
            tv.nx = nx1; tv.ny = ny1; tv.nz = nz1;
            cache.triangles.push_back(tv);
            
            tv.x = (float)p2.X(); tv.y = (float)p2.Y(); tv.z = (float)p2.Z();
            tv.nx = nx2; tv.ny = ny2; tv.nz = nz2;
            cache.triangles.push_back(tv);
            
            tv.x = (float)p3.X(); tv.y = (float)p3.Y(); tv.z = (float)p3.Z();
            tv.nx = nx3; tv.ny = ny3; tv.nz = nz3;
            cache.triangles.push_back(tv);
        }
    }
    
    cache.valid = !cache.triangles.empty();
}

// ============================================================================
// 3D Solid Rendering - Draw shape
// ============================================================================

void Viewport3D::renderSolidShape(const TopoDS_Shape& shape, int featureId,
                                    float r, float g, float b, float alpha) {
    if (shape.IsNull()) return;
    
    // Check cache
    auto it = m_tessCache.find(featureId);
    if (it == m_tessCache.end() || !it->second.valid) {
        TessCache cache;
        tessellateShape(shape, cache);
        m_tessCache[featureId] = std::move(cache);
        it = m_tessCache.find(featureId);
    }
    
    if (!it->second.valid || it->second.triangles.empty()) return;
    
    const auto& tris = it->second.triangles;
    
    // Render solid faces with two-sided lighting
    glEnable(GL_LIGHTING);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glColor4f(r, g, b, alpha);
    
    // Offset faces so edges render on top without z-fighting
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < tris.size(); i++) {
        glNormal3f(tris[i].nx, tris[i].ny, tris[i].nz);
        glVertex3f(tris[i].x, tris[i].y, tris[i].z);
    }
    glEnd();
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    
    // Render edges (wireframe overlay)
    renderSolidEdges(shape);
}

void Viewport3D::renderSolidEdges(const TopoDS_Shape& shape) {
    glDisable(GL_LIGHTING);
    glColor3f(0.12f, 0.12f, 0.15f);
    glLineWidth(1.5f);
    
    TopExp_Explorer explorer(shape, TopAbs_EDGE);
    for (; explorer.More(); explorer.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
        if (edge.IsNull()) continue;
        
        try {
            BRepAdaptor_Curve curve(edge);
            GCPnts_TangentialDeflection discretizer(curve, 0.1, 0.05);
            
            glBegin(GL_LINE_STRIP);
            for (int i = 1; i <= discretizer.NbPoints(); i++) {
                gp_Pnt p = discretizer.Value(i);
                glVertex3d(p.X(), p.Y(), p.Z());
            }
            glEnd();
        } catch (...) {}
    }
    
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

// ============================================================================
// Edge selection mode (for fillet/chamfer)
// ============================================================================

void Viewport3D::startEdgeSelection() {
    m_selectingEdges = true;
    m_selectedEdges3D.clear();
    emit statusMessage("Sélection arêtes : clic pour ajouter, Entrée pour valider");
    update();
}

void Viewport3D::stopEdgeSelection() {
    m_selectingEdges = false;
    update();
}

void Viewport3D::setPatternPreview(const std::vector<TopoDS_Shape>& shapes, bool isCut) {
    m_patternPreviewShapes = shapes;
    m_patternPreviewIsCut = isCut;
    // Tessellate preview shapes
    for (auto& s : m_patternPreviewShapes) {
        if (!s.IsNull()) {
            try {
                BRepMesh_IncrementalMesh mesher(s, 0.2, Standard_False, 0.5, Standard_True);
                mesher.Perform();
            } catch (...) {}
        }
    }
    update();
}

void Viewport3D::clearPatternPreview() {
    m_patternPreviewShapes.clear();
    m_patternPreviewIsCut = false;
    update();
}

void Viewport3D::startFaceSelection() {
    m_faceSelectionMode = true;
    setCursor(Qt::CrossCursor);
    emit statusMessage("Cliquez sur une face plane du solide...");
    update();
}

void Viewport3D::stopFaceSelection() {
    m_faceSelectionMode = false;
    m_hoveredEdge = TopoDS_Edge();
    setCursor(Qt::ArrowCursor);
    update();
}

// ============================================================================
// Profile selection mode (Fusion 360 style — click on a region)
// ============================================================================

void Viewport3D::startProfileSelection(std::shared_ptr<CADEngine::SketchFeature> sketch) {
    m_profileSelectionMode = true;
    m_profileSketch = sketch;
    m_hoveredRegionIndex = -1;
    m_selectedProfileFace = TopoDS_Face();
    m_selectedProfileIndices.clear();
    m_profileRegions.clear();
    
    if (!sketch || !sketch->getSketch2D()) return;
    
    auto regions = sketch->getSketch2D()->buildFaceRegions();
    
    std::cout << "[ProfileSelection] Got " << regions.size() << " regions" << std::endl;
    
    for (size_t i = 0; i < regions.size(); i++) {
        const auto& r = regions[i];
        ProfileRegion pr;
        pr.face = r.face;
        pr.description = r.description;
        
        // Pre-tessellate for ray picking
        bool tessOk = false;
        try {
            BRepMesh_IncrementalMesh mesher(r.face, 0.1, Standard_False, 0.3, Standard_True);
            mesher.Perform();
            tessOk = mesher.IsDone();
        } catch (...) {}
        
        // Verify tessellation has triangles
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(r.face, loc);
        int nTri = tri.IsNull() ? 0 : tri->NbTriangles();
        
        std::cout << "  Region " << i << ": " << r.description 
                  << " tessOk=" << tessOk << " triangles=" << nTri << std::endl;
        
        if (nTri > 0) {
            m_profileRegions.push_back(pr);
        } else {
            std::cout << "    → SKIPPED (no triangles)" << std::endl;
        }
    }
    
    setCursor(Qt::CrossCursor);
    emit statusMessage(QString("Sélectionnez un profil à extruder (%1 région%2 détectée%3)")
        .arg(m_profileRegions.size())
        .arg(m_profileRegions.size() > 1 ? "s" : "")
        .arg(m_profileRegions.size() > 1 ? "s" : ""));
    update();
}

void Viewport3D::stopProfileSelection() {
    m_profileSelectionMode = false;
    m_profileRegions.clear();
    m_selectedProfileIndices.clear();
    m_hoveredRegionIndex = -1;
    m_profileSketch = nullptr;
    setCursor(Qt::ArrowCursor);
    update();
}

void Viewport3D::confirmProfileSelection() {
    if (m_selectedProfileIndices.empty()) return;
    
    if (m_selectedProfileIndices.size() == 1) {
        // Single selection — use original signal
        int idx = *m_selectedProfileIndices.begin();
        if (idx >= 0 && idx < (int)m_profileRegions.size()) {
            m_selectedProfileFace = m_profileRegions[idx].face;
            QString desc = QString::fromStdString(m_profileRegions[idx].description);
            stopProfileSelection();
            emit profileSelected(m_selectedProfileFace, desc);
        }
    } else {
        // Multi selection — collect faces and emit multiProfileSelected
        std::vector<TopoDS_Face> faces;
        QStringList descs;
        for (int idx : m_selectedProfileIndices) {
            if (idx >= 0 && idx < (int)m_profileRegions.size()) {
                faces.push_back(m_profileRegions[idx].face);
                descs << QString::fromStdString(m_profileRegions[idx].description);
            }
        }
        QString desc = descs.join(" + ");
        stopProfileSelection();
        emit multiProfileSelected(faces, desc);
    }
}

int Viewport3D::pickProfileRegionAtScreen(int screenX, int screenY) {
    if (m_profileRegions.empty()) return -1;
    
    gp_Dir rayDir;
    gp_Pnt rayOrigin = screenTo3DRay(screenX, screenY, rayDir);
    
    // Test each region — smallest area first (inner profiles have priority)
    // Regions are already ordered: outer first, inner after, so iterate reverse
    double bestDist = 1e30;
    int bestRegion = -1;
    
    for (int ri = (int)m_profileRegions.size() - 1; ri >= 0; ri--) {
        const auto& region = m_profileRegions[ri];
        if (region.face.IsNull()) continue;
        
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(region.face, loc);
        if (tri.IsNull()) continue;
        
        gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : loc.Transformation();
        bool hasTransform = !loc.IsIdentity();
        
        for (int iTri = 1; iTri <= tri->NbTriangles(); iTri++) {
            int n1, n2, n3;
            tri->Triangle(iTri).Get(n1, n2, n3);
            
            gp_Pnt p1 = tri->Node(n1), p2 = tri->Node(n2), p3 = tri->Node(n3);
            if (hasTransform) { p1.Transform(trsf); p2.Transform(trsf); p3.Transform(trsf); }
            
            // Möller–Trumbore
            gp_Vec e1(p1, p2), e2(p1, p3);
            gp_Vec h = gp_Vec(rayDir).Crossed(e2);
            double a = e1.Dot(h);
            if (std::abs(a) < 1e-12) continue;
            double f = 1.0 / a;
            gp_Vec s(p1, rayOrigin);
            double u = f * s.Dot(h);
            if (u < 0.0 || u > 1.0) continue;
            gp_Vec q = s.Crossed(e1);
            double v = f * gp_Vec(rayDir).Dot(q);
            if (v < 0.0 || u + v > 1.0) continue;
            double t = f * e2.Dot(q);
            if (t > 1e-6 && t < bestDist) {
                bestDist = t;
                bestRegion = ri;
            }
        }
    }
    
    return bestRegion;
}

// ============================================================================
// 3D Ray casting for face/edge picking
// ============================================================================

gp_Pnt Viewport3D::screenTo3DRay(int screenX, int screenY, gp_Dir& rayDir) {
    GLdouble modelview[16], projection[16];
    GLint viewport[4];
    
    // Utiliser les matrices cachées si disponibles (hors paintGL)
    if (m_glMatricesValid) {
        memcpy(modelview, m_cachedModelview, sizeof(modelview));
        memcpy(projection, m_cachedProjection, sizeof(projection));
        memcpy(viewport, m_cachedViewport, sizeof(viewport));
    } else {
        glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
        glGetDoublev(GL_PROJECTION_MATRIX, projection);
        glGetIntegerv(GL_VIEWPORT, viewport);
    }
    
    // Convertir coords logiques → physiques pour gluUnProject (viewport en pixels physiques)
    double dpr = devicePixelRatio();
    double physX = screenX * dpr;
    double physY = screenY * dpr;
    double winY = viewport[3] - physY;

    GLdouble x1, y1, z1, x2, y2, z2;
    gluUnProject(physX, winY, 0.0, modelview, projection, viewport, &x1, &y1, &z1);
    gluUnProject(physX, winY, 1.0, modelview, projection, viewport, &x2, &y2, &z2);
    
    gp_Pnt origin(x1, y1, z1);
    gp_Vec dir(x2 - x1, y2 - y1, z2 - z1);
    if (dir.Magnitude() > 1e-12) {
        dir.Normalize();
        rayDir = gp_Dir(dir);
    } else {
        rayDir = gp_Dir(0, 0, -1);
    }
    
    return origin;
}

TopoDS_Face Viewport3D::pickFaceAtScreen(int screenX, int screenY) {
    if (m_currentBody.IsNull()) return TopoDS_Face();
    
    // Si les matrices GL ne sont pas encore valides, forcer une initialisation
    if (!m_glMatricesValid) {
        makeCurrent();
        // Recréer les matrices manuellement
        int w = width(), h = height();
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        double aspect = (double)w / (double)h;
        gluPerspective(45.0, aspect, 1.0, 20000.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        setupCamera();
        glGetDoublev(GL_MODELVIEW_MATRIX, m_cachedModelview);
        glGetDoublev(GL_PROJECTION_MATRIX, m_cachedProjection);
        glGetIntegerv(GL_VIEWPORT, m_cachedViewport);
        m_glMatricesValid = true;
    }
    
    // Forcer tessellation si pas encore faite
    {
        TopExp_Explorer testExp(m_currentBody, TopAbs_FACE);
        if (testExp.More()) {
            TopLoc_Location loc;
            Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(
                TopoDS::Face(testExp.Current()), loc);
            if (tri.IsNull()) {
                try {
                    BRepMesh_IncrementalMesh mesher(m_currentBody, 0.1, Standard_False, 0.3, Standard_True);
                    mesher.Perform();
                } catch (...) {}
            }
        }
    }
    
    gp_Dir rayDir;
    gp_Pnt rayOrigin = screenTo3DRay(screenX, screenY, rayDir);
    
    // Test intersection with each tessellated face
    // On stocke toutes les intersections pour choisir la meilleure
    struct FaceHit {
        TopoDS_Face face;
        double t;           // Distance le long du rayon
        bool frontFacing;   // Normale face caméra
    };
    std::vector<FaceHit> hits;
    
    TopExp_Explorer explorer(m_currentBody, TopAbs_FACE);
    for (; explorer.More(); explorer.Next()) {
        TopoDS_Face face = TopoDS::Face(explorer.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        if (tri.IsNull()) continue;
        
        gp_Trsf trsf = loc.IsIdentity() ? gp_Trsf() : loc.Transformation();
        bool hasTransform = !loc.IsIdentity();
        
        double faceBestDist = 1e30;
        bool faceFrontFacing = false;
        
        for (int iTri = 1; iTri <= tri->NbTriangles(); iTri++) {
            int n1, n2, n3;
            tri->Triangle(iTri).Get(n1, n2, n3);
            
            gp_Pnt p1 = tri->Node(n1);
            gp_Pnt p2 = tri->Node(n2);
            gp_Pnt p3 = tri->Node(n3);
            
            if (hasTransform) {
                p1.Transform(trsf);
                p2.Transform(trsf);
                p3.Transform(trsf);
            }
            
            // Möller–Trumbore ray-triangle intersection
            gp_Vec e1(p1, p2), e2(p1, p3);
            gp_Vec h = gp_Vec(rayDir).Crossed(e2);
            double a = e1.Dot(h);
            if (std::abs(a) < 1e-12) continue;
            
            double f = 1.0 / a;
            gp_Vec s(p1, rayOrigin);
            double u = f * s.Dot(h);
            if (u < 0.0 || u > 1.0) continue;
            
            gp_Vec q = s.Crossed(e1);
            double v = f * gp_Vec(rayDir).Dot(q);
            if (v < 0.0 || u + v > 1.0) continue;
            
            double t = f * e2.Dot(q);
            if (t > 1e-6 && t < faceBestDist) {
                faceBestDist = t;
                // Normale du triangle : corriger par l'orientation de la face OCCT
                gp_Vec triNormal = e1.Crossed(e2);
                if (face.Orientation() == TopAbs_REVERSED) triNormal.Reverse();
                faceFrontFacing = (triNormal.Dot(gp_Vec(rayDir)) < 0);
            }
        }
        
        if (faceBestDist < 1e29) {
            hits.push_back({face, faceBestDist, faceFrontFacing});
        }
    }
    
    if (hits.empty()) return TopoDS_Face();
    
    // Prendre la face la plus proche (plus petit t)
    // Front-facing n'est utilisé que pour départager des faces quasi-superposées (< 0.5mm)
    TopoDS_Face bestFace = hits[0].face;
    double bestDist = hits[0].t;
    bool bestFront = hits[0].frontFacing;
    
    for (size_t i = 1; i < hits.size(); i++) {
        double dt = hits[i].t - bestDist;
        if (dt < -0.1) {
            // Nettement plus proche → prendre
            bestFace = hits[i].face;
            bestDist = hits[i].t;
            bestFront = hits[i].frontFacing;
        } else if (std::abs(dt) < 0.5 && hits[i].frontFacing && !bestFront) {
            // Même position (< 0.5mm) mais front-facing → préférer
            bestFace = hits[i].face;
            bestDist = hits[i].t;
            bestFront = hits[i].frontFacing;
        }
    }
    
    // Debug: log picked face info (throttled - only when face changes)
    static TopoDS_Face lastLoggedFace;
    if (!bestFace.IsNull() && (lastLoggedFace.IsNull() || !bestFace.IsEqual(lastLoggedFace))) {
        lastLoggedFace = bestFace;
        try {
            GProp_GProps props;
            BRepGProp::SurfaceProperties(bestFace, props);
            gp_Pnt c = props.CentreOfMass();
            std::cout << "[Pick] Face center=(" << c.X() << "," << c.Y() << "," << c.Z() 
                      << ") t=" << bestDist << " front=" << bestFront
                      << " hits=" << hits.size() << std::endl;
        } catch (...) {}
    }
    
    return bestFace;
}

TopoDS_Edge Viewport3D::pickEdgeAtScreen(int screenX, int screenY, double tolerance) {
    if (m_currentBody.IsNull()) return TopoDS_Edge();
    
    GLdouble modelview[16], projection[16];
    GLint viewport[4];
    if (m_glMatricesValid) {
        memcpy(modelview, m_cachedModelview, sizeof(modelview));
        memcpy(projection, m_cachedProjection, sizeof(projection));
        memcpy(viewport, m_cachedViewport, sizeof(viewport));
    } else {
        glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
        glGetDoublev(GL_PROJECTION_MATRIX, projection);
        glGetIntegerv(GL_VIEWPORT, viewport);
    }
    
    double bestDist = tolerance;
    TopoDS_Edge bestEdge;
    
    TopExp_Explorer explorer(m_currentBody, TopAbs_EDGE);
    for (; explorer.More(); explorer.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
        if (edge.IsNull()) continue;
        
        try {
            BRepAdaptor_Curve curve(edge);
            double uFirst = curve.FirstParameter();
            double uLast = curve.LastParameter();
            
            // Échantillonnage uniforme le long de l'arête (fonctionne pour droites ET courbes)
            int nSamples = 20;
            for (int i = 0; i <= nSamples; i++) {
                double u = uFirst + (uLast - uFirst) * i / nSamples;
                gp_Pnt p = curve.Value(u);
                
                GLdouble sx, sy, sz;
                gluProject(p.X(), p.Y(), p.Z(), modelview, projection, viewport, &sx, &sy, &sz);
                sy = viewport[3] - sy;  // gluProject Y depuis bas → depuis haut (physique)

                // Ignorer les points derrière la caméra
                if (sz < 0.0 || sz > 1.0) continue;

                // Comparer en pixels physiques
                double dpr = devicePixelRatio();
                double physSX = screenX * dpr;
                double physSY = screenY * dpr;
                double dist = std::sqrt((sx - physSX) * (sx - physSX) + (sy - physSY) * (sy - physSY));
                if (dist < bestDist) {
                    bestDist = dist;
                    bestEdge = edge;
                }
            }
        } catch (...) {}
    }
    
    return bestEdge;
}

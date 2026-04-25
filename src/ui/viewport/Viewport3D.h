#ifndef VIEWPORT3D_H
#define VIEWPORT3D_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QPainter>
#include <memory>
#include <vector>
#include <set>
#include <map>

#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Dir.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>

// Forward declarations pour éviter dépendances circulaires
namespace CADEngine {
    class CADDocument;
    class SketchFeature;
    class Sketch2D;
    class SketchEntity;
    class Feature;
    class ExtrudeFeature;
    class RevolveFeature;
    class Fillet3DFeature;
    class Chamfer3DFeature;
}

#include "features/sketch/SketchFeature.h"
#include "features/sketch/Sketch2D.h"
#include "features/sketch/SketchEntity.h"
#include "ui/widgets/ViewCube.h"

/**
 * @brief Mode de visualisation
 */
enum class ViewMode {
    NORMAL_3D,    // Mode normal : navigation 3D libre
    SKETCH_2D     // Mode sketch : caméra fixe perpendiculaire au plan
};

/**
 * @brief Type d'outil de dessin
 */
enum class SketchTool {
    None,
    Line,
    Circle,
    Rectangle,
    Arc,
    Polyline,
    Ellipse,             // Ellipse (centre → rayon)
    Polygon,             // Polygone régulier
    ArcCenter,           // Arc depuis le centre
    Oblong,              // Rainure/clavette à bouts arrondis
    Dimension,           // Cotation linéaire
    ConstraintHorizontal,
    ConstraintVertical,
    ConstraintParallel,
    ConstraintPerpendicular,
    ConstraintCoincident,
    ConstraintConcentric,
    ConstraintSymmetric,
    ConstraintFix,
    Fillet               // Congé sur arête (outil géométrique)
};

/**
 * @brief Viewport 3D avec support mode sketch intégré
 * 
 * Gère deux modes :
 * - NORMAL_3D : Navigation 3D classique, rendu shapes
 * - SKETCH_2D : Mode édition sketch, ray-casting, snap, outils dessin
 */
class Viewport3D : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit Viewport3D(QWidget* parent = nullptr);
    ~Viewport3D() override;

    // Modes
    ViewMode getMode() const { return m_mode; }
    void setMode(ViewMode mode);
    
    // Document
    void setDocument(std::shared_ptr<CADEngine::CADDocument> doc) { m_document = doc; }
    
    // Sketch
    void enterSketchMode(std::shared_ptr<CADEngine::SketchFeature> sketch, double centerU = 0.0, double centerV = 0.0, double autoZoom = 0.0);
    void exitSketchMode();
    std::shared_ptr<CADEngine::SketchFeature> getActiveSketch() const { return m_activeSketch; }
    int getLastRightClickedSegmentIndex() const { return m_lastRightClickedSegmentIndex; }
    
    // Outils de dessin
    void setSketchTool(SketchTool tool);
    SketchTool getCurrentTool() const { return m_currentTool; }
    
    // Options de rendu
    void setShowGrid(bool show) { m_showGrid = show; update(); }
    void setShowAxes(bool show) { m_showAxes = show; update(); }
    
    // 3D Operations
    TopoDS_Shape getCurrentBody() const { return m_currentBody; }
    void setCurrentBody(const TopoDS_Shape& body) { 
        m_currentBody = body;
        m_tessCache.clear();  // Forcer retessellation pour le picking
    }
    void invalidateTessCache() { m_tessCache.clear(); update(); }
    void updateThemeColors();  // Met à jour les couleurs du viewport selon le thème
    
    // Edge selection mode (for fillet/chamfer)
    void startEdgeSelection();
    void stopEdgeSelection();
    
    // Face selection mode (Sketch on Face)
    void startFaceSelection();
    void stopFaceSelection();
    bool isFaceSelectionMode() const { return m_faceSelectionMode; }
    std::vector<TopoDS_Edge> getSelectedEdges3D() const { return m_selectedEdges3D; }
    void clearSelectedEdges3D() { m_selectedEdges3D.clear(); update(); }
    
    // Face selection (for sketch on face)
    bool hasSelectedFace() const { return m_hasSelectedFace; }
    TopoDS_Face getSelectedFace() const { return m_selectedFace; }
    void clearSelectedFace() { m_hasSelectedFace = false; m_selectedFace = TopoDS_Face(); }
    
    // Pattern preview (Fusion 360 style live preview)
    void setPatternPreview(const std::vector<TopoDS_Shape>& shapes, bool isCut = false);
    void clearPatternPreview();
    
    // Profile selection mode (Fusion 360 style - click region in viewport)
    struct ProfileRegion {
        TopoDS_Face face;
        std::string description;
        std::vector<std::vector<float>> triangles;  // Cached tessellation for picking
    };
    void startProfileSelection(std::shared_ptr<CADEngine::SketchFeature> sketch);
    void stopProfileSelection();
    bool isProfileSelectionMode() const { return m_profileSelectionMode; }
    TopoDS_Face getSelectedProfileFace() const { return m_selectedProfileFace; }
    void confirmProfileSelection();  // Confirm multi-selection with Enter
    
    // Navigation vues prédéfinies
    void setView(float angleX, float angleZ);
    
    // Accès caméra (pour ViewCube)
    float getCameraAngleX() const { return m_cameraAngleX; }
    float getCameraAngleZ() const { return m_cameraAngleY; }
    gp_Dir getCameraViewDirection() const;
    ViewCube* getViewCube() const { return m_viewCube; }
    
signals:
    void modeChanged(ViewMode mode);
    void sketchEntityAdded();
    void entityCreated(std::shared_ptr<CADEngine::SketchEntity> entity, std::shared_ptr<CADEngine::Sketch2D> sketch);
    void dimensionCreated(std::shared_ptr<CADEngine::Dimension> dimension, std::shared_ptr<CADEngine::Sketch2D> sketch);
    void statusMessage(const QString& message);
    void dimensionClicked(std::shared_ptr<CADEngine::Dimension> dimension);
    void toolChanged(SketchTool tool);
    void entityRightClicked(std::shared_ptr<CADEngine::SketchEntity> entity, QPoint globalPos);
    void dimensionRightClicked(std::shared_ptr<CADEngine::Dimension> dimension, QPoint globalPos);
    void filletRequested(std::shared_ptr<CADEngine::SketchPolyline> polyline, int vertexIndex);
    void filletRectCornerRequested(std::shared_ptr<CADEngine::SketchRectangle> rect, int cornerIdx);
    void filletLineCornerRequested(std::shared_ptr<CADEngine::SketchLine> line1, bool line1AtStart,
                                    std::shared_ptr<CADEngine::SketchLine> line2, bool line2AtStart);
    void faceClicked(TopoDS_Face face);
    void faceSelected(TopoDS_Face face);  // For Sketch on Face mode
    void faceSelectionCancelled();         // Escape during face selection
    void edgeSelected(TopoDS_Edge edge);
    void edgeSelectionConfirmed();         // Enter during edge selection
    void edgeSelectionCancelled();         // Escape during edge selection
    void profileSelected(TopoDS_Face face, QString description);  // For Fusion360-style profile picking
    void multiProfileSelected(std::vector<TopoDS_Face> faces, QString description);  // Multi-profile
    void profileSelectionCancelled();

protected:
    // OpenGL
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    // Events souris
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // === Rendu ===
    void renderNormalMode();
    void renderSketchMode();
    
    void renderGrid();
    void renderAxes();
    void renderSketchPlane();
    void renderSketchGrid();
    void renderSketchAxes();
    void renderSketchEntities();
    void renderTempEntity();
    void renderAngleIndicator(const gp_Pnt2d& origin, double refAngle, double curAngle, double arcRadius);
    void renderAllFeatures();  // Rendu de toutes les features du document
    void renderSketchIn3D(std::shared_ptr<CADEngine::SketchFeature> sketch);  // Rendu sketch en 3D
    
    // === Rendu 3D solides ===
    struct TessVertex { float x, y, z, nx, ny, nz; };
    struct TessCache {
        std::vector<TessVertex> triangles;  // 3 vertices par triangle
        bool valid = false;
    };
    std::map<int, TessCache> m_tessCache;  // feature ID → tessellation
    
    void renderSolidShape(const TopoDS_Shape& shape, int featureId, 
                          float r = 0.6f, float g = 0.65f, float b = 0.72f, float alpha = 1.0f);
    void tessellateShape(const TopoDS_Shape& shape, TessCache& cache);
    void setupLighting();
    void renderSolidEdges(const TopoDS_Shape& shape);
    
    // === Sélection face/arête 3D ===
    TopoDS_Face pickFaceAtScreen(int screenX, int screenY);
    TopoDS_Edge pickEdgeAtScreen(int screenX, int screenY, double tolerance = 20.0);
    int pickProfileRegionAtScreen(int screenX, int screenY);
    gp_Pnt screenTo3DRay(int screenX, int screenY, gp_Dir& rayDir);
    
    // Face sélectionnée (pour Sketch on Face)
    TopoDS_Face m_selectedFace;
    bool m_hasSelectedFace = false;
    
    // Mode sélection face interactive (Sketch on Face)
    bool m_faceSelectionMode = false;
    TopoDS_Face m_hoveredFace;      // Face sous le curseur
    TopoDS_Edge m_hoveredEdge;      // Arête sous le curseur
    
    // Arêtes sélectionnées (pour Fillet/Chamfer)
    std::vector<TopoDS_Edge> m_selectedEdges3D;
    bool m_selectingEdges = false;
    
    // Profile selection mode (Fusion 360 style)
    bool m_profileSelectionMode = false;
    std::vector<ProfileRegion> m_profileRegions;
    int m_hoveredRegionIndex = -1;
    TopoDS_Face m_selectedProfileFace;
    std::set<int> m_selectedProfileIndices;  // Multi-selection tracking
    std::shared_ptr<CADEngine::SketchFeature> m_profileSketch;
    
    // Pattern preview (fantômes semi-transparents)
    std::vector<TopoDS_Shape> m_patternPreviewShapes;
    bool m_patternPreviewIsCut = false;
    
    // Shape résultat courant (body accumulé)
    TopoDS_Shape m_currentBody;
    
    // Reference body shown as ghost wireframe during sketch mode
    TopoDS_Shape m_sketchRefBody;
    std::vector<gp_Pnt2d> m_refBodySnapPoints;  // Snap points from body edges on sketch plane
    void renderSketchReferenceBody();
    
    // === Caméra ===
    void setupCamera();
    void setupSketchCamera();
    
    // === Ray-casting ===
    gp_Pnt2d screenToSketch(int screenX, int screenY);
    gp_Pnt2d snapToGrid(const gp_Pnt2d& pt, double gridSize = 10.0);
    gp_Pnt2d snapToEntities(const gp_Pnt2d& pt, double tolerance = 15.0, 
                            std::shared_ptr<CADEngine::SketchEntity> excludeEntity = nullptr);  // Snap aux extrémités/centres
    
    // === Rendu texte ===
    void renderText(double x, double y, double z, const QString& text);
    void renderDimensions(const std::vector<std::shared_ptr<CADEngine::Dimension>>& dimensions);
    void renderAutoDimensions(const std::vector<std::shared_ptr<CADEngine::Dimension>>& dimensions);
    void renderLinearDimension(std::shared_ptr<CADEngine::LinearDimension> dim);
    void renderDiametralDimension(std::shared_ptr<CADEngine::DiametralDimension> dim);
    void renderAngularDimension(std::shared_ptr<CADEngine::AngularDimension> dim);
    void renderRadialDimension(std::shared_ptr<CADEngine::RadialDimension> dim);
    
    // === Détection clic dimensions ===
    std::shared_ptr<CADEngine::Dimension> findDimensionAtPoint(const gp_Pnt2d& pt);
    std::shared_ptr<CADEngine::SketchEntity> findEntityAtPoint(const gp_Pnt2d& pt);
    void handleAutoDimension(std::shared_ptr<CADEngine::SketchEntity> entity, const gp_Pnt2d& clickPoint);
    
    // === Contraintes géométriques ===
    void handleConstraintClick(const gp_Pnt2d& pt);
    
    // === Mise à jour dimensions après drag ===
    void updateDimensionsForLine(const gp_Pnt2d& oldStart, const gp_Pnt2d& oldEnd,
                                  const gp_Pnt2d& newStart, const gp_Pnt2d& newEnd);
    void updateDimensionsForCircle(const gp_Pnt2d& oldCenter, const gp_Pnt2d& newCenter);
    void updateDimensionsForRectangle(const gp_Pnt2d& oldCorner, double width, double height,
                                       const gp_Pnt2d& newCorner);
    
    // === Polyline vertex drag ===
    int findPolylineVertexAtPoint(std::shared_ptr<CADEngine::SketchPolyline> polyline,
                                   const gp_Pnt2d& pt, double tolerance);
    void updateDimensionsForPolylineVertex(std::shared_ptr<CADEngine::SketchPolyline> polyline,
                                            int vertexIndex);
    
    // === Coincident constraint helpers ===
    struct EndpointInfo {
        gp_Pnt2d point;
        std::shared_ptr<CADEngine::SketchEntity> entity;
        int vertexIndex;  // -1=start, -2=end, -3=center, >=0 polyline vertex index
    };
    EndpointInfo findNearestEndpoint(const gp_Pnt2d& pt, double tolerance);
    
    // === Navigation 3D ===
    void rotateView(int dx, int dy);
    void panView(int dx, int dy);
    void zoomView(int delta);
    
    // === Outils de dessin (finish au relâché) ===
    void handleLineToolFinish();
    void handleCircleToolFinish();
    void handleRectangleToolFinish();
    void handleArcToolFinish();
    void handlePolylineToolFinish();
    void handleDimensionToolFinish();
    void handleEllipseToolFinish();
    void handlePolygonToolFinish();
    void handleArcCenterToolFinish();
    void handleOblongToolFinish();
    
    // Structure pour stocker textes à afficher
    struct TextLabel {
        double x, y, z;  // Position 3D
        QString text;
    };
    std::vector<TextLabel> m_textLabels;  // Textes à afficher ce frame
    
    // Labels légers pour repères d'axes (sans fond, police fine)
    struct AxisTickLabel {
        double x, y, z;
        QString text;
        QColor color;
    };
    std::vector<AxisTickLabel> m_axisTickLabels;
    
    // Labels des axes 3D (avec couleur)
    struct AxisLabel {
        double x, y, z;
        QString text;
        QColor color;
    };
    std::vector<AxisLabel> m_axisLabels;
    
    // === État ===
    ViewMode m_mode;
    
    // Sketch
    std::shared_ptr<CADEngine::SketchFeature> m_activeSketch;
    std::shared_ptr<CADEngine::CADDocument> m_document;  // Document pour accès features
    SketchTool m_currentTool;
    std::vector<gp_Pnt2d> m_tempPoints;  // Points temporaires en cours de dessin
    gp_Pnt2d m_hoverPoint;  // Point de survol pour preview (Arc/Polyline)
    bool m_hasHoverPoint;   // Flag pour savoir si on a un hover
    gp_Pnt2d m_snapPoint;   // Point d'accrochage détecté
    bool m_hasSnapPoint;    // Flag pour afficher le snap
    
    // Highlight extrémités au survol
    std::vector<gp_Pnt2d> m_hoveredEndpoints;  // Extrémités sous la souris
    
    // Dernier segment cliqué droit (pour suppression segment polyline)
    int m_lastRightClickedSegmentIndex = -1;
    
    // Déplacement d'entités
    bool m_isDraggingEntity;
    std::shared_ptr<CADEngine::SketchEntity> m_draggedEntity;
    gp_Pnt2d m_dragStartPoint;
    
    // Déplacement de vertex individuel (polyline/rectangle)
    bool m_isDraggingVertex;
    int m_draggedVertexIndex;  // Index du sommet dans la polyline
    
    // Déplacement/édition dimensions
    bool m_isDraggingDimension;
    std::shared_ptr<CADEngine::Dimension> m_draggedDimension;
    double m_initialOffset;
    
    // Cotation angulaire (2 clics pour angle entre lignes)
    // Cotation angulaire (stockage 1er segment)
    struct AngleSegmentInfo {
        std::shared_ptr<CADEngine::SketchEntity> entity;
        int segmentIndex = -1;  // -1=SketchLine, >=0=segment polyline
        gp_Pnt2d startPt, endPt;  // Pour recherche de vertex commun
    };
    AngleSegmentInfo m_firstSegForAngle;
    AngleSegmentInfo m_firstLineForDistance;  // Ctrl+clic: distance perpendiculaire entre lignes
    
    // Arc Centre: suivi de l'angle accumulé (évite le flip à 180°)
    double m_arcCenterLastAngle = 0.0;
    double m_arcCenterAccumSweep = 0.0;
    bool m_arcCenterTracking = false;
    
    // Contraintes géométriques (sélection multi-entités)
    std::shared_ptr<CADEngine::SketchEntity> m_firstEntityForConstraint;
    int m_firstSegmentIndexForConstraint = -1;  // Index segment polyline (-1 = ligne simple)
    
    // Contrainte coïncidente (stockage endpoint sélectionné)
    EndpointInfo m_firstEndpointForCoincident;
    
    // Cotation point-à-point (2 clics sur endpoints d'entités différentes)
    EndpointInfo m_firstDimensionEndpoint;
    bool m_hasDimensionFirstPoint = false;
    
    // Polygon tool state
    int m_polygonSides = 6;  // Nombre de côtés par défaut
    
    // Contrainte Symétrique: workflow 3 clics (point1, point2, axe)
    struct SymmetricState {
        std::shared_ptr<CADEngine::SketchEntity> entity1;
        int vtx1 = -99;
        std::shared_ptr<CADEngine::SketchEntity> entity2;
        int vtx2 = -99;
        int step = 0;  // 0=attente pt1, 1=attente pt2, 2=attente axe
    };
    SymmetricState m_symmetricState;
    
    // Caméra
    float m_cameraDistance;
    
    // Couleurs viewport (thème)
    float m_bgR = 0.2f, m_bgG = 0.2f, m_bgB = 0.25f;
    float m_cameraAngleX;
    float m_cameraAngleY;
    float m_cameraPanX;
    float m_cameraPanY;
    float m_sketch2DZoom;  // Zoom pour mode sketch (size ortho)
    
    // Matrices GL cachées (mises à jour à chaque paintGL pour le picking)
    double m_cachedModelview[16] = {};
    double m_cachedProjection[16] = {};
    int m_cachedViewport[4] = {};
    bool m_glMatricesValid = false;
    
    // Navigation
    QPoint m_lastMousePos;
    bool m_isRotating;
    bool m_isPanning;
    
    // Options
    bool m_showGrid;
    bool m_showAxes;
    bool m_gridSnapEnabled;  // Ancrage grille actif (toggle avec G)
    
    // Snap H/V automatique
    gp_Pnt2d snapToHV(const gp_Pnt2d& origin, const gp_Pnt2d& pt, double angleTolDeg = 5.0);
    bool m_hvSnapActive;       // Un snap H ou V est actif ce frame
    bool m_hvSnapHorizontal;   // true=H, false=V
    gp_Pnt2d m_hvSnapOrigin;   // Point d'origine du snap (pour guide visuel)
    
    // ViewCube overlay
    ViewCube* m_viewCube;
    void repositionViewCube();
};

#endif // VIEWPORT3D_H

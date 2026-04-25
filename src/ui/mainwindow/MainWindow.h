#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QSplitter>
#include <QActionGroup>
#include <memory>
#include <vector>

#include "ui/viewport/Viewport3D.h"
#include "ui/tree/DocumentTree.h"
#include "core/document/CADDocument.h"
#include "core/commands/Command.h"
#include "ui/theme/ThemeManager.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    // File
    void onNew();
    void onOpen();
    void onSave();
    void onExportSTEP();
    void onExportSTL();
    
    // Create
    void onCreateSketch();
    void onFinishSketch();
    
    // 3D Operations
    void onExtrude();
    void onRevolve();
    void onFillet3D();
    void onChamfer3D();
    void onShell3D();
    void onPushPull();
    void onSweep();
    void onLoft();
    void onLinearPattern();
    void onCircularPattern();
    void onThread();
    void showThreadDialog(const TopoDS_Face& cylindricalFace);
    void onSketchOnFace();
    
    // Sketch Tools
    void onToolSelect();
    void onToolLine();
    void onToolCircle();
    void onToolRectangle();
    void onToolArc();
    void onToolPolyline();
    void onToolEllipse();
    void onToolPolygon();
    void onToolArcCenter();
    void onToolOblong();
    void onToolDimension();
    
    // View
    void onViewFront();
    void onViewTop();
    void onViewRight();
    void onViewIso();
    
    // Viewport
    void onModeChanged(ViewMode mode);
    void onSketchEntityAdded();
    void onEntityCreated(std::shared_ptr<CADEngine::SketchEntity> entity, std::shared_ptr<CADEngine::Sketch2D> sketch);
    void onDimensionCreated(std::shared_ptr<CADEngine::Dimension> dimension, std::shared_ptr<CADEngine::Sketch2D> sketch);
    void onStatusMessage(const QString& message);
    void onDimensionClicked(std::shared_ptr<CADEngine::Dimension> dimension);
    void onToolChanged(SketchTool tool);
    
    // Tree
    void onFeatureSelected(std::shared_ptr<CADEngine::Feature> feature);
    void onFeatureDoubleClicked(std::shared_ptr<CADEngine::Feature> feature);
    void onFeatureDeleted(std::shared_ptr<CADEngine::Feature> feature);
    void onExportSketchDXF(std::shared_ptr<CADEngine::SketchFeature> sketch);
    void onExportSketchPDF(std::shared_ptr<CADEngine::SketchFeature> sketch);
    
    // Suppression
    void onDeleteEntity();
    void onDeleteDimension();
    void onDeletePolylineSegment(int segmentIndex);
    void refreshAllAngularDimensions();
    void onFilletVertex(std::shared_ptr<CADEngine::SketchPolyline> polyline, int vertexIndex);
    
    // Contraintes géométriques
    void onConstraintHorizontal();
    void onConstraintVertical();
    void onConstraintParallel();
    void onConstraintPerpendicular();
    void onConstraintCoincident();
    void onConstraintConcentric();
    void onConstraintSymmetric();
    void onConstraintFix();
    void onFillet();
    
    // Undo/Redo
    void onUndo();
    void onRedo();
    
    // Thèmes
    void onThemeCycle();
    void refreshIcons();
    
    // Aide
    void showHelpSketch();
    void showHelpViewport();

private:
    void createActions();
    void createMenus();
    void createToolbars();
    void createStatusBar();
    void positionDialogRight(QDialog* dlg);  // Position dialog on right side of app
    
    void updateToolbars();
    void applyDimensionConstraint(std::shared_ptr<CADEngine::LinearDimension> dim, double newValue);
    void applyDiameterConstraint(std::shared_ptr<CADEngine::DiametralDimension> dim, double newDiameter);
    void updateAllRectangleDimensions(std::shared_ptr<CADEngine::SketchRectangle> rect, 
                                       std::shared_ptr<CADEngine::Sketch2D> sketch2D);
    void updateAllRectangleDimensionsFromOld(std::shared_ptr<CADEngine::SketchRectangle> rect,
                                              std::shared_ptr<CADEngine::Sketch2D> sketch2D,
                                              const gp_Pnt2d& oldP1, const gp_Pnt2d& oldP2,
                                              const gp_Pnt2d& oldP3, const gp_Pnt2d& oldP4);
    void updateAllPolylineDimensions(std::shared_ptr<CADEngine::SketchPolyline> polyline,
                                      std::shared_ptr<CADEngine::Sketch2D> sketch2D);
    
    void showPolygonDialog(std::shared_ptr<CADEngine::SketchPolygon> polygon);
    void showOblongDialog(std::shared_ptr<CADEngine::SketchOblong> oblong);
    
    // 3D Operation helpers
    std::shared_ptr<CADEngine::SketchFeature> getLastClosedSketch();
    TopoDS_Shape getAccumulatedBody();
    void showExtrudeDialog(std::shared_ptr<CADEngine::SketchFeature> sketch, 
                           const TopoDS_Face& selectedFace = TopoDS_Face(),
                           const QString& profileDesc = QString());
    void showMultiExtrudeDialog(std::shared_ptr<CADEngine::SketchFeature> sketch,
                                const std::vector<TopoDS_Face>& faces,
                                const QString& profileDesc);
    void showRevolveDialog(std::shared_ptr<CADEngine::SketchFeature> sketch);
    void showFillet3DDialog();
    void showChamfer3DDialog();
    
    // Widgets
    QSplitter* m_splitter;
    DocumentTree* m_tree;
    Viewport3D* m_viewport;
    
    // Toolbars
    QToolBar* m_mainToolbar;
    QToolBar* m_sketchToolbar;
    
    // Actions - File (menu seulement, pas de bouton toolbar)
    QAction* m_actNew;
    QAction* m_actOpen;
    QAction* m_actSave;
    QAction* m_actExportSTEP;
    QAction* m_actExportSTL;
    
    // Actions - Sketch
    QAction* m_actCreateSketch;
    QAction* m_actFinishSketch;
    
    // Actions - 3D Operations
    QAction* m_actExtrude;
    QAction* m_actRevolve;
    QAction* m_actFillet3D;
    QAction* m_actChamfer3D;
    QAction* m_actShell3D;
    QAction* m_actPushPull;
    QAction* m_actSweep;
    QAction* m_actLoft;
    QAction* m_actLinearPattern;
    QAction* m_actCircularPattern;
    QAction* m_actThread;
    
    // Pending edge operation (non-blocking selection)
    enum class PendingEdgeOp { None, Fillet, Chamfer };
    PendingEdgeOp m_pendingEdgeOp = PendingEdgeOp::None;
    
    enum class PendingFaceOp { None, Thread };
    PendingFaceOp m_pendingFaceOp = PendingFaceOp::None;
    TopoDS_Face m_pendingSelectedFace;
    QAction* m_actSketchOnFace;
    
    // 3D Operation toolbar
    QToolBar* m_operationsToolbar;
    
    QAction* m_actToolSelect;
    QAction* m_actToolLine;
    QAction* m_actToolCircle;
    QAction* m_actToolRectangle;
    QAction* m_actToolArc;
    QAction* m_actToolPolyline;
    QAction* m_actToolEllipse;
    QAction* m_actToolPolygon;
    QAction* m_actToolArcCenter;
    QAction* m_actToolOblong;
    QAction* m_actToolDimension;
    
    // Contraintes géométriques
    QAction* m_actConstraintHorizontal;
    QAction* m_actConstraintVertical;
    QAction* m_actConstraintParallel;
    QAction* m_actConstraintPerpendicular;
    QAction* m_actConstraintCoincident;
    QAction* m_actConstraintConcentric;
    QAction* m_actConstraintSymmetric;
    QAction* m_actConstraintFix;
    QAction* m_actFillet;
    
    // Actions - View
    QAction* m_actViewFront;
    QAction* m_actViewTop;
    QAction* m_actViewRight;
    QAction* m_actViewIso;
    
    QAction* m_actToggleTree;
    
    // Édition
    QAction* m_actDelete;
    QAction* m_actUndo;
    QAction* m_actRedo;
    
    // Sélection courante pour menu contextuel
    std::shared_ptr<CADEngine::SketchEntity> m_selectedEntity;
    std::shared_ptr<CADEngine::Dimension> m_selectedDimension;
    
    // Undo/Redo
    std::vector<std::shared_ptr<CADEngine::Command>> m_undoStack;
    std::vector<std::shared_ptr<CADEngine::Command>> m_redoStack;
    void executeCommand(std::shared_ptr<CADEngine::Command> command);
    void updateUndoRedoActions();
    
    // Document
    std::shared_ptr<CADEngine::CADDocument> m_document;
    
    // Status
    QLabel* m_statusLabel;
};

#endif // MAINWINDOW_H

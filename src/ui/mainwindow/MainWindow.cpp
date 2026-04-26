#include "MainWindow.h"
#include "features/sketch/SketchFeature.h"
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
#include "core/commands/AddEntityCommand.h"
#include "core/commands/DeleteEntityCommand.h"
#include "core/commands/AddDimensionCommand.h"
#include "core/commands/ModifyDimensionCommand.h"
#include "ui/icons/IconProvider.h"
#include "ui/theme/ThemeManager.h"

#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QTextStream>
#include <QInputDialog>
#include <QDialog>
#include <QVBoxLayout>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QPageLayout>
#include <QFontMetricsF>
#include <QScreen>
#include <iostream>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <QApplication>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QListWidget>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdint>

#include <BRep_Tool.hxx>
#include <Geom_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp_Explorer.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <TopLoc_Location.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <gp_Pln.hxx>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // Appliquer le thème sombre par défaut
    ThemeManager::applyTheme(ThemeManager::Dark, qApp);
    
    setWindowTitle("CAD-ENGINE v0.8 — 3D Operations");
    resize(1200, 800);
    
    // Document
    m_document = std::make_shared<CADEngine::CADDocument>();
    m_document->setName("Untitled");
    
    // Créer le splitter horizontal (tree | viewport)
    m_splitter = new QSplitter(Qt::Horizontal, this);
    
    // Tree à gauche
    m_tree = new DocumentTree(this);
    m_tree->setDocument(m_document);
    
    // Viewport à droite
    m_viewport = new Viewport3D(this);
    m_viewport->setDocument(m_document);  // Connecter le document
    
    // Ajouter au splitter
    m_splitter->addWidget(m_tree);
    m_splitter->addWidget(m_viewport);
    
    // Proportions initiales : 250px pour tree, reste pour viewport
    m_splitter->setStretchFactor(0, 0);  // Tree fixe
    m_splitter->setStretchFactor(1, 1);  // Viewport flexible
    m_splitter->setSizes(QList<int>() << 250 << 950);
    
    setCentralWidget(m_splitter);
    
    // Connexions viewport
    connect(m_viewport, &Viewport3D::modeChanged, this, &MainWindow::onModeChanged);
    connect(m_viewport, &Viewport3D::sketchEntityAdded, this, &MainWindow::onSketchEntityAdded);
    connect(m_viewport, &Viewport3D::entityCreated, this, &MainWindow::onEntityCreated);
    connect(m_viewport, &Viewport3D::dimensionCreated, this, &MainWindow::onDimensionCreated);
    connect(m_viewport, &Viewport3D::statusMessage, this, &MainWindow::onStatusMessage);
    connect(m_viewport, &Viewport3D::dimensionClicked, this, &MainWindow::onDimensionClicked);
    connect(m_viewport, &Viewport3D::entityRightClicked, [this](auto entity, QPoint pos) {
        m_selectedEntity = entity;
        QMenu menu;
        
        if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
            int segIdx = m_viewport->getLastRightClickedSegmentIndex();
            int numPoints = polyline ? (int)polyline->getPoints().size() : 0;
            int numSegments = numPoints - 1;
            
            if (numSegments > 1 && segIdx >= 0) {
                QAction* deleteSegAction = menu.addAction(
                    QString("Supprimer segment %1").arg(segIdx));
                connect(deleteSegAction, &QAction::triggered, [this, segIdx]() {
                    onDeletePolylineSegment(segIdx);
                });
                menu.addSeparator();
            }
            
            QAction* deleteAllAction = menu.addAction("Supprimer polyline entière");
            connect(deleteAllAction, &QAction::triggered, this, &MainWindow::onDeleteEntity);
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
            QAction* modifyAction = menu.addAction("Modifier polygone...");
            connect(modifyAction, &QAction::triggered, [this, entity]() {
                showPolygonDialog(std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity));
            });
            menu.addSeparator();
            menu.addAction(m_actDelete);
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
            QAction* modifyAction = menu.addAction("Modifier oblong...");
            connect(modifyAction, &QAction::triggered, [this, entity]() {
                showOblongDialog(std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity));
            });
            menu.addSeparator();
            menu.addAction(m_actDelete);
        }
        else {
            menu.addAction(m_actDelete);
        }
        
        menu.exec(pos);
    });
    connect(m_viewport, &Viewport3D::dimensionRightClicked, [this](auto dimension, QPoint pos) {
        m_selectedDimension = dimension;
        QMenu menu;
        QAction* deleteAction = menu.addAction("Supprimer cotation");
        connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteDimension);
        menu.exec(pos);
    });
    connect(m_viewport, &Viewport3D::toolChanged, this, &MainWindow::onToolChanged);
    
    // Connexion fillet
    connect(m_viewport, &Viewport3D::filletRequested, this, &MainWindow::onFilletVertex);
    connect(m_viewport, &Viewport3D::filletRectCornerRequested, this, &MainWindow::onFilletRectCorner);
    connect(m_viewport, &Viewport3D::filletLineCornerRequested, this, &MainWindow::onFilletLineCorner);
    connect(m_viewport, &Viewport3D::edgeSelectionConfirmed, this, [this]() {
        if (m_pendingEdgeOp == PendingEdgeOp::Fillet) {
            m_pendingEdgeOp = PendingEdgeOp::None;
            showFillet3DDialog();
        } else if (m_pendingEdgeOp == PendingEdgeOp::Chamfer) {
            m_pendingEdgeOp = PendingEdgeOp::None;
            showChamfer3DDialog();
        }
    });
    connect(m_viewport, &Viewport3D::edgeSelectionCancelled, this, [this]() {
        m_pendingEdgeOp = PendingEdgeOp::None;
    });
    
    // Connexions tree
    connect(m_tree, &DocumentTree::featureSelected, this, &MainWindow::onFeatureSelected);
    connect(m_tree, &DocumentTree::featureDoubleClicked, this, &MainWindow::onFeatureDoubleClicked);
    connect(m_tree, &DocumentTree::deleteFeatureRequested, this, &MainWindow::onFeatureDeleted);
    connect(m_tree, &DocumentTree::exportSketchDXFRequested, this, &MainWindow::onExportSketchDXF);
    connect(m_tree, &DocumentTree::exportSketchPDFRequested, this, &MainWindow::onExportSketchPDF);
    
    // UI
    createActions();
    createMenus();
    createToolbars();
    createStatusBar();
    
    updateToolbars();
    
    onStatusMessage("Prêt - Mode 3D");
}

MainWindow::~MainWindow() {
}

// ============================================================================
// Actions
// ============================================================================

void MainWindow::createActions() {
    auto ic = ThemeManager::isDark() ? IconProvider::darkColors() : IconProvider::lightColors();
    
    // File (menu seulement)
    m_actNew = new QAction("&Nouveau", this);
    m_actNew->setShortcut(QKeySequence::New);
    connect(m_actNew, &QAction::triggered, this, &MainWindow::onNew);
    
    m_actOpen = new QAction("&Ouvrir...", this);
    m_actOpen->setShortcut(QKeySequence::Open);
    connect(m_actOpen, &QAction::triggered, this, &MainWindow::onOpen);
    
    m_actSave = new QAction("&Enregistrer", this);
    m_actSave->setShortcut(QKeySequence::Save);
    connect(m_actSave, &QAction::triggered, this, &MainWindow::onSave);
    
    m_actExportSTEP = new QAction("Exporter STEP...", this);
    connect(m_actExportSTEP, &QAction::triggered, this, &MainWindow::onExportSTEP);
    
    m_actExportSTL = new QAction("Exporter STL...", this);
    connect(m_actExportSTL, &QAction::triggered, this, &MainWindow::onExportSTL);
    
    // Create
    m_actCreateSketch = new QAction(IconProvider::sketchIcon(ic), "Sketch", this);
    m_actCreateSketch->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_K));
    m_actCreateSketch->setToolTip("Créer un sketch (Ctrl+K)");
    connect(m_actCreateSketch, &QAction::triggered, this, &MainWindow::onCreateSketch);
    
    m_actFinishSketch = new QAction(IconProvider::finishSketchIcon(ic), "Terminer", this);
    m_actFinishSketch->setToolTip("Terminer le sketch");
    connect(m_actFinishSketch, &QAction::triggered, this, &MainWindow::onFinishSketch);
    
    // Sketch Tools (avec icônes)
    m_actToolSelect = new QAction(IconProvider::selectIcon(ic), "Sélection", this);
    m_actToolSelect->setToolTip("Sélection (Esc)");
    m_actToolSelect->setCheckable(true);
    m_actToolSelect->setChecked(true);
    connect(m_actToolSelect, &QAction::triggered, this, &MainWindow::onToolSelect);
    
    m_actToolLine = new QAction(IconProvider::lineIcon(ic), "Ligne", this);
    m_actToolLine->setToolTip("Ligne (L)");
    m_actToolLine->setShortcut(QKeySequence(Qt::Key_L));
    m_actToolLine->setCheckable(true);
    connect(m_actToolLine, &QAction::triggered, this, &MainWindow::onToolLine);
    
    m_actToolCircle = new QAction(IconProvider::circleIcon(ic), "Cercle", this);
    m_actToolCircle->setToolTip("Cercle (C)");
    m_actToolCircle->setShortcut(QKeySequence(Qt::Key_C));
    m_actToolCircle->setCheckable(true);
    connect(m_actToolCircle, &QAction::triggered, this, &MainWindow::onToolCircle);
    
    m_actToolRectangle = new QAction(IconProvider::rectangleIcon(ic), "Rectangle", this);
    m_actToolRectangle->setToolTip("Rectangle (R)");
    m_actToolRectangle->setShortcut(QKeySequence(Qt::Key_R));
    m_actToolRectangle->setCheckable(true);
    connect(m_actToolRectangle, &QAction::triggered, this, &MainWindow::onToolRectangle);
    
    m_actToolArc = new QAction(IconProvider::arcIcon(ic), "Arc Bézier", this);
    m_actToolArc->setToolTip("Arc Bézier (A) — Début → Fin → Contrôle");
    m_actToolArc->setShortcut(QKeySequence(Qt::Key_A));
    m_actToolArc->setCheckable(true);
    connect(m_actToolArc, &QAction::triggered, this, &MainWindow::onToolArc);
    
    m_actToolPolyline = new QAction(IconProvider::polylineIcon(ic), "Polyligne", this);
    m_actToolPolyline->setToolTip("Polyligne (P)");
    m_actToolPolyline->setShortcut(QKeySequence(Qt::Key_P));
    m_actToolPolyline->setCheckable(true);
    connect(m_actToolPolyline, &QAction::triggered, this, &MainWindow::onToolPolyline);
    
    m_actToolEllipse = new QAction(IconProvider::ellipseIcon(ic), "Ellipse", this);
    m_actToolEllipse->setToolTip("Ellipse (E)");
    m_actToolEllipse->setShortcut(QKeySequence(Qt::Key_E));
    m_actToolEllipse->setCheckable(true);
    connect(m_actToolEllipse, &QAction::triggered, this, &MainWindow::onToolEllipse);
    
    m_actToolPolygon = new QAction(IconProvider::polygonIcon(ic), "Polygone", this);
    m_actToolPolygon->setToolTip("Polygone");
    m_actToolPolygon->setCheckable(true);
    connect(m_actToolPolygon, &QAction::triggered, this, &MainWindow::onToolPolygon);
    
    m_actToolArcCenter = new QAction(IconProvider::arcCenterIcon(ic), "Arc Centre", this);
    m_actToolArcCenter->setToolTip("Arc depuis Centre (Shift+A)");
    m_actToolArcCenter->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_A));
    m_actToolArcCenter->setCheckable(true);
    connect(m_actToolArcCenter, &QAction::triggered, this, &MainWindow::onToolArcCenter);
    
    m_actToolOblong = new QAction(IconProvider::oblongIcon(ic), "Oblong", this);
    m_actToolOblong->setToolTip("Oblong/Rainure (O)");
    m_actToolOblong->setShortcut(QKeySequence(Qt::Key_O));
    m_actToolOblong->setCheckable(true);
    connect(m_actToolOblong, &QAction::triggered, this, &MainWindow::onToolOblong);
    
    m_actToolDimension = new QAction(IconProvider::dimensionIcon(ic), "Cotation", this);
    m_actToolDimension->setToolTip("Cotation (D) — Shift+Clic pour angle");
    m_actToolDimension->setShortcut(QKeySequence(Qt::Key_D));
    m_actToolDimension->setCheckable(true);
    connect(m_actToolDimension, &QAction::triggered, this, &MainWindow::onToolDimension);
    
    // Contraintes géométriques (avec icônes)
    m_actConstraintHorizontal = new QAction(IconProvider::constraintHIcon(ic), "Horizontal", this);
    m_actConstraintHorizontal->setToolTip("Contrainte Horizontale");
    connect(m_actConstraintHorizontal, &QAction::triggered, this, &MainWindow::onConstraintHorizontal);
    
    m_actConstraintVertical = new QAction(IconProvider::constraintVIcon(ic), "Vertical", this);
    m_actConstraintVertical->setToolTip("Contrainte Verticale");
    connect(m_actConstraintVertical, &QAction::triggered, this, &MainWindow::onConstraintVertical);
    
    m_actConstraintParallel = new QAction(IconProvider::constraintParallelIcon(ic), "Parallèle", this);
    m_actConstraintParallel->setToolTip("Contrainte Parallèle (2 segments)");
    connect(m_actConstraintParallel, &QAction::triggered, this, &MainWindow::onConstraintParallel);
    
    m_actConstraintPerpendicular = new QAction(IconProvider::constraintPerpIcon(ic), "Perpendiculaire", this);
    m_actConstraintPerpendicular->setToolTip("Contrainte Perpendiculaire (2 segments)");
    connect(m_actConstraintPerpendicular, &QAction::triggered, this, &MainWindow::onConstraintPerpendicular);
    
    m_actConstraintCoincident = new QAction(IconProvider::constraintCoincidentIcon(ic), "Coïncident", this);
    m_actConstraintCoincident->setToolTip("Contrainte Coïncidente (2 points)");
    connect(m_actConstraintCoincident, &QAction::triggered, this, &MainWindow::onConstraintCoincident);
    
    m_actConstraintConcentric = new QAction(IconProvider::constraintConcentricIcon(ic), "Concentrique", this);
    m_actConstraintConcentric->setToolTip("Contrainte Concentrique (2 cercles)");
    connect(m_actConstraintConcentric, &QAction::triggered, this, &MainWindow::onConstraintConcentric);
    
    m_actConstraintSymmetric = new QAction(IconProvider::constraintSymmetricIcon(ic), "Symétrique", this);
    m_actConstraintSymmetric->setToolTip("Centrer sur axe de symétrie");
    connect(m_actConstraintSymmetric, &QAction::triggered, this, &MainWindow::onConstraintSymmetric);
    
    m_actConstraintFix = new QAction(IconProvider::constraintFixIcon(ic), "Verrouiller", this);
    m_actConstraintFix->setToolTip("Verrouiller un point");
    connect(m_actConstraintFix, &QAction::triggered, this, &MainWindow::onConstraintFix);
    
    m_actFillet = new QAction(IconProvider::filletIcon(ic), "Congé", this);
    m_actFillet->setToolTip("Congé sur vertex de polyline");
    connect(m_actFillet, &QAction::triggered, this, &MainWindow::onFillet);
    
    // 3D Operations (avec icônes)
    m_actExtrude = new QAction(IconProvider::extrudeIcon(ic), "Extrusion", this);
    m_actExtrude->setToolTip("Extrusion (E) — Positif ou négatif");
    m_actExtrude->setShortcut(QKeySequence(Qt::Key_E));
    connect(m_actExtrude, &QAction::triggered, this, &MainWindow::onExtrude);
    
    m_actRevolve = new QAction(IconProvider::revolveIcon(ic), "Révolution", this);
    m_actRevolve->setToolTip("Révolution (R)");
    m_actRevolve->setShortcut(QKeySequence(Qt::Key_R));
    connect(m_actRevolve, &QAction::triggered, this, &MainWindow::onRevolve);
    
    m_actFillet3D = new QAction(IconProvider::fillet3DIcon(ic), "Congé 3D", this);
    m_actFillet3D->setToolTip("Congé 3D sur arêtes");
    connect(m_actFillet3D, &QAction::triggered, this, &MainWindow::onFillet3D);
    
    m_actChamfer3D = new QAction(IconProvider::chamfer3DIcon(ic), "Chanfrein 3D", this);
    m_actChamfer3D->setToolTip("Chanfrein 3D sur arêtes");
    connect(m_actChamfer3D, &QAction::triggered, this, &MainWindow::onChamfer3D);
    
    m_actShell3D = new QAction(IconProvider::shellIcon(ic), "Coque", this);
    m_actShell3D->setToolTip("Coque — Évider un solide (Shell)");
    connect(m_actShell3D, &QAction::triggered, this, &MainWindow::onShell3D);
    
    m_actPushPull = new QAction(IconProvider::pushPullIcon(ic), "Appuyer/Tirer", this);
    m_actPushPull->setToolTip("Appuyer/Tirer — Déplacer une face (Push/Pull)");
    connect(m_actPushPull, &QAction::triggered, this, &MainWindow::onPushPull);
    
    m_actSweep = new QAction(IconProvider::sweepIcon(ic), "Balayage", this);
    m_actSweep->setToolTip("Balayage — Extrusion le long d'un chemin");
    connect(m_actSweep, &QAction::triggered, this, &MainWindow::onSweep);
    
    m_actLoft = new QAction(IconProvider::loftIcon(ic), "Lissage", this);
    m_actLoft->setToolTip("Lissage — Transition entre profils");
    connect(m_actLoft, &QAction::triggered, this, &MainWindow::onLoft);
    
    m_actLinearPattern = new QAction(IconProvider::linearPatternIcon(ic), "Réseau linéaire", this);
    m_actLinearPattern->setToolTip("Réseau linéaire — Copies le long d'un axe");
    connect(m_actLinearPattern, &QAction::triggered, this, &MainWindow::onLinearPattern);
    
    m_actCircularPattern = new QAction(IconProvider::circularPatternIcon(ic), "Réseau circulaire", this);
    m_actCircularPattern->setToolTip("Réseau circulaire — Copies autour d'un axe");
    connect(m_actCircularPattern, &QAction::triggered, this, &MainWindow::onCircularPattern);
    
    m_actThread = new QAction(IconProvider::threadIcon(ic), "Filetage", this);
    m_actThread->setToolTip("Filetage métrique sur face cylindrique");
    connect(m_actThread, &QAction::triggered, this, &MainWindow::onThread);
    
    m_actSketchOnFace = new QAction(IconProvider::sketchOnFaceIcon(ic), "Sketch sur Face", this);
    m_actSketchOnFace->setToolTip("Créer sketch sur une face 3D");
    connect(m_actSketchOnFace, &QAction::triggered, this, &MainWindow::onSketchOnFace);
    
    // View
    m_actViewFront = new QAction("Vue Avant", this);
    connect(m_actViewFront, &QAction::triggered, this, &MainWindow::onViewFront);
    
    m_actViewTop = new QAction("Vue Dessus", this);
    connect(m_actViewTop, &QAction::triggered, this, &MainWindow::onViewTop);
    
    m_actViewRight = new QAction("Vue Droite", this);
    connect(m_actViewRight, &QAction::triggered, this, &MainWindow::onViewRight);
    
    m_actViewIso = new QAction("Vue Isométrique", this);
    connect(m_actViewIso, &QAction::triggered, this, &MainWindow::onViewIso);
    
    // Toggle arborescence
    m_actToggleTree = new QAction(IconProvider::toggleTreeIcon(ic), "", this);
    m_actToggleTree->setToolTip("Afficher/Masquer l'arborescence");
    connect(m_actToggleTree, &QAction::triggered, [this]() {
        m_tree->toggleCollapsed();
    });
    
    // Édition
    m_actDelete = new QAction("Supprimer", this);
    m_actDelete->setShortcut(Qt::Key_Delete);
    connect(m_actDelete, &QAction::triggered, this, &MainWindow::onDeleteEntity);
    addAction(m_actDelete);
    
    m_actUndo = new QAction(IconProvider::undoIcon(ic), "Annuler", this);
    m_actUndo->setShortcut(QKeySequence::Undo);
    m_actUndo->setToolTip("Annuler (Ctrl+Z)");
    m_actUndo->setEnabled(false);
    connect(m_actUndo, &QAction::triggered, this, &MainWindow::onUndo);
    addAction(m_actUndo);
    
    m_actRedo = new QAction(IconProvider::redoIcon(ic), "Rétablir", this);
    m_actRedo->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z));
    m_actRedo->setToolTip("Rétablir (Ctrl+Shift+Z)");
    m_actRedo->setEnabled(false);
    connect(m_actRedo, &QAction::triggered, this, &MainWindow::onRedo);
    addAction(m_actRedo);
}

void MainWindow::createMenus() {
    // Fichier
    QMenu* fileMenu = menuBar()->addMenu("&Fichier");
    fileMenu->addAction(m_actNew);
    fileMenu->addAction(m_actOpen);
    fileMenu->addAction(m_actSave);
    fileMenu->addSeparator();
    fileMenu->addAction(m_actExportSTEP);
    fileMenu->addAction(m_actExportSTL);
    fileMenu->addSeparator();
    fileMenu->addAction("&Quitter", QKeySequence::Quit, this, &QWidget::close);
    
    // Édition
    QMenu* editMenu = menuBar()->addMenu("É&dition");
    editMenu->addAction(m_actUndo);
    editMenu->addAction(m_actRedo);
    editMenu->addSeparator();
    editMenu->addAction(m_actDelete);
    
    // Créer
    QMenu* createMenu = menuBar()->addMenu("&Créer");
    createMenu->addAction(m_actCreateSketch);
    createMenu->addAction(m_actSketchOnFace);
    createMenu->addSeparator();
    createMenu->addAction(m_actExtrude);
    createMenu->addAction(m_actRevolve);
    createMenu->addSeparator();
    createMenu->addAction(m_actFillet3D);
    createMenu->addAction(m_actChamfer3D);
    createMenu->addAction(m_actShell3D);
    createMenu->addAction(m_actPushPull);
    createMenu->addSeparator();
    createMenu->addAction(m_actSweep);
    createMenu->addAction(m_actLoft);
    createMenu->addSeparator();
    createMenu->addAction(m_actLinearPattern);
    createMenu->addAction(m_actCircularPattern);
    createMenu->addSeparator();
    createMenu->addAction(m_actThread);
    
    // Vue
    QMenu* viewMenu = menuBar()->addMenu("&Vue");
    viewMenu->addAction(m_actViewFront);
    viewMenu->addAction(m_actViewTop);
    viewMenu->addAction(m_actViewRight);
    viewMenu->addAction(m_actViewIso);
    viewMenu->addSeparator();
    
    // Thème — bouton direct dans la barre de menu
    menuBar()->addAction("🎨 Thème", this, &MainWindow::onThemeCycle);
    
    // Aide
    QMenu* helpMenu = menuBar()->addMenu("&Aide");
    helpMenu->addAction("Raccourcis Sketch", this, &MainWindow::showHelpSketch);
    helpMenu->addAction("Navigation Viewport 3D", this, &MainWindow::showHelpViewport);
    helpMenu->addSeparator();
    helpMenu->addAction("À propos", [this]() {
        QMessageBox::about(this, "CAD-ENGINE",
            "CAD-ENGINE v0.8 — 3D Operations\n\n"
            "Moteur CAO paramétrique\n"
            "C++17 / Qt6 / OpenCASCADE 7.9.3 / OpenGL 3.3+\n\n"
            "Phase 1 3D :\n"
            "• Extrusion (positive/négative, Fusion 360 style)\n"
            "• Révolution (axe sketch ou global)\n"
            "• Congé 3D / Chanfrein 3D sur arêtes\n"
            "• Balayage (Sweep) — Extrusion le long d'un chemin\n"
            "• Lissage (Loft) — Transition entre profils\n"
            "• Réseau linéaire / circulaire\n"
            "• Filetage métrique\n"
            "• Sketch sur Face plane\n\n"
            "Sketch 2D avec contraintes géométriques,\n"
            "cotations paramétriques et congés.");
    });
}

void MainWindow::createToolbars() {
    // === MAIN TOOLBAR (always visible) ===
    m_mainToolbar = addToolBar("Principal");
    m_mainToolbar->setIconSize(QSize(24, 24));
    m_mainToolbar->setMovable(false);
    
    // Left group: Tree toggle + Undo/Redo
    m_mainToolbar->addAction(m_actToggleTree);
    m_mainToolbar->addSeparator();
    m_mainToolbar->addAction(m_actUndo);
    m_mainToolbar->addAction(m_actRedo);
    m_mainToolbar->addSeparator();
    
    // Sketch group (left side)
    m_mainToolbar->addAction(m_actCreateSketch);
    m_mainToolbar->addAction(m_actSketchOnFace);
    m_mainToolbar->addSeparator();
    
    // Spacer to push 3D operations to the right
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_mainToolbar->addWidget(spacer);
    
    // 3D Operations (right side)
    m_mainToolbar->addAction(m_actExtrude);
    m_mainToolbar->addAction(m_actRevolve);
    m_mainToolbar->addSeparator();
    m_mainToolbar->addAction(m_actFillet3D);
    m_mainToolbar->addAction(m_actChamfer3D);
    m_mainToolbar->addAction(m_actShell3D);
    m_mainToolbar->addAction(m_actPushPull);
    m_mainToolbar->addSeparator();
    m_mainToolbar->addAction(m_actSweep);
    m_mainToolbar->addAction(m_actLoft);
    m_mainToolbar->addSeparator();
    m_mainToolbar->addAction(m_actLinearPattern);
    m_mainToolbar->addAction(m_actCircularPattern);
    m_mainToolbar->addSeparator();
    m_mainToolbar->addAction(m_actThread);
    
    // === SKETCH TOOLBAR (visible only in sketch mode) ===
    // Layout: [spacerL] [✓ | Select | Dessin...] [spacerCenter] [Contraintes... | Congé] 
    m_sketchToolbar = addToolBar("Outils Sketch");
    m_sketchToolbar->setIconSize(QSize(24, 24));
    m_sketchToolbar->setMovable(false);
    
    // ── Left spacer (pousse les outils de dessin vers le centre) ──
    QWidget* sketchSpacerL = new QWidget();
    sketchSpacerL->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_sketchToolbar->addWidget(sketchSpacerL);
    
    // ── Groupe central : Validation + Sélection + Outils de dessin ──
    m_sketchToolbar->addAction(m_actFinishSketch);
    m_sketchToolbar->addSeparator();
    
    m_sketchToolbar->addAction(m_actToolSelect);
    m_sketchToolbar->addSeparator();
    
    m_sketchToolbar->addAction(m_actToolLine);
    m_sketchToolbar->addAction(m_actToolCircle);
    m_sketchToolbar->addAction(m_actToolRectangle);
    m_sketchToolbar->addAction(m_actToolArc);
    m_sketchToolbar->addAction(m_actToolArcCenter);
    m_sketchToolbar->addAction(m_actToolPolyline);
    m_sketchToolbar->addAction(m_actToolEllipse);
    m_sketchToolbar->addAction(m_actToolPolygon);
    m_sketchToolbar->addAction(m_actToolOblong);
    m_sketchToolbar->addSeparator();
    m_sketchToolbar->addAction(m_actToolDimension);
    m_sketchToolbar->addAction(m_actFillet);
    
    // ── Spacer central (sépare dessin ↔ contraintes) ──
    QWidget* sketchSpacerCenter = new QWidget();
    sketchSpacerCenter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_sketchToolbar->addWidget(sketchSpacerCenter);
    
    // ── Groupe droit : Contraintes (collées à droite) ──
    m_sketchToolbar->addAction(m_actConstraintHorizontal);
    m_sketchToolbar->addAction(m_actConstraintVertical);
    m_sketchToolbar->addAction(m_actConstraintParallel);
    m_sketchToolbar->addAction(m_actConstraintPerpendicular);
    m_sketchToolbar->addAction(m_actConstraintCoincident);
    m_sketchToolbar->addAction(m_actConstraintConcentric);
    m_sketchToolbar->addAction(m_actConstraintSymmetric);
    m_sketchToolbar->addAction(m_actConstraintFix);
}

void MainWindow::createStatusBar() {
    m_statusLabel = new QLabel("Prêt");
    statusBar()->addPermanentWidget(m_statusLabel);
}

void MainWindow::positionDialogRight(QDialog* dlg) {
    // Position dialog on the right side of the main window
    dlg->adjustSize();
    QRect mainGeo = geometry();
    int margin = 10;
    int dlgX = mainGeo.right() - dlg->width() - margin;
    int dlgY = mainGeo.top() + 80;  // Below toolbar
    // Clamp to screen
    QScreen* screen = this->screen();
    if (screen) {
        QRect screenGeo = screen->availableGeometry();
        if (dlgX + dlg->width() > screenGeo.right())
            dlgX = screenGeo.right() - dlg->width();
        if (dlgY + dlg->height() > screenGeo.bottom())
            dlgY = screenGeo.bottom() - dlg->height();
    }
    dlg->move(dlgX, dlgY);
}

void MainWindow::updateToolbars() {
    bool isSketchMode = (m_viewport->getMode() == ViewMode::SKETCH_2D);
    
    m_sketchToolbar->setVisible(isSketchMode);
    m_actCreateSketch->setEnabled(!isSketchMode);
    
    // 3D Operations: enabled only when NOT in sketch mode
    bool hasBody = !getAccumulatedBody().IsNull();
    m_actExtrude->setEnabled(!isSketchMode);
    m_actRevolve->setEnabled(!isSketchMode);
    m_actFillet3D->setEnabled(!isSketchMode && hasBody);
    m_actChamfer3D->setEnabled(!isSketchMode && hasBody);
    m_actShell3D->setEnabled(!isSketchMode && hasBody);
    m_actPushPull->setEnabled(!isSketchMode && hasBody);
    m_actSweep->setEnabled(!isSketchMode);
    m_actLoft->setEnabled(!isSketchMode);
    m_actLinearPattern->setEnabled(!isSketchMode && hasBody);
    m_actCircularPattern->setEnabled(!isSketchMode && hasBody);
    m_actThread->setEnabled(!isSketchMode && hasBody);
    m_actSketchOnFace->setEnabled(!isSketchMode && hasBody);
    
    // Swap E/R shortcuts based on mode to avoid conflicts
    if (isSketchMode) {
        // Sketch mode: E=Ellipse, R=Rectangle
        m_actToolEllipse->setShortcut(QKeySequence(Qt::Key_E));
        m_actToolRectangle->setShortcut(QKeySequence(Qt::Key_R));
        m_actExtrude->setShortcut(QKeySequence());
        m_actRevolve->setShortcut(QKeySequence());
    } else {
        // 3D mode: E=Extrude, R=Revolve
        m_actExtrude->setShortcut(QKeySequence(Qt::Key_E));
        m_actRevolve->setShortcut(QKeySequence(Qt::Key_R));
        m_actToolEllipse->setShortcut(QKeySequence());
        m_actToolRectangle->setShortcut(QKeySequence());
    }
}

// ============================================================================
// Slots - File
// ============================================================================

void MainWindow::onNew() {
    // Vérifier si le document actuel contient du travail
    if (m_document && !m_document->getAllFeatures().empty()) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Nouveau document");
        msgBox.setText("Le document actuel contient des modifications non enregistrées.");
        msgBox.setInformativeText("Voulez-vous enregistrer avant de créer un nouveau document ?");
        msgBox.setIcon(QMessageBox::Warning);
        
        QPushButton* saveBtn = msgBox.addButton("Enregistrer", QMessageBox::AcceptRole);
        QPushButton* discardBtn = msgBox.addButton("Ne pas enregistrer", QMessageBox::DestructiveRole);
        QPushButton* cancelBtn = msgBox.addButton("Annuler", QMessageBox::RejectRole);
        msgBox.setDefaultButton(cancelBtn);
        
        msgBox.exec();
        
        if (msgBox.clickedButton() == cancelBtn) {
            return;  // Annuler — ne rien faire
        }
        if (msgBox.clickedButton() == saveBtn) {
            onSave();  // Enregistrer d'abord
        }
        // "Ne pas enregistrer" → continuer sans sauvegarder
    }
    
    // Quitter le mode sketch si actif
    m_viewport->exitSketchMode();
    
    // Créer un nouveau document vierge
    m_document = std::make_shared<CADEngine::CADDocument>();
    m_document->setName("Untitled");
    
    // Reconnecter l'arbre et le viewport au nouveau document
    m_tree->setDocument(m_document);
    m_viewport->setDocument(m_document);
    
    // Réinitialiser complètement l'état 3D du viewport
    m_viewport->stopEdgeSelection();
    m_viewport->stopFaceSelection();
    m_viewport->stopProfileSelection();
    m_viewport->clearPatternPreview();
    m_viewport->invalidateTessCache();
    m_viewport->clearSelectedEdges3D();
    m_viewport->clearSelectedFace();
    
    // Vider l'historique undo/redo
    m_undoStack.clear();
    m_redoStack.clear();
    
    // Rafraîchir l'arbre
    m_tree->refresh();
    
    // Mettre à jour le titre
    setWindowTitle("CAD-ENGINE - Untitled");
    
    onStatusMessage("Nouveau document créé");
}

void MainWindow::onOpen() {
    QString filename = QFileDialog::getOpenFileName(
        this, "Ouvrir", "", "CAD-ENGINE (*.cadengine);;Tous (*)");
    
    if (filename.isEmpty()) return;
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Erreur", "Impossible d'ouvrir le fichier.");
        return;
    }
    
    QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        QMessageBox::warning(this, "Erreur", "Format de fichier invalide.");
        return;
    }
    
    QJsonObject root = jsonDoc.object();
    
    // Nouveau document
    m_viewport->exitSketchMode();
    m_document = std::make_shared<CADEngine::CADDocument>();
    m_document->setName(root["name"].toString("Untitled").toStdString());
    m_tree->setDocument(m_document);
    m_viewport->setDocument(m_document);
    m_viewport->invalidateTessCache();
    m_undoStack.clear();
    m_redoStack.clear();
    
    std::map<int, std::shared_ptr<CADEngine::Feature>> idMap;
    TopoDS_Shape currentBody;  // Corps accumulé pour les opérations booléennes
    
    QJsonArray features = root["features"].toArray();
    for (const auto& fVal : features) {
        QJsonObject fObj = fVal.toObject();
        QString type = fObj["type"].toString();
        QString name = fObj["name"].toString();
        int oldId = fObj["id"].toInt();
        
        std::shared_ptr<CADEngine::Feature> feature;
        
        if (type == "Sketch") {
            auto sketch = m_document->addFeature<CADEngine::SketchFeature>(name.toStdString());
            
            QJsonObject params = fObj["parameters"].toObject();
            
            // Reconstruire le plan EXACT depuis les paramètres stockés
            double ox = params.contains("plane_origin_x") ? params["plane_origin_x"].toObject()["value"].toDouble() : 0;
            double oy = params.contains("plane_origin_y") ? params["plane_origin_y"].toObject()["value"].toDouble() : 0;
            double oz = params.contains("plane_origin_z") ? params["plane_origin_z"].toObject()["value"].toDouble() : 0;
            double nx = params.contains("plane_normal_x") ? params["plane_normal_x"].toObject()["value"].toDouble() : 0;
            double ny = params.contains("plane_normal_y") ? params["plane_normal_y"].toObject()["value"].toDouble() : 0;
            double nz = params.contains("plane_normal_z") ? params["plane_normal_z"].toObject()["value"].toDouble() : 1;
            double xx = params.contains("plane_xdir_x") ? params["plane_xdir_x"].toObject()["value"].toDouble() : 1;
            double xy = params.contains("plane_xdir_y") ? params["plane_xdir_y"].toObject()["value"].toDouble() : 0;
            double xz = params.contains("plane_xdir_z") ? params["plane_xdir_z"].toObject()["value"].toDouble() : 0;
            
            gp_Pnt origin(ox, oy, oz);
            gp_Dir normal(nx, ny, nz);
            gp_Dir xDir(xx, xy, xz);
            gp_Ax3 ax3(origin, normal, xDir);
            gp_Pln plane(ax3);
            sketch->setPlane(plane);
            
            std::cout << "[LOAD] Sketch '" << name.toStdString() << "' oldId=" << oldId 
                      << " origin=(" << ox << "," << oy << "," << oz << ")"
                      << " normal=(" << nx << "," << ny << "," << nz << ")"
                      << " xdir=(" << xx << "," << xy << "," << xz << ")" << std::endl;
            
            // Stocker le plane_type pour les labels d'axes
            QString planeType = "XY";
            if (params.contains("plane_type"))
                planeType = params["plane_type"].toObject()["value"].toString();
            sketch->setString("plane_type", planeType.toStdString());
            
            auto sketch2D = sketch->getSketch2D();
            QJsonArray entities = fObj["entities"].toArray();
            
            for (const auto& eVal : entities) {
                QJsonObject eObj = eVal.toObject();
                QString eType = eObj["type"].toString();
                
                if (eType == "Line") {
                    sketch2D->addEntity(std::make_shared<CADEngine::SketchLine>(
                        gp_Pnt2d(eObj["x1"].toDouble(), eObj["y1"].toDouble()),
                        gp_Pnt2d(eObj["x2"].toDouble(), eObj["y2"].toDouble())));
                }
                else if (eType == "Circle") {
                    sketch2D->addEntity(std::make_shared<CADEngine::SketchCircle>(
                        gp_Pnt2d(eObj["cx"].toDouble(), eObj["cy"].toDouble()),
                        eObj["radius"].toDouble()));
                }
                else if (eType == "Arc") {
                    sketch2D->addEntity(std::make_shared<CADEngine::SketchArc>(
                        gp_Pnt2d(eObj["x1"].toDouble(), eObj["y1"].toDouble()),
                        gp_Pnt2d(eObj["x2"].toDouble(), eObj["y2"].toDouble()),
                        gp_Pnt2d(eObj["mx"].toDouble(), eObj["my"].toDouble()),
                        eObj["bezier"].toBool()));
                }
                else if (eType == "Rectangle") {
                    sketch2D->addEntity(std::make_shared<CADEngine::SketchRectangle>(
                        gp_Pnt2d(eObj["cx"].toDouble(), eObj["cy"].toDouble()),
                        eObj["width"].toDouble(), eObj["height"].toDouble()));
                }
                else if (eType == "Polyline") {
                    auto p = std::make_shared<CADEngine::SketchPolyline>();
                    for (const auto& ptVal : eObj["points"].toArray()) {
                        QJsonObject ptObj = ptVal.toObject();
                        p->addPoint(gp_Pnt2d(ptObj["x"].toDouble(), ptObj["y"].toDouble()));
                    }
                    sketch2D->addEntity(p);
                }
                else if (eType == "Ellipse") {
                    sketch2D->addEntity(std::make_shared<CADEngine::SketchEllipse>(
                        gp_Pnt2d(eObj["cx"].toDouble(), eObj["cy"].toDouble()),
                        eObj["majorRadius"].toDouble(), eObj["minorRadius"].toDouble(),
                        eObj["angle"].toDouble()));
                }
                else if (eType == "Polygon") {
                    sketch2D->addEntity(std::make_shared<CADEngine::SketchPolygon>(
                        gp_Pnt2d(eObj["cx"].toDouble(), eObj["cy"].toDouble()),
                        eObj["radius"].toDouble(), eObj["sides"].toInt(),
                        eObj["angle"].toDouble()));
                }
                else if (eType == "Oblong") {
                    sketch2D->addEntity(std::make_shared<CADEngine::SketchOblong>(
                        gp_Pnt2d(eObj["cx"].toDouble(), eObj["cy"].toDouble()),
                        eObj["length"].toDouble(), eObj["width"].toDouble(),
                        eObj["angle"].toDouble()));
                }
            }
            
            // Restaurer les contraintes (indices: -1=axeH, -2=axeV, >=0 = entité utilisateur)
            QJsonArray constraintsArr = fObj["constraints"].toArray();
            if (!constraintsArr.isEmpty()) {
                // Construire la liste des entités utilisateur (même ordre que sauvegarde)
                std::vector<std::shared_ptr<CADEngine::SketchEntity>> userEntities;
                for (const auto& e : sketch2D->getEntities()) {
                    if (!e->isConstruction()) userEntities.push_back(e);
                }
                auto axH = sketch2D->getAxisLineH();
                auto axV = sketch2D->getAxisLineV();
                
                for (const auto& cVal : constraintsArr) {
                    QJsonObject cObj = cVal.toObject();
                    QString cType = cObj["type"].toString();
                    
                    if (cType == "CenterOnAxis") {
                        int eIdx = cObj["entityIdx"].toInt();
                        int aIdx = cObj["axisIdx"].toInt();
                        
                        std::shared_ptr<CADEngine::SketchEntity> entity;
                        std::shared_ptr<CADEngine::SketchLine> axis;
                        
                        if (eIdx >= 0 && eIdx < (int)userEntities.size())
                            entity = userEntities[eIdx];
                        if (aIdx == -1 && axH) axis = axH;
                        else if (aIdx == -2 && axV) axis = axV;
                        else if (aIdx >= 0 && aIdx < (int)userEntities.size())
                            axis = std::dynamic_pointer_cast<CADEngine::SketchLine>(userEntities[aIdx]);
                        
                        if (entity && axis) {
                            sketch2D->addConstraint(
                                std::make_shared<CADEngine::CenterOnAxisConstraint>(entity, axis));
                        }
                    }
                }
                sketch2D->solveConstraints();
            }
            
            sketch->compute();
            std::cout << "[LOAD]   → entities=" << sketch->getEntityCount() 
                      << " compute=" << (sketch->isValid() ? "OK" : "FAIL") << std::endl;
            feature = sketch;
        }
        else if (type == "Extrude") {
            auto extrude = m_document->addFeature<CADEngine::ExtrudeFeature>(name.toStdString());
            QJsonObject params = fObj["parameters"].toObject();
            for (auto it = params.begin(); it != params.end(); ++it) {
                QJsonObject pObj = it.value().toObject();
                int pType = pObj["type"].toInt();
                if (pType == 0) extrude->setDouble(it.key().toStdString(), pObj["value"].toDouble());
                else if (pType == 1) extrude->setInt(it.key().toStdString(), pObj["value"].toInt());
                else if (pType == 2) extrude->setString(it.key().toStdString(), pObj["value"].toString().toStdString());
                else if (pType == 3) extrude->setBool(it.key().toStdString(), pObj["value"].toBool());
            }
            int sketchId = extrude->getInt("sketch_id");
            std::cout << "[LOAD] Extrude '" << name.toStdString() << "' oldId=" << oldId 
                      << " sketch_id=" << sketchId 
                      << " dist=" << extrude->getDouble("distance")
                      << " op=" << extrude->getInt("operation") << std::endl;
            
            if (idMap.count(sketchId)) {
                auto sk = std::dynamic_pointer_cast<CADEngine::SketchFeature>(idMap[sketchId]);
                if (sk) {
                    extrude->setSketchFeature(sk);
                    std::cout << "[LOAD]   → sketch found, newId=" << sk->getId() 
                              << " entities=" << sk->getEntityCount() << std::endl;
                }
            } else {
                std::cout << "[LOAD]   → sketch_id " << sketchId << " NOT FOUND in idMap!" << std::endl;
            }
            
            // Passer le corps accumulé pour les opérations booléennes (Cut/Join)
            if (!currentBody.IsNull()) {
                extrude->setExistingBody(currentBody);
                std::cout << "[LOAD]   → existingBody set (not null)" << std::endl;
            } else {
                std::cout << "[LOAD]   → no existingBody (null)" << std::endl;
            }
            
            bool ok = extrude->compute();
            std::cout << "[LOAD]   → compute=" << (ok ? "OK" : "FAIL") 
                      << " shape=" << (extrude->getShape().IsNull() ? "NULL" : "VALID") << std::endl;
            
            // Mettre à jour le corps accumulé
            if (extrude->isValid() && !extrude->getShape().IsNull()) {
                currentBody = extrude->getShape();
                std::cout << "[LOAD]   → currentBody updated" << std::endl;
            }
            feature = extrude;
        }
        else if (type == "Revolve") {
            auto revolve = m_document->addFeature<CADEngine::RevolveFeature>(name.toStdString());
            QJsonObject params = fObj["parameters"].toObject();
            for (auto it = params.begin(); it != params.end(); ++it) {
                QJsonObject pObj = it.value().toObject();
                int pType = pObj["type"].toInt();
                if (pType == 0) revolve->setDouble(it.key().toStdString(), pObj["value"].toDouble());
                else if (pType == 1) revolve->setInt(it.key().toStdString(), pObj["value"].toInt());
            }
            int sketchId = revolve->getInt("sketch_id");
            if (idMap.count(sketchId)) {
                auto sk = std::dynamic_pointer_cast<CADEngine::SketchFeature>(idMap[sketchId]);
                if (sk) { revolve->setSketchFeature(sk); revolve->addDependency(sk->getId()); }
            }
            if (!currentBody.IsNull()) {
                revolve->setExistingBody(currentBody);
            }
            revolve->compute();
            if (revolve->isValid() && !revolve->getShape().IsNull()) {
                currentBody = revolve->getShape();
            }
            feature = revolve;
        }
        // === Fillet 3D ===
        else if (type == "Fillet3D") {
            auto fillet = m_document->addFeature<CADEngine::Fillet3DFeature>(name.toStdString());
            QJsonObject params = fObj["parameters"].toObject();
            double radius = 2.0;
            bool allEdges = true;
            if (params.contains("radius")) radius = params["radius"].toObject()["value"].toDouble();
            if (params.contains("all_edges")) allEdges = params["all_edges"].toObject()["value"].toBool();
            
            if (!currentBody.IsNull()) {
                fillet->setBaseShape(currentBody);
                fillet->setRadius(radius);
                
                if (!allEdges && fObj.contains("edge_midpoints")) {
                    // Retrouver les arêtes par leurs midpoints
                    QJsonArray savedMidpoints = fObj["edge_midpoints"].toArray();
                    fillet->setAllEdges(false);
                    int matched = 0;
                    for (const auto& mpVal : savedMidpoints) {
                        QJsonObject mp = mpVal.toObject();
                        gp_Pnt target(mp["x"].toDouble(), mp["y"].toDouble(), mp["z"].toDouble());
                        
                        double bestDist = 1e10;
                        TopoDS_Edge bestEdge;
                        TopExp_Explorer exp(currentBody, TopAbs_EDGE);
                        for (; exp.More(); exp.Next()) {
                            TopoDS_Edge edge = TopoDS::Edge(exp.Current());
                            try {
                                double first, last;
                                Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
                                if (!curve.IsNull()) {
                                    gp_Pnt mid = curve->Value((first + last) / 2.0);
                                    double d = mid.Distance(target);
                                    if (d < bestDist) { bestDist = d; bestEdge = edge; }
                                }
                            } catch (...) {}
                        }
                        if (!bestEdge.IsNull() && bestDist < 1.0) {
                            fillet->addEdge(bestEdge);
                            matched++;
                        }
                    }
                    std::cout << "[LOAD] Fillet3D: matched " << matched << "/" << savedMidpoints.size() << " edges" << std::endl;
                } else {
                    fillet->setAllEdges(allEdges);
                }
                
                if (fillet->compute()) {
                    currentBody = fillet->getResultShape();
                }
            }
            std::cout << "[LOAD] Fillet3D radius=" << radius << " allEdges=" << allEdges 
                      << " result=" << (fillet->isValid() ? "OK" : "FAIL") << std::endl;
            feature = fillet;
        }
        // === Chamfer 3D ===
        else if (type == "Chamfer3D") {
            auto chamfer = m_document->addFeature<CADEngine::Chamfer3DFeature>(name.toStdString());
            QJsonObject params = fObj["parameters"].toObject();
            double distance = 2.0;
            bool allEdges = true;
            if (params.contains("distance")) distance = params["distance"].toObject()["value"].toDouble();
            if (params.contains("all_edges")) allEdges = params["all_edges"].toObject()["value"].toBool();
            
            if (!currentBody.IsNull()) {
                chamfer->setBaseShape(currentBody);
                chamfer->setDistance(distance);
                
                if (!allEdges && fObj.contains("edge_midpoints")) {
                    // Retrouver les arêtes par leurs midpoints
                    QJsonArray savedMidpoints = fObj["edge_midpoints"].toArray();
                    chamfer->setAllEdges(false);
                    int matched = 0;
                    for (const auto& mpVal : savedMidpoints) {
                        QJsonObject mp = mpVal.toObject();
                        gp_Pnt target(mp["x"].toDouble(), mp["y"].toDouble(), mp["z"].toDouble());
                        
                        double bestDist = 1e10;
                        TopoDS_Edge bestEdge;
                        TopExp_Explorer exp(currentBody, TopAbs_EDGE);
                        for (; exp.More(); exp.Next()) {
                            TopoDS_Edge edge = TopoDS::Edge(exp.Current());
                            try {
                                double first, last;
                                Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
                                if (!curve.IsNull()) {
                                    gp_Pnt mid = curve->Value((first + last) / 2.0);
                                    double d = mid.Distance(target);
                                    if (d < bestDist) { bestDist = d; bestEdge = edge; }
                                }
                            } catch (...) {}
                        }
                        if (!bestEdge.IsNull() && bestDist < 1.0) {
                            chamfer->addEdge(bestEdge);
                            matched++;
                        }
                    }
                    std::cout << "[LOAD] Chamfer3D: matched " << matched << "/" << savedMidpoints.size() << " edges" << std::endl;
                } else {
                    chamfer->setAllEdges(allEdges);
                }
                
                if (chamfer->compute()) {
                    currentBody = chamfer->getResultShape();
                }
            }
            std::cout << "[LOAD] Chamfer3D distance=" << distance << " allEdges=" << allEdges 
                      << " result=" << (chamfer->isValid() ? "OK" : "FAIL") << std::endl;
            feature = chamfer;
        }
        // === Shell (Coque) ===
        else if (type == "Shell") {
            auto shell = m_document->addFeature<CADEngine::ShellFeature>(name.toStdString());
            QJsonObject params = fObj["parameters"].toObject();
            double thickness = 2.0;
            bool outward = false;
            if (params.contains("thickness")) thickness = params["thickness"].toObject()["value"].toDouble();
            if (params.contains("outward")) outward = params["outward"].toObject()["value"].toBool();
            
            if (!currentBody.IsNull()) {
                shell->setExistingBody(currentBody);
                shell->setThickness(thickness);
                shell->setOutward(outward);
                
                TopoDS_Face bestFace;
                
                // Retrouver la face par centre + normale
                if (fObj.contains("open_face_centers")) {
                    QJsonArray faceCenters = fObj["open_face_centers"].toArray();
                    for (const auto& fcVal : faceCenters) {
                        QJsonObject fc = fcVal.toObject();
                        gp_Pnt targetCenter(fc["cx"].toDouble(), fc["cy"].toDouble(), fc["cz"].toDouble());
                        gp_Dir targetNormal(fc["nx"].toDouble(), fc["ny"].toDouble(), fc["nz"].toDouble());
                        
                        double bestScore = 1e10;
                        TopExp_Explorer exp(currentBody, TopAbs_FACE);
                        for (; exp.More(); exp.Next()) {
                            TopoDS_Face f = TopoDS::Face(exp.Current());
                            try {
                                GProp_GProps props;
                                BRepGProp::SurfaceProperties(f, props);
                                gp_Pnt center = props.CentreOfMass();
                                double distCenter = center.Distance(targetCenter);
                                
                                BRepAdaptor_Surface adaptor(f);
                                double uMid = (adaptor.FirstUParameter() + adaptor.LastUParameter()) / 2.0;
                                double vMid = (adaptor.FirstVParameter() + adaptor.LastVParameter()) / 2.0;
                                gp_Pnt pt; gp_Vec du, dv;
                                adaptor.D1(uMid, vMid, pt, du, dv);
                                gp_Vec n = du.Crossed(dv);
                                if (n.Magnitude() > 1e-10) {
                                    n.Normalize();
                                    if (f.Orientation() == TopAbs_REVERSED) n.Reverse();
                                    double dot = n.X()*targetNormal.X() + n.Y()*targetNormal.Y() + n.Z()*targetNormal.Z();
                                    double score = distCenter + (1.0 - dot) * 100.0;
                                    if (score < bestScore) { bestScore = score; bestFace = f; }
                                }
                            } catch (...) {}
                        }
                    }
                    if (!bestFace.IsNull()) {
                        std::cout << "[LOAD] Shell: face matched by center+normal" << std::endl;
                    }
                }
                
                // Fallback: plus grande face
                if (bestFace.IsNull()) {
                    double maxArea = 0;
                    TopExp_Explorer exp(currentBody, TopAbs_FACE);
                    for (; exp.More(); exp.Next()) {
                        TopoDS_Face f = TopoDS::Face(exp.Current());
                        GProp_GProps props;
                        BRepGProp::SurfaceProperties(f, props);
                        double area = props.Mass();
                        if (area > maxArea) { maxArea = area; bestFace = f; }
                    }
                    std::cout << "[LOAD] Shell: fallback to biggest face" << std::endl;
                }
                
                if (!bestFace.IsNull()) {
                    shell->addOpenFace(bestFace);
                    if (shell->compute()) {
                        currentBody = shell->getResultShape();
                    }
                }
            }
            std::cout << "[LOAD] Shell thickness=" << thickness << " outward=" << outward 
                      << " result=" << (shell->isValid() ? "OK" : "FAIL") << std::endl;
            feature = shell;
        }
        // === Linear Pattern ===
        else if (type == "LinearPattern") {
            auto pattern = m_document->addFeature<CADEngine::LinearPatternFeature>(name.toStdString());
            QJsonObject params = fObj["parameters"].toObject();
            for (auto it = params.begin(); it != params.end(); ++it) {
                QJsonObject pObj = it.value().toObject();
                int pType = pObj["type"].toInt();
                if (pType == 0) pattern->setDouble(it.key().toStdString(), pObj["value"].toDouble());
                else if (pType == 1) pattern->setInt(it.key().toStdString(), pObj["value"].toInt());
                else if (pType == 3) pattern->setBool(it.key().toStdString(), pObj["value"].toBool());
            }
            
            // Trouver la dernière extrusion pour featureShape
            TopoDS_Shape bodyBefore;
            TopoDS_Shape featureShape;
            int featureOp = 0;
            TopoDS_Shape prevBody;
            for (const auto& f : m_document->getAllFeatures()) {
                if (f->getId() == pattern->getId()) break;
                auto ext = std::dynamic_pointer_cast<CADEngine::ExtrudeFeature>(f);
                if (ext && ext->isValid()) {
                    bodyBefore = prevBody;
                    featureShape = ext->buildExtrudeShape();
                    featureOp = static_cast<int>(ext->getOperation());
                    prevBody = ext->getResultShape();
                    continue;
                }
                auto rev = std::dynamic_pointer_cast<CADEngine::RevolveFeature>(f);
                if (rev && rev->isValid()) { prevBody = rev->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); }
                auto fil = std::dynamic_pointer_cast<CADEngine::Fillet3DFeature>(f);
                if (fil && fil->isValid()) { prevBody = fil->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); }
                auto cham = std::dynamic_pointer_cast<CADEngine::Chamfer3DFeature>(f);
                if (cham && cham->isValid()) { prevBody = cham->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); }
                auto sh = std::dynamic_pointer_cast<CADEngine::ShellFeature>(f);
                if (sh && sh->isValid()) { prevBody = sh->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); }
            }
            
            if (!featureShape.IsNull() && !bodyBefore.IsNull()) {
                pattern->setExistingBody(bodyBefore);
                pattern->setFeatureShape(featureShape);
                pattern->setFeatureOperation(featureOp);
            } else if (!currentBody.IsNull()) {
                pattern->setBaseShape(currentBody);
            }
            
            if (pattern->compute()) {
                currentBody = pattern->getResultShape();
            }
            std::cout << "[LOAD] LinearPattern count=" << pattern->getCount() 
                      << " spacing=" << pattern->getSpacing()
                      << " result=" << (pattern->isValid() ? "OK" : "FAIL") << std::endl;
            feature = pattern;
        }
        // === Circular Pattern ===
        else if (type == "CircularPattern") {
            auto pattern = m_document->addFeature<CADEngine::CircularPatternFeature>(name.toStdString());
            QJsonObject params = fObj["parameters"].toObject();
            for (auto it = params.begin(); it != params.end(); ++it) {
                QJsonObject pObj = it.value().toObject();
                int pType = pObj["type"].toInt();
                if (pType == 0) pattern->setDouble(it.key().toStdString(), pObj["value"].toDouble());
                else if (pType == 1) pattern->setInt(it.key().toStdString(), pObj["value"].toInt());
                else if (pType == 3) pattern->setBool(it.key().toStdString(), pObj["value"].toBool());
            }
            
            // Même logique que LinearPattern pour trouver featureShape
            TopoDS_Shape bodyBefore;
            TopoDS_Shape featureShape;
            int featureOp = 0;
            TopoDS_Shape prevBody;
            for (const auto& f : m_document->getAllFeatures()) {
                if (f->getId() == pattern->getId()) break;
                auto ext = std::dynamic_pointer_cast<CADEngine::ExtrudeFeature>(f);
                if (ext && ext->isValid()) {
                    bodyBefore = prevBody;
                    featureShape = ext->buildExtrudeShape();
                    featureOp = static_cast<int>(ext->getOperation());
                    prevBody = ext->getResultShape();
                    continue;
                }
                auto rev = std::dynamic_pointer_cast<CADEngine::RevolveFeature>(f);
                if (rev && rev->isValid()) { prevBody = rev->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); }
                auto fil = std::dynamic_pointer_cast<CADEngine::Fillet3DFeature>(f);
                if (fil && fil->isValid()) { prevBody = fil->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); }
                auto cham = std::dynamic_pointer_cast<CADEngine::Chamfer3DFeature>(f);
                if (cham && cham->isValid()) { prevBody = cham->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); }
                auto sh = std::dynamic_pointer_cast<CADEngine::ShellFeature>(f);
                if (sh && sh->isValid()) { prevBody = sh->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); }
            }
            
            if (!featureShape.IsNull() && !bodyBefore.IsNull()) {
                pattern->setExistingBody(bodyBefore);
                pattern->setFeatureShape(featureShape);
                pattern->setFeatureOperation(featureOp);
            } else if (!currentBody.IsNull()) {
                pattern->setBaseShape(currentBody);
            }
            
            // Restaurer l'axe
            double ax = pattern->getDouble("axis_x");
            double ay = pattern->getDouble("axis_y");
            double az = pattern->getDouble("axis_z");
            if (std::abs(ax) + std::abs(ay) + std::abs(az) > 0.01) {
                pattern->setAxis(gp_Ax1(gp_Pnt(0,0,0), gp_Dir(ax, ay, az)));
            }
            
            if (pattern->compute()) {
                currentBody = pattern->getResultShape();
            }
            std::cout << "[LOAD] CircularPattern count=" << pattern->getCount()
                      << " angle=" << pattern->getAngle()
                      << " result=" << (pattern->isValid() ? "OK" : "FAIL") << std::endl;
            feature = pattern;
        }
        // === Sweep ===
        else if (type == "Sweep") {
            auto sweep = m_document->addFeature<CADEngine::SweepFeature>(name.toStdString());
            QJsonObject params = fObj["parameters"].toObject();
            for (auto it = params.begin(); it != params.end(); ++it) {
                QJsonObject pObj = it.value().toObject();
                int pType = pObj["type"].toInt();
                if (pType == 0) sweep->setDouble(it.key().toStdString(), pObj["value"].toDouble());
                else if (pType == 1) sweep->setInt(it.key().toStdString(), pObj["value"].toInt());
            }
            
            // Trouver les sketches profil et chemin via les dépendances
            QJsonArray deps = fObj["dependencies"].toArray();
            if (deps.size() >= 2) {
                int profileId = deps[0].toInt();
                int pathId = deps[1].toInt();
                if (idMap.count(profileId)) {
                    auto sk = std::dynamic_pointer_cast<CADEngine::SketchFeature>(idMap[profileId]);
                    if (sk) sweep->setProfileSketch(sk);
                }
                if (idMap.count(pathId)) {
                    auto sk = std::dynamic_pointer_cast<CADEngine::SketchFeature>(idMap[pathId]);
                    if (sk) sweep->setPathSketch(sk);
                }
            }
            if (!currentBody.IsNull()) sweep->setExistingBody(currentBody);
            
            if (sweep->compute()) {
                currentBody = sweep->getResultShape();
            }
            std::cout << "[LOAD] Sweep result=" << (sweep->isValid() ? "OK" : "FAIL") << std::endl;
            feature = sweep;
        }
        // === Thread ===
        else if (type == "Thread") {
            auto thread = m_document->addFeature<CADEngine::ThreadFeature>(name.toStdString());
            QJsonObject params = fObj["parameters"].toObject();
            for (auto it = params.begin(); it != params.end(); ++it) {
                QJsonObject pObj = it.value().toObject();
                int pType = pObj["type"].toInt();
                if (pType == 0) thread->setDouble(it.key().toStdString(), pObj["value"].toDouble());
                else if (pType == 1) thread->setInt(it.key().toStdString(), pObj["value"].toInt());
                else if (pType == 2) thread->setString(it.key().toStdString(), pObj["value"].toString().toStdString());
                else if (pType == 3) thread->setBool(it.key().toStdString(), pObj["value"].toBool());
            }
            
            // Thread nécessite une face cylindrique — chercher sur le body actuel
            if (!currentBody.IsNull()) {
                thread->setBaseShape(currentBody);
                // Chercher une face cylindrique
                TopExp_Explorer exp(currentBody, TopAbs_FACE);
                for (; exp.More(); exp.Next()) {
                    TopoDS_Face f = TopoDS::Face(exp.Current());
                    BRepAdaptor_Surface adaptor(f);
                    if (adaptor.GetType() == GeomAbs_Cylinder) {
                        thread->setCylindricalFace(f);
                        break;
                    }
                }
                if (thread->compute()) {
                    currentBody = thread->getResultShape();
                }
            }
            std::cout << "[LOAD] Thread result=" << (thread->isValid() ? "OK" : "FAIL") << std::endl;
            feature = thread;
        }
        // === Loft ===
        else if (type == "Loft") {
            auto loft = m_document->addFeature<CADEngine::LoftFeature>(name.toStdString());
            QJsonObject params = fObj["parameters"].toObject();
            for (auto it = params.begin(); it != params.end(); ++it) {
                QJsonObject pObj = it.value().toObject();
                int pType = pObj["type"].toInt();
                if (pType == 0) loft->setDouble(it.key().toStdString(), pObj["value"].toDouble());
                else if (pType == 1) loft->setInt(it.key().toStdString(), pObj["value"].toInt());
                else if (pType == 3) loft->setBool(it.key().toStdString(), pObj["value"].toBool());
            }
            
            // Restaurer les profils via les dépendances
            QJsonArray deps = fObj["dependencies"].toArray();
            for (const auto& dep : deps) {
                int skId = dep.toInt();
                if (idMap.count(skId)) {
                    auto sk = std::dynamic_pointer_cast<CADEngine::SketchFeature>(idMap[skId]);
                    if (sk) loft->addProfileSketch(sk);
                }
            }
            if (!currentBody.IsNull()) loft->setExistingBody(currentBody);
            
            if (loft->compute()) {
                currentBody = loft->getResultShape();
            }
            std::cout << "[LOAD] Loft profiles=" << loft->getProfileSketches().size()
                      << " result=" << (loft->isValid() ? "OK" : "FAIL") << std::endl;
            feature = loft;
        }
        // === PushPull ===
        else if (type == "PushPull") {
            auto pp = m_document->addFeature<CADEngine::PushPullFeature>(name.toStdString());
            QJsonObject params = fObj["parameters"].toObject();
            for (auto it = params.begin(); it != params.end(); ++it) {
                QJsonObject pObj = it.value().toObject();
                int pType = pObj["type"].toInt();
                if (pType == 0) pp->setDouble(it.key().toStdString(), pObj["value"].toDouble());
                else if (pType == 1) pp->setInt(it.key().toStdString(), pObj["value"].toInt());
                else if (pType == 3) pp->setBool(it.key().toStdString(), pObj["value"].toBool());
            }
            
            if (!currentBody.IsNull()) {
                pp->setExistingBody(currentBody);
                
                // Retrouver la face via centroïde + normale
                double nx = pp->getDouble("normal_x");
                double ny = pp->getDouble("normal_y");
                double nz = pp->getDouble("normal_z");
                
                bool hasCentroid = false;
                gp_Pnt targetCenter;
                try {
                    double cx = pp->getDouble("face_cx");
                    double cy = pp->getDouble("face_cy");
                    double cz = pp->getDouble("face_cz");
                    targetCenter = gp_Pnt(cx, cy, cz);
                    hasCentroid = true;
                } catch (...) {}
                
                gp_Dir targetNormal(nx, ny, nz);
                
                double bestScore = 1e10;
                TopoDS_Face bestFace;
                TopExp_Explorer exp(currentBody, TopAbs_FACE);
                for (; exp.More(); exp.Next()) {
                    TopoDS_Face f = TopoDS::Face(exp.Current());
                    try {
                        BRepAdaptor_Surface adaptor(f);
                        double uMid = (adaptor.FirstUParameter() + adaptor.LastUParameter()) / 2.0;
                        double vMid = (adaptor.FirstVParameter() + adaptor.LastVParameter()) / 2.0;
                        gp_Pnt pt; gp_Vec du, dv;
                        adaptor.D1(uMid, vMid, pt, du, dv);
                        gp_Vec n = du.Crossed(dv);
                        if (n.Magnitude() > 1e-10) {
                            n.Normalize();
                            if (f.Orientation() == TopAbs_REVERSED) n.Reverse();
                            double dot = n.X()*nx + n.Y()*ny + n.Z()*nz;
                            
                            double score;
                            if (hasCentroid) {
                                GProp_GProps props;
                                BRepGProp::SurfaceProperties(f, props);
                                gp_Pnt center = props.CentreOfMass();
                                double distCenter = center.Distance(targetCenter);
                                score = distCenter + (1.0 - dot) * 100.0;
                            } else {
                                score = (1.0 - dot) * 100.0;
                            }
                            if (score < bestScore) { bestScore = score; bestFace = f; }
                        }
                    } catch (...) {}
                }
                
                if (!bestFace.IsNull() && bestScore < 50.0) {
                    pp->setFace(bestFace);
                    if (pp->compute()) {
                        currentBody = pp->getResultShape();
                    }
                    std::cout << "[LOAD] PushPull: face matched (score=" << bestScore << ")" << std::endl;
                } else {
                    std::cout << "[LOAD] PushPull: no matching face found!" << std::endl;
                }
            }
            std::cout << "[LOAD] PushPull distance=" << pp->getDistance()
                      << " result=" << (pp->isValid() ? "OK" : "FAIL") << std::endl;
            feature = pp;
        }
        // === Type inconnu ===
        else {
            std::cout << "[LOAD] Type inconnu ignoré: " << type.toStdString() << std::endl;
        }
        
        if (feature) idMap[oldId] = feature;
    }
    
    m_viewport->invalidateTessCache();
    
    // Forcer un recompute complet de toutes les features
    m_document->recomputeAll();
    
    // Forcer la tessellation du corps final pour le picking de faces
    if (!currentBody.IsNull()) {
        try {
            BRepMesh_IncrementalMesh mesher(currentBody, 0.1, Standard_False, 0.3, Standard_True);
            mesher.Perform();
        } catch (...) {}
        m_viewport->setCurrentBody(currentBody);
    }
    
    m_tree->refresh();
    m_document->setModified(false);
    setWindowTitle(QString("CAD-ENGINE - %1").arg(QFileInfo(filename).baseName()));
    onStatusMessage(QString("Ouvert: %1").arg(filename));
    
    // Rafraîchir l'état des boutons (activer les outils 3D si un body existe)
    updateToolbars();
    
    // Forcer plusieurs cycles de rendu pour que les matrices GL soient valides
    m_viewport->update();
    QTimer::singleShot(50, this, [this]() { m_viewport->update(); });
    QTimer::singleShot(200, this, [this]() { m_viewport->update(); });
}

void MainWindow::onSave() {
    QString filename = QFileDialog::getSaveFileName(
        this, "Enregistrer", "", "CAD-ENGINE (*.cadengine)");
    
    if (filename.isEmpty()) return;
    if (!filename.toLower().endsWith(".cadengine"))
        filename += ".cadengine";
    
    QJsonObject root;
    root["version"] = 1;
    root["name"] = QString::fromStdString(m_document->getName());
    
    QJsonArray featuresArr;
    for (const auto& feature : m_document->getAllFeatures()) {
        QJsonObject fObj;
        fObj["type"] = QString::fromStdString(feature->getTypeName());
        fObj["name"] = QString::fromStdString(feature->getName());
        fObj["id"] = feature->getId();
        
        // Paramètres
        QJsonObject params;
        for (const auto& [pname, param] : feature->getParameters()) {
            QJsonObject pObj;
            pObj["type"] = static_cast<int>(param.type);
            switch (param.type) {
                case CADEngine::ParameterType::Double: pObj["value"] = param.doubleValue; break;
                case CADEngine::ParameterType::Integer: pObj["value"] = param.intValue; break;
                case CADEngine::ParameterType::String: pObj["value"] = QString::fromStdString(param.stringValue); break;
                case CADEngine::ParameterType::Boolean: pObj["value"] = param.boolValue; break;
                default: break;
            }
            params[QString::fromStdString(pname)] = pObj;
        }
        fObj["parameters"] = params;
        
        // Dépendances
        QJsonArray deps;
        for (int d : feature->getDependencies()) deps.append(d);
        fObj["dependencies"] = deps;
        
        // Entités sketch
        if (feature->getTypeName() == "Sketch") {
            auto sketchFeat = std::dynamic_pointer_cast<CADEngine::SketchFeature>(feature);
            if (sketchFeat && sketchFeat->getSketch2D()) {
                QJsonArray entities;
                for (const auto& entity : sketchFeat->getSketch2D()->getEntities()) {
                    if (entity->isConstruction()) continue;
                    QJsonObject eObj;
                    eObj["id"] = entity->getId();
                    
                    if (entity->getType() == CADEngine::SketchEntityType::Line) {
                        auto l = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
                        eObj["type"] = "Line";
                        eObj["x1"] = l->getStart().X(); eObj["y1"] = l->getStart().Y();
                        eObj["x2"] = l->getEnd().X(); eObj["y2"] = l->getEnd().Y();
                    }
                    else if (entity->getType() == CADEngine::SketchEntityType::Circle) {
                        auto c = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
                        eObj["type"] = "Circle";
                        eObj["cx"] = c->getCenter().X(); eObj["cy"] = c->getCenter().Y();
                        eObj["radius"] = c->getRadius();
                    }
                    else if (entity->getType() == CADEngine::SketchEntityType::Arc) {
                        auto a = std::dynamic_pointer_cast<CADEngine::SketchArc>(entity);
                        eObj["type"] = "Arc";
                        eObj["x1"] = a->getStart().X(); eObj["y1"] = a->getStart().Y();
                        eObj["x2"] = a->getEnd().X(); eObj["y2"] = a->getEnd().Y();
                        eObj["mx"] = a->getMid().X(); eObj["my"] = a->getMid().Y();
                        eObj["bezier"] = a->isBezier();
                    }
                    else if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
                        auto r = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
                        eObj["type"] = "Rectangle";
                        eObj["cx"] = r->getCorner().X(); eObj["cy"] = r->getCorner().Y();
                        eObj["width"] = r->getWidth(); eObj["height"] = r->getHeight();
                    }
                    else if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
                        auto p = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
                        eObj["type"] = "Polyline";
                        QJsonArray pts;
                        for (const auto& pt : p->getPoints()) {
                            QJsonObject ptObj; ptObj["x"] = pt.X(); ptObj["y"] = pt.Y();
                            pts.append(ptObj);
                        }
                        eObj["points"] = pts;
                    }
                    else if (entity->getType() == CADEngine::SketchEntityType::Ellipse) {
                        auto e = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
                        eObj["type"] = "Ellipse";
                        eObj["cx"] = e->getCenter().X(); eObj["cy"] = e->getCenter().Y();
                        eObj["majorRadius"] = e->getMajorRadius(); eObj["minorRadius"] = e->getMinorRadius();
                        eObj["angle"] = e->getRotation();
                    }
                    else if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
                        auto p = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
                        eObj["type"] = "Polygon";
                        eObj["cx"] = p->getCenter().X(); eObj["cy"] = p->getCenter().Y();
                        eObj["radius"] = p->getRadius(); eObj["sides"] = p->getSides();
                        eObj["angle"] = p->getRotation();
                    }
                    else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
                        auto o = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
                        eObj["type"] = "Oblong";
                        eObj["cx"] = o->getCenter().X(); eObj["cy"] = o->getCenter().Y();
                        eObj["length"] = o->getLength(); eObj["width"] = o->getWidth();
                        eObj["angle"] = o->getRotation();
                    }
                    entities.append(eObj);
                }
                fObj["entities"] = entities;
                
                // Build index map: entity pointer → index in saved array (excluding construction)
                std::map<std::shared_ptr<CADEngine::SketchEntity>, int> entityIndexMap;
                int idx = 0;
                for (const auto& entity : sketchFeat->getSketch2D()->getEntities()) {
                    if (entity->isConstruction()) continue;
                    entityIndexMap[entity] = idx++;
                }
                // Construction lines: axisH = -1, axisV = -2
                auto axH = sketchFeat->getSketch2D()->getAxisLineH();
                auto axV = sketchFeat->getSketch2D()->getAxisLineV();
                if (axH) entityIndexMap[axH] = -1;
                if (axV) entityIndexMap[axV] = -2;
                
                // Sauvegarder les contraintes
                QJsonArray constraintsArr;
                for (const auto& c : sketchFeat->getSketch2D()->getConstraints()) {
                    QJsonObject cObj;
                    cObj["type"] = QString::fromStdString(c->getDescription());
                    
                    if (c->getType() == CADEngine::ConstraintType::CenterOnAxis) {
                        auto coa = std::dynamic_pointer_cast<CADEngine::CenterOnAxisConstraint>(c);
                        if (coa && coa->getEntity() && coa->getAxisLine()) {
                            auto it1 = entityIndexMap.find(coa->getEntity());
                            auto it2 = entityIndexMap.find(std::dynamic_pointer_cast<CADEngine::SketchEntity>(coa->getAxisLine()));
                            if (it1 != entityIndexMap.end() && it2 != entityIndexMap.end()) {
                                cObj["entityIdx"] = it1->second;
                                cObj["axisIdx"] = it2->second;
                                constraintsArr.append(cObj);
                            }
                        }
                    }
                }
                fObj["constraints"] = constraintsArr;
            }
        }
        
        // === Sauvegarder les arêtes sélectionnées (Fillet/Chamfer) par midpoints ===
        if (feature->getTypeName() == "Fillet3D") {
            auto fillet = std::dynamic_pointer_cast<CADEngine::Fillet3DFeature>(feature);
            if (fillet && !fillet->isAllEdges() && !fillet->getSelectedEdges().empty()) {
                QJsonArray edgeMidpoints;
                for (const auto& edge : fillet->getSelectedEdges()) {
                    try {
                        double first, last;
                        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
                        if (!curve.IsNull()) {
                            gp_Pnt midPt = curve->Value((first + last) / 2.0);
                            QJsonObject pt;
                            pt["x"] = midPt.X(); pt["y"] = midPt.Y(); pt["z"] = midPt.Z();
                            edgeMidpoints.append(pt);
                        }
                    } catch (...) {}
                }
                fObj["edge_midpoints"] = edgeMidpoints;
            }
        }
        if (feature->getTypeName() == "Chamfer3D") {
            auto chamfer = std::dynamic_pointer_cast<CADEngine::Chamfer3DFeature>(feature);
            if (chamfer && !chamfer->isAllEdges() && !chamfer->getSelectedEdges().empty()) {
                QJsonArray edgeMidpoints;
                for (const auto& edge : chamfer->getSelectedEdges()) {
                    try {
                        double first, last;
                        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
                        if (!curve.IsNull()) {
                            gp_Pnt midPt = curve->Value((first + last) / 2.0);
                            QJsonObject pt;
                            pt["x"] = midPt.X(); pt["y"] = midPt.Y(); pt["z"] = midPt.Z();
                            edgeMidpoints.append(pt);
                        }
                    } catch (...) {}
                }
                fObj["edge_midpoints"] = edgeMidpoints;
            }
        }
        
        // === Sauvegarder le centre + normale des faces ouvertes (Shell) ===
        if (feature->getTypeName() == "Shell") {
            auto shell = std::dynamic_pointer_cast<CADEngine::ShellFeature>(feature);
            if (shell && !shell->getOpenFaces().empty()) {
                QJsonArray faceCenters;
                for (const auto& face : shell->getOpenFaces()) {
                    try {
                        GProp_GProps props;
                        BRepGProp::SurfaceProperties(face, props);
                        gp_Pnt center = props.CentreOfMass();
                        
                        BRepAdaptor_Surface adaptor(face);
                        double uMid = (adaptor.FirstUParameter() + adaptor.LastUParameter()) / 2.0;
                        double vMid = (adaptor.FirstVParameter() + adaptor.LastVParameter()) / 2.0;
                        gp_Pnt pt; gp_Vec du, dv;
                        adaptor.D1(uMid, vMid, pt, du, dv);
                        gp_Vec n = du.Crossed(dv);
                        if (n.Magnitude() > 1e-10) n.Normalize();
                        if (face.Orientation() == TopAbs_REVERSED) n.Reverse();
                        
                        QJsonObject fc;
                        fc["cx"] = center.X(); fc["cy"] = center.Y(); fc["cz"] = center.Z();
                        fc["nx"] = n.X(); fc["ny"] = n.Y(); fc["nz"] = n.Z();
                        faceCenters.append(fc);
                    } catch (...) {}
                }
                fObj["open_face_centers"] = faceCenters;
            }
        }
        
        // === Sauvegarder centre + normale pour PushPull ===
        if (feature->getTypeName() == "PushPull") {
            auto pp = std::dynamic_pointer_cast<CADEngine::PushPullFeature>(feature);
            if (pp) {
                // La normale est déjà sauvée dans les paramètres (normal_x/y/z)
                // On n'a rien de plus à faire
            }
        }
        
        // === Sauvegarder les dépendances Sweep (profil + chemin) ===
        if (feature->getTypeName() == "Sweep") {
            auto sweep = std::dynamic_pointer_cast<CADEngine::SweepFeature>(feature);
            if (sweep) {
                QJsonArray deps;
                if (sweep->getProfileSketch()) deps.append(sweep->getProfileSketch()->getId());
                if (sweep->getPathSketch()) deps.append(sweep->getPathSketch()->getId());
                fObj["dependencies"] = deps;
            }
        }
        
        // === Sauvegarder les dépendances Loft (profils) ===
        if (feature->getTypeName() == "Loft") {
            auto loft = std::dynamic_pointer_cast<CADEngine::LoftFeature>(feature);
            if (loft) {
                QJsonArray deps;
                for (const auto& sk : loft->getProfileSketches()) {
                    if (sk) deps.append(sk->getId());
                }
                fObj["dependencies"] = deps;
            }
        }
        
        featuresArr.append(fObj);
    }
    root["features"] = featuresArr;
    
    QJsonDocument jsonDoc(root);
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(jsonDoc.toJson(QJsonDocument::Indented));
        file.close();
        m_document->setModified(false);
        setWindowTitle(QString("CAD-ENGINE - %1").arg(QFileInfo(filename).baseName()));
        onStatusMessage(QString("Enregistré: %1").arg(filename));
    } else {
        QMessageBox::warning(this, "Erreur", "Impossible d'enregistrer le fichier.");
    }
}

void MainWindow::onExportSTEP() {
    QString filename = QFileDialog::getSaveFileName(
        this, "Exporter STEP", "", "STEP Files (*.step *.stp)");
    
    if (!filename.isEmpty()) {
        // TODO: Implémenter exportSTEP
        // if (m_document->exportSTEP(filename.toStdString())) {
        //     onStatusMessage("Exporté: " + filename);
        // } else {
        //     onStatusMessage("Erreur export STEP");
        // }
        onStatusMessage("Export STEP: À implémenter");
        QMessageBox::information(this, "Info", "Fonction export STEP à implémenter");
    }
}

void MainWindow::onExportSTL() {
    // Collecter toutes les shapes 3D du document
    auto features = m_document->getAllFeatures();
    
    // Trouver la dernière shape valide (résultat final de l'arbre)
    TopoDS_Shape finalShape;
    for (auto it = features.rbegin(); it != features.rend(); ++it) {
        auto shape = (*it)->getShape();
        if (!shape.IsNull() && shape.ShapeType() <= TopAbs_SOLID) {
            finalShape = shape;
            break;
        }
    }
    
    if (finalShape.IsNull()) {
        // Si pas de solid, essayer de faire un compound de tout
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        bool hasShape = false;
        
        for (const auto& feat : features) {
            auto shape = feat->getShape();
            if (!shape.IsNull()) {
                builder.Add(compound, shape);
                hasShape = true;
            }
        }
        
        if (!hasShape) {
            QMessageBox::warning(this, "Erreur", 
                "Aucune géométrie 3D à exporter.\nCréez d'abord une extrusion, révolution ou autre opération 3D.");
            return;
        }
        finalShape = compound;
    }
    
    // Dialogue de sauvegarde
    QString filename = QFileDialog::getSaveFileName(
        this, "Exporter STL", "", "Fichiers STL (*.stl)");
    
    if (filename.isEmpty()) return;
    
    // Assurer l'extension .stl
    if (!filename.toLower().endsWith(".stl"))
        filename += ".stl";
    
    // Maillage de la shape (tessellation)
    // Paramètre de déviation linéaire : plus petit = plus précis mais plus lourd
    double linearDeflection = 0.1;   // 0.1mm - bonne qualité pour impression 3D
    double angularDeflection = 0.5;  // 0.5 radians (~28°)
    
    BRepMesh_IncrementalMesh mesher(finalShape, linearDeflection, false, angularDeflection);
    mesher.Perform();
    
    if (!mesher.IsDone()) {
        QMessageBox::warning(this, "Erreur", "Échec du maillage de la géométrie.");
        return;
    }
    
    // Écriture STL binaire manuelle (pas besoin de TKStl)
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Erreur", 
            QString("Impossible de créer le fichier:\n%1").arg(filename));
        return;
    }
    
    // Compter les triangles d'abord
    int totalTriangles = 0;
    for (TopExp_Explorer exp(finalShape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        TopLoc_Location loc;
        auto tri = BRep_Tool::Triangulation(face, loc);
        if (!tri.IsNull()) {
            totalTriangles += tri->NbTriangles();
        }
    }
    
    // Header STL binaire : 80 octets
    char header[80];
    memset(header, 0, 80);
    snprintf(header, 80, "CAD-ENGINE STL Binary - %d triangles", totalTriangles);
    file.write(header, 80);
    
    // Nombre de triangles : uint32
    uint32_t triCount = static_cast<uint32_t>(totalTriangles);
    file.write(reinterpret_cast<const char*>(&triCount), 4);
    
    // Écrire chaque triangle : 12 floats (normal + 3 vertices) + 2 bytes attribut = 50 bytes
    for (TopExp_Explorer exp(finalShape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        TopLoc_Location loc;
        auto triangulation = BRep_Tool::Triangulation(face, loc);
        if (triangulation.IsNull()) continue;
        
        gp_Trsf transform;
        if (!loc.IsIdentity()) transform = loc.Transformation();
        bool hasTransform = !loc.IsIdentity();
        
        // Orientation de la face (normal inversée si reversed)
        bool reversed = (face.Orientation() == TopAbs_REVERSED);
        
        for (int i = 1; i <= triangulation->NbTriangles(); i++) {
            const Poly_Triangle& tri = triangulation->Triangle(i);
            int n1, n2, n3;
            tri.Get(n1, n2, n3);
            
            // Inverser l'ordre si face reversed (pour que la normale pointe vers l'extérieur)
            if (reversed) std::swap(n2, n3);
            
            gp_Pnt p1 = triangulation->Node(n1);
            gp_Pnt p2 = triangulation->Node(n2);
            gp_Pnt p3 = triangulation->Node(n3);
            
            if (hasTransform) {
                p1.Transform(transform);
                p2.Transform(transform);
                p3.Transform(transform);
            }
            
            // Calculer la normale du triangle
            gp_Vec v1(p1, p2);
            gp_Vec v2(p1, p3);
            gp_Vec normal = v1.Crossed(v2);
            if (normal.Magnitude() > 1e-10) normal.Normalize();
            
            // Normal (3 floats)
            float nx = static_cast<float>(normal.X());
            float ny = static_cast<float>(normal.Y());
            float nz = static_cast<float>(normal.Z());
            file.write(reinterpret_cast<const char*>(&nx), 4);
            file.write(reinterpret_cast<const char*>(&ny), 4);
            file.write(reinterpret_cast<const char*>(&nz), 4);
            
            // Vertex 1 (3 floats)
            float x, y, z;
            x = static_cast<float>(p1.X()); y = static_cast<float>(p1.Y()); z = static_cast<float>(p1.Z());
            file.write(reinterpret_cast<const char*>(&x), 4);
            file.write(reinterpret_cast<const char*>(&y), 4);
            file.write(reinterpret_cast<const char*>(&z), 4);
            
            // Vertex 2 (3 floats)
            x = static_cast<float>(p2.X()); y = static_cast<float>(p2.Y()); z = static_cast<float>(p2.Z());
            file.write(reinterpret_cast<const char*>(&x), 4);
            file.write(reinterpret_cast<const char*>(&y), 4);
            file.write(reinterpret_cast<const char*>(&z), 4);
            
            // Vertex 3 (3 floats)
            x = static_cast<float>(p3.X()); y = static_cast<float>(p3.Y()); z = static_cast<float>(p3.Z());
            file.write(reinterpret_cast<const char*>(&x), 4);
            file.write(reinterpret_cast<const char*>(&y), 4);
            file.write(reinterpret_cast<const char*>(&z), 4);
            
            // Attribut byte count (2 bytes, toujours 0)
            uint16_t attr = 0;
            file.write(reinterpret_cast<const char*>(&attr), 2);
        }
    }
    
    file.close();
    
    bool success = (file.error() == QFile::NoError);
    
    if (success) {
        QFileInfo fi(filename);
        double sizeMB = fi.size() / (1024.0 * 1024.0);
        
        onStatusMessage(QString("STL exporté: %1 (%2 triangles, %3 MB)")
            .arg(filename)
            .arg(totalTriangles)
            .arg(sizeMB, 0, 'f', 2));
        QMessageBox::information(this, "Export STL", 
            QString("Export STL binaire réussi !\n\n"
                    "Fichier: %1\n"
                    "Triangles: %2\n"
                    "Taille: %3 MB\n\n"
                    "Prêt pour Cura / PrusaSlicer / etc.")
            .arg(QFileInfo(filename).fileName())
            .arg(totalTriangles)
            .arg(sizeMB, 0, 'f', 2));
    } else {
        QMessageBox::warning(this, "Erreur", 
            QString("Échec de l'écriture du fichier STL:\n%1").arg(filename));
    }
}

// ============================================================================
// Slots - Create
// ============================================================================

void MainWindow::onCreateSketch() {
    // Dialogue de sélection du plan
    QStringList planes;
    planes << "XY (Horizontal)" << "XZ (Frontal)" << "YZ (Latéral)";
    
    bool ok;
    QString selected = QInputDialog::getItem(this, "Nouveau Sketch",
        "Plan de travail :", planes, 0, false, &ok);
    
    if (!ok) return;
    
    auto sketch = m_document->addFeature<CADEngine::SketchFeature>("Sketch");
    
    if (selected.startsWith("XY")) {
        sketch->setPlaneXY(0.0);
    } else if (selected.startsWith("XZ")) {
        sketch->setPlaneZX(0.0);
    } else {
        sketch->setPlaneYZ(0.0);
    }
    
    m_viewport->enterSketchMode(sketch);
    m_tree->refresh();
    updateToolbars();
}

void MainWindow::onFinishSketch() {
    if (m_viewport->getMode() == ViewMode::SKETCH_2D) {
        auto sketch = m_viewport->getActiveSketch();
        if (sketch) {
            // Recalculer le sketch
            sketch->compute();
            
            int count = sketch->getEntityCount();
            onStatusMessage(QString("Sketch terminé - %1 entités").arg(count));
        }
        
        m_viewport->exitSketchMode();
        m_tree->refresh();  // Mettre à jour l'arbre
        updateToolbars();
    }
}

// ============================================================================
// Slots - Sketch Tools
// ============================================================================

void MainWindow::onToolSelect() {
    m_actToolSelect->setChecked(true);
    m_actToolLine->setChecked(false);
    m_actToolCircle->setChecked(false);
    m_actToolRectangle->setChecked(false);
    m_actToolArc->setChecked(false);
    m_actToolPolyline->setChecked(false);
    m_actToolEllipse->setChecked(false);
    m_actToolPolygon->setChecked(false);
    m_actToolArcCenter->setChecked(false);
    m_actToolOblong->setChecked(false);
    m_actToolDimension->setChecked(false);
    m_viewport->setSketchTool(SketchTool::None);
}

void MainWindow::onToolLine() {
    m_actToolSelect->setChecked(false);
    m_actToolLine->setChecked(true);
    m_actToolCircle->setChecked(false);
    m_actToolRectangle->setChecked(false);
    m_actToolArc->setChecked(false);
    m_actToolPolyline->setChecked(false);
    m_actToolEllipse->setChecked(false);
    m_actToolPolygon->setChecked(false);
    m_actToolArcCenter->setChecked(false);
    m_actToolOblong->setChecked(false);
    m_actToolDimension->setChecked(false);
    m_viewport->setSketchTool(SketchTool::Line);
}

void MainWindow::onToolCircle() {
    m_actToolSelect->setChecked(false);
    m_actToolLine->setChecked(false);
    m_actToolCircle->setChecked(true);
    m_actToolRectangle->setChecked(false);
    m_actToolArc->setChecked(false);
    m_actToolPolyline->setChecked(false);
    m_actToolEllipse->setChecked(false);
    m_actToolPolygon->setChecked(false);
    m_actToolArcCenter->setChecked(false);
    m_actToolOblong->setChecked(false);
    m_actToolDimension->setChecked(false);
    m_viewport->setSketchTool(SketchTool::Circle);
}

void MainWindow::onToolRectangle() {
    m_actToolSelect->setChecked(false);
    m_actToolLine->setChecked(false);
    m_actToolCircle->setChecked(false);
    m_actToolRectangle->setChecked(true);
    m_actToolArc->setChecked(false);
    m_actToolPolyline->setChecked(false);
    m_actToolEllipse->setChecked(false);
    m_actToolPolygon->setChecked(false);
    m_actToolArcCenter->setChecked(false);
    m_actToolOblong->setChecked(false);
    m_actToolDimension->setChecked(false);
    m_viewport->setSketchTool(SketchTool::Rectangle);
}

void MainWindow::onToolArc() {
    m_actToolSelect->setChecked(false);
    m_actToolLine->setChecked(false);
    m_actToolCircle->setChecked(false);
    m_actToolRectangle->setChecked(false);
    m_actToolArc->setChecked(true);
    m_actToolPolyline->setChecked(false);
    m_actToolEllipse->setChecked(false);
    m_actToolPolygon->setChecked(false);
    m_actToolArcCenter->setChecked(false);
    m_actToolOblong->setChecked(false);
    m_actToolDimension->setChecked(false);
    m_viewport->setSketchTool(SketchTool::Arc);
}

void MainWindow::onToolPolyline() {
    m_actToolSelect->setChecked(false);
    m_actToolLine->setChecked(false);
    m_actToolCircle->setChecked(false);
    m_actToolRectangle->setChecked(false);
    m_actToolArc->setChecked(false);
    m_actToolPolyline->setChecked(true);
    m_actToolEllipse->setChecked(false);
    m_actToolPolygon->setChecked(false);
    m_actToolArcCenter->setChecked(false);
    m_actToolOblong->setChecked(false);
    m_actToolDimension->setChecked(false);
    m_viewport->setSketchTool(SketchTool::Polyline);
}

void MainWindow::onToolEllipse() {
    m_actToolSelect->setChecked(false);
    m_actToolLine->setChecked(false);
    m_actToolCircle->setChecked(false);
    m_actToolRectangle->setChecked(false);
    m_actToolArc->setChecked(false);
    m_actToolPolyline->setChecked(false);
    m_actToolEllipse->setChecked(true);
    m_actToolPolygon->setChecked(false);
    m_actToolArcCenter->setChecked(false);
    m_actToolOblong->setChecked(false);
    m_actToolDimension->setChecked(false);
    m_viewport->setSketchTool(SketchTool::Ellipse);
}

void MainWindow::onToolPolygon() {
    m_actToolSelect->setChecked(false);
    m_actToolLine->setChecked(false);
    m_actToolCircle->setChecked(false);
    m_actToolRectangle->setChecked(false);
    m_actToolArc->setChecked(false);
    m_actToolPolyline->setChecked(false);
    m_actToolEllipse->setChecked(false);
    m_actToolPolygon->setChecked(true);
    m_actToolArcCenter->setChecked(false);
    m_actToolOblong->setChecked(false);
    m_actToolDimension->setChecked(false);
    m_viewport->setSketchTool(SketchTool::Polygon);
}

void MainWindow::onToolArcCenter() {
    m_actToolSelect->setChecked(false);
    m_actToolLine->setChecked(false);
    m_actToolCircle->setChecked(false);
    m_actToolRectangle->setChecked(false);
    m_actToolArc->setChecked(false);
    m_actToolPolyline->setChecked(false);
    m_actToolEllipse->setChecked(false);
    m_actToolPolygon->setChecked(false);
    m_actToolArcCenter->setChecked(true);
    m_actToolOblong->setChecked(false);
    m_actToolDimension->setChecked(false);
    m_viewport->setSketchTool(SketchTool::ArcCenter);
}

void MainWindow::onToolOblong() {
    m_actToolSelect->setChecked(false);
    m_actToolLine->setChecked(false);
    m_actToolCircle->setChecked(false);
    m_actToolRectangle->setChecked(false);
    m_actToolArc->setChecked(false);
    m_actToolPolyline->setChecked(false);
    m_actToolEllipse->setChecked(false);
    m_actToolPolygon->setChecked(false);
    m_actToolArcCenter->setChecked(false);
    m_actToolOblong->setChecked(true);
    m_actToolDimension->setChecked(false);
    m_viewport->setSketchTool(SketchTool::Oblong);
}

void MainWindow::onToolDimension() {
    m_actToolSelect->setChecked(false);
    m_actToolLine->setChecked(false);
    m_actToolCircle->setChecked(false);
    m_actToolRectangle->setChecked(false);
    m_actToolArc->setChecked(false);
    m_actToolPolyline->setChecked(false);
    m_actToolEllipse->setChecked(false);
    m_actToolPolygon->setChecked(false);
    m_actToolArcCenter->setChecked(false);
    m_actToolOblong->setChecked(false);
    m_actToolDimension->setChecked(true);
    m_viewport->setSketchTool(SketchTool::Dimension);
}

// ============================================================================
// Slots - View
// ============================================================================

void MainWindow::onViewFront() {
    m_viewport->setView(0.0f, 0.0f);
    onStatusMessage("Vue Avant");
}

void MainWindow::onViewTop() {
    m_viewport->setView(-89.0f, 0.0f);
    onStatusMessage("Vue Dessus");
}

void MainWindow::onViewRight() {
    m_viewport->setView(0.0f, -90.0f);
    onStatusMessage("Vue Droite");
}

void MainWindow::onViewIso() {
    m_viewport->setView(30.0f, 45.0f);
    onStatusMessage("Vue Isométrique");
}

// ============================================================================
// Slots - Viewport
// ============================================================================

void MainWindow::onModeChanged(ViewMode mode) {
    updateToolbars();
    
    if (mode == ViewMode::SKETCH_2D) {
        onStatusMessage("Mode Sketch - Sélectionnez un outil");
    } else {
        onStatusMessage("Mode 3D");
    }
}

void MainWindow::onSketchEntityAdded() {
    auto sketch = m_viewport->getActiveSketch();
    if (sketch) {
        int count = sketch->getEntityCount();
        m_statusLabel->setText(QString("Entités: %1").arg(count));
        m_tree->refresh();  // Mettre à jour compteur dans l'arbre
        m_viewport->update();  // ← FORCER REDRAW !
    }
}

void MainWindow::onEntityCreated(std::shared_ptr<CADEngine::SketchEntity> entity, 
                                  std::shared_ptr<CADEngine::Sketch2D> sketch) {
    // Créer commande pour ajouter l'entité (pour undo/redo)
    auto command = std::make_shared<CADEngine::AddEntityCommand>(sketch, entity);
    executeCommand(command);
    
    // Mettre à jour UI
    m_tree->refresh();
    m_viewport->update();
    
    // Dialogue paramètres pour Polygone
    if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
        showPolygonDialog(std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity));
    }
    // Dialogue paramètres pour Oblong
    else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
        showOblongDialog(std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity));
    }
}

void MainWindow::onDimensionCreated(std::shared_ptr<CADEngine::Dimension> dimension, 
                                     std::shared_ptr<CADEngine::Sketch2D> sketch) {
    // Créer commande pour ajouter la dimension (pour undo/redo)
    auto command = std::make_shared<CADEngine::AddDimensionCommand>(sketch, dimension);
    executeCommand(command);
    
    // Mettre à jour UI
    m_viewport->update();
}

void MainWindow::onStatusMessage(const QString& message) {
    m_statusLabel->setText(message);
}

void MainWindow::onToolChanged(SketchTool tool) {
    // Mettre à jour les boutons de la toolbar selon l'outil actif
    m_actToolSelect->setChecked(tool == SketchTool::None);
    m_actToolLine->setChecked(tool == SketchTool::Line);
    m_actToolCircle->setChecked(tool == SketchTool::Circle);
    m_actToolRectangle->setChecked(tool == SketchTool::Rectangle);
    m_actToolArc->setChecked(tool == SketchTool::Arc);
    m_actToolPolyline->setChecked(tool == SketchTool::Polyline);
    m_actToolEllipse->setChecked(tool == SketchTool::Ellipse);
    m_actToolPolygon->setChecked(tool == SketchTool::Polygon);
    m_actToolArcCenter->setChecked(tool == SketchTool::ArcCenter);
    m_actToolOblong->setChecked(tool == SketchTool::Oblong);
    m_actToolDimension->setChecked(tool == SketchTool::Dimension);
}

void MainWindow::onDimensionClicked(std::shared_ptr<CADEngine::Dimension> dimension) {
    if (!dimension) return;
    
    // === AUTO-DIMENSIONS : modifier la géométrie de l'entité source ===
    if (dimension->isAutomatic() && dimension->getAutoSourceEntity()) {
        auto entity = dimension->getAutoSourceEntity();
        std::string prop = dimension->getAutoSourceProperty();
        double currentValue = dimension->getValue();
        
        // Déterminer le label du dialog selon la propriété
        QString label;
        if (prop == "diameter")       label = QString("Diamètre actuel: Ø%1 mm\nNouveau diamètre:").arg(currentValue, 0, 'f', 2);
        else if (prop == "arcRadius") label = QString("Rayon actuel: R%1 mm\nNouveau rayon:").arg(currentValue, 0, 'f', 2);
        else if (prop == "width")     label = QString("Largeur actuelle: %1 mm\nNouvelle largeur:").arg(currentValue, 0, 'f', 2);
        else if (prop == "height")    label = QString("Hauteur actuelle: %1 mm\nNouvelle hauteur:").arg(currentValue, 0, 'f', 2);
        else if (prop == "length")    label = QString("Longueur actuelle: %1 mm\nNouvelle longueur:").arg(currentValue, 0, 'f', 2);
        else if (prop == "entraxe")   label = QString("Entraxe actuel: %1 mm\nNouvel entraxe:").arg(currentValue, 0, 'f', 2);
        else if (prop == "oblongWidth") label = QString("Largeur actuelle: %1 mm\nNouvelle largeur:").arg(currentValue, 0, 'f', 2);
        else if (prop == "majorDiameter") label = QString("Grand diamètre: %1 mm\nNouveau:").arg(currentValue, 0, 'f', 2);
        else if (prop == "minorDiameter") label = QString("Petit diamètre: %1 mm\nNouveau:").arg(currentValue, 0, 'f', 2);
        else if (prop == "sideLength")    label = QString("Côté actuel: %1 mm\nNouveau côté:").arg(currentValue, 0, 'f', 2);
        else if (prop == "segment")       label = QString("Segment: %1 mm\nNouvelle longueur:").arg(currentValue, 0, 'f', 2);
        else label = QString("Valeur actuelle: %1 mm\nNouvelle valeur:").arg(currentValue, 0, 'f', 2);
        
        bool ok;
        double newValue = QInputDialog::getDouble(this, "Éditer Dimension", label,
            currentValue, 0.01, 100000.0, 2, &ok);
        
        if (!ok || std::abs(newValue - currentValue) < 1e-6) return;
        
        // Appliquer la modification à l'entité source
        bool applied = false;
        
        // --- CIRCLE : diamètre ---
        if (prop == "diameter") {
            auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
            if (circle) {
                circle->setRadius(newValue / 2.0);
                applied = true;
            }
        }
        // --- LINE : longueur (déplacer p2 le long de la direction) ---
        else if (prop == "length") {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
            if (line) {
                gp_Pnt2d p1 = line->getStart(), p2 = line->getEnd();
                double dx = p2.X() - p1.X(), dy = p2.Y() - p1.Y();
                double len = std::sqrt(dx*dx + dy*dy);
                if (len > 1e-6) {
                    double dirX = dx/len, dirY = dy/len;
                    line->setEnd(gp_Pnt2d(p1.X() + dirX * newValue, p1.Y() + dirY * newValue));
                }
                applied = true;
            }
        }
        // --- RECTANGLE : largeur ---
        else if (prop == "width") {
            auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
            if (rect) {
                rect->setWidth(newValue);
                applied = true;
            }
        }
        // --- RECTANGLE : hauteur ---
        else if (prop == "height") {
            auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
            if (rect) {
                rect->setHeight(newValue);
                applied = true;
            }
        }
        // --- OBLONG : entraxe → modifier la longueur totale ---
        else if (prop == "entraxe") {
            auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
            if (oblong) {
                // entraxe = length - width
                double newLength = newValue + oblong->getWidth();
                oblong->setLength(newLength);
                applied = true;
            }
        }
        // --- OBLONG : largeur ---
        else if (prop == "oblongWidth") {
            auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
            if (oblong) {
                // Garder l'entraxe constant → ajuster length aussi
                double entraxe = oblong->getLength() - oblong->getWidth();
                oblong->setWidth(newValue);
                oblong->setLength(entraxe + newValue);
                applied = true;
            }
        }
        // --- ELLIPSE : grand diamètre ---
        else if (prop == "majorDiameter") {
            auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
            if (ellipse) {
                double newMaj = newValue / 2.0;
                if (newMaj > ellipse->getMinorRadius())
                    ellipse->setMajorRadius(newMaj);
                else {
                    ellipse->setMajorRadius(ellipse->getMinorRadius());
                    ellipse->setMinorRadius(newMaj);
                }
                applied = true;
            }
        }
        // --- ELLIPSE : petit diamètre ---
        else if (prop == "minorDiameter") {
            auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
            if (ellipse) {
                double newMin = newValue / 2.0;
                if (newMin < ellipse->getMajorRadius())
                    ellipse->setMinorRadius(newMin);
                else {
                    ellipse->setMinorRadius(ellipse->getMajorRadius());
                    ellipse->setMajorRadius(newMin);
                }
                applied = true;
            }
        }
        // --- POLYGON : longueur côté → modifier le rayon ---
        else if (prop == "sideLength") {
            auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
            if (polygon && polygon->getSides() >= 3) {
                // sideLength = 2 * R * sin(π / N)
                double newRadius = newValue / (2.0 * std::sin(M_PI / polygon->getSides()));
                polygon->setRadius(newRadius);
                applied = true;
            }
        }
        // --- POLYLINE : segment individuel ---
        else if (prop == "segment") {
            auto poly = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
            if (poly) {
                int segIdx = dimension->getSegmentIndex();
                auto points = poly->getPoints();
                if (segIdx >= 0 && segIdx + 1 < (int)points.size()) {
                    double dx = points[segIdx+1].X() - points[segIdx].X();
                    double dy = points[segIdx+1].Y() - points[segIdx].Y();
                    double len = std::sqrt(dx*dx + dy*dy);
                    if (len > 1e-6) {
                        double dirX = dx/len, dirY = dy/len;
                        // Déplacer tous les points après segIdx
                        double delta = newValue - len;
                        for (int i = segIdx + 1; i < (int)points.size(); ++i) {
                            points[i] = gp_Pnt2d(points[i].X() + dirX * delta,
                                                   points[i].Y() + dirY * delta);
                        }
                        poly->setPoints(points);
                    }
                }
                applied = true;
            }
        }
        
        if (applied) {
            // Ré-appliquer les contraintes (symétrie, etc.)
            auto activeSketch = m_viewport->getActiveSketch();
            if (activeSketch && activeSketch->getSketch2D()) {
                activeSketch->getSketch2D()->solveConstraints();
                activeSketch->getSketch2D()->regenerateAutoDimensions();
            }
            m_viewport->update();
            m_statusLabel->setText(QString("Dimension modifiée: %1 → %2 mm")
                .arg(currentValue, 0, 'f', 2).arg(newValue, 0, 'f', 2));
        }
        return;
    }
    
    // === COTATIONS MANUELLES (comportement existant) ===
    if (dimension->getType() == CADEngine::DimensionType::Linear) {
        auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dimension);
        if (!linearDim) return;
        
        double currentValue = linearDim->getValue();
        
        // Dialog pour entrer nouvelle valeur
        bool ok;
        double newValue = QInputDialog::getDouble(
            this,
            "Éditer Cotation",
            QString("Distance actuelle: %1 mm\nNouvelle distance:").arg(currentValue, 0, 'f', 2),
            currentValue,  // Valeur par défaut
            0.01,          // Minimum
            10000.0,       // Maximum
            2,             // Décimales
            &ok
        );
        
        if (ok && std::abs(newValue - currentValue) > 1e-6) {
            // Créer commande pour modifier la dimension (undo/redo)
            auto command = std::make_shared<CADEngine::ModifyDimensionCommand>(
                linearDim, 
                currentValue, 
                newValue,
                [this, linearDim](double value) {
                    // Callback pour appliquer la modification à la géométrie
                    applyDimensionConstraint(linearDim, value);
                }
            );
            executeCommand(command);
            
            // Rafraîchir cotations angulaires (la longueur modifiée peut affecter des angles)
            refreshAllAngularDimensions();
            
            // Mettre à jour UI
            m_viewport->update();
            m_statusLabel->setText(QString("Dimension modifiée: %1 mm → %2 mm")
                .arg(currentValue, 0, 'f', 1).arg(newValue, 0, 'f', 1));
        }
    }
    // Gérer les dimensions diamétrales
    else if (dimension->getType() == CADEngine::DimensionType::Diametral) {
        auto diametralDim = std::dynamic_pointer_cast<CADEngine::DiametralDimension>(dimension);
        if (!diametralDim) return;
        
        double currentDiameter = diametralDim->getValue();
        
        // Dialog pour entrer nouveau diamètre
        bool ok;
        double newDiameter = QInputDialog::getDouble(
            this,
            "Éditer Diamètre",
            QString("Diamètre actuel: Ø%1 mm\nNouveau diamètre:").arg(currentDiameter, 0, 'f', 2),
            currentDiameter,
            0.01,
            10000.0,
            2,
            &ok
        );
        
        if (ok && std::abs(newDiameter - currentDiameter) > 1e-6) {
            // Créer commande pour modifier le diamètre (undo/redo)
            auto command = std::make_shared<CADEngine::ModifyDimensionCommand>(
                diametralDim, 
                currentDiameter, 
                newDiameter,
                [this, diametralDim](double value) {
                    // Callback pour appliquer la modification au cercle
                    applyDiameterConstraint(diametralDim, value);
                }
            );
            executeCommand(command);
            
            // Mettre à jour UI
            m_viewport->update();
            m_statusLabel->setText(QString("Diamètre modifié: Ø%1 mm → Ø%2 mm")
                .arg(currentDiameter, 0, 'f', 1).arg(newDiameter, 0, 'f', 1));
        }
    }
    // Gérer les dimensions angulaires
    else if (dimension->getType() == CADEngine::DimensionType::Angular) {
        auto angularDim = std::dynamic_pointer_cast<CADEngine::AngularDimension>(dimension);
        if (!angularDim) return;
        
        double currentAngle = angularDim->getValue();
        
        // Dialog pour entrer nouvel angle
        bool ok;
        double newAngle = QInputDialog::getDouble(
            this,
            "Éditer Angle",
            QString("Angle actuel: %1°\nNouvel angle:").arg(currentAngle, 0, 'f', 2),
            currentAngle,
            0.01,          // Minimum
            180.0,         // Maximum (angle intérieur)
            2,             // Décimales
            &ok
        );
        
        if (ok && std::abs(newAngle - currentAngle) > 1e-6) {
            auto command = std::make_shared<CADEngine::ModifyDimensionCommand>(
                angularDim, currentAngle, newAngle
            );
            executeCommand(command);
            
            // Rafraîchir TOUTES les cotations (angles adjacents + longueurs affectées)
            refreshAllAngularDimensions();
            
            m_viewport->update();
            m_statusLabel->setText(QString("Angle modifié: %1° → %2°")
                .arg(currentAngle, 0, 'f', 1).arg(newAngle, 0, 'f', 1));
        }
    }
    else {
        QMessageBox::information(this, "Non implémenté", 
            "L'édition de ce type de dimension n'est pas encore implémentée.");
    }
}

void MainWindow::applyDimensionConstraint(std::shared_ptr<CADEngine::LinearDimension> dim, double newValue) {
    // Récupérer les ANCIENS points de la dimension (AVANT modification)
    gp_Pnt2d oldP1 = dim->getPoint1();
    gp_Pnt2d oldP2 = dim->getPoint2();
    
    // Calculer direction
    double dx = oldP2.X() - oldP1.X();
    double dy = oldP2.Y() - oldP1.Y();
    double currentDist = std::sqrt(dx*dx + dy*dy);
    
    if (currentDist < 1e-6) return;
    
    // Normaliser direction
    double dirX = dx / currentDist;
    double dirY = dy / currentDist;
    
    // Calculer nouvelle position de p2
    gp_Pnt2d newP2(oldP1.X() + dirX * newValue, oldP1.Y() + dirY * newValue);
    
    // Mettre à jour la dimension (setPoint2 appelle updateText)
    dim->setPoint2(newP2);
    
    // ==============================================================
    // PRIORITÉ 1: Utiliser les sources entities si disponibles
    // (cross-entity dimensions: point A sur entité 1, point B sur entité 2)
    // On TRANSLATE l'entité entière pour conserver les contraintes
    // NOTE: Si les 2 sources = même entité (ex: rectangle), on saute au matching géométrique
    // ==============================================================
    if (dim->hasSources() && dim->getSourceEntity1() != dim->getSourceEntity2()) {
        auto srcEntity2 = dim->getSourceEntity2();
        int vtx2 = dim->getSourceVtx2();
        
        if (srcEntity2) {
            // Delta de déplacement
            double deltaX = newP2.X() - oldP2.X();
            double deltaY = newP2.Y() - oldP2.Y();
            
            // Translater l'entité ENTIÈRE (pas juste un point)
            if (srcEntity2->getType() == CADEngine::SketchEntityType::Line) {
                auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(srcEntity2);
                if (line) {
                    gp_Pnt2d s = line->getStart(), e = line->getEnd();
                    line->setStart(gp_Pnt2d(s.X() + deltaX, s.Y() + deltaY));
                    line->setEnd(gp_Pnt2d(e.X() + deltaX, e.Y() + deltaY));
                }
            }
            else if (srcEntity2->getType() == CADEngine::SketchEntityType::Circle) {
                auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(srcEntity2);
                if (circle) {
                    gp_Pnt2d c = circle->getCenter();
                    circle->setCenter(gp_Pnt2d(c.X() + deltaX, c.Y() + deltaY));
                }
            }
            else if (srcEntity2->getType() == CADEngine::SketchEntityType::Rectangle) {
                auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(srcEntity2);
                if (rect) {
                    gp_Pnt2d c = rect->getCorner();
                    rect->setCorner(gp_Pnt2d(c.X() + deltaX, c.Y() + deltaY));
                }
            }
            else if (srcEntity2->getType() == CADEngine::SketchEntityType::Polyline) {
                auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(srcEntity2);
                if (polyline) {
                    auto points = polyline->getPoints();
                    for (auto& pt : points) {
                        pt = gp_Pnt2d(pt.X() + deltaX, pt.Y() + deltaY);
                    }
                    polyline->setPoints(points);
                }
            }
            else if (srcEntity2->getType() == CADEngine::SketchEntityType::Ellipse) {
                auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(srcEntity2);
                if (ellipse) {
                    gp_Pnt2d c = ellipse->getCenter();
                    ellipse->setCenter(gp_Pnt2d(c.X() + deltaX, c.Y() + deltaY));
                }
            }
            else if (srcEntity2->getType() == CADEngine::SketchEntityType::Polygon) {
                auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(srcEntity2);
                if (polygon) {
                    gp_Pnt2d c = polygon->getCenter();
                    polygon->setCenter(gp_Pnt2d(c.X() + deltaX, c.Y() + deltaY));
                }
            }
            else if (srcEntity2->getType() == CADEngine::SketchEntityType::Oblong) {
                auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(srcEntity2);
                if (oblong) {
                    gp_Pnt2d c = oblong->getCenter();
                    oblong->setCenter(gp_Pnt2d(c.X() + deltaX, c.Y() + deltaY));
                }
            }
            
            // Rafraîchir toutes les dimensions qui référencent ces entités
            auto sketch = m_viewport->getActiveSketch();
            if (sketch && sketch->getSketch2D()) {
                for (auto& d : sketch->getSketch2D()->getDimensions()) {
                    if (d->getType() == CADEngine::DimensionType::Linear) {
                        auto ld = std::dynamic_pointer_cast<CADEngine::LinearDimension>(d);
                        if (ld && ld->hasSources()) ld->refreshFromSources();
                    }
                    else if (d->getType() == CADEngine::DimensionType::Angular) {
                        auto ad = std::dynamic_pointer_cast<CADEngine::AngularDimension>(d);
                        if (ad) ad->refreshFromSources();
                    }
                }
            }
            
            m_statusLabel->setText(QString("Contrainte cross-entity appliquée: %1 mm").arg(newValue, 0, 'f', 2));
            m_viewport->update();
            return;
        }
    }
    
    // ==============================================================
    // PRIORITÉ 2: Recherche par correspondance géométrique (same-entity)
    // ==============================================================
    auto sketch = m_viewport->getActiveSketch();
    if (sketch) {
        auto sketch2D = sketch->getSketch2D();
        if (sketch2D) {
            double tolerance = 1.0;  // Tolérance 1mm pour trouver l'entité
            
            for (const auto& entity : sketch2D->getEntities()) {
                if (entity->getType() == CADEngine::SketchEntityType::Line) {
                    auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
                    if (!line) continue;
                    
                    gp_Pnt2d start = line->getStart();
                    gp_Pnt2d end = line->getEnd();
                    
                    // Vérifier si cette ligne correspond aux ANCIENS points de la dimension
                    bool match1 = (start.Distance(oldP1) < tolerance && end.Distance(oldP2) < tolerance);
                    bool match2 = (start.Distance(oldP2) < tolerance && end.Distance(oldP1) < tolerance);
                    
                    if (match1) {
                        line->setEnd(newP2);
                        m_statusLabel->setText(QString("Contrainte appliquée: ligne mise à jour → %1 mm").arg(newValue, 0, 'f', 2));
                        m_viewport->update();
                        return;
                    }
                    else if (match2) {
                        line->setStart(newP2);
                        m_statusLabel->setText(QString("Contrainte appliquée: ligne mise à jour → %1 mm").arg(newValue, 0, 'f', 2));
                        m_viewport->update();
                        return;
                    }
                }
                else if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
                    auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
                    if (!rect) continue;
                    
                    gp_Pnt2d corner = rect->getCorner();
                    double width = rect->getWidth();
                    double height = rect->getHeight();
                    
                    // Les 4 coins du rectangle AVANT modification
                    gp_Pnt2d p1Rect = corner;
                    gp_Pnt2d p2Rect(corner.X() + width, corner.Y());
                    gp_Pnt2d p3Rect(corner.X() + width, corner.Y() + height);
                    gp_Pnt2d p4Rect(corner.X(), corner.Y() + height);
                    
                    // Vérifier quel côté correspond à la dimension (ANCIENS points)
                    // Côté bas (p1-p2) = largeur
                    if ((p1Rect.Distance(oldP1) < tolerance && p2Rect.Distance(oldP2) < tolerance) ||
                        (p1Rect.Distance(oldP2) < tolerance && p2Rect.Distance(oldP1) < tolerance)) {
                        rect->setWidth(newValue);
                        
                        // Mettre à jour TOUTES les dimensions du rectangle avec anciens coins
                        updateAllRectangleDimensionsFromOld(rect, sketch2D, p1Rect, p2Rect, p3Rect, p4Rect);
                        
                        m_statusLabel->setText(QString("Rectangle redimensionné (largeur): %1 mm").arg(newValue, 0, 'f', 2));
                        m_viewport->update();
                        return;
                    }
                    // Côté droit (p2-p3) = hauteur
                    else if ((p2Rect.Distance(oldP1) < tolerance && p3Rect.Distance(oldP2) < tolerance) ||
                             (p2Rect.Distance(oldP2) < tolerance && p3Rect.Distance(oldP1) < tolerance)) {
                        rect->setHeight(newValue);
                        
                        // Mettre à jour TOUTES les dimensions du rectangle avec anciens coins
                        updateAllRectangleDimensionsFromOld(rect, sketch2D, p1Rect, p2Rect, p3Rect, p4Rect);
                        
                        m_statusLabel->setText(QString("Rectangle redimensionné (hauteur): %1 mm").arg(newValue, 0, 'f', 2));
                        m_viewport->update();
                        return;
                    }
                    // Côté haut (p3-p4) = largeur
                    else if ((p3Rect.Distance(oldP1) < tolerance && p4Rect.Distance(oldP2) < tolerance) ||
                             (p3Rect.Distance(oldP2) < tolerance && p4Rect.Distance(oldP1) < tolerance)) {
                        rect->setWidth(newValue);
                        
                        // Mettre à jour TOUTES les dimensions du rectangle avec anciens coins
                        updateAllRectangleDimensionsFromOld(rect, sketch2D, p1Rect, p2Rect, p3Rect, p4Rect);
                        
                        m_statusLabel->setText(QString("Rectangle redimensionné (largeur): %1 mm").arg(newValue, 0, 'f', 2));
                        m_viewport->update();
                        return;
                    }
                    // Côté gauche (p4-p1) = hauteur
                    else if ((p4Rect.Distance(oldP1) < tolerance && p1Rect.Distance(oldP2) < tolerance) ||
                             (p4Rect.Distance(oldP2) < tolerance && p1Rect.Distance(oldP1) < tolerance)) {
                        rect->setHeight(newValue);
                        
                        // Mettre à jour TOUTES les dimensions du rectangle avec anciens coins
                        updateAllRectangleDimensionsFromOld(rect, sketch2D, p1Rect, p2Rect, p3Rect, p4Rect);
                        
                        m_statusLabel->setText(QString("Rectangle redimensionné (hauteur): %1 mm").arg(newValue, 0, 'f', 2));
                        m_viewport->update();
                        return;
                    }
                }
                else if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
                    auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
                    if (!polyline) continue;
                    
                    auto points = polyline->getPoints();
                    if (points.size() < 2) continue;
                    
                    // Trouver quel segment correspond à cette dimension
                    for (size_t i = 0; i < points.size() - 1; i++) {
                        gp_Pnt2d segStart = points[i];
                        gp_Pnt2d segEnd = points[i + 1];
                        
                        // Vérifier si cette dimension correspond à ce segment
                        bool match1 = (segStart.Distance(oldP1) < tolerance && segEnd.Distance(oldP2) < tolerance);
                        bool match2 = (segStart.Distance(oldP2) < tolerance && segEnd.Distance(oldP1) < tolerance);
                        
                        if (match1 || match2) {
                            // Trouvé le segment !
                            // Calculer direction du segment
                            double dx = segEnd.X() - segStart.X();
                            double dy = segEnd.Y() - segStart.Y();
                            double currentLen = std::sqrt(dx*dx + dy*dy);
                            
                            if (currentLen < 1e-6) continue;
                            
                            // Normaliser
                            double dirX = dx / currentLen;
                            double dirY = dy / currentLen;
                            
                            // Nouveau point d'extrémité
                            gp_Pnt2d newSegEnd(segStart.X() + dirX * newValue, 
                                              segStart.Y() + dirY * newValue);
                            
                            // Modifier le point dans la polyligne
                            points[i + 1] = newSegEnd;
                            
                            // Mettre à jour la polyligne avec les nouveaux points
                            polyline->setPoints(points);
                            
                            // Mettre à jour TOUTES les dimensions de cette polyligne
                            updateAllPolylineDimensions(polyline, sketch2D);
                            
                            m_statusLabel->setText(QString("Segment %1 de polyligne redimensionné: %2 mm")
                                .arg(i + 1).arg(newValue, 0, 'f', 2));
                            m_viewport->update();
                            return;
                        }
                    }
                }
            }
        }
    }
    
    // Si aucune entité trouvée, juste mettre à jour la dimension
    m_viewport->update();
    m_statusLabel->setText(QString("Cotation mise à jour: %1 mm (entité non liée)").arg(newValue, 0, 'f', 2));
}

void MainWindow::applyDiameterConstraint(std::shared_ptr<CADEngine::DiametralDimension> dim, double newDiameter) {
    // Mettre à jour la dimension
    dim->setValue(newDiameter);
    
    // Trouver le cercle associé
    auto sketch = m_viewport->getActiveSketch();
    if (sketch) {
        auto sketch2D = sketch->getSketch2D();
        if (sketch2D) {
            gp_Pnt2d center = dim->getCenter();
            double tolerance = 1.0;
            
            // Chercher le cercle correspondant
            for (const auto& entity : sketch2D->getEntities()) {
                if (entity->getType() == CADEngine::SketchEntityType::Circle) {
                    auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
                    if (!circle) continue;
                    
                    gp_Pnt2d circleCenter = circle->getCenter();
                    
                    // Vérifier si c'est le bon cercle (centre proche)
                    if (circleCenter.Distance(center) < tolerance) {
                        // Modifier le rayon du cercle
                        double newRadius = newDiameter / 2.0;
                        circle->setRadius(newRadius);
                        
                        m_statusLabel->setText(QString("Cercle redimensionné: Ø%1 mm → Ø%2 mm")
                            .arg(dim->getValue(), 0, 'f', 2)
                            .arg(newDiameter, 0, 'f', 2));
                        m_viewport->update();
                        return;
                    }
                }
            }
        }
    }
    
    // Si cercle non trouvé, juste mettre à jour la dimension
    m_viewport->update();
    m_statusLabel->setText(QString("Diamètre mis à jour: Ø%1 mm (cercle non lié)").arg(newDiameter, 0, 'f', 2));
}

void MainWindow::updateAllRectangleDimensions(std::shared_ptr<CADEngine::SketchRectangle> rect,
                                               std::shared_ptr<CADEngine::Sketch2D> sketch2D) {
    if (!rect || !sketch2D) return;
    
    // Récupérer les nouveaux coins du rectangle
    gp_Pnt2d corner = rect->getCorner();
    double width = rect->getWidth();
    double height = rect->getHeight();
    
    gp_Pnt2d p1 = corner;
    gp_Pnt2d p2(corner.X() + width, corner.Y());
    gp_Pnt2d p3(corner.X() + width, corner.Y() + height);
    gp_Pnt2d p4(corner.X(), corner.Y() + height);
    
    double tolerance = 5.0;  // Tolérance 5mm pour trouver les dimensions
    
    // Parcourir TOUTES les dimensions et mettre à jour celles qui appartiennent à ce rectangle
    for (const auto& dim : sketch2D->getDimensions()) {
        if (dim->getType() != CADEngine::DimensionType::Linear) continue;
        
        auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
        if (!linearDim) continue;
        
        gp_Pnt2d dimP1 = linearDim->getPoint1();
        gp_Pnt2d dimP2 = linearDim->getPoint2();
        
        // Vérifier si cette dimension est sur un des 4 côtés du rectangle
        // Côté bas (p1-p2)
        if ((dimP1.Distance(p1) < tolerance && dimP2.Distance(p2) < tolerance) ||
            (dimP1.Distance(p2) < tolerance && dimP2.Distance(p1) < tolerance)) {
            linearDim->setPoint1(p1);
            linearDim->setPoint2(p2);
        }
        // Côté droit (p2-p3)
        else if ((dimP1.Distance(p2) < tolerance && dimP2.Distance(p3) < tolerance) ||
                 (dimP1.Distance(p3) < tolerance && dimP2.Distance(p2) < tolerance)) {
            linearDim->setPoint1(p2);
            linearDim->setPoint2(p3);
        }
        // Côté haut (p3-p4)
        else if ((dimP1.Distance(p3) < tolerance && dimP2.Distance(p4) < tolerance) ||
                 (dimP1.Distance(p4) < tolerance && dimP2.Distance(p3) < tolerance)) {
            linearDim->setPoint1(p3);
            linearDim->setPoint2(p4);
        }
        // Côté gauche (p4-p1)
        else if ((dimP1.Distance(p4) < tolerance && dimP2.Distance(p1) < tolerance) ||
                 (dimP1.Distance(p1) < tolerance && dimP2.Distance(p4) < tolerance)) {
            linearDim->setPoint1(p4);
            linearDim->setPoint2(p1);
        }
    }
}

void MainWindow::updateAllRectangleDimensionsFromOld(std::shared_ptr<CADEngine::SketchRectangle> rect,
                                                      std::shared_ptr<CADEngine::Sketch2D> sketch2D,
                                                      const gp_Pnt2d& oldP1, const gp_Pnt2d& oldP2,
                                                      const gp_Pnt2d& oldP3, const gp_Pnt2d& oldP4) {
    if (!rect || !sketch2D) return;
    
    // Récupérer les NOUVEAUX coins du rectangle (après modification)
    gp_Pnt2d corner = rect->getCorner();
    double width = rect->getWidth();
    double height = rect->getHeight();
    
    gp_Pnt2d newP1 = corner;
    gp_Pnt2d newP2(corner.X() + width, corner.Y());
    gp_Pnt2d newP3(corner.X() + width, corner.Y() + height);
    gp_Pnt2d newP4(corner.X(), corner.Y() + height);
    
    double tolerance = 15.0;  // Tolérance large basée sur l'alignement
    
    // Parcourir TOUTES les dimensions et mettre à jour par ALIGNEMENT
    for (const auto& dim : sketch2D->getDimensions()) {
        if (dim->getType() != CADEngine::DimensionType::Linear) continue;
        
        auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
        if (!linearDim) continue;
        
        gp_Pnt2d dimP1 = linearDim->getPoint1();
        gp_Pnt2d dimP2 = linearDim->getPoint2();
        
        // Calculer direction de la dimension
        double dimDx = dimP2.X() - dimP1.X();
        double dimDy = dimP2.Y() - dimP1.Y();
        double dimLen = std::sqrt(dimDx*dimDx + dimDy*dimDy);
        
        if (dimLen < 1e-6) continue;
        
        // Vérifier chaque côté PAR ALIGNEMENT
        // Dimension horizontale ?
        if (std::abs(dimDy) < 0.1 * dimLen) {
            // Côté bas : Y proche de oldP1.Y
            double distY1 = std::abs(dimP1.Y() - oldP1.Y());
            double distY2 = std::abs(dimP2.Y() - oldP1.Y());
            if (distY1 < tolerance && distY2 < tolerance) {
                linearDim->setPoint1(newP1);
                linearDim->setPoint2(newP2);
                continue;
            }
            // Côté haut : Y proche de oldP3.Y
            distY1 = std::abs(dimP1.Y() - oldP3.Y());
            distY2 = std::abs(dimP2.Y() - oldP3.Y());
            if (distY1 < tolerance && distY2 < tolerance) {
                linearDim->setPoint1(newP3);
                linearDim->setPoint2(newP4);
                continue;
            }
        }
        // Dimension verticale ?
        else if (std::abs(dimDx) < 0.1 * dimLen) {
            // Côté droit : X proche de oldP2.X
            double distX1 = std::abs(dimP1.X() - oldP2.X());
            double distX2 = std::abs(dimP2.X() - oldP2.X());
            if (distX1 < tolerance && distX2 < tolerance) {
                linearDim->setPoint1(newP2);
                linearDim->setPoint2(newP3);
                continue;
            }
            // Côté gauche : X proche de oldP1.X
            distX1 = std::abs(dimP1.X() - oldP1.X());
            distX2 = std::abs(dimP2.X() - oldP1.X());
            if (distX1 < tolerance && distX2 < tolerance) {
                linearDim->setPoint1(newP4);
                linearDim->setPoint2(newP1);
                continue;
            }
        }
    }
}

void MainWindow::updateAllPolylineDimensions(std::shared_ptr<CADEngine::SketchPolyline> polyline,
                                              std::shared_ptr<CADEngine::Sketch2D> sketch2D) {
    if (!polyline || !sketch2D) return;
    
    auto points = polyline->getPoints();
    if (points.size() < 2) return;
    
    int polylineId = polyline->getId();
    
    // Parcourir toutes les dimensions
    for (const auto& dim : sketch2D->getDimensions()) {
        if (dim->getType() != CADEngine::DimensionType::Linear) continue;
        
        auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
        if (!linearDim) continue;
        
        // Cette dimension appartient-elle à cette polyligne ?
        if (linearDim->getEntityId() == polylineId) {
            int segmentIndex = linearDim->getSegmentIndex();
            
            // Vérifier que l'index est valide
            if (segmentIndex >= 0 && segmentIndex < (int)points.size() - 1) {
                // Mettre à jour avec les nouveaux points du segment
                gp_Pnt2d segStart = points[segmentIndex];
                gp_Pnt2d segEnd = points[segmentIndex + 1];
                
                linearDim->setPoint1(segStart);
                linearDim->setPoint2(segEnd);
            }
        }
    }
}

void MainWindow::onDeleteEntity() {
    if (!m_selectedEntity || !m_viewport->getActiveSketch()) return;
    
    auto sketch = m_viewport->getActiveSketch();
    auto sketch2D = sketch->getSketch2D();
    if (!sketch2D) return;
    
    int entityId = m_selectedEntity->getId();
    
    // Supprimer l'entité
    auto& entities = sketch2D->getEntitiesRef();
    entities.erase(
        std::remove_if(entities.begin(), entities.end(),
            [entityId](const auto& e) { return e->getId() == entityId; }),
        entities.end()
    );
    
    // Supprimer aussi les dimensions associées à cette entité
    auto& dimensions = sketch2D->getDimensionsRef();
    auto deletedEntityPtr = m_selectedEntity;  // Capturer avant reset
    dimensions.erase(
        std::remove_if(dimensions.begin(), dimensions.end(),
            [entityId, &deletedEntityPtr](const auto& d) {
                // Supprimer par entityId classique
                if (d->getEntityId() == entityId) return true;
                
                // Supprimer les cotations linéaires cross-entity qui référencent cette entité
                if (d->getType() == CADEngine::DimensionType::Linear) {
                    auto ld = std::dynamic_pointer_cast<CADEngine::LinearDimension>(d);
                    if (ld && ld->hasSources()) {
                        if (ld->getSourceEntity1() == deletedEntityPtr ||
                            ld->getSourceEntity2() == deletedEntityPtr) {
                            return true;
                        }
                    }
                }
                // Supprimer les cotations angulaires cross-entity
                else if (d->getType() == CADEngine::DimensionType::Angular) {
                    auto ad = std::dynamic_pointer_cast<CADEngine::AngularDimension>(d);
                    if (ad) {
                        if (ad->getSourceEntity1() == deletedEntityPtr ||
                            ad->getSourceEntity2() == deletedEntityPtr) {
                            return true;
                        }
                    }
                }
                return false;
            }),
        dimensions.end()
    );
    
    // Supprimer les contraintes qui référencent cette entité
    auto& constraints = sketch2D->getConstraintsRef();
    auto deletedEntity = m_selectedEntity;  // Capturer avant reset
    constraints.erase(
        std::remove_if(constraints.begin(), constraints.end(),
            [&deletedEntity](const auto& c) {
                if (!c) return true;
                
                // Horizontal / Vertical
                if (c->getType() == CADEngine::ConstraintType::Horizontal) {
                    auto hc = std::dynamic_pointer_cast<CADEngine::HorizontalConstraint>(c);
                    if (hc && hc->referencesEntity(deletedEntity)) return true;
                }
                if (c->getType() == CADEngine::ConstraintType::Vertical) {
                    auto vc = std::dynamic_pointer_cast<CADEngine::VerticalConstraint>(c);
                    if (vc && vc->referencesEntity(deletedEntity)) return true;
                }
                
                // Parallel
                if (c->getType() == CADEngine::ConstraintType::Parallel) {
                    auto pc = std::dynamic_pointer_cast<CADEngine::ParallelConstraint>(c);
                    if (pc && pc->referencesEntity(deletedEntity)) return true;
                }
                
                // Perpendicular
                if (c->getType() == CADEngine::ConstraintType::Perpendicular) {
                    auto pc = std::dynamic_pointer_cast<CADEngine::PerpendicularConstraint>(c);
                    if (pc && pc->referencesEntity(deletedEntity)) return true;
                }
                
                // Coincident
                if (c->getType() == CADEngine::ConstraintType::Coincident) {
                    auto cc = std::dynamic_pointer_cast<CADEngine::CoincidentConstraint>(c);
                    if (cc && cc->referencesEntity(deletedEntity)) return true;
                }
                
                // Concentric
                if (c->getType() == CADEngine::ConstraintType::Concentric) {
                    auto cc = std::dynamic_pointer_cast<CADEngine::ConcentricConstraint>(c);
                    if (cc && cc->referencesEntity(deletedEntity)) return true;
                }
                
                // Fix/Lock
                if (c->getType() == CADEngine::ConstraintType::Fix) {
                    auto lc = std::dynamic_pointer_cast<CADEngine::FixConstraint>(c);
                    if (lc && lc->referencesEntity(deletedEntity)) return true;
                }
                
                // Symmetric
                if (c->getType() == CADEngine::ConstraintType::Symmetric) {
                    auto sc = std::dynamic_pointer_cast<CADEngine::SymmetricConstraint>(c);
                    if (sc && sc->referencesEntity(deletedEntity)) return true;
                }
                
                return false;
            }),
        constraints.end()
    );
    
    m_selectedEntity.reset();
    
    // Nettoyage supplémentaire: supprimer contraintes/dimensions orphelines
    {
        auto& ents = sketch2D->getEntitiesRef();
        auto& cons = sketch2D->getConstraintsRef();
        cons.erase(
            std::remove_if(cons.begin(), cons.end(),
                [&ents](const auto& c) {
                    if (!c) return true;
                    // Vérifier que toutes les entités référencées existent encore
                    // Pour les contraintes à 2 entités (parallel, perp, etc.)
                    int refCount = 0;
                    for (const auto& e : ents) {
                        if (c->getType() == CADEngine::ConstraintType::Parallel) {
                            auto pc = std::dynamic_pointer_cast<CADEngine::ParallelConstraint>(c);
                            if (pc && pc->referencesEntity(e)) refCount++;
                        } else if (c->getType() == CADEngine::ConstraintType::Perpendicular) {
                            auto pc = std::dynamic_pointer_cast<CADEngine::PerpendicularConstraint>(c);
                            if (pc && pc->referencesEntity(e)) refCount++;
                        } else if (c->getType() == CADEngine::ConstraintType::Coincident) {
                            auto cc = std::dynamic_pointer_cast<CADEngine::CoincidentConstraint>(c);
                            if (cc && cc->referencesEntity(e)) refCount++;
                        } else if (c->getType() == CADEngine::ConstraintType::Concentric) {
                            auto cc = std::dynamic_pointer_cast<CADEngine::ConcentricConstraint>(c);
                            if (cc && cc->referencesEntity(e)) refCount++;
                        } else if (c->getType() == CADEngine::ConstraintType::Symmetric) {
                            auto sc = std::dynamic_pointer_cast<CADEngine::SymmetricConstraint>(c);
                            if (sc && sc->referencesEntity(e)) refCount++;
                        }
                    }
                    // Contraintes à 2 entités: besoin de 2 refs, sinon orpheline
                    if (c->getType() == CADEngine::ConstraintType::Parallel ||
                        c->getType() == CADEngine::ConstraintType::Perpendicular ||
                        c->getType() == CADEngine::ConstraintType::Coincident ||
                        c->getType() == CADEngine::ConstraintType::Concentric ||
                        c->getType() == CADEngine::ConstraintType::Symmetric) {
                        return refCount < 2;
                    }
                    return false;
                }),
            cons.end()
        );
        
        // Nettoyer dimensions cross-entity orphelines
        auto& dims = sketch2D->getDimensionsRef();
        dims.erase(
            std::remove_if(dims.begin(), dims.end(),
                [&ents](const auto& d) {
                    if (!d) return true;
                    if (d->getType() == CADEngine::DimensionType::Linear) {
                        auto ld = std::dynamic_pointer_cast<CADEngine::LinearDimension>(d);
                        if (ld && ld->hasSources()) {
                            auto s1 = ld->getSourceEntity1();
                            auto s2 = ld->getSourceEntity2();
                            if (s1 && std::find(ents.begin(), ents.end(), s1) == ents.end()) return true;
                            if (s2 && std::find(ents.begin(), ents.end(), s2) == ents.end()) return true;
                        }
                    }
                    return false;
                }),
            dims.end()
        );
    }
    
    m_viewport->update();
    m_statusLabel->setText("Entité supprimée");
}

void MainWindow::onDeleteDimension() {
    if (!m_selectedDimension || !m_viewport->getActiveSketch()) return;
    
    auto sketch = m_viewport->getActiveSketch();
    auto sketch2D = sketch->getSketch2D();
    if (!sketch2D) return;
    
    int dimId = m_selectedDimension->getId();
    
    // Supprimer la dimension
    auto& dimensions = sketch2D->getDimensionsRef();
    dimensions.erase(
        std::remove_if(dimensions.begin(), dimensions.end(),
            [dimId](const auto& d) { return d->getId() == dimId; }),
        dimensions.end()
    );
    
    m_selectedDimension.reset();
    m_viewport->update();
    m_statusLabel->setText("Cotation supprimée");
}

void MainWindow::refreshAllAngularDimensions() {
    if (!m_viewport->getActiveSketch()) return;
    auto sketch2D = m_viewport->getActiveSketch()->getSketch2D();
    if (!sketch2D) return;
    
    for (const auto& dim : sketch2D->getDimensions()) {
        if (dim->getType() == CADEngine::DimensionType::Angular) {
            auto angDim = std::dynamic_pointer_cast<CADEngine::AngularDimension>(dim);
            if (angDim) angDim->refreshFromSources();
        }
        // Rafraîchir aussi les cotations linéaires liées à des polylines
        else if (dim->getType() == CADEngine::DimensionType::Linear) {
            auto linDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
            if (!linDim || linDim->getEntityId() < 0) continue;
            int segIdx = linDim->getSegmentIndex();
            if (segIdx < 0) continue;
            // Chercher la polyline correspondante
            for (const auto& entity : sketch2D->getEntities()) {
                if (entity->getId() == linDim->getEntityId()) {
                    auto poly = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
                    if (poly) {
                        auto pts = poly->getPoints();
                        if (segIdx < (int)pts.size() - 1) {
                            linDim->setPoint1(pts[segIdx]);
                            linDim->setPoint2(pts[segIdx + 1]);
                        }
                    }
                    break;
                }
            }
        }
    }
}

void MainWindow::onFilletVertex(std::shared_ptr<CADEngine::SketchPolyline> polyline, int vertexIndex) {
    if (!polyline || !m_viewport->getActiveSketch()) return;
    
    auto points = polyline->getPoints();
    int n = (int)points.size();
    
    if (vertexIndex < 1 || vertexIndex >= n - 1) {
        m_statusLabel->setText("Le congé ne peut s'appliquer qu'aux vertices intérieurs !");
        return;
    }
    
    gp_Pnt2d P = points[vertexIndex - 1];  // Précédent
    gp_Pnt2d V = points[vertexIndex];       // Vertex (coin)
    gp_Pnt2d Q = points[vertexIndex + 1];   // Suivant
    
    // Vecteurs du coin
    double dx1 = P.X() - V.X(), dy1 = P.Y() - V.Y();
    double dx2 = Q.X() - V.X(), dy2 = Q.Y() - V.Y();
    double len1 = std::sqrt(dx1*dx1 + dy1*dy1);
    double len2 = std::sqrt(dx2*dx2 + dy2*dy2);
    
    if (len1 < 1e-6 || len2 < 1e-6) {
        m_statusLabel->setText("Segments trop courts pour un congé !");
        return;
    }
    
    // Angle entre les deux segments
    double cosAngle = (dx1*dx2 + dy1*dy2) / (len1 * len2);
    cosAngle = std::max(-1.0, std::min(1.0, cosAngle));
    double halfAngle = std::acos(cosAngle) / 2.0;
    
    if (halfAngle < 0.01 || halfAngle > M_PI/2 - 0.01) {
        m_statusLabel->setText("Angle trop petit ou trop obtus pour un congé !");
        return;
    }
    
    // Max rayon possible (ne pas dépasser les segments)
    double maxTrim = std::min(len1, len2) * 0.9;
    double maxRadius = maxTrim * std::tan(halfAngle);
    
    // Demander le rayon
    bool ok;
    double radius = QInputDialog::getDouble(
        this, "Congé",
        QString("Rayon du congé (max: %1 mm):").arg(maxRadius, 0, 'f', 1),
        std::min(5.0, maxRadius),  // Valeur par défaut
        0.1, maxRadius, 2, &ok
    );
    
    if (!ok) {
        m_viewport->setSketchTool(SketchTool::None);
        return;
    }
    
    // Calcul du congé
    double trimDist = radius / std::tan(halfAngle);
    
    // Points de tangence
    gp_Pnt2d T1(V.X() + dx1 / len1 * trimDist, V.Y() + dy1 / len1 * trimDist);
    gp_Pnt2d T2(V.X() + dx2 / len2 * trimDist, V.Y() + dy2 / len2 * trimDist);
    
    // Centre de l'arc : à distance R des deux segments
    // Le centre est sur la bissectrice de l'angle, à distance R/sin(halfAngle) du vertex
    double bisX = dx1/len1 + dx2/len2;
    double bisY = dy1/len1 + dy2/len2;
    double bisLen = std::sqrt(bisX*bisX + bisY*bisY);
    if (bisLen < 1e-6) {
        m_statusLabel->setText("Erreur de calcul du congé !");
        return;
    }
    double centerDist = radius / std::sin(halfAngle);
    gp_Pnt2d center(V.X() + bisX/bisLen * centerDist, 
                     V.Y() + bisY/bisLen * centerDist);
    
    // Générer les points de l'arc (T1 → T2 autour de center)
    double startAngle = std::atan2(T1.Y() - center.Y(), T1.X() - center.X());
    double endAngle = std::atan2(T2.Y() - center.Y(), T2.X() - center.X());
    
    // Déterminer le sens de l'arc (toujours le petit arc)
    double angleDiff = endAngle - startAngle;
    while (angleDiff > M_PI) angleDiff -= 2.0 * M_PI;
    while (angleDiff < -M_PI) angleDiff += 2.0 * M_PI;
    
    // Nombre de segments pour l'arc (proportionnel à l'angle)
    int arcSegments = std::max(4, (int)(std::abs(angleDiff) / (M_PI/18)));  // ~10° par segment
    
    // Construire nouvelle liste de points
    std::vector<gp_Pnt2d> newPoints;
    // Points avant le vertex
    for (int i = 0; i < vertexIndex; i++) {
        newPoints.push_back(points[i]);
    }
    
    // Points de l'arc (T1 → T2)
    for (int i = 0; i <= arcSegments; i++) {
        double t = (double)i / arcSegments;
        double a = startAngle + t * angleDiff;
        newPoints.push_back(gp_Pnt2d(center.X() + radius * std::cos(a),
                                      center.Y() + radius * std::sin(a)));
    }
    
    // Points après le vertex
    for (int i = vertexIndex + 1; i < n; i++) {
        newPoints.push_back(points[i]);
    }
    
    polyline->setPoints(newPoints);

    refreshAllAngularDimensions();
    m_viewport->update();
    m_viewport->setSketchTool(SketchTool::None);
    m_statusLabel->setText(QString("Congé R%1 appliqué au vertex %2")
        .arg(radius, 0, 'f', 1).arg(vertexIndex));
}

// ============================================================================
// Helper partagé : calcul géométrique du congé (P→V→Q)
// Retourne false si le congé est impossible
// ============================================================================
static bool computeFilletGeometry(
    const gp_Pnt2d& P, const gp_Pnt2d& V, const gp_Pnt2d& Q,
    double radius,
    gp_Pnt2d& T1, gp_Pnt2d& T2, gp_Pnt2d& arcMid)
{
    double dx1 = P.X()-V.X(), dy1 = P.Y()-V.Y();
    double dx2 = Q.X()-V.X(), dy2 = Q.Y()-V.Y();
    double len1 = std::sqrt(dx1*dx1+dy1*dy1);
    double len2 = std::sqrt(dx2*dx2+dy2*dy2);
    if (len1 < 1e-6 || len2 < 1e-6) return false;

    double cosA = std::max(-1.0, std::min(1.0, (dx1*dx2+dy1*dy2)/(len1*len2)));
    double half = std::acos(cosA) / 2.0;
    if (half < 0.01 || half > M_PI/2.0 - 0.01) return false;

    double trimDist = radius / std::tan(half);
    T1 = gp_Pnt2d(V.X()+dx1/len1*trimDist, V.Y()+dy1/len1*trimDist);
    T2 = gp_Pnt2d(V.X()+dx2/len2*trimDist, V.Y()+dy2/len2*trimDist);

    double bisX = dx1/len1 + dx2/len2;
    double bisY = dy1/len1 + dy2/len2;
    double bisLen = std::sqrt(bisX*bisX+bisY*bisY);
    if (bisLen < 1e-6) return false;

    double centerDist = radius / std::sin(half);
    // arcMid = point du cercle de congé le plus proche de V (sur la bissectrice)
    arcMid = gp_Pnt2d(V.X() + bisX/bisLen*(centerDist-radius),
                      V.Y() + bisY/bisLen*(centerDist-radius));
    return true;
}

static double askFilletRadius(QWidget* parent, const gp_Pnt2d& P, const gp_Pnt2d& V, const gp_Pnt2d& Q)
{
    double dx1=P.X()-V.X(), dy1=P.Y()-V.Y(), dx2=Q.X()-V.X(), dy2=Q.Y()-V.Y();
    double len1=std::sqrt(dx1*dx1+dy1*dy1), len2=std::sqrt(dx2*dx2+dy2*dy2);
    if (len1<1e-6||len2<1e-6) return -1.0;
    double cosA=std::max(-1.0,std::min(1.0,(dx1*dx2+dy1*dy2)/(len1*len2)));
    double half=std::acos(cosA)/2.0;
    if (half<0.01) return -1.0;
    double maxR = std::min(len1,len2)*0.9*std::tan(half);
    bool ok;
    double r = QInputDialog::getDouble(parent, "Congé",
        QString("Rayon du congé (max: %1 mm):").arg(maxR,0,'f',1),
        std::min(5.0,maxR), 0.1, maxR, 2, &ok);
    return ok ? r : -1.0;
}

// ============================================================================
// Congé sur coin de rectangle
// ============================================================================
void MainWindow::onFilletRectCorner(std::shared_ptr<CADEngine::SketchRectangle> rect, int cornerIdx)
{
    auto sketch = m_viewport->getActiveSketch();
    if (!rect || !sketch) return;
    auto sketch2D = sketch->getSketch2D();

    auto corners = rect->getKeyPoints();
    gp_Pnt2d P = corners[(cornerIdx+3)%4];  // coin précédent
    gp_Pnt2d V = corners[cornerIdx];          // coin à arrondir
    gp_Pnt2d Q = corners[(cornerIdx+1)%4];  // coin suivant

    double radius = askFilletRadius(this, P, V, Q);
    if (radius < 0) { m_viewport->setSketchTool(SketchTool::None); return; }

    gp_Pnt2d T1, T2, arcMid;
    if (!computeFilletGeometry(P, V, Q, radius, T1, T2, arcMid)) {
        QMessageBox::warning(this, "Congé", "Impossible de calculer le congé pour ce coin.");
        m_viewport->setSketchTool(SketchTool::None);
        return;
    }

    // Exploser le rectangle en 4 lignes + 1 arc
    // Les 4 côtés du rectangle : side[i] va de corners[i] à corners[(i+1)%4]
    // Le côté (cornerIdx+3)%4 finit en T1 ; le côté cornerIdx commence en T2
    sketch2D->removeEntity(rect);

    for (int i = 0; i < 4; i++) {
        gp_Pnt2d start = corners[i];
        gp_Pnt2d end   = corners[(i+1)%4];
        if (i == (cornerIdx+3)%4) end   = T1;   // côté qui arrive au coin : raccourci
        if (i == cornerIdx)        start = T2;   // côté qui part du coin : raccourci
        auto line = std::make_shared<CADEngine::SketchLine>(start, end);
        sketch2D->addEntity(line);
    }

    // Arc de congé : Bézier quadratique avec V comme point de contrôle
    // → courbe qui part de T1, se dirige vers le coin V, et arrive en T2
    auto arc = std::make_shared<CADEngine::SketchArc>(T1, T2, V, true);
    sketch2D->addEntity(arc);

    sketch2D->regenerateAutoDimensions();
    m_viewport->update();
    m_viewport->setSketchTool(SketchTool::None);
    m_statusLabel->setText(QString("Congé R%1 appliqué au coin %2 du rectangle").arg(radius,0,'f',1).arg(cornerIdx));
}

// ============================================================================
// Congé sur jonction de deux lignes
// ============================================================================
void MainWindow::onFilletLineCorner(
    std::shared_ptr<CADEngine::SketchLine> line1, bool line1AtStart,
    std::shared_ptr<CADEngine::SketchLine> line2, bool line2AtStart)
{
    auto sketch = m_viewport->getActiveSketch();
    if (!line1 || !line2 || !sketch) return;
    auto sketch2D = sketch->getSketch2D();

    // V = coin partagé, P = autre extrémité de line1, Q = autre extrémité de line2
    gp_Pnt2d V = line1AtStart ? line1->getStart() : line1->getEnd();
    gp_Pnt2d P = line1AtStart ? line1->getEnd()   : line1->getStart();
    gp_Pnt2d Q = line2AtStart ? line2->getEnd()   : line2->getStart();

    double radius = askFilletRadius(this, P, V, Q);
    if (radius < 0) { m_viewport->setSketchTool(SketchTool::None); return; }

    gp_Pnt2d T1, T2, arcMid;
    if (!computeFilletGeometry(P, V, Q, radius, T1, T2, arcMid)) {
        QMessageBox::warning(this, "Congé", "Impossible de calculer le congé pour cet angle.");
        m_viewport->setSketchTool(SketchTool::None);
        return;
    }

    // Raccourcir line1 : T1 remplace V dans line1
    if (line1AtStart) line1->setStart(T1); else line1->setEnd(T1);
    // Raccourcir line2 : T2 remplace V dans line2
    if (line2AtStart) line2->setStart(T2); else line2->setEnd(T2);

    // Arc de congé : Bézier avec V comme point de contrôle
    auto arc = std::make_shared<CADEngine::SketchArc>(T1, T2, V, true);
    sketch2D->addEntity(arc);

    sketch2D->regenerateAutoDimensions();
    m_viewport->update();
    m_viewport->setSketchTool(SketchTool::None);
    m_statusLabel->setText(QString("Congé R%1 appliqué à la jonction de lignes").arg(radius,0,'f',1));
}

void MainWindow::onDeletePolylineSegment(int segmentIndex) {
    if (!m_selectedEntity || !m_viewport->getActiveSketch()) return;
    if (m_selectedEntity->getType() != CADEngine::SketchEntityType::Polyline) return;
    
    auto sketch = m_viewport->getActiveSketch();
    auto sketch2D = sketch->getSketch2D();
    if (!sketch2D) return;
    
    auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(m_selectedEntity);
    if (!polyline) return;
    
    auto points = polyline->getPoints();
    int numPoints = (int)points.size();
    int numSegments = numPoints - 1;
    int polylineId = polyline->getId();
    
    if (numSegments <= 1) {
        // Un seul segment → supprimer toute la polyline
        onDeleteEntity();
        return;
    }
    
    if (segmentIndex < 0 || segmentIndex >= numSegments) return;
    
    // === 1. Supprimer les dimensions du segment supprimé ===
    auto& dimensions = sketch2D->getDimensionsRef();
    dimensions.erase(
        std::remove_if(dimensions.begin(), dimensions.end(),
            [polylineId, segmentIndex](const auto& d) {
                if (d->getEntityId() == polylineId) {
                    auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(d);
                    if (linearDim && linearDim->getSegmentIndex() == segmentIndex)
                        return true;
                }
                return false;
            }),
        dimensions.end()
    );
    
    // === 2. Supprimer les contraintes H/V sur ce segment ===
    auto& constraints = sketch2D->getConstraintsRef();
    constraints.erase(
        std::remove_if(constraints.begin(), constraints.end(),
            [&polyline, segmentIndex](const auto& c) {
                if (!c) return true;
                if (c->getType() == CADEngine::ConstraintType::Horizontal) {
                    auto hc = std::dynamic_pointer_cast<CADEngine::HorizontalConstraint>(c);
                    if (hc && hc->matchesPolylineSegment(polyline, segmentIndex)) return true;
                }
                if (c->getType() == CADEngine::ConstraintType::Vertical) {
                    auto vc = std::dynamic_pointer_cast<CADEngine::VerticalConstraint>(c);
                    if (vc && vc->matchesPolylineSegment(polyline, segmentIndex)) return true;
                }
                return false;
            }),
        constraints.end()
    );
    
    // === 3. Modifier la géométrie ===
    if (segmentIndex == 0) {
        // Supprimer le premier segment → retirer P0
        points.erase(points.begin());
        polyline->setPoints(points);
        
        // Décaler les indices de segment des dimensions restantes
        for (auto& d : dimensions) {
            if (d->getEntityId() == polylineId) {
                auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(d);
                if (linearDim && linearDim->getSegmentIndex() > 0) {
                    linearDim->setSegmentIndex(linearDim->getSegmentIndex() - 1);
                    // Mettre à jour les points
                    int newIdx = linearDim->getSegmentIndex();
                    if (newIdx >= 0 && newIdx < (int)points.size() - 1) {
                        linearDim->setPoint1(points[newIdx]);
                        linearDim->setPoint2(points[newIdx + 1]);
                    }
                }
            }
        }
        
        // Décaler les indices des contraintes H/V restantes
        for (auto& c : constraints) {
            if (c->getType() == CADEngine::ConstraintType::Horizontal) {
                auto hc = std::dynamic_pointer_cast<CADEngine::HorizontalConstraint>(c);
                if (hc && hc->getPolyline() == polyline && hc->getSegmentIndex() > 0) {
                    // Recréer avec index décalé
                    auto newC = std::make_shared<CADEngine::HorizontalConstraint>(
                        polyline, hc->getSegmentIndex() - 1);
                    c = newC;
                }
            }
            if (c->getType() == CADEngine::ConstraintType::Vertical) {
                auto vc = std::dynamic_pointer_cast<CADEngine::VerticalConstraint>(c);
                if (vc && vc->getPolyline() == polyline && vc->getSegmentIndex() > 0) {
                    auto newC = std::make_shared<CADEngine::VerticalConstraint>(
                        polyline, vc->getSegmentIndex() - 1);
                    c = newC;
                }
            }
        }
        
        m_statusLabel->setText(QString("Premier segment supprimé - %1 segments restants")
            .arg((int)points.size() - 1));
    }
    else if (segmentIndex == numSegments - 1) {
        // Supprimer le dernier segment → retirer dernier point
        points.pop_back();
        polyline->setPoints(points);
        
        // Pas besoin de décaler les indices (on a supprimé à la fin)
        // Juste mettre à jour les points des dimensions restantes
        for (auto& d : dimensions) {
            if (d->getEntityId() == polylineId) {
                auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(d);
                if (linearDim) {
                    int idx = linearDim->getSegmentIndex();
                    if (idx >= 0 && idx < (int)points.size() - 1) {
                        linearDim->setPoint1(points[idx]);
                        linearDim->setPoint2(points[idx + 1]);
                    }
                }
            }
        }
        
        m_statusLabel->setText(QString("Dernier segment supprimé - %1 segments restants")
            .arg((int)points.size() - 1));
    }
    else {
        // Segment au milieu → SPLIT en deux polylines
        // Polyline 1 : [P0 .. P_segIdx]
        // Polyline 2 : [P_{segIdx+1} .. Pn]
        
        std::vector<gp_Pnt2d> points1(points.begin(), points.begin() + segmentIndex + 1);
        std::vector<gp_Pnt2d> points2(points.begin() + segmentIndex + 1, points.end());
        
        // Modifier la polyline originale pour garder la première partie
        polyline->setPoints(points1);
        
        // Créer une nouvelle polyline pour la deuxième partie
        if (points2.size() >= 2) {
            auto newPolyline = std::make_shared<CADEngine::SketchPolyline>(points2);
            sketch2D->addEntity(newPolyline);
            
            // Transférer les dimensions de la 2ème partie vers la nouvelle polyline
            int newPolyId = newPolyline->getId();
            for (auto& d : dimensions) {
                if (d->getEntityId() == polylineId) {
                    auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(d);
                    if (linearDim) {
                        int oldIdx = linearDim->getSegmentIndex();
                        if (oldIdx > segmentIndex) {
                            // Ce segment fait partie de la 2ème polyline
                            int newIdx = oldIdx - segmentIndex - 1;
                            linearDim->setEntityId(newPolyId);
                            linearDim->setSegmentIndex(newIdx);
                            if (newIdx >= 0 && newIdx < (int)points2.size() - 1) {
                                linearDim->setPoint1(points2[newIdx]);
                                linearDim->setPoint2(points2[newIdx + 1]);
                            }
                        }
                    }
                }
            }
            
            // Transférer les contraintes H/V de la 2ème partie
            for (auto& c : constraints) {
                if (c->getType() == CADEngine::ConstraintType::Horizontal) {
                    auto hc = std::dynamic_pointer_cast<CADEngine::HorizontalConstraint>(c);
                    if (hc && hc->getPolyline() == polyline && hc->getSegmentIndex() > segmentIndex) {
                        int newIdx = hc->getSegmentIndex() - segmentIndex - 1;
                        c = std::make_shared<CADEngine::HorizontalConstraint>(newPolyline, newIdx);
                    }
                }
                if (c->getType() == CADEngine::ConstraintType::Vertical) {
                    auto vc = std::dynamic_pointer_cast<CADEngine::VerticalConstraint>(c);
                    if (vc && vc->getPolyline() == polyline && vc->getSegmentIndex() > segmentIndex) {
                        int newIdx = vc->getSegmentIndex() - segmentIndex - 1;
                        c = std::make_shared<CADEngine::VerticalConstraint>(newPolyline, newIdx);
                    }
                }
            }
        }
        
        // Supprimer les contraintes parallèle/perpendiculaire qui référençaient le segment supprimé
        // (trop complexe de les transférer correctement)
        constraints.erase(
            std::remove_if(constraints.begin(), constraints.end(),
                [&polyline, segmentIndex](const auto& c) {
                    if (!c) return true;
                    if (c->getType() == CADEngine::ConstraintType::Parallel) {
                        auto pc = std::dynamic_pointer_cast<CADEngine::ParallelConstraint>(c);
                        if (pc && pc->referencesEntity(polyline)) return true;
                    }
                    if (c->getType() == CADEngine::ConstraintType::Perpendicular) {
                        auto pc = std::dynamic_pointer_cast<CADEngine::PerpendicularConstraint>(c);
                        if (pc && pc->referencesEntity(polyline)) return true;
                    }
                    return false;
                }),
            constraints.end()
        );
        
        // Mettre à jour les points des dimensions de la première partie
        for (auto& d : dimensions) {
            if (d->getEntityId() == polylineId) {
                auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(d);
                if (linearDim) {
                    int idx = linearDim->getSegmentIndex();
                    if (idx >= 0 && idx < (int)points1.size() - 1) {
                        linearDim->setPoint1(points1[idx]);
                        linearDim->setPoint2(points1[idx + 1]);
                    }
                }
            }
        }
        
        m_statusLabel->setText(QString("Segment %1 supprimé - polyline scindée en 2")
            .arg(segmentIndex));
    }
    
    // Si la polyline restante n'a qu'un seul point, la supprimer
    if (polyline->getPoints().size() < 2) {
        auto& entities = sketch2D->getEntitiesRef();
        entities.erase(
            std::remove_if(entities.begin(), entities.end(),
                [polylineId](const auto& e) { return e->getId() == polylineId; }),
            entities.end()
        );
    }
    
    m_selectedEntity.reset();
    m_viewport->update();
}

void MainWindow::onUndo() {
    if (m_undoStack.empty()) return;
    
    // Récupérer dernière commande
    auto command = m_undoStack.back();
    m_undoStack.pop_back();
    
    qDebug() << "[onUndo]" << QString::fromStdString(command->getDescription());
    
    // Annuler
    command->undo();
    
    // Ajouter à redo stack
    m_redoStack.push_back(command);
    
    // Rafraîchir toutes les cotations (propagation après undo)
    refreshAllAngularDimensions();
    
    // Mettre à jour UI
    m_viewport->update();
    updateUndoRedoActions();
    m_statusLabel->setText(QString("Annulé: %1").arg(QString::fromStdString(command->getDescription())));
    
    qDebug() << "  → Undo stack size:" << m_undoStack.size();
}

void MainWindow::onRedo() {
    if (m_redoStack.empty()) {
        qDebug() << "[onRedo] REDO STACK VIDE !";
        return;
    }
    
    // Récupérer dernière commande annulée
    auto command = m_redoStack.back();
    m_redoStack.pop_back();
    
    qDebug() << "[onRedo]" << QString::fromStdString(command->getDescription());
    
    // Refaire
    command->redo();
    
    // Remettre dans undo stack
    m_undoStack.push_back(command);
    
    // Rafraîchir toutes les cotations (propagation après redo)
    refreshAllAngularDimensions();
    
    // Mettre à jour UI
    m_viewport->update();
    updateUndoRedoActions();
    m_statusLabel->setText(QString("Rétabli: %1").arg(QString::fromStdString(command->getDescription())));
    
    qDebug() << "  → Redo stack size:" << m_redoStack.size();
    qDebug() << "  → Undo stack size:" << m_undoStack.size();
}

// ============================================================================
// Contraintes géométriques
// ============================================================================

void MainWindow::onConstraintHorizontal() {
    m_viewport->setSketchTool(SketchTool::ConstraintHorizontal);
    m_statusLabel->setText("Cliquez sur une ligne à rendre horizontale");
}

void MainWindow::onConstraintVertical() {
    m_viewport->setSketchTool(SketchTool::ConstraintVertical);
    m_statusLabel->setText("Cliquez sur une ligne à rendre verticale");
}

void MainWindow::onConstraintParallel() {
    m_viewport->setSketchTool(SketchTool::ConstraintParallel);
    m_statusLabel->setText("Cliquez sur 2 lignes à rendre parallèles");
}

void MainWindow::onConstraintPerpendicular() {
    m_viewport->setSketchTool(SketchTool::ConstraintPerpendicular);
    m_statusLabel->setText("Cliquez sur 2 lignes à rendre perpendiculaires");
}

void MainWindow::onConstraintCoincident() {
    m_viewport->setSketchTool(SketchTool::ConstraintCoincident);
    m_statusLabel->setText("Cliquez sur 2 extrémités de lignes à rendre coïncidentes");
}

void MainWindow::onConstraintConcentric() {
    m_viewport->setSketchTool(SketchTool::ConstraintConcentric);
    m_statusLabel->setText("Cliquez sur 2 cercles à rendre concentriques");
}

void MainWindow::onConstraintSymmetric() {
    m_viewport->setSketchTool(SketchTool::ConstraintSymmetric);
    m_statusLabel->setText("Symétrie: Cliquez sur le 1er point");
}

void MainWindow::onConstraintFix() {
    m_viewport->setSketchTool(SketchTool::ConstraintFix);
    m_statusLabel->setText("Verrouillage: Cliquez sur un point à fixer");
}

void MainWindow::onFillet() {
    m_viewport->setSketchTool(SketchTool::Fillet);
    m_statusLabel->setText("Congé: Cliquez sur un vertex de polyline");
}

void MainWindow::executeCommand(std::shared_ptr<CADEngine::Command> command) {
    if (!command) return;
    
    qDebug() << "[executeCommand]" << QString::fromStdString(command->getDescription());
    
    // Exécuter
    command->execute();
    
    // Ajouter à l'historique
    m_undoStack.push_back(command);
    
    // Vider redo stack (nouvelle action = invalide les redo)
    m_redoStack.clear();
    
    // Mettre à jour UI
    updateUndoRedoActions();
    
    qDebug() << "  → Undo stack size:" << m_undoStack.size();
}

void MainWindow::updateUndoRedoActions() {
    bool undoEnabled = !m_undoStack.empty();
    bool redoEnabled = !m_redoStack.empty();
    
    m_actUndo->setEnabled(undoEnabled);
    m_actRedo->setEnabled(redoEnabled);
    
    qDebug() << "[updateUndoRedoActions] Undo:" << (undoEnabled ? "ENABLED" : "disabled") 
             << "| Redo:" << (redoEnabled ? "ENABLED" : "disabled");
    qDebug() << "  → Undo stack:" << m_undoStack.size() << "| Redo stack:" << m_redoStack.size();
}

void MainWindow::onExportSketchDXF(std::shared_ptr<CADEngine::SketchFeature> sketch) {
    if (!sketch) return;
    
    // Dialog pour choisir fichier
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Exporter Sketch en DXF",
        QString::fromStdString(sketch->getName()) + ".dxf",
        "Fichiers DXF (*.dxf)"
    );
    
    if (fileName.isEmpty()) return;
    
    // Créer fichier DXF
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Erreur", "Impossible de créer le fichier DXF");
        return;
    }
    
    QTextStream out(&file);
    
    // === En-tête DXF ===
    out << "  0\n";
    out << "SECTION\n";
    out << "  2\n";
    out << "HEADER\n";
    out << "  9\n";
    out << "$ACADVER\n";
    out << "  1\n";
    out << "AC1015\n";  // AutoCAD 2000
    out << "  0\n";
    out << "ENDSEC\n";
    
    // === Section TABLES ===
    out << "  0\n";
    out << "SECTION\n";
    out << "  2\n";
    out << "TABLES\n";
    out << "  0\n";
    out << "TABLE\n";
    out << "  2\n";
    out << "LAYER\n";
    out << "  0\n";
    out << "LAYER\n";
    out << "  2\n";
    out << "SKETCH\n";  // Nom du layer
    out << " 70\n";
    out << "0\n";
    out << " 62\n";
    out << "7\n";  // Couleur blanche/noire
    out << "  6\n";
    out << "CONTINUOUS\n";
    out << "  0\n";
    out << "ENDTAB\n";
    out << "  0\n";
    out << "ENDSEC\n";
    
    // === Section ENTITIES ===
    out << "  0\n";
    out << "SECTION\n";
    out << "  2\n";
    out << "ENTITIES\n";
    
    // Lambda: écrire une ligne DXF
    auto dxfLine = [&](double x1, double y1, double x2, double y2) {
        out << "  0\nLINE\n  8\nSKETCH\n";
        out << " 10\n" << QString::number(x1, 'f', 6) << "\n";
        out << " 20\n" << QString::number(y1, 'f', 6) << "\n";
        out << " 30\n0.0\n";
        out << " 11\n" << QString::number(x2, 'f', 6) << "\n";
        out << " 21\n" << QString::number(y2, 'f', 6) << "\n";
        out << " 31\n0.0\n";
    };

    auto sketch2D = sketch->getSketch2D();
    if (sketch2D) {
        for (const auto& entity : sketch2D->getEntities()) {
            if (entity->isConstruction()) continue;

            if (entity->getType() == CADEngine::SketchEntityType::Line) {
                auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
                if (line)
                    dxfLine(line->getStart().X(), line->getStart().Y(),
                            line->getEnd().X(),   line->getEnd().Y());
            }
            else if (entity->getType() == CADEngine::SketchEntityType::Circle) {
                auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
                if (circle) {
                    gp_Pnt2d center = circle->getCenter();
                    out << "  0\nCIRCLE\n  8\nSKETCH\n";
                    out << " 10\n" << QString::number(center.X(), 'f', 6) << "\n";
                    out << " 20\n" << QString::number(center.Y(), 'f', 6) << "\n";
                    out << " 30\n0.0\n";
                    out << " 40\n" << QString::number(circle->getRadius(), 'f', 6) << "\n";
                }
            }
            else if (entity->getType() == CADEngine::SketchEntityType::Arc) {
                auto arc = std::dynamic_pointer_cast<CADEngine::SketchArc>(entity);
                if (arc) {
                    gp_Pnt2d p1 = arc->getStart();
                    gp_Pnt2d p2 = arc->getMid();
                    gp_Pnt2d p3 = arc->getEnd();

                    if (arc->isBezier()) {
                        // Arc Bézier quadratique : approximé par 16 segments de ligne
                        gp_Pnt2d prev = p1;
                        for (int i = 1; i <= 16; ++i) {
                            double t = (double)i / 16.0;
                            double mt = 1.0 - t;
                            double nx = mt*mt*p1.X() + 2*mt*t*p2.X() + t*t*p3.X();
                            double ny = mt*mt*p1.Y() + 2*mt*t*p2.Y() + t*t*p3.Y();
                            dxfLine(prev.X(), prev.Y(), nx, ny);
                            prev = gp_Pnt2d(nx, ny);
                        }
                    } else {
                        // Arc circulaire : calculer centre et rayon
                        double ax = p1.X(), ay = p1.Y();
                        double bx = p2.X(), by = p2.Y();
                        double cx = p3.X(), cy = p3.Y();
                        double d = 2.0 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
                        if (std::abs(d) > 1e-10) {
                            double ux = ((ax*ax + ay*ay) * (by - cy) + (bx*bx + by*by) * (cy - ay) + (cx*cx + cy*cy) * (ay - by)) / d;
                            double uy = ((ax*ax + ay*ay) * (cx - bx) + (bx*bx + by*by) * (ax - cx) + (cx*cx + cy*cy) * (bx - ax)) / d;
                            double radius = std::sqrt((ax - ux)*(ax - ux) + (ay - uy)*(ay - uy));
                            double angle1 = std::atan2(ay - uy, ax - ux) * 180.0 / M_PI;
                            double angle2 = std::atan2(cy - uy, cx - ux) * 180.0 / M_PI;
                            out << "  0\nARC\n  8\nSKETCH\n";
                            out << " 10\n" << QString::number(ux, 'f', 6) << "\n";
                            out << " 20\n" << QString::number(uy, 'f', 6) << "\n";
                            out << " 30\n0.0\n";
                            out << " 40\n" << QString::number(radius, 'f', 6) << "\n";
                            out << " 50\n" << QString::number(angle1, 'f', 6) << "\n";
                            out << " 51\n" << QString::number(angle2, 'f', 6) << "\n";
                        }
                    }
                }
            }
            else if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
                auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
                if (rect) {
                    auto corners = rect->getKeyPoints();
                    for (int i = 0; i < 4; ++i) {
                        const auto& a = corners[i];
                        const auto& b = corners[(i + 1) % 4];
                        dxfLine(a.X(), a.Y(), b.X(), b.Y());
                    }
                }
            }
            else if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
                auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
                if (polyline) {
                    auto pts = polyline->getPoints();
                    for (size_t i = 0; i + 1 < pts.size(); ++i)
                        dxfLine(pts[i].X(), pts[i].Y(), pts[i+1].X(), pts[i+1].Y());
                }
            }
            else if (entity->getType() == CADEngine::SketchEntityType::Ellipse) {
                auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
                if (ellipse) {
                    // Approximation 64 segments
                    gp_Pnt2d center = ellipse->getCenter();
                    double a = ellipse->getMajorRadius(), b = ellipse->getMinorRadius();
                    double rot = ellipse->getRotation();
                    double cosR = std::cos(rot), sinR = std::sin(rot);
                    const int N = 64;
                    gp_Pnt2d prev(center.X() + a*cosR, center.Y() + a*sinR);
                    for (int i = 1; i <= N; ++i) {
                        double t = 2.0 * M_PI * i / N;
                        double lx = a * std::cos(t), ly = b * std::sin(t);
                        gp_Pnt2d cur(center.X() + lx*cosR - ly*sinR,
                                     center.Y() + lx*sinR + ly*cosR);
                        dxfLine(prev.X(), prev.Y(), cur.X(), cur.Y());
                        prev = cur;
                    }
                }
            }
            else if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
                auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
                if (polygon) {
                    auto verts = polygon->getVertices();
                    for (size_t i = 0; i < verts.size(); ++i) {
                        const auto& a = verts[i];
                        const auto& b = verts[(i + 1) % verts.size()];
                        dxfLine(a.X(), a.Y(), b.X(), b.Y());
                    }
                }
            }
            else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
                auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
                if (oblong) {
                    gp_Pnt2d center = oblong->getCenter();
                    double L = oblong->getLength(), W = oblong->getWidth();
                    double rot = oblong->getRotation();
                    double r = W / 2.0, halfS = (L - W) / 2.0;
                    double cosR = std::cos(rot), sinR = std::sin(rot);
                    // Deux droites + deux demi-cercles (32 segments chacun)
                    gp_Pnt2d p1(center.X() - halfS*cosR + r*(-sinR), center.Y() - halfS*sinR + r*cosR);
                    gp_Pnt2d p2(center.X() + halfS*cosR + r*(-sinR), center.Y() + halfS*sinR + r*cosR);
                    gp_Pnt2d p3(center.X() + halfS*cosR - r*(-sinR), center.Y() + halfS*sinR - r*cosR);
                    gp_Pnt2d p4(center.X() - halfS*cosR - r*(-sinR), center.Y() - halfS*sinR - r*cosR);
                    dxfLine(p1.X(), p1.Y(), p2.X(), p2.Y());
                    dxfLine(p3.X(), p3.Y(), p4.X(), p4.Y());
                    // Demi-cercle droit
                    double baseAngle = std::atan2(cosR, -sinR);
                    gp_Pnt2d c2(center.X() + halfS*cosR, center.Y() + halfS*sinR);
                    gp_Pnt2d prev2 = p2;
                    for (int i = 1; i <= 32; ++i) {
                        double a = baseAngle - M_PI * i / 32.0;
                        gp_Pnt2d cur(c2.X() + r*std::cos(a), c2.Y() + r*std::sin(a));
                        dxfLine(prev2.X(), prev2.Y(), cur.X(), cur.Y());
                        prev2 = cur;
                    }
                    // Demi-cercle gauche
                    gp_Pnt2d c1(center.X() - halfS*cosR, center.Y() - halfS*sinR);
                    gp_Pnt2d prev1 = p4;
                    for (int i = 1; i <= 32; ++i) {
                        double a = baseAngle + M_PI - M_PI * i / 32.0;
                        gp_Pnt2d cur(c1.X() + r*std::cos(a), c1.Y() + r*std::sin(a));
                        dxfLine(prev1.X(), prev1.Y(), cur.X(), cur.Y());
                        prev1 = cur;
                    }
                }
            }
        }
        
        // Ajouter les dimensions comme texte
        for (const auto& dim : sketch2D->getDimensions()) {
            if (dim->getType() == CADEngine::DimensionType::Linear) {
                auto linearDim = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
                if (linearDim) {
                    gp_Pnt2d textPos = linearDim->getTextPosition();
                    std::string text = linearDim->getText();
                    
                    out << "  0\n";
                    out << "TEXT\n";
                    out << "  8\n";
                    out << "SKETCH\n";
                    out << " 10\n";
                    out << QString::number(textPos.X(), 'f', 6) << "\n";
                    out << " 20\n";
                    out << QString::number(textPos.Y(), 'f', 6) << "\n";
                    out << " 30\n";
                    out << "0.0\n";
                    out << " 40\n";
                    out << "5.0\n";  // Hauteur texte
                    out << "  1\n";
                    out << QString::fromStdString(text) << "\n";
                }
            }
        }
    }
    
    // === Fin section ENTITIES ===
    out << "  0\n";
    out << "ENDSEC\n";
    
    // === EOF ===
    out << "  0\n";
    out << "EOF\n";
    
    file.close();
    
    m_statusLabel->setText(QString("Sketch exporté: %1").arg(fileName));
    QMessageBox::information(this, "Export réussi", 
        QString("Le sketch a été exporté en DXF :\n%1").arg(fileName));
}

// ============================================================================
// Export Sketch en PDF (avec cotations et angles)
// ============================================================================

void MainWindow::onExportSketchPDF(std::shared_ptr<CADEngine::SketchFeature> sketch) {
    if (!sketch) return;
    
    auto sketch2D = sketch->getSketch2D();
    if (!sketch2D) return;
    
    // Dialog pour choisir fichier
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Exporter Sketch en PDF",
        QString::fromStdString(sketch->getName()) + ".pdf",
        "Fichiers PDF (*.pdf)"
    );
    
    if (fileName.isEmpty()) return;
    
    // === Calcul du bounding box de toutes les entités + dimensions ===
    double minX = 1e18, minY = 1e18, maxX = -1e18, maxY = -1e18;
    
    auto expandBBox = [&](double x, double y) {
        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    };
    
    for (const auto& entity : sketch2D->getEntities()) {
        if (entity->isConstruction()) continue;
        if (entity->getType() == CADEngine::SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
            if (line) {
                expandBBox(line->getStart().X(), line->getStart().Y());
                expandBBox(line->getEnd().X(), line->getEnd().Y());
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
            if (circle) {
                double cx = circle->getCenter().X(), cy = circle->getCenter().Y();
                double r = circle->getRadius();
                expandBBox(cx - r, cy - r);
                expandBBox(cx + r, cy + r);
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Arc) {
            auto arc = std::dynamic_pointer_cast<CADEngine::SketchArc>(entity);
            if (arc) {
                expandBBox(arc->getStart().X(), arc->getStart().Y());
                expandBBox(arc->getMid().X(), arc->getMid().Y());
                expandBBox(arc->getEnd().X(), arc->getEnd().Y());
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
            if (rect) {
                for (const auto& pt : rect->getKeyPoints())
                    expandBBox(pt.X(), pt.Y());
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto poly = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
            if (poly) {
                for (const auto& pt : poly->getPoints())
                    expandBBox(pt.X(), pt.Y());
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Ellipse) {
            auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
            if (ellipse) {
                double cx = ellipse->getCenter().X(), cy = ellipse->getCenter().Y();
                double a = ellipse->getMajorRadius(), b = ellipse->getMinorRadius();
                expandBBox(cx - a, cy - b);
                expandBBox(cx + a, cy + b);
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
            auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
            if (polygon) {
                for (const auto& v : polygon->getVertices())
                    expandBBox(v.X(), v.Y());
            }
        }
        else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
            auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
            if (oblong) {
                double cx = oblong->getCenter().X(), cy = oblong->getCenter().Y();
                double L = oblong->getLength() / 2.0, W = oblong->getWidth() / 2.0;
                expandBBox(cx - L - W, cy - L - W);
                expandBBox(cx + L + W, cy + L + W);
            }
        }
    }
    
    // Ajouter les positions des cotations au bounding box
    // (manuelles + auto-générées)
    sketch2D->regenerateAutoDimensions();
    
    auto allDimsForBBox = sketch2D->getDimensions();
    auto autoDimsForBBox = sketch2D->getAutoDimensions();
    allDimsForBBox.insert(allDimsForBBox.end(), autoDimsForBBox.begin(), autoDimsForBBox.end());
    
    for (const auto& dim : allDimsForBBox) {
        gp_Pnt2d tp = dim->getTextPosition();
        expandBBox(tp.X(), tp.Y());
        
        if (dim->getType() == CADEngine::DimensionType::Linear) {
            auto ld = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
            if (ld) {
                expandBBox(ld->getDimensionLineStart().X(), ld->getDimensionLineStart().Y());
                expandBBox(ld->getDimensionLineEnd().X(), ld->getDimensionLineEnd().Y());
            }
        }
        else if (dim->getType() == CADEngine::DimensionType::Angular) {
            auto ad = std::dynamic_pointer_cast<CADEngine::AngularDimension>(dim);
            if (ad) {
                double r = ad->getRadius();
                expandBBox(ad->getCenter().X() - r, ad->getCenter().Y() - r);
                expandBBox(ad->getCenter().X() + r, ad->getCenter().Y() + r);
            }
        }
        else if (dim->getType() == CADEngine::DimensionType::Radial) {
            auto rd = std::dynamic_pointer_cast<CADEngine::RadialDimension>(dim);
            if (rd) {
                expandBBox(rd->getCenter().X(), rd->getCenter().Y());
                expandBBox(rd->getArrowPoint().X(), rd->getArrowPoint().Y());
            }
        }
    }
    
    // Vérifier qu'on a des données
    if (minX >= maxX || minY >= maxY) {
        QMessageBox::warning(this, "Erreur", "Le sketch est vide, rien à exporter.");
        return;
    }
    
    // Marge autour du dessin (en mm du sketch)
    double sketchMargin = std::max(maxX - minX, maxY - minY) * 0.12;
    minX -= sketchMargin;
    minY -= sketchMargin;
    maxX += sketchMargin;
    maxY += sketchMargin;
    
    double sketchW = maxX - minX;
    double sketchH = maxY - minY;
    
    // === Créer le PDF avec QPdfWriter ===
    QPdfWriter pdfWriter(fileName);
    pdfWriter.setPageSize(QPageSize(QPageSize::A4));
    
    // Choisir orientation selon le rapport d'aspect
    if (sketchW > sketchH)
        pdfWriter.setPageOrientation(QPageLayout::Landscape);
    else
        pdfWriter.setPageOrientation(QPageLayout::Portrait);
    
    pdfWriter.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    pdfWriter.setResolution(300);
    pdfWriter.setTitle(QString::fromStdString(sketch->getName()));
    pdfWriter.setCreator("CAD-ENGINE v0.8");
    
    QPainter painter(&pdfWriter);
    if (!painter.isActive()) {
        QMessageBox::warning(this, "Erreur", "Impossible de créer le fichier PDF.");
        return;
    }
    
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Zone de dessin disponible (en pixels à 300dpi)
    QRectF pageRect = painter.viewport();
    double pageW = pageRect.width();
    double pageH = pageRect.height();
    
    // Réserver espace en haut pour le titre et en bas pour le cartouche
    double titleH = pageH * 0.06;
    double cartoucheH = pageH * 0.05;
    double drawAreaH = pageH - titleH - cartoucheH;
    double drawAreaW = pageW;
    double drawAreaTop = titleH;
    
    // === Titre ===
    {
        QFont titleFont("Helvetica", 14, QFont::Bold);
        // Ajuster la taille en pixels pour le DPI du PDF
        titleFont.setPixelSize((int)(titleH * 0.45));
        painter.setFont(titleFont);
        painter.setPen(QPen(Qt::black, 2));
        painter.drawText(QRectF(0, 0, pageW, titleH),
                         Qt::AlignCenter,
                         QString::fromStdString(sketch->getName()));
        
        // Ligne séparatrice
        painter.setPen(QPen(Qt::darkGray, 3));
        painter.drawLine(QPointF(pageW * 0.05, titleH - 5),
                         QPointF(pageW * 0.95, titleH - 5));
    }
    
    // === Calcul du facteur d'échelle ===
    double scaleX = drawAreaW / sketchW;
    double scaleY = drawAreaH / sketchH;
    double scale = std::min(scaleX, scaleY) * 0.92;  // 92% pour marge interne
    
    // Offset pour centrer
    double offsetX = (drawAreaW - sketchW * scale) / 2.0;
    double offsetY = (drawAreaH - sketchH * scale) / 2.0;
    
    // Lambda: convertir coordonnées sketch → page PDF
    // Note: Y inversé car QPainter a Y vers le bas
    auto toPage = [&](double sx, double sy) -> QPointF {
        double px = (sx - minX) * scale + offsetX;
        double py = drawAreaTop + drawAreaH - ((sy - minY) * scale + offsetY);
        return QPointF(px, py);
    };
    
    // === Épaisseurs et styles ===
    double entityLineW = std::max(3.0, scale * 0.15);
    double dimLineW = std::max(1.5, entityLineW * 0.5);
    double arrowSize = std::max(scale * 1.5, 15.0);
    double crossSize = std::max(scale * 2.0, 10.0);
    
    QFont dimFont("Helvetica", 9);
    dimFont.setPixelSize((int)std::max(arrowSize * 1.8, 28.0));
    
    QPen entityPen(Qt::black, entityLineW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen dimPen(QColor(0, 100, 200), dimLineW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen extLinePen(QColor(0, 100, 200), dimLineW * 0.7, Qt::DashLine, Qt::RoundCap);
    QPen centerPen(QColor(120, 120, 120), dimLineW * 0.5, Qt::DashDotLine);
    
    // === DESSIN DES ENTITÉS ===
    painter.setPen(entityPen);
    painter.setBrush(Qt::NoBrush);
    
    for (const auto& entity : sketch2D->getEntities()) {
        if (entity->isConstruction()) continue;

        // --- LINE ---
        if (entity->getType() == CADEngine::SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
            if (line) {
                painter.drawLine(toPage(line->getStart().X(), line->getStart().Y()),
                                 toPage(line->getEnd().X(), line->getEnd().Y()));
            }
        }
        // --- CIRCLE ---
        else if (entity->getType() == CADEngine::SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<CADEngine::SketchCircle>(entity);
            if (circle) {
                QPointF c = toPage(circle->getCenter().X(), circle->getCenter().Y());
                double r = circle->getRadius() * scale;
                painter.drawEllipse(c, r, r);
                
                // Croix au centre
                painter.setPen(centerPen);
                painter.drawLine(QPointF(c.x() - crossSize, c.y()), QPointF(c.x() + crossSize, c.y()));
                painter.drawLine(QPointF(c.x(), c.y() - crossSize), QPointF(c.x(), c.y() + crossSize));
                painter.setPen(entityPen);
            }
        }
        // --- ARC ---
        else if (entity->getType() == CADEngine::SketchEntityType::Arc) {
            auto arc = std::dynamic_pointer_cast<CADEngine::SketchArc>(entity);
            if (arc) {
                gp_Pnt2d p1 = arc->getStart();
                gp_Pnt2d p2 = arc->getMid();
                gp_Pnt2d p3 = arc->getEnd();
                
                if (arc->isBezier()) {
                    // Bézier quadratique
                    QPainterPath path;
                    QPointF pp1 = toPage(p1.X(), p1.Y());
                    QPointF pp2 = toPage(p2.X(), p2.Y());
                    QPointF pp3 = toPage(p3.X(), p3.Y());
                    path.moveTo(pp1);
                    path.quadTo(pp2, pp3);
                    painter.drawPath(path);
                } else {
                    // Arc circulaire : calculer centre et rayon
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
                        
                        // Dessiner en polyline
                        int segments = std::max(32, (int)(std::abs(totalSweep) * 64.0 / M_PI));
                        QVector<QPointF> pts;
                        for (int i = 0; i <= segments; ++i) {
                            double a = a1 + totalSweep * i / segments;
                            pts.append(toPage(cx + radius * std::cos(a), cy + radius * std::sin(a)));
                        }
                        painter.drawPolyline(pts.data(), pts.size());
                    } else {
                        painter.drawLine(toPage(p1.X(), p1.Y()), toPage(p3.X(), p3.Y()));
                    }
                }
            }
        }
        // --- RECTANGLE ---
        else if (entity->getType() == CADEngine::SketchEntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<CADEngine::SketchRectangle>(entity);
            if (rect) {
                auto corners = rect->getKeyPoints();
                QPolygonF poly;
                for (const auto& c : corners)
                    poly << toPage(c.X(), c.Y());
                poly << poly.first();  // Fermer
                painter.drawPolygon(poly);
            }
        }
        // --- POLYLINE ---
        else if (entity->getType() == CADEngine::SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<CADEngine::SketchPolyline>(entity);
            if (polyline) {
                auto points = polyline->getPoints();
                QVector<QPointF> pts;
                for (const auto& pt : points)
                    pts.append(toPage(pt.X(), pt.Y()));
                painter.drawPolyline(pts.data(), pts.size());
            }
        }
        // --- ELLIPSE ---
        else if (entity->getType() == CADEngine::SketchEntityType::Ellipse) {
            auto ellipse = std::dynamic_pointer_cast<CADEngine::SketchEllipse>(entity);
            if (ellipse) {
                gp_Pnt2d center = ellipse->getCenter();
                double a = ellipse->getMajorRadius();
                double b = ellipse->getMinorRadius();
                double rot = ellipse->getRotation();
                double cosR = std::cos(rot), sinR = std::sin(rot);
                
                int segments = 64;
                QVector<QPointF> pts;
                for (int i = 0; i <= segments; ++i) {
                    double t = 2.0 * M_PI * i / segments;
                    double lx = a * std::cos(t);
                    double ly = b * std::sin(t);
                    double wx = center.X() + lx * cosR - ly * sinR;
                    double wy = center.Y() + lx * sinR + ly * cosR;
                    pts.append(toPage(wx, wy));
                }
                painter.drawPolygon(pts.data(), pts.size());
                
                // Croix au centre
                QPointF c = toPage(center.X(), center.Y());
                painter.setPen(centerPen);
                painter.drawLine(QPointF(c.x() - crossSize, c.y()), QPointF(c.x() + crossSize, c.y()));
                painter.drawLine(QPointF(c.x(), c.y() - crossSize), QPointF(c.x(), c.y() + crossSize));
                painter.setPen(entityPen);
            }
        }
        // --- POLYGON ---
        else if (entity->getType() == CADEngine::SketchEntityType::Polygon) {
            auto polygon = std::dynamic_pointer_cast<CADEngine::SketchPolygon>(entity);
            if (polygon) {
                auto verts = polygon->getVertices();
                QPolygonF poly;
                for (const auto& v : verts)
                    poly << toPage(v.X(), v.Y());
                poly << poly.first();
                painter.drawPolygon(poly);
                
                // Croix au centre
                QPointF c = toPage(polygon->getCenter().X(), polygon->getCenter().Y());
                painter.setPen(centerPen);
                painter.drawLine(QPointF(c.x() - crossSize, c.y()), QPointF(c.x() + crossSize, c.y()));
                painter.drawLine(QPointF(c.x(), c.y() - crossSize), QPointF(c.x(), c.y() + crossSize));
                painter.setPen(entityPen);
            }
        }
        // --- OBLONG ---
        else if (entity->getType() == CADEngine::SketchEntityType::Oblong) {
            auto oblong = std::dynamic_pointer_cast<CADEngine::SketchOblong>(entity);
            if (oblong) {
                gp_Pnt2d center = oblong->getCenter();
                double L = oblong->getLength();
                double W = oblong->getWidth();
                double rot = oblong->getRotation();
                double r = W / 2.0;
                double halfS = (L - W) / 2.0;
                
                double cosR = std::cos(rot), sinR = std::sin(rot);
                double axX = cosR, axY = sinR;
                double prX = -sinR, prY = cosR;
                
                // Construire le contour complet
                QVector<QPointF> pts;
                
                // Ligne haut (de gauche à droite)
                double p1x = center.X() - halfS*axX + r*prX;
                double p1y = center.Y() - halfS*axY + r*prY;
                double p2x = center.X() + halfS*axX + r*prX;
                double p2y = center.Y() + halfS*axY + r*prY;
                pts.append(toPage(p1x, p1y));
                pts.append(toPage(p2x, p2y));
                
                // Demi-cercle droit
                double c2x = center.X() + halfS*axX;
                double c2y = center.Y() + halfS*axY;
                double startAngle = std::atan2(prY, prX);
                for (int i = 1; i <= 32; ++i) {
                    double a = startAngle - M_PI * i / 32.0;
                    pts.append(toPage(c2x + r*std::cos(a), c2y + r*std::sin(a)));
                }
                
                // Ligne bas (de droite à gauche) — déjà connecté par le demi-cercle
                double p4x = center.X() - halfS*axX - r*prX;
                double p4y = center.Y() - halfS*axY - r*prY;
                pts.append(toPage(p4x, p4y));
                
                // Demi-cercle gauche
                double c1x = center.X() - halfS*axX;
                double c1y = center.Y() - halfS*axY;
                for (int i = 1; i <= 32; ++i) {
                    double a = startAngle + M_PI - M_PI * i / 32.0;
                    pts.append(toPage(c1x + r*std::cos(a), c1y + r*std::sin(a)));
                }
                
                painter.drawPolygon(pts.data(), pts.size());
                
                // Croix au centre
                QPointF c = toPage(center.X(), center.Y());
                painter.setPen(centerPen);
                painter.drawLine(QPointF(c.x() - crossSize, c.y()), QPointF(c.x() + crossSize, c.y()));
                painter.drawLine(QPointF(c.x(), c.y() - crossSize), QPointF(c.x(), c.y() + crossSize));
                painter.setPen(entityPen);
            }
        }
    }
    
    // === DESSIN DES COTATIONS ===
    
    // Couleur courante pour cotations (modifiable: bleu pour manuelles, vert pour auto)
    QColor curDimColor(0, 100, 200);      // Bleu par défaut
    QColor curDimTextColor(0, 80, 180);
    
    // Helper: dessiner une flèche pleine
    auto drawArrow = [&](QPointF tip, double dirX, double dirY) {
        // dirX, dirY = direction de la pointe (en pixels page)
        double len = std::sqrt(dirX*dirX + dirY*dirY);
        if (len < 1e-6) return;
        dirX /= len; dirY /= len;
        double perpX = -dirY, perpY = dirX;
        
        QPolygonF arrow;
        arrow << tip;
        arrow << QPointF(tip.x() - dirX * arrowSize + perpX * arrowSize * 0.3,
                         tip.y() - dirY * arrowSize + perpY * arrowSize * 0.3);
        arrow << QPointF(tip.x() - dirX * arrowSize - perpX * arrowSize * 0.3,
                         tip.y() - dirY * arrowSize - perpY * arrowSize * 0.3);
        painter.setPen(Qt::NoPen);
        painter.setBrush(curDimColor);
        painter.drawPolygon(arrow);
        painter.setBrush(Qt::NoBrush);
    };
    
    // Helper: dessiner texte de cotation centré
    auto drawDimText = [&](QPointF pos, const QString& text) {
        painter.setFont(dimFont);
        painter.setPen(curDimTextColor);
        
        QFontMetricsF fm(dimFont);
        double tw = fm.horizontalAdvance(text);
        double th = fm.height();
        
        // Fond blanc semi-transparent pour lisibilité
        QRectF bgRect(pos.x() - tw/2 - 4, pos.y() - th/2 - 2, tw + 8, th + 4);
        painter.setBrush(QColor(255, 255, 255, 220));
        painter.setPen(Qt::NoPen);
        painter.drawRect(bgRect);
        painter.setBrush(Qt::NoBrush);
        
        painter.setPen(curDimTextColor);
        painter.drawText(bgRect, Qt::AlignCenter, text);
    };
    
    // Lambda: dessiner une liste de cotations (réutilisé pour manuelles et auto)
    auto renderDimList = [&](const std::vector<std::shared_ptr<CADEngine::Dimension>>& dims) {
        
        QPen curDimPen(curDimColor, dimLineW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        QPen curExtPen(curDimColor, dimLineW * 0.7, Qt::DashLine, Qt::RoundCap);
        
        for (const auto& dim : dims) {
            if (!dim || !dim->isVisible()) continue;
        
        // === COTATION LINÉAIRE ===
        if (dim->getType() == CADEngine::DimensionType::Linear) {
            auto ld = std::dynamic_pointer_cast<CADEngine::LinearDimension>(dim);
            if (!ld) continue;
            
            // Lignes d'attache
            painter.setPen(curExtPen);
            painter.drawLine(toPage(ld->getExtensionLine1Start().X(), ld->getExtensionLine1Start().Y()),
                             toPage(ld->getExtensionLine1End().X(), ld->getExtensionLine1End().Y()));
            painter.drawLine(toPage(ld->getExtensionLine2Start().X(), ld->getExtensionLine2Start().Y()),
                             toPage(ld->getExtensionLine2End().X(), ld->getExtensionLine2End().Y()));
            
            // Ligne de cote
            QPointF dlStart = toPage(ld->getDimensionLineStart().X(), ld->getDimensionLineStart().Y());
            QPointF dlEnd = toPage(ld->getDimensionLineEnd().X(), ld->getDimensionLineEnd().Y());
            painter.setPen(curDimPen);
            painter.drawLine(dlStart, dlEnd);
            
            // Flèches
            double dx = dlEnd.x() - dlStart.x();
            double dy = dlEnd.y() - dlStart.y();
            drawArrow(dlStart, dx, dy);
            drawArrow(dlEnd, -dx, -dy);
            
            // Texte
            QPointF textPos = toPage(ld->getTextPosition().X(), ld->getTextPosition().Y());
            drawDimText(textPos, QString::fromStdString(ld->getText()));
        }
        // === COTATION ANGULAIRE ===
        else if (dim->getType() == CADEngine::DimensionType::Angular) {
            auto ad = std::dynamic_pointer_cast<CADEngine::AngularDimension>(dim);
            if (!ad) continue;
            
            gp_Pnt2d center = ad->getCenter();
            double radius = ad->getRadius();
            double startAngle = ad->getStartAngle();
            double sweep = ad->getSweep();
            
            if (sweep < 1e-6) continue;
            
            // Arc de cotation
            painter.setPen(curDimPen);
            int segments = 50;
            QVector<QPointF> arcPts;
            for (int i = 0; i <= segments; i++) {
                double t = (double)i / segments;
                double angle = startAngle + t * sweep;
                double x = center.X() + radius * std::cos(angle);
                double y = center.Y() + radius * std::sin(angle);
                arcPts.append(toPage(x, y));
            }
            painter.drawPolyline(arcPts.data(), arcPts.size());
            
            // Lignes de repère
            double a1 = ad->getAngle1();
            double a2 = ad->getAngle2();
            double dist1 = center.Distance(ad->getPoint1());
            double dist2 = center.Distance(ad->getPoint2());
            
            painter.setPen(curExtPen);
            if (dist1 < radius) {
                painter.drawLine(toPage(ad->getPoint1().X(), ad->getPoint1().Y()),
                                 toPage(center.X() + radius * std::cos(a1),
                                        center.Y() + radius * std::sin(a1)));
            }
            if (dist2 < radius) {
                painter.drawLine(toPage(ad->getPoint2().X(), ad->getPoint2().Y()),
                                 toPage(center.X() + radius * std::cos(a2),
                                        center.Y() + radius * std::sin(a2)));
            }
            
            // Flèches tangentes
            QPointF arcStart = toPage(center.X() + radius * std::cos(startAngle),
                                      center.Y() + radius * std::sin(startAngle));
            double tanSx = -std::sin(startAngle);
            double tanSy = std::cos(startAngle);
            QPointF tanStartPage = toPage(center.X() + radius * std::cos(startAngle) + tanSx * 0.1,
                                          center.Y() + radius * std::sin(startAngle) + tanSy * 0.1);
            drawArrow(arcStart, tanStartPage.x() - arcStart.x(), tanStartPage.y() - arcStart.y());
            
            double endAngle = startAngle + sweep;
            QPointF arcEnd = toPage(center.X() + radius * std::cos(endAngle),
                                    center.Y() + radius * std::sin(endAngle));
            double tanEx = std::sin(endAngle);
            double tanEy = -std::cos(endAngle);
            QPointF tanEndPage = toPage(center.X() + radius * std::cos(endAngle) + tanEx * 0.1,
                                        center.Y() + radius * std::sin(endAngle) + tanEy * 0.1);
            drawArrow(arcEnd, tanEndPage.x() - arcEnd.x(), tanEndPage.y() - arcEnd.y());
            
            QPointF textPos = toPage(ad->getTextPosition().X(), ad->getTextPosition().Y());
            drawDimText(textPos, QString::fromStdString(ad->getText()));
        }
        // === COTATION RADIALE ===
        else if (dim->getType() == CADEngine::DimensionType::Radial) {
            auto rd = std::dynamic_pointer_cast<CADEngine::RadialDimension>(dim);
            if (!rd) continue;
            
            gp_Pnt2d center = rd->getCenter();
            gp_Pnt2d arrowPt = rd->getArrowPoint();
            
            painter.setPen(curDimPen);
            QPointF pCenter = toPage(center.X(), center.Y());
            QPointF pArrow = toPage(arrowPt.X(), arrowPt.Y());
            painter.drawLine(pCenter, pArrow);
            
            double dx = pArrow.x() - pCenter.x();
            double dy = pArrow.y() - pCenter.y();
            drawArrow(pArrow, dx, dy);
            
            QPointF textPos = toPage(rd->getTextPosition().X(), rd->getTextPosition().Y());
            drawDimText(textPos, QString::fromStdString(rd->getText()));
        }
        // === COTATION DIAMÉTRALE ===
        else if (dim->getType() == CADEngine::DimensionType::Diametral) {
            auto dd = std::dynamic_pointer_cast<CADEngine::DiametralDimension>(dim);
            if (!dd) continue;
            
            gp_Pnt2d center = dd->getCenter();
            double diameter = dd->getDiameter();
            double radius = diameter / 2.0;
            double angle = dd->getAngle();
            double angleRad = angle * M_PI / 180.0;
            double textOffset = 20.0;
            
            gp_Pnt2d start(center.X() - radius * std::cos(angleRad),
                           center.Y() - radius * std::sin(angleRad));
            gp_Pnt2d end(center.X() + (radius + textOffset - 5.0) * std::cos(angleRad),
                         center.Y() + (radius + textOffset - 5.0) * std::sin(angleRad));
            
            QPointF pStart = toPage(start.X(), start.Y());
            QPointF pEnd = toPage(end.X(), end.Y());
            
            painter.setPen(curDimPen);
            painter.drawLine(pStart, pEnd);
            
            double dx = pEnd.x() - pStart.x();
            double dy = pEnd.y() - pStart.y();
            drawArrow(pEnd, dx, dy);
            
            QPointF textPos = toPage(dd->getTextPosition().X(), dd->getTextPosition().Y());
            drawDimText(textPos, QString::fromStdString(dd->getText()));
        }
        
        } // for dims
    }; // renderDimList lambda
    
    // --- Cotations manuelles (bleu) ---
    curDimColor = QColor(0, 100, 200);
    curDimTextColor = QColor(0, 80, 180);
    renderDimList(sketch2D->getDimensions());
    
    // --- Auto-cotations (vert) ---
    sketch2D->regenerateAutoDimensions();
    auto autoDimsForPdf = sketch2D->getAutoDimensions();
    if (!autoDimsForPdf.empty()) {
        curDimColor = QColor(30, 150, 30);
        curDimTextColor = QColor(20, 120, 20);
        renderDimList(autoDimsForPdf);
    }
    
    // === CARTOUCHE en bas ===
    {
        painter.setPen(QPen(Qt::darkGray, 3));
        double cartoucheTop = pageH - cartoucheH;
        painter.drawLine(QPointF(pageW * 0.05, cartoucheTop + 5),
                         QPointF(pageW * 0.95, cartoucheTop + 5));
        
        QFont smallFont("Helvetica", 8);
        smallFont.setPixelSize((int)(cartoucheH * 0.35));
        painter.setFont(smallFont);
        painter.setPen(Qt::darkGray);
        
        int entityCount = (int)sketch2D->getEntities().size();
        int dimCount = (int)sketch2D->getDimensions().size();
        int autoDimCount = (int)sketch2D->getAutoDimensions().size();
        
        QString info = QString("CAD-ENGINE v0.8  |  %1  |  %2 entités  |  %3 cotations + %4 auto  |  Unités: mm")
            .arg(QString::fromStdString(sketch->getName()))
            .arg(entityCount)
            .arg(dimCount)
            .arg(autoDimCount);
        
        painter.drawText(QRectF(0, cartoucheTop + 5, pageW, cartoucheH - 5),
                         Qt::AlignCenter, info);
    }
    
    painter.end();
    
    m_statusLabel->setText(QString("Sketch exporté en PDF: %1").arg(fileName));
    QMessageBox::information(this, "Export réussi",
        QString("Le sketch a été exporté en PDF :\n%1").arg(fileName));
}

// ============================================================================
// Slots Tree
// ============================================================================

void MainWindow::onFeatureSelected(std::shared_ptr<CADEngine::Feature> feature) {
    if (!feature) return;
    
    onStatusMessage(QString("Feature sélectionnée: %1").arg(QString::fromStdString(feature->getName())));
    
    // TODO: Highlight dans le viewport 3D
}

void MainWindow::onFeatureDoubleClicked(std::shared_ptr<CADEngine::Feature> feature) {
    if (!feature) return;
    
    onStatusMessage(QString("Édition de: %1").arg(QString::fromStdString(feature->getName())));
    
    // Sketch → enter edit mode
    auto sketch = std::dynamic_pointer_cast<CADEngine::SketchFeature>(feature);
    if (sketch) {
        double uCenter = 0.0, vCenter = 0.0, autoZoom = 0.0;
        if (sketch->getSketch2D()) {
            uCenter = sketch->getSketch2D()->getFaceCenterU();
            vCenter = sketch->getSketch2D()->getFaceCenterV();
            autoZoom = sketch->getSketch2D()->getFaceAutoZoom();
        }
        m_viewport->enterSketchMode(sketch, uCenter, vCenter, autoZoom);
        return;
    }
    
    // Extrude → edit distance dialog
    auto extrude = std::dynamic_pointer_cast<CADEngine::ExtrudeFeature>(feature);
    if (extrude) {
        bool ok;
        double currentDist = extrude->getDistance();
        double newDist = QInputDialog::getDouble(this, "Modifier Extrusion",
            "Distance (mm) :", currentDist, -5000, 5000, 2, &ok);
        if (ok) {
            extrude->setDistance(newDist);
            // Re-set existing body for boolean ops
            TopoDS_Shape body;
            auto features = m_document->getAllFeatures();
            for (const auto& f : features) {
                if (f->getId() == extrude->getId()) break;  // Stop before current
                auto e = std::dynamic_pointer_cast<CADEngine::ExtrudeFeature>(f);
                if (e && e->isValid()) body = e->getResultShape();
                auto r = std::dynamic_pointer_cast<CADEngine::RevolveFeature>(f);
                if (r && r->isValid()) body = r->getResultShape();
                auto fi = std::dynamic_pointer_cast<CADEngine::Fillet3DFeature>(f);
                if (fi && fi->isValid()) body = fi->getResultShape();
                auto ch = std::dynamic_pointer_cast<CADEngine::Chamfer3DFeature>(f);
                if (ch && ch->isValid()) body = ch->getResultShape();
            }
            if (!body.IsNull()) extrude->setExistingBody(body);
            
            if (extrude->compute()) {
                m_viewport->invalidateTessCache();
                m_viewport->setCurrentBody(getAccumulatedBody());
                m_viewport->update();
                m_tree->refresh();
                onStatusMessage(QString("Extrusion modifiée : %1 mm").arg(newDist, 0, 'f', 1));
            }
        }
        return;
    }
    
    // Revolve → edit angle
    auto revolve = std::dynamic_pointer_cast<CADEngine::RevolveFeature>(feature);
    if (revolve) {
        bool ok;
        double currentAngle = revolve->getAngle();
        double newAngle = QInputDialog::getDouble(this, "Modifier Révolution",
            "Angle (°) :", currentAngle, -360, 360, 1, &ok);
        if (ok) {
            revolve->setAngle(newAngle);
            if (revolve->compute()) {
                m_viewport->invalidateTessCache();
                m_viewport->setCurrentBody(getAccumulatedBody());
                m_viewport->update();
                m_tree->refresh();
                onStatusMessage(QString("Révolution modifiée : %1°").arg(newAngle, 0, 'f', 1));
            }
        }
        return;
    }
    
    // Fillet → edit radius
    auto fillet = std::dynamic_pointer_cast<CADEngine::Fillet3DFeature>(feature);
    if (fillet) {
        bool ok;
        double currentRadius = fillet->getRadius();
        double newRadius = QInputDialog::getDouble(this, "Modifier Congé",
            "Rayon (mm) :", currentRadius, 0.1, 500, 2, &ok);
        if (ok) {
            fillet->setRadius(newRadius);
            if (fillet->compute()) {
                m_viewport->invalidateTessCache();
                m_viewport->setCurrentBody(getAccumulatedBody());
                m_viewport->update();
                m_tree->refresh();
                onStatusMessage(QString("Congé modifié : R=%1 mm").arg(newRadius, 0, 'f', 2));
            }
        }
        return;
    }
    
    // Chamfer → edit distance
    auto chamfer = std::dynamic_pointer_cast<CADEngine::Chamfer3DFeature>(feature);
    if (chamfer) {
        bool ok;
        double currentDist = chamfer->getDistance();
        double newDist = QInputDialog::getDouble(this, "Modifier Chanfrein",
            "Distance (mm) :", currentDist, 0.1, 500, 2, &ok);
        if (ok) {
            chamfer->setDistance(newDist);
            if (chamfer->compute()) {
                m_viewport->invalidateTessCache();
                m_viewport->setCurrentBody(getAccumulatedBody());
                m_viewport->update();
                m_tree->refresh();
                onStatusMessage(QString("Chanfrein modifié : D=%1 mm").arg(newDist, 0, 'f', 2));
            }
        }
        return;
    }
    
    // Shell → edit thickness
    auto shell = std::dynamic_pointer_cast<CADEngine::ShellFeature>(feature);
    if (shell) {
        bool ok;
        double currentThickness = shell->getThickness();
        double newThickness = QInputDialog::getDouble(this, "Modifier Coque",
            "Épaisseur (mm) :", currentThickness, 0.1, 100, 2, &ok);
        if (ok) {
            shell->setThickness(newThickness);
            if (shell->compute()) {
                m_viewport->invalidateTessCache();
                m_viewport->setCurrentBody(getAccumulatedBody());
                m_viewport->update();
                m_tree->refresh();
                onStatusMessage(QString("Coque modifiée : E=%1 mm").arg(newThickness, 0, 'f', 2));
            }
        }
        return;
    }
}

void MainWindow::onFeatureDeleted(std::shared_ptr<CADEngine::Feature> feature) {
    if (!feature) return;
    
    // Demander confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Supprimer feature",
        QString("Supprimer '%1' ?").arg(QString::fromStdString(feature->getName())),
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes) {
        // Supprimer du document
        if (m_document->removeFeature(feature->getId())) {
            onStatusMessage(QString("Feature supprimée: %1").arg(QString::fromStdString(feature->getName())));
            m_tree->refresh();
            m_viewport->update();
        } else {
            QMessageBox::warning(
                this,
                "Erreur",
                QString("Impossible de supprimer '%1'").arg(QString::fromStdString(feature->getName()))
            );
        }
    }
}

// ============================================================================
// Thèmes
// ============================================================================

void MainWindow::onThemeCycle() {
    static const struct { ThemeManager::Theme theme; const char* name; } themes[] = {
        { ThemeManager::Dark,     "Sombre" },
        { ThemeManager::Light,    "Clair" },
        { ThemeManager::BlueDark, "Bleu Nuit" },
        { ThemeManager::Graphite, "Graphite" },
    };
    
    // Trouver le thème actuel et passer au suivant
    ThemeManager::Theme current = ThemeManager::currentTheme();
    int next = 0;
    for (int i = 0; i < 4; i++) {
        if (themes[i].theme == current) { next = (i + 1) % 4; break; }
    }
    
    ThemeManager::applyTheme(themes[next].theme, qApp);
    refreshIcons();
    onStatusMessage(QString("Thème : %1").arg(themes[next].name));
}

void MainWindow::refreshIcons() {
    auto ic = ThemeManager::isDark() ? IconProvider::darkColors() : IconProvider::lightColors();
    
    // Outils dessin
    m_actToolSelect->setIcon(IconProvider::selectIcon(ic));
    m_actToolLine->setIcon(IconProvider::lineIcon(ic));
    m_actToolCircle->setIcon(IconProvider::circleIcon(ic));
    m_actToolRectangle->setIcon(IconProvider::rectangleIcon(ic));
    m_actToolArc->setIcon(IconProvider::arcIcon(ic));
    m_actToolPolyline->setIcon(IconProvider::polylineIcon(ic));
    m_actToolEllipse->setIcon(IconProvider::ellipseIcon(ic));
    m_actToolPolygon->setIcon(IconProvider::polygonIcon(ic));
    m_actToolArcCenter->setIcon(IconProvider::arcCenterIcon(ic));
    m_actToolDimension->setIcon(IconProvider::dimensionIcon(ic));
    
    // Contraintes
    m_actConstraintHorizontal->setIcon(IconProvider::constraintHIcon(ic));
    m_actConstraintVertical->setIcon(IconProvider::constraintVIcon(ic));
    m_actConstraintParallel->setIcon(IconProvider::constraintParallelIcon(ic));
    m_actConstraintPerpendicular->setIcon(IconProvider::constraintPerpIcon(ic));
    m_actConstraintCoincident->setIcon(IconProvider::constraintCoincidentIcon(ic));
    m_actConstraintConcentric->setIcon(IconProvider::constraintConcentricIcon(ic));
    m_actConstraintSymmetric->setIcon(IconProvider::constraintSymmetricIcon(ic));
    m_actConstraintFix->setIcon(IconProvider::constraintFixIcon(ic));
    m_actFillet->setIcon(IconProvider::filletIcon(ic));
    
    // Autres
    m_actCreateSketch->setIcon(IconProvider::sketchIcon(ic));
    m_actFinishSketch->setIcon(IconProvider::finishSketchIcon(ic));
    m_actToggleTree->setIcon(IconProvider::toggleTreeIcon(ic));
    m_actUndo->setIcon(IconProvider::undoIcon(ic));
    m_actRedo->setIcon(IconProvider::redoIcon(ic));
    
    // 3D Operations
    m_actExtrude->setIcon(IconProvider::extrudeIcon(ic));
    m_actRevolve->setIcon(IconProvider::revolveIcon(ic));
    m_actFillet3D->setIcon(IconProvider::fillet3DIcon(ic));
    m_actChamfer3D->setIcon(IconProvider::chamfer3DIcon(ic));
    m_actShell3D->setIcon(IconProvider::shellIcon(ic));
    m_actPushPull->setIcon(IconProvider::pushPullIcon(ic));
    m_actSweep->setIcon(IconProvider::sweepIcon(ic));
    m_actLoft->setIcon(IconProvider::loftIcon(ic));
    m_actLinearPattern->setIcon(IconProvider::linearPatternIcon(ic));
    m_actCircularPattern->setIcon(IconProvider::circularPatternIcon(ic));
    m_actThread->setIcon(IconProvider::threadIcon(ic));
    m_actSketchOnFace->setIcon(IconProvider::sketchOnFaceIcon(ic));
    
    // Arbre document — mettre à jour les couleurs
    m_tree->updateTheme();
    
    // Viewport — mettre à jour la couleur de fond
    m_viewport->updateThemeColors();
}

// ============================================================================
// Aide
// ============================================================================

void MainWindow::showHelpSketch() {
    QDialog dlg(this);
    dlg.setWindowTitle("Aide — Sketch");
    dlg.resize(520, 480);
    
    auto* layout = new QVBoxLayout(&dlg);
    auto* text = new QTextBrowser(&dlg);
    text->setOpenExternalLinks(false);
    text->setHtml(R"(
        <h2 style="color:#2a82da">Raccourcis Sketch</h2>
        <table cellpadding="4" cellspacing="0" width="100%">
        <tr><td><b>Esc</b></td><td>Mode sélection / Annuler outil en cours</td></tr>
        <tr><td><b>L</b></td><td>Outil Ligne</td></tr>
        <tr><td><b>C</b></td><td>Outil Cercle</td></tr>
        <tr><td><b>R</b></td><td>Outil Rectangle</td></tr>
        <tr><td><b>A</b></td><td>Outil Arc (3 points)</td></tr>
        <tr><td><b>Shift+A</b></td><td>Outil Arc depuis Centre</td></tr>
        <tr><td><b>P</b></td><td>Outil Polyligne</td></tr>
        <tr><td><b>E</b></td><td>Outil Ellipse</td></tr>
        <tr><td><b>G</b></td><td>Activer/désactiver ancrage grille</td></tr>
        <tr><td><b>D</b></td><td>Outil Cotation</td></tr>
        <tr><td><b>Suppr</b></td><td>Supprimer entité sélectionnée</td></tr>
        <tr><td><b>Ctrl+Z</b></td><td>Annuler</td></tr>
        <tr><td><b>Ctrl+Shift+Z</b></td><td>Rétablir</td></tr>
        </table>
        
        <h3 style="color:#2a82da">Méthodes de dessin</h3>
        <p><b>Ligne :</b> Clic point 1 → Clic point 2</p>
        <p><b>Cercle :</b> Clic centre → Clic rayon</p>
        <p><b>Rectangle :</b> Clic coin 1 → Clic coin opposé</p>
        <p><b>Arc :</b> Clic point 1 → Clic point 2 → Clic point milieu</p>
        <p><b>Polyligne :</b> Clics successifs → Double-clic ou Entrée pour finir</p>
        
        <h3 style="color:#2a82da">Cotations</h3>
        <p><b>Clic sur segment :</b> Cotation linéaire (longueur)</p>
        <p><b>Clic sur cercle :</b> Cotation diamétrale</p>
        <p><b>Shift+Clic sur segment :</b> Cotation angulaire entre 2 segments</p>
        <p><b>Double-clic cotation :</b> Modifier la valeur (paramétrique)</p>
        <p><b>Drag cotation :</b> Ajuster l'offset d'affichage</p>
        
        <h3 style="color:#2a82da">Contraintes</h3>
        <p><b>Horizontal/Vertical :</b> Clic sur un segment</p>
        <p><b>Parallèle/Perpendiculaire :</b> Clic segment 1 → Clic segment 2</p>
        <p><b>Coïncident :</b> Clic point 1 → Clic point 2</p>
        <p><b>Concentrique :</b> Clic cercle 1 → Clic cercle 2</p>
        <p><b>Symétrique :</b> Clic point 1 → Clic point 2 → Clic axe</p>
        <p><b>Verrouillage :</b> Clic sur un point (toggle)</p>
        <p><b>Congé :</b> Clic sur un vertex intérieur de polyline → saisie rayon</p>
        
        <h3 style="color:#2a82da">Snapping</h3>
        <p>Les outils s'accrochent automatiquement aux extrémités, sommets et centres 
        (affichés en cyan au survol). La grille est active par défaut.</p>
    )");
    
    layout->addWidget(text);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    layout->addWidget(buttons);
    
    dlg.exec();
}

void MainWindow::showHelpViewport() {
    QDialog dlg(this);
    dlg.setWindowTitle("Aide — Viewport 3D");
    dlg.resize(480, 400);
    
    auto* layout = new QVBoxLayout(&dlg);
    auto* text = new QTextBrowser(&dlg);
    text->setOpenExternalLinks(false);
    text->setHtml(R"(
        <h2 style="color:#2a82da">Navigation Viewport 3D</h2>
        <table cellpadding="4" cellspacing="0" width="100%">
        <tr><td><b>Clic droit + drag</b></td><td>Rotation de la vue</td></tr>
        <tr><td><b>Clic milieu + drag</b></td><td>Panoramique (pan)</td></tr>
        <tr><td><b>Molette</b></td><td>Zoom avant/arrière</td></tr>
        </table>
        
        <h3 style="color:#2a82da">ViewCube (en haut à droite)</h3>
        <p>Cliquez sur une face du cube pour orienter la vue rapidement :</p>
        <p><b>Haut :</b> Vue dessus (plan XY) — <b>Avant :</b> Vue frontale (plan XZ)</p>
        <p><b>Droite :</b> Vue latérale (plan YZ)</p>
        <p>Le cube tourne en temps réel avec la caméra.</p>
        
        <h3 style="color:#2a82da">Vues prédéfinies (menu Vue)</h3>
        <p><b>Vue Avant :</b> Face Y- vers la caméra</p>
        <p><b>Vue Dessus :</b> Regarde vers le bas (Z-)</p>
        <p><b>Vue Droite :</b> Face X+ vers la caméra</p>
        <p><b>Vue Isométrique :</b> 30° / 45° (vue classique)</p>
        
        <h3 style="color:#2a82da">Mode Sketch 2D</h3>
        <p>En mode sketch, la navigation change :</p>
        <p><b>Clic milieu + drag :</b> Pan 2D</p>
        <p><b>Molette :</b> Zoom 2D proportionnel</p>
        <p><b>Clic gauche :</b> Sélection / Outil actif</p>
        <p><b>Clic droit :</b> Menu contextuel (supprimer segment, etc.)</p>
        <p><b>Drag sur vertex :</b> Déplacement vertex (polyline, rectangle)</p>
        
        <h3 style="color:#2a82da">Opérations 3D (Fusion 360 style)</h3>
        <p><b>Ctrl+E :</b> Extrusion — distance positive = haut, négative = bas (poche)</p>
        <p><b>Ctrl+R :</b> Révolution — axes standard ou ligne du sketch</p>
        <p><b>Congé 3D :</b> Sélection arêtes interactive (surbrillance jaune au survol, vert = sélectionné)</p>
        <p><b>Chanfrein 3D :</b> Même principe que congé</p>
        <p><b>Balayage :</b> Extrusion d'un profil le long d'un chemin (2 sketches)</p>
        <p><b>Lissage :</b> Transition lissée entre 2+ profils</p>
        <p><b>Réseau linéaire :</b> Copies le long d'un axe X/Y/Z</p>
        <p><b>Réseau circulaire :</b> Copies autour d'un axe</p>
        <p><b>Filetage :</b> Filetage métrique ISO (standard ou fin)</p>
        <p><b>Sketch sur Face :</b> Clic → la face se surligne en bleu → clic = sketch auto-créé</p>
        <p>Opérations : Nouveau corps / Joindre / Couper / Intersection</p>
        <p><b>Echap :</b> Annuler la sélection de face ou d'arêtes</p>
    )");
    
    layout->addWidget(text);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    layout->addWidget(buttons);
    
    dlg.exec();
}

// ============================================================================
// Dialogues paramètres Polygone / Oblong
// ============================================================================

void MainWindow::showPolygonDialog(std::shared_ptr<CADEngine::SketchPolygon> polygon) {
    if (!polygon) return;
    
    QDialog dlg(this);
    dlg.setWindowTitle("Paramètres Polygone");
    dlg.setMinimumWidth(320);
    auto* layout = new QVBoxLayout(&dlg);
    auto* form = new QFormLayout();
    
    auto* spinSides = new QSpinBox();
    spinSides->setRange(3, 32);
    spinSides->setValue(polygon->getSides());
    
    auto* spinInscribed = new QDoubleSpinBox();
    spinInscribed->setRange(0.1, 100000.0);
    spinInscribed->setDecimals(2);
    spinInscribed->setSuffix(" mm");
    spinInscribed->setValue(polygon->getRadius() * 2.0);
    
    auto* spinCircumscribed = new QDoubleSpinBox();
    spinCircumscribed->setRange(0.1, 100000.0);
    spinCircumscribed->setDecimals(2);
    spinCircumscribed->setSuffix(" mm");
    double circDiam = polygon->getRadius() * 2.0 * std::cos(M_PI / polygon->getSides());
    spinCircumscribed->setValue(circDiam);
    
    form->addRow("Nombre de côtés:", spinSides);
    form->addRow("Ø entre sommets (inscrit):", spinInscribed);
    form->addRow("Ø entre plats (circonscrit):", spinCircumscribed);
    layout->addLayout(form);
    
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    
    bool updating = false;
    connect(spinInscribed, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        [&](double val) {
            if (updating) return;
            updating = true;
            double r = val / 2.0;
            spinCircumscribed->setValue(r * 2.0 * std::cos(M_PI / spinSides->value()));
            updating = false;
        });
    connect(spinCircumscribed, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        [&](double val) {
            if (updating) return;
            updating = true;
            double r = val / (2.0 * std::cos(M_PI / spinSides->value()));
            spinInscribed->setValue(r * 2.0);
            updating = false;
        });
    connect(spinSides, QOverload<int>::of(&QSpinBox::valueChanged),
        [&](int sides) {
            if (updating) return;
            updating = true;
            double r = spinInscribed->value() / 2.0;
            spinCircumscribed->setValue(r * 2.0 * std::cos(M_PI / sides));
            updating = false;
        });
    
    positionDialogRight(&dlg);
    if (dlg.exec() == QDialog::Accepted) {
        polygon->setSides(spinSides->value());
        polygon->setRadius(spinInscribed->value() / 2.0);
        m_viewport->update();
        m_statusLabel->setText(QString("Polygone: %1 côtés, Ø inscrit=%2, Ø plats=%3")
            .arg(spinSides->value())
            .arg(spinInscribed->value(), 0, 'f', 1)
            .arg(spinCircumscribed->value(), 0, 'f', 1));
    }
}

void MainWindow::showOblongDialog(std::shared_ptr<CADEngine::SketchOblong> oblong) {
    if (!oblong) return;
    
    QDialog dlg(this);
    dlg.setWindowTitle("Paramètres Oblong / Rainure");
    dlg.setMinimumWidth(320);
    auto* layout = new QVBoxLayout(&dlg);
    auto* form = new QFormLayout();
    
    auto* spinLength = new QDoubleSpinBox();
    spinLength->setRange(1.0, 100000.0);
    spinLength->setDecimals(2);
    spinLength->setSuffix(" mm");
    spinLength->setValue(oblong->getLength());
    
    auto* spinCenterDist = new QDoubleSpinBox();
    spinCenterDist->setRange(0.0, 100000.0);
    spinCenterDist->setDecimals(2);
    spinCenterDist->setSuffix(" mm");
    spinCenterDist->setValue(oblong->getLength() - oblong->getWidth());
    
    auto* spinWidth = new QDoubleSpinBox();
    spinWidth->setRange(1.0, 100000.0);
    spinWidth->setDecimals(2);
    spinWidth->setSuffix(" mm");
    spinWidth->setValue(oblong->getWidth());
    
    form->addRow("Longueur totale:", spinLength);
    form->addRow("Entre-axes:", spinCenterDist);
    form->addRow("Largeur (Ø arrondi):", spinWidth);
    layout->addLayout(form);
    
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    
    bool updating = false;
    connect(spinLength, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        [&](double L) {
            if (updating) return;
            updating = true;
            double W = spinWidth->value();
            if (W > L) { W = L; spinWidth->setValue(W); }
            spinCenterDist->setValue(L - W);
            updating = false;
        });
    connect(spinCenterDist, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        [&](double D) {
            if (updating) return;
            updating = true;
            double W = spinWidth->value();
            spinLength->setValue(D + W);
            updating = false;
        });
    connect(spinWidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        [&](double W) {
            if (updating) return;
            updating = true;
            double L = spinLength->value();
            if (W > L) { spinLength->setValue(W); L = W; }
            spinCenterDist->setValue(L - W);
            updating = false;
        });
    
    positionDialogRight(&dlg);
    if (dlg.exec() == QDialog::Accepted) {
        double L = spinLength->value();
        double W = spinWidth->value();
        if (W > L) W = L;
        oblong->setLength(L);
        oblong->setWidth(W);
        m_viewport->update();
        m_statusLabel->setText(QString("Oblong: L=%1, entre-axes=%2, largeur=%3")
            .arg(L, 0, 'f', 1)
            .arg(L - W, 0, 'f', 1)
            .arg(W, 0, 'f', 1));
    }
}

// ============================================================================
// 3D Operations - Helpers
// ============================================================================

std::shared_ptr<CADEngine::SketchFeature> MainWindow::getLastClosedSketch() {
    auto features = m_document->getAllFeatures();
    std::shared_ptr<CADEngine::SketchFeature> lastSketch;
    
    for (const auto& f : features) {
        if (f->getType() == CADEngine::FeatureType::Sketch) {
            auto sketch = std::dynamic_pointer_cast<CADEngine::SketchFeature>(f);
            if (sketch && sketch->getEntityCount() > 0) {
                // Check if already used by an extrude/revolve
                bool used = false;
                for (const auto& f2 : features) {
                    auto ext = std::dynamic_pointer_cast<CADEngine::ExtrudeFeature>(f2);
                    if (ext && ext->getSketchFeature() == sketch) { used = true; break; }
                    auto rev = std::dynamic_pointer_cast<CADEngine::RevolveFeature>(f2);
                    if (rev && rev->getSketchFeature() == sketch) { used = true; break; }
                }
                if (!used) lastSketch = sketch;
            }
        }
    }
    return lastSketch;
}

TopoDS_Shape MainWindow::getAccumulatedBody() {
    auto features = m_document->getAllFeatures();
    TopoDS_Shape body;
    
    for (const auto& f : features) {
        auto ext = std::dynamic_pointer_cast<CADEngine::ExtrudeFeature>(f);
        if (ext && ext->isValid()) body = ext->getResultShape();
        auto rev = std::dynamic_pointer_cast<CADEngine::RevolveFeature>(f);
        if (rev && rev->isValid()) body = rev->getResultShape();
        auto fil = std::dynamic_pointer_cast<CADEngine::Fillet3DFeature>(f);
        if (fil && fil->isValid()) body = fil->getResultShape();
        auto cham = std::dynamic_pointer_cast<CADEngine::Chamfer3DFeature>(f);
        if (cham && cham->isValid()) body = cham->getResultShape();
        auto sweep = std::dynamic_pointer_cast<CADEngine::SweepFeature>(f);
        if (sweep && sweep->isValid()) body = sweep->getResultShape();
        auto loft = std::dynamic_pointer_cast<CADEngine::LoftFeature>(f);
        if (loft && loft->isValid()) body = loft->getResultShape();
        auto lp = std::dynamic_pointer_cast<CADEngine::LinearPatternFeature>(f);
        if (lp && lp->isValid()) body = lp->getResultShape();
        auto cp = std::dynamic_pointer_cast<CADEngine::CircularPatternFeature>(f);
        if (cp && cp->isValid()) body = cp->getResultShape();
        auto thr = std::dynamic_pointer_cast<CADEngine::ThreadFeature>(f);
        if (thr && thr->isValid()) body = thr->getResultShape();
        auto shell = std::dynamic_pointer_cast<CADEngine::ShellFeature>(f);
        if (shell && shell->isValid()) body = shell->getResultShape();
        auto pp = std::dynamic_pointer_cast<CADEngine::PushPullFeature>(f);
        if (pp && pp->isValid()) body = pp->getResultShape();
    }
    return body;
}

// ============================================================================
// Extrusion Dialog (Fusion 360 style)
// ============================================================================

void MainWindow::onExtrude() {
    // Find available sketches
    auto sketch = getLastClosedSketch();
    if (!sketch) {
        auto features = m_document->getAllFeatures();
        QStringList sketchNames;
        std::vector<std::shared_ptr<CADEngine::SketchFeature>> sketches;
        for (const auto& f : features) {
            if (f->getType() == CADEngine::FeatureType::Sketch) {
                auto s = std::dynamic_pointer_cast<CADEngine::SketchFeature>(f);
                if (s && s->getEntityCount() > 0) {
                    sketches.push_back(s);
                    sketchNames << QString::fromStdString(s->getName()) + 
                        QString(" (%1 entités)").arg(s->getEntityCount());
                }
            }
        }
        if (sketches.empty()) {
            QMessageBox::information(this, "Extrusion", 
                "Aucun sketch disponible.\nCréez d'abord un sketch avec un profil fermé.");
            return;
        }
        if (sketches.size() == 1) {
            sketch = sketches[0];
        } else {
            bool ok;
            QString sel = QInputDialog::getItem(this, "Extrusion", "Sketch source :", sketchNames, 0, false, &ok);
            if (!ok) return;
            int idx = sketchNames.indexOf(sel);
            if (idx >= 0) sketch = sketches[idx];
        }
    }
    
    if (!sketch) return;
    
    // Check if sketch has profiles to pick from
    auto sketch2D = sketch->getSketch2D();
    if (!sketch2D) return;
    
    auto regions = sketch2D->buildFaceRegions();
    
    if (regions.empty()) {
        // Fallback: try detectClosedProfiles directly
        auto profiles = sketch2D->detectClosedProfiles();
        if (profiles.empty()) {
            QMessageBox::information(this, "Extrusion", 
                "Aucun profil fermé trouvé dans le sketch.\nVérifiez que les contours sont fermés.");
            return;
        }
        // Use first profile directly
        if (!profiles[0].face.IsNull()) {
            showExtrudeDialog(sketch, profiles[0].face, QString::fromStdString(profiles[0].description));
        } else {
            QMessageBox::information(this, "Extrusion", 
                "Impossible de construire une face à partir du profil.");
        }
        return;
    }
    
    if (regions.size() == 1) {
        // Single profile — go directly to dialog with it
        showExtrudeDialog(sketch, regions[0].face, QString::fromStdString(regions[0].description));
        return;
    }
    
    // Multiple profiles — enter viewport selection mode (Fusion 360 style)
    m_viewport->startProfileSelection(sketch);
    
    disconnect(m_viewport, &Viewport3D::profileSelected, this, nullptr);
    disconnect(m_viewport, &Viewport3D::profileSelectionCancelled, this, nullptr);
    disconnect(m_viewport, &Viewport3D::multiProfileSelected, this, nullptr);
    
    connect(m_viewport, &Viewport3D::profileSelectionCancelled, this, [this]() {
        disconnect(m_viewport, &Viewport3D::profileSelected, this, nullptr);
        disconnect(m_viewport, &Viewport3D::profileSelectionCancelled, this, nullptr);
        disconnect(m_viewport, &Viewport3D::multiProfileSelected, this, nullptr);
    });
    
    connect(m_viewport, &Viewport3D::profileSelected, this, 
        [this, sketch](TopoDS_Face face, QString description) {
            disconnect(m_viewport, &Viewport3D::profileSelected, this, nullptr);
            disconnect(m_viewport, &Viewport3D::profileSelectionCancelled, this, nullptr);
            disconnect(m_viewport, &Viewport3D::multiProfileSelected, this, nullptr);
            showExtrudeDialog(sketch, face, description);
        });
    
    connect(m_viewport, &Viewport3D::multiProfileSelected, this,
        [this, sketch](std::vector<TopoDS_Face> faces, QString description) {
            disconnect(m_viewport, &Viewport3D::profileSelected, this, nullptr);
            disconnect(m_viewport, &Viewport3D::profileSelectionCancelled, this, nullptr);
            disconnect(m_viewport, &Viewport3D::multiProfileSelected, this, nullptr);
            
            if (faces.empty()) return;
            if (faces.size() == 1) {
                showExtrudeDialog(sketch, faces[0], description);
                return;
            }
            
            // Multi-profile: show dialog once with first face, then apply to all
            // Get params from dialog, then create one extrusion per face
            showMultiExtrudeDialog(sketch, faces, description);
        });
}

void MainWindow::showExtrudeDialog(std::shared_ptr<CADEngine::SketchFeature> sketch,
                                    const TopoDS_Face& selectedFace,
                                    const QString& profileDesc) {
    QDialog dlg(this);
    dlg.setWindowTitle("Extrusion — Fusion 360");
    dlg.setMinimumWidth(380);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    
    // Profile info
    QString profInfo = profileDesc.isEmpty() ? "Auto" : profileDesc;
    QLabel* infoLabel = new QLabel(QString("Profil : %1").arg(profInfo));
    infoLabel->setStyleSheet("font-weight: bold; color: #4CAF50; padding: 4px;");
    mainLayout->addWidget(infoLabel);
    
    // === Direction ===
    QGroupBox* dirGroup = new QGroupBox("Direction");
    QFormLayout* dirLayout = new QFormLayout(dirGroup);
    
    QComboBox* cbDirection = new QComboBox();
    cbDirection->addItem("Un côté");
    cbDirection->addItem("Deux côtés");
    cbDirection->addItem("Symétrique");
    dirLayout->addRow("Type :", cbDirection);
    
    QDoubleSpinBox* spinDist1 = new QDoubleSpinBox();
    spinDist1->setRange(-5000.0, 5000.0);
    spinDist1->setValue(20.0);
    spinDist1->setSuffix(" mm");
    spinDist1->setDecimals(2);
    dirLayout->addRow("Distance :", spinDist1);
    
    QDoubleSpinBox* spinDist2 = new QDoubleSpinBox();
    spinDist2->setRange(0.01, 5000.0);
    spinDist2->setValue(20.0);
    spinDist2->setSuffix(" mm");
    spinDist2->setDecimals(2);
    spinDist2->setVisible(false);
    QLabel* lblDist2 = new QLabel("Distance 2 :");
    lblDist2->setVisible(false);
    dirLayout->addRow(lblDist2, spinDist2);
    
    mainLayout->addWidget(dirGroup);
    
    // === Opération ===
    QGroupBox* opGroup = new QGroupBox("Opération");
    QFormLayout* opLayout = new QFormLayout(opGroup);
    
    QComboBox* cbOperation = new QComboBox();
    cbOperation->addItem("Nouveau corps");
    cbOperation->addItem("Joindre (fusion)");
    cbOperation->addItem("Couper (poche)");
    cbOperation->addItem("Intersection");
    
    TopoDS_Shape existingBody = getAccumulatedBody();
    if (!existingBody.IsNull()) {
        cbOperation->setCurrentIndex(1);
    }
    
    opLayout->addRow("Type :", cbOperation);
    mainLayout->addWidget(opGroup);
    
    connect(cbDirection, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [&](int idx) {
            spinDist2->setVisible(idx == 1);
            lblDist2->setVisible(idx == 1);
        });
    
    QLabel* tipLabel = new QLabel(
        "💡 Distance positive = direction normale\n"
        "💡 Distance négative = direction opposée");
    tipLabel->setStyleSheet("color: gray; font-size: 11px; padding: 4px;");
    mainLayout->addWidget(tipLabel);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    positionDialogRight(&dlg);
    if (dlg.exec() != QDialog::Accepted) return;
    
    // === Create extrude feature ===
    double dist1 = spinDist1->value();
    double dist2 = spinDist2->value();
    int dirIdx = cbDirection->currentIndex();
    int opIdx = cbOperation->currentIndex();
    
    auto extrude = m_document->addFeature<CADEngine::ExtrudeFeature>("Extrusion");
    extrude->setSketchFeature(sketch);
    
    // Use the pre-selected face from viewport picking
    if (!selectedFace.IsNull()) {
        extrude->setSelectedFace(selectedFace);
    }
    
    if (dirIdx == 0) {
        extrude->setDirection(CADEngine::ExtrudeDirection::OneSide);
        extrude->setDistance(dist1);
    } else if (dirIdx == 1) {
        extrude->setDirection(CADEngine::ExtrudeDirection::TwoSides);
        extrude->setDistance(std::abs(dist1));
        extrude->setDistance2(std::abs(dist2));
    } else {
        extrude->setDirection(CADEngine::ExtrudeDirection::Symmetric);
        extrude->setDistance(std::abs(dist1));
    }
    
    extrude->setOperation(static_cast<CADEngine::ExtrudeOperation>(opIdx));
    
    if (!existingBody.IsNull() && opIdx > 0) {
        extrude->setExistingBody(existingBody);
    }
    
    if (extrude->compute()) {
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        onStatusMessage(QString("Extrusion créée : %1 mm — %2").arg(dist1, 0, 'f', 1).arg(profInfo));
    } else {
        m_document->removeFeature(extrude->getId());
        QMessageBox::warning(this, "Extrusion", 
            "Échec de l'extrusion.\nVérifiez que le sketch contient un profil fermé.");
    }
}

void MainWindow::showMultiExtrudeDialog(std::shared_ptr<CADEngine::SketchFeature> sketch,
                                         const std::vector<TopoDS_Face>& faces,
                                         const QString& profileDesc) {
    if (faces.empty()) return;
    
    QDialog dlg(this);
    dlg.setWindowTitle("Extrusion Multi-Profils");
    dlg.setMinimumWidth(380);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    
    QLabel* infoLabel = new QLabel(QString("Profils sélectionnés : %1").arg(faces.size()));
    infoLabel->setStyleSheet("font-weight: bold; color: #4CAF50; padding: 4px;");
    mainLayout->addWidget(infoLabel);
    
    // Direction
    QGroupBox* dirGroup = new QGroupBox("Direction");
    QFormLayout* dirLayout = new QFormLayout(dirGroup);
    
    QComboBox* cbDirection = new QComboBox();
    cbDirection->addItem("Un côté");
    cbDirection->addItem("Deux côtés");
    cbDirection->addItem("Symétrique");
    dirLayout->addRow("Type :", cbDirection);
    
    QDoubleSpinBox* spinDist1 = new QDoubleSpinBox();
    spinDist1->setRange(-5000.0, 5000.0);
    spinDist1->setValue(20.0);
    spinDist1->setSuffix(" mm");
    spinDist1->setDecimals(2);
    dirLayout->addRow("Distance :", spinDist1);
    
    QDoubleSpinBox* spinDist2 = new QDoubleSpinBox();
    spinDist2->setRange(0.01, 5000.0);
    spinDist2->setValue(20.0);
    spinDist2->setSuffix(" mm");
    spinDist2->setDecimals(2);
    spinDist2->setVisible(false);
    QLabel* lblDist2 = new QLabel("Distance 2 :");
    lblDist2->setVisible(false);
    dirLayout->addRow(lblDist2, spinDist2);
    
    mainLayout->addWidget(dirGroup);
    
    connect(cbDirection, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int idx) {
        lblDist2->setVisible(idx == 1);
        spinDist2->setVisible(idx == 1);
    });
    
    // Operation
    QGroupBox* opGroup = new QGroupBox("Opération");
    QFormLayout* opLayout = new QFormLayout(opGroup);
    QComboBox* cbOperation = new QComboBox();
    cbOperation->addItem("Nouveau corps");
    cbOperation->addItem("Joindre (union)");
    cbOperation->addItem("Couper (soustraction)");
    cbOperation->addItem("Intersection");
    opLayout->addRow("Type :", cbOperation);
    mainLayout->addWidget(opGroup);
    
    // Check for existing body
    TopoDS_Shape existingBody = getAccumulatedBody();
    if (existingBody.IsNull()) {
        cbOperation->setCurrentIndex(0);
        cbOperation->setEnabled(false);
    } else {
        cbOperation->setCurrentIndex(1);
    }
    
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    positionDialogRight(&dlg);
    if (dlg.exec() != QDialog::Accepted) return;
    
    // Create one extrusion per face with same parameters
    double dist1 = spinDist1->value();
    double dist2 = spinDist2->value();
    int dirIdx = cbDirection->currentIndex();
    int opIdx = cbOperation->currentIndex();
    
    int successCount = 0;
    for (size_t i = 0; i < faces.size(); i++) {
        QString name = QString("Extrusion_%1").arg(i + 1);
        auto extrude = m_document->addFeature<CADEngine::ExtrudeFeature>(name.toStdString());
        extrude->setSketchFeature(sketch);
        extrude->setSelectedFace(faces[i]);
        
        if (dirIdx == 0) {
            extrude->setDirection(CADEngine::ExtrudeDirection::OneSide);
            extrude->setDistance(dist1);
        } else if (dirIdx == 1) {
            extrude->setDirection(CADEngine::ExtrudeDirection::TwoSides);
            extrude->setDistance(std::abs(dist1));
            extrude->setDistance2(std::abs(dist2));
        } else {
            extrude->setDirection(CADEngine::ExtrudeDirection::Symmetric);
            extrude->setDistance(std::abs(dist1));
        }
        
        extrude->setOperation(static_cast<CADEngine::ExtrudeOperation>(opIdx));
        
        if (!existingBody.IsNull() && opIdx > 0) {
            extrude->setExistingBody(existingBody);
        }
        
        if (extrude->compute()) {
            successCount++;
            // Update existing body for subsequent operations
            existingBody = getAccumulatedBody();
        } else {
            m_document->removeFeature(extrude->getId());
        }
    }
    
    if (successCount > 0) {
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        onStatusMessage(QString("%1/%2 extrusions créées").arg(successCount).arg(faces.size()));
    } else {
        QMessageBox::warning(this, "Extrusion", "Aucune extrusion n'a pu être créée.");
    }
}

// ============================================================================
// Revolution Dialog
// ============================================================================

void MainWindow::onRevolve() {
    auto sketch = getLastClosedSketch();
    if (!sketch) {
        auto features = m_document->getAllFeatures();
        QStringList sketchNames;
        std::vector<std::shared_ptr<CADEngine::SketchFeature>> sketches;
        for (const auto& f : features) {
            if (f->getType() == CADEngine::FeatureType::Sketch) {
                auto s = std::dynamic_pointer_cast<CADEngine::SketchFeature>(f);
                if (s && s->getEntityCount() > 0) {
                    sketches.push_back(s);
                    sketchNames << QString::fromStdString(s->getName());
                }
            }
        }
        if (sketches.empty()) {
            QMessageBox::information(this, "Révolution", 
                "Aucun sketch disponible.\nCréez d'abord un sketch.");
            return;
        }
        bool ok;
        QString sel = QInputDialog::getItem(this, "Révolution", "Sketch :", sketchNames, 0, false, &ok);
        if (!ok) return;
        int idx = sketchNames.indexOf(sel);
        if (idx >= 0) sketch = sketches[idx];
    }
    if (!sketch) return;
    showRevolveDialog(sketch);
}

void MainWindow::showRevolveDialog(std::shared_ptr<CADEngine::SketchFeature> sketch) {
    QDialog dlg(this);
    dlg.setWindowTitle("Révolution — Fusion 360");
    dlg.setMinimumWidth(380);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    
    QLabel* infoLabel = new QLabel(QString("Profil : %1 (%2 entités)")
        .arg(QString::fromStdString(sketch->getName()))
        .arg(sketch->getEntityCount()));
    infoLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    mainLayout->addWidget(infoLabel);
    
    QGroupBox* paramGroup = new QGroupBox("Paramètres");
    QFormLayout* form = new QFormLayout(paramGroup);
    
    QDoubleSpinBox* spinAngle = new QDoubleSpinBox();
    spinAngle->setRange(-360.0, 360.0);
    spinAngle->setValue(360.0);
    spinAngle->setSuffix(" °");
    spinAngle->setDecimals(1);
    form->addRow("Angle :", spinAngle);
    
    // Axis selection — includes sketch lines as potential axes
    QComboBox* cbAxis = new QComboBox();
    cbAxis->addItem("Axe X du sketch");
    cbAxis->addItem("Axe Y du sketch");
    cbAxis->addItem("Axe X global");
    cbAxis->addItem("Axe Y global");
    
    // Add sketch lines as axis options
    struct LineAxisInfo { gp_Pnt2d start; gp_Pnt2d end; };
    std::vector<LineAxisInfo> lineAxes;
    
    auto sketch2D = sketch->getSketch2D();
    if (sketch2D) {
        int lineIdx = 1;
        for (const auto& entity : sketch2D->getEntities()) {
            auto line = std::dynamic_pointer_cast<CADEngine::SketchLine>(entity);
            if (line) {
                double dx = line->getEnd().X() - line->getStart().X();
                double dy = line->getEnd().Y() - line->getStart().Y();
                QString desc;
                if (std::abs(dy) < 0.01) desc = "horizontale";
                else if (std::abs(dx) < 0.01) desc = "verticale";
                else desc = QString("(%.1f,%.1f)→(%.1f,%.1f)")
                    .arg(line->getStart().X()).arg(line->getStart().Y())
                    .arg(line->getEnd().X()).arg(line->getEnd().Y());
                cbAxis->addItem(QString("Ligne %1 (%2)").arg(lineIdx).arg(desc));
                lineAxes.push_back({line->getStart(), line->getEnd()});
                lineIdx++;
            }
        }
    }
    form->addRow("Axe :", cbAxis);
    
    QComboBox* cbOperation = new QComboBox();
    cbOperation->addItem("Nouveau corps");
    cbOperation->addItem("Joindre");
    cbOperation->addItem("Couper");
    cbOperation->addItem("Intersection");
    
    TopoDS_Shape existingBody = getAccumulatedBody();
    if (!existingBody.IsNull()) cbOperation->setCurrentIndex(1);
    
    form->addRow("Opération :", cbOperation);
    mainLayout->addWidget(paramGroup);
    
    // Tip
    QLabel* tipLabel = new QLabel(
        "💡 Angle négatif = révolution dans le sens inverse\n"
        "💡 Sélectionnez une ligne du sketch comme axe pour plus de contrôle");
    tipLabel->setStyleSheet("color: gray; font-size: 11px; padding: 4px;");
    mainLayout->addWidget(tipLabel);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    positionDialogRight(&dlg);
    if (dlg.exec() != QDialog::Accepted) return;
    
    auto revolve = m_document->addFeature<CADEngine::RevolveFeature>("Révolution");
    revolve->setSketchFeature(sketch);
    revolve->setAngle(spinAngle->value());
    
    int axisIdx = cbAxis->currentIndex();
    if (axisIdx < 4) {
        // Standard axes
        CADEngine::RevolveAxisType axisType;
        switch (axisIdx) {
            case 0: axisType = CADEngine::RevolveAxisType::SketchX; break;
            case 1: axisType = CADEngine::RevolveAxisType::SketchY; break;
            case 2: axisType = CADEngine::RevolveAxisType::XAxis; break;
            case 3: axisType = CADEngine::RevolveAxisType::YAxis; break;
            default: axisType = CADEngine::RevolveAxisType::SketchX; break;
        }
        revolve->setAxisType(axisType);
    } else {
        // Sketch line as custom axis
        int lineIndex = axisIdx - 4;
        if (lineIndex >= 0 && lineIndex < (int)lineAxes.size()) {
            const auto& la = lineAxes[lineIndex];
            gp_Pln plane = sketch2D->getPlane();
            
            // Convert 2D line to 3D axis
            gp_Pnt origin3D = plane.Location();
            gp_Dir xDir = plane.XAxis().Direction();
            gp_Dir yDir = plane.YAxis().Direction();
            
            gp_Pnt p1_3d(origin3D.XYZ() + la.start.X() * xDir.XYZ() + la.start.Y() * yDir.XYZ());
            gp_Pnt p2_3d(origin3D.XYZ() + la.end.X() * xDir.XYZ() + la.end.Y() * yDir.XYZ());
            
            gp_Vec axisVec(p1_3d, p2_3d);
            if (axisVec.Magnitude() > 1e-6) {
                gp_Ax1 customAxis(p1_3d, gp_Dir(axisVec));
                revolve->setAxisType(CADEngine::RevolveAxisType::CustomLine);
                revolve->setCustomAxis(customAxis);
            } else {
                revolve->setAxisType(CADEngine::RevolveAxisType::SketchX);
            }
        }
    }
    
    int opIdx = cbOperation->currentIndex();
    revolve->setOperation(static_cast<CADEngine::ExtrudeOperation>(opIdx));
    if (!existingBody.IsNull() && opIdx > 0) {
        revolve->setExistingBody(existingBody);
    }
    
    if (revolve->compute()) {
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        onStatusMessage(QString("Révolution créée : %1° autour de %2")
            .arg(spinAngle->value(), 0, 'f', 1)
            .arg(cbAxis->currentText()));
    } else {
        m_document->removeFeature(revolve->getId());
        QMessageBox::warning(this, "Révolution", 
            "Échec de la révolution.\nVérifiez le profil et l'axe de rotation.\n"
            "Le profil ne doit pas traverser l'axe de rotation.");
    }
}

// ============================================================================
// Fillet 3D Dialog
// ============================================================================

void MainWindow::onFillet3D() {
    TopoDS_Shape body = getAccumulatedBody();
    if (body.IsNull()) {
        QMessageBox::information(this, "Congé 3D", "Aucun solide existant.");
        return;
    }
    
    if (!m_viewport->getSelectedEdges3D().empty()) {
        showFillet3DDialog();
    } else {
        m_pendingEdgeOp = PendingEdgeOp::Fillet;
        m_viewport->startEdgeSelection();
        onStatusMessage("Congé 3D — Cliquez sur les arêtes à arrondir, puis Entrée pour valider (Échap = annuler)");
    }
}

void MainWindow::showFillet3DDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle("Congé 3D");
    dlg.setMinimumWidth(320);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    
    QFormLayout* form = new QFormLayout();
    
    QDoubleSpinBox* spinRadius = new QDoubleSpinBox();
    spinRadius->setRange(0.1, 500.0);
    spinRadius->setValue(2.0);
    spinRadius->setSuffix(" mm");
    spinRadius->setDecimals(2);
    form->addRow("Rayon :", spinRadius);
    
    QCheckBox* cbAllEdges = new QCheckBox("Toutes les arêtes");
    form->addRow(cbAllEdges);
    
    mainLayout->addLayout(form);
    
    // Edge selection instruction
    QLabel* tipLabel = new QLabel(
        "Pour sélectionner des arêtes :\n"
        "• Cochez 'Toutes les arêtes', ou\n"
        "• Cliquez sur les arêtes dans la vue 3D\n"
        "  avant d'ouvrir ce dialogue");
    tipLabel->setStyleSheet("color: gray; font-size: 11px; padding: 4px;");
    mainLayout->addWidget(tipLabel);
    
    // Show count of pre-selected edges
    auto preSelected = m_viewport->getSelectedEdges3D();
    if (!preSelected.empty()) {
        QLabel* selLabel = new QLabel(
            QString("✓ %1 arête(s) pré-sélectionnée(s)").arg(preSelected.size()));
        selLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
        mainLayout->addWidget(selLabel);
    }
    
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    positionDialogRight(&dlg);
    if (dlg.exec() != QDialog::Accepted) return;
    
    TopoDS_Shape body = getAccumulatedBody();
    
    auto fillet = m_document->addFeature<CADEngine::Fillet3DFeature>("Congé 3D");
    fillet->setBaseShape(body);
    fillet->setRadius(spinRadius->value());
    
    if (cbAllEdges->isChecked()) {
        fillet->setAllEdges(true);
    } else {
        auto edges = m_viewport->getSelectedEdges3D();
        if (edges.empty()) {
            QMessageBox::warning(this, "Congé 3D", "Aucune arête sélectionnée.\nCochez 'Toutes les arêtes' ou sélectionnez des arêtes d'abord.");
            m_document->removeFeature(fillet->getId());
            return;
        }
        for (const auto& e : edges) {
            fillet->addEdge(e);
        }
    }
    
    if (fillet->compute()) {
        m_viewport->clearSelectedEdges3D();
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        onStatusMessage(QString("Congé 3D : R=%1 mm").arg(spinRadius->value(), 0, 'f', 2));
    } else {
        m_document->removeFeature(fillet->getId());
        QMessageBox::warning(this, "Congé 3D", 
            "Échec du congé.\nLe rayon est peut-être trop grand pour les arêtes sélectionnées.");
    }
}

// ============================================================================
// Chamfer 3D Dialog
// ============================================================================

void MainWindow::onChamfer3D() {
    TopoDS_Shape body = getAccumulatedBody();
    if (body.IsNull()) {
        QMessageBox::information(this, "Chanfrein 3D", "Aucun solide existant.");
        return;
    }
    
    if (!m_viewport->getSelectedEdges3D().empty()) {
        showChamfer3DDialog();
    } else {
        m_pendingEdgeOp = PendingEdgeOp::Chamfer;
        m_viewport->startEdgeSelection();
        onStatusMessage("Chanfrein 3D — Cliquez sur les arêtes à chanfreiner, puis Entrée pour valider (Échap = annuler)");
    }
}

void MainWindow::showChamfer3DDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle("Chanfrein 3D");
    dlg.setMinimumWidth(320);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    
    QFormLayout* form = new QFormLayout();
    
    QDoubleSpinBox* spinDist = new QDoubleSpinBox();
    spinDist->setRange(0.1, 500.0);
    spinDist->setValue(2.0);
    spinDist->setSuffix(" mm");
    spinDist->setDecimals(2);
    form->addRow("Distance :", spinDist);
    
    QCheckBox* cbAllEdges = new QCheckBox("Toutes les arêtes");
    form->addRow(cbAllEdges);
    
    mainLayout->addLayout(form);
    
    auto preSelected = m_viewport->getSelectedEdges3D();
    if (!preSelected.empty()) {
        QLabel* selLabel = new QLabel(
            QString("✓ %1 arête(s) pré-sélectionnée(s)").arg(preSelected.size()));
        selLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
        mainLayout->addWidget(selLabel);
    }
    
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    positionDialogRight(&dlg);
    if (dlg.exec() != QDialog::Accepted) return;
    
    TopoDS_Shape body = getAccumulatedBody();
    
    auto chamfer = m_document->addFeature<CADEngine::Chamfer3DFeature>("Chanfrein 3D");
    chamfer->setBaseShape(body);
    chamfer->setDistance(spinDist->value());
    
    if (cbAllEdges->isChecked()) {
        chamfer->setAllEdges(true);
    } else {
        auto edges = m_viewport->getSelectedEdges3D();
        if (edges.empty()) {
            QMessageBox::warning(this, "Chanfrein 3D", "Aucune arête sélectionnée.\nCochez 'Toutes les arêtes' ou sélectionnez des arêtes d'abord.");
            m_document->removeFeature(chamfer->getId());
            return;
        }
        for (const auto& e : edges) {
            chamfer->addEdge(e);
        }
    }
    
    if (chamfer->compute()) {
        m_viewport->clearSelectedEdges3D();
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        onStatusMessage(QString("Chanfrein 3D : D=%1 mm").arg(spinDist->value(), 0, 'f', 2));
    } else {
        m_document->removeFeature(chamfer->getId());
        QMessageBox::warning(this, "Chanfrein 3D", 
            "Échec du chanfrein.\nLa distance est peut-être trop grande.");
    }
}

// ============================================================================
// Shell (Coque) Dialog
// ============================================================================

void MainWindow::onShell3D() {
    TopoDS_Shape body = getAccumulatedBody();
    if (body.IsNull()) {
        QMessageBox::information(this, "Coque", "Aucun solide existant.");
        return;
    }
    
    // Entrer en mode sélection de face
    m_viewport->startFaceSelection();
    onStatusMessage("Coque — Cliquez sur la face à ouvrir, puis Entrée pour valider");
    
    // Attendre la sélection de face
    // On utilise un dialogue non-modal : quand l'utilisateur a sélectionné une face et appuie Entrée,
    // le signal faceSelected est émis
    
    // Temporairement, on utilise un dialogue bloquant avec instructions
    QDialog dlg(this);
    dlg.setWindowTitle("Coque (Shell)");
    dlg.setMinimumWidth(320);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    
    QLabel* instr = new QLabel(
        "1. Cliquez sur la face à ouvrir dans la vue 3D\n"
        "2. Réglez l'épaisseur\n"
        "3. Cliquez OK");
    instr->setStyleSheet("color: #aaa; padding: 4px;");
    mainLayout->addWidget(instr);
    
    QFormLayout* form = new QFormLayout();
    
    QDoubleSpinBox* spinThickness = new QDoubleSpinBox();
    spinThickness->setRange(0.1, 100.0);
    spinThickness->setValue(2.0);
    spinThickness->setSuffix(" mm");
    spinThickness->setDecimals(2);
    form->addRow("Épaisseur :", spinThickness);
    
    QComboBox* cbDirection = new QComboBox();
    cbDirection->addItem("Vers l'intérieur");
    cbDirection->addItem("Vers l'extérieur");
    cbDirection->setCurrentIndex(0);
    form->addRow("Direction :", cbDirection);
    mainLayout->addLayout(form);
    
    // Label de face sélectionnée — mis à jour en temps réel
    QLabel* faceLabel = new QLabel("Aucune face sélectionnée");
    faceLabel->setStyleSheet("color: orange; font-weight: bold; padding: 4px;");
    mainLayout->addWidget(faceLabel);
    
    // Connecter le signal faceSelected pour mise à jour du label
    TopoDS_Face selectedFace;
    auto conn = connect(m_viewport, &Viewport3D::faceSelected, [&](const TopoDS_Face& face) {
        selectedFace = face;
        faceLabel->setText("✓ Face sélectionnée");
        faceLabel->setStyleSheet("color: #4CAF50; font-weight: bold; padding: 4px;");
    });
    
    // Si une face est déjà hover, la pré-sélectionner
    if (m_viewport->hasSelectedFace()) {
        selectedFace = m_viewport->getSelectedFace();
        faceLabel->setText("✓ Face pré-sélectionnée");
        faceLabel->setStyleSheet("color: #4CAF50; font-weight: bold; padding: 4px;");
    }
    
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    positionDialogRight(&dlg);
    int result = dlg.exec();
    
    disconnect(conn);
    m_viewport->stopFaceSelection();
    
    if (result != QDialog::Accepted) return;
    
    if (selectedFace.IsNull()) {
        QMessageBox::warning(this, "Coque", "Aucune face sélectionnée.");
        return;
    }
    
    auto shell = m_document->addFeature<CADEngine::ShellFeature>("Coque");
    shell->setExistingBody(body);
    shell->addOpenFace(selectedFace);
    shell->setThickness(spinThickness->value());
    shell->setOutward(cbDirection->currentIndex() == 1);
    
    if (shell->compute()) {
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        QString dir = shell->isOutward() ? "extérieur" : "intérieur";
        onStatusMessage(QString("Coque : %1 mm vers %2").arg(spinThickness->value(), 0, 'f', 2).arg(dir));
    } else {
        m_document->removeFeature(shell->getId());
        QMessageBox::warning(this, "Coque", 
            "Échec de la coque.\nL'épaisseur est peut-être trop grande pour cette géométrie.");
    }
}

// ============================================================================
// Push/Pull (Appuyer/Tirer une face)
// ============================================================================

void MainWindow::onPushPull() {
    TopoDS_Shape body = getAccumulatedBody();
    if (body.IsNull()) {
        QMessageBox::information(this, "Appuyer/Tirer", "Aucun solide existant.");
        return;
    }
    
    // Entrer en mode sélection de face
    m_viewport->startFaceSelection();
    onStatusMessage("Appuyer/Tirer — Cliquez sur la face à déplacer");
    
    QDialog dlg(this);
    dlg.setWindowTitle("Appuyer / Tirer");
    dlg.setMinimumWidth(350);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    
    QLabel* instr = new QLabel(
        "Cliquez sur une face dans la vue 3D\n"
        "+ = Tirer (ajouter matière)\n"
        "− = Pousser (retirer matière)");
    instr->setStyleSheet("color: #aaa; padding: 4px;");
    mainLayout->addWidget(instr);
    
    QFormLayout* form = new QFormLayout();
    
    QDoubleSpinBox* spinDist = new QDoubleSpinBox();
    spinDist->setRange(-500.0, 500.0);
    spinDist->setValue(10.0);
    spinDist->setSuffix(" mm");
    spinDist->setDecimals(2);
    form->addRow("Distance :", spinDist);
    mainLayout->addLayout(form);
    
    QLabel* faceLabel = new QLabel("Aucune face sélectionnée");
    faceLabel->setStyleSheet("color: orange; font-weight: bold; padding: 4px;");
    mainLayout->addWidget(faceLabel);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    // Sélection de face + preview live
    TopoDS_Face selectedFace;
    gp_Dir faceNormal(0,0,1);
    
    auto computeNormal = [this](const TopoDS_Face& face) -> gp_Dir {
        try {
            BRepAdaptor_Surface adaptor(face);
            double uMid = (adaptor.FirstUParameter() + adaptor.LastUParameter()) / 2.0;
            double vMid = (adaptor.FirstVParameter() + adaptor.LastVParameter()) / 2.0;
            gp_Pnt point;
            gp_Vec du, dv;
            adaptor.D1(uMid, vMid, point, du, dv);
            gp_Vec normal = du.Crossed(dv);
            if (normal.Magnitude() > 1e-10) {
                normal.Normalize();
                // Orienter vers la caméra (pas le flag Orientation qui est instable après Shell)
                gp_Dir viewDir = m_viewport->getCameraViewDirection();
                double dot = normal.X()*viewDir.X() + normal.Y()*viewDir.Y() + normal.Z()*viewDir.Z();
                if (dot > 0) normal.Reverse();
                return gp_Dir(normal);
            }
        } catch (...) {}
        return gp_Dir(0,0,1);
    };
    
    auto updatePreview = [&]() {
        if (selectedFace.IsNull()) {
            m_viewport->clearPatternPreview();
            return;
        }
        double dist = spinDist->value();
        if (std::abs(dist) < 0.01) {
            m_viewport->clearPatternPreview();
            return;
        }
        
        gp_Vec vec(faceNormal);
        vec.Scale(dist);
        
        try {
            BRepPrimAPI_MakePrism prism(selectedFace, vec);
            prism.Build();
            if (prism.IsDone()) {
                std::vector<TopoDS_Shape> preview = {prism.Shape()};
                m_viewport->setPatternPreview(preview, dist < 0);
            }
        } catch (...) {}
    };
    
    auto connFace = connect(m_viewport, &Viewport3D::faceSelected, [&](const TopoDS_Face& face) {
        selectedFace = face;
        faceNormal = computeNormal(face);
        faceLabel->setText("✓ Face sélectionnée");
        faceLabel->setStyleSheet("color: #4CAF50; font-weight: bold; padding: 4px;");
        updatePreview();
    });
    
    // Si une face est déjà sous le curseur
    if (m_viewport->hasSelectedFace()) {
        selectedFace = m_viewport->getSelectedFace();
        faceNormal = computeNormal(selectedFace);
        faceLabel->setText("✓ Face pré-sélectionnée");
        faceLabel->setStyleSheet("color: #4CAF50; font-weight: bold; padding: 4px;");
    }
    
    connect(spinDist, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double) { updatePreview(); });
    
    // Preview initial
    updatePreview();
    
    positionDialogRight(&dlg);
    int result = dlg.exec();
    
    disconnect(connFace);
    m_viewport->stopFaceSelection();
    m_viewport->clearPatternPreview();
    
    if (result != QDialog::Accepted) return;
    
    if (selectedFace.IsNull()) {
        QMessageBox::warning(this, "Appuyer/Tirer", "Aucune face sélectionnée.");
        return;
    }
    
    double dist = spinDist->value();
    if (std::abs(dist) < 0.01) return;
    
    auto pp = m_document->addFeature<CADEngine::PushPullFeature>(
        dist > 0 ? "Tirer face" : "Pousser face");
    pp->setExistingBody(body);
    gp_Dir camDir = m_viewport->getCameraViewDirection();
    pp->setFace(selectedFace, &camDir);
    pp->setDistance(dist);
    
    if (pp->compute()) {
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        onStatusMessage(QString("%1 : %2 mm")
            .arg(dist > 0 ? "Tirer" : "Pousser")
            .arg(std::abs(dist), 0, 'f', 2));
    } else {
        m_document->removeFeature(pp->getId());
        QMessageBox::warning(this, "Appuyer/Tirer",
            "Échec de l'opération.\nLa face n'est peut-être pas compatible.");
    }
}

// ============================================================================
// Sweep Dialog
// ============================================================================

void MainWindow::onSweep() {
    // Chercher les sketches disponibles
    auto features = m_document->getAllFeatures();
    std::vector<std::shared_ptr<CADEngine::SketchFeature>> sketches;
    QStringList sketchNames;
    for (const auto& f : features) {
        if (f->getType() == CADEngine::FeatureType::Sketch) {
            auto s = std::dynamic_pointer_cast<CADEngine::SketchFeature>(f);
            if (s && s->getEntityCount() > 0) {
                sketches.push_back(s);
                sketchNames << QString::fromStdString(f->getName());
            }
        }
    }
    
    if (sketches.size() < 2) {
        QMessageBox::information(this, "Balayage", 
            "Le balayage nécessite au moins 2 sketches :\n"
            "- Un sketch profil (section)\n"
            "- Un sketch chemin (trajectoire)");
        return;
    }
    
    QDialog dlg(this);
    dlg.setWindowTitle("Balayage (Sweep)");
    dlg.setMinimumWidth(350);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    QFormLayout* form = new QFormLayout();
    
    QComboBox* cbProfile = new QComboBox();
    cbProfile->addItems(sketchNames);
    form->addRow("Profil (section) :", cbProfile);
    
    QComboBox* cbPath = new QComboBox();
    cbPath->addItems(sketchNames);
    if (sketchNames.size() > 1) cbPath->setCurrentIndex(1);
    form->addRow("Chemin :", cbPath);
    
    QComboBox* cbOp = new QComboBox();
    cbOp->addItems({"Nouveau corps", "Joindre", "Couper", "Intersection"});
    if (!getAccumulatedBody().IsNull()) cbOp->setCurrentIndex(1);
    form->addRow("Opération :", cbOp);
    
    mainLayout->addLayout(form);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    positionDialogRight(&dlg);
    if (dlg.exec() != QDialog::Accepted) return;
    
    auto sweep = m_document->addFeature<CADEngine::SweepFeature>("Balayage");
    sweep->setProfileSketch(sketches[cbProfile->currentIndex()]);
    sweep->setPathSketch(sketches[cbPath->currentIndex()]);
    sweep->setOperation(static_cast<CADEngine::ExtrudeOperation>(cbOp->currentIndex()));
    sweep->setExistingBody(getAccumulatedBody());
    
    if (sweep->compute()) {
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        onStatusMessage("Balayage créé");
    } else {
        m_document->removeFeature(sweep->getId());
        QMessageBox::warning(this, "Erreur Balayage",
            "Le balayage a échoué.\n\n"
            "Vérifiez :\n"
            "• Le profil doit être une forme fermée (cercle, rectangle…)\n"
            "• Le chemin doit être continu (entités connectées bout à bout)\n"
            "• Profil et chemin doivent être sur des plans perpendiculaires\n"
            "  (ex : profil sur XY, chemin sur XZ ou YZ)");
    }
}

// ============================================================================
// Loft Dialog
// ============================================================================

void MainWindow::onLoft() {
    auto features = m_document->getAllFeatures();
    std::vector<std::shared_ptr<CADEngine::SketchFeature>> sketches;
    QStringList sketchNames;
    for (const auto& f : features) {
        if (f->getType() == CADEngine::FeatureType::Sketch) {
            auto s = std::dynamic_pointer_cast<CADEngine::SketchFeature>(f);
            if (s && s->getEntityCount() > 0) {
                sketches.push_back(s);
                sketchNames << QString::fromStdString(f->getName());
            }
        }
    }
    
    if (sketches.size() < 2) {
        QMessageBox::information(this, "Lissage", 
            "Le lissage nécessite au moins 2 sketches\n(profils sur des plans différents).");
        return;
    }
    
    QDialog dlg(this);
    dlg.setWindowTitle("Lissage (Loft)");
    dlg.setMinimumWidth(350);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    
    QLabel* lbl = new QLabel("Sélectionnez les profils dans l'ordre :");
    mainLayout->addWidget(lbl);
    
    QListWidget* profileList = new QListWidget();
    profileList->setSelectionMode(QAbstractItemView::MultiSelection);
    profileList->addItems(sketchNames);
    // Pré-sélectionner les 2 premiers
    if (profileList->count() >= 2) {
        profileList->item(0)->setSelected(true);
        profileList->item(1)->setSelected(true);
    }
    mainLayout->addWidget(profileList);
    
    QFormLayout* form = new QFormLayout();
    QComboBox* cbOp = new QComboBox();
    cbOp->addItems({"Nouveau corps", "Joindre", "Couper", "Intersection"});
    if (!getAccumulatedBody().IsNull()) cbOp->setCurrentIndex(1);
    form->addRow("Opération :", cbOp);
    
    QCheckBox* cbSolid = new QCheckBox("Solide (fermé)");
    cbSolid->setChecked(true);
    form->addRow(cbSolid);
    
    mainLayout->addLayout(form);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    positionDialogRight(&dlg);
    if (dlg.exec() != QDialog::Accepted) return;
    
    auto loft = m_document->addFeature<CADEngine::LoftFeature>("Lissage");
    loft->setOperation(static_cast<CADEngine::ExtrudeOperation>(cbOp->currentIndex()));
    loft->setSolid(cbSolid->isChecked());
    loft->setExistingBody(getAccumulatedBody());
    
    for (int i = 0; i < profileList->count(); i++) {
        if (profileList->item(i)->isSelected()) {
            loft->addProfileSketch(sketches[i]);
        }
    }
    
    if (loft->compute()) {
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        onStatusMessage("Lissage créé");
    } else {
        m_document->removeFeature(loft->getId());
        QMessageBox::warning(this, "Erreur Lissage", "Le lissage a échoué.\nVérifiez que les profils sont fermés et sur des plans différents.");
    }
}

// ============================================================================
// Linear Pattern Dialog
// ============================================================================

void MainWindow::onLinearPattern() {
    TopoDS_Shape body = getAccumulatedBody();
    if (body.IsNull()) {
        QMessageBox::information(this, "Réseau linéaire", "Aucun solide existant.");
        return;
    }
    
    // Trouver la dernière feature modifiante et le body AVANT elle
    auto features = m_document->getAllFeatures();
    TopoDS_Shape bodyBefore;
    TopoDS_Shape featureShape;
    int featureOp = 0;
    
    TopoDS_Shape prevBody;
    for (const auto& f : features) {
        auto ext = std::dynamic_pointer_cast<CADEngine::ExtrudeFeature>(f);
        if (ext && ext->isValid()) {
            bodyBefore = prevBody;
            featureShape = ext->buildExtrudeShape();
            featureOp = static_cast<int>(ext->getOperation());
            prevBody = ext->getResultShape();
            continue;
        }
        auto rev = std::dynamic_pointer_cast<CADEngine::RevolveFeature>(f);
        if (rev && rev->isValid()) { prevBody = rev->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
        auto fil = std::dynamic_pointer_cast<CADEngine::Fillet3DFeature>(f);
        if (fil && fil->isValid()) { prevBody = fil->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
        auto cham = std::dynamic_pointer_cast<CADEngine::Chamfer3DFeature>(f);
        if (cham && cham->isValid()) { prevBody = cham->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
        auto sh = std::dynamic_pointer_cast<CADEngine::ShellFeature>(f);
        if (sh && sh->isValid()) { prevBody = sh->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
        auto lp = std::dynamic_pointer_cast<CADEngine::LinearPatternFeature>(f);
        if (lp && lp->isValid()) { prevBody = lp->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
        auto cp = std::dynamic_pointer_cast<CADEngine::CircularPatternFeature>(f);
        if (cp && cp->isValid()) { prevBody = cp->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
    }

    TopoDS_Shape previewShape = !featureShape.IsNull() ? featureShape : body;
    bool isCut = (featureOp == 2);
    bool hasFeatureSep = !featureShape.IsNull();

    QDialog dlg(this);
    dlg.setWindowTitle("Réseau linéaire");
    dlg.setMinimumWidth(380);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    
    QLabel* infoLabel = new QLabel(
        hasFeatureSep ? "✓ Duplication de la dernière opération" : "⚠ Duplication du solide complet");
    infoLabel->setStyleSheet(hasFeatureSep ? "color: #4CAF50; padding: 4px;" : "color: orange; padding: 4px;");
    mainLayout->addWidget(infoLabel);
    
    // ── Direction 1 ──
    QGroupBox* grp1 = new QGroupBox("Direction 1");
    QFormLayout* form1 = new QFormLayout(grp1);
    
    QComboBox* cbAxis1 = new QComboBox();
    cbAxis1->addItems({"X+", "X-", "Y+", "Y-", "Z+", "Z-"});
    form1->addRow("Direction :", cbAxis1);
    
    QSpinBox* spinCount1 = new QSpinBox();
    spinCount1->setRange(2, 100);
    spinCount1->setValue(3);
    form1->addRow("Nombre :", spinCount1);
    
    QDoubleSpinBox* spinSpacing1 = new QDoubleSpinBox();
    spinSpacing1->setRange(0.1, 10000.0);
    spinSpacing1->setValue(20.0);
    spinSpacing1->setSuffix(" mm");
    spinSpacing1->setDecimals(2);
    form1->addRow("Espacement :", spinSpacing1);
    
    QCheckBox* cbSym1 = new QCheckBox("Symétrique");
    form1->addRow(cbSym1);
    mainLayout->addWidget(grp1);
    
    // ── Direction 2 (optionnelle) ──
    QCheckBox* cbDir2Enabled = new QCheckBox("Activer la 2ème direction (grille)");
    mainLayout->addWidget(cbDir2Enabled);
    
    QGroupBox* grp2 = new QGroupBox("Direction 2");
    QFormLayout* form2 = new QFormLayout(grp2);
    
    QComboBox* cbAxis2 = new QComboBox();
    cbAxis2->addItems({"X+", "X-", "Y+", "Y-", "Z+", "Z-"});
    cbAxis2->setCurrentIndex(2);  // Y+ par défaut
    form2->addRow("Direction :", cbAxis2);
    
    QSpinBox* spinCount2 = new QSpinBox();
    spinCount2->setRange(2, 100);
    spinCount2->setValue(3);
    form2->addRow("Nombre :", spinCount2);
    
    QDoubleSpinBox* spinSpacing2 = new QDoubleSpinBox();
    spinSpacing2->setRange(0.1, 10000.0);
    spinSpacing2->setValue(20.0);
    spinSpacing2->setSuffix(" mm");
    spinSpacing2->setDecimals(2);
    form2->addRow("Espacement :", spinSpacing2);
    
    QCheckBox* cbSym2 = new QCheckBox("Symétrique");
    form2->addRow(cbSym2);
    
    grp2->setEnabled(false);
    mainLayout->addWidget(grp2);
    
    connect(cbDir2Enabled, &QCheckBox::toggled, grp2, &QGroupBox::setEnabled);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    // === LIVE PREVIEW ===
    auto updatePreview = [&]() {
        gp_Dir dirs[] = {gp_Dir(1,0,0), gp_Dir(-1,0,0), gp_Dir(0,1,0), gp_Dir(0,-1,0), gp_Dir(0,0,1), gp_Dir(0,0,-1)};
        int count1 = spinCount1->value();
        double spacing1v = spinSpacing1->value();
        bool sym1 = cbSym1->isChecked();
        gp_Dir dir1 = dirs[cbAxis1->currentIndex()];
        
        bool dir2On = cbDir2Enabled->isChecked();
        int count2 = dir2On ? spinCount2->value() : 1;
        double spacing2v = dir2On ? spinSpacing2->value() : 0;
        bool sym2 = dir2On ? cbSym2->isChecked() : false;
        gp_Dir dir2 = dirs[cbAxis2->currentIndex()];
        
        int start1, end1;
        if (sym1) { start1 = -(count1/2); end1 = count1/2; }
        else { start1 = hasFeatureSep ? 0 : 1; end1 = count1 - 1; }
        
        int start2, end2;
        if (!dir2On || count2 < 2) { start2 = 0; end2 = 0; }
        else if (sym2) { start2 = -(count2/2); end2 = count2/2; }
        else { start2 = 0; end2 = count2 - 1; }
        
        std::vector<TopoDS_Shape> previews;
        for (int i = start1; i <= end1; i++) {
            for (int j = start2; j <= end2; j++) {
                if (i == 0 && j == 0 && !hasFeatureSep) continue;
                
                gp_Vec translation = gp_Vec(dir1) * (i * spacing1v);
                if (dir2On && count2 >= 2)
                    translation = translation + gp_Vec(dir2) * (j * spacing2v);
                
                gp_Trsf transform;
                transform.SetTranslation(translation);
                BRepBuilderAPI_Transform xform(previewShape, transform, true);
                if (xform.IsDone()) previews.push_back(xform.Shape());
            }
        }
        m_viewport->setPatternPreview(previews, isCut);
    };
    
    // Connecter tous les widgets au preview
    connect(spinCount1, QOverload<int>::of(&QSpinBox::valueChanged), [&](int) { updatePreview(); });
    connect(spinSpacing1, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double) { updatePreview(); });
    connect(cbAxis1, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int) { updatePreview(); });
    connect(cbSym1, &QCheckBox::toggled, [&](bool) { updatePreview(); });
    connect(cbDir2Enabled, &QCheckBox::toggled, [&](bool) { updatePreview(); });
    connect(spinCount2, QOverload<int>::of(&QSpinBox::valueChanged), [&](int) { updatePreview(); });
    connect(spinSpacing2, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double) { updatePreview(); });
    connect(cbAxis2, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int) { updatePreview(); });
    connect(cbSym2, &QCheckBox::toggled, [&](bool) { updatePreview(); });
    
    updatePreview();
    
    positionDialogRight(&dlg);
    int result = dlg.exec();
    m_viewport->clearPatternPreview();
    
    if (result != QDialog::Accepted) return;
    
    auto pattern = m_document->addFeature<CADEngine::LinearPatternFeature>("Réseau linéaire");
    
    if (hasFeatureSep && !bodyBefore.IsNull()) {
        pattern->setExistingBody(bodyBefore);
        pattern->setFeatureShape(featureShape);
        pattern->setFeatureOperation(featureOp);
    } else {
        pattern->setBaseShape(body);
    }
    
    gp_Dir dirs[] = {gp_Dir(1,0,0), gp_Dir(-1,0,0), gp_Dir(0,1,0), gp_Dir(0,-1,0), gp_Dir(0,0,1), gp_Dir(0,0,-1)};
    pattern->setDirection(dirs[cbAxis1->currentIndex()]);
    pattern->setCount(spinCount1->value());
    pattern->setSpacing(spinSpacing1->value());
    pattern->setSymmetric(cbSym1->isChecked());
    
    if (cbDir2Enabled->isChecked()) {
        pattern->setSecondDirectionEnabled(true);
        pattern->setDirection2(dirs[cbAxis2->currentIndex()]);
        pattern->setCount2(spinCount2->value());
        pattern->setSpacing2(spinSpacing2->value());
        pattern->setSymmetric2(cbSym2->isChecked());
    }
    
    if (pattern->compute()) {
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        QString msg = QString("Réseau linéaire : %1").arg(spinCount1->value());
        if (cbDir2Enabled->isChecked())
            msg += QString(" × %1 (grille)").arg(spinCount2->value());
        msg += QString(", esp.=%1mm").arg(spinSpacing1->value(), 0, 'f', 1);
        onStatusMessage(msg);
    } else {
        m_document->removeFeature(pattern->getId());
        QMessageBox::warning(this, "Erreur", "Le réseau linéaire a échoué.");
    }
}

// ============================================================================
// Circular Pattern Dialog
// ============================================================================

void MainWindow::onCircularPattern() {
    TopoDS_Shape body = getAccumulatedBody();
    if (body.IsNull()) {
        QMessageBox::information(this, "Réseau circulaire", "Aucun solide existant.");
        return;
    }
    
    // Trouver la dernière feature et le body avant
    auto features = m_document->getAllFeatures();
    TopoDS_Shape bodyBefore;
    TopoDS_Shape featureShape;
    int featureOp = 0;
    TopoDS_Shape prevBody;
    
    for (const auto& f : features) {
        auto ext = std::dynamic_pointer_cast<CADEngine::ExtrudeFeature>(f);
        if (ext && ext->isValid()) {
            bodyBefore = prevBody;
            featureShape = ext->buildExtrudeShape();
            featureOp = static_cast<int>(ext->getOperation());
            prevBody = ext->getResultShape();
            continue;
        }
        auto rev = std::dynamic_pointer_cast<CADEngine::RevolveFeature>(f);
        if (rev && rev->isValid()) { prevBody = rev->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
        auto fil = std::dynamic_pointer_cast<CADEngine::Fillet3DFeature>(f);
        if (fil && fil->isValid()) { prevBody = fil->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
        auto cham = std::dynamic_pointer_cast<CADEngine::Chamfer3DFeature>(f);
        if (cham && cham->isValid()) { prevBody = cham->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
        auto sh = std::dynamic_pointer_cast<CADEngine::ShellFeature>(f);
        if (sh && sh->isValid()) { prevBody = sh->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
        auto lp = std::dynamic_pointer_cast<CADEngine::LinearPatternFeature>(f);
        if (lp && lp->isValid()) { prevBody = lp->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
        auto cp = std::dynamic_pointer_cast<CADEngine::CircularPatternFeature>(f);
        if (cp && cp->isValid()) { prevBody = cp->getResultShape(); bodyBefore = TopoDS_Shape(); featureShape = TopoDS_Shape(); continue; }
    }

    TopoDS_Shape previewShape = !featureShape.IsNull() ? featureShape : body;
    bool isCut = (featureOp == 2);

    QDialog dlg(this);
    dlg.setWindowTitle("Réseau circulaire");
    dlg.setMinimumWidth(350);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    
    QLabel* infoLabel = new QLabel(
        featureShape.IsNull() ? "⚠ Duplication du solide complet" : "✓ Duplication de la dernière opération");
    infoLabel->setStyleSheet(featureShape.IsNull() ? "color: orange; padding: 4px;" : "color: #4CAF50; padding: 4px;");
    mainLayout->addWidget(infoLabel);
    
    QFormLayout* form = new QFormLayout();
    
    QComboBox* cbAxis = new QComboBox();
    cbAxis->addItems({"X+", "X-", "Y+", "Y-", "Z+", "Z-"});
    cbAxis->setCurrentIndex(4);  // Z+ par défaut
    form->addRow("Axe de rotation :", cbAxis);
    
    QSpinBox* spinCount = new QSpinBox();
    spinCount->setRange(2, 360);
    spinCount->setValue(6);
    form->addRow("Nombre :", spinCount);
    
    QDoubleSpinBox* spinAngle = new QDoubleSpinBox();
    spinAngle->setRange(1.0, 360.0);
    spinAngle->setValue(360.0);
    spinAngle->setSuffix("°");
    spinAngle->setDecimals(1);
    form->addRow("Angle total :", spinAngle);
    
    mainLayout->addLayout(form);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    // === LIVE PREVIEW ===
    auto updatePreview = [&]() {
        int count = spinCount->value();
        double totalAngleDeg = spinAngle->value();
        gp_Dir axDirs[] = {gp_Dir(1,0,0), gp_Dir(-1,0,0), gp_Dir(0,1,0), gp_Dir(0,-1,0), gp_Dir(0,0,1), gp_Dir(0,0,-1)};
        gp_Ax1 axis(gp_Pnt(0,0,0), axDirs[cbAxis->currentIndex()]);
        
        double stepAngle;
        if (std::abs(totalAngleDeg - 360.0) < 0.01)
            stepAngle = totalAngleDeg / count;
        else
            stepAngle = totalAngleDeg / (count - 1);
        double stepAngleRad = stepAngle * M_PI / 180.0;
        
        std::vector<TopoDS_Shape> previews;
        
        int startIdx = !featureShape.IsNull() ? 0 : 1;
        
        for (int i = startIdx; i < count; i++) {
            double angle = i * stepAngleRad;
            
            gp_Trsf transform;
            transform.SetRotation(axis, angle);
            
            BRepBuilderAPI_Transform xform(previewShape, transform, true);
            if (xform.IsDone()) {
                previews.push_back(xform.Shape());
            }
        }
        
        m_viewport->setPatternPreview(previews, isCut);
    };
    
    connect(spinCount, QOverload<int>::of(&QSpinBox::valueChanged), [&](int) { updatePreview(); });
    connect(spinAngle, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double) { updatePreview(); });
    connect(cbAxis, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int) { updatePreview(); });
    
    // Preview initial
    updatePreview();
    
    positionDialogRight(&dlg);
    int result = dlg.exec();
    
    m_viewport->clearPatternPreview();
    
    if (result != QDialog::Accepted) return;
    
    auto pattern = m_document->addFeature<CADEngine::CircularPatternFeature>("Réseau circulaire");
    
    if (!featureShape.IsNull() && !bodyBefore.IsNull()) {
        pattern->setExistingBody(bodyBefore);
        pattern->setFeatureShape(featureShape);
        pattern->setFeatureOperation(featureOp);
    } else {
        pattern->setBaseShape(body);
    }
    
    pattern->setCount(spinCount->value());
    pattern->setAngle(spinAngle->value());
    
    gp_Dir axDirs[] = {gp_Dir(1,0,0), gp_Dir(-1,0,0), gp_Dir(0,1,0), gp_Dir(0,-1,0), gp_Dir(0,0,1), gp_Dir(0,0,-1)};
    pattern->setAxis(gp_Ax1(gp_Pnt(0,0,0), axDirs[cbAxis->currentIndex()]));
    
    if (pattern->compute()) {
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        onStatusMessage(QString("Réseau circulaire : %1 copies, angle=%2°")
            .arg(spinCount->value()).arg(spinAngle->value(), 0, 'f', 1));
    } else {
        m_document->removeFeature(pattern->getId());
        QMessageBox::warning(this, "Erreur", "Le réseau circulaire a échoué.");
    }
}

// ============================================================================
// Thread Dialog
// ============================================================================

void MainWindow::onThread() {
    TopoDS_Shape body = getAccumulatedBody();
    if (body.IsNull()) {
        QMessageBox::information(this, "Filetage", "Aucun solide existant.");
        return;
    }
    
    // Enter face selection mode
    m_pendingFaceOp = PendingFaceOp::Thread;
    m_viewport->setCurrentBody(body);
    m_viewport->startFaceSelection();
    
    // Disconnect previous handlers
    disconnect(m_viewport, &Viewport3D::faceSelected, this, nullptr);
    disconnect(m_viewport, &Viewport3D::faceSelectionCancelled, this, nullptr);
    
    connect(m_viewport, &Viewport3D::faceSelectionCancelled, this, [this]() {
        disconnect(m_viewport, &Viewport3D::faceSelected, this, nullptr);
        disconnect(m_viewport, &Viewport3D::faceSelectionCancelled, this, nullptr);
        m_pendingFaceOp = PendingFaceOp::None;
        onStatusMessage("Sélection face annulée");
    });
    
    connect(m_viewport, &Viewport3D::faceSelected, this, [this](TopoDS_Face face) {
        disconnect(m_viewport, &Viewport3D::faceSelected, this, nullptr);
        disconnect(m_viewport, &Viewport3D::faceSelectionCancelled, this, nullptr);
        m_viewport->stopFaceSelection();
        
        if (m_pendingFaceOp != PendingFaceOp::Thread) return;
        m_pendingFaceOp = PendingFaceOp::None;
        
        // Vérifier que la face est cylindrique
        try {
            BRepAdaptor_Surface adaptor(face);
            if (adaptor.GetType() != GeomAbs_Cylinder) {
                QMessageBox::warning(this, "Filetage", 
                    "La face sélectionnée n'est pas cylindrique.\n"
                    "Sélectionnez une face cylindrique (fût de vis, trou...).");
                return;
            }
        } catch (...) {
            QMessageBox::warning(this, "Filetage", "Face invalide.");
            return;
        }
        
        showThreadDialog(face);
    });
    
    onStatusMessage("Filetage — Cliquez sur une face cylindrique (Échap = annuler)");
}

void MainWindow::showThreadDialog(const TopoDS_Face& cylindricalFace) {
    TopoDS_Shape body = getAccumulatedBody();
    if (body.IsNull()) return;
    
    // Auto-détection depuis la face
    double detectedDiameter = 10.0;
    double detectedLength = 20.0;
    bool isHole = false;
    
    try {
        BRepAdaptor_Surface adaptor(cylindricalFace);
        if (adaptor.GetType() == GeomAbs_Cylinder) {
            gp_Cylinder cyl = adaptor.Cylinder();
            detectedDiameter = cyl.Radius() * 2.0;
            double vMin = adaptor.FirstVParameter();
            double vMax = adaptor.LastVParameter();
            double vLen = std::abs(vMax - vMin);
            if (vLen > 0.1 && vLen < 5000.0) detectedLength = vLen;
            else detectedLength = detectedDiameter * 2.0;
            
            // Détecter si c'est un trou ou un fût via la normale de surface
            double uMid = (adaptor.FirstUParameter() + adaptor.LastUParameter()) / 2.0;
            double vMid = (vMin + vMax) / 2.0;
            gp_Pnt ptSurf;
            gp_Vec d1u, d1v;
            adaptor.D1(uMid, vMid, ptSurf, d1u, d1v);
            gp_Vec faceNormal = d1u.Crossed(d1v);
            if (cylindricalFace.Orientation() == TopAbs_REVERSED)
                faceNormal.Reverse();
            gp_Pnt axPt = cyl.Axis().Location();
            gp_Vec toSurf(axPt, ptSurf);
            gp_Vec axV(cyl.Axis().Direction());
            gp_Vec radial = toSurf - axV * toSurf.Dot(axV);
            if (radial.Magnitude() > 1e-6 && faceNormal.Dot(radial) < 0)
                isHole = true;
        }
    } catch (...) {}
    
    QDialog dlg(this);
    dlg.setWindowTitle("Filetage métrique");
    dlg.setMinimumWidth(420);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
    
    // Info
    QString faceTypeStr = isHole ? "Trou (face interne)" : "Fût (face externe)";
    QLabel* infoLabel = new QLabel(
        QString("%3 : Ø%1 mm, L≈%2 mm")
        .arg(detectedDiameter, 0, 'f', 1).arg(detectedLength, 0, 'f', 1).arg(faceTypeStr));
    infoLabel->setStyleSheet("color: #4CAF50; font-weight: bold; padding: 6px;");
    mainLayout->addWidget(infoLabel);
    
    QFormLayout* form = new QFormLayout();
    
    // Mode : Enlever ou Ajouter matière
    QComboBox* cbMode = new QComboBox();
    cbMode->addItem("Enlever matière (creuser les sillons dans le cylindre)");
    cbMode->addItem("Ajouter matière (construire les filets sur le cylindre)");
    form->addRow("Opération :", cbMode);
    
    // Type de filetage
    QComboBox* cbType = new QComboBox();
    cbType->addItems({"Métrique standard (pas gros)", "Métrique pas fin", "Personnalisé"});
    form->addRow("Type :", cbType);
    
    // Sens du pas
    QComboBox* cbHand = new QComboBox();
    cbHand->addItems({"Pas à droite (standard)", "Pas à gauche (inversé)"});
    form->addRow("Sens du pas :", cbHand);
    
    // Diamètre nominal
    QDoubleSpinBox* spinDia = new QDoubleSpinBox();
    spinDia->setRange(1.0, 200.0);
    spinDia->setValue(detectedDiameter);
    spinDia->setSuffix(" mm");
    spinDia->setDecimals(1);
    form->addRow("Diamètre nominal :", spinDia);
    
    // Pas
    QDoubleSpinBox* spinPitch = new QDoubleSpinBox();
    spinPitch->setRange(0.1, 20.0);
    spinPitch->setValue(1.5);
    spinPitch->setSuffix(" mm");
    spinPitch->setDecimals(2);
    form->addRow("Pas :", spinPitch);
    
    // Longueur
    QDoubleSpinBox* spinLength = new QDoubleSpinBox();
    spinLength->setRange(1.0, 1000.0);
    spinLength->setValue(std::min(detectedLength, 500.0));
    spinLength->setSuffix(" mm");
    spinLength->setDecimals(1);
    form->addRow("Longueur filetée :", spinLength);
    
    mainLayout->addLayout(form);
    
    // Description dynamique
    QLabel* descLabel = new QLabel();
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #888; padding: 4px; font-style: italic;");
    mainLayout->addWidget(descLabel);
    
    auto updateDesc = [&]() {
        int modeIdx = cbMode->currentIndex();
        double dp = CADEngine::ThreadFeature::getStandardDepth(spinPitch->value());
        double dia = detectedDiameter;
        if (modeIdx == 0) {
            // Enlever matière
            if (isHole) {
                // Trou : sillons vers l'extérieur (dans la paroi)
                descLabel->setText(QString(
                    "→ Taraudage : creuse les sillons dans la paroi du trou Ø%1\n"
                    "   Diamètre fond de filet : Ø%2 mm")
                    .arg(dia, 0, 'f', 1)
                    .arg(dia + dp * 2, 0, 'f', 1));
            } else {
                // Fût : sillons vers l'intérieur
                descLabel->setText(QString(
                    "→ Filetage : creuse les sillons dans le fût Ø%1\n"
                    "   Diamètre fond de filet : Ø%2 mm")
                    .arg(dia, 0, 'f', 1)
                    .arg(dia - dp * 2, 0, 'f', 1));
            }
        } else {
            // Ajouter matière
            if (isHole) {
                // Trou : filets vers l'intérieur (réduire le diamètre)
                descLabel->setText(QString(
                    "→ Ajoute des filets vers l'intérieur du trou Ø%1\n"
                    "   Diamètre crête : Ø%2 mm")
                    .arg(dia, 0, 'f', 1)
                    .arg(dia - dp * 2, 0, 'f', 1));
            } else {
                // Fût : filets vers l'extérieur
                descLabel->setText(QString(
                    "→ Ajoute des filets sur le fût Ø%1\n"
                    "   Diamètre crête : Ø%2 mm")
                    .arg(dia, 0, 'f', 1)
                    .arg(dia + dp * 2, 0, 'f', 1));
            }
        }
    };
    
    // Auto-update pitch + description
    auto updatePitch = [&]() {
        int typeIdx = cbType->currentIndex();
        if (typeIdx < 2) {
            double pitch = CADEngine::ThreadFeature::getStandardPitch(
                spinDia->value(), static_cast<CADEngine::ThreadType>(typeIdx));
            spinPitch->setValue(pitch);
            spinPitch->setEnabled(false);
        } else {
            spinPitch->setEnabled(true);
        }
        updateDesc();
    };
    
    connect(cbType, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int) { updatePitch(); });
    connect(spinDia, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double) { updatePitch(); });
    connect(cbMode, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int) { updateDesc(); });
    connect(spinPitch, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double) { updateDesc(); });
    updatePitch();
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(buttons);
    
    positionDialogRight(&dlg);
    if (dlg.exec() != QDialog::Accepted) return;
    
    auto thread = m_document->addFeature<CADEngine::ThreadFeature>("Filetage");
    thread->setBaseShape(body);
    thread->setCylindricalFace(cylindricalFace);
    thread->setThreadType(static_cast<CADEngine::ThreadType>(cbType->currentIndex()));
    thread->setMode(static_cast<CADEngine::ThreadMode>(cbMode->currentIndex()));
    thread->setLeftHand(cbHand->currentIndex() == 1);
    thread->setDiameter(spinDia->value());
    thread->setPitch(spinPitch->value());
    thread->setLength(spinLength->value());
    
    if (thread->compute()) {
        m_viewport->invalidateTessCache();
        m_viewport->setCurrentBody(getAccumulatedBody());
        m_viewport->update();
        m_tree->refresh();
        updateToolbars();
        QString modeStr = (cbMode->currentIndex() == 0) ? "ôté" : "ajouté";
        QString handStr = (cbHand->currentIndex() == 1) ? " gauche" : "";
        onStatusMessage(QString("Filetage M%1×%2 L=%3mm — matière %4%5")
            .arg(spinDia->value(), 0, 'f', 1)
            .arg(spinPitch->value(), 0, 'f', 2)
            .arg(spinLength->value(), 0, 'f', 1)
            .arg(modeStr).arg(handStr));
    } else {
        m_document->removeFeature(thread->getId());
        QMessageBox::warning(this, "Erreur Filetage", 
            "Le filetage a échoué.\nVérifiez les paramètres.");
    }
}

// ============================================================================
// Sketch on Face
// ============================================================================

void MainWindow::onSketchOnFace() {
    TopoDS_Shape body = getAccumulatedBody();
    if (body.IsNull()) {
        QMessageBox::information(this, "Sketch sur Face", 
            "Aucun solide existant.\nCréez d'abord une extrusion ou une révolution.");
        return;
    }
    
    // Ensure body is meshed for face picking
    try {
        BRepMesh_IncrementalMesh mesher(body, 0.1, Standard_False, 0.3, Standard_True);
        mesher.Perform();
    } catch (...) {}
    
    // Set body on viewport so picking works even before next render
    m_viewport->setCurrentBody(body);
    
    // Enter interactive face selection mode
    m_viewport->startFaceSelection();
    
    // Disconnect any previous connection to avoid stacking
    disconnect(m_viewport, &Viewport3D::faceSelected, this, nullptr);
    disconnect(m_viewport, &Viewport3D::faceSelectionCancelled, this, nullptr);
    
    // Handle Escape cancellation
    connect(m_viewport, &Viewport3D::faceSelectionCancelled, this, [this]() {
        disconnect(m_viewport, &Viewport3D::faceSelected, this, nullptr);
        disconnect(m_viewport, &Viewport3D::faceSelectionCancelled, this, nullptr);
        onStatusMessage("Sélection face annulée");
    });
    
    // Connect WITHOUT SingleShotConnection — we manage disconnect ourselves
    connect(m_viewport, &Viewport3D::faceSelected, this, [this](TopoDS_Face face) {
        // Face was clicked — check if it's planar
        try {
            BRepAdaptor_Surface adaptor(face);
            if (adaptor.GetType() == GeomAbs_Plane) {
                gp_Pln rawPlane = adaptor.Plane();
                
                // Get original OCCT axes
                gp_Dir normal = rawPlane.Axis().Direction();
                gp_Dir xDir = rawPlane.Position().XDirection();
                
                // CRITICAL: Flip normal for REVERSED faces → outward direction
                // This also fixes the Y axis: N×X gives intuitive "up"
                // e.g. Normal(0,1,0)×X(1,0,0) = Y(0,0,-1) BAD (sketch Y = world -Z)
                //      Normal(0,-1,0)×X(1,0,0) = Y(0,0,1)  GOOD (sketch Y = world +Z)
                if (face.Orientation() == TopAbs_REVERSED) {
                    normal.Reverse();
                }
                
                // Compute face centroid → use as plane origin (zero offset)
                GProp_GProps faceProps;
                BRepGProp::SurfaceProperties(face, faceProps);
                gp_Pnt faceCenter3D = faceProps.CentreOfMass();
                
                // Build sketch plane: centroid origin + corrected normal + original XDir
                gp_Ax2 ax2(faceCenter3D, normal, xDir);
                gp_Pln sketchPlane(ax2);
                
                gp_Dir yDir = ax2.YDirection();
                
                std::cout << "[SketchOnFace] Face center: (" 
                    << faceCenter3D.X() << ", " << faceCenter3D.Y() << ", " << faceCenter3D.Z() << ")" << std::endl;
                std::cout << "[SketchOnFace] Normal: (" 
                    << normal.X() << ", " << normal.Y() << ", " << normal.Z() << ")" << std::endl;
                std::cout << "[SketchOnFace] XDir: (" 
                    << xDir.X() << ", " << xDir.Y() << ", " << xDir.Z() 
                    << ") YDir: (" << yDir.X() << ", " << yDir.Y() << ", " << yDir.Z() << ")" << std::endl;
                std::cout << "[SketchOnFace] Orientation: " 
                    << (face.Orientation() == TopAbs_REVERSED ? "REVERSED" : "FORWARD") << std::endl;
                
                // Success — disconnect all, create sketch
                disconnect(m_viewport, &Viewport3D::faceSelected, this, nullptr);
                disconnect(m_viewport, &Viewport3D::faceSelectionCancelled, this, nullptr);
                
                auto sketch = m_document->addFeature<CADEngine::SketchFeature>("Sketch-Face");
                sketch->setPlane(sketchPlane);
                
                // Compute face extent in local coords for auto-zoom
                double faceExtent = 200.0; // default
                try {
                    Bnd_Box bbox;
                    BRepBndLib::Add(face, bbox);
                    if (!bbox.IsVoid()) {
                        double xmin, ymin, zmin, xmax, ymax, zmax;
                        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
                        // Project bbox corners to local sketch coords to find extent
                        double maxU = 0, maxV = 0;
                        gp_Pnt corners[8] = {
                            {xmin,ymin,zmin}, {xmax,ymin,zmin}, {xmin,ymax,zmin}, {xmax,ymax,zmin},
                            {xmin,ymin,zmax}, {xmax,ymin,zmax}, {xmin,ymax,zmax}, {xmax,ymax,zmax}
                        };
                        for (auto& c : corners) {
                            gp_Vec v(faceCenter3D, c);
                            double u = std::abs(v.Dot(gp_Vec(xDir)));
                            double vv = std::abs(v.Dot(gp_Vec(yDir)));
                            if (u > maxU) maxU = u;
                            if (vv > maxV) maxV = vv;
                        }
                        faceExtent = std::max(maxU, maxV) * 1.5; // 50% margin
                        if (faceExtent < 50.0) faceExtent = 50.0;
                        if (faceExtent > 2000.0) faceExtent = 2000.0;
                    }
                } catch (...) {}
                
                sketch->getSketch2D()->setFaceCenterOffset(0.0, 0.0, faceExtent);
                
                std::cout << "[SketchOnFace] Auto-zoom extent: " << faceExtent << std::endl;
                
                m_viewport->clearSelectedFace();
                m_viewport->stopFaceSelection();
                m_viewport->enterSketchMode(sketch, 0.0, 0.0, faceExtent);
                m_tree->refresh();
                updateToolbars();
                onStatusMessage("Sketch sur face créé — Mode édition sketch");
            }
            else {
                // Non-planar face — stay in selection mode, let user try another face
                m_viewport->clearSelectedFace();
                m_viewport->startFaceSelection();  // Re-enter selection mode
                
                QString faceType;
                if (adaptor.GetType() == GeomAbs_Cylinder) faceType = "cylindrique";
                else if (adaptor.GetType() == GeomAbs_Cone) faceType = "conique";
                else if (adaptor.GetType() == GeomAbs_Sphere) faceType = "sphérique";
                else faceType = "courbe";
                
                onStatusMessage(QString("⚠️ Face %1 — sélectionnez une face plane").arg(faceType));
            }
        } catch (...) {
            m_viewport->clearSelectedFace();
            m_viewport->startFaceSelection();
            onStatusMessage("⚠️ Face invalide — sélectionnez une face plane");
        }
    });
}

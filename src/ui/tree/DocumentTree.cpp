#include "DocumentTree.h"
#include "../../core/document/CADDocument.h"
#include "../../core/parametric/Feature.h"
#include "../../features/sketch/SketchFeature.h"
#include "../../features/primitives/BoxFeature.h"
#include "../../features/primitives/CylinderFeature.h"

#include <QHeaderView>
#include <QMenu>
#include <QDebug>
#include <QPropertyAnimation>
#include <QApplication>
#include <QInputDialog>

DocumentTree::DocumentTree(QWidget* parent)
    : QWidget(parent)
    , m_collapsed(false)
    , m_expandedWidth(250)
{
    setupUI();
}

DocumentTree::~DocumentTree() {
}

void DocumentTree::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // Bouton collapse/expand
    m_collapseButton = new QPushButton("▼ Document", this);
    m_collapseButton->setFixedHeight(30);
    connect(m_collapseButton, &QPushButton::clicked, this, &DocumentTree::toggleCollapsed);
    
    // Tree widget
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    
    connect(m_tree, &QTreeWidget::itemClicked, this, &DocumentTree::onItemClicked);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &DocumentTree::onItemDoubleClicked);
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &DocumentTree::onContextMenu);
    
    m_mainLayout->addWidget(m_collapseButton);
    m_mainLayout->addWidget(m_tree);
    
    // Largeurs pour l'animation
    setMinimumWidth(0);
    setMaximumWidth(m_expandedWidth);
    
    // Appliquer le thème initial
    updateTheme();
}

void DocumentTree::updateTheme() {
    QPalette pal = qApp->palette();
    QColor bgColor = pal.color(QPalette::Window);
    QColor baseColor = pal.color(QPalette::Base);
    QColor textColor = pal.color(QPalette::WindowText);
    QColor highlightColor = pal.color(QPalette::Highlight);
    QColor midColor = pal.color(QPalette::Mid);
    QColor altBase = pal.color(QPalette::AlternateBase);
    
    // Panel background
    setStyleSheet(QString("DocumentTree { background-color: %1; }").arg(bgColor.name()));
    
    // Bouton collapse
    m_collapseButton->setStyleSheet(QString(
        "QPushButton { "
        "  text-align: left; "
        "  padding-left: 5px; "
        "  background-color: %1; "
        "  border: none; "
        "  border-bottom: 1px solid %2; "
        "  color: %3; "
        "  font-weight: bold; "
        "}"
        "QPushButton:hover { "
        "  background-color: %2; "
        "}"
    ).arg(bgColor.name()).arg(midColor.name()).arg(textColor.name()));
    
    // Arbre
    // Hover: couleur intermédiaire entre bg et highlight
    QColor hoverColor = QColor(
        (bgColor.red() + highlightColor.red()) / 3 + bgColor.red() / 3,
        (bgColor.green() + highlightColor.green()) / 3 + bgColor.green() / 3,
        (bgColor.blue() + highlightColor.blue()) / 3 + bgColor.blue() / 3
    );
    
    m_tree->setStyleSheet(QString(
        "QTreeWidget { "
        "  border: none; "
        "  background-color: %1; "
        "  color: %2; "
        "  outline: none; "
        "}"
        "QTreeWidget::item { "
        "  padding: 4px; "
        "  border: none; "
        "}"
        "QTreeWidget::item:hover { "
        "  background-color: %3; "
        "}"
        "QTreeWidget::item:selected { "
        "  background-color: %4; "
        "  color: white; "
        "}"
        "QTreeWidget::branch { "
        "  background-color: %1; "
        "}"
    ).arg(bgColor.name()).arg(textColor.name())
     .arg(hoverColor.name()).arg(highlightColor.name()));
}

void DocumentTree::setDocument(std::shared_ptr<CADEngine::CADDocument> doc) {
    m_document = doc;
    refresh();
}

void DocumentTree::refresh() {
    m_tree->clear();
    m_itemFeatureMap.clear();
    
    if (!m_document) return;
    
    // Racine : nom du document
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_tree);
    rootItem->setText(0, "📄 Pièce sans nom");
    rootItem->setExpanded(true);
    
    // Ajouter toutes les features
    for (const auto& feature : m_document->getAllFeatures()) {
        addFeatureItem(feature, rootItem);
    }
}

QTreeWidgetItem* DocumentTree::addFeatureItem(std::shared_ptr<CADEngine::Feature> feature,
                                               QTreeWidgetItem* parent) {
    if (!feature) return nullptr;
    
    QTreeWidgetItem* item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(m_tree);
    
    // Icône selon le type
    QString icon;
    CADEngine::FeatureType type = feature->getType();
    
    if (type == CADEngine::FeatureType::Sketch) {
        icon = "✏️";
        
        // Si c'est un sketch, ajouter ses entités
        auto sketch = std::dynamic_pointer_cast<CADEngine::SketchFeature>(feature);
        if (sketch) {
            int entityCount = sketch->getEntityCount();
            item->setText(0, QString("%1 %2 (%3 entités)").arg(icon).arg(QString::fromStdString(feature->getName())).arg(entityCount));
        }
    }
    else if (type == CADEngine::FeatureType::Box) {
        icon = "📦";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else if (type == CADEngine::FeatureType::Cylinder) {
        icon = "🔵";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else if (type == CADEngine::FeatureType::Extrude) {
        icon = "⬆️";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else if (type == CADEngine::FeatureType::Revolve) {
        icon = "🔄";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else if (type == CADEngine::FeatureType::Fillet) {
        icon = "🔘";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else if (type == CADEngine::FeatureType::Chamfer) {
        icon = "🔷";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else if (type == CADEngine::FeatureType::LinearPattern) {
        icon = "📊";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else if (type == CADEngine::FeatureType::CircularPattern) {
        icon = "🔃";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else if (feature->getTypeName() == "Sweep") {
        icon = "〰️";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else if (feature->getTypeName() == "Loft") {
        icon = "🔺";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else if (feature->getTypeName() == "Thread") {
        icon = "🔩";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else if (feature->getTypeName() == "Shell") {
        icon = "📦";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    else {
        icon = "📐";
        item->setText(0, QString("%1 %2").arg(icon).arg(QString::fromStdString(feature->getName())));
    }
    
    // Associer feature à l'item
    m_itemFeatureMap[item] = feature;
    
    return item;
}

std::shared_ptr<CADEngine::Feature> DocumentTree::getFeature(QTreeWidgetItem* item) {
    return m_itemFeatureMap.value(item, nullptr);
}

void DocumentTree::toggleCollapsed() {
    m_collapsed = !m_collapsed;
    updateCollapseButton();
    
    if (m_collapsed) {
        // Masquer complètement
        hide();
        setMaximumWidth(0);
    } else {
        // Réafficher
        setMaximumWidth(m_expandedWidth);
        show();
    }
}

void DocumentTree::updateCollapseButton() {
    if (m_collapsed) {
        m_collapseButton->setText("►");
    } else {
        m_collapseButton->setText("▼ Document");
    }
}

void DocumentTree::onItemClicked(QTreeWidgetItem* item, int column) {
    auto feature = getFeature(item);
    if (feature) {
        qDebug() << "[DocumentTree] Feature sélectionnée:" << QString::fromStdString(feature->getName());
        emit featureSelected(feature);
    }
}

void DocumentTree::onItemDoubleClicked(QTreeWidgetItem* item, int column) {
    auto feature = getFeature(item);
    if (feature) {
        qDebug() << "[DocumentTree] Double-clic sur:" << QString::fromStdString(feature->getName());
        emit featureDoubleClicked(feature);
    }
}

void DocumentTree::onContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = m_tree->itemAt(pos);
    if (!item) return;
    
    auto feature = getFeature(item);
    if (!feature) return;
    
    QMenu menu(this);
    
    QAction* actRename = menu.addAction("Renommer");
    QAction* actDelete = menu.addAction("Supprimer");
    
    // Si c'est un sketch, ajouter option Export DXF
    QAction* actExportDXF = nullptr;
    QAction* actExportPDF = nullptr;
    auto sketch = std::dynamic_pointer_cast<CADEngine::SketchFeature>(feature);
    if (sketch) {
        menu.addSeparator();
        actExportDXF = menu.addAction("Exporter en DXF...");
        actExportPDF = menu.addAction("Export taille réelle 1:1 (PDF)...");
        menu.addAction("Exporter en Plan (PDF)...")->setData("plan");
    }
    
    menu.addSeparator();
    QAction* actVisible = menu.addAction("Visible");
    actVisible->setCheckable(true);
    actVisible->setChecked(true);  // TODO: Gérer visibilité
    
    QAction* selected = menu.exec(m_tree->mapToGlobal(pos));
    
    if (selected == actDelete) {
        qDebug() << "[DocumentTree] Suppression demandée:" << QString::fromStdString(feature->getName());
        emit deleteFeatureRequested(feature);
    }
    else if (selected == actRename) {
        QString currentName = QString::fromStdString(feature->getName());
        bool ok = false;
        QString newName = QInputDialog::getText(this, "Renommer",
            "Nouveau nom :", QLineEdit::Normal, currentName, &ok);
        if (ok && !newName.trimmed().isEmpty() && newName != currentName) {
            m_document->renameFeature(feature->getId(), newName.trimmed().toStdString());
            refresh();
        }
    }
    else if (actExportDXF && selected == actExportDXF) {
        qDebug() << "[DocumentTree] Export DXF demandé pour sketch:" << QString::fromStdString(feature->getName());
        emit exportSketchDXFRequested(sketch);
    }
    else if (actExportPDF && selected == actExportPDF) {
        emit exportSketchRealSizePDFRequested(sketch);
    }
    else if (selected && selected->data().toString() == "plan") {
        emit exportSketchPlanRequested(sketch);
    }
}

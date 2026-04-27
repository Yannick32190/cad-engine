#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <memory>

namespace CADEngine {
    class CADDocument;
    class Feature;
    class SketchFeature;
}

class DocumentTree : public QWidget {
    Q_OBJECT

public:
    explicit DocumentTree(QWidget* parent = nullptr);
    ~DocumentTree();
    
    // Associer un document
    void setDocument(std::shared_ptr<CADEngine::CADDocument> doc);
    
    // Rafraîchir l'arbre
    void refresh();
    
    // Mettre à jour les couleurs selon le thème actif
    void updateTheme();
    
    // Collapse/Expand
    void toggleCollapsed();
    bool isCollapsed() const { return m_collapsed; }

signals:
    void featureSelected(std::shared_ptr<CADEngine::Feature> feature);
    void featureDoubleClicked(std::shared_ptr<CADEngine::Feature> feature);
    void deleteFeatureRequested(std::shared_ptr<CADEngine::Feature> feature);
    void exportSketchDXFRequested(std::shared_ptr<CADEngine::SketchFeature> sketch);
    void exportSketchRealSizePDFRequested(std::shared_ptr<CADEngine::SketchFeature> sketch);
    void exportSketchPlanRequested(std::shared_ptr<CADEngine::SketchFeature> sketch);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onContextMenu(const QPoint& pos);

private:
    void setupUI();
    void updateCollapseButton();
    
    // Ajouter une feature à l'arbre
    QTreeWidgetItem* addFeatureItem(std::shared_ptr<CADEngine::Feature> feature, 
                                     QTreeWidgetItem* parent = nullptr);
    
    // Récupérer la feature associée à un item
    std::shared_ptr<CADEngine::Feature> getFeature(QTreeWidgetItem* item);
    
private:
    std::shared_ptr<CADEngine::CADDocument> m_document;
    
    QVBoxLayout* m_mainLayout;
    QPushButton* m_collapseButton;
    QTreeWidget* m_tree;
    
    bool m_collapsed;
    int m_expandedWidth;
    
    // Map item → feature
    QMap<QTreeWidgetItem*, std::shared_ptr<CADEngine::Feature>> m_itemFeatureMap;
};

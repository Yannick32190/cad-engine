#ifndef CADDOCUMENT_H
#define CADDOCUMENT_H

#include "../parametric/Feature.h"
#include <string>
#include <vector>
#include <memory>
#include <map>

// OCAF includes
#include <TDocStd_Document.hxx>
#include <TDF_Label.hxx>

namespace CADEngine {

/**
 * @brief CADDocument - Conteneur principal pour un projet CAO
 * 
 * Gère :
 * - Document OCAF (persistance)
 * - Arbre de features (liste ordonnée)
 * - Calcul paramétrique (recalcul en cascade)
 * - Sauvegarde/Chargement
 * 
 * Exemple d'utilisation :
 * CADDocument doc;
 * doc.setName("Piston_V1");
 * 
 * auto box = doc.addFeature<BoxFeature>("Body");
 * box->setDimensions(80, 80, 60);
 * box->compute();
 * 
 * doc.recomputeAll();
 * doc.saveNative("piston.cadengine");
 * doc.exportSTEP("piston.step");
 */
class CADDocument {
public:
    CADDocument();
    ~CADDocument();
    
    // Identification
    std::string getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    std::string getPath() const { return m_filePath; }
    
    // Gestion des features
    template<typename T>
    std::shared_ptr<T> addFeature(const std::string& name) {
        auto feature = std::make_shared<T>(name);
        addFeatureInternal(feature);
        return feature;
    }
    
    bool removeFeature(int featureId);
    bool removeFeature(const std::string& name);
    
    std::shared_ptr<Feature> getFeature(int id) const;
    std::shared_ptr<Feature> getFeature(const std::string& name) const;
    
    std::vector<std::shared_ptr<Feature>> getAllFeatures() const;
    int getFeatureCount() const { return static_cast<int>(m_features.size()); }
    
    // Calcul paramétrique
    bool recompute(int featureId);              // Recalculer une feature
    bool recomputeAll();                        // Recalculer toutes les features
    bool recomputeDependencies(int featureId);  // Recalculer les dépendants
    
    // État
    bool isModified() const { return m_modified; }
    void setModified(bool modified) { m_modified = modified; }
    
    bool isValid() const;  // Toutes les features valides ?
    
    // Sauvegarde/Chargement format natif (.cadengine = JSON + STEP)
    bool saveNative(const std::string& filepath);
    bool loadNative(const std::string& filepath);
    
    // Export formats standards
    bool exportSTEP(const std::string& filepath);
    bool exportSTL(const std::string& filepath, double deflection = 0.1);
    
    // Import
    bool importSTEP(const std::string& filepath, const std::string& featureName = "Imported");
    
    // Document OCAF (pour usage avancé)
    Handle(TDocStd_Document) getOCAFDocument() const { return m_ocafDoc; }

private:
    void addFeatureInternal(std::shared_ptr<Feature> feature);
    void assignFeatureId();
    std::vector<int> getComputationOrder() const;  // Ordre topologique
    bool hasCyclicDependency() const;
    
    std::string m_name;
    std::string m_filePath;
    bool m_modified;
    
    // Features (ordre = ordre de création)
    std::vector<std::shared_ptr<Feature>> m_features;
    std::map<int, std::shared_ptr<Feature>> m_featureById;
    std::map<std::string, std::shared_ptr<Feature>> m_featureByName;
    
    int m_nextFeatureId;
    
    // Document OCAF pour persistance
    Handle(TDocStd_Document) m_ocafDoc;
    TDF_Label m_mainLabel;
};

} // namespace CADEngine

#endif // CADDOCUMENT_H

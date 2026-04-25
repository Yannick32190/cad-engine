#include "CADDocument.h"
#include <TDocStd_Application.hxx>
#include <TDataStd_Name.hxx>
#include <STEPControl_Writer.hxx>
#include <STEPControl_Reader.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <set>

namespace CADEngine {

CADDocument::CADDocument()
    : m_name("Untitled")
    , m_modified(false)
    , m_nextFeatureId(1)
{
    // Initialisation document OCAF
    static Handle(TDocStd_Application) app = new TDocStd_Application();
    app->NewDocument("BinOcaf", m_ocafDoc);
    m_mainLabel = m_ocafDoc->Main();
}

CADDocument::~CADDocument() {
    m_features.clear();
    m_featureById.clear();
    m_featureByName.clear();
}

// Gestion des features

void CADDocument::addFeatureInternal(std::shared_ptr<Feature> feature) {
    if (!feature) return;
    
    // Assigner un ID unique
    feature->setId(m_nextFeatureId++);
    
    // Ajouter aux différentes maps
    m_features.push_back(feature);
    m_featureById[feature->getId()] = feature;
    m_featureByName[feature->getName()] = feature;
    
    m_modified = true;
}

bool CADDocument::removeFeature(int featureId) {
    auto it = m_featureById.find(featureId);
    if (it == m_featureById.end()) {
        return false;
    }
    
    auto feature = it->second;
    
    // Retirer de toutes les structures
    m_featureById.erase(featureId);
    m_featureByName.erase(feature->getName());
    
    auto vecIt = std::find(m_features.begin(), m_features.end(), feature);
    if (vecIt != m_features.end()) {
        m_features.erase(vecIt);
    }
    
    // TODO: Mettre à jour les dépendances des autres features
    
    m_modified = true;
    return true;
}

bool CADDocument::renameFeature(int featureId, const std::string& newName) {
    auto it = m_featureById.find(featureId);
    if (it == m_featureById.end()) return false;

    auto feature = it->second;
    m_featureByName.erase(feature->getName());
    feature->setName(newName);
    m_featureByName[newName] = feature;
    m_modified = true;
    return true;
}

bool CADDocument::removeFeature(const std::string& name) {
    auto it = m_featureByName.find(name);
    if (it == m_featureByName.end()) {
        return false;
    }
    return removeFeature(it->second->getId());
}

std::shared_ptr<Feature> CADDocument::getFeature(int id) const {
    auto it = m_featureById.find(id);
    return (it != m_featureById.end()) ? it->second : nullptr;
}

std::shared_ptr<Feature> CADDocument::getFeature(const std::string& name) const {
    auto it = m_featureByName.find(name);
    return (it != m_featureByName.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Feature>> CADDocument::getAllFeatures() const {
    return m_features;
}

// Calcul paramétrique

bool CADDocument::recompute(int featureId) {
    auto feature = getFeature(featureId);
    if (!feature) {
        return false;
    }
    
    // Si déjà à jour, pas besoin de recalculer
    if (feature->isUpToDate()) {
        return true;
    }
    
    // Recalculer les dépendances d'abord
    for (int depId : feature->getDependencies()) {
        if (!recompute(depId)) {
            return false;  // Dépendance invalide
        }
    }
    
    // Calculer la feature
    bool success = feature->compute();
    
    if (success) {
        m_modified = true;
    }
    
    return success;
}

bool CADDocument::recomputeAll() {
    // Obtenir l'ordre de calcul (tri topologique)
    std::vector<int> computeOrder = getComputationOrder();
    
    bool allSuccess = true;
    for (int featureId : computeOrder) {
        auto feature = getFeature(featureId);
        if (feature && !feature->isUpToDate()) {
            if (!feature->compute()) {
                allSuccess = false;
                // Continuer quand même pour calculer ce qui est possible
            }
        }
    }
    
    if (allSuccess) {
        m_modified = true;
    }
    
    return allSuccess;
}

bool CADDocument::recomputeDependencies(int featureId) {
    // Trouver toutes les features qui dépendent de featureId
    std::set<int> toRecompute;
    
    for (const auto& feature : m_features) {
        if (feature->dependsOn(featureId)) {
            toRecompute.insert(feature->getId());
        }
    }
    
    // Recalculer dans l'ordre
    bool allSuccess = true;
    for (int id : toRecompute) {
        if (!recompute(id)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

// État

bool CADDocument::isValid() const {
    for (const auto& feature : m_features) {
        if (!feature->isValid()) {
            return false;
        }
    }
    return true;
}

// Calcul de l'ordre topologique (pour dépendances)

std::vector<int> CADDocument::getComputationOrder() const {
    std::vector<int> order;
    std::set<int> visited;
    std::set<int> tempMark;
    
    // DFS pour tri topologique
    std::function<bool(int)> visit = [&](int featureId) -> bool {
        if (tempMark.count(featureId)) {
            return false;  // Cycle détecté
        }
        if (visited.count(featureId)) {
            return true;   // Déjà visité
        }
        
        tempMark.insert(featureId);
        
        auto feature = getFeature(featureId);
        if (feature) {
            for (int depId : feature->getDependencies()) {
                if (!visit(depId)) {
                    return false;
                }
            }
        }
        
        tempMark.erase(featureId);
        visited.insert(featureId);
        order.push_back(featureId);
        
        return true;
    };
    
    // Visiter toutes les features
    for (const auto& feature : m_features) {
        if (!visited.count(feature->getId())) {
            if (!visit(feature->getId())) {
                // Cycle détecté - retourner ordre simple
                order.clear();
                for (const auto& f : m_features) {
                    order.push_back(f->getId());
                }
                break;
            }
        }
    }
    
    return order;
}

bool CADDocument::hasCyclicDependency() const {
    std::set<int> visited;
    std::set<int> tempMark;
    
    std::function<bool(int)> hasCycle = [&](int featureId) -> bool {
        if (tempMark.count(featureId)) return true;
        if (visited.count(featureId)) return false;
        
        tempMark.insert(featureId);
        
        auto feature = getFeature(featureId);
        if (feature) {
            for (int depId : feature->getDependencies()) {
                if (hasCycle(depId)) return true;
            }
        }
        
        tempMark.erase(featureId);
        visited.insert(featureId);
        return false;
    };
    
    for (const auto& feature : m_features) {
        if (hasCycle(feature->getId())) {
            return true;
        }
    }
    
    return false;
}

} // namespace CADEngine

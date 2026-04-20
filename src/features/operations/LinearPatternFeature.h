#ifndef LINEARPATTERNFEATURE_H
#define LINEARPATTERNFEATURE_H

#include "../../core/parametric/Feature.h"
#include "ExtrudeFeature.h"
#include <gp_Dir.hxx>
#include <TopoDS_Shape.hxx>

namespace CADEngine {

/**
 * @brief LinearPatternFeature - Réseau linéaire (répétition le long d'un axe)
 * 
 * Duplique uniquement la dernière feature (pas tout le body).
 * existingBody = body AVANT la feature à dupliquer
 * featureShape = forme brute de la feature (extrusion, perçage, etc.)
 * operation = Cut/Join pour appliquer chaque copie
 */
class LinearPatternFeature : public Feature {
public:
    LinearPatternFeature(const std::string& name = "Réseau linéaire");
    virtual ~LinearPatternFeature() = default;
    
    bool compute() override;
    std::string getTypeName() const override { return "LinearPattern"; }
    
    // Configuration
    void setBaseShape(const TopoDS_Shape& shape) { m_baseShape = shape; invalidate(); }
    void setExistingBody(const TopoDS_Shape& body) { m_existingBody = body; }
    void setFeatureShape(const TopoDS_Shape& shape) { m_featureShape = shape; }
    void setFeatureOperation(int op) { setInt("feature_op", op); }
    
    // Direction 1
    void setDirection(const gp_Dir& dir);
    void setCount(int count);
    void setSpacing(double spacing);
    void setSymmetric(bool sym);
    
    // Direction 2 (optionnelle — grille)
    void setDirection2(const gp_Dir& dir);
    void setCount2(int count);
    void setSpacing2(double spacing);
    void setSymmetric2(bool sym);
    void setSecondDirectionEnabled(bool enabled);
    
    // Getters direction 1
    gp_Dir getDirection() const;
    int getCount() const;
    double getSpacing() const;
    bool isSymmetric() const;
    
    // Getters direction 2
    gp_Dir getDirection2() const;
    int getCount2() const;
    double getSpacing2() const;
    bool isSymmetric2() const;
    bool isSecondDirectionEnabled() const;
    
    int getFeatureOperation() const { return getInt("feature_op"); }
    TopoDS_Shape getResultShape() const { return m_resultShape; }

private:
    TopoDS_Shape m_baseShape;       // Legacy (body complet si pas de feature séparée)
    TopoDS_Shape m_existingBody;    // Body AVANT la feature
    TopoDS_Shape m_featureShape;    // Forme brute de la feature seule
    TopoDS_Shape m_resultShape;
};

} // namespace CADEngine

#endif // LINEARPATTERNFEATURE_H

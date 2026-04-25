#ifndef SKETCHFEATURE_H
#define SKETCHFEATURE_H

#include "../../core/parametric/Feature.h"
#include "Sketch2D.h"
#include <gp_Pln.hxx>
#include <TopoDS_Wire.hxx>
#include <memory>

namespace CADEngine {

/**
 * @brief Feature Sketch - Contient un sketch 2D dans un plan
 * 
 * Paramètres :
 * - plane_origin_x, plane_origin_y, plane_origin_z (double) : Origine du plan
 * - plane_normal_x, plane_normal_y, plane_normal_z (double) : Normale du plan
 * - plane_type (string) : "XY", "YZ", "ZX", ou "Custom"
 * 
 * Le sketch est stocké dans m_sketch2D et peut être édité
 * Le compute() génère un TopoDS_Wire à partir des entités
 */
class SketchFeature : public Feature {
public:
    SketchFeature(const std::string& name = "Sketch");
    virtual ~SketchFeature() = default;
    
    // Implémentation Feature
    bool compute() override;
    std::string getTypeName() const override { return "Sketch"; }
    
    // Plan de travail
    void setPlane(const gp_Pln& plane);
    gp_Pln getPlane() const;
    
    // Helpers pour plans standards
    void setPlaneXY(double z = 0.0);
    void setPlaneYZ(double x = 0.0);
    void setPlaneZX(double y = 0.0);
    
    // Accès au sketch 2D
    std::shared_ptr<Sketch2D> getSketch2D() const { return m_sketch2D; }
    
    // Entités
    void addEntity(std::shared_ptr<SketchEntity> entity);
    int getEntityCount() const;
    
    // Résultat
    TopoDS_Wire getWire() const;

private:
    void initializeParameters();
    void storePlaneInParameters();
    void loadPlaneFromParameters();
    
    std::shared_ptr<Sketch2D> m_sketch2D;
};

} // namespace CADEngine

#endif // SKETCHFEATURE_H

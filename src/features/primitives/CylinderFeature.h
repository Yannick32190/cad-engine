#ifndef CYLINDERFEATURE_H
#define CYLINDERFEATURE_H

#include "../../core/parametric/Feature.h"
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>

namespace CADEngine {

/**
 * @brief Feature Cylinder - Primitive cylindre
 * 
 * Paramètres :
 * - radius (double) : Rayon
 * - height (double) : Hauteur
 * - x, y, z (double) : Position de la base
 * - dx, dy, dz (double) : Direction de l'axe (normalisée automatiquement)
 * - angle (double) : Angle en degrés pour cylindre partiel (défaut = 360°)
 * 
 * Exemple :
 * auto cyl = std::make_shared<CylinderFeature>("Shaft");
 * cyl->setRadius(25.0);
 * cyl->setHeight(100.0);
 * cyl->setAxis(gp_Pnt(0,0,0), gp_Dir(0,0,1));  // Axe Z
 * cyl->compute();
 */
class CylinderFeature : public Feature {
public:
    CylinderFeature(const std::string& name = "Cylinder");
    virtual ~CylinderFeature() = default;
    
    // Implémentation Feature
    bool compute() override;
    std::string getTypeName() const override { return "Cylinder"; }
    
    // Méthodes spécifiques
    void setRadius(double radius);
    void setHeight(double height);
    void setAxis(const gp_Pnt& origin, const gp_Dir& direction);
    void setAngle(double angleDegrees);  // Pour cylindre partiel
    
    // Getters
    double getRadius() const;
    double getHeight() const;
    gp_Pnt getOrigin() const;
    gp_Dir getDirection() const;
    double getAngle() const;

private:
    void initializeParameters();
};

} // namespace CADEngine

#endif // CYLINDERFEATURE_H

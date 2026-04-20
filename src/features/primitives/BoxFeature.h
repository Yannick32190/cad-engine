#ifndef BOXFEATURE_H
#define BOXFEATURE_H

#include "../../core/parametric/Feature.h"
#include <gp_Pnt.hxx>

namespace CADEngine {

/**
 * @brief Feature Box - Primitive parallélépipède rectangle
 * 
 * Paramètres :
 * - width (double) : Largeur (X)
 * - depth (double) : Profondeur (Y)
 * - height (double) : Hauteur (Z)
 * - x, y, z (double) : Position du coin (optionnel, défaut = origine)
 * - centered (bool) : Si true, centre le box sur la position (défaut = false)
 * 
 * Exemple d'utilisation :
 * auto box = std::make_shared<BoxFeature>("Block");
 * box->setDouble("width", 100.0);
 * box->setDouble("depth", 50.0);
 * box->setDouble("height", 30.0);
 * box->compute();
 */
class BoxFeature : public Feature {
public:
    BoxFeature(const std::string& name = "Box");
    virtual ~BoxFeature() = default;
    
    // Implémentation Feature
    bool compute() override;
    std::string getTypeName() const override { return "Box"; }
    
    // Méthodes spécifiques
    void setDimensions(double width, double depth, double height);
    void setPosition(double x, double y, double z);
    void setPosition(const gp_Pnt& point);
    void setCentered(bool centered);
    
    // Getters
    double getWidth() const;
    double getDepth() const;
    double getHeight() const;
    gp_Pnt getPosition() const;
    bool isCentered() const;

private:
    void initializeParameters();
};

} // namespace CADEngine

#endif // BOXFEATURE_H

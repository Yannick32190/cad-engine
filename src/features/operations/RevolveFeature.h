#ifndef REVOLVEFEATURE_H
#define REVOLVEFEATURE_H

#include "../../core/parametric/Feature.h"
#include "../sketch/SketchFeature.h"
#include "ExtrudeFeature.h"  // Pour ExtrudeOperation
#include <gp_Ax1.hxx>
#include <TopoDS_Shape.hxx>
#include <memory>

namespace CADEngine {

enum class RevolveAxisType {
    XAxis,      // Axe X global
    YAxis,      // Axe Y global  
    SketchX,    // Axe X du sketch
    SketchY,    // Axe Y du sketch
    CustomLine  // Ligne du sketch comme axe
};

/**
 * @brief RevolveFeature - Révolution d'un profil sketch autour d'un axe
 * 
 * Paramètres :
 * - angle (double) : Angle de révolution en degrés (360 = complet)
 * - axis_type (int) : RevolveAxisType
 * - operation (int) : ExtrudeOperation (Join/Cut/NewBody/Intersect)
 */
class RevolveFeature : public Feature {
public:
    RevolveFeature(const std::string& name = "Révolution");
    virtual ~RevolveFeature() = default;
    
    bool compute() override;
    std::string getTypeName() const override { return "Revolve"; }
    
    // Configuration
    void setSketchFeature(std::shared_ptr<SketchFeature> sketch);
    void setAngle(double angleDeg);
    void setAxisType(RevolveAxisType type);
    void setCustomAxis(const gp_Ax1& axis);
    void setOperation(ExtrudeOperation op);
    void setExistingBody(const TopoDS_Shape& body);
    void setProfileIndex(int index);
    void setSelectedFace(const TopoDS_Face& face);
    
    // Getters
    std::shared_ptr<SketchFeature> getSketchFeature() const { return m_sketchFeature; }
    double getAngle() const;
    RevolveAxisType getAxisType() const;
    ExtrudeOperation getOperation() const;
    int getProfileIndex() const;
    TopoDS_Shape getResultShape() const { return m_resultShape; }
    
    // Preview
    TopoDS_Shape buildRevolveShape() const;

private:
    void initializeParameters();
    gp_Ax1 resolveAxis() const;
    TopoDS_Face buildFaceFromSketch() const;
    
    std::shared_ptr<SketchFeature> m_sketchFeature;
    TopoDS_Shape m_existingBody;
    TopoDS_Shape m_resultShape;
    TopoDS_Face m_selectedFace;
    gp_Ax1 m_customAxis;
};

} // namespace CADEngine

#endif // REVOLVEFEATURE_H

#ifndef PUSHPULLFEATURE_H
#define PUSHPULLFEATURE_H

#include "../../core/parametric/Feature.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Dir.hxx>

namespace CADEngine {

/**
 * @brief PushPullFeature — Appuyer/Tirer une face existante
 * 
 * Sélectionner une face du solide et la déplacer (positif = tirer, négatif = pousser).
 * Équivalent du Push/Pull de Fusion 360 / SketchUp.
 */
class PushPullFeature : public Feature {
public:
    PushPullFeature(const std::string& name = "Appuyer/Tirer");
    virtual ~PushPullFeature() = default;
    
    bool compute() override;
    std::string getTypeName() const override { return "PushPull"; }
    
    // Configuration
    void setExistingBody(const TopoDS_Shape& body) { m_existingBody = body; }
    void setFace(const TopoDS_Face& face, const gp_Dir* cameraDir = nullptr);
    void setDistance(double d) { setDouble("distance", d); invalidate(); }
    
    // Getters
    double getDistance() const { return getDouble("distance"); }
    TopoDS_Shape getResultShape() const { return m_resultShape; }
    
private:
    TopoDS_Shape m_existingBody;
    TopoDS_Face m_face;
    gp_Dir m_faceNormal;
    TopoDS_Shape m_resultShape;
};

} // namespace CADEngine

#endif // PUSHPULLFEATURE_H

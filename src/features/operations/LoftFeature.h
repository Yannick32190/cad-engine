#ifndef LOFTFEATURE_H
#define LOFTFEATURE_H

#include "../../core/parametric/Feature.h"
#include "../sketch/SketchFeature.h"
#include "ExtrudeFeature.h"
#include <TopoDS_Wire.hxx>
#include <memory>
#include <vector>

namespace CADEngine {

/**
 * @brief LoftFeature - Transition entre deux ou plusieurs profils (Loft)
 * 
 * Crée un solide lissé entre plusieurs sections (sketches).
 * Minimum 2 profils requis.
 */
class LoftFeature : public Feature {
public:
    LoftFeature(const std::string& name = "Lissage");
    virtual ~LoftFeature() = default;
    
    bool compute() override;
    std::string getTypeName() const override { return "Loft"; }
    
    // Configuration
    void addProfileSketch(std::shared_ptr<SketchFeature> sketch);
    void clearProfiles();
    void setOperation(ExtrudeOperation op);
    void setExistingBody(const TopoDS_Shape& body);
    void setSolid(bool solid);
    
    // Getters
    std::vector<std::shared_ptr<SketchFeature>> getProfileSketches() const { return m_profiles; }
    ExtrudeOperation getOperation() const;
    bool isSolid() const;
    TopoDS_Shape getResultShape() const { return m_resultShape; }

private:
    std::vector<std::shared_ptr<SketchFeature>> m_profiles;
    TopoDS_Shape m_existingBody;
    TopoDS_Shape m_resultShape;
};

} // namespace CADEngine

#endif // LOFTFEATURE_H

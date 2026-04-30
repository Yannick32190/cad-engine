#ifndef SWEEPFEATURE_H
#define SWEEPFEATURE_H

#include "../../core/parametric/Feature.h"
#include "../sketch/SketchFeature.h"
#include "ExtrudeFeature.h"
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <memory>

namespace CADEngine {

/**
 * @brief SweepFeature - Extrusion d'un profil le long d'un chemin (Sweep/Pipe)
 * 
 * Nécessite 2 sketches :
 * - Un sketch profil (section)
 * - Un sketch chemin (path)
 */
class SweepFeature : public Feature {
public:
    SweepFeature(const std::string& name = "Balayage");
    virtual ~SweepFeature() = default;
    
    bool compute() override;
    std::string getTypeName() const override { return "Sweep"; }
    
    // Configuration
    void setProfileSketch(std::shared_ptr<SketchFeature> sketch);
    void setPathSketch(std::shared_ptr<SketchFeature> sketch);
    void setOperation(ExtrudeOperation op);
    void setExistingBody(const TopoDS_Shape& body);
    void setProfileFace(const TopoDS_Face& face);
    void setPathWire(const TopoDS_Wire& wire);
    
    // Getters
    std::shared_ptr<SketchFeature> getProfileSketch() const { return m_profileSketch; }
    std::shared_ptr<SketchFeature> getPathSketch() const { return m_pathSketch; }
    ExtrudeOperation getOperation() const;
    TopoDS_Shape getResultShape() const { return m_resultShape; }

private:
    TopoDS_Face buildProfileFace() const;
    TopoDS_Wire buildProfileWire() const;
    TopoDS_Wire buildPathWire() const;
    
    std::shared_ptr<SketchFeature> m_profileSketch;
    std::shared_ptr<SketchFeature> m_pathSketch;
    TopoDS_Shape m_existingBody;
    TopoDS_Shape m_resultShape;
    TopoDS_Face m_profileFace;
    TopoDS_Wire m_pathWire;

    // Cached during buildPathWire(): true if any junction between consecutive path edges
    // is non-tangent (chamfer / sharp polyline corner). Drives transition mode in compute().
    mutable bool m_pathHasSharpCorners = false;
};

} // namespace CADEngine

#endif // SWEEPFEATURE_H

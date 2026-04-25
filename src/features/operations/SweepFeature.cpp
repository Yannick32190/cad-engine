#include "SweepFeature.h"

#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <GC_MakeSegment.hxx>
#include <gp_Pnt.hxx>

#include <iostream>

namespace CADEngine {

SweepFeature::SweepFeature(const std::string& name)
    : Feature(FeatureType::Unknown, name)
{
    setInt("operation", static_cast<int>(ExtrudeOperation::NewBody));
}

void SweepFeature::setProfileSketch(std::shared_ptr<SketchFeature> sketch) {
    m_profileSketch = sketch;
    if (sketch) addDependency(sketch->getId());
}

void SweepFeature::setPathSketch(std::shared_ptr<SketchFeature> sketch) {
    m_pathSketch = sketch;
    if (sketch) addDependency(sketch->getId());
}

void SweepFeature::setOperation(ExtrudeOperation op) {
    setInt("operation", static_cast<int>(op));
    invalidate();
}

void SweepFeature::setExistingBody(const TopoDS_Shape& body) {
    m_existingBody = body;
}

void SweepFeature::setProfileFace(const TopoDS_Face& face) {
    m_profileFace = face;
}

void SweepFeature::setPathWire(const TopoDS_Wire& wire) {
    m_pathWire = wire;
}

ExtrudeOperation SweepFeature::getOperation() const {
    return static_cast<ExtrudeOperation>(getInt("operation"));
}

TopoDS_Face SweepFeature::buildProfileFace() const {
    if (!m_profileFace.IsNull()) return m_profileFace;
    if (!m_profileSketch || !m_profileSketch->getSketch2D()) return TopoDS_Face();
    
    auto sketch2D = m_profileSketch->getSketch2D();
    auto regions = sketch2D->buildFaceRegions();
    if (regions.empty()) return TopoDS_Face();
    
    return regions[0].face;
}

TopoDS_Wire SweepFeature::buildPathWire() const {
    if (!m_pathWire.IsNull()) return m_pathWire;
    if (!m_pathSketch || !m_pathSketch->getSketch2D()) return TopoDS_Wire();
    
    auto sketch2D = m_pathSketch->getSketch2D();
    auto entities = sketch2D->getEntities();
    
    // Construire un wire à partir de toutes les entités du sketch chemin
    BRepBuilderAPI_MakeWire wireBuilder;
    gp_Pln plane = sketch2D->getPlane();
    
    for (const auto& entity : entities) {
        auto line = std::dynamic_pointer_cast<SketchLine>(entity);
        if (line) {
            gp_Pnt p1 = plane.Location().XYZ() + 
                         line->getStart().X() * gp_Vec(plane.XAxis().Direction()).XYZ() +
                         line->getStart().Y() * gp_Vec(plane.YAxis().Direction()).XYZ();
            gp_Pnt p2 = plane.Location().XYZ() + 
                         line->getEnd().X() * gp_Vec(plane.XAxis().Direction()).XYZ() +
                         line->getEnd().Y() * gp_Vec(plane.YAxis().Direction()).XYZ();
            if (p1.Distance(p2) > 1e-6) {
                wireBuilder.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
            }
        }
    }
    
    if (!wireBuilder.IsDone()) return TopoDS_Wire();
    return wireBuilder.Wire();
}

bool SweepFeature::compute() {
    std::cout << "[Sweep] Computing..." << std::endl;
    
    TopoDS_Face profile = buildProfileFace();
    if (profile.IsNull()) {
        std::cerr << "[Sweep] ERROR: No profile face" << std::endl;
        return false;
    }
    
    TopoDS_Wire path = buildPathWire();
    if (path.IsNull()) {
        std::cerr << "[Sweep] ERROR: No path wire" << std::endl;
        return false;
    }
    
    try {
        BRepOffsetAPI_MakePipe pipe(path, profile);
        pipe.Build();
        
        if (!pipe.IsDone()) {
            std::cerr << "[Sweep] ERROR: MakePipe failed" << std::endl;
            return false;
        }
        
        TopoDS_Shape sweepShape = pipe.Shape();
        
        // Appliquer opération booléenne
        ExtrudeOperation op = getOperation();
        if (op == ExtrudeOperation::NewBody || m_existingBody.IsNull()) {
            m_resultShape = sweepShape;
            m_shape = sweepShape;
        } else if (op == ExtrudeOperation::Join) {
            BRepAlgoAPI_Fuse fuse(m_existingBody, sweepShape);
            if (fuse.IsDone()) { m_resultShape = fuse.Shape(); m_shape = m_resultShape; }
            else { m_resultShape = sweepShape; m_shape = sweepShape; }
        } else if (op == ExtrudeOperation::Cut) {
            BRepAlgoAPI_Cut cut(m_existingBody, sweepShape);
            if (cut.IsDone()) { m_resultShape = cut.Shape(); m_shape = m_resultShape; }
            else return false;
        } else if (op == ExtrudeOperation::Intersect) {
            BRepAlgoAPI_Common common(m_existingBody, sweepShape);
            if (common.IsDone()) { m_resultShape = common.Shape(); m_shape = m_resultShape; }
            else return false;
        }
        
        m_upToDate = true;
        std::cout << "[Sweep] Success" << std::endl;
        return true;
        
    } catch (Standard_Failure& e) {
        std::cerr << "[Sweep] OCCT Exception: " << e.GetMessageString() << std::endl;
        return false;
    }
}

} // namespace CADEngine

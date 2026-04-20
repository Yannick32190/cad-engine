#include "LoftFeature.h"

#include <BRepOffsetAPI_ThruSections.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <iostream>

namespace CADEngine {

LoftFeature::LoftFeature(const std::string& name)
    : Feature(FeatureType::Unknown, name)
{
    setInt("operation", static_cast<int>(ExtrudeOperation::NewBody));
    setBool("solid", true);
}

void LoftFeature::addProfileSketch(std::shared_ptr<SketchFeature> sketch) {
    m_profiles.push_back(sketch);
    if (sketch) addDependency(sketch->getId());
    invalidate();
}

void LoftFeature::clearProfiles() {
    m_profiles.clear();
    invalidate();
}

void LoftFeature::setOperation(ExtrudeOperation op) {
    setInt("operation", static_cast<int>(op));
    invalidate();
}

void LoftFeature::setExistingBody(const TopoDS_Shape& body) {
    m_existingBody = body;
}

void LoftFeature::setSolid(bool solid) {
    setBool("solid", solid);
    invalidate();
}

ExtrudeOperation LoftFeature::getOperation() const {
    return static_cast<ExtrudeOperation>(getInt("operation"));
}

bool LoftFeature::isSolid() const {
    return getBool("solid");
}

bool LoftFeature::compute() {
    std::cout << "[Loft] Computing with " << m_profiles.size() << " profiles..." << std::endl;
    
    if (m_profiles.size() < 2) {
        std::cerr << "[Loft] ERROR: Need at least 2 profiles" << std::endl;
        return false;
    }
    
    try {
        bool solid = isSolid();
        BRepOffsetAPI_ThruSections loft(solid, /*ruled=*/false);
        
        for (auto& profile : m_profiles) {
            if (!profile || !profile->getSketch2D()) {
                std::cerr << "[Loft] ERROR: Invalid profile sketch" << std::endl;
                return false;
            }
            
            auto sketch2D = profile->getSketch2D();
            auto regions = sketch2D->buildFaceRegions();
            
            if (regions.empty()) {
                // Essayer avec un wire ouvert
                auto entities = sketch2D->getEntities();
                Handle(TopTools_HSequenceOfShape) edgeSeq = new TopTools_HSequenceOfShape();
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
                            edgeSeq->Append(BRepBuilderAPI_MakeEdge(p1, p2).Shape());
                        }
                    }
                }
                
                Handle(TopTools_HSequenceOfShape) wireSeq = new TopTools_HSequenceOfShape();
                ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edgeSeq, 1e-4, false, wireSeq);
                if (wireSeq->Length() > 0) {
                    loft.AddWire(TopoDS::Wire(wireSeq->Value(1)));
                } else {
                    std::cerr << "[Loft] ERROR: Cannot build wire for profile" << std::endl;
                    return false;
                }
            } else {
                // Extraire le wire de la première face
                TopExp_Explorer wireExp(regions[0].face, TopAbs_WIRE);
                if (wireExp.More()) {
                    loft.AddWire(TopoDS::Wire(wireExp.Current()));
                }
            }
        }
        
        loft.Build();
        
        if (!loft.IsDone()) {
            std::cerr << "[Loft] ERROR: ThruSections failed" << std::endl;
            return false;
        }
        
        TopoDS_Shape loftShape = loft.Shape();
        
        // Appliquer opération booléenne
        ExtrudeOperation op = getOperation();
        if (op == ExtrudeOperation::NewBody || m_existingBody.IsNull()) {
            m_resultShape = loftShape;
            m_shape = loftShape;
        } else if (op == ExtrudeOperation::Join) {
            BRepAlgoAPI_Fuse fuse(m_existingBody, loftShape);
            if (fuse.IsDone()) { m_resultShape = fuse.Shape(); m_shape = m_resultShape; }
            else { m_resultShape = loftShape; m_shape = loftShape; }
        } else if (op == ExtrudeOperation::Cut) {
            BRepAlgoAPI_Cut cut(m_existingBody, loftShape);
            if (cut.IsDone()) { m_resultShape = cut.Shape(); m_shape = m_resultShape; }
            else return false;
        } else if (op == ExtrudeOperation::Intersect) {
            BRepAlgoAPI_Common common(m_existingBody, loftShape);
            if (common.IsDone()) { m_resultShape = common.Shape(); m_shape = m_resultShape; }
            else return false;
        }
        
        m_upToDate = true;
        std::cout << "[Loft] Success" << std::endl;
        return true;
        
    } catch (Standard_Failure& e) {
        std::cerr << "[Loft] OCCT Exception: " << e.GetMessageString() << std::endl;
        return false;
    }
}

} // namespace CADEngine

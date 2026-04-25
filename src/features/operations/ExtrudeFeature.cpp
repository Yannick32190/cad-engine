#include "ExtrudeFeature.h"

#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepLib.hxx>
#include <ShapeFix_Solid.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Compound.hxx>
#include <BRep_Builder.hxx>
#include <iostream>
#include <TopExp_Explorer.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <Precision.hxx>

#include <iostream>

namespace CADEngine {

ExtrudeFeature::ExtrudeFeature(const std::string& name)
    : Feature(FeatureType::Extrude, name)
{
    initializeParameters();
}

void ExtrudeFeature::initializeParameters() {
    setDouble("distance", 10.0);
    setDouble("distance2", 10.0);
    setInt("direction", static_cast<int>(ExtrudeDirection::OneSide));
    setInt("operation", static_cast<int>(ExtrudeOperation::NewBody));
    setInt("sketch_id", -1);
    setInt("profile_index", -1);  // -1 = auto (first closed profile)
}

void ExtrudeFeature::setSketchFeature(std::shared_ptr<SketchFeature> sketch) {
    m_sketchFeature = sketch;
    if (sketch) {
        setInt("sketch_id", sketch->getId());
        addDependency(sketch->getId());
    }
}

void ExtrudeFeature::setDistance(double distance) {
    setDouble("distance", distance);
    invalidate();
}

void ExtrudeFeature::setDistance2(double distance) {
    setDouble("distance2", distance);
    invalidate();
}

void ExtrudeFeature::setDirection(ExtrudeDirection dir) {
    setInt("direction", static_cast<int>(dir));
    invalidate();
}

void ExtrudeFeature::setOperation(ExtrudeOperation op) {
    setInt("operation", static_cast<int>(op));
    invalidate();
}

void ExtrudeFeature::setExistingBody(const TopoDS_Shape& body) {
    m_existingBody = body;
}

void ExtrudeFeature::setProfileIndex(int index) {
    setInt("profile_index", index);
    invalidate();
}

void ExtrudeFeature::setSelectedFace(const TopoDS_Face& face) {
    m_selectedFace = face;
    invalidate();
}

double ExtrudeFeature::getDistance() const {
    return getDouble("distance");
}

double ExtrudeFeature::getDistance2() const {
    return getDouble("distance2");
}

ExtrudeDirection ExtrudeFeature::getDirection() const {
    return static_cast<ExtrudeDirection>(getInt("direction"));
}

ExtrudeOperation ExtrudeFeature::getOperation() const {
    return static_cast<ExtrudeOperation>(getInt("operation"));
}

int ExtrudeFeature::getProfileIndex() const {
    return getInt("profile_index");
}

// ============================================================================
// Construire la face depuis le sketch
// ============================================================================

TopoDS_Face ExtrudeFeature::buildFaceFromSketch() const {
    // If a face was directly selected in viewport (Fusion 360 style), use it
    if (!m_selectedFace.IsNull()) {
        return m_selectedFace;
    }
    
    if (!m_sketchFeature) return TopoDS_Face();
    
    auto sketch2D = m_sketchFeature->getSketch2D();
    if (!sketch2D || sketch2D->getEntityCount() == 0) return TopoDS_Face();
    
    gp_Pln plane = sketch2D->getPlane();
    int profileIdx = getProfileIndex();
    
    // === Primary: Use detectClosedProfiles for reliable per-entity detection ===
    auto profiles = sketch2D->detectClosedProfiles();
    
    if (!profiles.empty()) {
        int idx = (profileIdx >= 0 && profileIdx < (int)profiles.size()) ? profileIdx : 0;
        
        std::cout << "[ExtrudeFeature] Using profile " << idx << "/" << profiles.size() 
                  << ": " << profiles[idx].description << std::endl;
        
        if (!profiles[idx].face.IsNull()) {
            return profiles[idx].face;
        }
        
        // Face not pre-built, build from wire
        try {
            BRepBuilderAPI_MakeFace faceMaker(plane, profiles[idx].wire, Standard_True);
            if (faceMaker.IsDone()) return faceMaker.Face();
        } catch (...) {}
    }
    
    // === Fallback: FreeBounds on ALL edges ===
    auto edges = sketch2D->getEdges3D();
    if (edges.empty()) return TopoDS_Face();
    
    Handle(TopTools_HSequenceOfShape) edgeSequence = new TopTools_HSequenceOfShape();
    for (const auto& edge : edges) {
        if (!edge.IsNull()) edgeSequence->Append(edge);
    }
    
    if (edgeSequence->Length() == 0) return TopoDS_Face();
    
    Handle(TopTools_HSequenceOfShape) wires = new TopTools_HSequenceOfShape();
    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edgeSequence, 0.5, Standard_False, wires);
    
    for (int i = 1; i <= wires->Length(); i++) {
        TopoDS_Wire w = TopoDS::Wire(wires->Value(i));
        if (w.IsNull()) continue;
        
        try {
            BRepBuilderAPI_MakeFace faceMaker(plane, w, Standard_True);
            if (faceMaker.IsDone()) return faceMaker.Face();
        } catch (...) { continue; }
    }
    
    return TopoDS_Face();
}

// ============================================================================
// Construire le solide extrudé (sans opération booléenne)
// ============================================================================

TopoDS_Shape ExtrudeFeature::buildExtrudeShape() const {
    TopoDS_Face face = buildFaceFromSketch();
    if (face.IsNull()) {
        std::cerr << "[ExtrudeFeature] buildFaceFromSketch returned null face" << std::endl;
        return TopoDS_Shape();
    }
    
    if (!m_sketchFeature) {
        std::cerr << "[ExtrudeFeature] No sketch feature set" << std::endl;
        return TopoDS_Shape();
    }
    gp_Pln plane = m_sketchFeature->getSketch2D()->getPlane();
    gp_Dir normal = plane.Axis().Direction();
    
    std::cout << "[ExtrudeFeature] Plane origin: (" 
        << plane.Location().X() << ", " << plane.Location().Y() << ", " << plane.Location().Z() << ")" << std::endl;
    std::cout << "[ExtrudeFeature] Normal: (" 
        << normal.X() << ", " << normal.Y() << ", " << normal.Z() << ")" << std::endl;
    std::cout << "[ExtrudeFeature] Using selectedFace: " << (!m_selectedFace.IsNull() ? "YES" : "NO") << std::endl;
    
    double dist1 = getDistance();
    double dist2 = getDistance2();
    ExtrudeDirection dir = getDirection();
    
    // Normalise les normales sortantes du solide après extrusion
    auto fixSolid = [](TopoDS_Shape s) -> TopoDS_Shape {
        try {
            ShapeFix_Solid fixer;
            fixer.Init(TopoDS::Solid(s));
            fixer.Perform();
            return fixer.Solid();
        } catch (...) { return s; }
    };

    try {
        if (dir == ExtrudeDirection::OneSide) {
            gp_Vec vec(normal);
            vec.Scale(dist1);
            BRepPrimAPI_MakePrism prism(face, vec);
            prism.Build();
            if (prism.IsDone()) return fixSolid(prism.Shape());
        }
        else if (dir == ExtrudeDirection::Symmetric) {
            double halfDist = dist1 / 2.0;
            gp_Vec vecNeg(normal); vecNeg.Scale(-halfDist);
            gp_Vec vecFull(normal); vecFull.Scale(dist1);
            gp_Trsf trsf;
            trsf.SetTranslation(vecNeg);
            TopoDS_Face movedFace = TopoDS::Face(face.Moved(TopLoc_Location(trsf)));
            BRepPrimAPI_MakePrism prism(movedFace, vecFull);
            prism.Build();
            if (prism.IsDone()) return fixSolid(prism.Shape());
        }
        else if (dir == ExtrudeDirection::TwoSides) {
            gp_Vec vec1(normal); vec1.Scale(dist1);
            gp_Vec vec2(normal); vec2.Scale(-dist2);
            BRepPrimAPI_MakePrism prism1(face, vec1);
            prism1.Build();
            BRepPrimAPI_MakePrism prism2(face, vec2);
            prism2.Build();
            if (prism1.IsDone() && prism2.IsDone()) {
                BRepAlgoAPI_Fuse fuse(prism1.Shape(), prism2.Shape());
                fuse.Build();
                if (fuse.IsDone()) return fixSolid(fuse.Shape());
                return fixSolid(prism1.Shape());
            }
            else if (prism1.IsDone()) return fixSolid(prism1.Shape());
        }
    } catch (const Standard_Failure& e) {
        std::cerr << "[ExtrudeFeature] OCCT error: " << e.GetMessageString() << std::endl;
    } catch (...) {
        std::cerr << "[ExtrudeFeature] Unknown error during extrusion" << std::endl;
    }
    
    return TopoDS_Shape();
}

// ============================================================================
// Appliquer l'opération booléenne
// ============================================================================

TopoDS_Shape ExtrudeFeature::applyOperation(const TopoDS_Shape& extruded) const {
    ExtrudeOperation op = getOperation();
    
    if (op == ExtrudeOperation::NewBody || m_existingBody.IsNull()) {
        return extruded;
    }
    
    try {
        if (op == ExtrudeOperation::Join) {
            BRepAlgoAPI_Fuse fuse(m_existingBody, extruded);
            fuse.Build();
            if (fuse.IsDone()) return fuse.Shape();
        }
        else if (op == ExtrudeOperation::Cut) {
            BRepAlgoAPI_Cut cut(m_existingBody, extruded);
            cut.Build();
            if (cut.IsDone()) return cut.Shape();
        }
        else if (op == ExtrudeOperation::Intersect) {
            BRepAlgoAPI_Common common(m_existingBody, extruded);
            common.Build();
            if (common.IsDone()) return common.Shape();
        }
    } catch (const Standard_Failure& e) {
        std::cerr << "[ExtrudeFeature] Boolean operation failed: " << e.GetMessageString() << std::endl;
    }
    
    // Fallback: retourner le solide extrudé seul
    return extruded;
}

// ============================================================================
// compute() - Point d'entrée principal
// ============================================================================

bool ExtrudeFeature::compute() {
    try {
        // 1. Construire le solide extrudé
        TopoDS_Shape extruded = buildExtrudeShape();
        if (extruded.IsNull()) {
            std::cerr << "[ExtrudeFeature] Failed to build extrude shape" << std::endl;
            m_upToDate = false;
            return false;
        }
        
        // 2. Appliquer l'opération booléenne
        m_resultShape = applyOperation(extruded);
        m_shape = m_resultShape;
        
        m_upToDate = true;
        return true;
        
    } catch (const Standard_Failure& e) {
        std::cerr << "[ExtrudeFeature] compute() failed: " << e.GetMessageString() << std::endl;
        m_upToDate = false;
        return false;
    }
}

} // namespace CADEngine

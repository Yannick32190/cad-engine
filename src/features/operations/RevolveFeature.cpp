#include "RevolveFeature.h"

#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <TopoDS.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <iostream>
#include <cmath>

namespace CADEngine {

RevolveFeature::RevolveFeature(const std::string& name)
    : Feature(FeatureType::Revolve, name)
    , m_customAxis(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0))
{
    initializeParameters();
}

void RevolveFeature::initializeParameters() {
    setDouble("angle", 360.0);
    setInt("axis_type", static_cast<int>(RevolveAxisType::SketchX));
    setInt("operation", static_cast<int>(ExtrudeOperation::NewBody));
    setInt("profile_index", -1);
}

void RevolveFeature::setSketchFeature(std::shared_ptr<SketchFeature> sketch) {
    m_sketchFeature = sketch;
    if (sketch) {
        setInt("sketch_id", sketch->getId());
        addDependency(sketch->getId());
    }
}

void RevolveFeature::setAngle(double angleDeg) {
    setDouble("angle", angleDeg);
    invalidate();
}

void RevolveFeature::setAxisType(RevolveAxisType type) {
    setInt("axis_type", static_cast<int>(type));
    invalidate();
}

void RevolveFeature::setCustomAxis(const gp_Ax1& axis) {
    m_customAxis = axis;
    invalidate();
}

void RevolveFeature::setOperation(ExtrudeOperation op) {
    setInt("operation", static_cast<int>(op));
    invalidate();
}

void RevolveFeature::setExistingBody(const TopoDS_Shape& body) {
    m_existingBody = body;
}

void RevolveFeature::setProfileIndex(int index) {
    setInt("profile_index", index);
    invalidate();
}

void RevolveFeature::setSelectedFace(const TopoDS_Face& face) {
    m_selectedFace = face;
    invalidate();
}

double RevolveFeature::getAngle() const {
    return getDouble("angle");
}

RevolveAxisType RevolveFeature::getAxisType() const {
    return static_cast<RevolveAxisType>(getInt("axis_type"));
}

ExtrudeOperation RevolveFeature::getOperation() const {
    return static_cast<ExtrudeOperation>(getInt("operation"));
}

int RevolveFeature::getProfileIndex() const {
    return getInt("profile_index");
}

gp_Ax1 RevolveFeature::resolveAxis() const {
    RevolveAxisType axisType = getAxisType();
    
    if (axisType == RevolveAxisType::XAxis) {
        return gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0));
    }
    else if (axisType == RevolveAxisType::YAxis) {
        return gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0));
    }
    else if (axisType == RevolveAxisType::SketchX && m_sketchFeature) {
        gp_Pln plane = m_sketchFeature->getSketch2D()->getPlane();
        gp_Pnt origin = plane.Location();
        gp_Dir xDir = plane.XAxis().Direction();
        return gp_Ax1(origin, xDir);
    }
    else if (axisType == RevolveAxisType::SketchY && m_sketchFeature) {
        gp_Pln plane = m_sketchFeature->getSketch2D()->getPlane();
        gp_Pnt origin = plane.Location();
        gp_Dir yDir = plane.YAxis().Direction();
        return gp_Ax1(origin, yDir);
    }
    else if (axisType == RevolveAxisType::CustomLine) {
        return m_customAxis;
    }
    
    // Défaut: axe Y
    return gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0));
}

TopoDS_Face RevolveFeature::buildFaceFromSketch() const {
    // If a face was directly selected in viewport (Fusion 360 style), use it
    if (!m_selectedFace.IsNull()) {
        return m_selectedFace;
    }
    
    if (!m_sketchFeature) return TopoDS_Face();
    
    auto sketch2D = m_sketchFeature->getSketch2D();
    if (!sketch2D || sketch2D->getEntityCount() == 0) return TopoDS_Face();
    
    gp_Pln plane = sketch2D->getPlane();
    int profileIdx = getProfileIndex();
    
    // Use detectClosedProfiles for reliable detection
    auto profiles = sketch2D->detectClosedProfiles();
    if (!profiles.empty()) {
        int idx = (profileIdx >= 0 && profileIdx < (int)profiles.size()) ? profileIdx : 0;
        if (!profiles[idx].face.IsNull()) return profiles[idx].face;
        
        try {
            BRepBuilderAPI_MakeFace faceMaker(plane, profiles[idx].wire, Standard_True);
            if (faceMaker.IsDone()) return faceMaker.Face();
        } catch (...) {}
    }
    
    // Fallback: FreeBounds
    auto edges = sketch2D->getEdges3D();
    if (edges.empty()) return TopoDS_Face();
    
    Handle(TopTools_HSequenceOfShape) edgeSeq = new TopTools_HSequenceOfShape();
    for (const auto& edge : edges) {
        if (!edge.IsNull()) edgeSeq->Append(edge);
    }
    
    Handle(TopTools_HSequenceOfShape) wires = new TopTools_HSequenceOfShape();
    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edgeSeq, 0.5, Standard_False, wires);
    
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

TopoDS_Shape RevolveFeature::buildRevolveShape() const {
    TopoDS_Face face = buildFaceFromSketch();
    if (face.IsNull()) return TopoDS_Shape();
    
    gp_Ax1 axis = resolveAxis();
    double angleDeg = getAngle();
    double angleRad = angleDeg * M_PI / 180.0;
    
    try {
        if (std::abs(angleDeg - 360.0) < 0.01) {
            // Révolution complète
            BRepPrimAPI_MakeRevol revol(face, axis);
            revol.Build();
            if (revol.IsDone()) return revol.Shape();
        } else {
            // Révolution partielle
            BRepPrimAPI_MakeRevol revol(face, axis, angleRad);
            revol.Build();
            if (revol.IsDone()) return revol.Shape();
        }
    } catch (const Standard_Failure& e) {
        std::cerr << "[RevolveFeature] OCCT error: " << e.GetMessageString() << std::endl;
    }
    
    return TopoDS_Shape();
}

bool RevolveFeature::compute() {
    try {
        TopoDS_Shape revolved = buildRevolveShape();
        if (revolved.IsNull()) {
            m_upToDate = false;
            return false;
        }
        
        ExtrudeOperation op = getOperation();
        if (op == ExtrudeOperation::NewBody || m_existingBody.IsNull()) {
            m_resultShape = revolved;
        }
        else if (op == ExtrudeOperation::Join) {
            BRepAlgoAPI_Fuse fuse(m_existingBody, revolved);
            fuse.Build();
            m_resultShape = fuse.IsDone() ? fuse.Shape() : revolved;
        }
        else if (op == ExtrudeOperation::Cut) {
            BRepAlgoAPI_Cut cut(m_existingBody, revolved);
            cut.Build();
            m_resultShape = cut.IsDone() ? cut.Shape() : m_existingBody;
        }
        else if (op == ExtrudeOperation::Intersect) {
            BRepAlgoAPI_Common common(m_existingBody, revolved);
            common.Build();
            m_resultShape = common.IsDone() ? common.Shape() : revolved;
        }
        
        m_shape = m_resultShape;
        m_upToDate = true;
        return true;
        
    } catch (const Standard_Failure& e) {
        std::cerr << "[RevolveFeature] compute() failed: " << e.GetMessageString() << std::endl;
        m_upToDate = false;
        return false;
    }
}

} // namespace CADEngine

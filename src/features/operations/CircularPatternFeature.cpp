#include "CircularPatternFeature.h"

#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>

#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace CADEngine {

CircularPatternFeature::CircularPatternFeature(const std::string& name)
    : Feature(FeatureType::CircularPattern, name)
    , m_axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1))
{
    setInt("count", 6);
    setDouble("angle", 360.0);
    setBool("equal_spacing", true);
    setInt("feature_op", 0);
    setDouble("axis_x", 0.0);
    setDouble("axis_y", 0.0);
    setDouble("axis_z", 1.0);
}

void CircularPatternFeature::setAxis(const gp_Ax1& axis) {
    m_axis = axis;
    setDouble("axis_x", axis.Direction().X());
    setDouble("axis_y", axis.Direction().Y());
    setDouble("axis_z", axis.Direction().Z());
    invalidate();
}

void CircularPatternFeature::setCount(int count) {
    setInt("count", count);
    invalidate();
}

void CircularPatternFeature::setAngle(double angleDeg) {
    setDouble("angle", angleDeg);
    invalidate();
}

void CircularPatternFeature::setEqualSpacing(bool equal) {
    setBool("equal_spacing", equal);
    invalidate();
}

int CircularPatternFeature::getCount() const {
    return getInt("count");
}

double CircularPatternFeature::getAngle() const {
    return getDouble("angle");
}

bool CircularPatternFeature::isEqualSpacing() const {
    return getBool("equal_spacing");
}

bool CircularPatternFeature::compute() {
    std::cout << "[CircularPattern] Computing..." << std::endl;
    
    int count = getCount();
    double totalAngleDeg = getAngle();
    
    // Mode feature (correct) ou legacy (fallback)
    bool hasFeature = !m_featureShape.IsNull() && !m_existingBody.IsNull();
    bool hasBase = !m_baseShape.IsNull();
    
    if (!hasFeature && !hasBase) {
        std::cerr << "[CircularPattern] ERROR: No shape to pattern" << std::endl;
        return false;
    }
    
    if (count < 2) {
        m_resultShape = hasFeature ? m_existingBody : m_baseShape;
        m_shape = m_resultShape;
        m_upToDate = true;
        return true;
    }
    
    try {
        int featureOp = hasFeature ? getFeatureOperation() : 0;
        
        TopoDS_Shape result = hasFeature ? m_existingBody : m_baseShape;
        TopoDS_Shape shapeToPattern = hasFeature ? m_featureShape : m_baseShape;
        
        // Calculer l'angle entre chaque copie
        double stepAngle;
        if (std::abs(totalAngleDeg - 360.0) < 0.01) {
            stepAngle = totalAngleDeg / count;
        } else {
            stepAngle = totalAngleDeg / (count - 1);
        }
        double stepAngleRad = stepAngle * M_PI / 180.0;
        
        // hasFeature: existingBody n'a PAS la feature → inclure i=0
        // legacy: baseShape EST le body complet → commencer à i=1
        int startIdx = hasFeature ? 0 : 1;
        
        for (int i = startIdx; i < count; i++) {
            double angle = i * stepAngleRad;
            
            gp_Trsf transform;
            transform.SetRotation(m_axis, angle);
            
            BRepBuilderAPI_Transform xform(shapeToPattern, transform, true);
            if (!xform.IsDone()) continue;
            
            TopoDS_Shape copy = xform.Shape();
            
            if (hasFeature && featureOp == 2) {
                BRepAlgoAPI_Cut cut(result, copy);
                if (cut.IsDone()) result = cut.Shape();
            } else {
                BRepAlgoAPI_Fuse fuse(result, copy);
                if (fuse.IsDone()) result = fuse.Shape();
            }
        }
        
        m_resultShape = result;
        m_shape = result;
        m_upToDate = true;
        
        std::cout << "[CircularPattern] Success: " << count << " instances, angle=" 
                  << totalAngleDeg << "°, mode=" << (hasFeature ? "feature" : "legacy") << std::endl;
        return true;
        
    } catch (Standard_Failure& e) {
        std::cerr << "[CircularPattern] OCCT Exception: " << e.GetMessageString() << std::endl;
        return false;
    }
}

} // namespace CADEngine

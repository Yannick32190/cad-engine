#include "LinearPatternFeature.h"

#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <iostream>

namespace CADEngine {

LinearPatternFeature::LinearPatternFeature(const std::string& name)
    : Feature(FeatureType::LinearPattern, name)
{
    // Direction 1
    setInt("count", 3);
    setDouble("spacing", 20.0);
    setDouble("dir_x", 1.0);
    setDouble("dir_y", 0.0);
    setDouble("dir_z", 0.0);
    setBool("symmetric", false);
    // Direction 2
    setInt("count2", 1);
    setDouble("spacing2", 20.0);
    setDouble("dir2_x", 0.0);
    setDouble("dir2_y", 1.0);
    setDouble("dir2_z", 0.0);
    setBool("symmetric2", false);
    setBool("second_dir_enabled", false);
    
    setInt("feature_op", 0);
}

// Direction 1
void LinearPatternFeature::setDirection(const gp_Dir& dir) {
    setDouble("dir_x", dir.X()); setDouble("dir_y", dir.Y()); setDouble("dir_z", dir.Z());
    invalidate();
}
void LinearPatternFeature::setCount(int count) { setInt("count", count); invalidate(); }
void LinearPatternFeature::setSpacing(double spacing) { setDouble("spacing", spacing); invalidate(); }
void LinearPatternFeature::setSymmetric(bool sym) { setBool("symmetric", sym); invalidate(); }

gp_Dir LinearPatternFeature::getDirection() const {
    return gp_Dir(getDouble("dir_x"), getDouble("dir_y"), getDouble("dir_z"));
}
int LinearPatternFeature::getCount() const { return getInt("count"); }
double LinearPatternFeature::getSpacing() const { return getDouble("spacing"); }
bool LinearPatternFeature::isSymmetric() const { return getBool("symmetric"); }

// Direction 2
void LinearPatternFeature::setDirection2(const gp_Dir& dir) {
    setDouble("dir2_x", dir.X()); setDouble("dir2_y", dir.Y()); setDouble("dir2_z", dir.Z());
    invalidate();
}
void LinearPatternFeature::setCount2(int count) { setInt("count2", count); invalidate(); }
void LinearPatternFeature::setSpacing2(double spacing) { setDouble("spacing2", spacing); invalidate(); }
void LinearPatternFeature::setSymmetric2(bool sym) { setBool("symmetric2", sym); invalidate(); }
void LinearPatternFeature::setSecondDirectionEnabled(bool enabled) { setBool("second_dir_enabled", enabled); invalidate(); }

gp_Dir LinearPatternFeature::getDirection2() const {
    return gp_Dir(getDouble("dir2_x"), getDouble("dir2_y"), getDouble("dir2_z"));
}
int LinearPatternFeature::getCount2() const { return getInt("count2"); }
double LinearPatternFeature::getSpacing2() const { return getDouble("spacing2"); }
bool LinearPatternFeature::isSymmetric2() const { return getBool("symmetric2"); }
bool LinearPatternFeature::isSecondDirectionEnabled() const { return getBool("second_dir_enabled"); }

bool LinearPatternFeature::compute() {
    std::cout << "[LinearPattern] Computing..." << std::endl;
    
    bool hasFeature = !m_featureShape.IsNull() && !m_existingBody.IsNull();
    bool hasBase = !m_baseShape.IsNull();
    
    if (!hasFeature && !hasBase) {
        std::cerr << "[LinearPattern] ERROR: No shape to pattern" << std::endl;
        return false;
    }
    
    int count1 = getCount();
    double spacing1 = getSpacing();
    bool sym1 = isSymmetric();
    gp_Dir dir1 = getDirection();
    
    bool hasDir2 = isSecondDirectionEnabled();
    int count2 = hasDir2 ? getCount2() : 1;
    double spacing2 = hasDir2 ? getSpacing2() : 0;
    bool sym2 = hasDir2 ? isSymmetric2() : false;
    gp_Dir dir2 = hasDir2 ? getDirection2() : gp_Dir(0,1,0);
    
    int featureOp = hasFeature ? getFeatureOperation() : 0;
    TopoDS_Shape result = hasFeature ? m_existingBody : m_baseShape;
    TopoDS_Shape shapeToPattern = hasFeature ? m_featureShape : m_baseShape;
    
    try {
        // Générer toutes les positions (i,j) de la grille
        // Direction 1
        int start1, end1;
        if (sym1) { start1 = -(count1 / 2); end1 = count1 / 2; }
        else { start1 = hasFeature ? 0 : 1; end1 = count1 - 1; }
        
        // Direction 2
        int start2, end2;
        if (!hasDir2 || count2 < 2) { start2 = 0; end2 = 0; }
        else if (sym2) { start2 = -(count2 / 2); end2 = count2 / 2; }
        else { start2 = 0; end2 = count2 - 1; }
        
        for (int i = start1; i <= end1; i++) {
            for (int j = start2; j <= end2; j++) {
                // Skip l'original
                if (i == 0 && j == 0) {
                    if (!hasFeature && !sym1) continue;  // Legacy non-sym: original déjà dans body
                    if (hasFeature) {
                        // Mode feature: appliquer l'original à (0,0)
                    } else if (sym1 || sym2) {
                        continue;  // Sym legacy: skip (0,0)
                    }
                }
                // Legacy: skip original row when not in dir2
                if (!hasFeature && !sym1 && i == 0 && j == 0) continue;
                
                double offset1 = i * spacing1;
                double offset2 = j * spacing2;
                
                gp_Vec translation = gp_Vec(dir1) * offset1;
                if (hasDir2 && count2 >= 2) {
                    translation = translation + gp_Vec(dir2) * offset2;
                }
                
                gp_Trsf transform;
                transform.SetTranslation(translation);
                
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
        }
        
        m_resultShape = result;
        m_shape = result;
        m_upToDate = true;
        
        int total = (end1 - start1 + 1) * (end2 - start2 + 1);
        std::cout << "[LinearPattern] Success: " << total << " instances"
                  << (hasDir2 ? " (grille)" : "") << std::endl;
        return true;
        
    } catch (Standard_Failure& e) {
        std::cerr << "[LinearPattern] OCCT Exception: " << e.GetMessageString() << std::endl;
        return false;
    }
}

} // namespace CADEngine

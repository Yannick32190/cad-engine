#include "ShellFeature.h"
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <iostream>

namespace CADEngine {

ShellFeature::ShellFeature(const std::string& name)
    : Feature(FeatureType::Shell, name)
{
    setDouble("thickness", 2.0);
    setBool("outward", false);
}

bool ShellFeature::compute() {
    if (m_existingBody.IsNull()) {
        std::cerr << "[ShellFeature] No existing body" << std::endl;
        m_upToDate = false;
        return false;
    }
    
    if (m_openFaces.empty()) {
        std::cerr << "[ShellFeature] No open faces selected" << std::endl;
        m_upToDate = false;
        return false;
    }
    
    double thickness = getThickness();
    bool outward = isOutward();
    if (std::abs(thickness) < 1e-6) {
        std::cerr << "[ShellFeature] Thickness too small" << std::endl;
        m_upToDate = false;
        return false;
    }
    
    try {
        TopTools_ListOfShape facesToRemove;
        for (const auto& face : m_openFaces) {
            facesToRemove.Append(face);
        }
        
        // OCCT : offset négatif = vers l'intérieur, positif = vers l'extérieur
        double offset = outward ? thickness : -thickness;
        
        std::cout << "[ShellFeature] thickness=" << thickness 
                  << " outward=" << outward
                  << " offset=" << offset
                  << " faces=" << m_openFaces.size() << std::endl;
        
        BRepOffsetAPI_MakeThickSolid shellMaker;
        shellMaker.MakeThickSolidByJoin(
            m_existingBody,
            facesToRemove,
            offset,
            1e-3
        );
        shellMaker.Build();
        
        if (shellMaker.IsDone()) {
            m_resultShape = shellMaker.Shape();
            m_shape = m_resultShape;
            m_upToDate = true;
            int nFaces = 0;
            TopExp_Explorer fexp(m_resultShape, TopAbs_FACE);
            for (; fexp.More(); fexp.Next()) nFaces++;
            std::cout << "[ShellFeature] OK, faces=" << nFaces << std::endl;
            return true;
        } else {
            std::cerr << "[ShellFeature] MakeThickSolid failed" << std::endl;
        }
        
    } catch (const Standard_Failure& e) {
        std::cerr << "[ShellFeature] Exception: " << e.GetMessageString() << std::endl;
    } catch (...) {
        std::cerr << "[ShellFeature] Unknown error" << std::endl;
    }
    
    m_upToDate = false;
    return false;
}

} // namespace CADEngine

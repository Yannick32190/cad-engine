#include "Fillet3DFeature.h"

#include <BRepFilletAPI_MakeFillet.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <Precision.hxx>

#include <iostream>

namespace CADEngine {

Fillet3DFeature::Fillet3DFeature(const std::string& name)
    : Feature(FeatureType::Fillet, name)
{
    initializeParameters();
}

void Fillet3DFeature::initializeParameters() {
    setDouble("radius", 2.0);
    setBool("all_edges", false);
}

void Fillet3DFeature::setBaseShape(const TopoDS_Shape& shape) {
    m_baseShape = shape;
    invalidate();
}

void Fillet3DFeature::setRadius(double radius) {
    setDouble("radius", radius);
    invalidate();
}

void Fillet3DFeature::addEdge(const TopoDS_Edge& edge) {
    m_selectedEdges.push_back(edge);
    invalidate();
}

void Fillet3DFeature::clearEdges() {
    m_selectedEdges.clear();
    invalidate();
}

void Fillet3DFeature::setAllEdges(bool allEdges) {
    m_allEdges = allEdges;
    setBool("all_edges", allEdges);
    invalidate();
}

double Fillet3DFeature::getRadius() const {
    return getDouble("radius");
}

bool Fillet3DFeature::compute() {
    if (m_baseShape.IsNull()) {
        m_upToDate = false;
        return false;
    }
    
    double radius = getRadius();
    if (radius < Precision::Confusion()) {
        m_upToDate = false;
        return false;
    }
    
    try {
        BRepFilletAPI_MakeFillet fillet(m_baseShape);
        
        if (m_allEdges) {
            // Toutes les arêtes
            TopExp_Explorer explorer(m_baseShape, TopAbs_EDGE);
            for (; explorer.More(); explorer.Next()) {
                fillet.Add(radius, TopoDS::Edge(explorer.Current()));
            }
        } else {
            // Arêtes sélectionnées
            for (const auto& edge : m_selectedEdges) {
                fillet.Add(radius, edge);
            }
        }
        
        fillet.Build();
        if (fillet.IsDone()) {
            m_resultShape = fillet.Shape();
            m_shape = m_resultShape;
            m_upToDate = true;
            return true;
        }
        
    } catch (const Standard_Failure& e) {
        std::cerr << "[Fillet3D] OCCT error: " << e.GetMessageString() << std::endl;
    }
    
    m_upToDate = false;
    return false;
}

} // namespace CADEngine

#include "Chamfer3DFeature.h"

#include <BRepFilletAPI_MakeChamfer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <Precision.hxx>

#include <iostream>

namespace CADEngine {

Chamfer3DFeature::Chamfer3DFeature(const std::string& name)
    : Feature(FeatureType::Chamfer, name)
{
    initializeParameters();
}

void Chamfer3DFeature::initializeParameters() {
    setDouble("distance", 2.0);
    setBool("all_edges", false);
}

void Chamfer3DFeature::setBaseShape(const TopoDS_Shape& shape) {
    m_baseShape = shape;
    invalidate();
}

void Chamfer3DFeature::setDistance(double distance) {
    setDouble("distance", distance);
    invalidate();
}

void Chamfer3DFeature::addEdge(const TopoDS_Edge& edge) {
    m_selectedEdges.push_back(edge);
    invalidate();
}

void Chamfer3DFeature::clearEdges() {
    m_selectedEdges.clear();
    invalidate();
}

void Chamfer3DFeature::setAllEdges(bool allEdges) {
    m_allEdges = allEdges;
    setBool("all_edges", allEdges);
    invalidate();
}

double Chamfer3DFeature::getDistance() const {
    return getDouble("distance");
}

bool Chamfer3DFeature::compute() {
    if (m_baseShape.IsNull()) {
        m_upToDate = false;
        return false;
    }
    
    double dist = getDistance();
    if (dist < Precision::Confusion()) {
        m_upToDate = false;
        return false;
    }
    
    try {
        BRepFilletAPI_MakeChamfer chamfer(m_baseShape);
        
        if (m_allEdges) {
            TopExp_Explorer explorer(m_baseShape, TopAbs_EDGE);
            for (; explorer.More(); explorer.Next()) {
                TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
                chamfer.Add(dist, edge);
            }
        } else {
            for (const auto& edge : m_selectedEdges) {
                chamfer.Add(dist, edge);
            }
        }
        
        chamfer.Build();
        if (chamfer.IsDone()) {
            m_resultShape = chamfer.Shape();
            m_shape = m_resultShape;
            m_upToDate = true;
            return true;
        }
        
    } catch (const Standard_Failure& e) {
        std::cerr << "[Chamfer3D] OCCT error: " << e.GetMessageString() << std::endl;
    }
    
    m_upToDate = false;
    return false;
}

} // namespace CADEngine

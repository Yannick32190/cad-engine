#ifndef CHAMFER3DFEATURE_H
#define CHAMFER3DFEATURE_H

#include "../../core/parametric/Feature.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <vector>

namespace CADEngine {

class Chamfer3DFeature : public Feature {
public:
    Chamfer3DFeature(const std::string& name = "Chanfrein 3D");
    virtual ~Chamfer3DFeature() = default;
    
    bool compute() override;
    std::string getTypeName() const override { return "Chamfer3D"; }
    
    void setBaseShape(const TopoDS_Shape& shape);
    void setDistance(double distance);
    void addEdge(const TopoDS_Edge& edge);
    void clearEdges();
    void setAllEdges(bool allEdges);
    
    double getDistance() const;
    const std::vector<TopoDS_Edge>& getSelectedEdges() const { return m_selectedEdges; }
    bool isAllEdges() const { return m_allEdges; }
    TopoDS_Shape getResultShape() const { return m_resultShape; }

private:
    void initializeParameters();
    
    TopoDS_Shape m_baseShape;
    TopoDS_Shape m_resultShape;
    std::vector<TopoDS_Edge> m_selectedEdges;
    bool m_allEdges = false;
};

} // namespace CADEngine

#endif

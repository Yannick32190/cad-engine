#ifndef FILLET3DFEATURE_H
#define FILLET3DFEATURE_H

#include "../../core/parametric/Feature.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <vector>

namespace CADEngine {

/**
 * @brief Fillet3DFeature - Congé 3D sur arêtes d'un solide
 */
class Fillet3DFeature : public Feature {
public:
    Fillet3DFeature(const std::string& name = "Congé 3D");
    virtual ~Fillet3DFeature() = default;
    
    bool compute() override;
    std::string getTypeName() const override { return "Fillet3D"; }
    
    void setBaseShape(const TopoDS_Shape& shape);
    void setRadius(double radius);
    void addEdge(const TopoDS_Edge& edge);
    void clearEdges();
    void setAllEdges(bool allEdges);
    
    double getRadius() const;
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

#endif // FILLET3DFEATURE_H

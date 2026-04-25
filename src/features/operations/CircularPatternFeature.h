#ifndef CIRCULARPATTERNFEATURE_H
#define CIRCULARPATTERNFEATURE_H

#include "../../core/parametric/Feature.h"
#include <gp_Ax1.hxx>
#include <TopoDS_Shape.hxx>

namespace CADEngine {

/**
 * @brief CircularPatternFeature - Réseau circulaire (répétition autour d'un axe)
 *
 * Comme LinearPattern : duplique la feature, pas le body complet.
 */
class CircularPatternFeature : public Feature {
public:
    CircularPatternFeature(const std::string& name = "Réseau circulaire");
    virtual ~CircularPatternFeature() = default;
    
    bool compute() override;
    std::string getTypeName() const override { return "CircularPattern"; }
    
    // Configuration
    void setBaseShape(const TopoDS_Shape& shape) { m_baseShape = shape; invalidate(); }
    void setExistingBody(const TopoDS_Shape& body) { m_existingBody = body; }
    void setFeatureShape(const TopoDS_Shape& shape) { m_featureShape = shape; }
    void setFeatureOperation(int op) { setInt("feature_op", op); }
    
    void setAxis(const gp_Ax1& axis);
    void setCount(int count);
    void setAngle(double angleDeg);
    void setEqualSpacing(bool equal);
    
    // Getters
    gp_Ax1 getAxis() const { return m_axis; }
    int getCount() const;
    double getAngle() const;
    bool isEqualSpacing() const;
    int getFeatureOperation() const { return getInt("feature_op"); }
    TopoDS_Shape getResultShape() const { return m_resultShape; }

private:
    TopoDS_Shape m_baseShape;
    TopoDS_Shape m_existingBody;
    TopoDS_Shape m_featureShape;
    TopoDS_Shape m_resultShape;
    gp_Ax1 m_axis;
};

} // namespace CADEngine

#endif // CIRCULARPATTERNFEATURE_H

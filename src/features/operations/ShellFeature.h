#ifndef SHELLFEATURE_H
#define SHELLFEATURE_H

#include "../../core/parametric/Feature.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopTools_ListOfShape.hxx>

namespace CADEngine {

/**
 * @brief ShellFeature — Coque / Évidement de solide
 * 
 * Évide un solide en retirant une ou plusieurs faces et en appliquant
 * une épaisseur uniforme. Équivalent de "Shell" dans Fusion 360 / SolidWorks.
 */
class ShellFeature : public Feature {
public:
    ShellFeature(const std::string& name = "Coque");
    virtual ~ShellFeature() = default;
    
    bool compute() override;
    std::string getTypeName() const override { return "Shell"; }
    
    // Configuration
    void setExistingBody(const TopoDS_Shape& body) { m_existingBody = body; }
    void addOpenFace(const TopoDS_Face& face) { m_openFaces.push_back(face); }
    void clearOpenFaces() { m_openFaces.clear(); }
    
    void setThickness(double t) { setDouble("thickness", t); invalidate(); }
    double getThickness() const { return getDouble("thickness"); }
    
    void setOutward(bool o) { setBool("outward", o); invalidate(); }
    bool isOutward() const { return getBool("outward"); }
    
    // Résultat
    TopoDS_Shape getResultShape() const { return m_resultShape; }
    const std::vector<TopoDS_Face>& getOpenFaces() const { return m_openFaces; }

private:
    TopoDS_Shape m_existingBody;
    std::vector<TopoDS_Face> m_openFaces;  // Faces à retirer (ouvertures)
    TopoDS_Shape m_resultShape;
};

} // namespace CADEngine

#endif // SHELLFEATURE_H

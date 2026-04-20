#ifndef EXTRUDEFEATURE_H
#define EXTRUDEFEATURE_H

#include "../../core/parametric/Feature.h"
#include "../sketch/SketchFeature.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <memory>

namespace CADEngine {

enum class ExtrudeDirection {
    OneSide,        // Distance dans un sens (normal au plan)
    TwoSides,       // Distances différentes de chaque côté
    Symmetric       // Même distance des deux côtés
};

enum class ExtrudeOperation {
    NewBody,        // Créer un nouveau corps
    Join,           // Fusionner avec le corps existant (union)
    Cut,            // Soustraire du corps existant (poche)
    Intersect       // Intersection avec le corps existant
};

/**
 * @brief ExtrudeFeature - Extrusion d'un profil sketch (style Fusion 360)
 * 
 * Paramètres :
 * - distance (double) : Distance d'extrusion côté 1
 * - distance2 (double) : Distance d'extrusion côté 2 (pour TwoSides)
 * - direction (int) : ExtrudeDirection
 * - operation (int) : ExtrudeOperation
 * - sketch_id (int) : ID du SketchFeature source
 * 
 * compute() :
 * 1. Récupère le profil fermé du sketch
 * 2. Crée une face à partir du wire
 * 3. Extrude avec BRepPrimAPI_MakePrism
 * 4. Applique l'opération booléenne si nécessaire
 */
class ExtrudeFeature : public Feature {
public:
    ExtrudeFeature(const std::string& name = "Extrusion");
    virtual ~ExtrudeFeature() = default;
    
    bool compute() override;
    std::string getTypeName() const override { return "Extrude"; }
    
    // Configuration
    void setSketchFeature(std::shared_ptr<SketchFeature> sketch);
    void setDistance(double distance);
    void setDistance2(double distance);
    void setDirection(ExtrudeDirection dir);
    void setOperation(ExtrudeOperation op);
    void setExistingBody(const TopoDS_Shape& body);
    void setProfileIndex(int index);  // Which closed profile to extrude (-1 = auto)
    void setSelectedFace(const TopoDS_Face& face);  // Direct face from viewport picking
    
    // Getters
    std::shared_ptr<SketchFeature> getSketchFeature() const { return m_sketchFeature; }
    double getDistance() const;
    double getDistance2() const;
    ExtrudeDirection getDirection() const;
    ExtrudeOperation getOperation() const;
    int getProfileIndex() const;
    TopoDS_Shape getResultShape() const { return m_resultShape; }
    
    // Helper: construire le solide d'extrusion sans opération booléenne (pour preview)
    TopoDS_Shape buildExtrudeShape() const;

private:
    void initializeParameters();
    
    // Construire face(s) depuis le sketch
    TopoDS_Face buildFaceFromSketch() const;
    
    // Appliquer opération booléenne
    TopoDS_Shape applyOperation(const TopoDS_Shape& extruded) const;
    
    std::shared_ptr<SketchFeature> m_sketchFeature;
    TopoDS_Shape m_existingBody;  // Corps existant pour Join/Cut/Intersect
    TopoDS_Shape m_resultShape;   // Résultat final
    TopoDS_Face m_selectedFace;   // Face sélectionnée dans viewport (Fusion 360)
};

} // namespace CADEngine

#endif // EXTRUDEFEATURE_H

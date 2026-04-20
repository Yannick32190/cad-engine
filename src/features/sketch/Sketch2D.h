#ifndef SKETCH2D_H
#define SKETCH2D_H

#include "SketchEntity.h"
#include "Dimension.h"
#include "GeometricConstraint.h"
#include <gp_Pln.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Wire.hxx>
#include <vector>
#include <memory>
#include <map>

namespace CADEngine {

/**
 * @brief Système de sketch 2D dans un plan 3D
 * 
 * Gère la conversion entre coordonnées 2D (dans le plan) et 3D (globales)
 * Stocke les entités dessinées dans le plan
 */
class Sketch2D {
public:
    /**
     * @brief Constructeur avec plan
     * @param plane Plan de travail (XY, YZ, ZX ou custom)
     */
    explicit Sketch2D(const gp_Pln& plane);
    
    // Plan
    gp_Pln getPlane() const { return m_plane; }
    void setPlane(const gp_Pln& plane);
    void setFaceCenterOffset(double u, double v, double zoom = 0.0) { m_faceCenterU = u; m_faceCenterV = v; m_faceAutoZoom = zoom; }
    double getFaceCenterU() const { return m_faceCenterU; }
    double getFaceCenterV() const { return m_faceCenterV; }
    double getFaceAutoZoom() const { return m_faceAutoZoom; }
    
    // Système de coordonnées local
    gp_Ax2 getLocalAxes() const { return m_localAxes; }
    
    // Conversion 2D ↔ 3D
    /**
     * @brief Convertit un point 3D en coordonnées 2D du plan
     * @param pt3D Point 3D global
     * @return Point 2D dans le plan
     */
    gp_Pnt2d toLocal(const gp_Pnt& pt3D) const;
    
    /**
     * @brief Convertit un point 2D du plan en 3D global
     * @param pt2D Point 2D dans le plan
     * @return Point 3D global
     */
    gp_Pnt toGlobal(const gp_Pnt2d& pt2D) const;
    
    // Entités
    void addEntity(std::shared_ptr<SketchEntity> entity);
    void removeEntity(std::shared_ptr<SketchEntity> entity);
    void clearEntities();
    
    std::vector<std::shared_ptr<SketchEntity>> getEntities() const { return m_entities; }
    std::vector<std::shared_ptr<SketchEntity>>& getEntitiesRef() { return m_entities; }
    int getEntityCount() const { return static_cast<int>(m_entities.size()); }
    
    // Dimensions (cotations)
    void addDimension(std::shared_ptr<Dimension> dimension);
    void removeDimension(std::shared_ptr<Dimension> dimension);
    void clearDimensions();
    
    std::vector<std::shared_ptr<Dimension>> getDimensions() const { return m_dimensions; }
    std::vector<std::shared_ptr<Dimension>>& getDimensionsRef() { return m_dimensions; }
    int getDimensionCount() const { return static_cast<int>(m_dimensions.size()); }
    
    // Auto-dimensions (cotations automatiques sur toutes les entités)
    void regenerateAutoDimensions();
    std::vector<std::shared_ptr<Dimension>> getAutoDimensions() const { return m_autoDimensions; }
    
    // Lignes de construction d'axes (créées automatiquement)
    void createAxisConstructionLines();
    std::shared_ptr<SketchLine> getAxisLineH() const { return m_axisLineH; }
    std::shared_ptr<SketchLine> getAxisLineV() const { return m_axisLineV; }
    
    // Offsets persistants pour auto-cotations (survit à la régénération)
    void setAutoDimOffset(int entityId, const std::string& property, double offset);
    double getAutoDimOffset(int entityId, const std::string& property, double defaultOffset) const;
    
    // Angles persistants pour auto-cotations Diametral/Radial
    void setAutoDimAngle(int entityId, const std::string& property, double angle);
    double getAutoDimAngle(int entityId, const std::string& property, double defaultAngle) const;
    
    // Sélection
    std::shared_ptr<SketchEntity> findEntityAt(const gp_Pnt2d& pt, double tolerance = 5.0) const;
    void selectAll();
    void deselectAll();
    
    // Contraintes géométriques
    void addConstraint(std::shared_ptr<GeometricConstraint> constraint);
    void removeConstraint(std::shared_ptr<GeometricConstraint> constraint);
    void clearConstraints();
    std::vector<std::shared_ptr<GeometricConstraint>> getConstraints() const { return m_constraints; }
    std::vector<std::shared_ptr<GeometricConstraint>>& getConstraintsRef() { return m_constraints; }
    void solveConstraints();  // Applique toutes les contraintes
    
    // Génération géométrie 3D
    std::vector<TopoDS_Edge> getEdges3D() const;
    TopoDS_Wire getWire3D() const;  // Si contour fermé
    
    // Détection de profils fermés (pour extrusion)
    std::vector<TopoDS_Wire> getClosedWires3D() const;
    
    // Profil fermé avec métadonnées (pour sélection dans le viewport)
    struct ClosedProfile {
        TopoDS_Wire wire;
        TopoDS_Face face;          // Face plane pour preview
        std::string description;   // "Rectangle 50x30", "Cercle R25", etc.
        int entityIndex = -1;      // Index entité source (-1 = composite)
    };
    std::vector<ClosedProfile> detectClosedProfiles() const;
    
    // Régions cliquables (gère les profils imbriqués: rectangle - cercle = donut)
    struct FaceRegion {
        TopoDS_Face face;          // Face avec trous éventuels
        std::string description;   // "Rectangle 520x295 (1 trou)", "Cercle R50"
        double area = 0.0;         // Pour tri/affichage
    };
    std::vector<FaceRegion> buildFaceRegions() const;

private:
    gp_Pln m_plane;         // Plan de travail
    gp_Ax2 m_localAxes;     // Système coord 2D (X, Y dans le plan)
    double m_faceCenterU = 0.0;  // Camera center offset for face-based sketches
    double m_faceCenterV = 0.0;
    double m_faceAutoZoom = 0.0; // Auto-zoom extent for face-based sketches
    
    std::vector<std::shared_ptr<SketchEntity>> m_entities;
    std::vector<std::shared_ptr<Dimension>> m_dimensions;
    std::vector<std::shared_ptr<Dimension>> m_autoDimensions;
    std::shared_ptr<SketchLine> m_axisLineH;  // Axe horizontal (construction)
    std::shared_ptr<SketchLine> m_axisLineV;  // Axe vertical (construction)
    std::map<std::string, double> m_autoDimOffsets;  // key="entityId:property" → offset
    std::map<std::string, double> m_autoDimAngles;   // key="entityId:property" → angle
    std::vector<std::shared_ptr<GeometricConstraint>> m_constraints;
};

} // namespace CADEngine

#endif // SKETCH2D_H

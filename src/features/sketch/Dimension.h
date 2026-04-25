#ifndef DIMENSION_H
#define DIMENSION_H

#include <gp_Pnt2d.hxx>
#include <memory>
#include <string>
#include <utility>

namespace CADEngine {

// Forward declarations
class SketchLine;
class SketchPolyline;
class SketchCircle;
class SketchArc;
class SketchRectangle;
class SketchEntity;

// ============================================================================
// Types de dimensions
// ============================================================================

enum class DimensionType {
    Linear,      // Distance entre 2 points
    Angular,     // Angle entre 2 lignes
    Radial,      // Rayon d'un cercle/arc
    Diametral    // Diamètre d'un cercle
};

// ============================================================================
// Dimension - Classe de base abstraite
// ============================================================================

class Dimension {
public:
    Dimension(DimensionType type);
    virtual ~Dimension() = default;
    
    // Type
    DimensionType getType() const { return m_type; }
    
    // Identification
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }
    
    // Valeur
    virtual double getValue() const = 0;
    virtual void setValue(double value) = 0;
    
    // Texte affiché
    std::string getText() const { return m_text; }
    void setText(const std::string& text) { m_text = text; }
    
    // Position du texte (point d'ancrage)
    gp_Pnt2d getTextPosition() const { return m_textPosition; }
    void setTextPosition(const gp_Pnt2d& pos) { m_textPosition = pos; }
    
    // Visibilité
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    
    // Auto-dimension (générée automatiquement)
    bool isAutomatic() const { return m_isAutomatic; }
    void setAutomatic(bool automatic) { m_isAutomatic = automatic; }
    
    // Source entity pour auto-dimensions éditables
    void setAutoSource(std::shared_ptr<SketchEntity> entity, const std::string& property) {
        m_autoSourceEntity = entity; m_autoSourceProperty = property;
    }
    std::shared_ptr<SketchEntity> getAutoSourceEntity() const { return m_autoSourceEntity; }
    std::string getAutoSourceProperty() const { return m_autoSourceProperty; }
    
    // Métadonnées pour tracer l'entité associée
    void setEntityId(int entityId) { m_entityId = entityId; }
    int getEntityId() const { return m_entityId; }
    void setSegmentIndex(int index) { m_segmentIndex = index; }
    int getSegmentIndex() const { return m_segmentIndex; }
    
protected:
    DimensionType m_type;
    int m_id = -1;
    std::string m_text;
    gp_Pnt2d m_textPosition;
    bool m_visible = true;
    bool m_isAutomatic = false;
    
    // Source pour auto-dimensions éditables
    std::shared_ptr<SketchEntity> m_autoSourceEntity;
    std::string m_autoSourceProperty;  // "width", "height", "radius", "diameter", "length", etc.
    
    // Métadonnées pour tracer l'entité source
    int m_entityId = -1;      // ID de l'entité (ligne, cercle, polyligne...)
    int m_segmentIndex = -1;  // Index du segment (pour polyligne/rectangle)
    
    static int s_nextId;
};

// ============================================================================
// LinearDimension - Cotation linéaire (distance entre 2 points)
// ============================================================================

class LinearDimension : public Dimension {
public:
    LinearDimension(const gp_Pnt2d& point1, const gp_Pnt2d& point2);
    
    // Points mesurés
    gp_Pnt2d getPoint1() const { return m_point1; }
    gp_Pnt2d getPoint2() const { return m_point2; }
    void setPoint1(const gp_Pnt2d& pt) { m_point1 = pt; updateText(); }
    void setPoint2(const gp_Pnt2d& pt) { m_point2 = pt; updateText(); }
    void updateText();  // Recalculer le texte après modification points
    
    // Valeur (distance calculée)
    double getValue() const override;
    void setValue(double value) override; // Pour contraintes (future feature)
    
    // Offset de la ligne de cote (distance perpendiculaire à la ligne mesurée)
    double getOffset() const { return m_offset; }
    void setOffset(double offset) { m_offset = offset; updateText(); }
    
    // Points de la ligne de cote (calculés automatiquement)
    gp_Pnt2d getDimensionLineStart() const;
    gp_Pnt2d getDimensionLineEnd() const;
    
    // Points des lignes d'attache (extension lines)
    gp_Pnt2d getExtensionLine1Start() const { return m_point1; }
    gp_Pnt2d getExtensionLine1End() const;
    gp_Pnt2d getExtensionLine2Start() const { return m_point2; }
    gp_Pnt2d getExtensionLine2End() const;
    
    // === Source entities (pour cotations cross-entity polyvalentes) ===
    // vertexIndex: -1=start, -2=end, -3=center, >=0=polyline/rect vertex index
    void setSourceEntity1(std::shared_ptr<SketchEntity> entity, int vtxIdx) {
        m_sourceEntity1 = entity; m_sourceVtx1 = vtxIdx;
    }
    void setSourceEntity2(std::shared_ptr<SketchEntity> entity, int vtxIdx) {
        m_sourceEntity2 = entity; m_sourceVtx2 = vtxIdx;
    }
    
    std::shared_ptr<SketchEntity> getSourceEntity1() const { return m_sourceEntity1; }
    std::shared_ptr<SketchEntity> getSourceEntity2() const { return m_sourceEntity2; }
    int getSourceVtx1() const { return m_sourceVtx1; }
    int getSourceVtx2() const { return m_sourceVtx2; }
    bool hasSources() const { return m_sourceEntity1 != nullptr || m_sourceEntity2 != nullptr; }
    
    // Rafraîchir m_point1/m_point2 depuis les entités sources (après drag)
    void refreshFromSources();
    
    // Helper: extraire un point d'une entité selon son vertexIndex
    static gp_Pnt2d extractPoint(std::shared_ptr<SketchEntity> entity, int vtxIdx);
    
private:
    gp_Pnt2d m_point1;
    gp_Pnt2d m_point2;
    double m_offset = 20.0;  // Offset par défaut : 20mm
    
    // Entités sources pour cotations cross-entity
    std::shared_ptr<SketchEntity> m_sourceEntity1;
    std::shared_ptr<SketchEntity> m_sourceEntity2;
    int m_sourceVtx1 = -99;  // -99 = pas de source
    int m_sourceVtx2 = -99;
};

// ============================================================================
// ============================================================================
// AngularDimension - Cotation angulaire entre 2 segments partageant un vertex
// Stocke: center (vertex commun), point1 (extrémité seg1), point2 (extrémité seg2)
// L'angle mesuré est toujours l'angle intérieur (≤180°)
// ============================================================================

class AngularDimension : public Dimension {
public:
    AngularDimension(const gp_Pnt2d& center, const gp_Pnt2d& point1, const gp_Pnt2d& point2);
    
    // Accesseurs géométrie
    gp_Pnt2d getCenter() const { return m_center; }
    gp_Pnt2d getPoint1() const { return m_point1; }
    gp_Pnt2d getPoint2() const { return m_point2; }
    void setCenter(const gp_Pnt2d& c) { m_center = c; }
    void setPoint1(const gp_Pnt2d& p) { m_point1 = p; }
    void setPoint2(const gp_Pnt2d& p) { m_point2 = p; }
    
    double getValue() const override;   // Angle intérieur en degrés (0-180)
    void setValue(double value) override; // Modifie géométrie source pour imposer l'angle
    
    void updateText();  // Recalcule texte + position depuis m_center/m_point1/m_point2
    
    // Rayon de l'arc de cotation
    double getRadius() const { return m_radius; }
    void setRadius(double radius) { m_radius = radius; }
    
    // Angles pour le rendu (en radians)
    double getAngle1() const;  // atan2(point1 - center)
    double getAngle2() const;  // atan2(point2 - center)
    double getStartAngle() const;  // Début de l'arc (le plus petit angle)
    double getSweep() const;       // Amplitude de l'arc (toujours positif, ≤π)
    
    // Rafraîchir depuis les entités sources (après drag d'entité)
    void refreshFromSources();
    
    // Stocker les entités sources (pour modification et suivi de drag)
    void setSourceEntity1(std::shared_ptr<SketchEntity> entity, int segIdx = -1) {
        m_sourceEntity1 = entity; m_sourceSegIdx1 = segIdx;
    }
    void setSourceEntity2(std::shared_ptr<SketchEntity> entity, int segIdx = -1) {
        m_sourceEntity2 = entity; m_sourceSegIdx2 = segIdx;
    }
    
    std::shared_ptr<SketchEntity> getSourceEntity1() const { return m_sourceEntity1; }
    std::shared_ptr<SketchEntity> getSourceEntity2() const { return m_sourceEntity2; }
    int getSourceSegIdx1() const { return m_sourceSegIdx1; }
    int getSourceSegIdx2() const { return m_sourceSegIdx2; }
    
private:
    gp_Pnt2d m_center;   // Vertex commun
    gp_Pnt2d m_point1;   // Extrémité segment 1 (loin du vertex)
    gp_Pnt2d m_point2;   // Extrémité segment 2 (loin du vertex)
    double m_radius = 30.0;
    
    // Entités sources pour modification géométrie et suivi drag
    std::shared_ptr<SketchEntity> m_sourceEntity1;
    std::shared_ptr<SketchEntity> m_sourceEntity2;
    int m_sourceSegIdx1 = -1;  // -1=SketchLine, >=0=segment polyline
    int m_sourceSegIdx2 = -1;
    
    // Helpers : extraire les 2 endpoints d'un segment source
    // Retourne (vertex, farPoint) - vertex est le point proche de m_center
    std::pair<gp_Pnt2d, gp_Pnt2d> getSegEndpoints(
        std::shared_ptr<SketchEntity> entity, int segIdx) const;
};

// ============================================================================
// RadialDimension - Cotation de rayon
// ============================================================================

class RadialDimension : public Dimension {
public:
    RadialDimension(const gp_Pnt2d& center, double radius);
    
    gp_Pnt2d getCenter() const { return m_center; }
    double getRadius() const { return m_radius; }
    
    double getValue() const override { return m_radius; }
    void setValue(double value) override { m_radius = value; }
    
    // Point sur le cercle où la flèche pointe
    gp_Pnt2d getArrowPoint() const { return m_arrowPoint; }
    void setArrowPoint(const gp_Pnt2d& pt) { m_arrowPoint = pt; }
    
private:
    gp_Pnt2d m_center;
    double m_radius;
    gp_Pnt2d m_arrowPoint;
};

// ============================================================================
// DiametralDimension - Cotation de diamètre
// ============================================================================

class DiametralDimension : public Dimension {
public:
    DiametralDimension(const gp_Pnt2d& center, double diameter);
    
    gp_Pnt2d getCenter() const { return m_center; }
    double getDiameter() const { return m_diameter; }
    
    void setCenter(const gp_Pnt2d& center) { m_center = center; updateText(); }
    void updateText();
    
    double getValue() const override { return m_diameter; }
    void setValue(double value) override { m_diameter = value; updateText(); }
    
    // Angle de la ligne de cote
    double getAngle() const { return m_angle; }
    void setAngle(double angle) { m_angle = angle; updateText(); }
    
private:
    gp_Pnt2d m_center;
    double m_diameter;
    double m_angle = 45.0;  // Angle par défaut
};

} // namespace CADEngine

#endif // DIMENSION_H

#ifndef SKETCHENTITY_H
#define SKETCHENTITY_H

#include <gp_Pnt2d.hxx>
#include <gp_Pln.hxx>
#include <TopoDS_Edge.hxx>
#include <vector>
#include <memory>

namespace CADEngine {

/**
 * @brief Type d'entité de sketch
 */
enum class SketchEntityType {
    Line,
    Circle,
    Arc,
    Rectangle,
    Point,
    Polyline,  // Suite de segments connectés
    Ellipse,   // Ellipse (centre + demi-axes)
    Polygon,   // Polygone régulier (centre + rayon + nb côtés)
    Oblong     // Rainure/clavette à bouts arrondis (centre + longueur + largeur)
};

/**
 * @brief Classe de base pour toutes les entités 2D d'un sketch
 * 
 * Une entité sketch vit dans un espace 2D (coordonnées locales du plan)
 * mais peut être convertie en géométrie 3D (TopoDS_Edge) pour le rendu et l'extrusion
 */
class SketchEntity {
public:
    SketchEntity(SketchEntityType type);
    virtual ~SketchEntity() = default;
    
    // Type
    SketchEntityType getType() const { return m_type; }
    
    // Identification unique
    int getId() const { return m_id; }
    void setId(int id) { m_id = id; }
    
    // Points clés (pour le snap)
    virtual std::vector<gp_Pnt2d> getKeyPoints() const = 0;
    
    // Conversion 2D → 3D
    virtual TopoDS_Edge toEdge3D(const gp_Pln& plane) const = 0;
    
    // Multi-edge entities: retourne TOUTES les edges (défaut = 1 seule via toEdge3D)
    virtual std::vector<TopoDS_Edge> toEdges3D(const gp_Pln& plane) const {
        TopoDS_Edge e = toEdge3D(plane);
        if (!e.IsNull()) return { e };
        return {};
    }
    
    // Sélection/highlight
    bool isSelected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; }
    
    // Construction geometry (axes, lignes de construction - non exportées)
    bool isConstruction() const { return m_construction; }
    void setConstruction(bool c) { m_construction = c; }
    
    // Validité
    virtual bool isValid() const = 0;

protected:
    SketchEntityType m_type;
    bool m_selected;
    bool m_construction = false;
    int m_id = -1;
    
    static int s_nextId;
};

/**
 * @brief Ligne 2D
 */
class SketchLine : public SketchEntity {
public:
    SketchLine(const gp_Pnt2d& start, const gp_Pnt2d& end);
    
    gp_Pnt2d getStart() const { return m_start; }
    gp_Pnt2d getEnd() const { return m_end; }
    
    void setStart(const gp_Pnt2d& pt) { m_start = pt; }
    void setEnd(const gp_Pnt2d& pt) { m_end = pt; }
    
    // Implémentation SketchEntity
    std::vector<gp_Pnt2d> getKeyPoints() const override;
    TopoDS_Edge toEdge3D(const gp_Pln& plane) const override;
    bool isValid() const override;

private:
    gp_Pnt2d m_start;
    gp_Pnt2d m_end;
};

/**
 * @brief Cercle 2D
 */
class SketchCircle : public SketchEntity {
public:
    SketchCircle(const gp_Pnt2d& center, double radius);
    
    gp_Pnt2d getCenter() const { return m_center; }
    double getRadius() const { return m_radius; }
    
    void setCenter(const gp_Pnt2d& pt) { m_center = pt; }
    void setRadius(double r) { m_radius = r; }
    
    // Implémentation SketchEntity
    std::vector<gp_Pnt2d> getKeyPoints() const override;
    TopoDS_Edge toEdge3D(const gp_Pln& plane) const override;
    bool isValid() const override;

private:
    gp_Pnt2d m_center;
    double m_radius;
};

/**
 * @brief Arc de cercle 2D (3 points : début, fin, intermédiaire)
 */
class SketchArc : public SketchEntity {
public:
    SketchArc(const gp_Pnt2d& start, const gp_Pnt2d& end, const gp_Pnt2d& mid, bool bezier = false);
    
    gp_Pnt2d getStart() const { return m_start; }
    gp_Pnt2d getEnd() const { return m_end; }
    gp_Pnt2d getMid() const { return m_mid; }
    bool isBezier() const { return m_isBezier; }
    
    // Implémentation SketchEntity
    std::vector<gp_Pnt2d> getKeyPoints() const override;
    TopoDS_Edge toEdge3D(const gp_Pln& plane) const override;
    bool isValid() const override;

private:
    gp_Pnt2d m_start;
    gp_Pnt2d m_end;
    gp_Pnt2d m_mid;
    bool m_isBezier;  // true = Bézier quadratique (Arc 3pt), false = arc circulaire (ArcCenter)
};

/**
 * @brief Rectangle 2D (coin + largeur/hauteur)
 */
class SketchRectangle : public SketchEntity {
public:
    SketchRectangle(const gp_Pnt2d& corner, double width, double height);
    
    gp_Pnt2d getCorner() const { return m_corner; }
    double getWidth() const { return m_width; }
    double getHeight() const { return m_height; }
    
    void setCorner(const gp_Pnt2d& corner) { m_corner = corner; }
    void setWidth(double width) { m_width = width; }
    void setHeight(double height) { m_height = height; }
    
    // Implémentation SketchEntity
    std::vector<gp_Pnt2d> getKeyPoints() const override;
    TopoDS_Edge toEdge3D(const gp_Pln& plane) const override;
    std::vector<TopoDS_Edge> toEdges3D(const gp_Pln& plane) const override;
    bool isValid() const override;

private:
    gp_Pnt2d m_corner;
    double m_width;
    double m_height;
};

/**
 * @brief Polyligne 2D (suite de points connectés)
 */
class SketchPolyline : public SketchEntity {
public:
    // Arc data for a fillet vertex: the range [startIdx, endIdx] in m_points
    // is replaced by a single proper circular arc edge in toEdges3D.
    struct FilletArc {
        int    startIdx;   // m_points index of T1 (first tangent point)
        int    endIdx;     // m_points index of T2 (last tangent point)
        double cx, cy;     // arc centre (2D sketch coordinates)
        double radius;
        double startAngle, angleDiff; // arc start and signed span (rad)
    };

    SketchPolyline();
    SketchPolyline(const std::vector<gp_Pnt2d>& points);

    void addPoint(const gp_Pnt2d& pt);
    std::vector<gp_Pnt2d> getPoints() const { return m_points; }
    void setPoints(const std::vector<gp_Pnt2d>& points) { m_points = points; }
    int getPointCount() const { return static_cast<int>(m_points.size()); }

    // Fillet arc management
    void addFilletArc(const FilletArc& arc) { m_filletArcs.push_back(arc); }
    // Must be called BEFORE setPoints() when inserting arc points at insertPos,
    // so that existing arc indices are shifted correctly.
    void shiftFilletArcs(int insertPos, int insertedCount) {
        for (auto& fa : m_filletArcs) {
            if (fa.startIdx >= insertPos) fa.startIdx += insertedCount;
            if (fa.endIdx   >= insertPos) fa.endIdx   += insertedCount;
        }
    }
    const std::vector<FilletArc>& getFilletArcs() const { return m_filletArcs; }

    // Implémentation SketchEntity
    std::vector<gp_Pnt2d> getKeyPoints() const override;
    TopoDS_Edge toEdge3D(const gp_Pln& plane) const override;
    std::vector<TopoDS_Edge> toEdges3D(const gp_Pln& plane) const override;
    bool isValid() const override;

private:
    std::vector<gp_Pnt2d> m_points;
    std::vector<FilletArc> m_filletArcs;
};

/**
 * @brief Ellipse 2D (centre + demi-grand axe + demi-petit axe + rotation)
 */
class SketchEllipse : public SketchEntity {
public:
    SketchEllipse(const gp_Pnt2d& center, double majorRadius, double minorRadius, double rotation = 0.0);
    
    gp_Pnt2d getCenter() const { return m_center; }
    double getMajorRadius() const { return m_majorRadius; }
    double getMinorRadius() const { return m_minorRadius; }
    double getRotation() const { return m_rotation; }
    
    void setCenter(const gp_Pnt2d& c) { m_center = c; }
    void setMajorRadius(double r) { m_majorRadius = r; }
    void setMinorRadius(double r) { m_minorRadius = r; }
    void setRotation(double r) { m_rotation = r; }
    
    std::vector<gp_Pnt2d> getKeyPoints() const override;
    TopoDS_Edge toEdge3D(const gp_Pln& plane) const override;
    bool isValid() const override;

private:
    gp_Pnt2d m_center;
    double m_majorRadius;
    double m_minorRadius;
    double m_rotation;  // radians
};

/**
 * @brief Polygone régulier 2D (centre + rayon + nombre de côtés + rotation)
 */
class SketchPolygon : public SketchEntity {
public:
    SketchPolygon(const gp_Pnt2d& center, double radius, int sides, double rotation = 0.0);
    
    gp_Pnt2d getCenter() const { return m_center; }
    double getRadius() const { return m_radius; }
    int getSides() const { return m_sides; }
    double getRotation() const { return m_rotation; }
    
    void setCenter(const gp_Pnt2d& c) { m_center = c; }
    void setRadius(double r) { m_radius = r; }
    void setSides(int s) { m_sides = s; }
    void setRotation(double r) { m_rotation = r; }
    
    // Retourne les vertices du polygone
    std::vector<gp_Pnt2d> getVertices() const;
    
    std::vector<gp_Pnt2d> getKeyPoints() const override;
    TopoDS_Edge toEdge3D(const gp_Pln& plane) const override;
    std::vector<TopoDS_Edge> toEdges3D(const gp_Pln& plane) const override;
    bool isValid() const override;

private:
    gp_Pnt2d m_center;
    double m_radius;
    int m_sides;
    double m_rotation;  // radians
};

/**
 * @brief Oblong 2D (rainure/clavette à bouts arrondis)
 * Forme: 2 droites parallèles + 2 demi-cercles aux extrémités
 * Défini par: centre + longueur totale + largeur (= diamètre arrondi) + rotation
 */
class SketchOblong : public SketchEntity {
public:
    SketchOblong(const gp_Pnt2d& center, double length, double width, double rotation = 0.0);
    
    gp_Pnt2d getCenter() const { return m_center; }
    double getLength() const { return m_length; }
    double getWidth() const { return m_width; }
    double getRotation() const { return m_rotation; }
    
    void setCenter(const gp_Pnt2d& c) { m_center = c; }
    void setLength(double l) { m_length = l; }
    void setWidth(double w) { m_width = w; }
    void setRotation(double r) { m_rotation = r; }
    
    // Retourne les 2 centres des demi-cercles
    gp_Pnt2d getCenter1() const;
    gp_Pnt2d getCenter2() const;
    double getRadius() const { return m_width / 2.0; }
    
    std::vector<gp_Pnt2d> getKeyPoints() const override;
    TopoDS_Edge toEdge3D(const gp_Pln& plane) const override;
    std::vector<TopoDS_Edge> toEdges3D(const gp_Pln& plane) const override;
    bool isValid() const override;

private:
    gp_Pnt2d m_center;
    double m_length;   // Longueur totale (bord à bord)
    double m_width;    // Largeur (= diamètre des arrondis)
    double m_rotation; // radians
};

} // namespace CADEngine

#endif // SKETCHENTITY_H

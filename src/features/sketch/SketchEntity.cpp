#include "SketchEntity.h"
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GC_MakeCircle.hxx>
#include <GC_MakeSegment.hxx>
#include <gce_MakeCirc.hxx>
#include <Precision.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Ellipse.hxx>
#include <gp_Elips.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <cmath>

namespace CADEngine {

// ============================================================================
// SketchEntity - Base
// ============================================================================

int SketchEntity::s_nextId = 1;

SketchEntity::SketchEntity(SketchEntityType type)
    : m_type(type)
    , m_selected(false)
    , m_id(s_nextId++)
{
}

// ============================================================================
// SketchLine
// ============================================================================

SketchLine::SketchLine(const gp_Pnt2d& start, const gp_Pnt2d& end)
    : SketchEntity(SketchEntityType::Line)
    , m_start(start)
    , m_end(end)
{
}

std::vector<gp_Pnt2d> SketchLine::getKeyPoints() const {
    return { m_start, m_end };
}

TopoDS_Edge SketchLine::toEdge3D(const gp_Pln& plane) const {
    // Helper pour convertir 2D → 3D
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec.Multiplied(pt2D.X()) + yVec.Multiplied(pt2D.Y()));
    };
    
    // Convertir points 2D → 3D dans le plan
    gp_Pnt start3D = toGlobal(m_start);
    gp_Pnt end3D = toGlobal(m_end);
    
    // Créer segment 3D
    GC_MakeSegment segmentMaker(start3D, end3D);
    if (!segmentMaker.IsDone()) {
        return TopoDS_Edge();
    }
    
    BRepBuilderAPI_MakeEdge edgeMaker(segmentMaker.Value());
    return edgeMaker.Edge();
}

bool SketchLine::isValid() const {
    return m_start.Distance(m_end) > Precision::Confusion();
}

// ============================================================================
// SketchCircle
// ============================================================================

SketchCircle::SketchCircle(const gp_Pnt2d& center, double radius)
    : SketchEntity(SketchEntityType::Circle)
    , m_center(center)
    , m_radius(radius)
{
}

std::vector<gp_Pnt2d> SketchCircle::getKeyPoints() const {
    // Centre + 4 points cardinaux
    return {
        m_center,
        gp_Pnt2d(m_center.X() + m_radius, m_center.Y()),      // Est
        gp_Pnt2d(m_center.X(), m_center.Y() + m_radius),      // Nord
        gp_Pnt2d(m_center.X() - m_radius, m_center.Y()),      // Ouest
        gp_Pnt2d(m_center.X(), m_center.Y() - m_radius)       // Sud
    };
}

TopoDS_Edge SketchCircle::toEdge3D(const gp_Pln& plane) const {
    // Helper pour convertir 2D → 3D
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec.Multiplied(pt2D.X()) + yVec.Multiplied(pt2D.Y()));
    };
    
    // Centre 3D
    gp_Pnt center3D = toGlobal(m_center);
    
    // Créer cercle 3D dans le plan
    gp_Ax2 ax2(center3D, plane.Axis().Direction());
    gp_Circ circle(ax2, m_radius);
    
    BRepBuilderAPI_MakeEdge edgeMaker(circle);
    return edgeMaker.Edge();
}

bool SketchCircle::isValid() const {
    return m_radius > Precision::Confusion();
}

// ============================================================================
// SketchArc
// ============================================================================

SketchArc::SketchArc(const gp_Pnt2d& start, const gp_Pnt2d& end, const gp_Pnt2d& mid, bool bezier)
    : SketchEntity(SketchEntityType::Arc)
    , m_start(start)
    , m_end(end)
    , m_mid(mid)
    , m_isBezier(bezier)
{
}

std::vector<gp_Pnt2d> SketchArc::getKeyPoints() const {
    return { m_start, m_end, m_mid };
}

TopoDS_Edge SketchArc::toEdge3D(const gp_Pln& plane) const {
    // Helper pour convertir 2D → 3D
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec.Multiplied(pt2D.X()) + yVec.Multiplied(pt2D.Y()));
    };
    
    gp_Pnt start3D = toGlobal(m_start);
    gp_Pnt end3D = toGlobal(m_end);
    gp_Pnt mid3D = toGlobal(m_mid);
    
    if (m_isBezier) {
        // Bézier quadratique : mid est un point de contrôle (pas sur la courbe)
        TColgp_Array1OfPnt poles(1, 3);
        poles(1) = start3D;
        poles(2) = mid3D;
        poles(3) = end3D;
        
        TColStd_Array1OfReal weights(1, 3);
        weights(1) = 1.0;
        weights(2) = 1.0;
        weights(3) = 1.0;
        
        TColStd_Array1OfInteger mults(1, 2);
        mults(1) = 3;
        mults(2) = 3;
        
        TColStd_Array1OfReal knots(1, 2);
        knots(1) = 0.0;
        knots(2) = 1.0;
        
        Handle(Geom_BSplineCurve) bezier = new Geom_BSplineCurve(
            poles, weights, knots, mults, 2);
        
        BRepBuilderAPI_MakeEdge edgeMaker(bezier);
        if (edgeMaker.IsDone()) return edgeMaker.Edge();
    } else {
        // Arc circulaire passant par 3 points (start, mid, end)
        GC_MakeArcOfCircle arcMaker(start3D, mid3D, end3D);
        if (arcMaker.IsDone()) {
            BRepBuilderAPI_MakeEdge edgeMaker(arcMaker.Value());
            if (edgeMaker.IsDone()) return edgeMaker.Edge();
        }
    }
    
    // Fallback: segment droit
    GC_MakeSegment segMaker(start3D, end3D);
    if (segMaker.IsDone()) {
        BRepBuilderAPI_MakeEdge edgeMaker(segMaker.Value());
        return edgeMaker.Edge();
    }
    return TopoDS_Edge();
}

bool SketchArc::isValid() const {
    // Vérifier que les 3 points ne sont pas alignés
    double x1 = m_start.X(), y1 = m_start.Y();
    double x2 = m_mid.X(), y2 = m_mid.Y();
    double x3 = m_end.X(), y3 = m_end.Y();
    
    double det = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);
    return std::abs(det) > Precision::Confusion();
}

// ============================================================================
// SketchRectangle
// ============================================================================

SketchRectangle::SketchRectangle(const gp_Pnt2d& corner, double width, double height)
    : SketchEntity(SketchEntityType::Rectangle)
    , m_corner(corner)
    , m_width(width)
    , m_height(height)
{
}

std::vector<gp_Pnt2d> SketchRectangle::getKeyPoints() const {
    // 4 coins
    return {
        m_corner,
        gp_Pnt2d(m_corner.X() + m_width, m_corner.Y()),
        gp_Pnt2d(m_corner.X() + m_width, m_corner.Y() + m_height),
        gp_Pnt2d(m_corner.X(), m_corner.Y() + m_height)
    };
}

TopoDS_Edge SketchRectangle::toEdge3D(const gp_Pln& plane) const {
    // Helper pour convertir 2D → 3D
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec.Multiplied(pt2D.X()) + yVec.Multiplied(pt2D.Y()));
    };
    
    // Rectangle = 4 lignes → wire fermé
    auto corners = getKeyPoints();
    
    BRepBuilderAPI_MakeWire wireMaker;
    
    for (size_t i = 0; i < corners.size(); ++i) {
        gp_Pnt2d start2D = corners[i];
        gp_Pnt2d end2D = corners[(i + 1) % corners.size()];
        
        gp_Pnt start3D = toGlobal(start2D);
        gp_Pnt end3D = toGlobal(end2D);
        
        GC_MakeSegment segMaker(start3D, end3D);
        if (segMaker.IsDone()) {
            BRepBuilderAPI_MakeEdge edgeMaker(segMaker.Value());
            wireMaker.Add(edgeMaker.Edge());
        }
    }
    
    if (!wireMaker.IsDone()) {
        return TopoDS_Edge();
    }
    
    // Retourner la première edge du wire (compat)
    return wireMaker.Edge();
}

std::vector<TopoDS_Edge> SketchRectangle::toEdges3D(const gp_Pln& plane) const {
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec.Multiplied(pt2D.X()) + yVec.Multiplied(pt2D.Y()));
    };

    auto corners = getKeyPoints();
    std::vector<TopoDS_Edge> edges;

    for (size_t i = 0; i < corners.size(); ++i) {
        gp_Pnt start3D = toGlobal(corners[i]);
        gp_Pnt end3D   = toGlobal(corners[(i + 1) % corners.size()]);
        if (start3D.Distance(end3D) < 1e-6) continue;
        GC_MakeSegment segMaker(start3D, end3D);
        if (segMaker.IsDone()) {
            BRepBuilderAPI_MakeEdge em(segMaker.Value());
            if (em.IsDone()) edges.push_back(em.Edge());
        }
    }
    return edges;
}

bool SketchRectangle::isValid() const {
    return m_width > Precision::Confusion() && 
           m_height > Precision::Confusion();
}

// ============================================================================
// SketchPolyline
// ============================================================================

SketchPolyline::SketchPolyline()
    : SketchEntity(SketchEntityType::Polyline) {
}

SketchPolyline::SketchPolyline(const std::vector<gp_Pnt2d>& points)
    : SketchEntity(SketchEntityType::Polyline)
    , m_points(points) {
}

void SketchPolyline::addPoint(const gp_Pnt2d& pt) {
    m_points.push_back(pt);
}

std::vector<gp_Pnt2d> SketchPolyline::getKeyPoints() const {
    return m_points;
}

TopoDS_Edge SketchPolyline::toEdge3D(const gp_Pln& plane) const {
    // Pour compatibilité : retourne le premier segment
    if (m_points.size() < 2) return TopoDS_Edge();
    
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec * pt2D.X() + yVec * pt2D.Y());
    };
    
    gp_Pnt p1 = toGlobal(m_points[0]);
    gp_Pnt p2 = toGlobal(m_points[1]);
    
    return BRepBuilderAPI_MakeEdge(p1, p2).Edge();
}

std::vector<TopoDS_Edge> SketchPolyline::toEdges3D(const gp_Pln& plane) const {
    if (m_points.size() < 2) return {};
    
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec * pt2D.X() + yVec * pt2D.Y());
    };
    
    std::vector<TopoDS_Edge> edges;
    for (size_t i = 0; i + 1 < m_points.size(); ++i) {
        gp_Pnt p1 = toGlobal(m_points[i]);
        gp_Pnt p2 = toGlobal(m_points[i + 1]);
        
        if (p1.Distance(p2) < 1e-6) continue;
        
        try {
            BRepBuilderAPI_MakeEdge edgeMaker(p1, p2);
            if (edgeMaker.IsDone()) edges.push_back(edgeMaker.Edge());
        } catch (...) {}
    }
    return edges;
}

bool SketchPolyline::isValid() const {
    return m_points.size() >= 2;
}

// ============================================================================
// SketchEllipse
// ============================================================================

SketchEllipse::SketchEllipse(const gp_Pnt2d& center, double majorRadius, double minorRadius, double rotation)
    : SketchEntity(SketchEntityType::Ellipse)
    , m_center(center)
    , m_majorRadius(majorRadius)
    , m_minorRadius(minorRadius)
    , m_rotation(rotation)
{
}

std::vector<gp_Pnt2d> SketchEllipse::getKeyPoints() const {
    double cosR = std::cos(m_rotation);
    double sinR = std::sin(m_rotation);
    return {
        m_center,
        // Extrémités grand axe
        gp_Pnt2d(m_center.X() + m_majorRadius * cosR, m_center.Y() + m_majorRadius * sinR),
        gp_Pnt2d(m_center.X() - m_majorRadius * cosR, m_center.Y() - m_majorRadius * sinR),
        // Extrémités petit axe
        gp_Pnt2d(m_center.X() - m_minorRadius * sinR, m_center.Y() + m_minorRadius * cosR),
        gp_Pnt2d(m_center.X() + m_minorRadius * sinR, m_center.Y() - m_minorRadius * cosR)
    };
}

TopoDS_Edge SketchEllipse::toEdge3D(const gp_Pln& plane) const {
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec.Multiplied(pt2D.X()) + yVec.Multiplied(pt2D.Y()));
    };
    
    gp_Pnt center3D = toGlobal(m_center);
    gp_Dir normal = plane.Axis().Direction();
    
    // Direction du grand axe dans l'espace 3D
    gp_Dir xDir3D = plane.Position().XDirection();
    gp_Dir yDir3D = plane.Position().YDirection();
    double cosR = std::cos(m_rotation);
    double sinR = std::sin(m_rotation);
    gp_Dir majorDir(
        xDir3D.X() * cosR + yDir3D.X() * sinR,
        xDir3D.Y() * cosR + yDir3D.Y() * sinR,
        xDir3D.Z() * cosR + yDir3D.Z() * sinR
    );
    
    gp_Ax2 ax2(center3D, normal, majorDir);
    gp_Elips ellipse(ax2, m_majorRadius, m_minorRadius);
    
    BRepBuilderAPI_MakeEdge edgeMaker(ellipse);
    if (edgeMaker.IsDone()) return edgeMaker.Edge();
    return TopoDS_Edge();
}

bool SketchEllipse::isValid() const {
    return m_majorRadius > Precision::Confusion() && 
           m_minorRadius > Precision::Confusion() &&
           m_majorRadius >= m_minorRadius;
}

// ============================================================================
// SketchPolygon
// ============================================================================

SketchPolygon::SketchPolygon(const gp_Pnt2d& center, double radius, int sides, double rotation)
    : SketchEntity(SketchEntityType::Polygon)
    , m_center(center)
    , m_radius(radius)
    , m_sides(sides)
    , m_rotation(rotation)
{
}

std::vector<gp_Pnt2d> SketchPolygon::getVertices() const {
    std::vector<gp_Pnt2d> verts;
    verts.reserve(m_sides);
    for (int i = 0; i < m_sides; i++) {
        double angle = m_rotation + 2.0 * M_PI * i / m_sides;
        verts.emplace_back(
            m_center.X() + m_radius * std::cos(angle),
            m_center.Y() + m_radius * std::sin(angle)
        );
    }
    return verts;
}

std::vector<gp_Pnt2d> SketchPolygon::getKeyPoints() const {
    auto verts = getVertices();
    // Ajouter le centre en premier
    verts.insert(verts.begin(), m_center);
    return verts;
}

TopoDS_Edge SketchPolygon::toEdge3D(const gp_Pln& plane) const {
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec.Multiplied(pt2D.X()) + yVec.Multiplied(pt2D.Y()));
    };
    
    auto verts = getVertices();
    if (verts.size() < 3) return TopoDS_Edge();
    
    BRepBuilderAPI_MakeWire wireMaker;
    for (size_t i = 0; i < verts.size(); ++i) {
        gp_Pnt p1 = toGlobal(verts[i]);
        gp_Pnt p2 = toGlobal(verts[(i + 1) % verts.size()]);
        
        GC_MakeSegment segMaker(p1, p2);
        if (segMaker.IsDone()) {
            BRepBuilderAPI_MakeEdge edgeMaker(segMaker.Value());
            wireMaker.Add(edgeMaker.Edge());
        }
    }
    
    if (!wireMaker.IsDone()) return TopoDS_Edge();
    return wireMaker.Edge();
}

std::vector<TopoDS_Edge> SketchPolygon::toEdges3D(const gp_Pln& plane) const {
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec.Multiplied(pt2D.X()) + yVec.Multiplied(pt2D.Y()));
    };

    auto verts = getVertices();
    if (verts.size() < 3) return {};

    std::vector<TopoDS_Edge> edges;
    for (size_t i = 0; i < verts.size(); ++i) {
        gp_Pnt p1 = toGlobal(verts[i]);
        gp_Pnt p2 = toGlobal(verts[(i + 1) % verts.size()]);
        if (p1.Distance(p2) < 1e-6) continue;
        GC_MakeSegment segMaker(p1, p2);
        if (segMaker.IsDone()) {
            BRepBuilderAPI_MakeEdge em(segMaker.Value());
            if (em.IsDone()) edges.push_back(em.Edge());
        }
    }
    return edges;
}

bool SketchPolygon::isValid() const {
    return m_radius > Precision::Confusion() && m_sides >= 3;
}

// ============================================================================
// SketchOblong
// ============================================================================

SketchOblong::SketchOblong(const gp_Pnt2d& center, double length, double width, double rotation)
    : SketchEntity(SketchEntityType::Oblong)
    , m_center(center)
    , m_length(length)
    , m_width(width)
    , m_rotation(rotation)
{
}

gp_Pnt2d SketchOblong::getCenter1() const {
    double halfStraight = (m_length - m_width) / 2.0;
    double cosR = std::cos(m_rotation), sinR = std::sin(m_rotation);
    return gp_Pnt2d(m_center.X() - halfStraight * cosR,
                     m_center.Y() - halfStraight * sinR);
}

gp_Pnt2d SketchOblong::getCenter2() const {
    double halfStraight = (m_length - m_width) / 2.0;
    double cosR = std::cos(m_rotation), sinR = std::sin(m_rotation);
    return gp_Pnt2d(m_center.X() + halfStraight * cosR,
                     m_center.Y() + halfStraight * sinR);
}

std::vector<gp_Pnt2d> SketchOblong::getKeyPoints() const {
    double cosR = std::cos(m_rotation), sinR = std::sin(m_rotation);
    double halfL = m_length / 2.0;
    double halfW = m_width / 2.0;
    
    return {
        m_center,
        // Extrémités sur l'axe long
        gp_Pnt2d(m_center.X() + halfL * cosR, m_center.Y() + halfL * sinR),
        gp_Pnt2d(m_center.X() - halfL * cosR, m_center.Y() - halfL * sinR),
        // Centres des demi-cercles
        getCenter1(), getCenter2(),
        // Points haut/bas des droites
        gp_Pnt2d(m_center.X() - halfW * sinR, m_center.Y() + halfW * cosR),
        gp_Pnt2d(m_center.X() + halfW * sinR, m_center.Y() - halfW * cosR)
    };
}

TopoDS_Edge SketchOblong::toEdge3D(const gp_Pln& plane) const {
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec.Multiplied(pt2D.X()) + yVec.Multiplied(pt2D.Y()));
    };
    
    double cosR = std::cos(m_rotation), sinR = std::sin(m_rotation);
    double halfStraight = (m_length - m_width) / 2.0;
    double r = m_width / 2.0;
    
    // Direction axe long et perpendiculaire
    double axX = cosR, axY = sinR;
    double prX = -sinR, prY = cosR;
    
    // Les 4 coins de la partie droite
    gp_Pnt2d p1(m_center.X() - halfStraight*axX + r*prX, m_center.Y() - halfStraight*axY + r*prY);
    gp_Pnt2d p2(m_center.X() + halfStraight*axX + r*prX, m_center.Y() + halfStraight*axY + r*prY);
    gp_Pnt2d p3(m_center.X() + halfStraight*axX - r*prX, m_center.Y() + halfStraight*axY - r*prY);
    gp_Pnt2d p4(m_center.X() - halfStraight*axX - r*prX, m_center.Y() - halfStraight*axY - r*prY);
    
    BRepBuilderAPI_MakeWire wireMaker;
    
    // Ligne haut (p1→p2)
    GC_MakeSegment seg1(toGlobal(p1), toGlobal(p2));
    if (seg1.IsDone()) wireMaker.Add(BRepBuilderAPI_MakeEdge(seg1.Value()).Edge());
    
    // Demi-cercle droit (p2→p3 autour de center2)
    gp_Pnt2d c2 = getCenter2();
    gp_Pnt2d midR(c2.X() + r*axX, c2.Y() + r*axY);
    GC_MakeArcOfCircle arc1(toGlobal(p2), toGlobal(midR), toGlobal(p3));
    if (arc1.IsDone()) wireMaker.Add(BRepBuilderAPI_MakeEdge(arc1.Value()).Edge());
    
    // Ligne bas (p3→p4)
    GC_MakeSegment seg2(toGlobal(p3), toGlobal(p4));
    if (seg2.IsDone()) wireMaker.Add(BRepBuilderAPI_MakeEdge(seg2.Value()).Edge());
    
    // Demi-cercle gauche (p4→p1 autour de center1)
    gp_Pnt2d c1 = getCenter1();
    gp_Pnt2d midL(c1.X() - r*axX, c1.Y() - r*axY);
    GC_MakeArcOfCircle arc2(toGlobal(p4), toGlobal(midL), toGlobal(p1));
    if (arc2.IsDone()) wireMaker.Add(BRepBuilderAPI_MakeEdge(arc2.Value()).Edge());
    
    if (!wireMaker.IsDone()) return TopoDS_Edge();
    return wireMaker.Edge();
}

std::vector<TopoDS_Edge> SketchOblong::toEdges3D(const gp_Pln& plane) const {
    auto toGlobal = [&plane](const gp_Pnt2d& pt2D) -> gp_Pnt {
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        return origin.Translated(xVec.Multiplied(pt2D.X()) + yVec.Multiplied(pt2D.Y()));
    };
    
    double cosR = std::cos(m_rotation), sinR = std::sin(m_rotation);
    double halfStraight = (m_length - m_width) / 2.0;
    double r = m_width / 2.0;
    
    double axX = cosR, axY = sinR;
    double prX = -sinR, prY = cosR;
    
    gp_Pnt2d p1(m_center.X() - halfStraight*axX + r*prX, m_center.Y() - halfStraight*axY + r*prY);
    gp_Pnt2d p2(m_center.X() + halfStraight*axX + r*prX, m_center.Y() + halfStraight*axY + r*prY);
    gp_Pnt2d p3(m_center.X() + halfStraight*axX - r*prX, m_center.Y() + halfStraight*axY - r*prY);
    gp_Pnt2d p4(m_center.X() - halfStraight*axX - r*prX, m_center.Y() - halfStraight*axY - r*prY);
    
    std::vector<TopoDS_Edge> edges;
    
    // Line top (p1→p2)
    GC_MakeSegment seg1(toGlobal(p1), toGlobal(p2));
    if (seg1.IsDone()) {
        BRepBuilderAPI_MakeEdge e(seg1.Value());
        if (e.IsDone()) edges.push_back(e.Edge());
    }
    
    // Arc right (p2→p3)
    gp_Pnt2d c2 = getCenter2();
    gp_Pnt2d midR(c2.X() + r*axX, c2.Y() + r*axY);
    GC_MakeArcOfCircle arc1(toGlobal(p2), toGlobal(midR), toGlobal(p3));
    if (arc1.IsDone()) {
        BRepBuilderAPI_MakeEdge e(arc1.Value());
        if (e.IsDone()) edges.push_back(e.Edge());
    }
    
    // Line bottom (p3→p4)
    GC_MakeSegment seg2(toGlobal(p3), toGlobal(p4));
    if (seg2.IsDone()) {
        BRepBuilderAPI_MakeEdge e(seg2.Value());
        if (e.IsDone()) edges.push_back(e.Edge());
    }
    
    // Arc left (p4→p1)
    gp_Pnt2d c1 = getCenter1();
    gp_Pnt2d midL(c1.X() - r*axX, c1.Y() - r*axY);
    GC_MakeArcOfCircle arc2(toGlobal(p4), toGlobal(midL), toGlobal(p1));
    if (arc2.IsDone()) {
        BRepBuilderAPI_MakeEdge e(arc2.Value());
        if (e.IsDone()) edges.push_back(e.Edge());
    }
    
    return edges;
}

bool SketchOblong::isValid() const {
    return m_length > Precision::Confusion() && 
           m_width > Precision::Confusion() &&
           m_length >= m_width;
}

} // namespace CADEngine

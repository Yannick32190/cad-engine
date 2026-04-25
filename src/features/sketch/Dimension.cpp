#include "Dimension.h"
#include "SketchEntity.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <QDebug>

namespace CADEngine {

int Dimension::s_nextId = 1;

// ============================================================================
// Dimension - Base
// ============================================================================

Dimension::Dimension(DimensionType type)
    : m_type(type)
    , m_id(s_nextId++)
    , m_textPosition(0, 0)
    , m_visible(true)
{
}

// ============================================================================
// LinearDimension
// ============================================================================

LinearDimension::LinearDimension(const gp_Pnt2d& point1, const gp_Pnt2d& point2)
    : Dimension(DimensionType::Linear)
    , m_point1(point1)
    , m_point2(point2)
{
    // Calculer position du texte par défaut (milieu + offset)
    double mx = (point1.X() + point2.X()) / 2.0;
    double my = (point1.Y() + point2.Y()) / 2.0;
    
    // Vecteur perpendiculaire pour offset
    double dx = point2.X() - point1.X();
    double dy = point2.Y() - point1.Y();
    double len = std::sqrt(dx*dx + dy*dy);
    
    if (len > 1e-6) {
        double perpX = -dy / len;
        double perpY = dx / len;
        m_textPosition = gp_Pnt2d(mx + perpX * m_offset, my + perpY * m_offset);
    } else {
        m_textPosition = gp_Pnt2d(mx, my);
    }
    
    // Générer texte automatiquement
    double distance = getValue();
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << distance;
    m_text = oss.str();
}

double LinearDimension::getValue() const {
    double dx = m_point2.X() - m_point1.X();
    double dy = m_point2.Y() - m_point1.Y();
    return std::sqrt(dx*dx + dy*dy);
}

void LinearDimension::setValue(double value) {
    // Pour contraintes dimensionnelles (Phase 3.4)
    // TODO: Ajuster la géométrie pour respecter cette distance
}

gp_Pnt2d LinearDimension::extractPoint(std::shared_ptr<SketchEntity> entity, int vtxIdx) {
    if (!entity) return gp_Pnt2d(0, 0);
    
    if (entity->getType() == SketchEntityType::Line) {
        auto line = std::dynamic_pointer_cast<SketchLine>(entity);
        if (!line) return gp_Pnt2d(0, 0);
        if (vtxIdx == -1) return line->getStart();
        if (vtxIdx == -2) return line->getEnd();
    }
    else if (entity->getType() == SketchEntityType::Circle) {
        auto circle = std::dynamic_pointer_cast<SketchCircle>(entity);
        if (!circle) return gp_Pnt2d(0, 0);
        if (vtxIdx == -3) return circle->getCenter();
    }
    else if (entity->getType() == SketchEntityType::Arc) {
        auto arc = std::dynamic_pointer_cast<SketchArc>(entity);
        if (!arc) return gp_Pnt2d(0, 0);
        if (vtxIdx == -1) return arc->getStart();
        if (vtxIdx == -2) return arc->getEnd();
        if (vtxIdx == -3) return arc->getMid();  // or center if available
    }
    else if (entity->getType() == SketchEntityType::Rectangle) {
        auto rect = std::dynamic_pointer_cast<SketchRectangle>(entity);
        if (!rect) return gp_Pnt2d(0, 0);
        auto corners = rect->getKeyPoints();
        if (vtxIdx >= 0 && vtxIdx < (int)corners.size()) return corners[vtxIdx];
    }
    else if (entity->getType() == SketchEntityType::Polyline) {
        auto poly = std::dynamic_pointer_cast<SketchPolyline>(entity);
        if (!poly) return gp_Pnt2d(0, 0);
        auto pts = poly->getPoints();
        if (vtxIdx >= 0 && vtxIdx < (int)pts.size()) return pts[vtxIdx];
    }
    else if (entity->getType() == SketchEntityType::Ellipse) {
        auto ellipse = std::dynamic_pointer_cast<SketchEllipse>(entity);
        if (!ellipse) return gp_Pnt2d(0, 0);
        if (vtxIdx == -3) return ellipse->getCenter();
        auto kp = ellipse->getKeyPoints();
        if (vtxIdx >= 0 && vtxIdx < (int)kp.size()) return kp[vtxIdx];
    }
    else if (entity->getType() == SketchEntityType::Polygon) {
        auto polygon = std::dynamic_pointer_cast<SketchPolygon>(entity);
        if (!polygon) return gp_Pnt2d(0, 0);
        if (vtxIdx == -3) return polygon->getCenter();
        auto verts = polygon->getVertices();
        if (vtxIdx >= 0 && vtxIdx < (int)verts.size()) return verts[vtxIdx];
    }
    else if (entity->getType() == SketchEntityType::Oblong) {
        auto oblong = std::dynamic_pointer_cast<SketchOblong>(entity);
        if (!oblong) return gp_Pnt2d(0, 0);
        if (vtxIdx == -3) return oblong->getCenter();
        auto kp = oblong->getKeyPoints();
        if (vtxIdx >= 0 && vtxIdx < (int)kp.size()) return kp[vtxIdx];
    }
    
    return gp_Pnt2d(0, 0);
}

void LinearDimension::refreshFromSources() {
    // Cas spécial: distance perpendiculaire entre 2 lignes (vtxIdx <= -4)
    if (m_sourceVtx1 <= -4 && m_sourceVtx2 <= -4 && m_sourceEntity1 && m_sourceEntity2) {
        // Helper: extraire start/end d'une ligne ou segment polyline
        auto getLinePoints = [](std::shared_ptr<SketchEntity> entity, int vtx, gp_Pnt2d& s, gp_Pnt2d& e) {
            if (vtx == -4 && entity->getType() == SketchEntityType::Line) {
                auto line = std::dynamic_pointer_cast<SketchLine>(entity);
                s = line->getStart(); e = line->getEnd();
            } else if (vtx <= -100 && entity->getType() == SketchEntityType::Polyline) {
                int segIdx = -(vtx + 100);
                auto poly = std::dynamic_pointer_cast<SketchPolyline>(entity);
                auto pts = poly->getPoints();
                if (segIdx + 1 < (int)pts.size()) { s = pts[segIdx]; e = pts[segIdx + 1]; }
            } else if (entity->getType() == SketchEntityType::Line) {
                auto line = std::dynamic_pointer_cast<SketchLine>(entity);
                s = line->getStart(); e = line->getEnd();
            }
        };
        
        gp_Pnt2d a1, a2, b1, b2;
        getLinePoints(m_sourceEntity1, m_sourceVtx1, a1, a2);
        getLinePoints(m_sourceEntity2, m_sourceVtx2, b1, b2);
        
        // Direction et normale de la ligne 1
        double adx = a2.X() - a1.X(), ady = a2.Y() - a1.Y();
        double alen = std::sqrt(adx*adx + ady*ady);
        if (alen > 1e-6) {
            double nx = -ady / alen, ny = adx / alen;
            double aMidX = (a1.X() + a2.X()) / 2.0;
            double aMidY = (a1.Y() + a2.Y()) / 2.0;
            double bMidX = (b1.X() + b2.X()) / 2.0;
            double bMidY = (b1.Y() + b2.Y()) / 2.0;
            
            double perpDist = (bMidX - aMidX) * nx + (bMidY - aMidY) * ny;
            
            m_point1 = gp_Pnt2d(aMidX, aMidY);
            m_point2 = gp_Pnt2d(aMidX + perpDist * nx, aMidY + perpDist * ny);
        }
        updateText();
        return;
    }
    
    // Cas standard: point-à-point
    if (m_sourceEntity1 && m_sourceVtx1 != -99) {
        m_point1 = extractPoint(m_sourceEntity1, m_sourceVtx1);
    }
    if (m_sourceEntity2 && m_sourceVtx2 != -99) {
        m_point2 = extractPoint(m_sourceEntity2, m_sourceVtx2);
    }
    updateText();
}

void LinearDimension::updateText() {
    // Recalculer le texte basé sur la distance actuelle
    double distance = getValue();
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << distance;
    m_text = oss.str();
    
    // Recalculer aussi la position du texte (milieu + offset)
    double mx = (m_point1.X() + m_point2.X()) / 2.0;
    double my = (m_point1.Y() + m_point2.Y()) / 2.0;
    
    // Vecteur perpendiculaire pour offset
    double dx = m_point2.X() - m_point1.X();
    double dy = m_point2.Y() - m_point1.Y();
    double len = std::sqrt(dx*dx + dy*dy);
    
    if (len > 1e-6) {
        double perpX = -dy / len;
        double perpY = dx / len;
        m_textPosition = gp_Pnt2d(mx + perpX * m_offset, my + perpY * m_offset);
    } else {
        m_textPosition = gp_Pnt2d(mx, my);
    }
}

gp_Pnt2d LinearDimension::getDimensionLineStart() const {
    double dx = m_point2.X() - m_point1.X();
    double dy = m_point2.Y() - m_point1.Y();
    double len = std::sqrt(dx*dx + dy*dy);
    
    if (len < 1e-6) return m_point1;
    
    double perpX = -dy / len;
    double perpY = dx / len;
    
    return gp_Pnt2d(m_point1.X() + perpX * m_offset, 
                    m_point1.Y() + perpY * m_offset);
}

gp_Pnt2d LinearDimension::getDimensionLineEnd() const {
    double dx = m_point2.X() - m_point1.X();
    double dy = m_point2.Y() - m_point1.Y();
    double len = std::sqrt(dx*dx + dy*dy);
    
    if (len < 1e-6) return m_point2;
    
    double perpX = -dy / len;
    double perpY = dx / len;
    
    return gp_Pnt2d(m_point2.X() + perpX * m_offset, 
                    m_point2.Y() + perpY * m_offset);
}

gp_Pnt2d LinearDimension::getExtensionLine1End() const {
    return getDimensionLineStart();
}

gp_Pnt2d LinearDimension::getExtensionLine2End() const {
    return getDimensionLineEnd();
}

// ============================================================================
// AngularDimension - RÉÉCRITURE COMPLÈTE
// ============================================================================

// Normalise un angle dans [-π, π]
static double normalizeAngle(double a) {
    while (a > M_PI) a -= 2.0 * M_PI;
    while (a < -M_PI) a += 2.0 * M_PI;
    return a;
}

AngularDimension::AngularDimension(const gp_Pnt2d& center, const gp_Pnt2d& point1, const gp_Pnt2d& point2)
    : Dimension(DimensionType::Angular)
    , m_center(center)
    , m_point1(point1)
    , m_point2(point2)
{
    updateText();
}

double AngularDimension::getAngle1() const {
    return std::atan2(m_point1.Y() - m_center.Y(), m_point1.X() - m_center.X());
}

double AngularDimension::getAngle2() const {
    return std::atan2(m_point2.Y() - m_center.Y(), m_point2.X() - m_center.X());
}

double AngularDimension::getSweep() const {
    // Différence signée, normalisée dans [-π, π] → on prend l'angle intérieur
    double diff = normalizeAngle(getAngle2() - getAngle1());
    return std::abs(diff);  // Toujours positif, ≤ π
}

double AngularDimension::getStartAngle() const {
    double a1 = getAngle1();
    double a2 = getAngle2();
    double diff = normalizeAngle(a2 - a1);
    
    // On dessine l'arc dans le sens antihoraire depuis startAngle
    // Si diff > 0 : l'arc va de a1 à a2 (antihoraire)
    // Si diff < 0 : l'arc va de a2 à a1 (antihoraire)
    if (diff >= 0) return a1;
    else return a2;
}

double AngularDimension::getValue() const {
    double sweep = getSweep();
    return sweep * 180.0 / M_PI;
}

void AngularDimension::setValue(double newAngleDeg) {
    double currentAngleDeg = getValue();
    if (std::abs(currentAngleDeg) < 1e-6) return;
    
    double deltaRad = (newAngleDeg - currentAngleDeg) * M_PI / 180.0;
    
    // Déterminer le sens de rotation : 
    // Si l'angle va de a1 vers a2 dans le sens antihoraire (diff > 0), 
    // augmenter l'angle = tourner point2 dans le sens antihoraire (+delta)
    // Si diff < 0, c'est l'inverse
    double diff = normalizeAngle(getAngle2() - getAngle1());
    if (diff < 0) deltaRad = -deltaRad;
    
    // Appliquer rotation sur l'entité source 2
    if (m_sourceEntity2) {
        auto [vertexPt, farPt] = getSegEndpoints(m_sourceEntity2, m_sourceSegIdx2);
        
        double dx = farPt.X() - m_center.X();
        double dy = farPt.Y() - m_center.Y();
        double dist = std::sqrt(dx*dx + dy*dy);
        double angle = std::atan2(dy, dx);
        
        double newAngle = angle + deltaRad;
        gp_Pnt2d newFarPt(
            m_center.X() + dist * std::cos(newAngle),
            m_center.Y() + dist * std::sin(newAngle)
        );
        
        // Écrire dans l'entité source
        if (m_sourceSegIdx2 < 0) {
            // SketchLine
            auto line = std::dynamic_pointer_cast<SketchLine>(m_sourceEntity2);
            if (line) {
                if (line->getStart().Distance(m_center) < 0.1)
                    line->setEnd(newFarPt);
                else
                    line->setStart(newFarPt);
            }
        } else {
            // Segment polyline
            auto poly = std::dynamic_pointer_cast<SketchPolyline>(m_sourceEntity2);
            if (poly) {
                auto pts = poly->getPoints();
                int idx = m_sourceSegIdx2;
                if (idx < (int)pts.size() - 1) {
                    // Le point loin du vertex
                    if (pts[idx].Distance(m_center) < 0.1)
                        pts[idx + 1] = newFarPt;
                    else
                        pts[idx] = newFarPt;
                    poly->setPoints(pts);
                }
            }
        }
    }
    
    // Rafraîchir points depuis les sources
    refreshFromSources();
}

void AngularDimension::refreshFromSources() {
    // Rafraîchir point1 depuis source1
    if (m_sourceEntity1) {
        auto [vertexPt, farPt] = getSegEndpoints(m_sourceEntity1, m_sourceSegIdx1);
        m_center = vertexPt;  // Le vertex doit être cohérent
        m_point1 = farPt;
    }
    
    // Rafraîchir point2 depuis source2
    if (m_sourceEntity2) {
        auto [vertexPt, farPt] = getSegEndpoints(m_sourceEntity2, m_sourceSegIdx2);
        m_point2 = farPt;
    }
    
    updateText();
}

std::pair<gp_Pnt2d, gp_Pnt2d> AngularDimension::getSegEndpoints(
    std::shared_ptr<SketchEntity> entity, int segIdx) const
{
    gp_Pnt2d pA(0,0), pB(0,0);
    
    if (segIdx < 0) {
        // SketchLine
        auto line = std::dynamic_pointer_cast<SketchLine>(entity);
        if (line) {
            pA = line->getStart();
            pB = line->getEnd();
        }
    } else {
        // Segment polyline
        auto poly = std::dynamic_pointer_cast<SketchPolyline>(entity);
        if (poly) {
            auto pts = poly->getPoints();
            if (segIdx < (int)pts.size() - 1) {
                pA = pts[segIdx];
                pB = pts[segIdx + 1];
            }
        }
    }
    
    // Identifier qui est le vertex (proche de m_center) et qui est le point loin
    if (pA.Distance(m_center) <= pB.Distance(m_center))
        return {pA, pB};  // pA = vertex, pB = far
    else
        return {pB, pA};  // pB = vertex, pA = far
}

void AngularDimension::updateText() {
    double angle = getValue();
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << angle << "\xC2\xB0";  // °
    m_text = oss.str();
    
    // Position du texte au milieu de l'arc
    double startA = getStartAngle();
    double sweep = getSweep();
    double midAngle = startA + sweep / 2.0;
    
    m_textPosition = gp_Pnt2d(
        m_center.X() + (m_radius + 5.0) * std::cos(midAngle),
        m_center.Y() + (m_radius + 5.0) * std::sin(midAngle)
    );
}

// ============================================================================
// RadialDimension
// ============================================================================

RadialDimension::RadialDimension(const gp_Pnt2d& center, double radius)
    : Dimension(DimensionType::Radial)
    , m_center(center)
    , m_radius(radius)
    , m_arrowPoint(center.X() + radius, center.Y())
{
    // Position texte par défaut
    m_textPosition = gp_Pnt2d(center.X() + radius * 0.7, center.Y());
    
    // Générer texte
    std::ostringstream oss;
    oss << "R" << std::fixed << std::setprecision(1) << radius;
    m_text = oss.str();
}

// ============================================================================
// DiametralDimension
// ============================================================================

DiametralDimension::DiametralDimension(const gp_Pnt2d& center, double diameter)
    : Dimension(DimensionType::Diametral)
    , m_center(center)
    , m_diameter(diameter)
{
    // Position texte À L'EXTÉRIEUR du cercle (comme Fusion 360)
    double angleRad = m_angle * M_PI / 180.0;
    double radius = diameter / 2.0;
    double textOffset = 20.0;  // 20mm à l'extérieur
    m_textPosition = gp_Pnt2d(
        center.X() + (radius + textOffset) * std::cos(angleRad),
        center.Y() + (radius + textOffset) * std::sin(angleRad)
    );
    
    // Générer texte avec symbole diamètre
    std::ostringstream oss;
    oss << "Ø" << std::fixed << std::setprecision(1) << diameter;
    m_text = oss.str();
}

void DiametralDimension::updateText() {
    // Recalculer texte et position
    std::ostringstream oss;
    oss << "Ø" << std::fixed << std::setprecision(1) << m_diameter;
    m_text = oss.str();
    
    // Recalculer position du texte À L'EXTÉRIEUR
    double angleRad = m_angle * M_PI / 180.0;
    double radius = m_diameter / 2.0;
    double textOffset = 20.0;  // 20mm à l'extérieur
    m_textPosition = gp_Pnt2d(
        m_center.X() + (radius + textOffset) * std::cos(angleRad),
        m_center.Y() + (radius + textOffset) * std::sin(angleRad)
    );
}

} // namespace CADEngine

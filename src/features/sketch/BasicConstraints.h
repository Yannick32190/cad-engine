#ifndef BASICCONSTRAINTS_H
#define BASICCONSTRAINTS_H

#include "GeometricConstraint.h"
#include <cmath>
#include <algorithm>

namespace CADEngine {

// ============================================================================
// HorizontalConstraint - Segment horizontal (ligne ou segment de polyline)
// ============================================================================

class HorizontalConstraint : public GeometricConstraint {
public:
    // Constructeur pour SketchLine
    HorizontalConstraint(std::shared_ptr<SketchLine> line)
        : m_line(line), m_segmentIndex(-1)
    {
    }
    
    // Constructeur pour segment de polyline
    HorizontalConstraint(std::shared_ptr<SketchPolyline> polyline, int segmentIndex)
        : m_polyline(polyline), m_segmentIndex(segmentIndex)
    {
    }
    
    ConstraintType getType() const override { return ConstraintType::Horizontal; }
    std::string getDescription() const override { return "Horizontal"; }
    std::string getSymbol() const override { return "H"; }
    int getPriority() const override { return 1; }
    
    // Accesseurs pour les points du segment contraint
    gp_Pnt2d getSegStart() const {
        if (m_line) return m_line->getStart();
        if (m_polyline && m_segmentIndex >= 0) {
            auto pts = m_polyline->getPoints();
            if (m_segmentIndex < (int)pts.size()) return pts[m_segmentIndex];
        }
        return gp_Pnt2d(0, 0);
    }
    
    gp_Pnt2d getSegEnd() const {
        if (m_line) return m_line->getEnd();
        if (m_polyline && m_segmentIndex >= 0) {
            auto pts = m_polyline->getPoints();
            if (m_segmentIndex + 1 < (int)pts.size()) return pts[m_segmentIndex + 1];
        }
        return gp_Pnt2d(0, 0);
    }
    
    bool isSatisfied(double tolerance = 1e-6) const override {
        double dy = std::abs(getSegEnd().Y() - getSegStart().Y());
        return dy < tolerance;
    }
    
    void apply() override {
        gp_Pnt2d start = getSegStart();
        gp_Pnt2d end = getSegEnd();
        
        double length = start.Distance(end);
        if (length < 1.0) length = 50.0;
        
        double direction = (end.X() > start.X()) ? 1.0 : -1.0;
        gp_Pnt2d newEnd(start.X() + direction * length, start.Y());
        
        if (m_line) {
            m_line->setEnd(newEnd);
        } else if (m_polyline && m_segmentIndex >= 0) {
            auto pts = m_polyline->getPoints();
            if (m_segmentIndex + 1 < (int)pts.size()) {
                pts[m_segmentIndex + 1] = newEnd;
                m_polyline->setPoints(pts);
            }
        }
    }
    
    std::shared_ptr<SketchLine> getLine() const { return m_line; }
    std::shared_ptr<SketchPolyline> getPolyline() const { return m_polyline; }
    int getSegmentIndex() const { return m_segmentIndex; }
    bool isPolylineConstraint() const { return m_polyline != nullptr; }
    
    // Vérifie si cette contrainte porte sur la même cible
    bool matchesLine(std::shared_ptr<SketchLine> line) const {
        return m_line && m_line == line;
    }
    bool matchesPolylineSegment(std::shared_ptr<SketchPolyline> polyline, int segIdx) const {
        return m_polyline && m_polyline == polyline && m_segmentIndex == segIdx;
    }
    
    bool referencesEntity(std::shared_ptr<SketchEntity> entity) const {
        return (m_line && m_line == std::dynamic_pointer_cast<SketchLine>(entity)) ||
               (m_polyline && m_polyline == std::dynamic_pointer_cast<SketchPolyline>(entity));
    }
    
private:
    std::shared_ptr<SketchLine> m_line;
    std::shared_ptr<SketchPolyline> m_polyline;
    int m_segmentIndex;
};

// ============================================================================
// VerticalConstraint - Segment vertical (ligne ou segment de polyline)
// ============================================================================

class VerticalConstraint : public GeometricConstraint {
public:
    // Constructeur pour SketchLine
    VerticalConstraint(std::shared_ptr<SketchLine> line)
        : m_line(line), m_segmentIndex(-1)
    {
    }
    
    // Constructeur pour segment de polyline
    VerticalConstraint(std::shared_ptr<SketchPolyline> polyline, int segmentIndex)
        : m_polyline(polyline), m_segmentIndex(segmentIndex)
    {
    }
    
    ConstraintType getType() const override { return ConstraintType::Vertical; }
    std::string getDescription() const override { return "Vertical"; }
    std::string getSymbol() const override { return "V"; }
    int getPriority() const override { return 1; }
    
    gp_Pnt2d getSegStart() const {
        if (m_line) return m_line->getStart();
        if (m_polyline && m_segmentIndex >= 0) {
            auto pts = m_polyline->getPoints();
            if (m_segmentIndex < (int)pts.size()) return pts[m_segmentIndex];
        }
        return gp_Pnt2d(0, 0);
    }
    
    gp_Pnt2d getSegEnd() const {
        if (m_line) return m_line->getEnd();
        if (m_polyline && m_segmentIndex >= 0) {
            auto pts = m_polyline->getPoints();
            if (m_segmentIndex + 1 < (int)pts.size()) return pts[m_segmentIndex + 1];
        }
        return gp_Pnt2d(0, 0);
    }
    
    bool isSatisfied(double tolerance = 1e-6) const override {
        double dx = std::abs(getSegEnd().X() - getSegStart().X());
        return dx < tolerance;
    }
    
    void apply() override {
        gp_Pnt2d start = getSegStart();
        gp_Pnt2d end = getSegEnd();
        
        double length = start.Distance(end);
        if (length < 1.0) length = 50.0;
        
        double direction = (end.Y() > start.Y()) ? 1.0 : -1.0;
        gp_Pnt2d newEnd(start.X(), start.Y() + direction * length);
        
        if (m_line) {
            m_line->setEnd(newEnd);
        } else if (m_polyline && m_segmentIndex >= 0) {
            auto pts = m_polyline->getPoints();
            if (m_segmentIndex + 1 < (int)pts.size()) {
                pts[m_segmentIndex + 1] = newEnd;
                m_polyline->setPoints(pts);
            }
        }
    }
    
    std::shared_ptr<SketchLine> getLine() const { return m_line; }
    std::shared_ptr<SketchPolyline> getPolyline() const { return m_polyline; }
    int getSegmentIndex() const { return m_segmentIndex; }
    bool isPolylineConstraint() const { return m_polyline != nullptr; }
    
    bool matchesLine(std::shared_ptr<SketchLine> line) const {
        return m_line && m_line == line;
    }
    bool matchesPolylineSegment(std::shared_ptr<SketchPolyline> polyline, int segIdx) const {
        return m_polyline && m_polyline == polyline && m_segmentIndex == segIdx;
    }
    
    bool referencesEntity(std::shared_ptr<SketchEntity> entity) const {
        return (m_line && m_line == std::dynamic_pointer_cast<SketchLine>(entity)) ||
               (m_polyline && m_polyline == std::dynamic_pointer_cast<SketchPolyline>(entity));
    }
    
private:
    std::shared_ptr<SketchLine> m_line;
    std::shared_ptr<SketchPolyline> m_polyline;
    int m_segmentIndex;
};

// ============================================================================
// ParallelConstraint - 2 segments parallèles (lignes et/ou segments polyline)
// ============================================================================

class ParallelConstraint : public GeometricConstraint {
public:
    // Ligne + Ligne
    ParallelConstraint(std::shared_ptr<SketchLine> line1, 
                       std::shared_ptr<SketchLine> line2)
        : m_line1(line1), m_line2(line2), m_segIdx1(-1), m_segIdx2(-1)
    {
    }
    
    // Ligne + Polyline segment
    ParallelConstraint(std::shared_ptr<SketchLine> line1,
                       std::shared_ptr<SketchPolyline> poly2, int segIdx2)
        : m_line1(line1), m_poly2(poly2), m_segIdx1(-1), m_segIdx2(segIdx2)
    {
    }
    
    // Polyline segment + Ligne
    ParallelConstraint(std::shared_ptr<SketchPolyline> poly1, int segIdx1,
                       std::shared_ptr<SketchLine> line2)
        : m_poly1(poly1), m_line2(line2), m_segIdx1(segIdx1), m_segIdx2(-1)
    {
    }
    
    // Polyline segment + Polyline segment
    ParallelConstraint(std::shared_ptr<SketchPolyline> poly1, int segIdx1,
                       std::shared_ptr<SketchPolyline> poly2, int segIdx2)
        : m_poly1(poly1), m_poly2(poly2), m_segIdx1(segIdx1), m_segIdx2(segIdx2)
    {
    }
    
    ConstraintType getType() const override { return ConstraintType::Parallel; }
    std::string getDescription() const override { return "Parallel"; }
    std::string getSymbol() const override { return "//"; }
    int getPriority() const override { return 3; }
    
    // Accesseurs segments
    gp_Pnt2d getSeg1Start() const {
        if (m_line1) return m_line1->getStart();
        if (m_poly1 && m_segIdx1 >= 0) {
            auto pts = m_poly1->getPoints();
            if (m_segIdx1 < (int)pts.size()) return pts[m_segIdx1];
        }
        return gp_Pnt2d(0, 0);
    }
    gp_Pnt2d getSeg1End() const {
        if (m_line1) return m_line1->getEnd();
        if (m_poly1 && m_segIdx1 >= 0) {
            auto pts = m_poly1->getPoints();
            if (m_segIdx1 + 1 < (int)pts.size()) return pts[m_segIdx1 + 1];
        }
        return gp_Pnt2d(0, 0);
    }
    gp_Pnt2d getSeg2Start() const {
        if (m_line2) return m_line2->getStart();
        if (m_poly2 && m_segIdx2 >= 0) {
            auto pts = m_poly2->getPoints();
            if (m_segIdx2 < (int)pts.size()) return pts[m_segIdx2];
        }
        return gp_Pnt2d(0, 0);
    }
    gp_Pnt2d getSeg2End() const {
        if (m_line2) return m_line2->getEnd();
        if (m_poly2 && m_segIdx2 >= 0) {
            auto pts = m_poly2->getPoints();
            if (m_segIdx2 + 1 < (int)pts.size()) return pts[m_segIdx2 + 1];
        }
        return gp_Pnt2d(0, 0);
    }
    
    bool isSatisfied(double tolerance = 1e-6) const override {
        double dx1 = getSeg1End().X() - getSeg1Start().X();
        double dy1 = getSeg1End().Y() - getSeg1Start().Y();
        double dx2 = getSeg2End().X() - getSeg2Start().X();
        double dy2 = getSeg2End().Y() - getSeg2Start().Y();
        
        double cross = std::abs(dx1 * dy2 - dy1 * dx2);
        return cross < tolerance * std::max(std::sqrt(dx1*dx1 + dy1*dy1), 
                                            std::sqrt(dx2*dx2 + dy2*dy2));
    }
    
    void apply() override {
        // Aligner segment2 sur la direction de segment1
        double dx1 = getSeg1End().X() - getSeg1Start().X();
        double dy1 = getSeg1End().Y() - getSeg1Start().Y();
        double len1 = std::sqrt(dx1*dx1 + dy1*dy1);
        if (len1 < 1e-6) return;
        
        double dirX = dx1 / len1;
        double dirY = dy1 / len1;
        
        // Direction actuelle du segment 2
        gp_Pnt2d start2 = getSeg2Start();
        gp_Pnt2d end2 = getSeg2End();
        double dx2 = end2.X() - start2.X();
        double dy2 = end2.Y() - start2.Y();
        double len2 = start2.Distance(end2);
        if (len2 < 1e-6) return;
        
        // Choisir la direction parallèle la plus proche (évite flip 180°)
        double dot = dirX * dx2 + dirY * dy2;
        if (dot < 0) {
            dirX = -dirX;
            dirY = -dirY;
        }
        
        gp_Pnt2d newEnd2(start2.X() + dirX * len2, start2.Y() + dirY * len2);
        
        if (m_line2) {
            m_line2->setEnd(newEnd2);
        } else if (m_poly2 && m_segIdx2 >= 0) {
            auto pts = m_poly2->getPoints();
            if (m_segIdx2 + 1 < (int)pts.size()) {
                pts[m_segIdx2 + 1] = newEnd2;
                m_poly2->setPoints(pts);
            }
        }
    }
    
    // Vérifie si cette contrainte référence une entité donnée
    bool referencesEntity(std::shared_ptr<SketchEntity> entity) const {
        return (m_line1 && m_line1 == std::dynamic_pointer_cast<SketchLine>(entity)) ||
               (m_line2 && m_line2 == std::dynamic_pointer_cast<SketchLine>(entity)) ||
               (m_poly1 && m_poly1 == std::dynamic_pointer_cast<SketchPolyline>(entity)) ||
               (m_poly2 && m_poly2 == std::dynamic_pointer_cast<SketchPolyline>(entity));
    }
    
private:
    std::shared_ptr<SketchLine> m_line1;
    std::shared_ptr<SketchLine> m_line2;
    std::shared_ptr<SketchPolyline> m_poly1;
    std::shared_ptr<SketchPolyline> m_poly2;
    int m_segIdx1;
    int m_segIdx2;
};

// ============================================================================
// PerpendicularConstraint - 2 segments perpendiculaires (lignes et/ou polyline)
// ============================================================================

class PerpendicularConstraint : public GeometricConstraint {
public:
    // Ligne + Ligne
    PerpendicularConstraint(std::shared_ptr<SketchLine> line1, 
                            std::shared_ptr<SketchLine> line2)
        : m_line1(line1), m_line2(line2), m_segIdx1(-1), m_segIdx2(-1)
    {
    }
    
    // Ligne + Polyline segment
    PerpendicularConstraint(std::shared_ptr<SketchLine> line1,
                            std::shared_ptr<SketchPolyline> poly2, int segIdx2)
        : m_line1(line1), m_poly2(poly2), m_segIdx1(-1), m_segIdx2(segIdx2)
    {
    }
    
    // Polyline segment + Ligne
    PerpendicularConstraint(std::shared_ptr<SketchPolyline> poly1, int segIdx1,
                            std::shared_ptr<SketchLine> line2)
        : m_poly1(poly1), m_line2(line2), m_segIdx1(segIdx1), m_segIdx2(-1)
    {
    }
    
    // Polyline segment + Polyline segment
    PerpendicularConstraint(std::shared_ptr<SketchPolyline> poly1, int segIdx1,
                            std::shared_ptr<SketchPolyline> poly2, int segIdx2)
        : m_poly1(poly1), m_poly2(poly2), m_segIdx1(segIdx1), m_segIdx2(segIdx2)
    {
    }
    
    ConstraintType getType() const override { return ConstraintType::Perpendicular; }
    std::string getDescription() const override { return "Perpendicular"; }
    std::string getSymbol() const override { return "⊥"; }
    int getPriority() const override { return 3; }
    
    gp_Pnt2d getSeg1Start() const {
        if (m_line1) return m_line1->getStart();
        if (m_poly1 && m_segIdx1 >= 0) {
            auto pts = m_poly1->getPoints();
            if (m_segIdx1 < (int)pts.size()) return pts[m_segIdx1];
        }
        return gp_Pnt2d(0, 0);
    }
    gp_Pnt2d getSeg1End() const {
        if (m_line1) return m_line1->getEnd();
        if (m_poly1 && m_segIdx1 >= 0) {
            auto pts = m_poly1->getPoints();
            if (m_segIdx1 + 1 < (int)pts.size()) return pts[m_segIdx1 + 1];
        }
        return gp_Pnt2d(0, 0);
    }
    gp_Pnt2d getSeg2Start() const {
        if (m_line2) return m_line2->getStart();
        if (m_poly2 && m_segIdx2 >= 0) {
            auto pts = m_poly2->getPoints();
            if (m_segIdx2 < (int)pts.size()) return pts[m_segIdx2];
        }
        return gp_Pnt2d(0, 0);
    }
    gp_Pnt2d getSeg2End() const {
        if (m_line2) return m_line2->getEnd();
        if (m_poly2 && m_segIdx2 >= 0) {
            auto pts = m_poly2->getPoints();
            if (m_segIdx2 + 1 < (int)pts.size()) return pts[m_segIdx2 + 1];
        }
        return gp_Pnt2d(0, 0);
    }
    
    bool isSatisfied(double tolerance = 1e-6) const override {
        double dx1 = getSeg1End().X() - getSeg1Start().X();
        double dy1 = getSeg1End().Y() - getSeg1Start().Y();
        double dx2 = getSeg2End().X() - getSeg2Start().X();
        double dy2 = getSeg2End().Y() - getSeg2Start().Y();
        
        double dot = std::abs(dx1 * dx2 + dy1 * dy2);
        return dot < tolerance * std::sqrt((dx1*dx1 + dy1*dy1) * (dx2*dx2 + dy2*dy2));
    }
    
    void apply() override {
        // Direction de segment1
        double dx1 = getSeg1End().X() - getSeg1Start().X();
        double dy1 = getSeg1End().Y() - getSeg1Start().Y();
        double len1 = std::sqrt(dx1*dx1 + dy1*dy1);
        if (len1 < 1e-6) return;
        
        // Direction perpendiculaire (-dy, dx)
        double perpX = -dy1 / len1;
        double perpY = dx1 / len1;
        
        // Direction actuelle du segment 2
        gp_Pnt2d start2 = getSeg2Start();
        gp_Pnt2d end2 = getSeg2End();
        double dx2 = end2.X() - start2.X();
        double dy2 = end2.Y() - start2.Y();
        double len2 = start2.Distance(end2);
        if (len2 < 1e-6) return;
        
        // Choisir la perpendiculaire la plus proche de la direction actuelle
        // (évite le flip de 180°)
        double dot = perpX * dx2 + perpY * dy2;
        if (dot < 0) {
            perpX = -perpX;
            perpY = -perpY;
        }
        
        gp_Pnt2d newEnd2(start2.X() + perpX * len2, start2.Y() + perpY * len2);
        
        if (m_line2) {
            m_line2->setEnd(newEnd2);
        } else if (m_poly2 && m_segIdx2 >= 0) {
            auto pts = m_poly2->getPoints();
            if (m_segIdx2 + 1 < (int)pts.size()) {
                pts[m_segIdx2 + 1] = newEnd2;
                m_poly2->setPoints(pts);
            }
        }
    }
    
    bool referencesEntity(std::shared_ptr<SketchEntity> entity) const {
        return (m_line1 && m_line1 == std::dynamic_pointer_cast<SketchLine>(entity)) ||
               (m_line2 && m_line2 == std::dynamic_pointer_cast<SketchLine>(entity)) ||
               (m_poly1 && m_poly1 == std::dynamic_pointer_cast<SketchPolyline>(entity)) ||
               (m_poly2 && m_poly2 == std::dynamic_pointer_cast<SketchPolyline>(entity));
    }
    
private:
    std::shared_ptr<SketchLine> m_line1;
    std::shared_ptr<SketchLine> m_line2;
    std::shared_ptr<SketchPolyline> m_poly1;
    std::shared_ptr<SketchPolyline> m_poly2;
    int m_segIdx1;
    int m_segIdx2;
};

// ============================================================================
// CoincidentConstraint - 2 points au même endroit (générique)
// Supporte: lignes, cercles, arcs, polylignes, rectangles
// vertexIndex: -1=start, -2=end, -3=center, >=0 polyline/rect vertex
// ============================================================================

class CoincidentConstraint : public GeometricConstraint {
public:
    CoincidentConstraint(std::shared_ptr<SketchEntity> entity1, int vertexIndex1,
                         std::shared_ptr<SketchEntity> entity2, int vertexIndex2)
        : m_entity1(entity1), m_vertexIndex1(vertexIndex1)
        , m_entity2(entity2), m_vertexIndex2(vertexIndex2)
    {
    }
    
    ConstraintType getType() const override { return ConstraintType::Coincident; }
    std::string getDescription() const override { return "Coincident"; }
    std::string getSymbol() const override { return "•"; }
    int getPriority() const override { return 2; }
    
    gp_Pnt2d getPoint(std::shared_ptr<SketchEntity> entity, int vertexIndex) const {
        if (!entity) return gp_Pnt2d(0, 0);
        
        if (entity->getType() == SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<SketchLine>(entity);
            if (!line) return gp_Pnt2d(0, 0);
            if (vertexIndex == -1) return line->getStart();
            if (vertexIndex == -2) return line->getEnd();
        }
        else if (entity->getType() == SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<SketchCircle>(entity);
            if (circle && vertexIndex == -3) return circle->getCenter();
        }
        else if (entity->getType() == SketchEntityType::Arc) {
            auto arc = std::dynamic_pointer_cast<SketchArc>(entity);
            if (!arc) return gp_Pnt2d(0, 0);
            if (vertexIndex == -1) return arc->getStart();
            if (vertexIndex == -2) return arc->getEnd();
        }
        else if (entity->getType() == SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<SketchPolyline>(entity);
            if (!polyline) return gp_Pnt2d(0, 0);
            auto pts = polyline->getPoints();
            if (vertexIndex >= 0 && vertexIndex < (int)pts.size()) {
                return pts[vertexIndex];
            }
        }
        else if (entity->getType() == SketchEntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<SketchRectangle>(entity);
            if (!rect) return gp_Pnt2d(0, 0);
            auto corners = rect->getKeyPoints();
            if (vertexIndex >= 0 && vertexIndex < (int)corners.size()) {
                return corners[vertexIndex];
            }
        }
        return gp_Pnt2d(0, 0);
    }
    
    void setPoint(std::shared_ptr<SketchEntity> entity, int vertexIndex, const gp_Pnt2d& pt) {
        if (!entity) return;
        
        if (entity->getType() == SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<SketchLine>(entity);
            if (!line) return;
            if (vertexIndex == -1) line->setStart(pt);
            if (vertexIndex == -2) line->setEnd(pt);
        }
        else if (entity->getType() == SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<SketchCircle>(entity);
            if (circle && vertexIndex == -3) circle->setCenter(pt);
        }
        else if (entity->getType() == SketchEntityType::Polyline) {
            auto polyline = std::dynamic_pointer_cast<SketchPolyline>(entity);
            if (!polyline) return;
            auto pts = polyline->getPoints();
            if (vertexIndex >= 0 && vertexIndex < (int)pts.size()) {
                pts[vertexIndex] = pt;
                polyline->setPoints(pts);
            }
        }
        // Arc et Rectangle : pas de setters individuels pour l'instant
    }
    
    bool isSatisfied(double tolerance = 1e-6) const override {
        if (!m_entity1 || !m_entity2) return false;
        gp_Pnt2d p1 = getPoint(m_entity1, m_vertexIndex1);
        gp_Pnt2d p2 = getPoint(m_entity2, m_vertexIndex2);
        return p1.Distance(p2) < tolerance;
    }
    
    void apply() override {
        if (!m_entity1 || !m_entity2) return;
        
        gp_Pnt2d p1 = getPoint(m_entity1, m_vertexIndex1);
        gp_Pnt2d p2 = getPoint(m_entity2, m_vertexIndex2);
        
        // Position moyenne
        gp_Pnt2d avg((p1.X() + p2.X()) / 2.0, (p1.Y() + p2.Y()) / 2.0);
        
        setPoint(m_entity1, m_vertexIndex1, avg);
        setPoint(m_entity2, m_vertexIndex2, avg);
    }
    
    std::shared_ptr<SketchEntity> getEntity1() const { return m_entity1; }
    std::shared_ptr<SketchEntity> getEntity2() const { return m_entity2; }
    int getVertexIndex1() const { return m_vertexIndex1; }
    int getVertexIndex2() const { return m_vertexIndex2; }
    
    bool referencesEntity(std::shared_ptr<SketchEntity> entity) const {
        return m_entity1 == entity || m_entity2 == entity;
    }
    
private:
    std::shared_ptr<SketchEntity> m_entity1;
    int m_vertexIndex1;
    std::shared_ptr<SketchEntity> m_entity2;
    int m_vertexIndex2;
};

// ============================================================================
// ConcentricConstraint - 2 cercles même centre
// ============================================================================

class ConcentricConstraint : public GeometricConstraint {
public:
    ConcentricConstraint(std::shared_ptr<SketchCircle> circle1,
                         std::shared_ptr<SketchCircle> circle2)
        : m_circle1(circle1), m_circle2(circle2)
    {
    }
    
    ConstraintType getType() const override { return ConstraintType::Concentric; }
    std::string getDescription() const override { return "Concentric"; }
    std::string getSymbol() const override { return "◎"; }
    int getPriority() const override { return 2; }
    
    bool isSatisfied(double tolerance = 1e-6) const override {
        if (!m_circle1 || !m_circle2) return false;
        return m_circle1->getCenter().Distance(m_circle2->getCenter()) < tolerance;
    }
    
    void apply() override {
        if (!m_circle1 || !m_circle2) return;
        
        gp_Pnt2d c1 = m_circle1->getCenter();
        gp_Pnt2d c2 = m_circle2->getCenter();
        
        // Centre moyen
        gp_Pnt2d avg((c1.X() + c2.X()) / 2.0, (c1.Y() + c2.Y()) / 2.0);
        
        m_circle1->setCenter(avg);
        m_circle2->setCenter(avg);
    }
    
    bool referencesEntity(std::shared_ptr<SketchEntity> entity) const {
        auto circle = std::dynamic_pointer_cast<SketchCircle>(entity);
        return circle && (m_circle1 == circle || m_circle2 == circle);
    }
    
private:
    std::shared_ptr<SketchCircle> m_circle1;
    std::shared_ptr<SketchCircle> m_circle2;
};

// ============================================================================
// FixConstraint - Verrouillage d'un point (ne bouge pas au drag)
// vertexIndex: -1=start, -2=end, -3=center, >=0 polyline/rect vertex
// ============================================================================

class FixConstraint : public GeometricConstraint {
public:
    FixConstraint(std::shared_ptr<SketchEntity> entity, int vertexIndex, const gp_Pnt2d& lockedPos)
        : m_entity(entity), m_vertexIndex(vertexIndex), m_lockedPosition(lockedPos)
    {
    }
    
    ConstraintType getType() const override { return ConstraintType::Fix; }
    std::string getDescription() const override { return "Fix"; }
    std::string getSymbol() const override { return "\xF0\x9F\x94\x92"; }  // 🔒
    int getPriority() const override { return 0; }  // Haute priorité
    
    gp_Pnt2d getLockedPosition() const { return m_lockedPosition; }
    std::shared_ptr<SketchEntity> getEntity() const { return m_entity; }
    int getVertexIndex() const { return m_vertexIndex; }
    
    gp_Pnt2d getPoint() const {
        if (!m_entity) return m_lockedPosition;
        if (m_entity->getType() == SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<SketchLine>(m_entity);
            if (line) {
                if (m_vertexIndex == -1) return line->getStart();
                if (m_vertexIndex == -2) return line->getEnd();
            }
        } else if (m_entity->getType() == SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<SketchCircle>(m_entity);
            if (circle && m_vertexIndex == -3) return circle->getCenter();
        } else if (m_entity->getType() == SketchEntityType::Polyline) {
            auto poly = std::dynamic_pointer_cast<SketchPolyline>(m_entity);
            if (poly) {
                auto pts = poly->getPoints();
                if (m_vertexIndex >= 0 && m_vertexIndex < (int)pts.size())
                    return pts[m_vertexIndex];
            }
        } else if (m_entity->getType() == SketchEntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<SketchRectangle>(m_entity);
            if (rect) {
                auto corners = rect->getKeyPoints();
                if (m_vertexIndex >= 0 && m_vertexIndex < (int)corners.size())
                    return corners[m_vertexIndex];
            }
        }
        return m_lockedPosition;
    }
    
    bool isSatisfied(double tolerance = 1e-6) const override {
        return getPoint().Distance(m_lockedPosition) < tolerance;
    }
    
    void apply() override {
        if (!m_entity) return;
        if (m_entity->getType() == SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<SketchLine>(m_entity);
            if (line) {
                if (m_vertexIndex == -1) line->setStart(m_lockedPosition);
                if (m_vertexIndex == -2) line->setEnd(m_lockedPosition);
            }
        } else if (m_entity->getType() == SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<SketchCircle>(m_entity);
            if (circle && m_vertexIndex == -3) circle->setCenter(m_lockedPosition);
        } else if (m_entity->getType() == SketchEntityType::Polyline) {
            auto poly = std::dynamic_pointer_cast<SketchPolyline>(m_entity);
            if (poly) {
                auto pts = poly->getPoints();
                if (m_vertexIndex >= 0 && m_vertexIndex < (int)pts.size()) {
                    pts[m_vertexIndex] = m_lockedPosition;
                    poly->setPoints(pts);
                }
            }
        }
    }
    
    bool matchesVertex(std::shared_ptr<SketchEntity> entity, int vertexIdx) const {
        return m_entity == entity && m_vertexIndex == vertexIdx;
    }
    
    bool referencesEntity(std::shared_ptr<SketchEntity> entity) const {
        return m_entity == entity;
    }
    
private:
    std::shared_ptr<SketchEntity> m_entity;
    int m_vertexIndex;
    gp_Pnt2d m_lockedPosition;
};

// ============================================================================
// SymmetricConstraint - 2 points symétriques par rapport à un axe (ligne/segment)
// ============================================================================

class SymmetricConstraint : public GeometricConstraint {
public:
    // Axe = SketchLine
    SymmetricConstraint(std::shared_ptr<SketchEntity> entity1, int vtx1,
                        std::shared_ptr<SketchEntity> entity2, int vtx2,
                        std::shared_ptr<SketchLine> axisLine)
        : m_entity1(entity1), m_vtx1(vtx1)
        , m_entity2(entity2), m_vtx2(vtx2)
        , m_axisLine(axisLine)
    {
    }
    
    // Axe = segment de polyline
    SymmetricConstraint(std::shared_ptr<SketchEntity> entity1, int vtx1,
                        std::shared_ptr<SketchEntity> entity2, int vtx2,
                        std::shared_ptr<SketchPolyline> axisPoly, int axisSegIdx)
        : m_entity1(entity1), m_vtx1(vtx1)
        , m_entity2(entity2), m_vtx2(vtx2)
        , m_axisPoly(axisPoly), m_axisSegIdx(axisSegIdx)
    {
    }
    
    ConstraintType getType() const override { return ConstraintType::Symmetric; }
    std::string getDescription() const override { return "Symmetric"; }
    std::string getSymbol() const override { return "\xE2\x87\x94"; }  // ⇔
    int getPriority() const override { return 3; }
    
    // Helper : lire/écrire un point dans une entité (réutilise la logique Coincident)
    gp_Pnt2d readPoint(std::shared_ptr<SketchEntity> entity, int vtx) const {
        if (!entity) return gp_Pnt2d(0, 0);
        if (entity->getType() == SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<SketchLine>(entity);
            if (line) { return (vtx == -1) ? line->getStart() : line->getEnd(); }
        } else if (entity->getType() == SketchEntityType::Circle) {
            auto c = std::dynamic_pointer_cast<SketchCircle>(entity);
            if (c && vtx == -3) return c->getCenter();
        } else if (entity->getType() == SketchEntityType::Arc) {
            auto a = std::dynamic_pointer_cast<SketchArc>(entity);
            if (a) {
                if (vtx == -1) return a->getStart();
                if (vtx == -2) return a->getEnd();
                if (vtx == -3) return a->getMid();
            }
        } else if (entity->getType() == SketchEntityType::Rectangle) {
            auto r = std::dynamic_pointer_cast<SketchRectangle>(entity);
            if (r) {
                gp_Pnt2d c = r->getCorner();
                double w = r->getWidth(), h = r->getHeight();
                if (vtx == 0) return c;                              // coin bas-gauche
                if (vtx == 1) return gp_Pnt2d(c.X()+w, c.Y());     // coin bas-droit
                if (vtx == 2) return gp_Pnt2d(c.X()+w, c.Y()+h);   // coin haut-droit
                if (vtx == 3) return gp_Pnt2d(c.X(), c.Y()+h);     // coin haut-gauche
                if (vtx == -3) return gp_Pnt2d(c.X()+w/2, c.Y()+h/2); // centre
            }
        } else if (entity->getType() == SketchEntityType::Ellipse) {
            auto e = std::dynamic_pointer_cast<SketchEllipse>(entity);
            if (e && vtx == -3) return e->getCenter();
        } else if (entity->getType() == SketchEntityType::Polygon) {
            auto p = std::dynamic_pointer_cast<SketchPolygon>(entity);
            if (p && vtx == -3) return p->getCenter();
        } else if (entity->getType() == SketchEntityType::Oblong) {
            auto o = std::dynamic_pointer_cast<SketchOblong>(entity);
            if (o && vtx == -3) return o->getCenter();
        } else if (entity->getType() == SketchEntityType::Polyline) {
            auto p = std::dynamic_pointer_cast<SketchPolyline>(entity);
            if (p) { auto pts = p->getPoints(); if (vtx >= 0 && vtx < (int)pts.size()) return pts[vtx]; }
        }
        return gp_Pnt2d(0, 0);
    }
    
    void writePoint(std::shared_ptr<SketchEntity> entity, int vtx, const gp_Pnt2d& pt) const {
        if (!entity) return;
        if (entity->getType() == SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<SketchLine>(entity);
            if (line) { if (vtx == -1) line->setStart(pt); else if (vtx == -2) line->setEnd(pt); }
        } else if (entity->getType() == SketchEntityType::Circle) {
            auto c = std::dynamic_pointer_cast<SketchCircle>(entity);
            if (c && vtx == -3) c->setCenter(pt);
        } else if (entity->getType() == SketchEntityType::Arc) {
            // Arc: read-only pour la symétrie (pas de setters individuels)
        } else if (entity->getType() == SketchEntityType::Rectangle) {
            auto r = std::dynamic_pointer_cast<SketchRectangle>(entity);
            if (r) {
                if (vtx == -3) {
                    // Déplacer le centre → déplacer le corner (le rect garde ses dimensions)
                    double w = r->getWidth(), h = r->getHeight();
                    r->setCorner(gp_Pnt2d(pt.X() - w/2, pt.Y() - h/2));
                } else if (vtx >= 0 && vtx <= 3) {
                    // Déplacer un coin → recalculer le rectangle englobant
                    gp_Pnt2d c = r->getCorner();
                    double w = r->getWidth(), h = r->getHeight();
                    gp_Pnt2d corners[4];
                    corners[0] = c;
                    corners[1] = gp_Pnt2d(c.X()+w, c.Y());
                    corners[2] = gp_Pnt2d(c.X()+w, c.Y()+h);
                    corners[3] = gp_Pnt2d(c.X(), c.Y()+h);
                    corners[vtx] = pt;
                    double minX = corners[0].X(), minY = corners[0].Y();
                    double maxX = corners[0].X(), maxY = corners[0].Y();
                    for (int i = 1; i < 4; i++) {
                        if (corners[i].X() < minX) minX = corners[i].X();
                        if (corners[i].Y() < minY) minY = corners[i].Y();
                        if (corners[i].X() > maxX) maxX = corners[i].X();
                        if (corners[i].Y() > maxY) maxY = corners[i].Y();
                    }
                    r->setCorner(gp_Pnt2d(minX, minY));
                    r->setWidth(maxX - minX);
                    r->setHeight(maxY - minY);
                }
            }
        } else if (entity->getType() == SketchEntityType::Ellipse) {
            auto e = std::dynamic_pointer_cast<SketchEllipse>(entity);
            if (e && vtx == -3) e->setCenter(pt);
        } else if (entity->getType() == SketchEntityType::Polygon) {
            auto p = std::dynamic_pointer_cast<SketchPolygon>(entity);
            if (p && vtx == -3) p->setCenter(pt);
        } else if (entity->getType() == SketchEntityType::Oblong) {
            auto o = std::dynamic_pointer_cast<SketchOblong>(entity);
            if (o && vtx == -3) o->setCenter(pt);
        } else if (entity->getType() == SketchEntityType::Polyline) {
            auto p = std::dynamic_pointer_cast<SketchPolyline>(entity);
            if (p) { auto pts = p->getPoints(); if (vtx >= 0 && vtx < (int)pts.size()) { pts[vtx] = pt; p->setPoints(pts); } }
        }
    }
    
    // Points de l'axe
    gp_Pnt2d getAxisStart() const {
        if (m_axisLine) return m_axisLine->getStart();
        if (m_axisPoly && m_axisSegIdx >= 0) {
            auto pts = m_axisPoly->getPoints();
            if (m_axisSegIdx < (int)pts.size()) return pts[m_axisSegIdx];
        }
        return gp_Pnt2d(0, 0);
    }
    
    gp_Pnt2d getAxisEnd() const {
        if (m_axisLine) return m_axisLine->getEnd();
        if (m_axisPoly && m_axisSegIdx >= 0) {
            auto pts = m_axisPoly->getPoints();
            if (m_axisSegIdx + 1 < (int)pts.size()) return pts[m_axisSegIdx + 1];
        }
        return gp_Pnt2d(0, 0);
    }
    
    // Miroir d'un point par rapport à l'axe
    gp_Pnt2d mirrorPoint(const gp_Pnt2d& pt) const {
        gp_Pnt2d a = getAxisStart();
        gp_Pnt2d b = getAxisEnd();
        double dx = b.X() - a.X();
        double dy = b.Y() - a.Y();
        double len2 = dx * dx + dy * dy;
        if (len2 < 1e-12) return pt;
        
        // Projection de pt sur la droite AB
        double t = ((pt.X() - a.X()) * dx + (pt.Y() - a.Y()) * dy) / len2;
        double projX = a.X() + t * dx;
        double projY = a.Y() + t * dy;
        
        // Miroir = 2 * projection - point
        return gp_Pnt2d(2.0 * projX - pt.X(), 2.0 * projY - pt.Y());
    }
    
    bool isSatisfied(double tolerance = 1e-6) const override {
        // Rectangles : vérifier que le centre de r2 = miroir du centre de r1
        if (m_entity1 && m_entity2 &&
            m_entity1->getType() == SketchEntityType::Rectangle &&
            m_entity2->getType() == SketchEntityType::Rectangle) {
            auto r1 = std::dynamic_pointer_cast<SketchRectangle>(m_entity1);
            auto r2 = std::dynamic_pointer_cast<SketchRectangle>(m_entity2);
            if (r1 && r2) {
                gp_Pnt2d c1(r1->getCorner().X() + r1->getWidth()/2,
                            r1->getCorner().Y() + r1->getHeight()/2);
                gp_Pnt2d c2(r2->getCorner().X() + r2->getWidth()/2,
                            r2->getCorner().Y() + r2->getHeight()/2);
                gp_Pnt2d mc = mirrorPoint(c1);
                return mc.Distance(c2) < tolerance &&
                       std::abs(r1->getWidth() - r2->getWidth()) < tolerance &&
                       std::abs(r1->getHeight() - r2->getHeight()) < tolerance;
            }
        }
        
        // Cas général
        gp_Pnt2d p1 = readPoint(m_entity1, m_vtx1);
        gp_Pnt2d p2 = readPoint(m_entity2, m_vtx2);
        gp_Pnt2d m = mirrorPoint(p1);
        return m.Distance(p2) < tolerance;
    }
    
    void apply() override {
        // Cas spécial : Rectangle → miroir du centre + copie dimensions
        if (m_entity1 && m_entity2 &&
            m_entity1->getType() == SketchEntityType::Rectangle &&
            m_entity2->getType() == SketchEntityType::Rectangle) {
            auto r1 = std::dynamic_pointer_cast<SketchRectangle>(m_entity1);
            auto r2 = std::dynamic_pointer_cast<SketchRectangle>(m_entity2);
            if (r1 && r2) {
                // Centre de r1
                gp_Pnt2d c1(r1->getCorner().X() + r1->getWidth()/2,
                            r1->getCorner().Y() + r1->getHeight()/2);
                // Miroir du centre
                gp_Pnt2d mc = mirrorPoint(c1);
                // Copier dimensions + placer au miroir
                r2->setWidth(r1->getWidth());
                r2->setHeight(r1->getHeight());
                r2->setCorner(gp_Pnt2d(mc.X() - r1->getWidth()/2,
                                        mc.Y() - r1->getHeight()/2));
                return;
            }
        }
        
        // Cas spécial : Circle → miroir du centre + copie rayon
        if (m_entity1 && m_entity2 &&
            m_entity1->getType() == SketchEntityType::Circle &&
            m_entity2->getType() == SketchEntityType::Circle) {
            auto c1 = std::dynamic_pointer_cast<SketchCircle>(m_entity1);
            auto c2 = std::dynamic_pointer_cast<SketchCircle>(m_entity2);
            if (c1 && c2) {
                gp_Pnt2d mc = mirrorPoint(c1->getCenter());
                c2->setCenter(mc);
                c2->setRadius(c1->getRadius());
                return;
            }
        }
        
        // Cas spécial : Ellipse → miroir du centre + copie dimensions
        if (m_entity1 && m_entity2 &&
            m_entity1->getType() == SketchEntityType::Ellipse &&
            m_entity2->getType() == SketchEntityType::Ellipse) {
            auto e1 = std::dynamic_pointer_cast<SketchEllipse>(m_entity1);
            auto e2 = std::dynamic_pointer_cast<SketchEllipse>(m_entity2);
            if (e1 && e2) {
                gp_Pnt2d mc = mirrorPoint(e1->getCenter());
                e2->setCenter(mc);
                e2->setMajorRadius(e1->getMajorRadius());
                e2->setMinorRadius(e1->getMinorRadius());
                return;
            }
        }
        
        // Cas spécial : Polygon → miroir du centre + copie rayon/sides
        if (m_entity1 && m_entity2 &&
            m_entity1->getType() == SketchEntityType::Polygon &&
            m_entity2->getType() == SketchEntityType::Polygon) {
            auto p1 = std::dynamic_pointer_cast<SketchPolygon>(m_entity1);
            auto p2 = std::dynamic_pointer_cast<SketchPolygon>(m_entity2);
            if (p1 && p2) {
                gp_Pnt2d mc = mirrorPoint(p1->getCenter());
                p2->setCenter(mc);
                p2->setRadius(p1->getRadius());
                p2->setSides(p1->getSides());
                return;
            }
        }
        
        // Cas spécial : Oblong → miroir du centre + copie dimensions
        if (m_entity1 && m_entity2 &&
            m_entity1->getType() == SketchEntityType::Oblong &&
            m_entity2->getType() == SketchEntityType::Oblong) {
            auto o1 = std::dynamic_pointer_cast<SketchOblong>(m_entity1);
            auto o2 = std::dynamic_pointer_cast<SketchOblong>(m_entity2);
            if (o1 && o2) {
                gp_Pnt2d mc = mirrorPoint(o1->getCenter());
                o2->setCenter(mc);
                o2->setLength(o1->getLength());
                o2->setWidth(o1->getWidth());
                return;
            }
        }
        
        // Cas général : Point1 est le "maître", point2 est ajusté
        gp_Pnt2d p1 = readPoint(m_entity1, m_vtx1);
        gp_Pnt2d mirrored = mirrorPoint(p1);
        writePoint(m_entity2, m_vtx2, mirrored);
    }
    
    gp_Pnt2d getPoint1() const { return readPoint(m_entity1, m_vtx1); }
    gp_Pnt2d getPoint2() const { return readPoint(m_entity2, m_vtx2); }
    
    std::shared_ptr<SketchEntity> getEntity1() const { return m_entity1; }
    std::shared_ptr<SketchEntity> getEntity2() const { return m_entity2; }
    int getVtx1() const { return m_vtx1; }
    int getVtx2() const { return m_vtx2; }
    
    bool referencesEntity(std::shared_ptr<SketchEntity> entity) const {
        return m_entity1 == entity || m_entity2 == entity ||
               (m_axisLine && m_axisLine == std::dynamic_pointer_cast<SketchLine>(entity)) ||
               (m_axisPoly && m_axisPoly == std::dynamic_pointer_cast<SketchPolyline>(entity));
    }
    
private:
    std::shared_ptr<SketchEntity> m_entity1;
    int m_vtx1;
    std::shared_ptr<SketchEntity> m_entity2;
    int m_vtx2;
    std::shared_ptr<SketchLine> m_axisLine;
    std::shared_ptr<SketchPolyline> m_axisPoly;
    int m_axisSegIdx = -1;
};

// ============================================================================
// CenterOnAxisConstraint - Entité centrée sur un axe (symétrie persistante)
// ============================================================================

class CenterOnAxisConstraint : public GeometricConstraint {
public:
    CenterOnAxisConstraint(std::shared_ptr<SketchEntity> entity,
                           std::shared_ptr<SketchLine> axisLine)
        : m_entity(entity), m_axisLine(axisLine)
    {
    }
    
    ConstraintType getType() const override { return ConstraintType::CenterOnAxis; }
    std::string getDescription() const override { return "CenterOnAxis"; }
    std::string getSymbol() const override { return "\xE2\x8E\x85"; }  // ⎅ symmetry
    int getPriority() const override { return 2; }
    
    // Centre de l'entité
    gp_Pnt2d getEntityCenter() const {
        if (!m_entity) return gp_Pnt2d(0, 0);
        auto t = m_entity->getType();
        if (t == SketchEntityType::Rectangle) {
            auto r = std::dynamic_pointer_cast<SketchRectangle>(m_entity);
            if (r) return gp_Pnt2d(r->getCorner().X()+r->getWidth()/2, r->getCorner().Y()+r->getHeight()/2);
        } else if (t == SketchEntityType::Circle) {
            auto c = std::dynamic_pointer_cast<SketchCircle>(m_entity);
            if (c) return c->getCenter();
        } else if (t == SketchEntityType::Ellipse) {
            auto e = std::dynamic_pointer_cast<SketchEllipse>(m_entity);
            if (e) return e->getCenter();
        } else if (t == SketchEntityType::Polygon) {
            auto p = std::dynamic_pointer_cast<SketchPolygon>(m_entity);
            if (p) return p->getCenter();
        } else if (t == SketchEntityType::Oblong) {
            auto o = std::dynamic_pointer_cast<SketchOblong>(m_entity);
            if (o) return o->getCenter();
        } else if (t == SketchEntityType::Line) {
            auto l = std::dynamic_pointer_cast<SketchLine>(m_entity);
            if (l) return gp_Pnt2d((l->getStart().X()+l->getEnd().X())/2,
                                    (l->getStart().Y()+l->getEnd().Y())/2);
        } else if (t == SketchEntityType::Polyline) {
            auto pl = std::dynamic_pointer_cast<SketchPolyline>(m_entity);
            if (pl) {
                auto pts = pl->getPoints();
                double cx=0, cy=0;
                for (const auto& p : pts) { cx+=p.X(); cy+=p.Y(); }
                if (!pts.empty()) return gp_Pnt2d(cx/pts.size(), cy/pts.size());
            }
        }
        return gp_Pnt2d(0, 0);
    }
    
    // Projection du centre sur l'axe
    gp_Pnt2d projectOnAxis(const gp_Pnt2d& center) const {
        if (!m_axisLine) return center;
        gp_Pnt2d a = m_axisLine->getStart(), b = m_axisLine->getEnd();
        double dx = b.X()-a.X(), dy = b.Y()-a.Y();
        double len2 = dx*dx+dy*dy;
        if (len2 < 1e-12) return center;
        double t = ((center.X()-a.X())*dx + (center.Y()-a.Y())*dy) / len2;
        return gp_Pnt2d(a.X()+t*dx, a.Y()+t*dy);
    }
    
    bool isSatisfied(double tolerance = 1e-6) const override {
        gp_Pnt2d center = getEntityCenter();
        gp_Pnt2d proj = projectOnAxis(center);
        return center.Distance(proj) < tolerance;
    }
    
    void apply() override {
        if (!m_entity || !m_axisLine) return;
        gp_Pnt2d center = getEntityCenter();
        gp_Pnt2d proj = projectOnAxis(center);
        double dx = proj.X()-center.X(), dy = proj.Y()-center.Y();
        if (std::abs(dx) < 1e-9 && std::abs(dy) < 1e-9) return;
        
        auto t = m_entity->getType();
        if (t == SketchEntityType::Rectangle) {
            auto r = std::dynamic_pointer_cast<SketchRectangle>(m_entity);
            if (r) r->setCorner(gp_Pnt2d(r->getCorner().X()+dx, r->getCorner().Y()+dy));
        } else if (t == SketchEntityType::Circle) {
            auto c = std::dynamic_pointer_cast<SketchCircle>(m_entity);
            if (c) c->setCenter(gp_Pnt2d(c->getCenter().X()+dx, c->getCenter().Y()+dy));
        } else if (t == SketchEntityType::Ellipse) {
            auto e = std::dynamic_pointer_cast<SketchEllipse>(m_entity);
            if (e) e->setCenter(gp_Pnt2d(e->getCenter().X()+dx, e->getCenter().Y()+dy));
        } else if (t == SketchEntityType::Polygon) {
            auto p = std::dynamic_pointer_cast<SketchPolygon>(m_entity);
            if (p) p->setCenter(gp_Pnt2d(p->getCenter().X()+dx, p->getCenter().Y()+dy));
        } else if (t == SketchEntityType::Oblong) {
            auto o = std::dynamic_pointer_cast<SketchOblong>(m_entity);
            if (o) o->setCenter(gp_Pnt2d(o->getCenter().X()+dx, o->getCenter().Y()+dy));
        } else if (t == SketchEntityType::Line) {
            auto l = std::dynamic_pointer_cast<SketchLine>(m_entity);
            if (l) {
                l->setStart(gp_Pnt2d(l->getStart().X()+dx, l->getStart().Y()+dy));
                l->setEnd(gp_Pnt2d(l->getEnd().X()+dx, l->getEnd().Y()+dy));
            }
        } else if (t == SketchEntityType::Polyline) {
            auto pl = std::dynamic_pointer_cast<SketchPolyline>(m_entity);
            if (pl) {
                auto pts = pl->getPoints();
                for (auto& p : pts) p = gp_Pnt2d(p.X()+dx, p.Y()+dy);
                pl->setPoints(pts);
            }
        }
    }
    
    // Pour l'affichage du symbole
    gp_Pnt2d getSymbolPosition() const { return getEntityCenter(); }
    std::shared_ptr<SketchEntity> getEntity() const { return m_entity; }
    std::shared_ptr<SketchLine> getAxisLine() const { return m_axisLine; }
    
    bool referencesEntity(std::shared_ptr<SketchEntity> entity) const {
        return m_entity == entity ||
               (m_axisLine && m_axisLine == std::dynamic_pointer_cast<SketchLine>(entity));
    }
    
private:
    std::shared_ptr<SketchEntity> m_entity;
    std::shared_ptr<SketchLine> m_axisLine;
};

} // namespace CADEngine

#endif // BASICCONSTRAINTS_H

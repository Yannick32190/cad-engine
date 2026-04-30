#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H

#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QFont>
#include <cmath>

/**
 * @brief Génère des icônes vectorielles style FreeCAD pour les outils sketch
 * 
 * Toutes les icônes sont dessinées via QPainter pour un rendu net à toute taille.
 * Deux variantes : clair (stroke foncé) et sombre (stroke clair).
 */
class IconProvider {
public:
    static constexpr int SIZE = 24;
    
    // Couleurs adaptées au thème
    struct Colors {
        QColor stroke;      // Trait principal
        QColor fill;        // Remplissage
        QColor accent;      // Accent (bleu/orange)
        QColor accent2;     // Accent secondaire
        QColor bg;          // Fond transparent ou léger
        QColor dimStroke;   // Trait atténué
    };
    
    static Colors lightColors() {
        return { QColor(40, 40, 40), QColor(220, 235, 255), QColor(30, 120, 220),
                 QColor(220, 80, 30), QColor(0,0,0,0), QColor(140, 140, 140) };
    }
    
    static Colors darkColors() {
        return { QColor(220, 225, 230), QColor(60, 70, 85), QColor(80, 170, 255),
                 QColor(255, 140, 60), QColor(0,0,0,0), QColor(130, 135, 140) };
    }
    
    // ================================================================
    // OUTILS DESSIN
    // ================================================================
    
    static QIcon selectIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        QPen pen(c.stroke, 2.0); p.setPen(pen);
        // Flèche curseur
        QPolygonF arrow;
        arrow << QPointF(5, 3) << QPointF(5, 19) << QPointF(10, 15) 
              << QPointF(14, 20) << QPointF(17, 18) << QPointF(13, 13) << QPointF(18, 11);
        p.setBrush(QColor(255,255,255,200));
        p.drawPolygon(arrow);
        return QIcon(pm);
    }
    
    static QIcon lineIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(c.accent, 2.2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(4, 20, 20, 4);
        // Points aux extrémités
        p.setPen(Qt::NoPen); p.setBrush(c.accent2);
        p.drawEllipse(QPointF(4, 20), 2.5, 2.5);
        p.drawEllipse(QPointF(20, 4), 2.5, 2.5);
        return QIcon(pm);
    }
    
    static QIcon circleIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(c.accent, 2.2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(3, 3, 18, 18));
        // Centre
        p.setPen(Qt::NoPen); p.setBrush(c.accent2);
        p.drawEllipse(QPointF(12, 12), 2, 2);
        // Rayon en pointillé
        p.setPen(QPen(c.dimStroke, 1.0, Qt::DashLine));
        p.drawLine(12, 12, 21, 12);
        return QIcon(pm);
    }
    
    static QIcon rectangleIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(c.accent, 2.2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        p.setBrush(QColor(c.fill.red(), c.fill.green(), c.fill.blue(), 60));
        p.drawRect(QRectF(3, 5, 18, 14));
        return QIcon(pm);
    }
    
    static QIcon arcIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(c.accent, 2.2, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        // Arc de 90° (de 180° à 270° dans le système Qt)
        p.drawArc(QRectF(1, 1, 22, 22), 30 * 16, 120 * 16);
        // Points extrémités
        double cx = 12, cy = 12, r = 11;
        p.setPen(Qt::NoPen); p.setBrush(c.accent2);
        p.drawEllipse(QPointF(cx + r*cos(30*M_PI/180), cy - r*sin(30*M_PI/180)), 2.5, 2.5);
        p.drawEllipse(QPointF(cx + r*cos(150*M_PI/180), cy - r*sin(150*M_PI/180)), 2.5, 2.5);
        return QIcon(pm);
    }
    
    static QIcon polylineIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(c.accent, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPolygonF poly;
        poly << QPointF(3, 20) << QPointF(8, 7) << QPointF(15, 16) << QPointF(21, 4);
        p.drawPolyline(poly);
        // Vertices
        p.setPen(Qt::NoPen); p.setBrush(c.accent2);
        for (const auto& pt : poly) p.drawEllipse(pt, 2.5, 2.5);
        return QIcon(pm);
    }
    
    static QIcon dimensionIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Ligne cotation
        p.setPen(QPen(c.accent, 1.5));
        p.drawLine(4, 12, 20, 12);
        // Flèches
        QPolygonF arL, arR;
        arL << QPointF(4, 12) << QPointF(8, 10) << QPointF(8, 14);
        arR << QPointF(20, 12) << QPointF(16, 10) << QPointF(16, 14);
        p.setBrush(c.accent); p.setPen(Qt::NoPen);
        p.drawPolygon(arL); p.drawPolygon(arR);
        // Extension lines
        p.setPen(QPen(c.dimStroke, 1.0));
        p.drawLine(4, 6, 4, 18);
        p.drawLine(20, 6, 20, 18);
        // Texte "D"
        p.setPen(c.stroke);
        QFont f; f.setPixelSize(9); f.setBold(true); p.setFont(f);
        p.drawText(QRectF(8, 2, 8, 10), Qt::AlignCenter, "D");
        return QIcon(pm);
    }
    
    // ================================================================
    // CONTRAINTES
    // ================================================================
    
    static QIcon constraintHIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Ligne horizontale
        p.setPen(QPen(QColor(0, 160, 0), 2.5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(3, 14, 21, 14);
        // "H"
        p.setPen(QColor(0, 140, 0));
        QFont f; f.setPixelSize(11); f.setBold(true); p.setFont(f);
        p.drawText(QRectF(0, 0, SIZE, 12), Qt::AlignCenter, "H");
        return QIcon(pm);
    }
    
    static QIcon constraintVIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Ligne verticale
        p.setPen(QPen(QColor(0, 160, 0), 2.5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(12, 3, 12, 21);
        // "V"
        p.setPen(QColor(0, 140, 0));
        QFont f; f.setPixelSize(10); f.setBold(true); p.setFont(f);
        p.drawText(QRectF(14, 1, 10, 12), Qt::AlignLeft, "V");
        return QIcon(pm);
    }
    
    static QIcon constraintParallelIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(180, 0, 180), 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(4, 6, 20, 6);
        p.drawLine(4, 14, 20, 14);
        // Symbole "//"
        p.setPen(QPen(QColor(180, 0, 180), 1.5));
        p.drawLine(10, 8, 12, 12);
        p.drawLine(12, 8, 14, 12);
        return QIcon(pm);
    }
    
    static QIcon constraintPerpIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(180, 0, 180), 2.0, Qt::SolidLine, Qt::RoundCap));
        // L inversé (angle droit)
        p.drawLine(6, 4, 6, 20);
        p.drawLine(6, 20, 20, 20);
        // Carré angle droit
        p.setPen(QPen(QColor(180, 0, 180), 1.2));
        p.drawLine(6, 15, 11, 15);
        p.drawLine(11, 15, 11, 20);
        return QIcon(pm);
    }
    
    static QIcon constraintCoincidentIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Deux lignes convergentes
        p.setPen(QPen(c.dimStroke, 1.5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(3, 4, 12, 12);
        p.drawLine(21, 4, 12, 12);
        p.drawLine(12, 12, 12, 21);
        // Point coïncident (gros point vert)
        p.setPen(Qt::NoPen); p.setBrush(QColor(0, 200, 0));
        p.drawEllipse(QPointF(12, 12), 4, 4);
        return QIcon(pm);
    }
    
    static QIcon constraintConcentricIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(180, 0, 180), 1.8)); p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(3, 3, 18, 18));
        p.drawEllipse(QRectF(7, 7, 10, 10));
        // Centre
        p.setPen(Qt::NoPen); p.setBrush(QColor(180, 0, 180));
        p.drawEllipse(QPointF(12, 12), 2, 2);
        return QIcon(pm);
    }
    
    static QIcon constraintSymmetricIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Axe de symétrie (trait-point)
        p.setPen(QPen(QColor(140, 0, 200), 1.2, Qt::DashDotLine));
        p.drawLine(12, 2, 12, 22);
        // Deux points symétriques
        p.setPen(Qt::NoPen); p.setBrush(QColor(140, 0, 200));
        p.drawEllipse(QPointF(5, 10), 3, 3);
        p.drawEllipse(QPointF(19, 10), 3, 3);
        // Flèche double
        p.setPen(QPen(QColor(140, 0, 200), 1.5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(5, 10, 19, 10);
        return QIcon(pm);
    }
    
    static QIcon constraintFixIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Cadenas stylisé
        p.setPen(QPen(QColor(200, 50, 50), 2.0));
        // Anse
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(7, 2, 10, 10), 0, 180 * 16);
        // Corps
        p.setBrush(QColor(200, 50, 50));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(5, 10, 14, 11), 2, 2);
        // Trou de clé
        p.setBrush(c.bg.alpha() > 0 ? c.bg : QColor(255,255,255));
        p.drawEllipse(QPointF(12, 14), 2, 2);
        p.drawRect(QRectF(11, 14, 2, 4));
        return QIcon(pm);
    }
    
    static QIcon filletIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Coin droit (pointillé = avant)
        p.setPen(QPen(c.dimStroke, 1.2, Qt::DotLine));
        p.drawLine(5, 20, 5, 5);
        p.drawLine(5, 20, 20, 20);
        // Lignes avec congé (après)
        p.setPen(QPen(c.accent, 2.2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(5, 5, 5, 12);
        // Arc du congé
        QPainterPath path;
        path.moveTo(5, 12);
        path.quadTo(5, 20, 12, 20);
        p.drawPath(path);
        p.drawLine(12, 20, 20, 20);
        // "R" 
        p.setPen(c.accent2);
        QFont f; f.setPixelSize(9); f.setBold(true); p.setFont(f);
        p.drawText(QRectF(13, 5, 10, 10), Qt::AlignCenter, "R");
        return QIcon(pm);
    }
    
    static QIcon ellipseIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Ellipse
        p.setPen(QPen(c.accent, 2.0));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QRectF(3, 7, 18, 10));
        // Centre
        p.setPen(QPen(c.accent2, 1.5));
        p.drawLine(10, 12, 14, 12);
        p.drawLine(12, 10, 12, 14);
        // Axes pointillés
        p.setPen(QPen(c.dimStroke, 0.8, Qt::DotLine));
        p.drawLine(3, 12, 21, 12);
        p.drawLine(12, 7, 12, 17);
        return QIcon(pm);
    }
    
    static QIcon polygonIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Hexagone
        p.setPen(QPen(c.accent, 2.0));
        p.setBrush(Qt::NoBrush);
        QPolygonF hex;
        for (int i = 0; i < 6; i++) {
            double angle = M_PI / 6.0 + 2.0 * M_PI * i / 6.0;
            hex << QPointF(12 + 9 * std::cos(angle), 12 + 9 * std::sin(angle));
        }
        p.drawPolygon(hex);
        // Centre
        p.setPen(QPen(c.accent2, 1.5));
        p.drawLine(10, 12, 14, 12);
        p.drawLine(12, 10, 12, 14);
        return QIcon(pm);
    }
    
    static QIcon arcCenterIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Arc
        p.setPen(QPen(c.accent, 2.2));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(2, 2, 18, 18), 30*16, 120*16);
        // Centre + rayon pointillé
        p.setPen(QPen(c.dimStroke, 1.0, Qt::DotLine));
        p.drawLine(11, 11, 18, 5);
        // Point au centre
        p.setPen(Qt::NoPen);
        p.setBrush(c.accent2);
        p.drawEllipse(QPointF(11, 11), 2.5, 2.5);
        // Extrémités de l'arc
        p.setBrush(c.accent);
        p.drawEllipse(QPointF(18, 5), 2, 2);
        p.drawEllipse(QPointF(4, 5), 2, 2);
        return QIcon(pm);
    }
    
    static QIcon oblongIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Oblong = rectangle avec bouts arrondis
        p.setPen(QPen(c.accent, 2.0));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(2, 7, 20, 10), 5, 5);
        // Croix au centre
        p.setPen(QPen(c.dimStroke, 1.0));
        p.drawLine(10, 12, 14, 12);
        p.drawLine(12, 10, 12, 14);
        return QIcon(pm);
    }
    
    static QIcon finishSketchIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Checkmark vert
        p.setPen(QPen(QColor(30, 180, 30), 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawLine(4, 13, 9, 19);
        p.drawLine(9, 19, 20, 5);
        return QIcon(pm);
    }
    
    static QIcon sketchIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Grille stylisée
        p.setPen(QPen(c.dimStroke, 0.8));
        for (int i = 0; i < 4; i++) {
            int x = 4 + i * 5;
            p.drawLine(x, 4, x, 19);
            p.drawLine(4, x, 19, x);
        }
        // Crayon en overlay
        p.setPen(QPen(c.accent, 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(8, 16, 18, 6);
        p.setPen(Qt::NoPen); p.setBrush(c.accent2);
        p.drawEllipse(QPointF(18, 6), 2, 2);
        return QIcon(pm);
    }
    
    static QIcon toggleTreeIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(c.stroke, 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(4, 6, 20, 6);
        p.drawLine(4, 12, 20, 12);
        p.drawLine(4, 18, 20, 18);
        return QIcon(pm);
    }
    
    static QIcon undoIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(c.stroke, 2.0, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(5, 6, 14, 14), 90*16, 180*16);
        // Flèche
        QPolygonF ar; ar << QPointF(5, 12) << QPointF(9, 8) << QPointF(9, 16);
        p.setBrush(c.stroke); p.setPen(Qt::NoPen); p.drawPolygon(ar);
        return QIcon(pm);
    }
    
    static QIcon redoIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(c.stroke, 2.0, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(5, 6, 14, 14), 270*16, 180*16);
        QPolygonF ar; ar << QPointF(19, 12) << QPointF(15, 8) << QPointF(15, 16);
        p.setBrush(c.stroke); p.setPen(Qt::NoPen); p.drawPolygon(ar);
        return QIcon(pm);
    }
    
    // ================================================================
    // OPÉRATIONS 3D
    // ================================================================
    
    static QIcon extrudeIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Base rectangle
        p.setPen(QPen(c.accent, 1.8));
        p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 40));
        QPolygonF base; 
        base << QPointF(4, 18) << QPointF(14, 20) << QPointF(20, 16) << QPointF(10, 14);
        p.drawPolygon(base);
        // Vertical edges (extrusion)
        p.setPen(QPen(c.accent, 1.5));
        p.drawLine(QPointF(4, 18), QPointF(4, 8));
        p.drawLine(QPointF(14, 20), QPointF(14, 10));
        p.drawLine(QPointF(20, 16), QPointF(20, 6));
        // Top face
        p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 80));
        QPolygonF top;
        top << QPointF(4, 8) << QPointF(14, 10) << QPointF(20, 6) << QPointF(10, 4);
        p.drawPolygon(top);
        // Arrow up
        p.setPen(QPen(c.accent2, 2.2, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(12, 16), QPointF(12, 3));
        QPolygonF arw; arw << QPointF(12, 1) << QPointF(9, 5) << QPointF(15, 5);
        p.setBrush(c.accent2); p.setPen(Qt::NoPen); p.drawPolygon(arw);
        return QIcon(pm);
    }
    
    static QIcon revolveIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Axe de révolution (trait-point vertical à gauche)
        p.setPen(QPen(c.accent2, 1.5, Qt::DashDotLine));
        p.drawLine(5, 1, 5, 23);
        // Profil source (L-shape à droite de l'axe)
        p.setPen(QPen(c.stroke, 1.8));
        p.drawLine(5, 5, 14, 5);
        p.drawLine(14, 5, 14, 14);
        p.drawLine(14, 14, 8, 14);
        p.drawLine(8, 14, 8, 19);
        p.drawLine(8, 19, 5, 19);
        // Flèche circulaire de révolution (3/4 de tour)
        p.setPen(QPen(c.accent, 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(QRectF(12, 14, 10, 10), 90*16, -270*16);
        // Pointe de flèche
        QPolygonF arr;
        arr << QPointF(17, 14) << QPointF(15, 17) << QPointF(19, 17);
        p.setBrush(c.accent); p.setPen(Qt::NoPen);
        p.drawPolygon(arr);
        return QIcon(pm);
    }
    
    static QIcon fillet3DIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        
        // Profil en L : deux arêtes formant un angle droit
        // Arête verticale (haut → coin)
        p.setPen(QPen(c.stroke, 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(6, 2), QPointF(6, 11));
        // Arête horizontale (coin → droite)  
        p.drawLine(QPointF(15, 20), QPointF(22, 20));
        
        // Arc de congé (du bas de la verticale au début de l'horizontale)
        // Rouge vif bien visible
        p.setPen(QPen(QColor(220, 40, 40), 2.5, Qt::SolidLine, Qt::RoundCap));
        QPainterPath arcPath;
        arcPath.moveTo(6, 11);
        arcPath.cubicTo(6, 17, 8, 20, 15, 20);
        p.drawPath(arcPath);
        
        // Petite flèche/indicateur "R" 
        QFont f; f.setPixelSize(9); f.setBold(true); p.setFont(f);
        p.setPen(QColor(220, 40, 40));
        p.drawText(QRectF(13, 4, 10, 12), Qt::AlignCenter, "R");
        
        return QIcon(pm);
    }
    
    static QIcon chamfer3DIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        
        // Profil en L : deux arêtes formant un angle droit
        // Arête verticale (haut → point de coupe)
        p.setPen(QPen(c.stroke, 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(6, 2), QPointF(6, 11));
        // Arête horizontale (point de coupe → droite)
        p.drawLine(QPointF(15, 20), QPointF(22, 20));
        
        // Ligne de chanfrein (diagonale reliant les deux arêtes)
        // Bleu vif bien visible
        p.setPen(QPen(QColor(30, 120, 220), 2.5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(6, 11), QPointF(15, 20));
        
        // Label "C"
        QFont f; f.setPixelSize(9); f.setBold(true); p.setFont(f);
        p.setPen(QColor(30, 120, 220));
        p.drawText(QRectF(13, 4, 10, 12), Qt::AlignCenter, "C");
        
        return QIcon(pm);
    }
    
    // ── Sweep (extrusion le long d'un chemin) ──
    static QIcon sweepIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Profil cercle (section)
        p.setPen(QPen(c.accent, 2.0));
        p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 50));
        p.drawEllipse(QPointF(5, 17), 4, 4);
        // Chemin courbe (spline)
        p.setPen(QPen(c.stroke, 1.8, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        QPainterPath swPath;
        swPath.moveTo(5, 17);
        swPath.cubicTo(5, 8, 12, 4, 20, 6);
        p.drawPath(swPath);
        // Profil fantôme à l'arrivée
        p.setPen(QPen(c.accent, 1.5, Qt::DashLine));
        p.drawEllipse(QPointF(20, 6), 3, 3);
        // Flèche direction
        QPolygonF swArr;
        swArr << QPointF(20, 3) << QPointF(22, 6) << QPointF(18, 6);
        p.setBrush(c.stroke); p.setPen(Qt::NoPen);
        p.drawPolygon(swArr);
        return QIcon(pm);
    }
    
    // ── Loft (extrusion entre deux profils) ──
    static QIcon loftIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Profil bas (rectangle)
        p.setPen(QPen(c.accent, 2.0));
        p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 40));
        p.drawRect(2, 17, 12, 5);
        // Profil haut (cercle)
        p.drawEllipse(QPointF(12, 5), 5, 4);
        // Lignes de transition
        p.setPen(QPen(c.stroke, 1.2, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawLine(QPointF(2, 17), QPointF(7, 5));
        p.drawLine(QPointF(14, 17), QPointF(17, 5));
        // Ligne guide pointillée
        p.setPen(QPen(c.dimStroke, 0.8, Qt::DotLine));
        p.drawLine(QPointF(8, 17), QPointF(12, 5));
        return QIcon(pm);
    }
    
    // ── Réseau linéaire ──
    static QIcon linearPatternIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Forme source
        p.setPen(QPen(c.stroke, 1.4));
        p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 80));
        p.drawRect(1, 13, 6, 6);
        // Copies
        p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 40));
        p.setPen(QPen(c.stroke, 1.0));
        p.drawRect(9, 13, 6, 6);
        p.drawRect(17, 13, 6, 6);
        // Flèche direction
        p.setPen(QPen(c.accent, 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(2, 8, 20, 8);
        QPolygonF lpArr;
        lpArr << QPointF(18, 5) << QPointF(22, 8) << QPointF(18, 11);
        p.setBrush(c.accent); p.setPen(Qt::NoPen);
        p.drawPolygon(lpArr);
        return QIcon(pm);
    }
    
    // ── Réseau circulaire ──
    static QIcon circularPatternIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Cercle guide
        p.setPen(QPen(c.dimStroke, 1.0, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(12, 12), 9, 9);
        // Point central
        p.setPen(Qt::NoPen);
        p.setBrush(c.accent2);
        p.drawEllipse(QPointF(12, 12), 2, 2);
        // Copies en cercle
        p.setPen(QPen(c.stroke, 1.2));
        for (int i = 0; i < 6; i++) {
            double a = i * 60.0 * M_PI / 180.0 - M_PI / 2.0;
            double x = 12.0 + 9.0 * cos(a) - 2.0;
            double y = 12.0 + 9.0 * sin(a) - 2.0;
            if (i == 0)
                p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 120));
            else
                p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 50));
            p.drawRect(QRectF(x, y, 4, 4));
        }
        return QIcon(pm);
    }
    
    // ── Filetage (Thread) ──
    static QIcon threadIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // Cylindre
        p.setPen(QPen(c.stroke, 1.4));
        p.drawLine(6, 4, 6, 20);
        p.drawLine(18, 4, 18, 20);
        // Ellipse haut
        p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 30));
        p.drawEllipse(QRectF(6, 1, 12, 6));
        // Ellipse bas
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(6, 17, 12, 6), 0, -180 * 16);
        // Filets hélicoïdaux
        p.setPen(QPen(c.accent, 1.8, Qt::SolidLine, Qt::RoundCap));
        for (int i = 0; i < 5; i++) {
            double y = 6.0 + i * 3.0;
            p.drawLine(QPointF(6, y), QPointF(18, y + 1.5));
            p.drawLine(QPointF(18, y + 1.5), QPointF(6, y + 3.0));
        }
        return QIcon(pm);
    }
    
    static QIcon sketchOnFaceIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        // 3D box face
        p.setPen(QPen(c.stroke, 1.2));
        p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 40));
        QPolygonF face; 
        face << QPointF(2, 8) << QPointF(12, 4) << QPointF(22, 8) << QPointF(12, 12);
        p.drawPolygon(face);
        // Vertical edges
        p.drawLine(QPointF(2, 8), QPointF(2, 18));
        p.drawLine(QPointF(22, 8), QPointF(22, 18));
        p.drawLine(QPointF(12, 12), QPointF(12, 22));
        // Bottom face lines
        p.drawLine(QPointF(2, 18), QPointF(12, 22));
        p.drawLine(QPointF(22, 18), QPointF(12, 22));
        // Sketch pencil on top face
        p.setPen(QPen(c.accent2, 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(8, 7), QPointF(16, 9));
        p.drawLine(QPointF(9, 9), QPointF(15, 7));
        return QIcon(pm);
    }
    
    // ── Shell (coque / évidement) ──
    static QIcon shellIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        
        // Boîte extérieure (contour solide)
        p.setPen(QPen(c.stroke, 1.8));
        p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 30));
        p.drawRoundedRect(QRectF(2, 2, 20, 20), 2, 2);
        
        // Évidement intérieur (zone creuse)
        p.setPen(QPen(QColor(255, 140, 30), 1.8, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(QColor(255, 140, 30, 25));
        p.drawRoundedRect(QRectF(5, 5, 14, 14), 1, 1);
        
        // Flèches épaisseur (vers l'intérieur)
        p.setPen(QPen(QColor(255, 140, 30), 1.5, Qt::SolidLine, Qt::RoundCap));
        // Flèche haut
        p.drawLine(QPointF(12, 1), QPointF(12, 5));
        p.drawLine(QPointF(10, 3), QPointF(12, 5));
        p.drawLine(QPointF(14, 3), QPointF(12, 5));
        // Flèche bas
        p.drawLine(QPointF(12, 23), QPointF(12, 19));
        p.drawLine(QPointF(10, 21), QPointF(12, 19));
        p.drawLine(QPointF(14, 21), QPointF(12, 19));
        
        return QIcon(pm);
    }
    
    static QIcon pushPullIcon(const Colors& c = lightColors()) {
        QPixmap pm(SIZE, SIZE); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        
        // Face (rectangle horizontal au centre)
        p.setPen(QPen(c.stroke, 1.5));
        p.setBrush(QColor(c.accent.red(), c.accent.green(), c.accent.blue(), 40));
        p.drawRect(QRectF(6, 9, 12, 6));
        
        // Flèche vers le haut (Tirer +)
        p.setPen(QPen(QColor(76, 175, 80), 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(12, 8), QPointF(12, 1));
        p.drawLine(QPointF(9, 4), QPointF(12, 1));
        p.drawLine(QPointF(15, 4), QPointF(12, 1));
        // "+"
        p.setPen(QPen(QColor(76, 175, 80), 1.5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(20, 3), QPointF(20, 7));
        p.drawLine(QPointF(18, 5), QPointF(22, 5));
        
        // Flèche vers le bas (Pousser -)
        p.setPen(QPen(QColor(244, 67, 54), 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(12, 16), QPointF(12, 23));
        p.drawLine(QPointF(9, 20), QPointF(12, 23));
        p.drawLine(QPointF(15, 20), QPointF(12, 23));
        // "−"
        p.setPen(QPen(QColor(244, 67, 54), 1.5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(QPointF(18, 19), QPointF(22, 19));
        
        return QIcon(pm);
    }
};

#endif // ICONPROVIDER_H

#ifndef VIEWCUBE_H
#define VIEWCUBE_H

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>
#include <vector>

/**
 * @brief ViewCube - Widget de navigation 3D style Fusion 360
 * 
 * Affiche un cube 3D interactif en overlay du viewport.
 * Cliquer sur une face, arête ou coin oriente la caméra.
 */
class ViewCube : public QWidget {
    Q_OBJECT

public:
    explicit ViewCube(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_hoveredFace(-1)
        , m_angleX(-30.0f)
        , m_angleZ(45.0f)
    {
        setFixedSize(140, 140);
        setMouseTracking(true);
        setAttribute(Qt::WA_TransparentForMouseEvents, false);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setCursor(Qt::PointingHandCursor);
    }
    
    void setCameraAngles(float angleX, float angleZ) {
        m_angleX = angleX;
        m_angleZ = angleZ;
        update();
    }

signals:
    void viewSelected(float angleX, float angleZ);  // Angles caméra demandés

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        int cx = width() / 2;
        int cy = height() / 2;
        
        // Fond semi-transparent
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 30));
        p.drawEllipse(QPointF(cx, cy), 65, 65);
        
        // Mêmes rotations que la caméra viewport :
        //   glRotatef(angleX, 1, 0, 0);  → pitch
        //   glRotatef(angleY, 0, 0, 1);  → yaw
        double aX = m_angleX * M_PI / 180.0;
        double aZ = m_angleZ * M_PI / 180.0;
        double s = 32.0;
        
        // Projection : même que setupCamera()
        auto project = [&](double x, double y, double z, double& sx, double& sy, double& depth) {
            // Step 1: Rotate around Z by aZ
            double x1 = x * cos(aZ) - y * sin(aZ);
            double y1 = x * sin(aZ) + y * cos(aZ);
            // Step 2: Rotate around X by aX
            double x2 = x1;
            double y2 = y1 * cos(aX) - z * sin(aX);
            double z2 = y1 * sin(aX) + z * cos(aX);
            // Screen: x2 right, y2 up (Qt Y inverted)
            sx = cx + x2 * s;
            sy = cy - y2 * s;
            depth = z2;
        };
        
        // 8 sommets du cube [-1,+1]^3
        struct V3 { double x, y, z; };
        V3 verts[8] = {
            {-1,-1,-1}, {+1,-1,-1}, {+1,+1,-1}, {-1,+1,-1},
            {-1,-1,+1}, {+1,-1,+1}, {+1,+1,+1}, {-1,+1,+1}
        };
        
        QPointF proj[8];
        double depth[8];
        for (int i = 0; i < 8; i++) {
            double sx, sy, d;
            project(verts[i].x, verts[i].y, verts[i].z, sx, sy, d);
            proj[i] = QPointF(sx, sy);
            depth[i] = d;
        }
        
        // 6 faces : indices + labels + couleur
        // Convention Fusion 360 :
        // Haut=Z+, Bas=Z-, Avant=Y-, Arrière=Y+, Droite=X+, Gauche=X-
        struct Face { int v[4]; const char* label; QColor color; };
        Face faces[6] = {
            {{0,1,2,3}, "BAS",      QColor(180,180,200,180)},  // Z-
            {{4,5,6,7}, "HAUT",     QColor(200,220,240,180)},  // Z+
            {{0,1,5,4}, "AVANT",    QColor(180,200,220,180)},  // Y-
            {{2,3,7,6}, "ARRIERE",  QColor(180,200,220,180)},  // Y+
            {{0,3,7,4}, "GAUCHE",   QColor(200,200,210,180)},  // X-
            {{1,2,6,5}, "DROITE",   QColor(200,200,210,180)},  // X+
        };
        
        // Calculer profondeur moyenne de chaque face pour Z-sort
        struct FaceSort { int idx; double z; };
        FaceSort order[6];
        for (int i = 0; i < 6; i++) {
            double avgZ = 0;
            for (int j = 0; j < 4; j++) avgZ += depth[faces[i].v[j]];
            order[i] = {i, avgZ / 4.0};
        }
        // Tri : face la plus loin d'abord (painter's algorithm)
        for (int i = 0; i < 5; i++)
            for (int j = i+1; j < 6; j++)
                if (order[i].z > order[j].z) std::swap(order[i], order[j]);
        
        // Stocker les polygones des 3 faces visibles (les dernières dans l'ordre) pour hit-test
        m_visibleFaces.clear();
        
        for (int k = 0; k < 6; k++) {
            int fi = order[k].idx;
            Face& f = faces[fi];
            
            QPolygonF poly;
            for (int j = 0; j < 4; j++) poly << proj[f.v[j]];
            
            QColor fc = f.color;
            if (fi == m_hoveredFace) {
                fc = QColor(100, 180, 255, 200);
            }
            
            p.setBrush(fc);
            p.setPen(QPen(QColor(60, 70, 80, 200), 1.2));
            p.drawPolygon(poly);
            
            // Label au centre
            QPointF center(0, 0);
            for (int j = 0; j < 4; j++) center += proj[f.v[j]];
            center /= 4.0;
            
            // Vérifier si face visible (normale vers nous)
            QPointF e1 = proj[f.v[1]] - proj[f.v[0]];
            QPointF e2 = proj[f.v[3]] - proj[f.v[0]];
            double cross = e1.x() * e2.y() - e1.y() * e2.x();
            
            if (cross < 0) {  // Face visible (winding CW en Qt coords)
                m_visibleFaces.push_back({fi, poly});
                
                p.setPen(QColor(30, 30, 40, 220));
                QFont font;
                font.setPixelSize(11);
                font.setBold(true);
                p.setFont(font);
                p.drawText(QRectF(center.x() - 30, center.y() - 8, 60, 16), 
                           Qt::AlignCenter, f.label);
            }
        }
        
        // Axes sous le cube
        double axLen = 18.0;
        QPointF axOrigin(cx, cy + 50);
        
        auto projectAxis = [&](double x, double y, double z) -> QPointF {
            double x1 = x * cos(aZ) - y * sin(aZ);
            double y1 = x * sin(aZ) + y * cos(aZ);
            double x2 = x1;
            double y2 = y1 * cos(aX) - z * sin(aX);
            return QPointF(axOrigin.x() + x2 * axLen, axOrigin.y() - y2 * axLen);
        };
        
        // X (rouge)
        QPointF axX = projectAxis(1, 0, 0);
        p.setPen(QPen(QColor(220, 50, 50), 2.0)); p.drawLine(axOrigin, axX);
        p.setPen(QColor(220, 50, 50));
        QFont af; af.setPixelSize(10); af.setBold(true); p.setFont(af);
        p.drawText(axX + QPointF(-4, -4), "X");
        
        // Y (vert)
        QPointF axY = projectAxis(0, 1, 0);
        p.setPen(QPen(QColor(50, 180, 50), 2.0)); p.drawLine(axOrigin, axY);
        p.setPen(QColor(50, 180, 50));
        p.drawText(axY + QPointF(-4, -4), "Y");
        
        // Z (bleu)
        QPointF axZ = projectAxis(0, 0, 1);
        p.setPen(QPen(QColor(50, 100, 220), 2.0)); p.drawLine(axOrigin, axZ);
        p.setPen(QColor(50, 100, 220));
        p.drawText(axZ + QPointF(-4, -4), "Z");
    }
    
    void mouseMoveEvent(QMouseEvent* event) override {
        int oldHover = m_hoveredFace;
        m_hoveredFace = -1;
        
        QPointF pos = event->position();
        // Hit-test sur les faces visibles (dernier dessiné = premier testé)
        for (int i = (int)m_visibleFaces.size() - 1; i >= 0; i--) {
            if (m_visibleFaces[i].poly.containsPoint(pos, Qt::OddEvenFill)) {
                m_hoveredFace = m_visibleFaces[i].faceIndex;
                break;
            }
        }
        
        if (m_hoveredFace != oldHover) update();
    }
    
    void leaveEvent(QEvent*) override {
        m_hoveredFace = -1;
        update();
    }
    
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && m_hoveredFace >= 0) {
            // Angles caméra {angleX, angleY} pour chaque face
            // Camera: glRotatef(angleX, 1,0,0) puis glRotatef(angleY, 0,0,1)
            // angleX=0,angleY=0 → caméra sur +Z → vue HAUT (plan XY)
            // angleX=-90,angleY=0 → caméra sur -Y → vue AVANT (plan XZ)
            //
            // Face 0=Bas(Z-), 1=Haut(Z+), 2=Avant(Y-), 3=Arrière(Y+), 4=Gauche(X-), 5=Droite(X+)
            float angles[][2] = {
                { 180.0f,   0.0f},  // 0: BAS     → caméra sur -Z, regarde vers +Z
                {   0.0f,   0.0f},  // 1: HAUT    → caméra sur +Z, regarde vers -Z (plan XY)
                { -90.0f,   0.0f},  // 2: AVANT   → caméra sur -Y, regarde vers +Y (plan XZ)
                { -90.0f, 180.0f},  // 3: ARRIERE → caméra sur +Y, regarde vers -Y
                { -90.0f,  90.0f},  // 4: GAUCHE  → caméra sur -X, regarde vers +X (plan YZ)
                { -90.0f, -90.0f},  // 5: DROITE  → caméra sur +X, regarde vers -X
            };
            
            emit viewSelected(angles[m_hoveredFace][0], angles[m_hoveredFace][1]);
        }
    }
    
private:
    struct VisibleFace {
        int faceIndex;
        QPolygonF poly;
    };
    
    std::vector<VisibleFace> m_visibleFaces;
    int m_hoveredFace;
    float m_angleX;
    float m_angleZ;
};

#endif // VIEWCUBE_H

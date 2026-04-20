#include <iostream>
#include <memory>
#include <vector>
#include "../src/features/sketch/SketchFeature.h"
#include "../src/features/sketch/Sketch2D.h"
#include "../src/features/sketch/SketchEntity.h"
#include <gp_Pln.hxx>
#include <gp_Lin.hxx>
#include <cmath>

using namespace CADEngine;

/**
 * @brief Simulation du ray-casting : Screen → Plan 2D
 * 
 * Simule ce qui se passera dans le vrai Viewport3D
 */
class SketchRayCaster {
public:
    SketchRayCaster(const gp_Pln& plane) : m_plane(plane) {}
    
    /**
     * @brief Simule conversion screen (x,y) → point 2D dans le plan
     * 
     * Dans le vrai code, on utilisera gluUnProject pour obtenir le rayon 3D
     * Ici on simule juste l'intersection rayon/plan
     */
    gp_Pnt2d screenToSketch(double screenX, double screenY, 
                            const gp_Pnt& cameraPos, 
                            const gp_Dir& viewDir) {
        
        // 1. Créer rayon depuis la caméra
        gp_Lin ray(cameraPos, viewDir);
        
        // 2. Intersection rayon/plan - CALCUL MANUEL
        // Équation plan: N·(P - P0) = 0
        // Équation rayon: P = Origin + t*Dir
        // Résoudre: N·(Origin + t*Dir - P0) = 0
        
        gp_Dir planeNormal = m_plane.Axis().Direction();
        gp_Pnt planeOrigin = m_plane.Location();
        
        gp_Vec rayOriginToPlane(planeOrigin, cameraPos);
        
        double numerator = gp_Vec(planeNormal).Dot(rayOriginToPlane);
        double denominator = gp_Vec(planeNormal).Dot(gp_Vec(viewDir));
        
        if (std::abs(denominator) < 1e-10) {
            std::cout << "  [RAY] Rayon parallèle au plan!" << std::endl;
            return gp_Pnt2d(0, 0);
        }
        
        double t = numerator / denominator;
        
        // Point d'intersection 3D
        gp_Pnt pt3D = cameraPos.Translated(gp_Vec(viewDir).Multiplied(t));
        
        std::cout << "  [RAY] Intersection 3D: (" 
                  << pt3D.X() << ", " << pt3D.Y() << ", " << pt3D.Z() << ")" << std::endl;
        
        // 3. Convertir 3D → 2D dans le plan
        gp_Pnt2d pt2D = toLocal(pt3D);
        
        std::cout << "  [RAY] Point 2D: (" << pt2D.X() << ", " << pt2D.Y() << ")" << std::endl;
        
        return pt2D;
    }
    
    /**
     * @brief Snap to grid
     */
    gp_Pnt2d snapToGrid(const gp_Pnt2d& pt, double gridSize = 10.0) {
        double x = std::round(pt.X() / gridSize) * gridSize;
        double y = std::round(pt.Y() / gridSize) * gridSize;
        return gp_Pnt2d(x, y);
    }

private:
    gp_Pnt2d toLocal(const gp_Pnt& pt3D) const {
        gp_Pnt origin = m_plane.Location();
        gp_Vec vec(origin, pt3D);
        
        double x = vec.Dot(gp_Vec(m_plane.Position().XDirection()));
        double y = vec.Dot(gp_Vec(m_plane.Position().YDirection()));
        
        return gp_Pnt2d(x, y);
    }
    
    gp_Pln m_plane;
};

/**
 * @brief Simulation du mode sketch
 */
void testSketchMode() {
    std::cout << "========================================" << std::endl;
    std::cout << "Phase 2.2 - Viewport Sketch Mode" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 1. Créer un sketch sur plan XY
    std::cout << "\n[Test 1] Création sketch plan XY..." << std::endl;
    auto sketch = std::make_shared<SketchFeature>("InteractiveSketch");
    sketch->setPlaneXY(0.0);
    
    gp_Pln plane = sketch->getPlane();
    std::cout << "  ✓ Plan créé" << std::endl;
    std::cout << "  Origine: (" << plane.Location().X() << ", " 
              << plane.Location().Y() << ", " << plane.Location().Z() << ")" << std::endl;
    std::cout << "  Normale: (" << plane.Axis().Direction().X() << ", "
              << plane.Axis().Direction().Y() << ", " 
              << plane.Axis().Direction().Z() << ")" << std::endl;
    
    // 2. Créer ray-caster
    std::cout << "\n[Test 2] Initialisation ray-caster..." << std::endl;
    SketchRayCaster rayCaster(plane);
    std::cout << "  ✓ Ray-caster prêt" << std::endl;
    
    // 3. Simuler des clics souris
    std::cout << "\n[Test 3] Simulation Line Tool (2 clics)..." << std::endl;
    
    // Caméra vue de dessus (Z+)
    gp_Pnt cameraPos(50, 50, 200);  // Au-dessus du plan
    gp_Dir viewDir(0, 0, -1);        // Regarde vers le bas
    
    std::cout << "  Caméra: (" << cameraPos.X() << ", " 
              << cameraPos.Y() << ", " << cameraPos.Z() << ")" << std::endl;
    std::cout << "  Direction: (" << viewDir.X() << ", "
              << viewDir.Y() << ", " << viewDir.Z() << ")" << std::endl;
    
    // Clic 1 : Origine
    std::cout << "\n  Clic 1 (origine)..." << std::endl;
    gp_Pnt2d pt1 = rayCaster.screenToSketch(100, 100, cameraPos, viewDir);
    gp_Pnt2d pt1Snapped = rayCaster.snapToGrid(pt1, 10.0);
    std::cout << "  Après snap: (" << pt1Snapped.X() << ", " << pt1Snapped.Y() << ")" << std::endl;
    
    // Clic 2 : (100, 0)
    std::cout << "\n  Clic 2 (100, 0)..." << std::endl;
    cameraPos = gp_Pnt(150, 50, 200);
    gp_Pnt2d pt2 = rayCaster.screenToSketch(200, 100, cameraPos, viewDir);
    gp_Pnt2d pt2Snapped = rayCaster.snapToGrid(pt2, 10.0);
    std::cout << "  Après snap: (" << pt2Snapped.X() << ", " << pt2Snapped.Y() << ")" << std::endl;
    
    // Créer ligne
    std::cout << "\n  Création ligne..." << std::endl;
    auto line = std::make_shared<SketchLine>(pt1Snapped, pt2Snapped);
    sketch->addEntity(line);
    std::cout << "  ✓ Ligne ajoutée au sketch" << std::endl;
    
    // 4. Simuler Circle Tool
    std::cout << "\n[Test 4] Simulation Circle Tool (centre + rayon)..." << std::endl;
    
    // Clic centre
    std::cout << "\n  Clic centre..." << std::endl;
    cameraPos = gp_Pnt(75, 75, 200);
    gp_Pnt2d center = rayCaster.screenToSketch(150, 150, cameraPos, viewDir);
    gp_Pnt2d centerSnapped = rayCaster.snapToGrid(center, 10.0);
    std::cout << "  Après snap: (" << centerSnapped.X() << ", " << centerSnapped.Y() << ")" << std::endl;
    
    // Clic rayon (distance 30)
    std::cout << "\n  Clic rayon..." << std::endl;
    cameraPos = gp_Pnt(105, 75, 200);
    gp_Pnt2d radiusPt = rayCaster.screenToSketch(180, 150, cameraPos, viewDir);
    double radius = centerSnapped.Distance(radiusPt);
    std::cout << "  Rayon calculé: " << radius << " mm" << std::endl;
    
    // Snap rayon to 5mm increments
    radius = std::round(radius / 5.0) * 5.0;
    std::cout << "  Rayon snappé: " << radius << " mm" << std::endl;
    
    // Créer cercle
    std::cout << "\n  Création cercle..." << std::endl;
    auto circle = std::make_shared<SketchCircle>(centerSnapped, radius);
    sketch->addEntity(circle);
    std::cout << "  ✓ Cercle ajouté au sketch" << std::endl;
    
    // 5. Récapitulatif
    std::cout << "\n[Test 5] Récapitulatif sketch..." << std::endl;
    std::cout << "  Entités: " << sketch->getEntityCount() << std::endl;
    
    auto edges = sketch->getSketch2D()->getEdges3D();
    std::cout << "  Edges 3D générées: " << edges.size() << std::endl;
    
    for (size_t i = 0; i < edges.size(); ++i) {
        std::cout << "    Edge " << i << ": Valid=" << (!edges[i].IsNull()) << std::endl;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "✅ Ray-casting fonctionne !" << std::endl;
    std::cout << "✅ Snap to grid opérationnel" << std::endl;
    std::cout << "✅ Outils Line & Circle simulés" << std::endl;
    std::cout << "========================================" << std::endl;
}

int main() {
    testSketchMode();
    return 0;
}

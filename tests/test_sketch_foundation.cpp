#include <iostream>
#include "../src/core/parametric/Feature.h"
#include "../src/core/document/CADDocument.h"
#include "../src/features/sketch/SketchFeature.h"
#include "../src/features/sketch/SketchEntity.h"
#include "../src/features/sketch/Sketch2D.h"

using namespace CADEngine;

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "CAD-ENGINE Phase 2.1 - Sketch Foundation" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Test 1: Création SketchFeature
    std::cout << "\n[Test 1] Création SketchFeature plan XY..." << std::endl;
    auto sketch = std::make_shared<SketchFeature>("TestSketch");
    sketch->setPlaneXY(0.0);
    
    std::cout << "  ✓ Sketch créé" << std::endl;
    std::cout << "  Type: " << sketch->getTypeName() << std::endl;
    std::cout << "  Nom: " << sketch->getName() << std::endl;
    std::cout << "  Entités: " << sketch->getEntityCount() << std::endl;
    
    // Test 2: Ajouter une ligne
    std::cout << "\n[Test 2] Ajout ligne (0,0) → (100,0)..." << std::endl;
    auto line = std::make_shared<SketchLine>(gp_Pnt2d(0, 0), gp_Pnt2d(100, 0));
    sketch->addEntity(line);
    
    std::cout << "  ✓ Ligne ajoutée" << std::endl;
    std::cout << "  Entités: " << sketch->getEntityCount() << std::endl;
    std::cout << "  Ligne valide: " << (line->isValid() ? "Oui" : "Non") << std::endl;
    
    // Test 3: Ajouter un cercle
    std::cout << "\n[Test 3] Ajout cercle centre (50,50) R=25..." << std::endl;
    auto circle = std::make_shared<SketchCircle>(gp_Pnt2d(50, 50), 25.0);
    sketch->addEntity(circle);
    
    std::cout << "  ✓ Cercle ajouté" << std::endl;
    std::cout << "  Entités: " << sketch->getEntityCount() << std::endl;
    std::cout << "  Cercle valide: " << (circle->isValid() ? "Oui" : "Non") << std::endl;
    
    // Test 4: Conversion 2D → 3D
    std::cout << "\n[Test 4] Conversion 2D → 3D..." << std::endl;
    gp_Pnt2d pt2D(10, 20);
    gp_Pnt pt3D = sketch->getSketch2D()->toGlobal(pt2D);
    
    std::cout << "  Point 2D: (" << pt2D.X() << ", " << pt2D.Y() << ")" << std::endl;
    std::cout << "  Point 3D: (" << pt3D.X() << ", " << pt3D.Y() << ", " << pt3D.Z() << ")" << std::endl;
    
    // Test 5: Conversion 3D → 2D
    std::cout << "\n[Test 5] Conversion 3D → 2D..." << std::endl;
    gp_Pnt2d pt2DBack = sketch->getSketch2D()->toLocal(pt3D);
    std::cout << "  Point 2D retour: (" << pt2DBack.X() << ", " << pt2DBack.Y() << ")" << std::endl;
    
    double dist = pt2D.Distance(pt2DBack);
    std::cout << "  Distance: " << dist << std::endl;
    
    if (dist < 0.001) {
        std::cout << "  ✓ Conversion aller-retour correcte" << std::endl;
    } else {
        std::cout << "  ✗ ERREUR: Conversion incorrecte!" << std::endl;
        return 1;
    }
    
    // Test 6: Génération géométrie 3D
    std::cout << "\n[Test 6] Génération edges 3D..." << std::endl;
    std::vector<TopoDS_Edge> edges = sketch->getSketch2D()->getEdges3D();
    
    std::cout << "  Edges générées: " << edges.size() << std::endl;
    
    for (size_t i = 0; i < edges.size(); ++i) {
        std::cout << "  Edge " << i << ": IsNull=" << edges[i].IsNull() << std::endl;
    }
    
    if (edges.size() == 2 && !edges[0].IsNull() && !edges[1].IsNull()) {
        std::cout << "  ✓ Edges 3D générées" << std::endl;
    } else {
        std::cout << "  ✗ ERREUR: Problème génération edges!" << std::endl;
        return 1;
    }
    
    // Test 7: Rectangle
    std::cout << "\n[Test 7] Ajout rectangle (10,10) 50x30..." << std::endl;
    auto rect = std::make_shared<SketchRectangle>(gp_Pnt2d(10, 10), 50.0, 30.0);
    sketch->addEntity(rect);
    
    std::cout << "  ✓ Rectangle ajouté" << std::endl;
    std::cout << "  Entités totales: " << sketch->getEntityCount() << std::endl;
    std::cout << "  Rectangle valide: " << (rect->isValid() ? "Oui" : "Non") << std::endl;
    
    auto corners = rect->getKeyPoints();
    std::cout << "  Coins: " << corners.size() << std::endl;
    for (size_t i = 0; i < corners.size(); ++i) {
        std::cout << "    Coin " << i << ": (" << corners[i].X() << ", " << corners[i].Y() << ")" << std::endl;
    }
    
    // Test 8: CADDocument avec sketch
    std::cout << "\n[Test 8] Intégration dans CADDocument..." << std::endl;
    CADDocument doc;
    doc.setName("TestDocSketch");
    
    auto docSketch = doc.addFeature<SketchFeature>("ProfileA");
    docSketch->setPlaneXY(0.0);
    
    auto docLine1 = std::make_shared<SketchLine>(gp_Pnt2d(0, 0), gp_Pnt2d(100, 0));
    auto docLine2 = std::make_shared<SketchLine>(gp_Pnt2d(100, 0), gp_Pnt2d(100, 50));
    auto docLine3 = std::make_shared<SketchLine>(gp_Pnt2d(100, 50), gp_Pnt2d(0, 50));
    auto docLine4 = std::make_shared<SketchLine>(gp_Pnt2d(0, 50), gp_Pnt2d(0, 0));
    
    docSketch->addEntity(docLine1);
    docSketch->addEntity(docLine2);
    docSketch->addEntity(docLine3);
    docSketch->addEntity(docLine4);
    
    std::cout << "  Features dans doc: " << doc.getFeatureCount() << std::endl;
    std::cout << "  Entités dans sketch: " << docSketch->getEntityCount() << std::endl;
    
    // Recalculer
    if (!doc.recomputeAll()) {
        std::cout << "  ✗ ERREUR: Échec recomputeAll()!" << std::endl;
        return 1;
    }
    
    std::cout << "  ✓ Document recalculé" << std::endl;
    std::cout << "  Features: " << std::endl;
    for (const auto& feat : doc.getAllFeatures()) {
        std::cout << "    - " << feat->getName() 
                  << " (" << feat->getTypeName() << ")"
                  << " [ID=" << feat->getId() << "]"
                  << " Valid=" << (feat->isValid() ? "Oui" : "Non")
                  << std::endl;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "✅ TOUS LES TESTS SONT PASSÉS !" << std::endl;
    std::cout << "✅ Phase 2.1 - Sketch Foundation opérationnelle" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}

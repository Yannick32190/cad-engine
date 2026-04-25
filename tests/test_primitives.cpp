#include <iostream>
#include "../src/core/parametric/Feature.h"
#include "../src/core/document/CADDocument.h"
#include "../src/features/primitives/BoxFeature.h"
#include "../src/features/primitives/CylinderFeature.h"

#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>

using namespace CADEngine;

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "CAD-ENGINE Phase 1 - Test Primitives" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Test 1: Création BoxFeature
    std::cout << "\n[Test 1] BoxFeature 100x50x30..." << std::endl;
    auto box = std::make_shared<BoxFeature>("TestBox");
    box->setDimensions(100.0, 50.0, 30.0);
    box->setPosition(0.0, 0.0, 0.0);
    
    std::cout << "  Debug: width=" << box->getDouble("width") << std::endl;
    std::cout << "  Debug: depth=" << box->getDouble("depth") << std::endl;
    std::cout << "  Debug: height=" << box->getDouble("height") << std::endl;
    
    if (!box->compute()) {
        std::cout << "  ✗ ERREUR: Échec compute()" << std::endl;
        std::cout << "  Shape valid: " << (!box->getShape().IsNull()) << std::endl;
        return 1;
    }
    
    std::cout << "  ✓ Box créé" << std::endl;
    std::cout << "  Type: " << box->getTypeName() << std::endl;
    std::cout << "  ID: " << box->getId() << std::endl;
    std::cout << "  Nom: " << box->getName() << std::endl;
    std::cout << "  Valid: " << (box->isValid() ? "Oui" : "Non") << std::endl;
    
    // Vérifier le volume
    GProp_GProps props;
    BRepGProp::VolumeProperties(box->getShape(), props);
    double volume = props.Mass();
    double expected = 100.0 * 50.0 * 30.0;
    
    std::cout << "  Volume: " << volume << " mm³" << std::endl;
    std::cout << "  Attendu: " << expected << " mm³" << std::endl;
    
    if (std::abs(volume - expected) < 0.01) {
        std::cout << "  ✓ Volume correct" << std::endl;
    } else {
        std::cout << "  ✗ ERREUR: Volume incorrect!" << std::endl;
        return 1;
    }
    
    // Test 2: Modification paramètres
    std::cout << "\n[Test 2] Modification BoxFeature -> 200x100x60..." << std::endl;
    box->setDimensions(200.0, 100.0, 60.0);
    box->compute();
    
    BRepGProp::VolumeProperties(box->getShape(), props);
    volume = props.Mass();
    expected = 200.0 * 100.0 * 60.0;
    
    std::cout << "  Volume: " << volume << " mm³" << std::endl;
    
    if (std::abs(volume - expected) < 0.01) {
        std::cout << "  ✓ Modification réussie" << std::endl;
    } else {
        std::cout << "  ✗ ERREUR: Volume après modif incorrect!" << std::endl;
        return 1;
    }
    
    // Test 3: CylinderFeature
    std::cout << "\n[Test 3] CylinderFeature R=50, H=100..." << std::endl;
    auto cyl = std::make_shared<CylinderFeature>("TestCylinder");
    cyl->setRadius(50.0);
    cyl->setHeight(100.0);
    cyl->setAxis(gp_Pnt(0,0,0), gp_Dir(0,0,1));
    
    if (!cyl->compute()) {
        std::cout << "  ✗ ERREUR: Échec compute()" << std::endl;
        return 1;
    }
    
    std::cout << "  ✓ Cylinder créé" << std::endl;
    
    BRepGProp::VolumeProperties(cyl->getShape(), props);
    volume = props.Mass();
    expected = M_PI * 50.0 * 50.0 * 100.0;
    
    std::cout << "  Volume: " << volume << " mm³" << std::endl;
    std::cout << "  Attendu: " << expected << " mm³" << std::endl;
    
    if (std::abs(volume - expected) < 1.0) {
        std::cout << "  ✓ Volume correct" << std::endl;
    } else {
        std::cout << "  ✗ ERREUR: Volume incorrect!" << std::endl;
        return 1;
    }
    
    // Test 4: CADDocument
    std::cout << "\n[Test 4] CADDocument avec features..." << std::endl;
    CADDocument doc;
    doc.setName("TestDocument");
    
    auto docBox = doc.addFeature<BoxFeature>("Body");
    docBox->setDimensions(80.0, 80.0, 60.0);
    
    auto docCyl = doc.addFeature<CylinderFeature>("Hole");
    docCyl->setRadius(20.0);
    docCyl->setHeight(80.0);
    
    std::cout << "  Features dans doc: " << doc.getFeatureCount() << std::endl;
    
    // Recalculer toutes les features
    if (!doc.recomputeAll()) {
        std::cout << "  ✗ ERREUR: Échec recomputeAll()" << std::endl;
        return 1;
    }
    
    std::cout << "  ✓ Document créé et recalculé" << std::endl;
    std::cout << "  Features: " << std::endl;
    for (const auto& feat : doc.getAllFeatures()) {
        std::cout << "    - " << feat->getName() 
                  << " (" << feat->getTypeName() << ")"
                  << " [ID=" << feat->getId() << "]"
                  << " Valid=" << (feat->isValid() ? "Oui" : "Non")
                  << std::endl;
    }
    
    // Test 5: Paramètres
    std::cout << "\n[Test 5] Système de paramètres..." << std::endl;
    std::cout << "  Box width: " << docBox->getDouble("width") << std::endl;
    std::cout << "  Cylinder radius: " << docCyl->getDouble("radius") << std::endl;
    std::cout << "  ✓ Paramètres accessibles" << std::endl;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "✅ TOUS LES TESTS SONT PASSÉS !" << std::endl;
    std::cout << "✅ Phase 1 - Noyau CAO opérationnel" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}

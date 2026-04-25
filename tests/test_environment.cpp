#include <iostream>
#include <QCoreApplication>
#include <QDebug>

// OpenCASCADE includes
#include <Standard_Version.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    std::cout << "========================================" << std::endl;
    std::cout << "CAD-ENGINE - Test Environnement" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Test 1: Versions
    std::cout << "\n[Test 1] Versions:" << std::endl;
    std::cout << "  Qt version: " << qVersion() << std::endl;
    std::cout << "  OCCT version: " << OCC_VERSION_COMPLETE << std::endl;
    
    // Test 2: Création point 3D
    std::cout << "\n[Test 2] Création point 3D:" << std::endl;
    gp_Pnt point1(0.0, 0.0, 0.0);
    gp_Pnt point2(10.0, 20.0, 30.0);
    std::cout << "  Point 1: (" << point1.X() << ", " << point1.Y() << ", " << point1.Z() << ")" << std::endl;
    std::cout << "  Point 2: (" << point2.X() << ", " << point2.Y() << ", " << point2.Z() << ")" << std::endl;
    
    // Test 3: Vecteur
    std::cout << "\n[Test 3] Calcul vecteur:" << std::endl;
    gp_Vec vec(point1, point2);
    std::cout << "  Vecteur: (" << vec.X() << ", " << vec.Y() << ", " << vec.Z() << ")" << std::endl;
    std::cout << "  Magnitude: " << vec.Magnitude() << std::endl;
    
    // Test 4: Création d'un cube
    std::cout << "\n[Test 4] Création primitive Box 100x100x100:" << std::endl;
    try {
        TopoDS_Shape box = BRepPrimAPI_MakeBox(100.0, 100.0, 100.0).Shape();
        
        // Calcul du volume
        GProp_GProps props;
        BRepGProp::VolumeProperties(box, props);
        double volume = props.Mass();
        
        std::cout << "  ✓ Box créé avec succès" << std::endl;
        std::cout << "  Volume: " << volume << " mm³" << std::endl;
        std::cout << "  Volume attendu: 1000000 mm³" << std::endl;
        
        if (std::abs(volume - 1000000.0) < 0.01) {
            std::cout << "  ✓ Volume correct" << std::endl;
        } else {
            std::cout << "  ✗ ERREUR: Volume incorrect!" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cout << "  ✗ ERREUR lors de la création du box: " << e.what() << std::endl;
        return 1;
    }
    
    // Test 5: Création d'un cylindre
    std::cout << "\n[Test 5] Création primitive Cylinder R=50, H=100:" << std::endl;
    try {
        gp_Ax2 axis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));
        TopoDS_Shape cylinder = BRepPrimAPI_MakeCylinder(axis, 50.0, 100.0).Shape();
        
        GProp_GProps props;
        BRepGProp::VolumeProperties(cylinder, props);
        double volume = props.Mass();
        double expected = M_PI * 50.0 * 50.0 * 100.0;
        
        std::cout << "  ✓ Cylinder créé avec succès" << std::endl;
        std::cout << "  Volume: " << volume << " mm³" << std::endl;
        std::cout << "  Volume attendu: " << expected << " mm³" << std::endl;
        
        if (std::abs(volume - expected) < 1.0) {
            std::cout << "  ✓ Volume correct" << std::endl;
        } else {
            std::cout << "  ✗ ERREUR: Volume incorrect!" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cout << "  ✗ ERREUR lors de la création du cylindre: " << e.what() << std::endl;
        return 1;
    }
    
    // Test 6: Opération booléenne (Fusion)
    std::cout << "\n[Test 6] Opération booléenne FUSE (Box + Cylinder):" << std::endl;
    try {
        TopoDS_Shape box = BRepPrimAPI_MakeBox(100.0, 100.0, 100.0).Shape();
        gp_Ax2 axis(gp_Pnt(50, 50, 0), gp_Dir(0, 0, 1));
        TopoDS_Shape cylinder = BRepPrimAPI_MakeCylinder(axis, 30.0, 150.0).Shape();
        
        BRepAlgoAPI_Fuse fuseMaker(box, cylinder);
        if (!fuseMaker.IsDone()) {
            std::cout << "  ✗ ERREUR: Fusion échouée!" << std::endl;
            return 1;
        }
        
        TopoDS_Shape result = fuseMaker.Shape();
        
        GProp_GProps props;
        BRepGProp::VolumeProperties(result, props);
        double volume = props.Mass();
        
        std::cout << "  ✓ Fusion réussie" << std::endl;
        std::cout << "  Volume résultant: " << volume << " mm³" << std::endl;
        
        if (volume > 1000000.0) { // Volume doit être > que le box seul
            std::cout << "  ✓ Volume cohérent" << std::endl;
        } else {
            std::cout << "  ✗ ERREUR: Volume incohérent!" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cout << "  ✗ ERREUR lors de la fusion: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "✅ TOUS LES TESTS SONT PASSÉS !" << std::endl;
    std::cout << "✅ Environnement OCCT 7.9.3 + Qt6 opérationnel" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}

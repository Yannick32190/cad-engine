#include "CylinderFeature.h"
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <gp_Ax2.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <iostream>
#include <cmath>

namespace CADEngine {

CylinderFeature::CylinderFeature(const std::string& name)
    : Feature(FeatureType::Cylinder, name)
{
    initializeParameters();
}

void CylinderFeature::initializeParameters() {
    // Dimensions par défaut
    setDouble("radius", 50.0);
    setDouble("height", 100.0);
    
    // Position par défaut : origine
    setDouble("x", 0.0);
    setDouble("y", 0.0);
    setDouble("z", 0.0);
    
    // Direction par défaut : axe Z
    setDouble("dx", 0.0);
    setDouble("dy", 0.0);
    setDouble("dz", 1.0);
    
    // Angle complet par défaut
    setDouble("angle", 360.0);
}

bool CylinderFeature::compute() {
    try {
        // Récupération des paramètres
        double radius = getDouble("radius");
        double height = getDouble("height");
        double x = getDouble("x");
        double y = getDouble("y");
        double z = getDouble("z");
        double dx = getDouble("dx");
        double dy = getDouble("dy");
        double dz = getDouble("dz");
        double angleDeg = getDouble("angle");
        
        // Validation
        if (radius <= Precision::Confusion() || 
            height <= Precision::Confusion()) {
            return false;
        }
        
        if (angleDeg <= 0.0 || angleDeg > 360.0) {
            angleDeg = 360.0;
        }
        
        // Création de l'axe
        gp_Pnt origin(x, y, z);
        gp_Dir direction(dx, dy, dz);  // Normalisé automatiquement
        gp_Ax2 axis(origin, direction);
        
        // Conversion angle en radians
        double angleRad = angleDeg * M_PI / 180.0;
        
        // Création du cylindre
        // NOTE: IsDone() ne fonctionne pas dans OCCT 7.9.3 pour les primitives!
        BRepPrimAPI_MakeCylinder cylMaker(axis, radius, height, angleRad);
        m_shape = cylMaker.Shape();
        
        if (m_shape.IsNull()) {
            return false;
        }
        
        m_upToDate = true;
        
        return true;
        
    } catch (const Standard_Failure& e) {
        std::cout << "CylinderFeature::compute() - OCCT Exception: " 
                  << e.GetMessageString() << std::endl;
        m_shape = TopoDS_Shape();
        m_upToDate = false;
        return false;
    } catch (const std::exception& e) {
        std::cout << "CylinderFeature::compute() - std::exception: " << e.what() << std::endl;
        m_shape = TopoDS_Shape();
        m_upToDate = false;
        return false;
    } catch (...) {
        std::cout << "CylinderFeature::compute() - Unknown exception!" << std::endl;
        m_shape = TopoDS_Shape();
        m_upToDate = false;
        return false;
    }
}

// Méthodes de configuration

void CylinderFeature::setRadius(double radius) {
    setDouble("radius", radius);
}

void CylinderFeature::setHeight(double height) {
    setDouble("height", height);
}

void CylinderFeature::setAxis(const gp_Pnt& origin, const gp_Dir& direction) {
    setDouble("x", origin.X());
    setDouble("y", origin.Y());
    setDouble("z", origin.Z());
    setDouble("dx", direction.X());
    setDouble("dy", direction.Y());
    setDouble("dz", direction.Z());
}

void CylinderFeature::setAngle(double angleDegrees) {
    setDouble("angle", angleDegrees);
}

// Getters

double CylinderFeature::getRadius() const {
    return getDouble("radius");
}

double CylinderFeature::getHeight() const {
    return getDouble("height");
}

gp_Pnt CylinderFeature::getOrigin() const {
    return gp_Pnt(
        getDouble("x"),
        getDouble("y"),
        getDouble("z")
    );
}

gp_Dir CylinderFeature::getDirection() const {
    return gp_Dir(
        getDouble("dx"),
        getDouble("dy"),
        getDouble("dz")
    );
}

double CylinderFeature::getAngle() const {
    return getDouble("angle");
}

} // namespace CADEngine

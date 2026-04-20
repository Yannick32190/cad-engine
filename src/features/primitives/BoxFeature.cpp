#include "BoxFeature.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <iostream>

namespace CADEngine {

BoxFeature::BoxFeature(const std::string& name)
    : Feature(FeatureType::Box, name)
{
    initializeParameters();
}

void BoxFeature::initializeParameters() {
    // Dimensions par défaut : 100x100x100 mm
    setDouble("width", 100.0);
    setDouble("depth", 100.0);
    setDouble("height", 100.0);
    
    // Position par défaut : origine
    setDouble("x", 0.0);
    setDouble("y", 0.0);
    setDouble("z", 0.0);
    
    // Par défaut : coin au point spécifié
    setBool("centered", false);
}

bool BoxFeature::compute() {
    try {
        // Récupération des paramètres
        double width = getDouble("width");
        double depth = getDouble("depth");
        double height = getDouble("height");
        double x = getDouble("x");
        double y = getDouble("y");
        double z = getDouble("z");
        bool centered = getBool("centered");
        
        std::cout << "BoxFeature::compute() - width=" << width 
                  << " depth=" << depth << " height=" << height << std::endl;
        
        // Validation
        if (width <= Precision::Confusion() || 
            depth <= Precision::Confusion() || 
            height <= Precision::Confusion()) {
            std::cout << "BoxFeature::compute() - Dimensions invalides!" << std::endl;
            return false;  // Dimensions invalides
        }
        
        // Position de départ
        gp_Pnt startPoint;
        if (centered) {
            // Si centré, décaler de la moitié des dimensions
            startPoint = gp_Pnt(
                x - width / 2.0,
                y - depth / 2.0,
                z - height / 2.0
            );
        } else {
            startPoint = gp_Pnt(x, y, z);
        }
        
        std::cout << "BoxFeature::compute() - Avant BRepPrimAPI_MakeBox" << std::endl;
        std::cout << "  position: " << x << ", " << y << ", " << z << std::endl;
        std::cout << "  dimensions: " << width << " x " << depth << " x " << height << std::endl;
        
        // Création du box OCCT à l'origine
        // NOTE: IsDone() ne fonctionne pas dans OCCT 7.9.3 pour les primitives!
        // On appelle directement Shape()
        BRepPrimAPI_MakeBox boxMaker(width, depth, height);
        TopoDS_Shape tempShape = boxMaker.Shape();
        
        std::cout << "BoxFeature::compute() - Shape obtenu, IsNull=" 
                  << tempShape.IsNull() << std::endl;
        
        if (tempShape.IsNull()) {
            std::cout << "BoxFeature::compute() - Shape is null!" << std::endl;
            return false;
        }
        
        // Appliquer translation si nécessaire
        if (std::abs(x) > Precision::Confusion() || 
            std::abs(y) > Precision::Confusion() || 
            std::abs(z) > Precision::Confusion()) {
            
            gp_Vec translation(x, y, z);
            if (centered) {
                translation = gp_Vec(x - width/2.0, y - depth/2.0, z - height/2.0);
            }
            
            gp_Trsf transform;
            transform.SetTranslation(translation);
            BRepBuilderAPI_Transform transformer(tempShape, transform, Standard_True);
            m_shape = transformer.Shape();
        } else if (centered) {
            // Juste centrer
            gp_Vec translation(-width/2.0, -depth/2.0, -height/2.0);
            gp_Trsf transform;
            transform.SetTranslation(translation);
            BRepBuilderAPI_Transform transformer(tempShape, transform, Standard_True);
            m_shape = transformer.Shape();
        } else {
            m_shape = tempShape;
        }
        
        m_upToDate = true;
        
        std::cout << "BoxFeature::compute() - Success! Shape.IsNull()=" 
                  << m_shape.IsNull() << std::endl;
        
        return true;
        
    } catch (const Standard_Failure& e) {
        std::cout << "BoxFeature::compute() - OCCT Exception: " 
                  << e.GetMessageString() << std::endl;
        m_shape = TopoDS_Shape();
        m_upToDate = false;
        return false;
    } catch (const std::exception& e) {
        std::cout << "BoxFeature::compute() - std::exception: " << e.what() << std::endl;
        m_shape = TopoDS_Shape();
        m_upToDate = false;
        return false;
    } catch (...) {
        std::cout << "BoxFeature::compute() - Unknown exception!" << std::endl;
        m_shape = TopoDS_Shape();
        m_upToDate = false;
        return false;
    }
}

// Méthodes de configuration

void BoxFeature::setDimensions(double width, double depth, double height) {
    setDouble("width", width);
    setDouble("depth", depth);
    setDouble("height", height);
}

void BoxFeature::setPosition(double x, double y, double z) {
    setDouble("x", x);
    setDouble("y", y);
    setDouble("z", z);
}

void BoxFeature::setPosition(const gp_Pnt& point) {
    setPosition(point.X(), point.Y(), point.Z());
}

void BoxFeature::setCentered(bool centered) {
    setBool("centered", centered);
}

// Getters

double BoxFeature::getWidth() const {
    return getDouble("width");
}

double BoxFeature::getDepth() const {
    return getDouble("depth");
}

double BoxFeature::getHeight() const {
    return getDouble("height");
}

gp_Pnt BoxFeature::getPosition() const {
    return gp_Pnt(
        getDouble("x"),
        getDouble("y"),
        getDouble("z")
    );
}

bool BoxFeature::isCentered() const {
    return getBool("centered");
}

} // namespace CADEngine

#include "SketchFeature.h"
#include <gp_Ax3.hxx>
#include <cmath>

namespace CADEngine {

SketchFeature::SketchFeature(const std::string& name)
    : Feature(FeatureType::Sketch, name)  // TODO: Ajouter FeatureType::Sketch
{
    // Plan XY par défaut
    gp_Pln defaultPlane(gp_Ax3(gp::Origin(), gp::DZ(), gp::DX()));
    m_sketch2D = std::make_shared<Sketch2D>(defaultPlane);
    
    initializeParameters();
}

void SketchFeature::initializeParameters() {
    // Plan par défaut : XY
    setString("plane_type", "XY");
    setDouble("plane_origin_x", 0.0);
    setDouble("plane_origin_y", 0.0);
    setDouble("plane_origin_z", 0.0);
    setDouble("plane_normal_x", 0.0);
    setDouble("plane_normal_y", 0.0);
    setDouble("plane_normal_z", 1.0);
    setDouble("plane_xdir_x", 1.0);
    setDouble("plane_xdir_y", 0.0);
    setDouble("plane_xdir_z", 0.0);
}

bool SketchFeature::compute() {
    try {
        // Charger le plan depuis les paramètres
        loadPlaneFromParameters();
        
        // Générer le wire depuis les entités du sketch
        if (m_sketch2D->getEntityCount() == 0) {
            // Pas d'entités = sketch vide mais valide
            m_shape = TopoDS_Shape();
            m_upToDate = true;
            return true;
        }
        
        TopoDS_Wire wire = m_sketch2D->getWire3D();
        
        if (!wire.IsNull()) {
            m_shape = wire;
            m_upToDate = true;
            return true;
        }
        
        // Si le wire est null mais qu'on a des entités, 
        // stocker les edges séparément dans un compound
        // Pour l'instant on accepte un sketch sans wire fermé
        m_upToDate = true;
        return true;
        
    } catch (const Standard_Failure& e) {
        m_shape = TopoDS_Shape();
        m_upToDate = false;
        return false;
    }
}

// ============================================================================
// Gestion du plan
// ============================================================================

void SketchFeature::setPlane(const gp_Pln& plane) {
    m_sketch2D->setPlane(plane);
    storePlaneInParameters();
    // Créer les axes de construction (si pas encore faits)
    m_sketch2D->createAxisConstructionLines();
    invalidate();
}

gp_Pln SketchFeature::getPlane() const {
    return m_sketch2D->getPlane();
}

void SketchFeature::setPlaneXY(double z) {
    gp_Ax3 ax3(gp_Pnt(0, 0, z), gp_Dir(0, 0, 1), gp_Dir(1, 0, 0));
    gp_Pln plane(ax3);
    setPlane(plane);  // → crée les axes de construction automatiquement
    setString("plane_type", "XY");
    setDouble("plane_origin_z", z);
}

void SketchFeature::setPlaneYZ(double x) {
    gp_Ax3 ax3(gp_Pnt(x, 0, 0), gp_Dir(1, 0, 0), gp_Dir(0, 1, 0));
    gp_Pln plane(ax3);
    setPlane(plane);
    setString("plane_type", "YZ");
    setDouble("plane_origin_x", x);
}

void SketchFeature::setPlaneZX(double y) {
    gp_Ax3 ax3(gp_Pnt(0, y, 0), gp_Dir(0, 1, 0), gp_Dir(1, 0, 0));
    gp_Pln plane(ax3);
    setPlane(plane);
    setString("plane_type", "ZX");
    setDouble("plane_origin_y", y);
}

void SketchFeature::storePlaneInParameters() {
    gp_Pln plane = m_sketch2D->getPlane();
    gp_Pnt origin = plane.Location();
    gp_Dir normal = plane.Axis().Direction();
    gp_Dir xDir = plane.XAxis().Direction();
    
    setDouble("plane_origin_x", origin.X());
    setDouble("plane_origin_y", origin.Y());
    setDouble("plane_origin_z", origin.Z());
    setDouble("plane_normal_x", normal.X());
    setDouble("plane_normal_y", normal.Y());
    setDouble("plane_normal_z", normal.Z());
    setDouble("plane_xdir_x", xDir.X());
    setDouble("plane_xdir_y", xDir.Y());
    setDouble("plane_xdir_z", xDir.Z());
    
    // Auto-detect plane_type si pas encore défini correctement
    // (important pour les sketches sur face)
    try { getString("plane_type"); } catch (...) {
        // Détecter depuis la normale
        if (std::abs(normal.Z()) > 0.99)      setString("plane_type", "XY");
        else if (std::abs(normal.X()) > 0.99)  setString("plane_type", "YZ");
        else if (std::abs(normal.Y()) > 0.99)  setString("plane_type", "ZX");
        else                                    setString("plane_type", "Custom");
    }
}

void SketchFeature::loadPlaneFromParameters() {
    double ox = getDouble("plane_origin_x");
    double oy = getDouble("plane_origin_y");
    double oz = getDouble("plane_origin_z");
    double nx = getDouble("plane_normal_x");
    double ny = getDouble("plane_normal_y");
    double nz = getDouble("plane_normal_z");
    
    gp_Pnt origin(ox, oy, oz);
    gp_Dir normal(nx, ny, nz);
    
    // Reconstruct with EXACT XDirection to preserve orientation
    // If XDir params don't exist (old sketches), fall back to gp_Pln(origin, normal)
    try {
        double xx = getDouble("plane_xdir_x");
        double xy = getDouble("plane_xdir_y");
        double xz = getDouble("plane_xdir_z");
        
        if (std::abs(xx) + std::abs(xy) + std::abs(xz) > 1e-10) {
            gp_Dir xDir(xx, xy, xz);
            gp_Ax2 ax2(origin, normal, xDir);
            gp_Pln plane(ax2);
            m_sketch2D->setPlane(plane);
        } else {
            gp_Pln plane(origin, normal);
            m_sketch2D->setPlane(plane);
        }
    } catch (...) {
        // Old sketch without xdir params — use default XDir
        gp_Pln plane(origin, normal);
        m_sketch2D->setPlane(plane);
    }
}

// ============================================================================
// Entités
// ============================================================================

void SketchFeature::addEntity(std::shared_ptr<SketchEntity> entity) {
    m_sketch2D->addEntity(entity);
    invalidate();
}

int SketchFeature::getEntityCount() const {
    return m_sketch2D->getEntityCount();
}

TopoDS_Wire SketchFeature::getWire() const {
    return m_sketch2D->getWire3D();
}

} // namespace CADEngine

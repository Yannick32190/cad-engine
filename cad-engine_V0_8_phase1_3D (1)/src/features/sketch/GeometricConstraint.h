#ifndef GEOMETRICCONSTRAINT_H
#define GEOMETRICCONSTRAINT_H

#include "SketchEntity.h"
#include <gp_Pnt2d.hxx>
#include <memory>
#include <string>

namespace CADEngine {

/**
 * @brief Types de contraintes géométriques
 */
enum class ConstraintType {
    Horizontal,      // Ligne horizontale
    Vertical,        // Ligne verticale
    Parallel,        // 2 lignes parallèles
    Perpendicular,   // 2 lignes perpendiculaires
    Coincident,      // 2 points au même endroit
    Concentric,      // 2 cercles/arcs même centre
    Equal,           // 2 entités même dimension (longueur/rayon)
    Tangent,         // Ligne tangente à cercle/arc
    Fix,             // Point/entité fixée (ne bouge pas)
    Symmetric,       // 2 points symétriques par rapport à un axe
    CenterOnAxis     // Entité centrée sur un axe de symétrie
};

/**
 * @brief Classe de base pour les contraintes géométriques
 * 
 * Une contrainte définit une relation géométrique entre des entités
 * qui doit être maintenue par le solveur.
 */
class GeometricConstraint {
public:
    virtual ~GeometricConstraint() = default;
    
    virtual ConstraintType getType() const = 0;
    virtual std::string getDescription() const = 0;
    
    /**
     * @brief Vérifie si la contrainte est satisfaite
     */
    virtual bool isSatisfied(double tolerance = 1e-6) const = 0;
    
    /**
     * @brief Applique la contrainte (modifie les entités)
     */
    virtual void apply() = 0;
    
    /**
     * @brief Symbole visuel pour affichage (Unicode)
     */
    virtual std::string getSymbol() const = 0;
    
    // Priorité d'application (0 = haute priorité)
    virtual int getPriority() const { return 5; }
    
    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }
    
    int getId() const { return m_id; }
    
protected:
    GeometricConstraint() : m_id(s_nextId++), m_active(true) {}
    
    int m_id;
    bool m_active;
    
    static int s_nextId;
};

} // namespace CADEngine

#endif // GEOMETRICCONSTRAINT_H

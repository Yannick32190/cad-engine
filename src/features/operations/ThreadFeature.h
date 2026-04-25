#ifndef THREADFEATURE_H
#define THREADFEATURE_H

#include "../../core/parametric/Feature.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

namespace CADEngine {

enum class ThreadType {
    MetricCoarse,   // Pas standard ISO
    MetricFine,     // Pas fin ISO
    Custom          // Personnalisé
};

enum class ThreadMode {
    RemoveMaterial,  // Enlever matière (ex: Ø20 → creuser sillons)
    AddMaterial      // Ajouter matière (ex: Ø17.5 → construire filets vers Ø20)
};

/**
 * @brief ThreadFeature — Filetage par révolution
 * 
 * Génère un profil en dents de scie dans le plan axial
 * et le tourne à 360° autour de l'axe du cylindre.
 * 
 * Deux modes :
 * - RemoveMaterial : CUT d'un anneau denté (sillons)
 * - AddMaterial : FUSE d'un anneau denté (crêtes)
 * 
 * Le pas de vis gauche/droite est indiqué visuellement.
 */
class ThreadFeature : public Feature {
public:
    ThreadFeature(const std::string& name = "Filetage");
    virtual ~ThreadFeature() = default;
    
    bool compute() override;
    std::string getTypeName() const override { return "Thread"; }
    
    // Setters
    void setBaseShape(const TopoDS_Shape& shape);
    void setCylindricalFace(const TopoDS_Face& face);
    void setThreadType(ThreadType type);
    void setMode(ThreadMode mode);
    void setLeftHand(bool left);         // true = pas à gauche
    void setDiameter(double diameter);   // Diamètre nominal M
    void setPitch(double pitch);
    void setDepth(double depth);         // 0 = auto ISO
    void setLength(double length);
    
    // Getters
    ThreadType getThreadType() const;
    ThreadMode getMode() const;
    bool isLeftHand() const;
    double getDiameter() const;
    double getPitch() const;
    double getDepth() const;
    double getLength() const;
    TopoDS_Shape getResultShape() const { return m_resultShape; }
    
    // Helpers statiques
    static double getStandardPitch(double diameter, ThreadType type);
    static double getStandardDepth(double pitch);

private:
    TopoDS_Shape buildThreadSolid() const;
    TopoDS_Shape buildRevolutionFallback(
        const gp_Ax1& axis, const gp_Dir& radDir, const gp_Dir& axDir,
        const gp_Pnt& start, double R, double Rtooth, double Rclose,
        double pitch, double length, int nTeeth) const;
    gp_Ax1 getCylinderAxis() const;
    double getCylinderRadius() const;
    double getCylinderVMin() const;
    
    TopoDS_Shape m_baseShape;
    TopoDS_Shape m_resultShape;
    TopoDS_Face m_cylindricalFace;
};

} // namespace CADEngine

#endif // THREADFEATURE_H

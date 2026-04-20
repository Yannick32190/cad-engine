#include "PushPullFeature.h"
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <GeomLProp_SLProps.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <iostream>
#include <cmath>

namespace CADEngine {

PushPullFeature::PushPullFeature(const std::string& name)
    : Feature(FeatureType::Unknown, name)
    , m_faceNormal(0, 0, 1)
{
    setDouble("distance", 10.0);
    setDouble("normal_x", 0.0);
    setDouble("normal_y", 0.0);
    setDouble("normal_z", 1.0);
    setDouble("face_cx", 0.0);
    setDouble("face_cy", 0.0);
    setDouble("face_cz", 0.0);
}

void PushPullFeature::setFace(const TopoDS_Face& face, const gp_Dir* cameraDir) {
    m_face = face;
    
    // Calculer la normale de la face au centre
    try {
        BRepAdaptor_Surface adaptor(face);
        double uMid = (adaptor.FirstUParameter() + adaptor.LastUParameter()) / 2.0;
        double vMid = (adaptor.FirstVParameter() + adaptor.LastVParameter()) / 2.0;
        
        gp_Pnt point;
        gp_Vec du, dv;
        adaptor.D1(uMid, vMid, point, du, dv);
        
        gp_Vec normal = du.Crossed(dv);
        if (normal.Magnitude() > 1e-10) {
            normal.Normalize();
            
            if (cameraDir) {
                // Méthode robuste : la normale doit pointer VERS la caméra
                // (= dans la direction opposée au regard)
                // Le vecteur caméra pointe VERS la scène, donc si dot(normal, cameraDir) > 0,
                // la normale pointe dans le même sens que la caméra = AWAY from camera = mauvais
                double dot = normal.X() * cameraDir->X() + normal.Y() * cameraDir->Y() + normal.Z() * cameraDir->Z();
                if (dot > 0) {
                    normal.Reverse();
                    std::cout << "[PushPull] Normal flipped (was pointing away from camera)" << std::endl;
                }
            } else {
                // Fallback sans caméra : utiliser l'orientation OCCT
                if (face.Orientation() == TopAbs_REVERSED) {
                    normal.Reverse();
                }
            }
            m_faceNormal = gp_Dir(normal);
        }
        
        // Sauvegarder la normale dans les paramètres pour le save/load
        setDouble("normal_x", m_faceNormal.X());
        setDouble("normal_y", m_faceNormal.Y());
        setDouble("normal_z", m_faceNormal.Z());
        
        // Sauvegarder le centroïde de la face (pour retrouver la bonne face au chargement)
        GProp_GProps props;
        BRepGProp::SurfaceProperties(face, props);
        gp_Pnt center = props.CentreOfMass();
        setDouble("face_cx", center.X());
        setDouble("face_cy", center.Y());
        setDouble("face_cz", center.Z());
        
        std::cout << "[PushPull] Face normal: (" 
                  << m_faceNormal.X() << ", " << m_faceNormal.Y() << ", " << m_faceNormal.Z() << ")"
                  << " center: (" << center.X() << ", " << center.Y() << ", " << center.Z() << ")" << std::endl;
    } catch (...) {
        std::cerr << "[PushPull] Erreur calcul normale" << std::endl;
    }
}

bool PushPullFeature::compute() {
    if (m_existingBody.IsNull() || m_face.IsNull()) {
        std::cerr << "[PushPull] No body or face" << std::endl;
        m_upToDate = false;
        return false;
    }
    
    double distance = getDistance();
    if (std::abs(distance) < 1e-6) {
        m_resultShape = m_existingBody;
        m_shape = m_existingBody;
        m_upToDate = true;
        return true;
    }
    
    try {
        // Direction d'extrusion = normale * distance
        // distance > 0 → tirer (ajouter matière vers l'extérieur)
        // distance < 0 → pousser (retirer matière vers l'intérieur)
        gp_Vec vec(m_faceNormal);
        vec.Scale(distance);
        
        // Créer le prisme à partir de la face
        BRepPrimAPI_MakePrism prism(m_face, vec);
        prism.Build();
        
        if (!prism.IsDone()) {
            std::cerr << "[PushPull] MakePrism failed" << std::endl;
            m_upToDate = false;
            return false;
        }
        
        TopoDS_Shape prismShape = prism.Shape();
        
        if (distance > 0) {
            // Tirer → Fuse (ajouter matière)
            BRepAlgoAPI_Fuse fuse(m_existingBody, prismShape);
            fuse.Build();
            if (fuse.IsDone()) {
                // Nettoyer la topologie : fusionner les faces coplanaires
                // (élimine les arêtes/faces fantômes de l'ancienne jonction)
                ShapeUpgrade_UnifySameDomain unify(fuse.Shape());
                unify.Build();
                m_resultShape = unify.Shape();
                m_shape = m_resultShape;
                m_upToDate = true;
                
                // Debug: compter les faces
                int nFaces = 0;
                TopExp_Explorer exp(m_resultShape, TopAbs_FACE);
                for (; exp.More(); exp.Next()) nFaces++;
                std::cout << "[PushPull] Pull OK, distance=" << distance 
                          << ", faces=" << nFaces << std::endl;
                return true;
            }
        } else {
            // Pousser → Cut (retirer matière)
            // Créer le prisme dans la direction OPPOSÉE pour couper
            gp_Vec vecCut(m_faceNormal);
            vecCut.Scale(-distance);  // distance est négatif, donc -distance est positif
            vecCut.Reverse();         // Vers l'intérieur
            
            BRepPrimAPI_MakePrism prismCut(m_face, vecCut);
            prismCut.Build();
            
            if (prismCut.IsDone()) {
                BRepAlgoAPI_Cut cut(m_existingBody, prismCut.Shape());
                cut.Build();
                if (cut.IsDone()) {
                    ShapeUpgrade_UnifySameDomain unify(cut.Shape());
                    unify.Build();
                    m_resultShape = unify.Shape();
                    m_shape = m_resultShape;
                    m_upToDate = true;
                    
                    int nFaces = 0;
                    TopExp_Explorer exp(m_resultShape, TopAbs_FACE);
                    for (; exp.More(); exp.Next()) nFaces++;
                    std::cout << "[PushPull] Push OK, distance=" << distance 
                              << ", faces=" << nFaces << std::endl;
                    return true;
                }
            }
        }
        
    } catch (const Standard_Failure& e) {
        std::cerr << "[PushPull] Exception: " << e.GetMessageString() << std::endl;
    } catch (...) {
        std::cerr << "[PushPull] Unknown error" << std::endl;
    }
    
    m_upToDate = false;
    return false;
}

} // namespace CADEngine

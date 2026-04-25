#include "SweepFeature.h"

#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepOffsetAPI_MakePipeShell.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepTools.hxx>
#include <gp_Dir.hxx>

#include <iostream>

namespace CADEngine {

SweepFeature::SweepFeature(const std::string& name)
    : Feature(FeatureType::Unknown, name)
{
    setInt("operation", static_cast<int>(ExtrudeOperation::NewBody));
}

void SweepFeature::setProfileSketch(std::shared_ptr<SketchFeature> sketch) {
    m_profileSketch = sketch;
    if (sketch) addDependency(sketch->getId());
}

void SweepFeature::setPathSketch(std::shared_ptr<SketchFeature> sketch) {
    m_pathSketch = sketch;
    if (sketch) addDependency(sketch->getId());
}

void SweepFeature::setOperation(ExtrudeOperation op) {
    setInt("operation", static_cast<int>(op));
    invalidate();
}

void SweepFeature::setExistingBody(const TopoDS_Shape& body) {
    m_existingBody = body;
}

void SweepFeature::setProfileFace(const TopoDS_Face& face) {
    m_profileFace = face;
}

void SweepFeature::setPathWire(const TopoDS_Wire& wire) {
    m_pathWire = wire;
}

ExtrudeOperation SweepFeature::getOperation() const {
    return static_cast<ExtrudeOperation>(getInt("operation"));
}

TopoDS_Face SweepFeature::buildProfileFace() const {
    if (!m_profileFace.IsNull()) return m_profileFace;
    if (!m_profileSketch || !m_profileSketch->getSketch2D()) return TopoDS_Face();

    auto sketch2D = m_profileSketch->getSketch2D();
    auto regions = sketch2D->buildFaceRegions();
    if (regions.empty()) {
        std::cerr << "[Sweep] Profile sketch has no closed region" << std::endl;
        return TopoDS_Face();
    }
    return regions[0].face;
}

TopoDS_Wire SweepFeature::buildProfileWire() const {
    if (!m_profileSketch || !m_profileSketch->getSketch2D()) return TopoDS_Wire();
    auto sketch2D = m_profileSketch->getSketch2D();
    auto regions = sketch2D->buildFaceRegions();
    if (!regions.empty() && !regions[0].face.IsNull()) {
        TopoDS_Wire w = BRepTools::OuterWire(regions[0].face);
        if (!w.IsNull()) return w;
    }
    // fallback : reconstruire depuis les arêtes
    auto edges = sketch2D->getEdges3D();
    BRepBuilderAPI_MakeWire wm;
    for (const auto& e : edges)
        if (!e.IsNull()) wm.Add(e);
    if (wm.IsDone()) return wm.Wire();
    return TopoDS_Wire();
}

TopoDS_Wire SweepFeature::buildPathWire() const {
    if (!m_pathWire.IsNull()) return m_pathWire;
    if (!m_pathSketch || !m_pathSketch->getSketch2D()) return TopoDS_Wire();

    auto sketch2D = m_pathSketch->getSketch2D();
    auto entities = sketch2D->getEntities();
    gp_Pln plane = sketch2D->getPlane();

    // Collecter toutes les arêtes réelles (hors construction)
    Handle(TopTools_HSequenceOfShape) edgeSeq = new TopTools_HSequenceOfShape();
    int edgeCount = 0;

    for (const auto& entity : entities) {
        if (!entity || !entity->isValid() || entity->isConstruction()) continue;
        auto edges = entity->toEdges3D(plane);
        for (const auto& edge : edges) {
            if (!edge.IsNull()) {
                edgeSeq->Append(edge);
                edgeCount++;
            }
        }
    }

    std::cout << "[Sweep] Path wire: " << edgeCount << " real edge(s)" << std::endl;

    if (edgeCount == 0) {
        std::cerr << "[Sweep] ERROR: Path sketch has no usable entities" << std::endl;
        return TopoDS_Wire();
    }

    // Collecte des arêtes dans un vecteur pour le tri topologique
    std::vector<TopoDS_Edge> pool;
    for (int i = 1; i <= edgeSeq->Length(); i++)
        pool.push_back(TopoDS::Edge(edgeSeq->Value(i)));

    // Tri topologique : on chaîne les arêtes en cherchant à chaque étape
    // l'arête dont un endpoint coïncide avec la fin courante du wire.
    auto edgeStart = [](const TopoDS_Edge& e) -> gp_Pnt {
        TopExp_Explorer ex(e, TopAbs_VERTEX);
        return BRep_Tool::Pnt(TopoDS::Vertex(ex.Current()));
    };
    auto edgeEnd = [](const TopoDS_Edge& e) -> gp_Pnt {
        TopExp_Explorer ex(e, TopAbs_VERTEX);
        ex.Next();
        if (ex.More()) return BRep_Tool::Pnt(TopoDS::Vertex(ex.Current()));
        // edge dégénérée : retourner le premier vertex
        TopExp_Explorer ex2(e, TopAbs_VERTEX);
        return BRep_Tool::Pnt(TopoDS::Vertex(ex2.Current()));
    };

    const double matchTol = 1.0; // tolérance de connexion en mm

    std::vector<TopoDS_Edge> ordered;
    std::vector<bool> used(pool.size(), false);

    // Démarrer avec la première arête
    ordered.push_back(pool[0]);
    used[0] = true;
    gp_Pnt currentEnd = edgeEnd(pool[0]);

    while (ordered.size() < pool.size()) {
        bool found = false;
        for (size_t i = 0; i < pool.size(); i++) {
            if (used[i]) continue;
            gp_Pnt s = edgeStart(pool[i]);
            gp_Pnt e = edgeEnd(pool[i]);
            if (currentEnd.Distance(s) < matchTol) {
                // Arête en avant
                ordered.push_back(pool[i]);
                used[i] = true;
                currentEnd = e;
                found = true;
                break;
            } else if (currentEnd.Distance(e) < matchTol) {
                // Arête à inverser
                TopoDS_Edge rev = TopoDS::Edge(pool[i].Reversed());
                ordered.push_back(rev);
                used[i] = true;
                currentEnd = s;
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "[Sweep] WARNING: Path wire has gap, stopping at " << ordered.size() << " edges" << std::endl;
            break;
        }
    }

    BRepBuilderAPI_MakeWire wireBuilder;
    for (const auto& e : ordered)
        wireBuilder.Add(e);

    if (wireBuilder.IsDone()) {
        std::cout << "[Sweep] Path wire: built with topological sort (" << ordered.size() << " edges)" << std::endl;
        return wireBuilder.Wire();
    }

    std::cerr << "[Sweep] ERROR: Path wire build failed after topological sort" << std::endl;
    return TopoDS_Wire();
}

bool SweepFeature::compute() {
    std::cout << "[Sweep] Computing..." << std::endl;

    TopoDS_Face profile = buildProfileFace();
    if (profile.IsNull()) {
        std::cerr << "[Sweep] ERROR: No profile face — profile sketch must be a closed shape" << std::endl;
        return false;
    }

    TopoDS_Wire path = buildPathWire();
    if (path.IsNull()) {
        std::cerr << "[Sweep] ERROR: No path wire" << std::endl;
        return false;
    }

    // Vérifier que profil et chemin ne sont pas sur le même plan (pipe dégénéré)
    if (m_profileSketch && m_pathSketch &&
        m_profileSketch->getSketch2D() && m_pathSketch->getSketch2D())
    {
        gp_Dir profileNormal = m_profileSketch->getSketch2D()->getPlane().Axis().Direction();
        gp_Dir pathNormal    = m_pathSketch->getSketch2D()->getPlane().Axis().Direction();
        if (profileNormal.IsParallel(pathNormal, 0.01)) {
            std::cerr << "[Sweep] ERROR: Profile and path sketches are on parallel planes — "
                         "use perpendicular planes (e.g. profile on XY, path on XZ)" << std::endl;
            return false;
        }
    }

    try {
        TopoDS_Shape sweepShape;

        // MakePipeShell gère correctement les coins à 90° (mode RightCorner)
        // contrairement à MakePipe qui aplatit les transitions aux angles
        TopoDS_Wire profileWire = buildProfileWire();
        bool usedPipeShell = false;

        if (!profileWire.IsNull()) {
            try {
                BRepOffsetAPI_MakePipeShell pipeShell(path);
                pipeShell.Add(profileWire);
                pipeShell.SetTransitionMode(BRepBuilderAPI_RightCorner);

                if (pipeShell.IsReady()) {
                    pipeShell.Build();
                    if (pipeShell.IsDone()) {
                        pipeShell.MakeSolid();
                        sweepShape = pipeShell.Shape();
                        usedPipeShell = true;
                        std::cout << "[Sweep] MakePipeShell (RightCorner): OK" << std::endl;
                    }
                }
            } catch (Standard_Failure& e) {
                std::cerr << "[Sweep] MakePipeShell failed: " << e.GetMessageString()
                          << " — fallback to MakePipe" << std::endl;
            }
        }

        // Fallback : MakePipe classique
        if (!usedPipeShell) {
            BRepOffsetAPI_MakePipe pipe(path, profile);
            pipe.Build();
            if (!pipe.IsDone()) {
                std::cerr << "[Sweep] ERROR: MakePipe failed" << std::endl;
                return false;
            }
            sweepShape = pipe.Shape();
            std::cout << "[Sweep] MakePipe (fallback): OK" << std::endl;
        }

        ExtrudeOperation op = getOperation();
        if (op == ExtrudeOperation::NewBody || m_existingBody.IsNull()) {
            m_resultShape = sweepShape;
            m_shape = sweepShape;
        } else if (op == ExtrudeOperation::Join) {
            BRepAlgoAPI_Fuse fuse(m_existingBody, sweepShape);
            if (fuse.IsDone()) { m_resultShape = fuse.Shape(); m_shape = m_resultShape; }
            else { m_resultShape = sweepShape; m_shape = sweepShape; }
        } else if (op == ExtrudeOperation::Cut) {
            BRepAlgoAPI_Cut cut(m_existingBody, sweepShape);
            if (cut.IsDone()) { m_resultShape = cut.Shape(); m_shape = m_resultShape; }
            else return false;
        } else if (op == ExtrudeOperation::Intersect) {
            BRepAlgoAPI_Common common(m_existingBody, sweepShape);
            if (common.IsDone()) { m_resultShape = common.Shape(); m_shape = m_resultShape; }
            else return false;
        }

        m_upToDate = true;
        std::cout << "[Sweep] Success" << std::endl;
        return true;

    } catch (Standard_Failure& e) {
        std::cerr << "[Sweep] OCCT Exception: " << e.GetMessageString() << std::endl;
        return false;
    }
}

} // namespace CADEngine

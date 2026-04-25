#include "SweepFeature.h"

#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
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

TopoDS_Wire SweepFeature::buildPathWire() const {
    if (!m_pathWire.IsNull()) return m_pathWire;
    if (!m_pathSketch || !m_pathSketch->getSketch2D()) return TopoDS_Wire();

    auto sketch2D = m_pathSketch->getSketch2D();
    auto entities = sketch2D->getEntities();
    gp_Pln plane = sketch2D->getPlane();

    BRepBuilderAPI_MakeWire wireBuilder;
    int edgeCount = 0;

    for (const auto& entity : entities) {
        // Ignorer les entités de construction (axes, lignes d'aide)
        if (!entity || !entity->isValid() || entity->isConstruction()) continue;
        auto edges = entity->toEdges3D(plane);
        for (const auto& edge : edges) {
            if (!edge.IsNull()) {
                wireBuilder.Add(edge);
                edgeCount++;
            }
        }
    }

    std::cout << "[Sweep] Path wire: " << edgeCount << " real edge(s) added" << std::endl;

    if (edgeCount == 0) {
        std::cerr << "[Sweep] ERROR: Path sketch has no usable entities (only construction lines?)" << std::endl;
        return TopoDS_Wire();
    }
    if (!wireBuilder.IsDone()) {
        // Donner un message d'erreur précis selon le code d'erreur
        auto err = wireBuilder.Error();
        if (err == BRepBuilderAPI_DisconnectedWire)
            std::cerr << "[Sweep] ERROR: Path wire is disconnected — entities must be connected end-to-end" << std::endl;
        else if (err == BRepBuilderAPI_NonManifoldWire)
            std::cerr << "[Sweep] ERROR: Path wire is non-manifold" << std::endl;
        else
            std::cerr << "[Sweep] ERROR: Path wire build failed (code=" << err << ")" << std::endl;
        return TopoDS_Wire();
    }
    return wireBuilder.Wire();
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
        BRepOffsetAPI_MakePipe pipe(path, profile);
        pipe.Build();

        if (!pipe.IsDone()) {
            std::cerr << "[Sweep] ERROR: MakePipe failed — check that path is continuous "
                         "and profile is at path start" << std::endl;
            return false;
        }

        TopoDS_Shape sweepShape = pipe.Shape();

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

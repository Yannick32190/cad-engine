#include "Sketch2D.h"
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <BRep_Tool.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BOPAlgo_Builder.hxx>
#include <BOPAlgo_PaveFiller.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <Geom_Surface.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace CADEngine {

Sketch2D::Sketch2D(const gp_Pln& plane)
    : m_plane(plane)
{
    // Extraire le système d'axes du plan
    const gp_Ax3& ax3 = plane.Position();
    m_localAxes = gp_Ax2(ax3.Location(), ax3.Direction(), ax3.XDirection());
}

void Sketch2D::setPlane(const gp_Pln& plane) {
    m_plane = plane;
    const gp_Ax3& ax3 = plane.Position();
    m_localAxes = gp_Ax2(ax3.Location(), ax3.Direction(), ax3.XDirection());
}

// ============================================================================
// Conversion 2D ↔ 3D
// ============================================================================

gp_Pnt2d Sketch2D::toLocal(const gp_Pnt& pt3D) const {
    // Utiliser gp_Pln::Value() en mode inverse
    // On projette le point 3D sur le plan et on récupère les coordonnées 2D
    
    // Vecteur du point au centre du plan
    gp_Vec vec(m_localAxes.Location(), pt3D);
    
    // Projection sur les axes X et Y du plan
    double x = vec.Dot(gp_Vec(m_localAxes.XDirection()));
    double y = vec.Dot(gp_Vec(m_localAxes.YDirection()));
    
    return gp_Pnt2d(x, y);
}

gp_Pnt Sketch2D::toGlobal(const gp_Pnt2d& pt2D) const {
    // Calculer point 3D = Origin + pt2D.X * XDir + pt2D.Y * YDir
    gp_Pnt origin = m_localAxes.Location();
    gp_Vec xVec(m_localAxes.XDirection());
    gp_Vec yVec(m_localAxes.YDirection());
    
    gp_Pnt result = origin.Translated(xVec.Multiplied(pt2D.X()) + yVec.Multiplied(pt2D.Y()));
    return result;
}

// ============================================================================
// Gestion des entités
// ============================================================================

void Sketch2D::addEntity(std::shared_ptr<SketchEntity> entity) {
    if (entity && entity->isValid()) {
        m_entities.push_back(entity);
    }
}

void Sketch2D::removeEntity(std::shared_ptr<SketchEntity> entity) {
    auto it = std::find(m_entities.begin(), m_entities.end(), entity);
    if (it != m_entities.end()) {
        m_entities.erase(it);
    }
}

void Sketch2D::clearEntities() {
    m_entities.clear();
}

// ============================================================================
// Sélection
// ============================================================================

std::shared_ptr<SketchEntity> Sketch2D::findEntityAt(const gp_Pnt2d& pt, double tolerance) const {
    for (const auto& entity : m_entities) {
        // Vérifier la distance aux points clés
        for (const auto& keyPt : entity->getKeyPoints()) {
            if (pt.Distance(keyPt) < tolerance) {
                return entity;
            }
        }
        
        // TODO: Vérifier distance au corps de l'entité (ligne, arc...)
    }
    
    return nullptr;
}

void Sketch2D::selectAll() {
    for (auto& entity : m_entities) {
        entity->setSelected(true);
    }
}

void Sketch2D::deselectAll() {
    for (auto& entity : m_entities) {
        entity->setSelected(false);
    }
}

// ============================================================================
// Génération géométrie 3D
// ============================================================================

std::vector<TopoDS_Edge> Sketch2D::getEdges3D() const {
    std::vector<TopoDS_Edge> edges;
    
    for (const auto& entity : m_entities) {
        // Skip construction geometry (axes, lignes de construction)
        if (entity->isConstruction()) continue;
        
        // Use toEdges3D() which returns ALL edges for multi-edge entities
        auto entityEdges = entity->toEdges3D(m_plane);
        for (auto& e : entityEdges) {
            if (!e.IsNull()) {
                edges.push_back(e);
            }
        }
    }
    
    return edges;
}

TopoDS_Wire Sketch2D::getWire3D() const {
    auto edges = getEdges3D();
    if (edges.empty()) return TopoDS_Wire();
    
    BRepBuilderAPI_MakeWire wireMaker;
    
    for (const auto& edge : edges) {
        if (!edge.IsNull()) {
            try {
                wireMaker.Add(edge);
            } catch (...) {
                // Edge might not connect, skip it
            }
        }
    }
    
    if (wireMaker.IsDone()) {
        return wireMaker.Wire();
    }
    
    return TopoDS_Wire();
}

std::vector<TopoDS_Wire> Sketch2D::getClosedWires3D() const {
    std::vector<TopoDS_Wire> closedWires;
    
    auto edges = getEdges3D();
    if (edges.empty()) return closedWires;
    
    gp_Pln plane = getPlane();
    
    // Use ShapeAnalysis_FreeBounds to find all connected wires
    Handle(TopTools_HSequenceOfShape) edgeSeq = new TopTools_HSequenceOfShape();
    for (const auto& edge : edges) {
        if (!edge.IsNull()) {
            edgeSeq->Append(edge);
        }
    }
    
    if (edgeSeq->Length() == 0) return closedWires;
    
    Handle(TopTools_HSequenceOfShape) wires = new TopTools_HSequenceOfShape();
    ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edgeSeq, 0.1, Standard_False, wires);
    
    for (int i = 1; i <= wires->Length(); i++) {
        TopoDS_Wire w = TopoDS::Wire(wires->Value(i));
        if (w.IsNull()) continue;
        
        // Test if wire is closed by trying to make a face
        // If MakeFace succeeds, the wire is closed
        try {
            BRepBuilderAPI_MakeFace testFace(plane, w, Standard_True);
            if (testFace.IsDone()) {
                closedWires.push_back(w);
            }
        } catch (...) {}
    }
    
    // Also check single entities that are inherently closed (circle, ellipse)
    for (const auto& entity : m_entities) {
        if (entity->isConstruction()) continue;
        if (entity->getType() == SketchEntityType::Circle ||
            entity->getType() == SketchEntityType::Ellipse) {
            TopoDS_Edge edge = entity->toEdge3D(m_plane);
            if (!edge.IsNull()) {
                try {
                    BRepBuilderAPI_MakeWire wm(edge);
                    if (wm.IsDone()) {
                        BRepBuilderAPI_MakeFace testFace(plane, wm.Wire(), Standard_True);
                        if (testFace.IsDone()) {
                            closedWires.push_back(wm.Wire());
                        }
                    }
                } catch (...) {}
            }
        }
    }
    
    std::cout << "[Sketch2D] Found " << closedWires.size() << " closed wire(s) from " 
              << edges.size() << " edges" << std::endl;
    
    return closedWires;
}

// ============================================================================
// Detect Closed Profiles — individualized for profile selection
// ============================================================================

std::vector<Sketch2D::ClosedProfile> Sketch2D::detectClosedProfiles() const {
    std::vector<ClosedProfile> profiles;
    gp_Pln plane = getPlane();
    
    if (m_entities.empty()) return profiles;
    
    // ========================================================================
    // PHASE 1: Detect inherently closed entities (circle, ellipse, rectangle,
    //          polygon, oblong) — each becomes its own profile directly
    // ========================================================================
    std::vector<int> closedEntityIndices;  // entities handled in phase 1
    
    for (int idx = 0; idx < (int)m_entities.size(); idx++) {
        const auto& entity = m_entities[idx];
        if (entity->isConstruction()) continue;
        SketchEntityType type = entity->getType();
        
        bool isClosedEntity = (type == SketchEntityType::Circle ||
                               type == SketchEntityType::Ellipse ||
                               type == SketchEntityType::Rectangle ||
                               type == SketchEntityType::Polygon ||
                               type == SketchEntityType::Oblong);
        
        if (!isClosedEntity) continue;
        
        // Build wire from this entity's own edges
        auto edges = entity->toEdges3D(plane);
        if (edges.empty()) {
            std::cout << "[detectProfiles] Entity " << idx << " (" << (int)type << "): no edges" << std::endl;
            continue;
        }
        
        std::cout << "[detectProfiles] Entity " << idx << " (" << (int)type << "): " << edges.size() << " edges" << std::endl;
        
        TopoDS_Wire wire;
        bool wireOk = false;
        
        // Method 1: Direct BRepBuilderAPI_MakeWire (fast, works when edges chain perfectly)
        try {
            BRepBuilderAPI_MakeWire wireMaker;
            for (const auto& e : edges) {
                if (!e.IsNull()) wireMaker.Add(e);
            }
            if (wireMaker.IsDone()) {
                wire = wireMaker.Wire();
                wireOk = true;
                std::cout << "  → MakeWire direct: OK" << std::endl;
            } else {
                std::cout << "  → MakeWire direct: FAILED (error " << wireMaker.Error() << ")" << std::endl;
            }
        } catch (...) {
            std::cout << "  → MakeWire direct: EXCEPTION" << std::endl;
        }
        
        // Method 2: ConnectEdgesToWires with tolerance (for arcs with floating-point gaps)
        if (!wireOk) {
            try {
                Handle(TopTools_HSequenceOfShape) edgeSeq = new TopTools_HSequenceOfShape();
                for (const auto& e : edges) {
                    if (!e.IsNull()) edgeSeq->Append(e);
                }
                
                Handle(TopTools_HSequenceOfShape) wireSeq = new TopTools_HSequenceOfShape();
                ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edgeSeq, 0.1, Standard_False, wireSeq);
                
                if (wireSeq->Length() >= 1) {
                    wire = TopoDS::Wire(wireSeq->Value(1));
                    if (!wire.IsNull()) {
                        wireOk = true;
                        std::cout << "  → ConnectEdgesToWires(0.1): OK (" << wireSeq->Length() << " wire(s))" << std::endl;
                    }
                }
            } catch (...) {
                std::cout << "  → ConnectEdgesToWires: EXCEPTION" << std::endl;
            }
        }
        
        // Method 3: Wider tolerance (last resort)
        if (!wireOk) {
            try {
                Handle(TopTools_HSequenceOfShape) edgeSeq = new TopTools_HSequenceOfShape();
                for (const auto& e : edges) {
                    if (!e.IsNull()) edgeSeq->Append(e);
                }
                
                Handle(TopTools_HSequenceOfShape) wireSeq = new TopTools_HSequenceOfShape();
                ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edgeSeq, 1.0, Standard_False, wireSeq);
                
                if (wireSeq->Length() >= 1) {
                    wire = TopoDS::Wire(wireSeq->Value(1));
                    if (!wire.IsNull()) {
                        wireOk = true;
                        std::cout << "  → ConnectEdgesToWires(1.0): OK" << std::endl;
                    }
                }
            } catch (...) {}
        }
        
        if (!wireOk || wire.IsNull()) {
            std::cout << "  → SKIPPED: could not build wire" << std::endl;
            continue;
        }
        
        // Build face from wire
        try {
            BRepBuilderAPI_MakeFace faceMaker(plane, wire, Standard_True);
            if (faceMaker.IsDone()) {
                TopoDS_Face face = faceMaker.Face();
                
                GProp_GProps props;
                BRepGProp::SurfaceProperties(face, props);
                double area = props.Mass();
                
                if (area > 1.0) {
                    ClosedProfile prof;
                    prof.wire = wire;
                    prof.face = face;
                    prof.entityIndex = idx;
                    
                    // Build description
                    if (type == SketchEntityType::Circle) {
                        auto c = std::dynamic_pointer_cast<SketchCircle>(entity);
                        prof.description = c ? "Cercle R" + std::to_string((int)c->getRadius()) : "Cercle";
                    } else if (type == SketchEntityType::Ellipse) {
                        prof.description = "Ellipse";
                    } else if (type == SketchEntityType::Rectangle) {
                        auto r = std::dynamic_pointer_cast<SketchRectangle>(entity);
                        prof.description = r ? "Rectangle " + std::to_string((int)r->getWidth()) + "x" + std::to_string((int)r->getHeight()) : "Rectangle";
                    } else if (type == SketchEntityType::Polygon) {
                        auto p = std::dynamic_pointer_cast<SketchPolygon>(entity);
                        prof.description = p ? "Polygone " + std::to_string(p->getSides()) + " côtés" : "Polygone";
                    } else if (type == SketchEntityType::Oblong) {
                        auto o = std::dynamic_pointer_cast<SketchOblong>(entity);
                        prof.description = o ? "Oblong " + std::to_string((int)o->getLength()) + "x" + std::to_string((int)o->getWidth()) : "Oblong";
                    }
                    
                    profiles.push_back(prof);
                    closedEntityIndices.push_back(idx);
                }
            }
        } catch (...) {}
    }
    
    // ========================================================================
    // PHASE 2: Try to connect remaining loose edges (lines, arcs, polylines)
    //          into additional closed wires
    // ========================================================================
    std::vector<TopoDS_Edge> looseEdges;
    
    for (int idx = 0; idx < (int)m_entities.size(); idx++) {
        // Skip entities already handled in phase 1
        if (std::find(closedEntityIndices.begin(), closedEntityIndices.end(), idx) != closedEntityIndices.end())
            continue;
        // Skip construction geometry (axes)
        if (m_entities[idx]->isConstruction()) continue;
        
        auto edges = m_entities[idx]->toEdges3D(plane);
        for (const auto& e : edges) {
            if (!e.IsNull()) looseEdges.push_back(e);
        }
    }
    
    if (!looseEdges.empty()) {
        // Try connecting loose edges into wires
        Handle(TopTools_HSequenceOfShape) edgeSeq = new TopTools_HSequenceOfShape();
        for (const auto& e : looseEdges) {
            edgeSeq->Append(e);
        }
        
        Handle(TopTools_HSequenceOfShape) wireSeq = new TopTools_HSequenceOfShape();
        ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edgeSeq, 1.0, Standard_False, wireSeq);
        
        for (int i = 1; i <= wireSeq->Length(); i++) {
            TopoDS_Wire w = TopoDS::Wire(wireSeq->Value(i));
            if (w.IsNull()) continue;
            
            try {
                BRepBuilderAPI_MakeFace faceMaker(plane, w, Standard_True);
                if (faceMaker.IsDone()) {
                    TopoDS_Face face = faceMaker.Face();
                    GProp_GProps props;
                    BRepGProp::SurfaceProperties(face, props);
                    double area = props.Mass();
                    
                    if (area > 1.0) {
                        ClosedProfile prof;
                        prof.wire = w;
                        prof.face = face;
                        prof.entityIndex = -1;
                        
                        int edgeCount = 0;
                        TopExp_Explorer wexp(w, TopAbs_EDGE);
                        for (; wexp.More(); wexp.Next()) edgeCount++;
                        prof.description = "Profil (" + std::to_string(edgeCount) + " arêtes)";
                        
                        profiles.push_back(prof);
                    }
                }
            } catch (...) {}
        }
    }
    
    // ========================================================================
    // PHASE 3: If there are intersections between entities, try BOPAlgo to
    //          split edges and find additional closed regions
    // ========================================================================
    if (m_entities.size() > 1) {
        // Collect ALL edges (including from closed entities) for splitting
        std::vector<TopoDS_Edge> allEdges;
        for (const auto& entity : m_entities) {
            if (entity->isConstruction()) continue;
            auto edges = entity->toEdges3D(plane);
            for (const auto& e : edges) {
                if (!e.IsNull()) allEdges.push_back(e);
            }
        }
        
        if (allEdges.size() >= 2) {
            std::vector<TopoDS_Edge> splitEdges;
            try {
                BOPAlgo_Builder builder;
                for (const auto& e : allEdges) {
                    builder.AddArgument(e);
                }
                builder.Perform();
                
                if (!builder.HasErrors()) {
                    const TopoDS_Shape& result = builder.Shape();
                    TopExp_Explorer exp(result, TopAbs_EDGE);
                    for (; exp.More(); exp.Next()) {
                        splitEdges.push_back(TopoDS::Edge(exp.Current()));
                    }
                }
            } catch (...) {}
            
            // Only proceed if BOPAlgo produced MORE edges than input (i.e. it actually split something)
            if (splitEdges.size() > allEdges.size()) {
                Handle(TopTools_HSequenceOfShape) edgeSeq = new TopTools_HSequenceOfShape();
                for (const auto& e : splitEdges) {
                    edgeSeq->Append(e);
                }
                
                Handle(TopTools_HSequenceOfShape) wireSeq = new TopTools_HSequenceOfShape();
                ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edgeSeq, 0.5, Standard_False, wireSeq);
                
                for (int i = 1; i <= wireSeq->Length(); i++) {
                    TopoDS_Wire w = TopoDS::Wire(wireSeq->Value(i));
                    if (w.IsNull()) continue;
                    
                    try {
                        BRepBuilderAPI_MakeFace faceMaker(plane, w, Standard_True);
                        if (faceMaker.IsDone()) {
                            TopoDS_Face face = faceMaker.Face();
                            GProp_GProps props;
                            BRepGProp::SurfaceProperties(face, props);
                            double area = props.Mass();
                            
                            if (area > 1.0) {
                                // Check it's not a duplicate of an existing profile
                                bool isDuplicate = false;
                                for (const auto& existing : profiles) {
                                    GProp_GProps existProps;
                                    BRepGProp::SurfaceProperties(existing.face, existProps);
                                    double existArea = existProps.Mass();
                                    if (std::abs(area - existArea) < 1.0) {
                                        // Similar area — check centroid distance
                                        gp_Pnt c1 = props.CentreOfMass();
                                        gp_Pnt c2 = existProps.CentreOfMass();
                                        if (c1.Distance(c2) < 1.0) {
                                            isDuplicate = true;
                                            break;
                                        }
                                    }
                                }
                                
                                if (!isDuplicate) {
                                    ClosedProfile prof;
                                    prof.wire = w;
                                    prof.face = face;
                                    prof.entityIndex = -1;
                                    
                                    int edgeCount = 0;
                                    TopExp_Explorer wexp(w, TopAbs_EDGE);
                                    for (; wexp.More(); wexp.Next()) edgeCount++;
                                    prof.description = "Région (" + std::to_string(edgeCount) + " arêtes)";
                                    
                                    profiles.push_back(prof);
                                }
                            }
                        }
                    } catch (...) {}
                }
            }
        }
    }
    
    return profiles;
}

// ============================================================================
// Build Face Regions — handles nested profiles (Fusion 360 style)
// ============================================================================

std::vector<Sketch2D::FaceRegion> Sketch2D::buildFaceRegions() const {
    std::vector<FaceRegion> regions;
    auto profiles = detectClosedProfiles();
    
    if (profiles.empty()) return regions;
    
    gp_Pln plane = getPlane();
    
    std::cout << "[buildFaceRegions] " << profiles.size() << " profiles detected" << std::endl;
    
    // If only 1 profile, just return it directly
    if (profiles.size() == 1) {
        if (!profiles[0].face.IsNull()) {
            FaceRegion region;
            region.face = profiles[0].face;
            region.description = profiles[0].description;
            GProp_GProps props;
            BRepGProp::SurfaceProperties(region.face, props);
            region.area = props.Mass();
            regions.push_back(region);
        }
        return regions;
    }
    
    // Multiple profiles — compute areas, centroids, detect nesting
    struct ProfileInfo {
        int index;
        double area;
        TopoDS_Wire wire;
        TopoDS_Face face;
        std::string description;
        gp_Pnt2d centroid2D;  // In sketch local coords
    };
    
    std::vector<ProfileInfo> sorted;
    for (int i = 0; i < (int)profiles.size(); i++) {
        if (profiles[i].face.IsNull() || profiles[i].wire.IsNull()) continue;
        
        ProfileInfo pi;
        pi.index = i;
        pi.wire = profiles[i].wire;
        pi.face = profiles[i].face;
        pi.description = profiles[i].description;
        
        GProp_GProps props;
        BRepGProp::SurfaceProperties(pi.face, props);
        pi.area = props.Mass();
        
        // Project 3D centroid to 2D sketch local coordinates
        gp_Pnt c3d = props.CentreOfMass();
        gp_Vec fromOrigin(plane.Location(), c3d);
        double u = fromOrigin.Dot(gp_Vec(plane.Position().XDirection()));
        double v = fromOrigin.Dot(gp_Vec(plane.Position().YDirection()));
        pi.centroid2D = gp_Pnt2d(u, v);
        
        std::cout << "  Profile " << i << ": " << pi.description 
                  << " area=" << pi.area 
                  << " center2D=(" << u << "," << v << ")" << std::endl;
        
        sorted.push_back(pi);
    }
    
    // Sort largest first
    std::sort(sorted.begin(), sorted.end(), 
        [](const ProfileInfo& a, const ProfileInfo& b) { return a.area > b.area; });
    
    // ========================================================================
    // 2D containment test: check if point is inside a closed profile
    // ========================================================================
    auto isPointInsideProfile = [&](const gp_Pnt2d& testPt, const ProfileInfo& outer) -> bool {
        // Build test point in 3D on the sketch plane
        gp_Pnt origin = plane.Location();
        gp_Vec xVec(plane.Position().XDirection());
        gp_Vec yVec(plane.Position().YDirection());
        gp_Pnt testPt3D = origin.Translated(xVec.Multiplied(testPt.X()) + yVec.Multiplied(testPt.Y()));
        
        // Method 1: BRepClass_FaceClassifier with 3D point + tolerance
        try {
            BRepClass_FaceClassifier classifier(outer.face, testPt3D, 0.1);
            TopAbs_State state = classifier.State();
            if (state == TopAbs_IN) {
                std::cout << "    Containment(3D): IN" << std::endl;
                return true;
            }
            if (state == TopAbs_ON) {
                std::cout << "    Containment(3D): ON → treating as IN" << std::endl;
                return true;
            }
            std::cout << "    Containment(3D): OUT" << std::endl;
        } catch (...) {
            std::cout << "    Containment(3D): EXCEPTION" << std::endl;
        }
        
        // Method 2: Fallback with UV projection (for faces where 3D classifier fails)
        try {
            TopLoc_Location loc;
            Handle(Geom_Surface) surf = BRep_Tool::Surface(outer.face, loc);
            if (!surf.IsNull()) {
                GeomAPI_ProjectPointOnSurf projector(testPt3D, surf);
                if (projector.NbPoints() > 0) {
                    double uParam, vParam;
                    projector.LowerDistanceParameters(uParam, vParam);
                    gp_Pnt2d uvPt(uParam, vParam);
                    BRepClass_FaceClassifier classifier(outer.face, uvPt, 0.1);
                    TopAbs_State state = classifier.State();
                    if (state == TopAbs_IN || state == TopAbs_ON) {
                        std::cout << "    Containment(UV): IN" << std::endl;
                        return true;
                    }
                    std::cout << "    Containment(UV): state=" << (int)state << std::endl;
                }
            }
        } catch (...) {}
        
        // Method 3: Simple area-based heuristic
        // If inner area < 80% of outer area and centroids are close to outer center
        // it's probably inside (for simple nested cases like rect+oblong)
        try {
            GProp_GProps outerProps;
            BRepGProp::SurfaceProperties(outer.face, outerProps);
            gp_Pnt outerCenter = outerProps.CentreOfMass();
            
            gp_Pnt testPt3D2 = origin.Translated(xVec.Multiplied(testPt.X()) + yVec.Multiplied(testPt.Y()));
            double distToCenter = outerCenter.Distance(testPt3D2);
            
            // Estimate "radius" of outer profile from area
            double outerRadius = std::sqrt(outer.area / M_PI);
            
            if (distToCenter < outerRadius * 0.9) {
                std::cout << "    Containment(heuristic): dist=" << distToCenter 
                          << " < outerR*0.9=" << outerRadius*0.9 << " → IN" << std::endl;
                return true;
            }
        } catch (...) {}
        
        return false;
    };
    
    // Track which profiles are used as holes
    std::vector<bool> usedAsHole(sorted.size(), false);
    
    // For each profile (largest first), find smaller profiles inside it
    for (int i = 0; i < (int)sorted.size(); i++) {
        if (usedAsHole[i]) continue;
        
        std::vector<int> holeIndices;
        
        for (int j = i + 1; j < (int)sorted.size(); j++) {
            if (usedAsHole[j]) continue;
            if (sorted[j].area >= sorted[i].area * 0.95) continue; // Skip similar-size
            
            if (isPointInsideProfile(sorted[j].centroid2D, sorted[i])) {
                std::cout << "  → Profile " << sorted[j].index 
                          << " is INSIDE profile " << sorted[i].index << std::endl;
                holeIndices.push_back(j);
                usedAsHole[j] = true;
            }
        }
        
        // Build face with holes using BRepAlgoAPI_Cut (robust with arcs)
        bool faceBuilt = false;
        if (!holeIndices.empty()) {
            try {
                // Start with the outer face
                TopoDS_Shape currentFace = sorted[i].face;
                int holeCount = 0;
                
                for (int hi : holeIndices) {
                    // Extrude inner face to make a thin solid for cutting
                    // Alternative: use BRepAlgoAPI_Cut face-to-face
                    try {
                        BRepAlgoAPI_Cut cutter(currentFace, sorted[hi].face);
                        cutter.Build();
                        if (cutter.IsDone() && !cutter.Shape().IsNull()) {
                            // Extract face from cut result
                            TopExp_Explorer faceExp(cutter.Shape(), TopAbs_FACE);
                            if (faceExp.More()) {
                                currentFace = faceExp.Current();
                                holeCount++;
                                std::cout << "  → Cut hole " << holeCount 
                                          << " (" << sorted[hi].description << ") OK" << std::endl;
                            }
                        }
                    } catch (...) {
                        std::cout << "  → Cut hole FAILED for " << sorted[hi].description << std::endl;
                    }
                }
                
                if (holeCount > 0 && !currentFace.IsNull()) {
                    // Ensure we have a proper face
                    TopoDS_Face resultFace;
                    if (currentFace.ShapeType() == TopAbs_FACE) {
                        resultFace = TopoDS::Face(currentFace);
                    } else {
                        TopExp_Explorer fExp(currentFace, TopAbs_FACE);
                        if (fExp.More()) resultFace = TopoDS::Face(fExp.Current());
                    }
                    
                    if (!resultFace.IsNull()) {
                        // Tessellate for picking
                        BRepMesh_IncrementalMesh mesher(resultFace, 0.1, Standard_False, 0.3, Standard_True);
                        mesher.Perform();
                        
                        FaceRegion region;
                        region.face = resultFace;
                        region.area = sorted[i].area;
                        region.description = sorted[i].description;
                        region.description += " (" + std::to_string(holeCount) + 
                            " trou" + (holeCount > 1 ? "s" : "") + ")";
                        regions.push_back(region);
                        faceBuilt = true;
                    }
                }
            } catch (...) {
                std::cout << "  → MakeFace with holes FAILED completely" << std::endl;
            }
            
            // If BRepAlgoAPI_Cut failed, try the wire reversal approach as fallback
            if (!faceBuilt) {
                try {
                    BRepBuilderAPI_MakeFace faceMaker(plane, sorted[i].wire, Standard_True);
                    if (faceMaker.IsDone()) {
                        int holeCount = 0;
                        for (int hi : holeIndices) {
                            // Try adding inner wire reversed
                            TopoDS_Wire innerWire = TopoDS::Wire(sorted[hi].wire.Reversed());
                            faceMaker.Add(innerWire);
                            
                            if (faceMaker.IsDone()) {
                                holeCount++;
                                std::cout << "  → Wire reversal hole " << holeCount << " OK" << std::endl;
                            } else {
                                // Try with non-reversed wire
                                std::cout << "  → Wire reversed FAILED, trying non-reversed" << std::endl;
                                BRepBuilderAPI_MakeFace faceMaker2(plane, sorted[i].wire, Standard_True);
                                faceMaker2.Add(sorted[hi].wire);
                                if (faceMaker2.IsDone()) {
                                    faceMaker = faceMaker2;
                                    holeCount++;
                                    std::cout << "  → Wire non-reversed hole " << holeCount << " OK" << std::endl;
                                } else {
                                    std::cout << "  → Wire hole FAILED both ways" << std::endl;
                                }
                            }
                        }
                        
                        TopoDS_Face resultFace = faceMaker.Face();
                        BRepMesh_IncrementalMesh mesher(resultFace, 0.1, Standard_False, 0.3, Standard_True);
                        mesher.Perform();
                        
                        FaceRegion region;
                        region.face = resultFace;
                        region.area = sorted[i].area;
                        region.description = sorted[i].description;
                        if (holeCount > 0) {
                            region.description += " (" + std::to_string(holeCount) + 
                                " trou" + (holeCount > 1 ? "s" : "") + ")";
                        }
                        regions.push_back(region);
                        faceBuilt = true;
                        std::cout << "  → Wire reversal fallback: " << holeCount << " holes" << std::endl;
                    }
                } catch (...) {
                    std::cout << "  → Wire reversal fallback also FAILED" << std::endl;
                }
            }
        }
        
        // Final fallback: add outer profile without holes
        if (!faceBuilt) {
            FaceRegion region;
            region.face = sorted[i].face;
            region.area = sorted[i].area;
            region.description = sorted[i].description;
            regions.push_back(region);
        }
    }
    
    // Always add inner profiles as standalone selectable regions too
    for (int i = 0; i < (int)sorted.size(); i++) {
        if (!usedAsHole[i]) continue;
        
        FaceRegion region;
        region.face = sorted[i].face;
        region.area = sorted[i].area;
        region.description = sorted[i].description;
        regions.push_back(region);
    }
    
    std::cout << "[buildFaceRegions] Result: " << regions.size() << " region(s)" << std::endl;
    return regions;
}

// ============================================================================
// Dimensions
// ============================================================================

void Sketch2D::addDimension(std::shared_ptr<Dimension> dimension) {
    if (dimension) {
        m_dimensions.push_back(dimension);
    }
}

void Sketch2D::removeDimension(std::shared_ptr<Dimension> dimension) {
    auto it = std::find(m_dimensions.begin(), m_dimensions.end(), dimension);
    if (it != m_dimensions.end()) {
        m_dimensions.erase(it);
    }
}

void Sketch2D::clearDimensions() {
    m_dimensions.clear();
}

// ============================================================================
// Auto-dimensions (cotations automatiques sur toutes les entités)
// ============================================================================

void Sketch2D::createAxisConstructionLines() {
    // Ne pas recréer si elles existent déjà
    if (m_axisLineH && m_axisLineV) return;
    
    double axisLen = 10000.0;  // Très longues lignes (10m) pour couvrir tout le sketch
    
    // Axe horizontal (passe par l'origine)
    m_axisLineH = std::make_shared<SketchLine>(
        gp_Pnt2d(-axisLen, 0.0), gp_Pnt2d(axisLen, 0.0));
    m_axisLineH->setConstruction(true);
    addEntity(m_axisLineH);
    
    // Axe vertical (passe par l'origine)
    m_axisLineV = std::make_shared<SketchLine>(
        gp_Pnt2d(0.0, -axisLen), gp_Pnt2d(0.0, axisLen));
    m_axisLineV->setConstruction(true);
    addEntity(m_axisLineV);
}

void Sketch2D::regenerateAutoDimensions() {
    m_autoDimensions.clear();
    
    double autoOffset = 10.0;  // Offset standard pour auto-cotations (mm)
    
    // Helper: créer une cotation linéaire automatique
    auto makeAutoLinear = [&](const gp_Pnt2d& p1, const gp_Pnt2d& p2, double offset) 
        -> std::shared_ptr<LinearDimension> 
    {
        auto dim = std::make_shared<LinearDimension>(p1, p2);
        dim->setOffset(offset);
        dim->setAutomatic(true);
        return dim;
    };
    
    // Helper: appliquer l'offset persistant si disponible
    auto applyStoredOffset = [&](std::shared_ptr<Dimension> dim, int entityId, const std::string& prop, double defOffset) {
        if (dim->getType() == DimensionType::Linear) {
            auto ld = std::dynamic_pointer_cast<LinearDimension>(dim);
            if (ld) ld->setOffset(getAutoDimOffset(entityId, prop, defOffset));
        }
    };
    
    for (const auto& entity : m_entities) {
        // Skip construction geometry (axes)
        if (entity->isConstruction()) continue;
        
        // === LINE : longueur ===
        if (entity->getType() == SketchEntityType::Line) {
            auto line = std::dynamic_pointer_cast<SketchLine>(entity);
            if (line && line->getStart().Distance(line->getEnd()) > 0.1) {
                auto dim = makeAutoLinear(line->getStart(), line->getEnd(), autoOffset);
                dim->setAutoSource(entity, "length");
                m_autoDimensions.push_back(dim);
            }
        }
        // === CIRCLE : diamètre ===
        else if (entity->getType() == SketchEntityType::Circle) {
            auto circle = std::dynamic_pointer_cast<SketchCircle>(entity);
            if (circle && circle->getRadius() > 0.01) {
                auto dim = std::make_shared<DiametralDimension>(
                    circle->getCenter(), circle->getRadius() * 2.0);
                dim->setAutomatic(true);
                dim->setAutoSource(entity, "diameter");
                m_autoDimensions.push_back(dim);
            }
        }
        // === ARC : rayon ===
        else if (entity->getType() == SketchEntityType::Arc) {
            auto arc = std::dynamic_pointer_cast<SketchArc>(entity);
            if (arc) {
                gp_Pnt2d p1 = arc->getStart();
                gp_Pnt2d p2 = arc->getMid();
                gp_Pnt2d p3 = arc->getEnd();
                
                if (!arc->isBezier()) {
                    double ax = p2.X() - p1.X(), ay = p2.Y() - p1.Y();
                    double bx = p3.X() - p2.X(), by = p3.Y() - p2.Y();
                    double det = 2.0 * (ax * by - ay * bx);
                    
                    if (std::abs(det) > 1e-6) {
                        double aMag = ax*ax + ay*ay;
                        double bMag = bx*bx + by*by;
                        double cx = p1.X() + (aMag * by - bMag * ay) / det;
                        double cy = p1.Y() + (bMag * ax - aMag * bx) / det;
                        double radius = std::sqrt((p1.X()-cx)*(p1.X()-cx) + (p1.Y()-cy)*(p1.Y()-cy));
                        
                        gp_Pnt2d center(cx, cy);
                        auto dim = std::make_shared<RadialDimension>(center, radius);
                        dim->setAutomatic(true);
                        dim->setAutoSource(entity, "arcRadius");
                        dim->setArrowPoint(p2);
                        dim->setTextPosition(gp_Pnt2d(
                            (cx + p2.X()) / 2.0,
                            (cy + p2.Y()) / 2.0));
                        m_autoDimensions.push_back(dim);
                    }
                } else {
                    if (p1.Distance(p3) > 0.1) {
                        auto dim = makeAutoLinear(p1, p3, autoOffset);
                        dim->setAutoSource(entity, "chord");
                        m_autoDimensions.push_back(dim);
                    }
                }
            }
        }
        // === RECTANGLE : largeur + hauteur ===
        else if (entity->getType() == SketchEntityType::Rectangle) {
            auto rect = std::dynamic_pointer_cast<SketchRectangle>(entity);
            if (rect) {
                auto corners = rect->getKeyPoints();
                if (corners.size() >= 4) {
                    if (corners[0].Distance(corners[1]) > 0.1) {
                        auto dim = makeAutoLinear(corners[0], corners[1], -autoOffset);
                        dim->setAutoSource(entity, "width");
                        m_autoDimensions.push_back(dim);
                    }
                    if (corners[0].Distance(corners[3]) > 0.1) {
                        auto dim = makeAutoLinear(corners[0], corners[3], autoOffset);
                        dim->setAutoSource(entity, "height");
                        m_autoDimensions.push_back(dim);
                    }
                }
            }
        }
        // === POLYLINE : longueur de chaque segment ===
        else if (entity->getType() == SketchEntityType::Polyline) {
            auto poly = std::dynamic_pointer_cast<SketchPolyline>(entity);
            if (poly) {
                auto points = poly->getPoints();
                for (size_t i = 0; i + 1 < points.size(); ++i) {
                    if (points[i].Distance(points[i+1]) > 0.1) {
                        auto dim = makeAutoLinear(points[i], points[i+1], autoOffset);
                        dim->setAutoSource(entity, "segment");
                        dim->setSegmentIndex((int)i);
                        m_autoDimensions.push_back(dim);
                    }
                }
            }
        }
        // === ELLIPSE : grand diamètre + petit diamètre ===
        else if (entity->getType() == SketchEntityType::Ellipse) {
            auto ellipse = std::dynamic_pointer_cast<SketchEllipse>(entity);
            if (ellipse) {
                gp_Pnt2d center = ellipse->getCenter();
                double a = ellipse->getMajorRadius();
                double b = ellipse->getMinorRadius();
                double rot = ellipse->getRotation();
                double cosR = std::cos(rot), sinR = std::sin(rot);
                
                gp_Pnt2d majP1(center.X() - a * cosR, center.Y() - a * sinR);
                gp_Pnt2d majP2(center.X() + a * cosR, center.Y() + a * sinR);
                auto dimMaj = makeAutoLinear(majP1, majP2, autoOffset);
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(1) << (a * 2.0);
                dimMaj->setText(oss.str());
                dimMaj->setAutoSource(entity, "majorDiameter");
                m_autoDimensions.push_back(dimMaj);
                
                gp_Pnt2d minP1(center.X() - b * (-sinR), center.Y() - b * cosR);
                gp_Pnt2d minP2(center.X() + b * (-sinR), center.Y() + b * cosR);
                auto dimMin = makeAutoLinear(minP1, minP2, -autoOffset);
                std::ostringstream oss2;
                oss2 << std::fixed << std::setprecision(1) << (b * 2.0);
                dimMin->setText(oss2.str());
                dimMin->setAutoSource(entity, "minorDiameter");
                m_autoDimensions.push_back(dimMin);
            }
        }
        // === POLYGON : longueur d'un côté ===
        else if (entity->getType() == SketchEntityType::Polygon) {
            auto polygon = std::dynamic_pointer_cast<SketchPolygon>(entity);
            if (polygon && polygon->getSides() >= 3) {
                auto verts = polygon->getVertices();
                if (verts.size() >= 2 && verts[0].Distance(verts[1]) > 0.1) {
                    auto dim = makeAutoLinear(verts[0], verts[1], -autoOffset);
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(1) 
                        << verts[0].Distance(verts[1]) << " (x" << polygon->getSides() << ")";
                    dim->setText(oss.str());
                    dim->setAutoSource(entity, "sideLength");
                    m_autoDimensions.push_back(dim);
                }
            }
        }
        // === OBLONG : entraxe + largeur ===
        else if (entity->getType() == SketchEntityType::Oblong) {
            auto oblong = std::dynamic_pointer_cast<SketchOblong>(entity);
            if (oblong) {
                gp_Pnt2d c1 = oblong->getCenter1();
                gp_Pnt2d c2 = oblong->getCenter2();
                double width = oblong->getWidth();
                double rot = oblong->getRotation();
                gp_Pnt2d center = oblong->getCenter();
                
                if (c1.Distance(c2) > 0.1) {
                    auto dimEntr = makeAutoLinear(c1, c2, autoOffset);
                    dimEntr->setAutoSource(entity, "entraxe");
                    m_autoDimensions.push_back(dimEntr);
                }
                
                double perpX = -std::sin(rot);
                double perpY = std::cos(rot);
                gp_Pnt2d wP1(center.X() - (width/2.0) * perpX,
                             center.Y() - (width/2.0) * perpY);
                gp_Pnt2d wP2(center.X() + (width/2.0) * perpX,
                             center.Y() + (width/2.0) * perpY);
                auto dimW = makeAutoLinear(wP1, wP2, -autoOffset);
                dimW->setAutoSource(entity, "oblongWidth");
                m_autoDimensions.push_back(dimW);
            }
        }
    }
    
    // Appliquer les offsets persistants (stockés par l'utilisateur via Ctrl+drag)
    for (auto& dim : m_autoDimensions) {
        auto srcEntity = dim->getAutoSourceEntity();
        if (!srcEntity) continue;
        int eid = srcEntity->getId();
        std::string prop = dim->getAutoSourceProperty();
        
        if (dim->getType() == DimensionType::Linear) {
            auto ld = std::dynamic_pointer_cast<LinearDimension>(dim);
            if (ld) {
                double stored = getAutoDimOffset(eid, prop, ld->getOffset());
                ld->setOffset(stored);
            }
        }
        else if (dim->getType() == DimensionType::Diametral) {
            auto dd = std::dynamic_pointer_cast<DiametralDimension>(dim);
            if (dd) {
                double stored = getAutoDimAngle(eid, prop, dd->getAngle());
                dd->setAngle(stored);
            }
        }
        else if (dim->getType() == DimensionType::Radial) {
            auto rd = std::dynamic_pointer_cast<RadialDimension>(dim);
            if (rd) {
                double stored = getAutoDimAngle(eid, prop, 0.0);
                if (m_autoDimAngles.count(std::to_string(eid) + ":" + prop)) {
                    // Recalculer arrow point à partir de l'angle stocké
                    double r = rd->getRadius();
                    gp_Pnt2d c = rd->getCenter();
                    double angleRad = stored * M_PI / 180.0;
                    rd->setArrowPoint(gp_Pnt2d(c.X() + r * std::cos(angleRad),
                                               c.Y() + r * std::sin(angleRad)));
                    rd->setTextPosition(gp_Pnt2d(
                        (c.X() + rd->getArrowPoint().X()) / 2.0,
                        (c.Y() + rd->getArrowPoint().Y()) / 2.0));
                }
            }
        }
    }
}

void Sketch2D::setAutoDimOffset(int entityId, const std::string& property, double offset) {
    std::string key = std::to_string(entityId) + ":" + property;
    m_autoDimOffsets[key] = offset;
}

double Sketch2D::getAutoDimOffset(int entityId, const std::string& property, double defaultOffset) const {
    std::string key = std::to_string(entityId) + ":" + property;
    auto it = m_autoDimOffsets.find(key);
    if (it != m_autoDimOffsets.end()) return it->second;
    return defaultOffset;
}

void Sketch2D::setAutoDimAngle(int entityId, const std::string& property, double angle) {
    std::string key = std::to_string(entityId) + ":" + property;
    m_autoDimAngles[key] = angle;
}

double Sketch2D::getAutoDimAngle(int entityId, const std::string& property, double defaultAngle) const {
    std::string key = std::to_string(entityId) + ":" + property;
    auto it = m_autoDimAngles.find(key);
    if (it != m_autoDimAngles.end()) return it->second;
    return defaultAngle;
}

// ============================================================================
// Contraintes géométriques
// ============================================================================

void Sketch2D::addConstraint(std::shared_ptr<GeometricConstraint> constraint) {
    if (constraint) {
        m_constraints.push_back(constraint);
    }
}

void Sketch2D::removeConstraint(std::shared_ptr<GeometricConstraint> constraint) {
    auto it = std::find(m_constraints.begin(), m_constraints.end(), constraint);
    if (it != m_constraints.end()) {
        m_constraints.erase(it);
    }
}

void Sketch2D::clearConstraints() {
    m_constraints.clear();
}

void Sketch2D::solveConstraints() {
    // Solveur simple : application itérative par priorité
    // On applique les contraintes plusieurs fois jusqu'à convergence
    
    const int maxIterations = 10;
    const double tolerance = 1e-6;
    
    for (int iter = 0; iter < maxIterations; iter++) {
        bool allSatisfied = true;
        
        // Trier par priorité (0 = haute priorité)
        auto sortedConstraints = m_constraints;
        std::sort(sortedConstraints.begin(), sortedConstraints.end(),
            [](const auto& a, const auto& b) {
                return a->getPriority() < b->getPriority();
            });
        
        // Appliquer chaque contrainte active
        for (const auto& constraint : sortedConstraints) {
            if (!constraint->isActive()) continue;
            
            if (!constraint->isSatisfied(tolerance)) {
                constraint->apply();
                allSatisfied = false;
            }
        }
        
        // Si toutes les contraintes sont satisfaites, on arrête
        if (allSatisfied) break;
    }
}

} // namespace CADEngine

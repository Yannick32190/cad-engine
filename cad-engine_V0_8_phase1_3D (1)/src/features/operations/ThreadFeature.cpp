#include "ThreadFeature.h"

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepOffsetAPI_MakePipeShell.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <ShapeFix_Shape.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Curve.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>
#include <TopoDS.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListOfShape.hxx>

#include <iostream>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace CADEngine {

ThreadFeature::ThreadFeature(const std::string& name)
    : Feature(FeatureType::Unknown, name)
{
    setInt("thread_type", 0);
    setInt("mode", 0);
    setBool("left_hand", false);
    setDouble("diameter", 10.0);
    setDouble("pitch", 1.5);
    setDouble("depth", 0.0);
    setDouble("length", 20.0);
}

void ThreadFeature::setBaseShape(const TopoDS_Shape& s) { m_baseShape = s; invalidate(); }
void ThreadFeature::setCylindricalFace(const TopoDS_Face& f) { m_cylindricalFace = f; invalidate(); }
void ThreadFeature::setThreadType(ThreadType t) { setInt("thread_type", (int)t); invalidate(); }
void ThreadFeature::setMode(ThreadMode m) { setInt("mode", (int)m); invalidate(); }
void ThreadFeature::setLeftHand(bool l) { setBool("left_hand", l); invalidate(); }
void ThreadFeature::setDiameter(double d) { setDouble("diameter", d); invalidate(); }
void ThreadFeature::setPitch(double p) { setDouble("pitch", p); invalidate(); }
void ThreadFeature::setDepth(double d) { setDouble("depth", d); invalidate(); }
void ThreadFeature::setLength(double l) { setDouble("length", l); invalidate(); }

ThreadType ThreadFeature::getThreadType() const { return (ThreadType)getInt("thread_type"); }
ThreadMode ThreadFeature::getMode() const { return (ThreadMode)getInt("mode"); }
bool ThreadFeature::isLeftHand() const { return getBool("left_hand"); }
double ThreadFeature::getDiameter() const { return getDouble("diameter"); }
double ThreadFeature::getPitch() const { return getDouble("pitch"); }
double ThreadFeature::getDepth() const { return getDouble("depth"); }
double ThreadFeature::getLength() const { return getDouble("length"); }

double ThreadFeature::getStandardPitch(double d, ThreadType type) {
    if (type == ThreadType::MetricCoarse) {
        if (d <= 3) return 0.5;   if (d <= 4) return 0.7;
        if (d <= 5) return 0.8;   if (d <= 6) return 1.0;
        if (d <= 8) return 1.25;  if (d <= 10) return 1.5;
        if (d <= 12) return 1.75; if (d <= 16) return 2.0;
        if (d <= 20) return 2.5;  if (d <= 24) return 3.0;
        if (d <= 30) return 3.5;  return 4.0;
    } else if (type == ThreadType::MetricFine) {
        if (d <= 3) return 0.35;  if (d <= 6) return 0.5;
        if (d <= 8) return 0.75;  if (d <= 10) return 1.0;
        if (d <= 12) return 1.25; if (d <= 16) return 1.5;
        if (d <= 24) return 2.0;  return 2.0;
    }
    return 1.5;
}

double ThreadFeature::getStandardDepth(double pitch) {
    return pitch * std::sqrt(3.0) / 2.0 * 5.0 / 8.0;
}

gp_Ax1 ThreadFeature::getCylinderAxis() const {
    if (!m_cylindricalFace.IsNull()) {
        try {
            BRepAdaptor_Surface s(m_cylindricalFace);
            if (s.GetType() == GeomAbs_Cylinder)
                return s.Cylinder().Axis();
        } catch (...) {}
    }
    return gp_Ax1(gp_Pnt(0,0,0), gp_Dir(0,0,1));
}

double ThreadFeature::getCylinderRadius() const {
    if (!m_cylindricalFace.IsNull()) {
        try {
            BRepAdaptor_Surface s(m_cylindricalFace);
            if (s.GetType() == GeomAbs_Cylinder)
                return s.Cylinder().Radius();
        } catch (...) {}
    }
    return getDiameter() / 2.0;
}

double ThreadFeature::getCylinderVMin() const {
    if (!m_cylindricalFace.IsNull()) {
        try {
            BRepAdaptor_Surface s(m_cylindricalFace);
            double v = s.FirstVParameter();
            if (std::abs(v) < 1e6) return v;
        } catch (...) {}
    }
    return 0.0;
}

// ─── Fallback : révolution (anneaux) ────────────────────────
TopoDS_Shape ThreadFeature::buildRevolutionFallback(
    const gp_Ax1& axis, const gp_Dir& radDir, const gp_Dir& axDir,
    const gp_Pnt& start, double R, double Rtooth, double Rclose,
    double pitch, double length, int nTeeth) const
{
    auto pt3d = [&](double r, double z) -> gp_Pnt {
        return gp_Pnt(start.XYZ()
            + r * gp_Vec(radDir).XYZ()
            + z * gp_Vec(axDir).XYZ());
    };
    
    double zEnd = nTeeth * pitch;
    std::vector<gp_Pnt> pts;
    
    pts.push_back(pt3d(Rclose, 0));
    pts.push_back(pt3d(R, 0));
    for (int i = 0; i < nTeeth; i++) {
        double z0 = i * pitch;
        pts.push_back(pt3d(Rtooth, z0 + pitch * 0.5));
        pts.push_back(pt3d(R, z0 + pitch));
    }
    pts.push_back(pt3d(Rclose, zEnd));
    
    if (pts.size() < 4) return TopoDS_Shape();
    
    BRepBuilderAPI_MakeWire wbuilder;
    for (size_t i = 0; i < pts.size(); i++) {
        size_t j = (i + 1) % pts.size();
        if (pts[i].Distance(pts[j]) < 1e-7) continue;
        try {
            wbuilder.Add(BRepBuilderAPI_MakeEdge(pts[i], pts[j]).Edge());
        } catch (...) { return TopoDS_Shape(); }
    }
    if (!wbuilder.IsDone()) return TopoDS_Shape();
    
    BRepBuilderAPI_MakeFace fbuilder(wbuilder.Wire());
    if (!fbuilder.IsDone()) return TopoDS_Shape();
    
    try {
        BRepPrimAPI_MakeRevol revol(fbuilder.Face(), axis, 2.0 * M_PI);
        revol.Build();
        if (revol.IsDone()) {
            std::cout << "[Thread] Revolution fallback OK" << std::endl;
            return revol.Shape();
        }
    } catch (...) {}
    return TopoDS_Shape();
}

// ─── Méthode principale : sweep hélicoïdal (méthode FreeCAD) ─
TopoDS_Shape ThreadFeature::buildThreadSolid() const {
    double pitch = getPitch();
    double length = getLength();
    double depth = getDepth();
    if (depth <= 0.0) depth = getStandardDepth(pitch);
    
    double faceR = getCylinderRadius();
    ThreadMode mode = getMode();
    bool leftHand = isLeftHand();
    
    gp_Ax1 axis = getCylinderAxis();
    gp_Pnt axOrig = axis.Location();
    gp_Dir axDir = axis.Direction();
    
    gp_Dir xDir;
    if (std::abs(axDir.Z()) < 0.99)
        xDir = gp_Dir(gp_Vec(0,0,1).Crossed(gp_Vec(axDir)));
    else
        xDir = gp_Dir(gp_Vec(1,0,0).Crossed(gp_Vec(axDir)));
    gp_Dir yDir = gp_Dir(gp_Vec(axDir).Crossed(gp_Vec(xDir)));
    
    // Détecter trou vs fût
    bool isHole = false;
    if (!m_cylindricalFace.IsNull()) {
        try {
            BRepAdaptor_Surface surf(m_cylindricalFace);
            double uMid = (surf.FirstUParameter() + surf.LastUParameter()) / 2.0;
            double vMid = (surf.FirstVParameter() + surf.LastVParameter()) / 2.0;
            gp_Pnt ptS; gp_Vec d1u, d1v;
            surf.D1(uMid, vMid, ptS, d1u, d1v);
            gp_Vec fn = d1u.Crossed(d1v);
            if (m_cylindricalFace.Orientation() == TopAbs_REVERSED) fn.Reverse();
            gp_Vec toS(axOrig, ptS);
            gp_Vec axV(axDir);
            gp_Vec rad = toS - axV * toS.Dot(axV);
            if (rad.Magnitude() > 1e-6 && fn.Dot(rad) < 0) isHole = true;
        } catch (...) {}
    }
    
    bool teethOutward;
    if (mode == ThreadMode::RemoveMaterial)
        teethOutward = isHole;
    else
        teethOutward = !isHole;
    
    double vMin = getCylinderVMin();
    gp_Pnt start = axOrig.XYZ() + vMin * gp_Vec(axDir).XYZ();
    
    int nTurns = std::max(1, (int)std::ceil(length / pitch));
    if (nTurns > 40) nTurns = 40;
    
    double R = faceR;
    double Rtooth = teethOutward ? (R + depth) : (R - depth);
    double Rclose = teethOutward ? std::max(0.1, R - depth * 0.5) : (R + depth * 0.5);
    double dirSign = leftHand ? -1.0 : 1.0;
    int nTeeth = std::max(1, (int)(length / pitch));
    if (nTeeth > 200) nTeeth = 200;
    
    std::cout << "[Thread] " << (isHole?"HOLE":"SHAFT")
              << " " << (mode==ThreadMode::RemoveMaterial?"CUT":"ADD")
              << " R=" << R << " depth=" << depth
              << " teethOut=" << teethOutward << std::endl;
    
    // ── 1. Hélice BSpline ──
    // 48 points par tour pour une BSpline très lisse (C2)
    int ptsPerTurn = 48;
    int totalPts = nTurns * ptsPerTurn + 1;
    if (totalPts > 2000) totalPts = 2000;
    
    TColgp_Array1OfPnt helixArr(1, totalPts);
    for (int i = 0; i < totalPts; i++) {
        double t = (double)i / ptsPerTurn;
        double angle = t * 2.0 * M_PI * dirSign;
        double zOff = t * pitch;
        gp_XYZ pt = start.XYZ()
            + R * cos(angle) * gp_Vec(xDir).XYZ()
            + R * sin(angle) * gp_Vec(yDir).XYZ()
            + zOff * gp_Vec(axDir).XYZ();
        helixArr.SetValue(i + 1, gp_Pnt(pt));
    }
    
    Handle(Geom_BSplineCurve) helixCurve;
    try {
        GeomAPI_PointsToBSpline fitter(helixArr, 3, 8, GeomAbs_C2, 1e-4);
        if (!fitter.IsDone()) {
            std::cerr << "[Thread] BSpline fit failed → revolution" << std::endl;
            return buildRevolutionFallback(axis, xDir, axDir, start,
                R, Rtooth, Rclose, pitch, length, nTeeth);
        }
        helixCurve = fitter.Curve();
    } catch (...) {
        std::cerr << "[Thread] BSpline exception → revolution" << std::endl;
        return buildRevolutionFallback(axis, xDir, axDir, start,
            R, Rtooth, Rclose, pitch, length, nTeeth);
    }
    
    Handle(Geom_Curve) helixGeom = helixCurve;
    BRepBuilderAPI_MakeEdge helixEdge(helixGeom);
    if (!helixEdge.IsDone()) {
        std::cerr << "[Thread] MakeEdge failed → revolution" << std::endl;
        return buildRevolutionFallback(axis, xDir, axDir, start,
            R, Rtooth, Rclose, pitch, length, nTeeth);
    }
    
    BRepBuilderAPI_MakeWire helixWire;
    helixWire.Add(helixEdge.Edge());
    if (!helixWire.IsDone()) {
        std::cerr << "[Thread] MakeWire failed → revolution" << std::endl;
        return buildRevolutionFallback(axis, xDir, axDir, start,
            R, Rtooth, Rclose, pitch, length, nTeeth);
    }
    
    std::cout << "[Thread] Helix BSpline OK (" << totalPts << " pts)" << std::endl;
    
    // ── 2. Profil triangulaire au point de départ ──
    // Pour une hélice, Frenet Normal pointe vers le centre = -radial
    // Donc la dent radiale vers l'extérieur = direction -Normal = xDir
    gp_Pnt P0(start.XYZ() + R * gp_Vec(xDir).XYZ());
    
    double halfW = pitch * 0.28;
    double overlap = depth * 0.35;
    
    gp_Pnt tip, b1, b2;
    if (teethOutward) {
        tip = gp_Pnt(P0.XYZ() + depth * gp_Vec(xDir).XYZ());
        b1  = gp_Pnt(P0.XYZ() - overlap * gp_Vec(xDir).XYZ() + halfW * gp_Vec(axDir).XYZ());
        b2  = gp_Pnt(P0.XYZ() - overlap * gp_Vec(xDir).XYZ() - halfW * gp_Vec(axDir).XYZ());
    } else {
        tip = gp_Pnt(P0.XYZ() - depth * gp_Vec(xDir).XYZ());
        b1  = gp_Pnt(P0.XYZ() + overlap * gp_Vec(xDir).XYZ() + halfW * gp_Vec(axDir).XYZ());
        b2  = gp_Pnt(P0.XYZ() + overlap * gp_Vec(xDir).XYZ() - halfW * gp_Vec(axDir).XYZ());
    }
    
    if (tip.Distance(b1) < 1e-6) {
        return buildRevolutionFallback(axis, xDir, axDir, start,
            R, Rtooth, Rclose, pitch, length, nTeeth);
    }
    
    BRepBuilderAPI_MakeWire pw;
    pw.Add(BRepBuilderAPI_MakeEdge(tip, b1).Edge());
    pw.Add(BRepBuilderAPI_MakeEdge(b1, b2).Edge());
    pw.Add(BRepBuilderAPI_MakeEdge(b2, tip).Edge());
    if (!pw.IsDone()) {
        return buildRevolutionFallback(axis, xDir, axDir, start,
            R, Rtooth, Rclose, pitch, length, nTeeth);
    }
    
    // ── 3. Sweep hélicoïdal (méthode FreeCAD) ──
    // MakePipeShell avec Frenet mode
    // Pour une hélice : N pointe vers centre, B ≈ axe
    // Le profil est balayé le long de l'hélice avec une orientation stable
    try {
        BRepOffsetAPI_MakePipeShell sweeper(helixWire.Wire());
        sweeper.SetMode(Standard_True);  // Frenet mode (comme FreeCAD)
        sweeper.Add(pw.Wire());
        sweeper.Build();
        
        if (!sweeper.IsDone()) {
            std::cerr << "[Thread] PipeShell Build failed → revolution" << std::endl;
            return buildRevolutionFallback(axis, xDir, axDir, start,
                R, Rtooth, Rclose, pitch, length, nTeeth);
        }
        
        std::cout << "[Thread] PipeShell Build OK" << std::endl;
        
        // Fermer le solide
        if (sweeper.MakeSolid()) {
            TopoDS_Shape result = sweeper.Shape();
            int nS = 0, nF = 0;
            for (TopExp_Explorer ex(result, TopAbs_SOLID); ex.More(); ex.Next()) nS++;
            for (TopExp_Explorer ex(result, TopAbs_FACE); ex.More(); ex.Next()) nF++;
            std::cout << "[Thread] Sweep SOLID OK: " << nS << " solids, " << nF << " faces" << std::endl;
            if (nS > 0) {
                // Valider le solide avant de l'utiliser dans le booléen
                BRepCheck_Analyzer sweepCheck(result);
                if (!sweepCheck.IsValid()) {
                    std::cout << "[Thread] Sweep solid invalid, trying fix..." << std::endl;
                    try {
                        ShapeFix_Shape fixer(result);
                        fixer.Perform();
                        result = fixer.Shape();
                        BRepCheck_Analyzer sweepCheck2(result);
                        if (!sweepCheck2.IsValid()) {
                            std::cout << "[Thread] Sweep still invalid → revolution" << std::endl;
                            return buildRevolutionFallback(axis, xDir, axDir, start,
                                R, Rtooth, Rclose, pitch, length, nTeeth);
                        }
                    } catch (...) {
                        return buildRevolutionFallback(axis, xDir, axDir, start,
                            R, Rtooth, Rclose, pitch, length, nTeeth);
                    }
                }
                return result;
            }
        }
        
        std::cout << "[Thread] MakeSolid failed → revolution" << std::endl;
        return buildRevolutionFallback(axis, xDir, axDir, start,
            R, Rtooth, Rclose, pitch, length, nTeeth);
        
    } catch (Standard_Failure& e) {
        std::cerr << "[Thread] PipeShell: " << e.GetMessageString() << " → revolution" << std::endl;
        return buildRevolutionFallback(axis, xDir, axDir, start,
            R, Rtooth, Rclose, pitch, length, nTeeth);
    } catch (...) {
        std::cerr << "[Thread] PipeShell crash → revolution" << std::endl;
        return buildRevolutionFallback(axis, xDir, axDir, start,
            R, Rtooth, Rclose, pitch, length, nTeeth);
    }
}

// ─── Compute ────────────────────────────────────────────────
bool ThreadFeature::compute() {
    ThreadMode mode = getMode();
    std::cout << "[Thread] === M" << getDiameter() << "x" << getPitch()
              << " L=" << getLength() << " "
              << (mode == ThreadMode::RemoveMaterial ? "CUT" : "ADD")
              << (isLeftHand() ? " LEFT" : " RIGHT") << " ===" << std::endl;
    
    if (m_baseShape.IsNull()) {
        std::cerr << "[Thread] No base shape" << std::endl;
        return false;
    }
    
    int baseFaces = 0;
    for (TopExp_Explorer ex(m_baseShape, TopAbs_FACE); ex.More(); ex.Next())
        baseFaces++;
    
    try {
        TopoDS_Shape tool = buildThreadSolid();
        
        if (tool.IsNull()) {
            std::cout << "[Thread] Build null — cosmetic" << std::endl;
            m_resultShape = m_baseShape;
            m_shape = m_baseShape;
            m_upToDate = true;
            return true;
        }
        
        TopoDS_Shape result;
        bool opOk = false;
        
        if (mode == ThreadMode::RemoveMaterial) {
            BRepAlgoAPI_Cut op;
            TopTools_ListOfShape args, tools;
            args.Append(m_baseShape);
            tools.Append(tool);
            op.SetArguments(args);
            op.SetTools(tools);
            op.SetFuzzyValue(0.01);
            op.Build();
            if (op.IsDone() && !op.Shape().IsNull()) {
                result = op.Shape();
                opOk = true;
            }
            if (!opOk) {
                BRepAlgoAPI_Cut op2(m_baseShape, tool);
                if (op2.IsDone() && !op2.Shape().IsNull()) {
                    result = op2.Shape();
                    opOk = true;
                }
            }
        } else {
            BRepAlgoAPI_Fuse op;
            TopTools_ListOfShape args, tools;
            args.Append(m_baseShape);
            tools.Append(tool);
            op.SetArguments(args);
            op.SetTools(tools);
            op.SetFuzzyValue(0.05);
            op.Build();
            if (op.IsDone() && !op.Shape().IsNull()) {
                result = op.Shape();
                opOk = true;
            }
            if (!opOk) {
                BRepAlgoAPI_Fuse op2(m_baseShape, tool);
                if (op2.IsDone() && !op2.Shape().IsNull()) {
                    result = op2.Shape();
                    opOk = true;
                }
            }
        }
        
        if (opOk) {
            int nFaces = 0;
            for (TopExp_Explorer ex(result, TopAbs_FACE); ex.More(); ex.Next())
                nFaces++;
            
            if (nFaces >= baseFaces) {
                // Valider la shape avant de l'afficher
                BRepCheck_Analyzer check(result);
                if (!check.IsValid()) {
                    std::cout << "[Thread] Result invalid, trying ShapeFix..." << std::endl;
                    try {
                        ShapeFix_Shape fixer(result);
                        fixer.Perform();
                        result = fixer.Shape();
                        BRepCheck_Analyzer check2(result);
                        if (!check2.IsValid()) {
                            std::cout << "[Thread] Still invalid after fix — cosmetic" << std::endl;
                            m_resultShape = m_baseShape;
                            m_shape = m_baseShape;
                            m_upToDate = true;
                            return true;
                        }
                        std::cout << "[Thread] ShapeFix OK" << std::endl;
                    } catch (...) {
                        std::cout << "[Thread] ShapeFix crashed — cosmetic" << std::endl;
                        m_resultShape = m_baseShape;
                        m_shape = m_baseShape;
                        m_upToDate = true;
                        return true;
                    }
                }
                m_resultShape = result;
                m_shape = m_resultShape;
                std::cout << "[Thread] *** SUCCESS *** (" << nFaces << " faces)" << std::endl;
            } else {
                std::cout << "[Thread] Lost faces — cosmetic" << std::endl;
                m_resultShape = m_baseShape;
                m_shape = m_baseShape;
            }
        } else {
            std::cout << "[Thread] Boolean FAILED — cosmetic" << std::endl;
            m_resultShape = m_baseShape;
            m_shape = m_baseShape;
        }
        
        m_upToDate = true;
        return true;
        
    } catch (Standard_Failure& e) {
        std::cerr << "[Thread] " << e.GetMessageString() << std::endl;
    } catch (...) {
        std::cerr << "[Thread] Unknown exception" << std::endl;
    }
    
    m_resultShape = m_baseShape;
    m_shape = m_baseShape;
    m_upToDate = true;
    return true;
}

} // namespace CADEngine

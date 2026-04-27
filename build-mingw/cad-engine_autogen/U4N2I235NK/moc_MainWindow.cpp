/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/mainwindow/MainWindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "onNew",
        "",
        "onOpen",
        "onSave",
        "onExportSTEP",
        "onExportSTL",
        "onCreateSketch",
        "onFinishSketch",
        "onExtrude",
        "onRevolve",
        "onFillet3D",
        "onChamfer3D",
        "onShell3D",
        "onPushPull",
        "onSweep",
        "onLoft",
        "onLinearPattern",
        "onCircularPattern",
        "onThread",
        "showThreadDialog",
        "TopoDS_Face",
        "cylindricalFace",
        "onSketchOnFace",
        "onToolSelect",
        "onToolLine",
        "onToolCircle",
        "onToolRectangle",
        "onToolArc",
        "onToolPolyline",
        "onToolEllipse",
        "onToolPolygon",
        "onToolArcCenter",
        "onToolOblong",
        "onToolDimension",
        "onViewFront",
        "onViewTop",
        "onViewRight",
        "onViewIso",
        "onModeChanged",
        "ViewMode",
        "mode",
        "onSketchEntityAdded",
        "onEntityCreated",
        "std::shared_ptr<CADEngine::SketchEntity>",
        "entity",
        "std::shared_ptr<CADEngine::Sketch2D>",
        "sketch",
        "onDimensionCreated",
        "std::shared_ptr<CADEngine::Dimension>",
        "dimension",
        "onStatusMessage",
        "message",
        "onDimensionClicked",
        "onToolChanged",
        "SketchTool",
        "tool",
        "onFeatureSelected",
        "std::shared_ptr<CADEngine::Feature>",
        "feature",
        "onFeatureDoubleClicked",
        "onFeatureDeleted",
        "onExportSketchDXF",
        "std::shared_ptr<CADEngine::SketchFeature>",
        "onExportSketchRealSizePDF",
        "onExportSketchPlan",
        "onDeleteEntity",
        "onDeleteDimension",
        "onDeletePolylineSegment",
        "segmentIndex",
        "refreshAllAngularDimensions",
        "onFilletVertex",
        "std::shared_ptr<CADEngine::SketchPolyline>",
        "polyline",
        "vertexIndex",
        "onFilletRectCorner",
        "std::shared_ptr<CADEngine::SketchRectangle>",
        "rect",
        "cornerIdx",
        "onFilletLineCorner",
        "std::shared_ptr<CADEngine::SketchLine>",
        "line1",
        "line1AtStart",
        "line2",
        "line2AtStart",
        "onConstraintHorizontal",
        "onConstraintVertical",
        "onConstraintParallel",
        "onConstraintPerpendicular",
        "onConstraintCoincident",
        "onConstraintConcentric",
        "onConstraintSymmetric",
        "onConstraintFix",
        "onFillet",
        "onUndo",
        "onRedo",
        "onThemeCycle",
        "refreshIcons",
        "showHelpSketch",
        "showHelpViewport"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onNew'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onOpen'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSave'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onExportSTEP'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onExportSTL'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCreateSketch'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFinishSketch'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onExtrude'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRevolve'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFillet3D'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onChamfer3D'
        QtMocHelpers::SlotData<void()>(12, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onShell3D'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPushPull'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSweep'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onLoft'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onLinearPattern'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onCircularPattern'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onThread'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'showThreadDialog'
        QtMocHelpers::SlotData<void(const TopoDS_Face &)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 21, 22 },
        }}),
        // Slot 'onSketchOnFace'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToolSelect'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToolLine'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToolCircle'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToolRectangle'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToolArc'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToolPolyline'
        QtMocHelpers::SlotData<void()>(29, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToolEllipse'
        QtMocHelpers::SlotData<void()>(30, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToolPolygon'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToolArcCenter'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToolOblong'
        QtMocHelpers::SlotData<void()>(33, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onToolDimension'
        QtMocHelpers::SlotData<void()>(34, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onViewFront'
        QtMocHelpers::SlotData<void()>(35, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onViewTop'
        QtMocHelpers::SlotData<void()>(36, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onViewRight'
        QtMocHelpers::SlotData<void()>(37, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onViewIso'
        QtMocHelpers::SlotData<void()>(38, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onModeChanged'
        QtMocHelpers::SlotData<void(ViewMode)>(39, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 40, 41 },
        }}),
        // Slot 'onSketchEntityAdded'
        QtMocHelpers::SlotData<void()>(42, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onEntityCreated'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::SketchEntity>, std::shared_ptr<CADEngine::Sketch2D>)>(43, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 44, 45 }, { 0x80000000 | 46, 47 },
        }}),
        // Slot 'onDimensionCreated'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::Dimension>, std::shared_ptr<CADEngine::Sketch2D>)>(48, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 49, 50 }, { 0x80000000 | 46, 47 },
        }}),
        // Slot 'onStatusMessage'
        QtMocHelpers::SlotData<void(const QString &)>(51, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 52 },
        }}),
        // Slot 'onDimensionClicked'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::Dimension>)>(53, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 49, 50 },
        }}),
        // Slot 'onToolChanged'
        QtMocHelpers::SlotData<void(SketchTool)>(54, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 55, 56 },
        }}),
        // Slot 'onFeatureSelected'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::Feature>)>(57, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 58, 59 },
        }}),
        // Slot 'onFeatureDoubleClicked'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::Feature>)>(60, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 58, 59 },
        }}),
        // Slot 'onFeatureDeleted'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::Feature>)>(61, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 58, 59 },
        }}),
        // Slot 'onExportSketchDXF'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::SketchFeature>)>(62, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 63, 47 },
        }}),
        // Slot 'onExportSketchRealSizePDF'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::SketchFeature>)>(64, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 63, 47 },
        }}),
        // Slot 'onExportSketchPlan'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::SketchFeature>)>(65, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 63, 47 },
        }}),
        // Slot 'onDeleteEntity'
        QtMocHelpers::SlotData<void()>(66, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDeleteDimension'
        QtMocHelpers::SlotData<void()>(67, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onDeletePolylineSegment'
        QtMocHelpers::SlotData<void(int)>(68, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 69 },
        }}),
        // Slot 'refreshAllAngularDimensions'
        QtMocHelpers::SlotData<void()>(70, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFilletVertex'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::SketchPolyline>, int)>(71, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 72, 73 }, { QMetaType::Int, 74 },
        }}),
        // Slot 'onFilletRectCorner'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::SketchRectangle>, int)>(75, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 76, 77 }, { QMetaType::Int, 78 },
        }}),
        // Slot 'onFilletLineCorner'
        QtMocHelpers::SlotData<void(std::shared_ptr<CADEngine::SketchLine>, bool, std::shared_ptr<CADEngine::SketchLine>, bool)>(79, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 80, 81 }, { QMetaType::Bool, 82 }, { 0x80000000 | 80, 83 }, { QMetaType::Bool, 84 },
        }}),
        // Slot 'onConstraintHorizontal'
        QtMocHelpers::SlotData<void()>(85, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onConstraintVertical'
        QtMocHelpers::SlotData<void()>(86, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onConstraintParallel'
        QtMocHelpers::SlotData<void()>(87, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onConstraintPerpendicular'
        QtMocHelpers::SlotData<void()>(88, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onConstraintCoincident'
        QtMocHelpers::SlotData<void()>(89, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onConstraintConcentric'
        QtMocHelpers::SlotData<void()>(90, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onConstraintSymmetric'
        QtMocHelpers::SlotData<void()>(91, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onConstraintFix'
        QtMocHelpers::SlotData<void()>(92, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onFillet'
        QtMocHelpers::SlotData<void()>(93, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onUndo'
        QtMocHelpers::SlotData<void()>(94, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRedo'
        QtMocHelpers::SlotData<void()>(95, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onThemeCycle'
        QtMocHelpers::SlotData<void()>(96, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'refreshIcons'
        QtMocHelpers::SlotData<void()>(97, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'showHelpSketch'
        QtMocHelpers::SlotData<void()>(98, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'showHelpViewport'
        QtMocHelpers::SlotData<void()>(99, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->onNew(); break;
        case 1: _t->onOpen(); break;
        case 2: _t->onSave(); break;
        case 3: _t->onExportSTEP(); break;
        case 4: _t->onExportSTL(); break;
        case 5: _t->onCreateSketch(); break;
        case 6: _t->onFinishSketch(); break;
        case 7: _t->onExtrude(); break;
        case 8: _t->onRevolve(); break;
        case 9: _t->onFillet3D(); break;
        case 10: _t->onChamfer3D(); break;
        case 11: _t->onShell3D(); break;
        case 12: _t->onPushPull(); break;
        case 13: _t->onSweep(); break;
        case 14: _t->onLoft(); break;
        case 15: _t->onLinearPattern(); break;
        case 16: _t->onCircularPattern(); break;
        case 17: _t->onThread(); break;
        case 18: _t->showThreadDialog((*reinterpret_cast<std::add_pointer_t<TopoDS_Face>>(_a[1]))); break;
        case 19: _t->onSketchOnFace(); break;
        case 20: _t->onToolSelect(); break;
        case 21: _t->onToolLine(); break;
        case 22: _t->onToolCircle(); break;
        case 23: _t->onToolRectangle(); break;
        case 24: _t->onToolArc(); break;
        case 25: _t->onToolPolyline(); break;
        case 26: _t->onToolEllipse(); break;
        case 27: _t->onToolPolygon(); break;
        case 28: _t->onToolArcCenter(); break;
        case 29: _t->onToolOblong(); break;
        case 30: _t->onToolDimension(); break;
        case 31: _t->onViewFront(); break;
        case 32: _t->onViewTop(); break;
        case 33: _t->onViewRight(); break;
        case 34: _t->onViewIso(); break;
        case 35: _t->onModeChanged((*reinterpret_cast<std::add_pointer_t<ViewMode>>(_a[1]))); break;
        case 36: _t->onSketchEntityAdded(); break;
        case 37: _t->onEntityCreated((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchEntity>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Sketch2D>>>(_a[2]))); break;
        case 38: _t->onDimensionCreated((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Dimension>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Sketch2D>>>(_a[2]))); break;
        case 39: _t->onStatusMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 40: _t->onDimensionClicked((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Dimension>>>(_a[1]))); break;
        case 41: _t->onToolChanged((*reinterpret_cast<std::add_pointer_t<SketchTool>>(_a[1]))); break;
        case 42: _t->onFeatureSelected((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Feature>>>(_a[1]))); break;
        case 43: _t->onFeatureDoubleClicked((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Feature>>>(_a[1]))); break;
        case 44: _t->onFeatureDeleted((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Feature>>>(_a[1]))); break;
        case 45: _t->onExportSketchDXF((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchFeature>>>(_a[1]))); break;
        case 46: _t->onExportSketchRealSizePDF((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchFeature>>>(_a[1]))); break;
        case 47: _t->onExportSketchPlan((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchFeature>>>(_a[1]))); break;
        case 48: _t->onDeleteEntity(); break;
        case 49: _t->onDeleteDimension(); break;
        case 50: _t->onDeletePolylineSegment((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 51: _t->refreshAllAngularDimensions(); break;
        case 52: _t->onFilletVertex((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchPolyline>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 53: _t->onFilletRectCorner((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchRectangle>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 54: _t->onFilletLineCorner((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchLine>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchLine>>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[4]))); break;
        case 55: _t->onConstraintHorizontal(); break;
        case 56: _t->onConstraintVertical(); break;
        case 57: _t->onConstraintParallel(); break;
        case 58: _t->onConstraintPerpendicular(); break;
        case 59: _t->onConstraintCoincident(); break;
        case 60: _t->onConstraintConcentric(); break;
        case 61: _t->onConstraintSymmetric(); break;
        case 62: _t->onConstraintFix(); break;
        case 63: _t->onFillet(); break;
        case 64: _t->onUndo(); break;
        case 65: _t->onRedo(); break;
        case 66: _t->onThemeCycle(); break;
        case 67: _t->refreshIcons(); break;
        case 68: _t->showHelpSketch(); break;
        case 69: _t->showHelpViewport(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 70)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 70;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 70)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 70;
    }
    return _id;
}
QT_WARNING_POP

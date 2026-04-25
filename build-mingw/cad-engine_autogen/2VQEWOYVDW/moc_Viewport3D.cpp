/****************************************************************************
** Meta object code from reading C++ file 'Viewport3D.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/viewport/Viewport3D.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Viewport3D.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10Viewport3DE_t {};
} // unnamed namespace

template <> constexpr inline auto Viewport3D::qt_create_metaobjectdata<qt_meta_tag_ZN10Viewport3DE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Viewport3D",
        "modeChanged",
        "",
        "ViewMode",
        "mode",
        "sketchEntityAdded",
        "entityCreated",
        "std::shared_ptr<CADEngine::SketchEntity>",
        "entity",
        "std::shared_ptr<CADEngine::Sketch2D>",
        "sketch",
        "dimensionCreated",
        "std::shared_ptr<CADEngine::Dimension>",
        "dimension",
        "statusMessage",
        "message",
        "dimensionClicked",
        "toolChanged",
        "SketchTool",
        "tool",
        "entityRightClicked",
        "QPoint",
        "globalPos",
        "dimensionRightClicked",
        "filletRequested",
        "std::shared_ptr<CADEngine::SketchPolyline>",
        "polyline",
        "vertexIndex",
        "faceClicked",
        "TopoDS_Face",
        "face",
        "faceSelected",
        "faceSelectionCancelled",
        "edgeSelected",
        "TopoDS_Edge",
        "edge",
        "edgeSelectionConfirmed",
        "edgeSelectionCancelled",
        "profileSelected",
        "description",
        "multiProfileSelected",
        "std::vector<TopoDS_Face>",
        "faces",
        "profileSelectionCancelled"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'modeChanged'
        QtMocHelpers::SignalData<void(ViewMode)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'sketchEntityAdded'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'entityCreated'
        QtMocHelpers::SignalData<void(std::shared_ptr<CADEngine::SketchEntity>, std::shared_ptr<CADEngine::Sketch2D>)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 }, { 0x80000000 | 9, 10 },
        }}),
        // Signal 'dimensionCreated'
        QtMocHelpers::SignalData<void(std::shared_ptr<CADEngine::Dimension>, std::shared_ptr<CADEngine::Sketch2D>)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 }, { 0x80000000 | 9, 10 },
        }}),
        // Signal 'statusMessage'
        QtMocHelpers::SignalData<void(const QString &)>(14, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 15 },
        }}),
        // Signal 'dimensionClicked'
        QtMocHelpers::SignalData<void(std::shared_ptr<CADEngine::Dimension>)>(16, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 },
        }}),
        // Signal 'toolChanged'
        QtMocHelpers::SignalData<void(SketchTool)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 18, 19 },
        }}),
        // Signal 'entityRightClicked'
        QtMocHelpers::SignalData<void(std::shared_ptr<CADEngine::SketchEntity>, QPoint)>(20, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 7, 8 }, { 0x80000000 | 21, 22 },
        }}),
        // Signal 'dimensionRightClicked'
        QtMocHelpers::SignalData<void(std::shared_ptr<CADEngine::Dimension>, QPoint)>(23, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 12, 13 }, { 0x80000000 | 21, 22 },
        }}),
        // Signal 'filletRequested'
        QtMocHelpers::SignalData<void(std::shared_ptr<CADEngine::SketchPolyline>, int)>(24, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 25, 26 }, { QMetaType::Int, 27 },
        }}),
        // Signal 'faceClicked'
        QtMocHelpers::SignalData<void(TopoDS_Face)>(28, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 29, 30 },
        }}),
        // Signal 'faceSelected'
        QtMocHelpers::SignalData<void(TopoDS_Face)>(31, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 29, 30 },
        }}),
        // Signal 'faceSelectionCancelled'
        QtMocHelpers::SignalData<void()>(32, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'edgeSelected'
        QtMocHelpers::SignalData<void(TopoDS_Edge)>(33, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 34, 35 },
        }}),
        // Signal 'edgeSelectionConfirmed'
        QtMocHelpers::SignalData<void()>(36, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'edgeSelectionCancelled'
        QtMocHelpers::SignalData<void()>(37, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'profileSelected'
        QtMocHelpers::SignalData<void(TopoDS_Face, QString)>(38, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 29, 30 }, { QMetaType::QString, 39 },
        }}),
        // Signal 'multiProfileSelected'
        QtMocHelpers::SignalData<void(std::vector<TopoDS_Face>, QString)>(40, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 41, 42 }, { QMetaType::QString, 39 },
        }}),
        // Signal 'profileSelectionCancelled'
        QtMocHelpers::SignalData<void()>(43, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Viewport3D, qt_meta_tag_ZN10Viewport3DE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Viewport3D::staticMetaObject = { {
    QMetaObject::SuperData::link<QOpenGLWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10Viewport3DE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10Viewport3DE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10Viewport3DE_t>.metaTypes,
    nullptr
} };

void Viewport3D::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Viewport3D *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->modeChanged((*reinterpret_cast<std::add_pointer_t<ViewMode>>(_a[1]))); break;
        case 1: _t->sketchEntityAdded(); break;
        case 2: _t->entityCreated((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchEntity>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Sketch2D>>>(_a[2]))); break;
        case 3: _t->dimensionCreated((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Dimension>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Sketch2D>>>(_a[2]))); break;
        case 4: _t->statusMessage((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->dimensionClicked((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Dimension>>>(_a[1]))); break;
        case 6: _t->toolChanged((*reinterpret_cast<std::add_pointer_t<SketchTool>>(_a[1]))); break;
        case 7: _t->entityRightClicked((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchEntity>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 8: _t->dimensionRightClicked((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Dimension>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 9: _t->filletRequested((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchPolyline>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 10: _t->faceClicked((*reinterpret_cast<std::add_pointer_t<TopoDS_Face>>(_a[1]))); break;
        case 11: _t->faceSelected((*reinterpret_cast<std::add_pointer_t<TopoDS_Face>>(_a[1]))); break;
        case 12: _t->faceSelectionCancelled(); break;
        case 13: _t->edgeSelected((*reinterpret_cast<std::add_pointer_t<TopoDS_Edge>>(_a[1]))); break;
        case 14: _t->edgeSelectionConfirmed(); break;
        case 15: _t->edgeSelectionCancelled(); break;
        case 16: _t->profileSelected((*reinterpret_cast<std::add_pointer_t<TopoDS_Face>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 17: _t->multiProfileSelected((*reinterpret_cast<std::add_pointer_t<std::vector<TopoDS_Face>>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 18: _t->profileSelectionCancelled(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(ViewMode )>(_a, &Viewport3D::modeChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)()>(_a, &Viewport3D::sketchEntityAdded, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(std::shared_ptr<CADEngine::SketchEntity> , std::shared_ptr<CADEngine::Sketch2D> )>(_a, &Viewport3D::entityCreated, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(std::shared_ptr<CADEngine::Dimension> , std::shared_ptr<CADEngine::Sketch2D> )>(_a, &Viewport3D::dimensionCreated, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(const QString & )>(_a, &Viewport3D::statusMessage, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(std::shared_ptr<CADEngine::Dimension> )>(_a, &Viewport3D::dimensionClicked, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(SketchTool )>(_a, &Viewport3D::toolChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(std::shared_ptr<CADEngine::SketchEntity> , QPoint )>(_a, &Viewport3D::entityRightClicked, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(std::shared_ptr<CADEngine::Dimension> , QPoint )>(_a, &Viewport3D::dimensionRightClicked, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(std::shared_ptr<CADEngine::SketchPolyline> , int )>(_a, &Viewport3D::filletRequested, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(TopoDS_Face )>(_a, &Viewport3D::faceClicked, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(TopoDS_Face )>(_a, &Viewport3D::faceSelected, 11))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)()>(_a, &Viewport3D::faceSelectionCancelled, 12))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(TopoDS_Edge )>(_a, &Viewport3D::edgeSelected, 13))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)()>(_a, &Viewport3D::edgeSelectionConfirmed, 14))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)()>(_a, &Viewport3D::edgeSelectionCancelled, 15))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(TopoDS_Face , QString )>(_a, &Viewport3D::profileSelected, 16))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)(std::vector<TopoDS_Face> , QString )>(_a, &Viewport3D::multiProfileSelected, 17))
            return;
        if (QtMocHelpers::indexOfMethod<void (Viewport3D::*)()>(_a, &Viewport3D::profileSelectionCancelled, 18))
            return;
    }
}

const QMetaObject *Viewport3D::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Viewport3D::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10Viewport3DE_t>.strings))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QOpenGLFunctions"))
        return static_cast< QOpenGLFunctions*>(this);
    return QOpenGLWidget::qt_metacast(_clname);
}

int Viewport3D::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QOpenGLWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void Viewport3D::modeChanged(ViewMode _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void Viewport3D::sketchEntityAdded()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Viewport3D::entityCreated(std::shared_ptr<CADEngine::SketchEntity> _t1, std::shared_ptr<CADEngine::Sketch2D> _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}

// SIGNAL 3
void Viewport3D::dimensionCreated(std::shared_ptr<CADEngine::Dimension> _t1, std::shared_ptr<CADEngine::Sketch2D> _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void Viewport3D::statusMessage(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}

// SIGNAL 5
void Viewport3D::dimensionClicked(std::shared_ptr<CADEngine::Dimension> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 5, nullptr, _t1);
}

// SIGNAL 6
void Viewport3D::toolChanged(SketchTool _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 6, nullptr, _t1);
}

// SIGNAL 7
void Viewport3D::entityRightClicked(std::shared_ptr<CADEngine::SketchEntity> _t1, QPoint _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 7, nullptr, _t1, _t2);
}

// SIGNAL 8
void Viewport3D::dimensionRightClicked(std::shared_ptr<CADEngine::Dimension> _t1, QPoint _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1, _t2);
}

// SIGNAL 9
void Viewport3D::filletRequested(std::shared_ptr<CADEngine::SketchPolyline> _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 9, nullptr, _t1, _t2);
}

// SIGNAL 10
void Viewport3D::faceClicked(TopoDS_Face _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 10, nullptr, _t1);
}

// SIGNAL 11
void Viewport3D::faceSelected(TopoDS_Face _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 11, nullptr, _t1);
}

// SIGNAL 12
void Viewport3D::faceSelectionCancelled()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void Viewport3D::edgeSelected(TopoDS_Edge _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 13, nullptr, _t1);
}

// SIGNAL 14
void Viewport3D::edgeSelectionConfirmed()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}

// SIGNAL 15
void Viewport3D::edgeSelectionCancelled()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void Viewport3D::profileSelected(TopoDS_Face _t1, QString _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 16, nullptr, _t1, _t2);
}

// SIGNAL 17
void Viewport3D::multiProfileSelected(std::vector<TopoDS_Face> _t1, QString _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 17, nullptr, _t1, _t2);
}

// SIGNAL 18
void Viewport3D::profileSelectionCancelled()
{
    QMetaObject::activate(this, &staticMetaObject, 18, nullptr);
}
QT_WARNING_POP

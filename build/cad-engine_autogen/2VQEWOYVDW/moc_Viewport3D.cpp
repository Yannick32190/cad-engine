/****************************************************************************
** Meta object code from reading C++ file 'Viewport3D.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/viewport/Viewport3D.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Viewport3D.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_Viewport3D_t {
    uint offsetsAndSizes[86];
    char stringdata0[11];
    char stringdata1[12];
    char stringdata2[1];
    char stringdata3[9];
    char stringdata4[5];
    char stringdata5[18];
    char stringdata6[14];
    char stringdata7[41];
    char stringdata8[7];
    char stringdata9[37];
    char stringdata10[7];
    char stringdata11[17];
    char stringdata12[38];
    char stringdata13[10];
    char stringdata14[14];
    char stringdata15[8];
    char stringdata16[17];
    char stringdata17[12];
    char stringdata18[11];
    char stringdata19[5];
    char stringdata20[19];
    char stringdata21[10];
    char stringdata22[22];
    char stringdata23[16];
    char stringdata24[43];
    char stringdata25[9];
    char stringdata26[12];
    char stringdata27[12];
    char stringdata28[12];
    char stringdata29[5];
    char stringdata30[13];
    char stringdata31[23];
    char stringdata32[13];
    char stringdata33[12];
    char stringdata34[5];
    char stringdata35[23];
    char stringdata36[23];
    char stringdata37[16];
    char stringdata38[12];
    char stringdata39[21];
    char stringdata40[25];
    char stringdata41[6];
    char stringdata42[26];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_Viewport3D_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_Viewport3D_t qt_meta_stringdata_Viewport3D = {
    {
        QT_MOC_LITERAL(0, 10),  // "Viewport3D"
        QT_MOC_LITERAL(11, 11),  // "modeChanged"
        QT_MOC_LITERAL(23, 0),  // ""
        QT_MOC_LITERAL(24, 8),  // "ViewMode"
        QT_MOC_LITERAL(33, 4),  // "mode"
        QT_MOC_LITERAL(38, 17),  // "sketchEntityAdded"
        QT_MOC_LITERAL(56, 13),  // "entityCreated"
        QT_MOC_LITERAL(70, 40),  // "std::shared_ptr<CADEngine::Sk..."
        QT_MOC_LITERAL(111, 6),  // "entity"
        QT_MOC_LITERAL(118, 36),  // "std::shared_ptr<CADEngine::Sk..."
        QT_MOC_LITERAL(155, 6),  // "sketch"
        QT_MOC_LITERAL(162, 16),  // "dimensionCreated"
        QT_MOC_LITERAL(179, 37),  // "std::shared_ptr<CADEngine::Di..."
        QT_MOC_LITERAL(217, 9),  // "dimension"
        QT_MOC_LITERAL(227, 13),  // "statusMessage"
        QT_MOC_LITERAL(241, 7),  // "message"
        QT_MOC_LITERAL(249, 16),  // "dimensionClicked"
        QT_MOC_LITERAL(266, 11),  // "toolChanged"
        QT_MOC_LITERAL(278, 10),  // "SketchTool"
        QT_MOC_LITERAL(289, 4),  // "tool"
        QT_MOC_LITERAL(294, 18),  // "entityRightClicked"
        QT_MOC_LITERAL(313, 9),  // "globalPos"
        QT_MOC_LITERAL(323, 21),  // "dimensionRightClicked"
        QT_MOC_LITERAL(345, 15),  // "filletRequested"
        QT_MOC_LITERAL(361, 42),  // "std::shared_ptr<CADEngine::Sk..."
        QT_MOC_LITERAL(404, 8),  // "polyline"
        QT_MOC_LITERAL(413, 11),  // "vertexIndex"
        QT_MOC_LITERAL(425, 11),  // "faceClicked"
        QT_MOC_LITERAL(437, 11),  // "TopoDS_Face"
        QT_MOC_LITERAL(449, 4),  // "face"
        QT_MOC_LITERAL(454, 12),  // "faceSelected"
        QT_MOC_LITERAL(467, 22),  // "faceSelectionCancelled"
        QT_MOC_LITERAL(490, 12),  // "edgeSelected"
        QT_MOC_LITERAL(503, 11),  // "TopoDS_Edge"
        QT_MOC_LITERAL(515, 4),  // "edge"
        QT_MOC_LITERAL(520, 22),  // "edgeSelectionConfirmed"
        QT_MOC_LITERAL(543, 22),  // "edgeSelectionCancelled"
        QT_MOC_LITERAL(566, 15),  // "profileSelected"
        QT_MOC_LITERAL(582, 11),  // "description"
        QT_MOC_LITERAL(594, 20),  // "multiProfileSelected"
        QT_MOC_LITERAL(615, 24),  // "std::vector<TopoDS_Face>"
        QT_MOC_LITERAL(640, 5),  // "faces"
        QT_MOC_LITERAL(646, 25)   // "profileSelectionCancelled"
    },
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
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_Viewport3D[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      19,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  128,    2, 0x06,    1 /* Public */,
       5,    0,  131,    2, 0x06,    3 /* Public */,
       6,    2,  132,    2, 0x06,    4 /* Public */,
      11,    2,  137,    2, 0x06,    7 /* Public */,
      14,    1,  142,    2, 0x06,   10 /* Public */,
      16,    1,  145,    2, 0x06,   12 /* Public */,
      17,    1,  148,    2, 0x06,   14 /* Public */,
      20,    2,  151,    2, 0x06,   16 /* Public */,
      22,    2,  156,    2, 0x06,   19 /* Public */,
      23,    2,  161,    2, 0x06,   22 /* Public */,
      27,    1,  166,    2, 0x06,   25 /* Public */,
      30,    1,  169,    2, 0x06,   27 /* Public */,
      31,    0,  172,    2, 0x06,   29 /* Public */,
      32,    1,  173,    2, 0x06,   30 /* Public */,
      35,    0,  176,    2, 0x06,   32 /* Public */,
      36,    0,  177,    2, 0x06,   33 /* Public */,
      37,    2,  178,    2, 0x06,   34 /* Public */,
      39,    2,  183,    2, 0x06,   37 /* Public */,
      42,    0,  188,    2, 0x06,   40 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7, 0x80000000 | 9,    8,   10,
    QMetaType::Void, 0x80000000 | 12, 0x80000000 | 9,   13,   10,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void, 0x80000000 | 12,   13,
    QMetaType::Void, 0x80000000 | 18,   19,
    QMetaType::Void, 0x80000000 | 7, QMetaType::QPoint,    8,   21,
    QMetaType::Void, 0x80000000 | 12, QMetaType::QPoint,   13,   21,
    QMetaType::Void, 0x80000000 | 24, QMetaType::Int,   25,   26,
    QMetaType::Void, 0x80000000 | 28,   29,
    QMetaType::Void, 0x80000000 | 28,   29,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 33,   34,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 28, QMetaType::QString,   29,   38,
    QMetaType::Void, 0x80000000 | 40, QMetaType::QString,   41,   38,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject Viewport3D::staticMetaObject = { {
    QMetaObject::SuperData::link<QOpenGLWidget::staticMetaObject>(),
    qt_meta_stringdata_Viewport3D.offsetsAndSizes,
    qt_meta_data_Viewport3D,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_Viewport3D_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Viewport3D, std::true_type>,
        // method 'modeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<ViewMode, std::false_type>,
        // method 'sketchEntityAdded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'entityCreated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::SketchEntity>, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::Sketch2D>, std::false_type>,
        // method 'dimensionCreated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::Dimension>, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::Sketch2D>, std::false_type>,
        // method 'statusMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'dimensionClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::Dimension>, std::false_type>,
        // method 'toolChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<SketchTool, std::false_type>,
        // method 'entityRightClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::SketchEntity>, std::false_type>,
        QtPrivate::TypeAndForceComplete<QPoint, std::false_type>,
        // method 'dimensionRightClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::Dimension>, std::false_type>,
        QtPrivate::TypeAndForceComplete<QPoint, std::false_type>,
        // method 'filletRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::SketchPolyline>, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'faceClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<TopoDS_Face, std::false_type>,
        // method 'faceSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<TopoDS_Face, std::false_type>,
        // method 'faceSelectionCancelled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'edgeSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<TopoDS_Edge, std::false_type>,
        // method 'edgeSelectionConfirmed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'edgeSelectionCancelled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'profileSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<TopoDS_Face, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'multiProfileSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::vector<TopoDS_Face>, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'profileSelectionCancelled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void Viewport3D::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Viewport3D *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->modeChanged((*reinterpret_cast< std::add_pointer_t<ViewMode>>(_a[1]))); break;
        case 1: _t->sketchEntityAdded(); break;
        case 2: _t->entityCreated((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::SketchEntity>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::Sketch2D>>>(_a[2]))); break;
        case 3: _t->dimensionCreated((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::Dimension>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::Sketch2D>>>(_a[2]))); break;
        case 4: _t->statusMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->dimensionClicked((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::Dimension>>>(_a[1]))); break;
        case 6: _t->toolChanged((*reinterpret_cast< std::add_pointer_t<SketchTool>>(_a[1]))); break;
        case 7: _t->entityRightClicked((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::SketchEntity>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 8: _t->dimensionRightClicked((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::Dimension>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 9: _t->filletRequested((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::SketchPolyline>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 10: _t->faceClicked((*reinterpret_cast< std::add_pointer_t<TopoDS_Face>>(_a[1]))); break;
        case 11: _t->faceSelected((*reinterpret_cast< std::add_pointer_t<TopoDS_Face>>(_a[1]))); break;
        case 12: _t->faceSelectionCancelled(); break;
        case 13: _t->edgeSelected((*reinterpret_cast< std::add_pointer_t<TopoDS_Edge>>(_a[1]))); break;
        case 14: _t->edgeSelectionConfirmed(); break;
        case 15: _t->edgeSelectionCancelled(); break;
        case 16: _t->profileSelected((*reinterpret_cast< std::add_pointer_t<TopoDS_Face>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 17: _t->multiProfileSelected((*reinterpret_cast< std::add_pointer_t<std::vector<TopoDS_Face>>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 18: _t->profileSelectionCancelled(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Viewport3D::*)(ViewMode );
            if (_t _q_method = &Viewport3D::modeChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)();
            if (_t _q_method = &Viewport3D::sketchEntityAdded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(std::shared_ptr<CADEngine::SketchEntity> , std::shared_ptr<CADEngine::Sketch2D> );
            if (_t _q_method = &Viewport3D::entityCreated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(std::shared_ptr<CADEngine::Dimension> , std::shared_ptr<CADEngine::Sketch2D> );
            if (_t _q_method = &Viewport3D::dimensionCreated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(const QString & );
            if (_t _q_method = &Viewport3D::statusMessage; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(std::shared_ptr<CADEngine::Dimension> );
            if (_t _q_method = &Viewport3D::dimensionClicked; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(SketchTool );
            if (_t _q_method = &Viewport3D::toolChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(std::shared_ptr<CADEngine::SketchEntity> , QPoint );
            if (_t _q_method = &Viewport3D::entityRightClicked; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(std::shared_ptr<CADEngine::Dimension> , QPoint );
            if (_t _q_method = &Viewport3D::dimensionRightClicked; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(std::shared_ptr<CADEngine::SketchPolyline> , int );
            if (_t _q_method = &Viewport3D::filletRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(TopoDS_Face );
            if (_t _q_method = &Viewport3D::faceClicked; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(TopoDS_Face );
            if (_t _q_method = &Viewport3D::faceSelected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)();
            if (_t _q_method = &Viewport3D::faceSelectionCancelled; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(TopoDS_Edge );
            if (_t _q_method = &Viewport3D::edgeSelected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)();
            if (_t _q_method = &Viewport3D::edgeSelectionConfirmed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)();
            if (_t _q_method = &Viewport3D::edgeSelectionCancelled; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(TopoDS_Face , QString );
            if (_t _q_method = &Viewport3D::profileSelected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)(std::vector<TopoDS_Face> , QString );
            if (_t _q_method = &Viewport3D::multiProfileSelected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 17;
                return;
            }
        }
        {
            using _t = void (Viewport3D::*)();
            if (_t _q_method = &Viewport3D::profileSelectionCancelled; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 18;
                return;
            }
        }
    }
}

const QMetaObject *Viewport3D::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Viewport3D::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Viewport3D.stringdata0))
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
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void Viewport3D::modeChanged(ViewMode _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Viewport3D::sketchEntityAdded()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Viewport3D::entityCreated(std::shared_ptr<CADEngine::SketchEntity> _t1, std::shared_ptr<CADEngine::Sketch2D> _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void Viewport3D::dimensionCreated(std::shared_ptr<CADEngine::Dimension> _t1, std::shared_ptr<CADEngine::Sketch2D> _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Viewport3D::statusMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void Viewport3D::dimensionClicked(std::shared_ptr<CADEngine::Dimension> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void Viewport3D::toolChanged(SketchTool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void Viewport3D::entityRightClicked(std::shared_ptr<CADEngine::SketchEntity> _t1, QPoint _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void Viewport3D::dimensionRightClicked(std::shared_ptr<CADEngine::Dimension> _t1, QPoint _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void Viewport3D::filletRequested(std::shared_ptr<CADEngine::SketchPolyline> _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void Viewport3D::faceClicked(TopoDS_Face _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void Viewport3D::faceSelected(TopoDS_Face _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void Viewport3D::faceSelectionCancelled()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void Viewport3D::edgeSelected(TopoDS_Edge _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
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
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void Viewport3D::multiProfileSelected(std::vector<TopoDS_Face> _t1, QString _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void Viewport3D::profileSelectionCancelled()
{
    QMetaObject::activate(this, &staticMetaObject, 18, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

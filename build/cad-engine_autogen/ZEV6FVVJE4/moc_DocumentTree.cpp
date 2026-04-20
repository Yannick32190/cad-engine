/****************************************************************************
** Meta object code from reading C++ file 'DocumentTree.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/ui/tree/DocumentTree.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DocumentTree.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_DocumentTree_t {
    uint offsetsAndSizes[36];
    char stringdata0[13];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[36];
    char stringdata4[8];
    char stringdata5[21];
    char stringdata6[23];
    char stringdata7[25];
    char stringdata8[42];
    char stringdata9[7];
    char stringdata10[25];
    char stringdata11[14];
    char stringdata12[17];
    char stringdata13[5];
    char stringdata14[7];
    char stringdata15[20];
    char stringdata16[14];
    char stringdata17[4];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_DocumentTree_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_DocumentTree_t qt_meta_stringdata_DocumentTree = {
    {
        QT_MOC_LITERAL(0, 12),  // "DocumentTree"
        QT_MOC_LITERAL(13, 15),  // "featureSelected"
        QT_MOC_LITERAL(29, 0),  // ""
        QT_MOC_LITERAL(30, 35),  // "std::shared_ptr<CADEngine::Fe..."
        QT_MOC_LITERAL(66, 7),  // "feature"
        QT_MOC_LITERAL(74, 20),  // "featureDoubleClicked"
        QT_MOC_LITERAL(95, 22),  // "deleteFeatureRequested"
        QT_MOC_LITERAL(118, 24),  // "exportSketchDXFRequested"
        QT_MOC_LITERAL(143, 41),  // "std::shared_ptr<CADEngine::Sk..."
        QT_MOC_LITERAL(185, 6),  // "sketch"
        QT_MOC_LITERAL(192, 24),  // "exportSketchPDFRequested"
        QT_MOC_LITERAL(217, 13),  // "onItemClicked"
        QT_MOC_LITERAL(231, 16),  // "QTreeWidgetItem*"
        QT_MOC_LITERAL(248, 4),  // "item"
        QT_MOC_LITERAL(253, 6),  // "column"
        QT_MOC_LITERAL(260, 19),  // "onItemDoubleClicked"
        QT_MOC_LITERAL(280, 13),  // "onContextMenu"
        QT_MOC_LITERAL(294, 3)   // "pos"
    },
    "DocumentTree",
    "featureSelected",
    "",
    "std::shared_ptr<CADEngine::Feature>",
    "feature",
    "featureDoubleClicked",
    "deleteFeatureRequested",
    "exportSketchDXFRequested",
    "std::shared_ptr<CADEngine::SketchFeature>",
    "sketch",
    "exportSketchPDFRequested",
    "onItemClicked",
    "QTreeWidgetItem*",
    "item",
    "column",
    "onItemDoubleClicked",
    "onContextMenu",
    "pos"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_DocumentTree[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   62,    2, 0x06,    1 /* Public */,
       5,    1,   65,    2, 0x06,    3 /* Public */,
       6,    1,   68,    2, 0x06,    5 /* Public */,
       7,    1,   71,    2, 0x06,    7 /* Public */,
      10,    1,   74,    2, 0x06,    9 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      11,    2,   77,    2, 0x08,   11 /* Private */,
      15,    2,   82,    2, 0x08,   14 /* Private */,
      16,    1,   87,    2, 0x08,   17 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, 0x80000000 | 8,    9,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 12, QMetaType::Int,   13,   14,
    QMetaType::Void, 0x80000000 | 12, QMetaType::Int,   13,   14,
    QMetaType::Void, QMetaType::QPoint,   17,

       0        // eod
};

Q_CONSTINIT const QMetaObject DocumentTree::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_DocumentTree.offsetsAndSizes,
    qt_meta_data_DocumentTree,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_DocumentTree_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DocumentTree, std::true_type>,
        // method 'featureSelected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::Feature>, std::false_type>,
        // method 'featureDoubleClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::Feature>, std::false_type>,
        // method 'deleteFeatureRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::Feature>, std::false_type>,
        // method 'exportSketchDXFRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::SketchFeature>, std::false_type>,
        // method 'exportSketchPDFRequested'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<std::shared_ptr<CADEngine::SketchFeature>, std::false_type>,
        // method 'onItemClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QTreeWidgetItem *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onItemDoubleClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QTreeWidgetItem *, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onContextMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>
    >,
    nullptr
} };

void DocumentTree::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DocumentTree *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->featureSelected((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::Feature>>>(_a[1]))); break;
        case 1: _t->featureDoubleClicked((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::Feature>>>(_a[1]))); break;
        case 2: _t->deleteFeatureRequested((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::Feature>>>(_a[1]))); break;
        case 3: _t->exportSketchDXFRequested((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::SketchFeature>>>(_a[1]))); break;
        case 4: _t->exportSketchPDFRequested((*reinterpret_cast< std::add_pointer_t<std::shared_ptr<CADEngine::SketchFeature>>>(_a[1]))); break;
        case 5: _t->onItemClicked((*reinterpret_cast< std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 6: _t->onItemDoubleClicked((*reinterpret_cast< std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 7: _t->onContextMenu((*reinterpret_cast< std::add_pointer_t<QPoint>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DocumentTree::*)(std::shared_ptr<CADEngine::Feature> );
            if (_t _q_method = &DocumentTree::featureSelected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DocumentTree::*)(std::shared_ptr<CADEngine::Feature> );
            if (_t _q_method = &DocumentTree::featureDoubleClicked; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (DocumentTree::*)(std::shared_ptr<CADEngine::Feature> );
            if (_t _q_method = &DocumentTree::deleteFeatureRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (DocumentTree::*)(std::shared_ptr<CADEngine::SketchFeature> );
            if (_t _q_method = &DocumentTree::exportSketchDXFRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (DocumentTree::*)(std::shared_ptr<CADEngine::SketchFeature> );
            if (_t _q_method = &DocumentTree::exportSketchPDFRequested; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *DocumentTree::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DocumentTree::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DocumentTree.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int DocumentTree::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void DocumentTree::featureSelected(std::shared_ptr<CADEngine::Feature> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DocumentTree::featureDoubleClicked(std::shared_ptr<CADEngine::Feature> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DocumentTree::deleteFeatureRequested(std::shared_ptr<CADEngine::Feature> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DocumentTree::exportSketchDXFRequested(std::shared_ptr<CADEngine::SketchFeature> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void DocumentTree::exportSketchPDFRequested(std::shared_ptr<CADEngine::SketchFeature> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE

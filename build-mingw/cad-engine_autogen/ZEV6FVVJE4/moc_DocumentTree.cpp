/****************************************************************************
** Meta object code from reading C++ file 'DocumentTree.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ui/tree/DocumentTree.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DocumentTree.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN12DocumentTreeE_t {};
} // unnamed namespace

template <> constexpr inline auto DocumentTree::qt_create_metaobjectdata<qt_meta_tag_ZN12DocumentTreeE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
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
        "QPoint",
        "pos"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'featureSelected'
        QtMocHelpers::SignalData<void(std::shared_ptr<CADEngine::Feature>)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'featureDoubleClicked'
        QtMocHelpers::SignalData<void(std::shared_ptr<CADEngine::Feature>)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'deleteFeatureRequested'
        QtMocHelpers::SignalData<void(std::shared_ptr<CADEngine::Feature>)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'exportSketchDXFRequested'
        QtMocHelpers::SignalData<void(std::shared_ptr<CADEngine::SketchFeature>)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 9 },
        }}),
        // Signal 'exportSketchPDFRequested'
        QtMocHelpers::SignalData<void(std::shared_ptr<CADEngine::SketchFeature>)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 8, 9 },
        }}),
        // Slot 'onItemClicked'
        QtMocHelpers::SlotData<void(QTreeWidgetItem *, int)>(11, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 12, 13 }, { QMetaType::Int, 14 },
        }}),
        // Slot 'onItemDoubleClicked'
        QtMocHelpers::SlotData<void(QTreeWidgetItem *, int)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 12, 13 }, { QMetaType::Int, 14 },
        }}),
        // Slot 'onContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(16, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 17, 18 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DocumentTree, qt_meta_tag_ZN12DocumentTreeE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DocumentTree::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12DocumentTreeE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12DocumentTreeE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12DocumentTreeE_t>.metaTypes,
    nullptr
} };

void DocumentTree::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DocumentTree *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->featureSelected((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Feature>>>(_a[1]))); break;
        case 1: _t->featureDoubleClicked((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Feature>>>(_a[1]))); break;
        case 2: _t->deleteFeatureRequested((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::Feature>>>(_a[1]))); break;
        case 3: _t->exportSketchDXFRequested((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchFeature>>>(_a[1]))); break;
        case 4: _t->exportSketchPDFRequested((*reinterpret_cast<std::add_pointer_t<std::shared_ptr<CADEngine::SketchFeature>>>(_a[1]))); break;
        case 5: _t->onItemClicked((*reinterpret_cast<std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 6: _t->onItemDoubleClicked((*reinterpret_cast<std::add_pointer_t<QTreeWidgetItem*>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 7: _t->onContextMenu((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DocumentTree::*)(std::shared_ptr<CADEngine::Feature> )>(_a, &DocumentTree::featureSelected, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DocumentTree::*)(std::shared_ptr<CADEngine::Feature> )>(_a, &DocumentTree::featureDoubleClicked, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DocumentTree::*)(std::shared_ptr<CADEngine::Feature> )>(_a, &DocumentTree::deleteFeatureRequested, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DocumentTree::*)(std::shared_ptr<CADEngine::SketchFeature> )>(_a, &DocumentTree::exportSketchDXFRequested, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (DocumentTree::*)(std::shared_ptr<CADEngine::SketchFeature> )>(_a, &DocumentTree::exportSketchPDFRequested, 4))
            return;
    }
}

const QMetaObject *DocumentTree::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DocumentTree::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12DocumentTreeE_t>.strings))
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
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void DocumentTree::featureSelected(std::shared_ptr<CADEngine::Feature> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void DocumentTree::featureDoubleClicked(std::shared_ptr<CADEngine::Feature> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void DocumentTree::deleteFeatureRequested(std::shared_ptr<CADEngine::Feature> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void DocumentTree::exportSketchDXFRequested(std::shared_ptr<CADEngine::SketchFeature> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void DocumentTree::exportSketchPDFRequested(std::shared_ptr<CADEngine::SketchFeature> _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}
QT_WARNING_POP

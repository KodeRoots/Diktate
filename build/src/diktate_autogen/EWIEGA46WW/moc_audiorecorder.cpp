/****************************************************************************
** Meta object code from reading C++ file 'audiorecorder.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/audiorecorder.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'audiorecorder.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.1. It"
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
struct qt_meta_tag_ZN13AudioRecorderE_t {};
} // unnamed namespace

template <> constexpr inline auto AudioRecorder::qt_create_metaobjectdata<qt_meta_tag_ZN13AudioRecorderE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AudioRecorder",
        "recordingStarted",
        "",
        "recordingFinished",
        "tempFilePath",
        "errorOccurred",
        "message",
        "startRecording",
        "stopRecording",
        "isRecording"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'recordingStarted'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'recordingFinished'
        QtMocHelpers::SignalData<void(QString)>(3, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 4 },
        }}),
        // Signal 'errorOccurred'
        QtMocHelpers::SignalData<void(QString)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 6 },
        }}),
        // Method 'startRecording'
        QtMocHelpers::MethodData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'stopRecording'
        QtMocHelpers::MethodData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'isRecording'
        QtMocHelpers::MethodData<bool() const>(9, 2, QMC::AccessPublic, QMetaType::Bool),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AudioRecorder, qt_meta_tag_ZN13AudioRecorderE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AudioRecorder::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13AudioRecorderE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13AudioRecorderE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13AudioRecorderE_t>.metaTypes,
    nullptr
} };

void AudioRecorder::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AudioRecorder *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->recordingStarted(); break;
        case 1: _t->recordingFinished((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->errorOccurred((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->startRecording(); break;
        case 4: _t->stopRecording(); break;
        case 5: { bool _r = _t->isRecording();
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AudioRecorder::*)()>(_a, &AudioRecorder::recordingStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AudioRecorder::*)(QString )>(_a, &AudioRecorder::recordingFinished, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AudioRecorder::*)(QString )>(_a, &AudioRecorder::errorOccurred, 2))
            return;
    }
}

const QMetaObject *AudioRecorder::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AudioRecorder::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13AudioRecorderE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AudioRecorder::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void AudioRecorder::recordingStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AudioRecorder::recordingFinished(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void AudioRecorder::errorOccurred(QString _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}
QT_WARNING_POP

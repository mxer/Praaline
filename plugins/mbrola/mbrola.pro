# Praaline Plugin
# Bridge to the MBROLA Speech Synthesis system
# (c) plugin: George Christodoulides 2014-2015

! include( ../../common.pri ) {
    error( Could not find the common.pri file! )
}

CONFIG += plugin dll qt thread warn_on stl rtti exceptions c++11
TEMPLATE = lib
DEFINES += PLUGIN_MBROLA_LIBRARY

QT += gui sql
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

CONFIG(debug, debug|release) {
    TARGET = mbrolad
} else {
    TARGET = mbrola
}

# put plugin in the same directory as Praaline, inside the plugins sub-directory
win32 {
    DESTDIR += ../../../../../../app/build/plugins/
}
else {
    DESTDIR = ../../app/build/plugins/
}

INCLUDEPATH += . .. ../.. ../../pncore ../../app/interfaces
DEPENDPATH += . .. ../.. ../../pncore

# Qtilities include for plugin interfaces
QTILITIES += extension_system
QTILITIES_SOURCE_BASE  = $$PWD/../../dependencies/qtilities
QTILITIES_BUILD_BASE   = $$OUT_PWD/../../dependencies/qtilities
QTILITIES_DEPENDENCIES = $$OUT_PWD/../../dependencies
include(../../dependencies/qtilities/src/Qtilities.pri)


# Build folder organisation
CONFIG( debug, debug|release ) {
    # debug
    COMPONENTSPATH = build/debug
} else {
    # release
    COMPONENTSPATH = build/release
}
# Application components
LIBS +=  \
        -L../../pncore/$${COMPONENTSPATH} -lpncore$${PRAALINE_LIB_POSTFIX} \
        $$LIBS
PRE_TARGETDEPS += \
        ../../pncore/$${COMPONENTSPATH}/libpncore$${PRAALINE_LIB_POSTFIX}.$${LIB_SUFFIX}

HEADERS += \ 
    pluginmbrola_global.h \
    pluginmbrola.h \ 
    annotationpluginpraatscript.h \
    mbrolafilemanager.h \
    pseudolanguage.h \
    MBROLAResynthesiser.h
	
SOURCES += \ 
    pluginmbrola.cpp \ 
    annotationpluginpraatscript.cpp \
    mbrolafilemanager.cpp \
    pseudolanguage.cpp \
    MBROLAResynthesiser.cpp
	

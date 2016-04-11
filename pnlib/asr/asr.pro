# Praaline
# (c) George Christodoulides 2012-2016

! include( ../../common.pri ) {
    error( Could not find the common.pri file! )
}

CONFIG += staticlib qt thread warn_on stl rtti exceptions c++11

INCLUDEPATH += . .. ../.. ../../pncore
DEPENDPATH += . .. ../.. ../../pncore

TARGET = praaline-asr

# Linking dynamically with PocketSphinx
win32 {
    POCKETSPHINX_BASE_PATH = C:/Qt/mingw-4.9.2-x32
}
unix {
    POCKETSPHINX_BASE_PATH = /usr/local
}
INCLUDEPATH += $${POCKETSPHINX_BASE_PATH}/include/pocketsphinx \
               $${POCKETSPHINX_BASE_PATH}/include/sphinxbase \
               $${POCKETSPHINX_BASE_PATH}/include/
win32 {
    LIBS += -L$${POCKETSPHINX_BASE_PATH}/lib -lpocketsphinx -lsphinxbase -liconv
}
unix {
    LIBS += -L$${POCKETSPHINX_BASE_PATH}/lib -lpocketsphinx -lsphinxbase
}

HEADERS += \
    SpeechToken.h \
    SpeechRecognitionRecipes.h \
    AbstractSpeechRecogniser.h \
    AbstractLanguageModelTrainer.h \
    AbstractForcedAligner.h \
    AbstractFeatureExtractor.h \
    AbstractAcousticModelTrainer.h \
    AbstractAcousticModelAdapter.h \
    sphinx/SphinxSegmentation.h \
    sphinx/SphinxRecogniser.h \
    sphinx/SphinxPronunciationDictionary.h \
    sphinx/SphinxFeatureExtractor.h \
    sphinx/SphinxAcousticModelAdapter.h \
    htk/HTKForcedAligner.h \
    htk/HTKAcousticModelTrainer.h \
    htk/HTKAcousticModelAdapter.h \
    phonetiser/RuleBasedPhonetiser.h \
    phonetiser/ExternalPhonetiser.h \
    phonetiser/DictionaryPhonetiser.h \
    phonetiser/AbstractPhonetiser.h \
    syllabifier/AbstractSyllabifier.h \
    sphinx/SphinxConfiguration.h \
    sphinx/SphinxOfflineRecogniser.h \
    sphinx/SphinxAcousticModelTrainer.h

SOURCES += \
    SpeechRecognitionRecipes.cpp \
    sphinx/SphinxSegmentation.cpp \
    sphinx/SphinxRecogniser.cpp \
    sphinx/SphinxPronunciationDictionary.cpp \
    sphinx/SphinxFeatureExtractor.cpp \
    sphinx/SphinxAcousticModelAdapter.cpp \
    htk/HTKForcedAligner.cpp \
    htk/HTKAcousticModelTrainer.cpp \
    htk/HTKAcousticModelAdapter.cpp \
    phonetiser/RuleBasedPhonetiser.cpp \
    phonetiser/ExternalPhonetiser.cpp \
    phonetiser/DictionaryPhonetiser.cpp \
    sphinx/SphinxConfiguration.cpp \
    sphinx/SphinxOfflineRecogniser.cpp \
    sphinx/SphinxAcousticModelTrainer.cpp

SUBDIRS += \
    phonetiser/phonetiser.pro


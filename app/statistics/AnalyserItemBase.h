#ifndef ANALYSERITEMBASE_H
#define ANALYSERITEMBASE_H

#include <QObject>
#include <QPointer>
#include "pncore/statistics/StatisticalMeasureDefinition.h"

namespace Praaline {
namespace Core {
class CorpusCommunication;
}
}

// The Curiously Recursive Template Pattern
template<class T>
class AnalyserItemDefinitionsBase
{
    static QStringList measureIDsForCommunication() {
        return T::measureIDsForCommunication();
    }
    static QStringList measureIDsForSpeaker() {
        return T::measureIDsForSpeaker();
    }
    static QStringList vectorMeasureIDsForCommunication() {
        return T::vectorMeasureIDsForCommunication();
    }
    static QStringList vectorMeasureIDsForSpeaker() {
        return T::vectorMeasureIDsForSpeaker();
    }
    static Praaline::Core::StatisticalMeasureDefinition measureDefinition(const QString &measureID) {
        return T::measureDefinition(measureID);
    }
};

class AnalyserItemBase : public QObject
{
    Q_OBJECT
public:
    explicit AnalyserItemBase(QObject *parent = 0) : QObject(parent) {}
    virtual ~AnalyserItemBase() {}

    virtual QStringList speakerIDs() const = 0;
    virtual double measureCom(const QString &measureID) const = 0;
    virtual double measureSpk(const QString &speakerID, const QString &measureID) const = 0;
    virtual QList<double> vectorMeasureCom(const QString &measureID) const = 0;
    virtual QList<double> vectorMeasureSpk(const QString &speakerID, const QString &measureID) const = 0;

    virtual void analyse(QPointer<Praaline::Core::CorpusCommunication> com) = 0;
};

#endif // ANALYSERITEMBASE_H

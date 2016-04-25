#ifndef ANALYSERTEMPORAL_H
#define ANALYSERTEMPORAL_H

#include <QObject>
#include <QPointer>
#include "pncore/statistics/StatisticalMeasureDefinition.h"

struct AnalyserTemporalData;
class Corpus;
class CorpusCommunication;
class IntervalTier;

class AnalyserTemporal : public QObject
{
    Q_OBJECT
public:
    explicit AnalyserTemporal(QObject *parent = 0);
    virtual ~AnalyserTemporal();

    static StatisticalMeasureDefinition measureDefinitionForCommunication(const QString &measureID);
    static StatisticalMeasureDefinition measureDefinitionForSpeaker(const QString &measureID);
    static QList<QString> measureIDsForCommunication();
    static QList<QString> measureIDsForSpeaker();

    double measureCom(const QString &measureID) const;
    double measureSpk(const QString &speakerID, const QString &measureID) const;
    QList<QString> speakerIDs() const;

    IntervalTier *getSpeakerTimeline(QPointer<Corpus> corpus, QPointer<CorpusCommunication> com);
    void calculate(QPointer<Corpus> corpus, QPointer<CorpusCommunication> com);

signals:

public slots:

private:
    AnalyserTemporalData *d;
    static QHash<QString, StatisticalMeasureDefinition> measureDefsCom;
    static QHash<QString, StatisticalMeasureDefinition> measureDefsSpk;
};

#endif // ANALYSERTEMPORAL_H

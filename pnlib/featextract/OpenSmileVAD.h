#ifndef OPENSMILEVAD_H
#define OPENSMILEVAD_H

#include <QObject>
#include <QString>
#include <QList>
#include <QPair>
#include "pncore/base/RealTime.h"

namespace Praaline {
namespace Core {
class Interval;
class CorpusRecording;
class IntervalTier;
}
}

class OpenSmileVAD : public QObject
{
    Q_OBJECT
public:
    explicit OpenSmileVAD(QObject *parent = 0);

    static bool
    runVAD(const QString &filenameInputWave, QList<QPair<double, double> > &resultVADActivations);

    static Praaline::Core::IntervalTier *
    splitToUtterances(QList<QPair<double, double> > &VADActivations,
                      RealTime minimumDurationSilent, RealTime minimumDurationVoice,
                      QString textSilent, QString textVoice);

    static QList<Praaline::Core::Interval *>
    splitToUtterancesWithoutPauses(QList<QPair<double, double> > &VADActivations,
                                   RealTime minimumDurationSilent, RealTime minimumDurationVoice,
                                   QString textVoice);

signals:

public slots:
};

#endif // OPENSMILEVAD_H

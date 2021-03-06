#ifndef DISFLUENCYANALYSERTOOL_H
#define DISFLUENCYANALYSERTOOL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "pncore/base/RealTime.h"
#include "pncore/annotation/Interval.h"
#include "pncore/annotation/IntervalTier.h"
#include "disfluency.h"

class DisfluencyAnalyserTool : public QObject
{
    Q_OBJECT
public:
    DisfluencyAnalyserTool(IntervalTier *tierTokens, QObject *parent = 0);

    Disfluency* disfluency(int index) const;
    QList<Disfluency *> disfluencies() const;

    bool readFromTier(IntervalTier *tierDisfluency, const QString &attribute = QString());

signals:

public slots:

private:
    IntervalTier *m_tierTokens;
    QList<Disfluency *> m_disfluencies;

    static QStringList m_simpleDisfluencyTags;
    static QStringList m_structuredDisfluecnyTags;

};

#endif // DISFLUENCYANALYSERTOOL_H

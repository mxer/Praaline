#ifndef SPEAKERTIMELINE_H
#define SPEAKERTIMELINE_H

#include "pncore_global.h"
#include <QObject>
#include <QString>
#include <QPointer>
#include "AnnotationTierGroup.h"

namespace Praaline {
namespace Core {

class PRAALINE_CORE_SHARED_EXPORT SpeakerTimeline : public QObject
{
    Q_OBJECT
public:
    explicit SpeakerTimeline(QMap<QString, QPointer<AnnotationTierGroup> > &tiers, const QString &levelID, QObject *parent = 0);
    ~SpeakerTimeline();


signals:

public slots:

private:
    QMap<QString, QPointer<AnnotationTierGroup> > &m_tiers;
    QString m_levelID;
};

} // namespace Core
} // namespace Praaline

#endif // SPEAKERTIMELINE_H
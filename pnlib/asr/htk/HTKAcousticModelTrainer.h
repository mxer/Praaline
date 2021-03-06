#ifndef HTKACOUSTICMODELTRAINER_H
#define HTKACOUSTICMODELTRAINER_H

#include <QObject>
#include "pncore/corpus/CorpusCommunication.h"

namespace Praaline {
namespace ASR {

struct HTKAcousticModelTrainerData;

class HTKAcousticModelTrainer : public QObject
{
    Q_OBJECT
public:
    explicit HTKAcousticModelTrainer(QObject *parent = 0);
    ~HTKAcousticModelTrainer();

signals:

public slots:

private:
    bool createMasterLabelFileFromTokensAndPhones(
            const QString &filename, QList<QPointer<Praaline::Core::CorpusCommunication> > trainingCommunications,
            const QString &levelSegment, const QString &levelToken, const QString &levelPhone);
    bool createMasterLabelFileFromTokens(
            const QString &filename, QList<QPointer<Praaline::Core::CorpusCommunication> > trainingCommunications,
            const QString &levelSegment, const QString &levelToken, const QString &attributePhonetisation);
    bool createMLF(
            const QString &filename, QList<QPointer<Praaline::Core::CorpusCommunication> > trainingCommunications,
            const QString &levelSegment, const QString &levelToken, const QString &levelPhone, const QString &attributePhonetisation);

    HTKAcousticModelTrainerData *d;
};

} // namespace ASR
} // namespace Praaline

#endif // HTKACOUSTICMODELTRAINER_H

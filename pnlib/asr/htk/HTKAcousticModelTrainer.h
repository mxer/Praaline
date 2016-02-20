#ifndef HTKACOUSTICMODELTRAINER_H
#define HTKACOUSTICMODELTRAINER_H

#include <QObject>
#include "pncore/corpus/corpus.h"
#include "pncore/corpus/corpuscommunication.h"

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
    bool createMasterLabelFileFromTokensAndPhones(const QString &filename, Corpus *trainingCorpus,
                                                  QList<QPointer<CorpusCommunication> > trainingCommunications,
                                                  const QString &levelSegment, const QString &levelToken, const QString &levelPhone);
    bool createMasterLabelFileFromTokens(const QString &filename, Corpus *trainingCorpus,
                                         QList<QPointer<CorpusCommunication> > trainingCommunications,
                                         const QString &levelSegment, const QString &levelToken, const QString &attributePhonetisation);
    bool createMLF(const QString &filename, Corpus *trainingCorpus,
                   QList<QPointer<CorpusCommunication> > trainingCommunications,
                   const QString &levelSegment, const QString &levelToken, const QString &levelPhone,
                   const QString &attributePhonetisation);

    HTKAcousticModelTrainerData *d;
};

#endif // HTKACOUSTICMODELTRAINER_H
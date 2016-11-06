#ifndef EXTERNALPHONETISER_H
#define EXTERNALPHONETISER_H

#include <QObject>
#include <QString>
#include "pncore/annotation/IntervalTier.h"
#include "pnlib/asr/SpeechToken.h"
#include "pnlib/asr/phonetiser/AbstractPhonetiser.h"

struct ExternalPhonetiserData;

class ExternalPhonetiser : public AbstractPhonetiser
{
    Q_OBJECT
public:
    explicit ExternalPhonetiser(QObject *parent = 0);
    ~ExternalPhonetiser();

    static QList<SpeechToken> phonetiseList(QList<Praaline::Core::Interval *> tokens, const QString &attributeID = QString());
    static QList<SpeechToken> phonetiseTier(Praaline::Core::IntervalTier *tier_tokens, const QString &attributeID = QString());
    static void addPhonetisationToTokens(Praaline::Core::IntervalTier *tier_tokens, const QString &attributeIDOrthographic,
                                         const QString &attributeIDPhonetisation);

signals:

public slots:

private:
    ExternalPhonetiserData *d;
};

#endif // EXTERNALPHONETISER_H

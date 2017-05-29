#ifndef XMLTRANSCRIPTION_H
#define XMLTRANSCRIPTION_H

#include <QString>
#include <QXmlStreamReader>
#include "pncore/base/RealTime.h"

class XMLTranscription
{
public:

    class TurnInfo {
    public:
        QString speakerID;
        QStringList tokens;
    };

    class ParagraphInfo {
    public:
        RealTime tMin;
        RealTime tMax;
        QString topic;
        QList<TurnInfo> turns;
    };

    QString topic;
    QString source_file;
    QList<ParagraphInfo> paragraphs;

    XMLTranscription();

    bool load(const QString &filename);

private:
    bool readText(QXmlStreamReader &xml);
    bool readParagraph(QXmlStreamReader &xml, ParagraphInfo &paragraph);
    bool readTurn(QXmlStreamReader &xml, TurnInfo &turn);
};

#endif // XMLTRANSCRIPTION_H

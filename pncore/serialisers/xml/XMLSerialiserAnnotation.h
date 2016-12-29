#ifndef XMLSERIALISERANNOTATION_H
#define XMLSERIALISERANNOTATION_H

#include "XMLSerialiserBase.h"
#include "structure/AnnotationStructure.h"
#include "annotation/AnnotationTierGroup.h"
#include "annotation/AnnotationTier.h"

namespace Praaline {
namespace Core {

class XMLSerialiserAnnotation : public XMLSerialiserBase
{

public:
    static bool saveTiers(const QString &annotationID, const QString &speakerID,
                          AnnotationTierGroup *tiers, AnnotationStructure *structure, const QString &filename);
    static bool saveTier(const QString &annotationID, const QString &speakerID,
                         AnnotationTier *tier, AnnotationStructure *structure, const QString &filename);

    static AnnotationTierGroup *getTiers(const QString &annotationID, const QString &speakerID,
                                         const AnnotationStructure *structure, const QString &filename,
                                         const QStringList &levelIDs = QStringList());
    static AnnotationTier *getTier(const QString &annotationID, const QString &speakerID,
                                   const AnnotationStructure *structure, const QString &filename,
                                   const QString &levelID, const QStringList &attributeIDs = QStringList());

    static QMap<RealTime, Interval *> getSpeakerTimeline(const QString &annotationID, const QString &levelID);

private:
    XMLSerialiserAnnotation();    
};

} // namespace Core
} // namespace Praaline

#endif // XMLSERIALISERANNOTATION_H
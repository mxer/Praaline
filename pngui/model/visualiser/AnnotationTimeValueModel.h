#ifndef ANNOTATIONTIMEVALUEMODEL_H
#define ANNOTATIONTIMEVALUEMODEL_H

#include <QPointer>
#include <QMap>
#include <QList>
#include <QPair>
#include <QStringList>

#include "pncore/base/RealTime.h"
#include "pncore/annotation/AnnotationTierGroup.h"
#include "svcore/base/XmlExportable.h"
#include "svcore/data/model/Model.h"

struct AnnotationTimeValueModelData;

class AnnotationTimeValueModel : public Model
{
    Q_OBJECT
public:
    explicit AnnotationTimeValueModel(sv_samplerate_t sampleRate,
                                      QMap<QString, QPointer<Praaline::Core::AnnotationTierGroup> > &tiers);
    virtual ~AnnotationTimeValueModel();

    // Model base class implementation
    virtual std::string getType() const { return "AnnotationTimeValueModel"; }
    virtual bool isOK() const { return true; }
    virtual sv_frame_t getStartFrame() const;
    virtual sv_frame_t getEndFrame() const;
    virtual sv_samplerate_t getSampleRate() const;
    virtual QString getTypeName() const { return tr("Annotation Time-Value"); }
    virtual void toXml(QTextStream &out, QString indent = "", QString extraAttributes = "") const;

protected:
    AnnotationTimeValueModelData *d;
};

#endif // ANNOTATIONTIMEVALUEMODEL_H

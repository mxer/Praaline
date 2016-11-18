#include <QDebug>
#include <QPointer>
#include <QtPlugin>
#include <QIcon>
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <ExtensionSystemConstants>

#include "pncore/corpus/Corpus.h"
#include "pncore/corpus/CorpusBookmark.h"
#include "pncore/serialisers/xml/XMLSerialiserCorpusBookmark.h"
#include "pncore/interfaces/praat/PraatTextGrid.h"

#include "ProsodicBoundariesAnnotator.h"
#include "SyllableProminenceAnnotator.h"
#include "pluginpromise.h"


using namespace Qtilities::ExtensionSystem;
using namespace Praaline::Plugins;

struct Praaline::Plugins::Promise::PluginPromisePrivateData {
    PluginPromisePrivateData() : createLevels(true), overwrite(true) {}

    bool createLevels;
    bool overwrite;
    QString command;
};

Praaline::Plugins::Promise::PluginPromise::PluginPromise(QObject* parent) : QObject(parent)
{
    d = new PluginPromisePrivateData;
    setObjectName(pluginName());
}

Praaline::Plugins::Promise::PluginPromise::~PluginPromise()
{
    delete d;
}

bool Praaline::Plugins::Promise::PluginPromise::initialize(const QStringList &arguments, QStringList *error_strings) {
    Q_UNUSED(arguments)
    Q_UNUSED(error_strings)

    return true;
}

bool Praaline::Plugins::Promise::PluginPromise::initializeDependencies(QStringList *error_strings) {
    Q_UNUSED(error_strings)

    return true;
}

void Praaline::Plugins::Promise::PluginPromise::finalize() {

}

QString Praaline::Plugins::Promise::PluginPromise::pluginName() const {
    return "Promise";
}

QtilitiesCategory Praaline::Plugins::Promise::PluginPromise::pluginCategory() const {
    return QtilitiesCategory(QApplication::applicationName());
}

Qtilities::Core::VersionInformation Praaline::Plugins::Promise::PluginPromise::pluginVersionInformation() const {
    VersionInformation version_info(1, 0, 0);
    return version_info;
}

QString Praaline::Plugins::Promise::PluginPromise::pluginPublisher() const {
    return "George Christodoulides";
}

QString Praaline::Plugins::Promise::PluginPromise::pluginPublisherWebsite() const {
    return "http://www.corpusannotation.org";
}

QString Praaline::Plugins::Promise::PluginPromise::pluginPublisherContact() const {
    return "info@corpusannotation.org";
}

QString Praaline::Plugins::Promise::PluginPromise::pluginDescription() const {
    return tr("Promise is a prosodic prominence annotator.");
}

QString Praaline::Plugins::Promise::PluginPromise::pluginCopyright() const {
    return QString(tr("Copyright") + " 2012-2014, George Christodoulides");
}

QString Praaline::Plugins::Promise::PluginPromise::pluginLicense() const {
    return tr("GPL v.3");
}

QList<IAnnotationPlugin::PluginParameter> Praaline::Plugins::Promise::PluginPromise::pluginParameters() const
{
    QList<IAnnotationPlugin::PluginParameter> parameters;
    parameters << PluginParameter("command", "Command", QVariant::String, d->command);
    return parameters;
}

void Praaline::Plugins::Promise::PluginPromise::setParameters(QHash<QString, QVariant> parameters)
{
    if (parameters.contains("command")) d->command = parameters.value("command").toString();
}

void Praaline::Plugins::Promise::PluginPromise::process(Corpus *corpus, QList<QPointer<CorpusCommunication> > communications)
{    
    QString appPath = QCoreApplication::applicationDirPath();
    QString filenameModelCrossNoPOS = appPath + "/plugins/promise/cross_nopos.model";
    QString filenameModelCrossPOS = appPath + "/plugins/promise/cross_pos.model";

    SyllableProminenceAnnotator *promise = new SyllableProminenceAnnotator();

    QPointer<IntervalTier> tier_syll;
    QPointer<IntervalTier> tier_token;

    // Save features used into the following intermediate files
    QFile fileFeatures(QString("/home/george/promise_features.txt"));
    if (! fileFeatures.open(QFile::WriteOnly | QFile::Text)) {
        printMessage("Cannot write to intermediate file (features).");
        return;
    }
    QTextStream streamFeatures(&fileFeatures);
    streamFeatures.setCodec("UTF-8");
    streamFeatures.generateByteOrderMark();
    QFile fileFeaturesCRF(QString("/home/george/promise_featurescrf.txt"));
    if (! fileFeaturesCRF.open(QFile::WriteOnly | QFile::Text)) {
        printMessage("Cannot write to intermediate file (CRF features).");
        return;
    }
    QTextStream streamFeaturesCRF(&fileFeaturesCRF);
    streamFeaturesCRF.setCodec("UTF-8");
    streamFeaturesCRF.generateByteOrderMark();
    // End save intermediate files

    int countDone = 0;
    madeProgress(0);
    printMessage("Promise Prosodic Prominence Annotator ver. 1.0 running");
    foreach(QPointer<CorpusCommunication> com, communications) {
        if (!com) continue;
        printMessage(QString("Annotating %1").arg(com->ID()));
        foreach (QPointer<CorpusAnnotation> annot, com->annotations()) {
            if (!annot) continue;
            QMap<QString, QPointer<AnnotationTierGroup> > tiersAll = corpus->datastoreAnnotations()->getTiersAllSpeakers(annot->ID());
            foreach (QString speakerID, tiersAll.keys()) {
                printMessage(QString("   speaker %1").arg(speakerID));
                QPointer<AnnotationTierGroup> tiers = tiersAll.value(speakerID);
                if (!tiers) continue;

                tier_syll = tiers->getIntervalTierByName("syll");
                if (!tier_syll) { printMessage("   Tier not found: syll (syllables). Aborting."); continue; }
                tier_token = tiers->getIntervalTierByName("tok_min");
                if (!tier_token) { printMessage("   Tier not found: tok_min (tokens). Aborting."); continue; }

                IntervalTier *tier_promise_nopos = promise->annotate(annot->ID(), filenameModelCrossNoPOS, false, "promise",
                                                                     tier_syll, tier_token, speakerID, streamFeatures, streamFeaturesCRF);
                IntervalTier *tier_promise_pos = promise->annotate(annot->ID(), filenameModelCrossPOS, true, "promise_pos",
                                                                   tier_syll, tier_token, speakerID, streamFeatures, streamFeaturesCRF);
                for (int i = 0; i < tier_syll->countItems(); ++i) {
                    if (tier_promise_nopos && i < tier_promise_nopos->countItems()) {
                        tier_syll->interval(i)->setAttribute("promise", tier_promise_nopos->interval(i)->text());
                    }
                    if (tier_promise_pos && i < tier_promise_pos->countItems()) {
                        tier_syll->interval(i)->setAttribute("promise_pos", tier_promise_pos->interval(i)->text());
                    }
                }

                corpus->datastoreAnnotations()->saveTier(annot->ID(), speakerID, tier_syll);
                qDebug() << QString("Annotated %1, speaker %2").arg(annot->ID()).arg(speakerID);
                delete tier_promise_nopos;
                delete tier_promise_pos;
            }
            qDeleteAll(tiersAll);
            QApplication::processEvents();
        }
        countDone++;
        madeProgress(countDone * 100 / communications.count());
    }
    delete promise;

    fileFeatures.close();
    fileFeaturesCRF.close();

    madeProgress(100);
    printMessage("Promise finished.");
}

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    using namespace Praaline::Plugins::Promise;
    Q_EXPORT_PLUGIN2(PluginPromise, PluginPromise)
#endif

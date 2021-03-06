#include <QString>
#include <QList>
#include <QStringList>
#include <QMap>
#include <QHash>
#include <QPointer>
#include <QScopedPointer>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QTime>

#include "ProsodicBoundaries.h"
#include "pncore/base/RealValueList.h"
#include "svcore/data/model/SparseOneDimensionalModel.h"
#include "svcore/data/model/SparseTimeValueModel.h"
#include "pncore/corpus/Corpus.h"
#include "pncore/corpus/CorpusSpeaker.h"
#include "pncore/corpus/CorpusBookmark.h"
#include "pncore/annotation/AnnotationTierGroup.h"
#include "pncore/annotation/PointTier.h"
#include "pncore/datastore/CorpusRepository.h"
#include "pncore/datastore/AnnotationDatastore.h"
#include "pncore/serialisers/xml/XMLSerialiserCorpusBookmark.h"
using namespace Praaline::Core;

#include "ProsodicBoundaries.h"
#include "ProsodicBoundariesExperimentAnalysis.h"



// ====================================================================================================================
// 2. Read results from OpenSesame files into Praaline database
// ====================================================================================================================

bool ProsodicBoundariesExperimentAnalysis::resultsReadTapping(const QString &subjectID, const QString &filename, Corpus *corpus)
{
    QFile file(filename);
    if (!file.open( QIODevice::ReadOnly | QIODevice::Text )) return false;
    QTextStream stream(&file);
    QString stimulusID;
    long t0 = 0, t1 = 0;
    RealTime timeKeyDown, timeKeyUp;
    QList<Interval *> eventIntervals;
    int state = 0; // 0 = start, 1 = in tapping, 2 = seen key down (key up > 1, ends > 0)
    do {
        QString line = stream.readLine().trimmed();
        if (line.startsWith("<tapping ")) {
            QList<QString> fields = line.split("\"");
            stimulusID = QString(fields.at(1)).replace("stimuli/", "").replace(".wav", "");
            t0 = fields.at(3).toLong();
            state = 1;
        }
        else if (line.startsWith("<key_down ") && state == 1) {
            QList<QString> fields = line.split("\"");
            long time = fields.at(3).toLong();
            timeKeyDown = RealTime::fromMilliseconds(time);
            state = 2;
        }
        else if (line.startsWith("<key_up ") && state == 2) {
            QList<QString> fields = line.split("\"");
            long time = fields.at(3).toLong();
            timeKeyUp = RealTime::fromMilliseconds(time);
            // add interval
            eventIntervals << new Interval(timeKeyDown, timeKeyUp, "x");
            state = 1;
        }
        else if (line.startsWith("<ends")) {
            QList<QString> fields = line.split("\"");
            t1 = fields.at(3).toLong();
            if (state == 2) { // pending key down
                timeKeyUp = RealTime::fromMilliseconds(t1 - t0);
                eventIntervals << new Interval(timeKeyDown, timeKeyUp, "x");
            }
            state = 0;
            // add to database
            IntervalTier *tier = new IntervalTier("tapping", eventIntervals, RealTime(0, 0), RealTime::fromMilliseconds(t1 - t0));
            corpus->repository()->annotations()->saveTier(stimulusID, subjectID, tier);
            delete tier;
            eventIntervals.clear();
        }
    } while (!stream.atEnd());
    file.close();
    return true;
}

bool ProsodicBoundariesExperimentAnalysis::resultsReadParticipantInfo(const QString &subjectID, const QString &expeType,
                                        const QString &filename, Corpus *corpus)
{
    QFile file(filename);
    if (!file.open( QIODevice::ReadOnly | QIODevice::Text )) return false;
    QTextStream stream(&file);
    QPointer<CorpusSpeaker> spk;
    do {
        QString line = stream.readLine().trimmed();
        if (line.startsWith("<participant_id ")) {
            // 0                     1 2     3 4    5 6             7 8       9
            // <participant_id prog="0" sex="1" L1="0" hearingprob="1" music="1" />
            QStringList fields = line.split("\"");
            QString programme = (fields.at(1) == "0") ? "FIAL" : "PSP";
            QString sex = (fields.at(3) == "0") ? "M" : "F";
            QString L1 = (fields.at(5) == "0") ? "FR" : "other";
            bool hearingProb = (fields.at(7) == "0") ? true : false;
            bool music = (fields.at(9) == "0") ? true : false;
            spk = corpus->speaker(subjectID);
            if (spk) {
                spk->setName(spk->ID());
                spk->setProperty("sex", sex);
                spk->setProperty("isExperimentSubject", true);
                spk->setProperty("experimentType", expeType);
                spk->setProperty("studyProgramme", programme);
                spk->setProperty("languageL1", L1);
                spk->setProperty("hasKnownHearingProblems", hearingProb);
                spk->setProperty("hasMusicalTraining", music);
            }
        }
    } while (!stream.atEnd());
    file.close();
    return true;
}

// ====================================================================================================================
// 3. Initial analysis: convert tapping to Perceived Prosodic Boundaries and attribute to syllables
// ====================================================================================================================

void ProsodicBoundariesExperimentAnalysis::analysisCalculateDeltaRT(Corpus *corpus)
{
    if (!corpus) return;
    QList<QList<RealTime> > beeps;
    QList<RealTime> beeps01, beeps02;
    beeps01 << RealTime::fromSeconds(0.982267573)
            << RealTime::fromSeconds(4.059274376)
            << RealTime::fromSeconds(11.925895691)
            << RealTime::fromSeconds(21.612879818)
            << RealTime::fromSeconds(22.501360544)
            << RealTime::fromSeconds(32.0);
    beeps02 << RealTime::fromSeconds(0.989750566)
            << RealTime::fromSeconds(10.675374149)
            << RealTime::fromSeconds(12.062766439)
            << RealTime::fromSeconds(15.136507936)
            << RealTime::fromSeconds(22.500136054)
            << RealTime::fromSeconds(32.0);
    beeps << beeps01 << beeps02;
    foreach (CorpusSpeaker *spk, corpus->speakers()) {
        if (spk->property("isExperimentSubject").toInt() == 0) continue;
        if (spk->property("exclude").toInt() > 0) continue;

        QList<RealValueList > dRTs;
        RealValueList dRTsAll;
        dRTs << RealValueList() << RealValueList();
        for (int k = 1; k <= 2; ++k) {
            QString annotationID = QString("beeps0%1").arg(k);
            AnnotationTierGroup *tiers = corpus->repository()->annotations()->getTiers(annotationID, spk->ID());
            if (!tiers) { qDebug() << "no beeps annotation" << annotationID << spk->ID(); continue; }
            QString tierName = "tapping";
            // The next two lines only for the Nassima experiment
            if (k == 1) tierName = "tapping_boundaries";
            else if (k == 2) tierName = "tapping_pauses";
            IntervalTier *tier_tap = tiers->getIntervalTierByName(tierName);
            if (!tier_tap) { qDebug() << "no tier tapping"; continue; }
            QList<RealTime> tap_down;
            foreach (Interval *intv, tier_tap->intervals()) {
                if (intv->text() == "x") tap_down << intv->tMin();
            }
            for (int i = 0; i < 5; ++i) {
                RealTime beep = beeps.at(k - 1).at(i);
                RealTime nextBeep = beeps.at(k - 1).at(i + 1);
                bool found = false;
                foreach (RealTime t, tap_down) {
                    if ((!found) && (t > beep) && (t < nextBeep)) {
                        RealTime dRT = t - beep;
                        qDebug() << spk->ID() << " " << beep.toDouble() << " "  << t.toDouble();
                        if (dRT < RealTime::fromMilliseconds(600)) {
                            dRTs[k-1].append(dRT.toDouble());
                            dRTsAll.append(dRT.toDouble());
                        }
                        found = true;
                    }
                }
            }
            spk->setProperty(QString("drt_avg_0%1").arg(k), dRTs[k-1].mean());
            spk->setProperty(QString("drt_sd_0%1").arg(k), dRTs[k-1].stddev());
            dRTs[k-1].clear();
            delete tiers;
        }
        spk->setProperty("drt_avg", dRTsAll.mean());
        spk->setProperty("drt_sd", dRTsAll.stddev());
        dRTsAll.clear();
    }
    corpus->save();
}

void ProsodicBoundariesExperimentAnalysis::analysisCreateAdjustedTappingTier(Corpus *corpus, const QString &tierName)
{
    if (!corpus) return;
    foreach (CorpusCommunication *com, corpus->communications()) {
        foreach (CorpusSpeaker *spk, corpus->speakers()) {
            if (!spk->property("isExperimentSubject").toBool()) continue;
            AnnotationTierGroup *tiers = corpus->repository()->annotations()->getTiers(com->ID(), spk->ID());
            if (!tiers) { qDebug() << "no beeps annotation" << com->ID() << spk->ID(); continue; }
            IntervalTier *tier_tap = tiers->getIntervalTierByName(tierName);
            if (!tier_tap) { qDebug() << "no tier tapping"; continue; }
            IntervalTier *tier_tapAdj = new IntervalTier(tier_tap, tierName + "Adj");
            RealTime delta = RealTime::fromSeconds(spk->property("drt_avg").toDouble());
            tier_tapAdj->timeShift(-delta);
            corpus->repository()->annotations()->saveTier(com->ID(), spk->ID(), tier_tapAdj);
        }
    }
}

void ProsodicBoundariesExperimentAnalysis::analysisCalculateSmoothedTappingModel(Corpus *corpus, int maxNumberOfSubjects, const QString &tierName)
{
    if (!corpus) return;
    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());
    foreach (CorpusCommunication *com, corpus->communications()) {
        QMap<QString, QPointer<AnnotationTierGroup> > tiers = corpus->repository()->annotations()->getTiersAllSpeakers(com->ID());

        // Sparse one-dimensional model of key-down events, adjusted for subject RT
        sv_samplerate_t sampleRate = 16000;
        QScopedPointer<SparseOneDimensionalModel> modelInstants(new SparseOneDimensionalModel(sampleRate, 1));
        RealTime tMax;
        int totalAnnotators = 0;
        QStringList subjectIDs;
        // Select subjects (randomly up to maxNumberOfSubjects).
        foreach (QString subjectID, tiers.keys()) {
            if (subjectID.isEmpty()) continue;
            QPointer<AnnotationTierGroup> tiersSubj = tiers.value(subjectID);
            if (!tiersSubj->tierNames().contains(tierName + "Adj")) continue;
            subjectIDs << subjectID;
        }
        if (maxNumberOfSubjects > 0)  {
            while (subjectIDs.count() > maxNumberOfSubjects) {
                subjectIDs.removeAt(qrand() % subjectIDs.count());
            }
        }
        qDebug() << com->ID() << " with " << subjectIDs.count() << " subjects: " << subjectIDs.join(", ");

        foreach (QString subjectID, subjectIDs) {
            QPointer<AnnotationTierGroup> tiersSubj = tiers.value(subjectID);
            QPointer<IntervalTier> tier_tapping = tiersSubj->getIntervalTierByName(tierName + "Adj");
            if (!tier_tapping) continue;
            if (subjectID.isEmpty()) continue;
            totalAnnotators++; // Count the subject as a potential annotator even if they did not tap
            if (tier_tapping->count() == 1) continue; // The subject did not tap (not a single time) on this sample
            if (tier_tapping->tMax() > tMax) tMax = tier_tapping->tMax();
            foreach(Interval *intv, tier_tapping->intervals()) {
                if (intv->text() != "x") continue;
                SparseOneDimensionalModel::Point point(RealTime::realTime2Frame(intv->tMin(), sampleRate),
                                                       subjectID);
                modelInstants->addPoint(point);
            }
        }

        // Smoothed time-value model, calculated using a sliding window / also saved as a point-tier
        QList<Point *> smoothPoints;

        RealTime step = RealTime::fromMilliseconds(25);// calculate smoothed curve
        //    ProsodicBoundariesExperimentAnalysis::analysisCalculateSmoothedTappingModel(corpus);
        RealTime slidingWindowLeft = RealTime::fromMilliseconds(250);
        RealTime slidingWindowRight = RealTime::fromMilliseconds(250);
        RealTime groupingWindowLeft = RealTime::fromMilliseconds(500);
        RealTime groupingWindowRight = RealTime::fromMilliseconds(500);
        RealTime groupingDistanceLimit = RealTime::fromMilliseconds(300);

        // convert time values to frames in current resolution
        sv_frame_t stepF = RealTime::realTime2Frame(step, sampleRate);
        sv_frame_t slidingWindowLeftF = RealTime::realTime2Frame(slidingWindowLeft, sampleRate);
        sv_frame_t slidingWindowRightF = RealTime::realTime2Frame(slidingWindowRight, sampleRate);
        sv_frame_t groupingWindowLeftF = RealTime::realTime2Frame(groupingWindowLeft, sampleRate);
        sv_frame_t groupingWindowRightF = RealTime::realTime2Frame(groupingWindowRight, sampleRate);
        sv_frame_t groupingDistanceLimitF = RealTime::realTime2Frame(groupingDistanceLimit, sampleRate);

        for (sv_frame_t frame = 0; frame <= RealTime::realTime2Frame(tMax, sampleRate); frame += stepF) {
            // outermost limits
            sv_frame_t groupingFrame0 = frame - groupingWindowLeftF;
            sv_frame_t groupingFrame1 = frame + groupingWindowRightF;
            // 1. Select group of PPBs at a specific point
            SparseOneDimensionalModel::PointList PPBs;
            sv_frame_t previousFrame;
            // IMPORTANT NOTE: getPoints is not precise (may return +/-2 points before and after the interval passed to
            // the function - we have to check the returned values! (cf. Sonic Visualiser documentation)
            // Left side
            SparseOneDimensionalModel::PointList leftPPBs = modelInstants->getPoints(groupingFrame0, frame);
            previousFrame = frame;
            for (SparseOneDimensionalModel::PointList::const_reverse_iterator i = leftPPBs.rbegin(); i != leftPPBs.rend(); ++i) {
                const SparseOneDimensionalModel::Point &p(*i);
                if (p.label.isEmpty()) continue;
                if (p.frame > frame) continue;
                if (p.frame < groupingFrame0) break;
                if (previousFrame -  p.frame > groupingDistanceLimitF) break;
                PPBs.insert(p);
                previousFrame = p.frame;
            }
            // Right side
            SparseOneDimensionalModel::PointList rightPPBs = modelInstants->getPoints(frame, groupingFrame1);
            previousFrame = frame;
            for (SparseOneDimensionalModel::PointList::const_iterator i = rightPPBs.begin(); i != rightPPBs.end(); ++i) {
                const SparseOneDimensionalModel::Point &p(*i);
                if (p.label.isEmpty()) continue;
                if (p.frame <= frame) continue;
                if (p.frame > groupingFrame1) break;
                if (p.frame - previousFrame > groupingDistanceLimitF) break;
                PPBs.insert(p);
                previousFrame = p.frame;
            }
            // Calculations based on group
            QStringList groupSubjectsHavingAnnotated;
            QStringList groupTimesAdj;
            QStringList groupTimesOrig;
            RealTime groupFirstPPB, groupLastPPB;
            bool isFirst = true;
            for (SparseOneDimensionalModel::PointList::const_iterator i = PPBs.begin(); i != PPBs.end(); ++i) {
                const SparseOneDimensionalModel::Point &p(*i);
                if (p.label.isEmpty()) { qDebug() << "empty label" << com->ID() << p.frame; continue; }
                RealTime t = RealTime::frame2RealTime(p.frame, sampleRate);
                if (isFirst) { groupFirstPPB = t; isFirst = false; }
                if (!groupSubjectsHavingAnnotated.contains(p.label)) {
                    CorpusSpeaker *spk = corpus->speaker(p.label);
                    groupSubjectsHavingAnnotated << p.label;
                    groupTimesAdj << QString("%1").arg(t.toDouble());
                    RealTime orig = t + RealTime::fromSeconds(spk->property("drt_avg").toDouble());
                    groupTimesOrig << QString("%1").arg(orig.toDouble());
                    if (groupLastPPB < t) groupLastPPB = t;
                }
            }

            // 2. Calculate continuous strength using a sliding window
            sv_frame_t slidingFrame0 = frame - slidingWindowLeftF;
            sv_frame_t slidingFrame1 = frame + slidingWindowRightF;
            int slidingCount = 0;
            QStringList slidingSubjectsHavingAnnotated;
            SparseOneDimensionalModel::PointList windowPPBs = modelInstants->getPoints(slidingFrame0, slidingFrame1);
            for (SparseOneDimensionalModel::PointList::const_iterator i = windowPPBs.begin(); i != windowPPBs.end(); ++i) {
                const SparseOneDimensionalModel::Point &p(*i);
                if (p.label.isEmpty()) continue;
                if (p.frame < slidingFrame0) continue;
                if (p.frame > slidingFrame1) continue;
                if (!slidingSubjectsHavingAnnotated.contains(p.label)) {
                    slidingSubjectsHavingAnnotated << p.label;
                    slidingCount++;
                }
            }

            // 3. Update values
            com->setProperty(tierName + "totalAnnotators", totalAnnotators);
            double force = ((double)slidingCount) / ((double)totalAnnotators);
            Point *sp = new Point(RealTime::frame2RealTime(frame, sampleRate), groupSubjectsHavingAnnotated.join(","));
            sp->setAttribute("force", force);
            sp->setAttribute("firstPPB", groupFirstPPB.toDouble());
            sp->setAttribute("lastPPB", groupLastPPB.toDouble());
            sp->setAttribute("subjects", groupSubjectsHavingAnnotated.join("|"));
            sp->setAttribute("timesOrig", groupTimesOrig.join("|"));
            sp->setAttribute("timesAdj", groupTimesAdj.join("|"));
            smoothPoints << sp;
        }
        QScopedPointer<PointTier> tier_smooth(new PointTier(tierName + "_smooth", smoothPoints, RealTime(0,0), tMax));

        // Calculate local maxima
        QList<Point *> maxima;
        foreach (Point *p, tier_smooth->findLocalMaxima(RealTime::fromMilliseconds(250), "force")) {
            double force = p->attribute("force").toDouble();
            if (force < 0.2) continue;
            maxima << p;
        }
        QList<Point *> filtered;
        int i = 0;
        QList<Point *> group;
        while (i < maxima.count()) {
            group << maxima.at(i);
            if ((i == maxima.count() - 1) || (maxima.at(i+1)->time() - maxima.at(i)->time() > RealTime::fromMilliseconds(250))) {
                int j = group.count() / 2;
                filtered << group.at(j);
                group.clear();
            } else {
                group << maxima.at(i);
            }
            ++i;
        }
        foreach (Point *p, filtered) {
            p->setAttribute("localMax", true);
        }

        corpus->repository()->annotations()->saveTier(com->ID(), "smooth", tier_smooth.data());
        qDeleteAll(tiers);
    }
    corpus->save(); // make sure you save the total annotators data!
}

struct PerceivedBoundary {
    QString subject;
    RealTime timeAdj;
    RealTime timeOrig;
};

QList<PerceivedBoundary> groupFromLists(QStringList &subjects, QStringList &timesAdj, QStringList &timesOrig)
{
    QList<PerceivedBoundary> list;
    if (subjects.count() != timesAdj.count() || timesAdj.count() != timesOrig.count()) {
        qDebug() << "bad group!";
        return list;
    }
    for (int i = 0; i < subjects.count(); ++i) {
        PerceivedBoundary ppb;
        ppb.subject = subjects.at(i);
        ppb.timeAdj = RealTime::fromSeconds(timesAdj.at(i).toDouble());
        ppb.timeOrig = RealTime::fromSeconds(timesOrig.at(i).toDouble());
        list << ppb;
    }
    return list;
}

QList<PerceivedBoundary> groupFromSP(Point *sp)
{
    QStringList subjects = sp->attribute("subjects").toString().split("|");
    QStringList timesAdj = sp->attribute("timesAdj").toString().split("|");
    QStringList timesOrig = sp->attribute("timesOrig").toString().split("|");
    return groupFromLists(subjects, timesAdj, timesOrig);
}

QList<PerceivedBoundary> groupFromSyll(Interval *syll, QString prefix)
{
    QStringList subjects = syll->attribute(prefix + "Subjects").toString().split("|");
    QStringList timesAdj = syll->attribute(prefix + "TimesAdj").toString().split("|");
    QStringList timesOrig = syll->attribute(prefix + "TimesOrig").toString().split("|");
    return groupFromLists(subjects, timesAdj, timesOrig);
}

void ProsodicBoundariesExperimentAnalysis::analysisAttributeTappingToSyllablesLocalMaxima(Corpus *corpus, const QString &levelForUnits,
                                                                                          const QString &prefix, const QString &tierName)
{
    if (!corpus) return;
    foreach (CorpusCommunication *com, corpus->communications()) {
        QMap<QString, QPointer<AnnotationTierGroup> > tiers = corpus->repository()->annotations()->getTiersAllSpeakers(com->ID());
        QPointer<AnnotationTierGroup> tiersSmooth = tiers.value("smooth"); // smooth is the fake speakerID
        if (!tiersSmooth) continue;
        QPointer<PointTier> tier_smooth = tiersSmooth->getPointTierByName(tierName + "_smooth");
        if (!tier_smooth) continue;
        foreach (QString speakerID, tiers.keys()) {
            QPointer<AnnotationTierGroup> tiersSpk = tiers.value(speakerID);
            IntervalTier *tier_syll = tiersSpk->getIntervalTierByName("syll");
            if (!tier_syll) continue;

            // Clean up PPB data in syllables
            foreach (Interval *syll, tier_syll->intervals()) {
                syll->setAttribute(prefix + "Delay", 0.0);
                syll->setAttribute(prefix + "Dispersion", 0.0);
                syll->setAttribute(prefix + "Force", 0.0);
                syll->setAttribute(prefix + "FirstPPB", 0.0);
                syll->setAttribute(prefix + "LastPPB", 0.0);
                syll->setAttribute(prefix + "Subjects", "");
                syll->setAttribute(prefix + "TimesAdj", "");
                syll->setAttribute(prefix + "TimesOrig", "");
            }

            if (tier_syll->count() < 10) {
                corpus->repository()->annotations()->saveTier(com->ID(), speakerID, tier_syll);
                continue; // (secondary speakers, not analysed)
            }

            QPointer<IntervalTier> tier_basic_units = tiersSpk->getIntervalTierByName(levelForUnits);
            if (!tier_basic_units) continue;
            QPointer<IntervalTier> tier_tokens = tiersSpk->getIntervalTierByName("tok_min");

            // For each syllable, mark which is the corresponding "last syll" of the basic unit
            QList<int> lastSyllables;
            int last_syll = -1;
            foreach (Interval *unit, tier_basic_units->intervals()) {
                if (unit->isPauseSilent()) {
                    lastSyllables << ((last_syll < 0) ? 0 : last_syll);
                    last_syll++;
                } else {
                    Interval *syllAtRightBoundaryOfUnit = tier_syll->intervalAtTime(unit->tMax() - RealTime(0, 10));
                    if (syllAtRightBoundaryOfUnit->tMax() == unit->tMax()) {
                        int prev_last_syll = last_syll;
                        last_syll = tier_syll->intervalIndexAtTime(unit->tMax() - RealTime(0, 10));
                        for (int i = prev_last_syll; i < last_syll; ++i) lastSyllables << last_syll;
                    } else {
                        // otherwise, enchainement, continue to next word
                        qDebug() << com->ID() << " " << syllAtRightBoundaryOfUnit->tMax().toDouble() << " " << unit->tMax().toDouble();
                    }
                }
            }            
            if (lastSyllables.count() != tier_syll->count()) {
                qDebug() << "Error: lastSyllables count != syllables count " << lastSyllables.count() << " " << tier_syll->count();
                continue; // and investiage
            }
            // Corrections to the algorithm for long pauses and monosyllabic words
            // s0 _ s1 s2 ==> move boundary from s1 to s0
            // s0 _ s1 _  AND s1 is a # ==> move boundary to s0
            for (int i = 1; i < tier_syll->count() - 2; ++i) {
                Interval *s0 = tier_syll->interval(i - 1);
                Interval *sX = tier_syll->interval(i);
                Interval *s1 = tier_syll->interval(i + 1);
                if ((!s0->isPauseSilent()) && (sX->isPauseSilent()) && (!s1->isPauseSilent())) {
                    Interval *s2 = tier_syll->interval(i + 2);
                    if (!s2->isPauseSilent()) {
                        lastSyllables[i + 1] = i - 1;
                        qDebug() << "Move potential boundary position " << com->ID() << " for syll " << i + 1 << " to syll " << i-1;
                    } else {
                        if (s1->attribute("boundary").toString().contains("#")) {
                            lastSyllables[i + 1] = i - 1;
                            qDebug() << "Move potential boundary position " << com->ID() << " for syll " << i + 1 << " to syll " << i-1;
                        }
                    }
                }
            }
            // Add an attribute to "last syllables" indicating that they are a potential PPB site
            for (int i = 0; i < tier_syll->count(); ++i) {
                if (lastSyllables.at(i) == i)
                    tier_syll->interval(i)->setAttribute(prefix + "PotentialSite", true);
                else
                    tier_syll->interval(i)->setAttribute(prefix + "PotentialSite", false);
            }

            QString tokens, syllables, potentials;
            foreach (Interval *unit, tier_basic_units->intervals()) {
                foreach (Interval *token, tier_tokens->getIntervalsContainedIn(unit)) tokens.append(token->text()).append(" ");
                tokens = tokens.trimmed().append("\t");
                foreach (Interval *syll, tier_syll->getIntervalsContainedIn(unit)) syllables.append(syll->text()).append(" ");
                syllables = syllables.trimmed().append("\t");
                for (int i = 0; i < tier_syll->count(); ++i) {
                    if (lastSyllables.at(i) == i) potentials.append("1 "); else potentials.append("0 ");
                }
                potentials = potentials.trimmed().append("\t");
            }


            // Go through local maxima and attribute PPB groups to syllables
            QHash<int, QList<int> > assignments;
            for (int spIndex = 0; spIndex < tier_smooth->count(); ++spIndex) {
                Point *sp = tier_smooth->point(spIndex);
                if (sp->attribute("localMax").toBool()) {
                    int target_syll;
                    if (sp->time() > tier_syll->tMax())
                        target_syll = tier_syll->count() - 1;
                    else
                        target_syll = tier_syll->intervalIndexAtTime(sp->time());
                    if (target_syll < 0) {
                        qDebug() << "syll not found" << com->ID() << " at " << sp->time().toDouble();
                        continue;
                    }
                    // move to corresponding last syllable
                    target_syll = lastSyllables.at(target_syll);

                    if (assignments.contains(target_syll)) {
                        QList<int> list = assignments.value(target_syll);
                        list.append(spIndex);
                        assignments[target_syll] = list;
                    } else {
                        assignments[target_syll] << spIndex;
                    }
                }
            }

            foreach (int syllIndex, assignments.keys()) {
                QList<int> groups = assignments.value(syllIndex);
                QMap<RealTime, PerceivedBoundary> merged;
                QList<QString> mergedSubjectIDs;
                foreach (int spIndex, groups) {
                    QList<PerceivedBoundary> group = groupFromSP(tier_smooth->point(spIndex));
                    foreach (PerceivedBoundary PPB, group) {
                        if (!mergedSubjectIDs.contains(PPB.subject)) {
                            merged.insert(PPB.timeAdj, PPB);
                            mergedSubjectIDs << PPB.subject;
                        } else {
                            qDebug() << "Avoided counting tapping by the same subject twice " << com->ID() << " syll index " << syllIndex;
                        }
                    }
                }
                QStringList subjects, timesAdj, timesOrig;
                foreach (PerceivedBoundary PPB, merged) {
                    if (!subjects.contains(PPB.subject)) {
                        subjects << PPB.subject;
                        timesAdj << QString("%1").arg(PPB.timeAdj.toDouble());
                        timesOrig << QString("%1").arg(PPB.timeOrig.toDouble());
                    } else {
                        qDebug() << "Error subject included twice " << com->ID() << " syll index " << syllIndex << " subject " << PPB.subject;
                    }
                }
                int totalAnnotators = com->property(tierName + "totalAnnotators").toInt();
                double force = ((double)(merged.count())) / ((double)totalAnnotators);
                Interval *syll = tier_syll->interval(syllIndex);

                if (force > 1.0) {
                    qDebug() << "Error force > 1 at " << com->ID() << " syll index " << syllIndex << " force " << force;
                    continue;
                }

                syll->setAttribute(prefix + "Force", force);
                syll->setAttribute(prefix + "FirstPPB", merged.first().timeAdj.toDouble());
                syll->setAttribute(prefix + "LastPPB", merged.last().timeAdj.toDouble());
                syll->setAttribute(prefix + "Subjects", subjects.join("|"));
                syll->setAttribute(prefix + "TimesAdj", timesAdj.join("|"));
                syll->setAttribute(prefix + "TimesOrig", timesOrig.join("|"));
            }
            corpus->repository()->annotations()->saveTier(com->ID(), speakerID, tier_syll);
        }
        qDeleteAll(tiers);
        qDebug() << com->ID();
    }
}

void ProsodicBoundariesExperimentAnalysis::analysisStabilisation(Corpus *corpus, int maxNumberOfSubjects, int iterations, QString prefix)
{
    if (!corpus) return;
    QMap<QString, RealValueList > syllBoundaryForces;

    for (int iter = 0; iter < iterations; ++iter) {
        qDebug() << "ITERATION " << iter << "==========================================================";
        analysisCalculateSmoothedTappingModel(corpus, maxNumberOfSubjects);
        analysisAttributeTappingToSyllablesLocalMaxima(corpus, "tok_min", prefix);

        foreach (CorpusCommunication *com, corpus->communications()) {
            QMap<QString, QPointer<AnnotationTierGroup> > tiers = corpus->repository()->annotations()->getTiersAllSpeakers(com->ID());
            foreach (QString speakerID, tiers.keys()) {
                QPointer<AnnotationTierGroup> tiersSpk = tiers.value(speakerID);
                IntervalTier *tier_syll = tiersSpk->getIntervalTierByName("syll");
                if (!tier_syll) continue;
                for (int syllNo = 0; syllNo < tier_syll->count(); ++syllNo) {
                    Interval *syll = tier_syll->interval(syllNo);
                    if (!syll->attribute(prefix + "PotentialSite").toBool()) continue;
                    QString ID = QString("%1\t%2\t%3\t%4")
                            .arg(com->ID())
                            .arg(syllNo, 3, 10, QChar('0'))
                            .arg(QString(syll->text()).replace("\t", "").replace("\n", ""))
                            .arg(QString::number(syll->tMin().toDouble()));
                    syllBoundaryForces[ID].append(syll->attribute(prefix + "Force").toDouble());
                }
            }
            qDeleteAll(tiers);
        }
    }
    // Save results
    QString path = "/home/george/Dropbox/2015-10 SP8 - Prosodic boundaries perception experiment/analyses/";
    QFile file(path + QString("convergence_%1_%2.txt").arg(maxNumberOfSubjects).arg(iterations));
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ) return;
    QTextStream out(&file);
    out.setCodec("UTF-8");
    foreach (QString ID, syllBoundaryForces.keys()) {
        out << ID << "\t";
        out << QString::number(syllBoundaryForces[ID].mean(), 'f', 6) << "\t";
        out << QString::number(syllBoundaryForces[ID].stddev(), 'f', 6) << "\t";
        foreach (double f, syllBoundaryForces[ID]) {
            out << QString::number(f, 'f', 3) << ";";
        }
        out << "\n";
    }
}


void ProsodicBoundariesExperimentAnalysis::calculateDelayAndDispersion(Corpus *corpus, QString prefix)
{
    if (!corpus) return;
    QStringList vowels;
    vowels << "a" << "e" << "i" << "o" << "u" << "E" << "O" << "2" << "9" << "y" << "a~" << "e~" << "o~" << "2~" << "9~" << "@" ;
    foreach (CorpusCommunication *com, corpus->communications()) {
        QMap<QString, QPointer<AnnotationTierGroup> > tiers = corpus->repository()->annotations()->getTiersAllSpeakers(com->ID());
        foreach (QString speakerID, tiers.keys()) {
            QPointer<AnnotationTierGroup> tiersSpk = tiers.value(speakerID);
            IntervalTier *tier_syll = tiersSpk->getIntervalTierByName("syll");
            if (!tier_syll) continue;
            IntervalTier *tier_phone = tiersSpk->getIntervalTierByName("phone");
            if (!tier_phone) continue;

            foreach (Interval *syll, tier_syll->intervals()) {
                if (syll->attribute(prefix + "Force").toDouble() == 0.0) continue;
                QList<PerceivedBoundary> PPBs = groupFromSyll(syll, prefix);
                if (PPBs.isEmpty()) continue;
                // Find syll nucleus / center
                RealTime t0;
                QList<Interval *> phones = tier_phone->getIntervalsContainedIn(syll);
                foreach (Interval *phone, phones) {
                    if (vowels.contains(phone->text())) {
                        t0 = phone->tMin();
                    }
                }
                if (t0 == RealTime()) {
                    t0 = syll->tCenter();
                }
                // Now calculate average delay and dispersion
                RealValueList responseTimes;
                foreach (PerceivedBoundary PPB, PPBs) {
                    responseTimes << (PPB.timeOrig - t0).toDouble();
                }
                syll->setAttribute(prefix + "Delay", responseTimes.mean());
                syll->setAttribute(prefix + "Dispersion", responseTimes.stddev());
            }

            corpus->repository()->annotations()->saveTier(com->ID(), speakerID, tier_syll);
        }
        qDeleteAll(tiers);
        qDebug() << com->ID();
    }
}

void ProsodicBoundariesExperimentAnalysis::analysisCalculateCoverage(Corpus *corpus, QString prefix)
{
    if (!corpus) return;
    QHash<QString, int> AllPPBs;
    foreach (CorpusCommunication *com, corpus->communications()) {
        QString id = com->ID();
        if (!id.startsWith("A") && !id.startsWith("B")) continue;

        QMap<QString, QPointer<AnnotationTierGroup> > tiers = corpus->repository()->annotations()->getTiersAllSpeakers(com->ID());
        foreach (QString speakerID, tiers.keys()) {
            QPointer<AnnotationTierGroup> tiersSpk = tiers.value(speakerID);
            IntervalTier *tier_syll = tiersSpk->getIntervalTierByName("syll");
            if (!tier_syll) continue;
            foreach (Interval *syll, tier_syll->intervals()) {
                if (syll->attribute(prefix + "Force").toDouble() == 0.0) continue;
                QList<PerceivedBoundary> PPBs = groupFromSyll(syll, prefix);
                foreach (PerceivedBoundary PPB, PPBs) {
                    QString hash = QString("%1_%2_%3").arg(com->ID()).arg(PPB.subject).arg(PPB.timeOrig.toDouble());
                    if (AllPPBs.contains(hash))
                        AllPPBs[hash] = AllPPBs[hash] + 1;
                    else
                        AllPPBs.insert(hash, 1);
                }
            }
        }
        qDeleteAll(tiers);
        qDebug() << com->ID();
    }
    qDebug() << AllPPBs.keys().count();
}

// ====================================================================================================================
// 4. Create feature files for statistical analyses
// ====================================================================================================================

void ProsodicBoundariesExperimentAnalysis::statExtractFeaturesForModelling(const QString &filename,
                                                                           Corpus *corpus, QString prefix, bool multilevel)
{
    if (!corpus) return;
    QStringList ppbAttributeIDs;
    ppbAttributeIDs << prefix + "Delay" << prefix + "Dispersion" << prefix + "Force" <<
                       prefix + "FirstPPB" << prefix + "LastPPB" << "promise_pos";
    // prefix + "Subjects" << prefix + "TimesAdj" << prefix + "TimesOrig"

    QFile file(filename);
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ) return;
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << "stimulusID\tstimulusType\tspeaker\tsyll_ID\tsyll_tmin\tsyll\t"
           "offlineBoundaryType\tofflineContour\tofflineBoundary\t"
           "durNextPause\tlogdurNextPause\tlogdurNextPauseZ\t"
           "durSyllRel20\tdurSyllRel30\tdurSyllRel40\tdurSyllRel50\t"
           "logdurSyllRel20\tlogdurSyllRel30\tlogdurSyllRel40\tlogdurSyllRel50\t"
           "f0meanSyllRel20\tf0meanSyllRel30\tf0meanSyllRel40\tf0meanSyllRel50\t"
           "intrasyllab_up\tintrasyllab_down\ttrajectory\t"
           "tok_mwu\tsequence\trection\tsyntacticBoundaryType\tpos_mwu\tpos_mwu_cat\tpos_clilex\tcontext";
    foreach (QString attribute, ppbAttributeIDs) {
        out << "\t" << attribute;
    }
    if (multilevel) {
        out << "\tsubjectID\tresponseTimeAdj\tresponseTimeOrig\n";
    } else {
        out << "\n";
    }
    foreach (CorpusCommunication *com, corpus->communications()) {
        if (com->property("exclude").toBool()) continue;

        QMap<QString, QPointer<AnnotationTierGroup> > tiers = corpus->repository()->annotations()->getTiersAllSpeakers(com->ID());
        foreach (QString speakerID, tiers.keys()) {
            QPointer<AnnotationTierGroup> tiersSpk = tiers.value(speakerID);
            IntervalTier *tier_syll = tiersSpk->getIntervalTierByName("syll");
            if (!tier_syll) continue;
            // Select syllables that we want to analyse: all syllables with a boundary force greater than 0.01
            QList<int> ppbSyllables;
            for (int i = 0; i < tier_syll->count(); ++i) {
                Interval *syll = tier_syll->interval(i);
                if (syll->attribute(prefix + "Force").toDouble() <= 0.01) continue;
                ppbSyllables << i;
            }
            // Analyse the selected syllables
            if (ppbSyllables.isEmpty()) continue;
            ProsodicBoundaries PBAnalyser;
            PBAnalyser.setAdditionalAttributeIDs(ppbAttributeIDs);
            QStringList results = PBAnalyser.analyseBoundaryListToStrings(corpus, com->ID(), speakerID, ppbSyllables);
            // If the output file will be used for multi-level modelling, we need one line per tapping (i.e. per subject), otherwise
            // we need one line per syllable.
            if (!multilevel) {
                // One line per syllable
                foreach (QString line, results) out << line << "\n";
            } else {
                foreach (QString line, results) {
                    // One line per subject
                    QStringList fields = line.split("\t");
                    QStringList subjectsPPB = fields.at(fields.count() - 3).split("|");
                    QStringList responseTimesAdj = fields.at(fields.count() - 2).split("|");
                    QStringList responseTimesOrig = fields.at(fields.count() - 1).split("|");
                    for (int i = 0; i < subjectsPPB.count(); ++i) {
                        out << fields.mid(fields.count() - 3).join("\t") << "\t";
                        out << subjectsPPB.at(i) << "\t";
                        out << responseTimesAdj.at(i) << "\t";
                        out << responseTimesOrig.at(i) << "\n";
                    }
                }
            }
        }
        qDeleteAll(tiers);
        qDebug() << com->ID();
    }

    file.close();
}

void ProsodicBoundariesExperimentAnalysis::statExtractFeaturesForModellingPerSubject(const QString &filename,
                                                                                     Corpus *corpus, QString prefix,
                                                                                     QStringList subjectIDs)
{
    if (!corpus) return;
    QStringList ppbAttributeIDs;
    ppbAttributeIDs << prefix + "Delay" << prefix + "Dispersion" << prefix + "Force" <<
                       prefix + "FirstPPB" << prefix + "LastPPB" << "promise_pos" <<
                       prefix + "Subjects";

    QFile file(filename);
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ) return;
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << "stimulusID\tstimulusType\tspeaker\tsyll_ID\tsyll_tmin\tsyll\t"
           "offlineBoundaryType\tofflineContour\tofflineBoundary\t"
           "durNextPause\tlogdurNextPause\tlogdurNextPauseZ\t"
           "durSyllRel20\tdurSyllRel30\tdurSyllRel40\tdurSyllRel50\t"
           "logdurSyllRel20\tlogdurSyllRel30\tlogdurSyllRel40\tlogdurSyllRel50\t"
           "f0meanSyllRel20\tf0meanSyllRel30\tf0meanSyllRel40\tf0meanSyllRel50\t"
           "intrasyllab_up\tintrasyllab_down\ttrajectory\t"
           "tok_mwu\tsequence\trection\tsyntacticBoundaryType\tpos_mwu\tpos_mwu_cat\tpos_clilex\tcontext";
    foreach (QString attribute, ppbAttributeIDs) {
        out << "\t" << attribute;
    }
    foreach (QString subjectID, subjectIDs) {
        out << "\t" << subjectID;
    }
    out << "\n";

    foreach (CorpusCommunication *com, corpus->communications()) {
        if (com->property("exclude").toBool()) continue;

        QMap<QString, QPointer<AnnotationTierGroup> > tiers = corpus->repository()->annotations()->getTiersAllSpeakers(com->ID());
        foreach (QString speakerID, tiers.keys()) {
            QPointer<AnnotationTierGroup> tiersSpk = tiers.value(speakerID);
            IntervalTier *tier_syll = tiersSpk->getIntervalTierByName("syll");
            if (!tier_syll) continue;
            // Select syllables that we want to analyse: all syllables that are a potential boundary site
            QList<int> ppbSyllables;
            for (int i = 0; i < tier_syll->count(); ++i) {
                Interval *syll = tier_syll->interval(i);
                if (!syll->attribute(prefix + "PotentialSite").toBool()) continue;
                ppbSyllables << i;
            }
            // Analyse the selected syllables
            if (ppbSyllables.isEmpty()) continue;
            ProsodicBoundaries PBAnalyser;
            PBAnalyser.setAdditionalAttributeIDs(ppbAttributeIDs);
            QStringList results = PBAnalyser.analyseBoundaryListToStrings(corpus, com->ID(), speakerID, ppbSyllables);
            // One line per syllable, with separate columns indicating whether each subject annotated this boundary
            foreach (QString line, results) {
                QStringList subjectsPPB = line.section("\t", - 1).split("|");
                out << line;
                foreach (QString subjectID, subjectIDs) {
                    if (subjectsPPB.contains(subjectID))
                        out << "\tB";
                    else
                        out << "\to";
                }
                out << "\n";
            }
        }
        qDeleteAll(tiers);
        qDebug() << com->ID();
    }

    file.close();
}


double getCohenKappa(const QList<bool> &annotations1, const QList<bool> &annotations2)
{
    int n00 = 0, n01 = 0, n10 = 0, n11 = 0;
    if (annotations1.count() != annotations2.count()) return -1.0;
    for (int i = 0; i < annotations1.count(); ++i) {
        bool label1 = annotations1.at(i);
        bool label2 = annotations2.at(i);
        //                   annotator1
        //                   true      false
        // annotator2 true    n00       n01
        //            false   n10       n11
        if      ((label1 == true)  && (label2 == true))  n00++;
        else if ((label1 == false) && (label2 == true))  n01++;
        else if ((label1 == true)  && (label2 == false)) n10++;
        else if ((label1 == false) && (label2 == false)) n11++;
    }
    double Pa = (double(n00 + n11)) / (double(n00+n01+n10+n11));
    double Pe = (double((n00+n10)*(n00+n01)+(n10+n11)*(n01+n11))) / (double(n00+n01+n10+n11)) / (double(n00+n01+n10+n11));
    double k = (Pa - Pe) / (1 - Pe);
    return k;
}

void ProsodicBoundariesExperimentAnalysis::statInterAnnotatorAgreement(const QString &filenameCohen, const QString &filenameFleiss, Corpus *corpus,
                                                                       const QString &prefix, const QString &tapping_level)
{
    QFile fileCohen(filenameCohen);
    QFile fileFleiss(filenameFleiss);
    if ( !fileCohen.open( QIODevice::WriteOnly | QIODevice::Text ) ) return;
    if ( !fileFleiss.open( QIODevice::WriteOnly | QIODevice::Text ) ) return;
    QTextStream outCohen(&fileCohen);
    QTextStream outFleiss(&fileFleiss);
    outCohen.setCodec("UTF-8");
    outFleiss.setCodec("UTF-8");
    outCohen << "stimulusID\t" << "stimulusType\t" << "annotator1\t" << "annotator2\t" << "kappa\n";
    outFleiss << "stimulusID\t" << "stimulusType\t" << "kappa\n";

    foreach (CorpusCommunication *com, corpus->communications()) {
        if (com->property("exclude").toBool()) continue;

        // Get list of subjects who tapped during this sample
        QList<QString> annotatorsForSample = corpus->repository()->annotations()->getSpeakersActiveInLevel(com->ID(), tapping_level);
        qDebug() << com->ID() << " " << annotatorsForSample.count();
        // To calculate Cohen's kappa we need all the annotations by Subject ID -> (Syllable ID, Perceived as boundary?)
        QHash<QString, QList<bool> > boundaryAnnotations;
        foreach (QString subjectID, annotatorsForSample) {
            boundaryAnnotations.insert(subjectID, QList<bool>());
        }
        // To calculcate Fleiss' kappa we need the number of subjects who annotated each syllable as being
        // a boundary and the number of subjects who annotated the same syllable as not a boudnary.
        QList<QPair<int, int> > boundaryAnnotationsBySyllable;
        // Cohen kappa scores for each pair of annotators in this sample
        QHash<QPair<QString, QString>, double> kappaScoresForSample;

        // Process each syllable and categorise only those that are potential prosodic boundary sites.
        QMap<QString, QPointer<AnnotationTierGroup> > tiers = corpus->repository()->annotations()->getTiersAllSpeakers(com->ID());
        foreach (QString speakerID, tiers.keys()) {
            QPointer<AnnotationTierGroup> tiersSpk = tiers.value(speakerID);
            IntervalTier *tier_syll = tiersSpk->getIntervalTierByName("syll");
            if (!tier_syll) continue;
            foreach (Interval *syll, tier_syll->intervals()) {
                if (!syll->attribute(prefix + "PotentialSite").toBool()) continue;
                QStringList subjectsPPB = syll->attribute(prefix + "Subjects").toString().split("|");
                int numberOfSubjectsPB0 = 0, numberOfSubjectsPB1 = 0;
                foreach (QString subjectID, annotatorsForSample) {
                    if (subjectsPPB.contains(subjectID)) {
                        boundaryAnnotations[subjectID] << true;
                        ++numberOfSubjectsPB1;
                    }
                    else {
                        boundaryAnnotations[subjectID] << false;
                        ++numberOfSubjectsPB0;
                    }
                }
                if (numberOfSubjectsPB0 + numberOfSubjectsPB1 != annotatorsForSample.count()) {
                    qDebug() << "Error " << annotatorsForSample.count() << " " << numberOfSubjectsPB0 << " " << numberOfSubjectsPB1;
                }
                boundaryAnnotationsBySyllable << QPair<int, int>(numberOfSubjectsPB0, numberOfSubjectsPB1);
            }
        }

        // Calculate Cohen's kappa for each pair of annotators
        QPair<QString, QString> annotatorPair;
        foreach (QString annotator1, annotatorsForSample) {
            foreach (QString annotator2, annotatorsForSample) {
                if (annotator1 == annotator2)
                    continue;
                if (kappaScoresForSample.contains(QPair<QString, QString>(annotator2, annotator1)))
                    continue;
                annotatorPair = QPair<QString, QString>(annotator1, annotator2);
                double k = getCohenKappa(boundaryAnnotations[annotator1], boundaryAnnotations[annotator2]);
                if (k > 1.0) qDebug() << com->ID() << " " << annotator1 << " " << annotator2 << " " << k;
                kappaScoresForSample.insert(annotatorPair, k);
            }
        }

        // Calculcate Fleiss' kappa for the sample
        int nCases = boundaryAnnotationsBySyllable.count();
        int nAnnotators = annotatorsForSample.count();
        double totalP = 0.0, meanP = 0.0, meanPe = 0.0;
        int totalPB0 = 0, totalPB1 = 0;
        double marginalPB0 = 0.0, marginalPB1 = 0.0;

        QPair<int, int> x;
        foreach (x, boundaryAnnotationsBySyllable) {
            totalPB0 += x.first;
            totalPB1 += x.second;
            if (x.first + x.second != annotatorsForSample.count()) {
                qDebug() << "Error";
            }
            double P = ((double)(x.first * x.first + x.second * x.second - nAnnotators)) /
                       ((double)(nAnnotators * (nAnnotators - 1)));
            totalP = totalP + P;
            // DEBUG: outFleiss << com->ID() << "\t" << x.first << "\t" << x.second << "\t" << P << "\n";
        }
        marginalPB0 = ((double)totalPB0) / ((double)(nCases * nAnnotators));
        marginalPB1 = ((double)totalPB1) / ((double)(nCases * nAnnotators));

        meanP = totalP / ((double)nCases);
        meanPe = marginalPB0 * marginalPB0 + marginalPB1 * marginalPB1;
        double fleissKappa = (meanP - meanPe) / (1.0 - meanPe);
        // DEBUG: outFleiss << com->ID() << "\t" << marginalPB0 << "\t" << marginalPB1 << "\t" << fleissKappa << "\n\n";

        foreach (annotatorPair, kappaScoresForSample.keys()) {
            outCohen << com->ID() << "\t" << com->ID().right(1) << "\t";
            outCohen << annotatorPair.first << "\t";
            outCohen << annotatorPair.second << "\t";
            outCohen << kappaScoresForSample.value(annotatorPair) << "\n";
        }
        outFleiss << com->ID() << "\t" << com->ID().right(1) << "\t";
        outFleiss << fleissKappa << "\n";

        // Clean up memory
        qDeleteAll(tiers);
    }

    fileCohen.close();
    fileFleiss.close();
}

void ProsodicBoundariesExperimentAnalysis::statCorrespondanceNSandMS(const QString &filenameTable, const QString &filenameBookmarks,
                                                                     Corpus *corpus, QString prefix)
{
    if (!corpus) return;
    QStringList ppbAttributeIDs;
    ppbAttributeIDs << "promise_pos" <<
                       prefix + "Delay" << prefix + "Dispersion" << prefix + "Force" <<
                       prefix + "FirstPPB" << prefix + "LastPPB";
    // prefix + "Subjects" << prefix + "TimesAdj" << prefix + "TimesOrig"

    QFile file(filenameTable);
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ) return;
    QTextStream out(&file);
    out.setCodec("UTF-8");

    // Print header row
    QStringList fieldLabels;
    fieldLabels << "stimulusID" << "stimulusType" << "speaker" << "syll_ID" << "syll_tmin" << "syll";
    fieldLabels << "offlineBoundaryType" << "offlineContour" << "offlineBoundary";
    fieldLabels << "durNextPause" << "logdurNextPause" << "logdurNextPauseZ";
    fieldLabels << "durSyllRel20" << "durSyllRel30" << "durSyllRel40" << "durSyllRel50";
    fieldLabels << "logdurSyllRel20" << "logdurSyllRel30" << "logdurSyllRel40" << "logdurSyllRel50";
    fieldLabels << "f0meanSyllRel20" << "f0meanSyllRel30" << "f0meanSyllRel40" << "f0meanSyllRel50";
    fieldLabels << "intrasyllab_up" << "intrasyllab_down" << "trajectory";
    fieldLabels << "tok_mwu" << "sequence" << "rection" << "syntacticBoundaryType";
    fieldLabels << "pos_mwu" << "pos_mwu_cat" << "pos_clilex" << "context";
    fieldLabels << ppbAttributeIDs;

    int indexTimecode(0), indexForceLeft(0), indexForceRight(0);
    QString headerRow;
    for (int i = 0; i < fieldLabels.count(); ++i) {
        QString fieldLabel = fieldLabels.at(i);
        headerRow = headerRow.append(QString("ns_%1\tms_%1\t").arg(fieldLabel));
        if      (fieldLabel == "syll_tmin") indexTimecode = i;
        else if (fieldLabel == prefix + "Force") {
            indexForceLeft = i; indexForceRight = i;
        }
    }
    headerRow.chop(1);
    out << headerRow << "\n";

    QList<QPointer<CorpusBookmark> > bookmarks;

    foreach (CorpusCommunication *com, corpus->communications()) {
        QString id = com->ID();
        if (com->property("exclude").toBool()) continue;
        if (id.endsWith("S")) continue;

        QString idNS = id; QString idMS = id.remove("N").append("S");
        QList<QString> featuresNS, featuresMS;

        QMap<QString, QPointer<AnnotationTierGroup> > tiers = corpus->repository()->annotations()->getTiersAllSpeakers(com->ID());
        foreach (QString speakerID, tiers.keys()) {
            QPointer<AnnotationTierGroup> tiersSpk = tiers.value(speakerID);
            IntervalTier *tier_syll = tiersSpk->getIntervalTierByName("syll");
            if (!tier_syll) continue;
            QList<int> ppbSyllables;
            for (int i = 0; i < tier_syll->count(); ++i) {
                Interval *syll = tier_syll->interval(i);
                if (!syll->attribute(prefix + "PotentialSite").toBool()) continue;
                ppbSyllables << i;
            }
            ProsodicBoundaries PBAnalyser;
            PBAnalyser.setAdditionalAttributeIDs(ppbAttributeIDs);
            featuresNS = PBAnalyser.analyseBoundaryListToStrings(corpus, idNS, speakerID, ppbSyllables);
            featuresMS = PBAnalyser.analyseBoundaryListToStrings(corpus, idMS, speakerID, ppbSyllables);
            for (int i = 0; i < featuresNS.count(); ++i) {
                QStringList fieldsNS = featuresNS.at(i).split("\t");
                QStringList fieldsMS = featuresMS.at(i).split("\t");
                QString line;
                for (int j = 0; j < fieldsNS.count(); ++j) {
                    line = line.append(QString("%1\t%2\t").arg(fieldsNS.at(j)).arg(fieldsMS.at(j)));
                }
                line.chop(1);
                out << line << "\n";
                // if intersting case, add it to bookmarks
                double forceNS = QString(fieldsNS.at(indexForceLeft)).replace(",", ".").toDouble();
                double forceMS = QString(fieldsMS.at(indexForceRight)).replace(",", ".").toDouble();
                RealTime t = RealTime::fromSeconds(QString(fieldsNS.at(indexTimecode)).replace(",", ".").toDouble());
                if ((forceNS - forceMS > 0.40) || (forceMS - forceNS > 0.40)) {
                    QString name = QString("NS: %1 MS: %2")
                            .arg(QString::number(forceNS * 100.0, 'f', 0))
                            .arg(QString::number(forceMS * 100.0, 'f', 0));
                    bookmarks << new CorpusBookmark(corpus->ID(), idNS, idNS, t, name);
                    bookmarks << new CorpusBookmark(corpus->ID(), idMS, idMS, t, name);
                }
            }
        }
        qDeleteAll(tiers);
        qDebug() << idNS << "<>" << idMS;
    }
    file.close();

    if (!filenameBookmarks.isEmpty())
        XMLSerialiserCorpusBookmark::saveCorpusBookmarks(bookmarks, filenameBookmarks);
    qDeleteAll(bookmarks);
}

void ProsodicBoundariesExperimentAnalysis::statCorrespondanceInternal(const QString &filenameTable, const QString &filenameBookmarks,
                                                                      Corpus *corpus,
                                                                      const QString &prefixLeft, const QString &prefixRight,
                                                                      double limitLeft, double limitRight)
{
    if (!corpus) return;
    QStringList ppbAttributeIDs;
    ppbAttributeIDs << "promise_pos" <<
                       prefixLeft + "Delay" << prefixRight + "Delay" <<
                       prefixLeft + "Dispersion" << prefixRight + "Dispersion" <<
                       prefixLeft + "Force" << prefixRight + "Force" <<
                       prefixLeft + "FirstPPB" << prefixRight + "FirstPPB" <<
                       prefixLeft + "LastPPB" << prefixRight + "LastPPB";
    // prefix + "Subjects" << prefix + "TimesAdj" << prefix + "TimesOrig"

    QFile file(filenameTable);
    if ( !file.open( QIODevice::WriteOnly | QIODevice::Text ) ) return;
    QTextStream out(&file);
    out.setCodec("UTF-8");

    // Print header row
    QStringList fieldLabels;
    fieldLabels << "stimulusID" << "stimulusType" << "speaker" << "syll_ID" << "syll_tmin" << "syll";
    fieldLabels << "offlineBoundaryType" << "offlineContour" << "offlineBoundary";
    fieldLabels << "durNextPause" << "logdurNextPause" << "logdurNextPauseZ";
    fieldLabels << "durSyllRel20" << "durSyllRel30" << "durSyllRel40" << "durSyllRel50";
    fieldLabels << "logdurSyllRel20" << "logdurSyllRel30" << "logdurSyllRel40" << "logdurSyllRel50";
    fieldLabels << "f0meanSyllRel20" << "f0meanSyllRel30" << "f0meanSyllRel40" << "f0meanSyllRel50";
    fieldLabels << "intrasyllab_up" << "intrasyllab_down" << "trajectory";
    fieldLabels << "tok_mwu" << "sequence" << "rection" << "syntacticBoundaryType";
    fieldLabels << "pos_mwu" << "pos_mwu_cat" << "pos_clilex" << "context";
    fieldLabels << ppbAttributeIDs;

    int indexTimecode(0), indexForceLeft(0), indexForceRight(0);
    QString headerRow;
    for (int i = 0; i < fieldLabels.count(); ++i) {
        QString fieldLabel = fieldLabels.at(i);
        headerRow = headerRow.append(QString("%1\t").arg(fieldLabel));
        if      (fieldLabel == "syll_tmin") indexTimecode = i;
        else if (fieldLabel == prefixLeft + "Force") indexForceLeft = i;
        else if (fieldLabel == prefixRight + "Force") indexForceRight = i;
    }
    headerRow.chop(1);
    out << headerRow << "\n";

    QList<QPointer<CorpusBookmark> > bookmarks;

    foreach (CorpusCommunication *com, corpus->communications()) {
        QString id = com->ID();
        if (com->property("exclude").toBool()) continue;

        QList<QString> features;

        QMap<QString, QPointer<AnnotationTierGroup> > tiers = corpus->repository()->annotations()->getTiersAllSpeakers(com->ID());
        foreach (QString speakerID, tiers.keys()) {
            QPointer<AnnotationTierGroup> tiersSpk = tiers.value(speakerID);
            IntervalTier *tier_syll = tiersSpk->getIntervalTierByName("syll");
            if (!tier_syll) continue;
            QList<int> ppbSyllables;
            for (int i = 0; i < tier_syll->count(); ++i) {
                Interval *syll = tier_syll->interval(i);
                if (!syll->attribute(prefixLeft + "PotentialSite").toBool() && !syll->attribute(prefixRight + "PotentialSite").toBool())
                    continue;
                ppbSyllables << i;
            }
            ProsodicBoundaries PBAnalyser;
            PBAnalyser.setAdditionalAttributeIDs(ppbAttributeIDs);
            features = PBAnalyser.analyseBoundaryListToStrings(corpus, id, speakerID, ppbSyllables);
            for (int i = 0; i < features.count(); ++i) {
                QStringList fields = features.at(i).split("\t");
                QString line;
                for (int j = 0; j < fields.count(); ++j) {
                    line = line.append(QString("%1\t").arg(fields.at(j)));
                }
                line.chop(1);
                out << line << "\n";
                // if intersting case, add it to bookmarks
                double forceLeft = QString(fields.at(indexForceLeft)).replace(",", ".").toDouble();
                double forceRight = QString(fields.at(indexForceRight)).replace(",", ".").toDouble();
                RealTime t = RealTime::fromSeconds(QString(fields.at(indexTimecode)).replace(",", ".").toDouble());
                if ((forceLeft - forceRight > limitLeft) || (forceRight - forceLeft > limitRight)) {
                    QString name = QString("%1: %2 %3: %4")
                            .arg(prefixLeft)
                            .arg(QString::number(forceLeft * 100.0, 'f', 0))
                            .arg(prefixRight)
                            .arg(QString::number(forceRight * 100.0, 'f', 0));
                    bookmarks << new CorpusBookmark(corpus->ID(), id, id, t, name);
                }
            }
        }
        qDeleteAll(tiers);
        qDebug() << id;
    }
    file.close();

    if (!filenameBookmarks.isEmpty())
        XMLSerialiserCorpusBookmark::saveCorpusBookmarks(bookmarks, filenameBookmarks);
    qDeleteAll(bookmarks);
}


void ProsodicBoundariesExperimentAnalysis::analysisCheckBoundaryRightAfterPause(Corpus *corpus)
{
    QString path = "/home/george/Dropbox/2015-10 SP8 - Prosodic boundaries perception experiment/analyses/";
    QList<QPointer<CorpusBookmark> > bookmarks;
    foreach (CorpusCommunication *com, corpus->communications()) {
        QString id = com->ID();
        if (!id.startsWith("A") && !id.startsWith("B")) continue;

        QMap<QString, QPointer<AnnotationTierGroup> > tiers = corpus->repository()->annotations()->getTiersAllSpeakers(com->ID());
        foreach (QString speakerID, tiers.keys()) {
            QPointer<AnnotationTierGroup> tiersSpk = tiers.value(speakerID);
            IntervalTier *tier_syll = tiersSpk->getIntervalTierByName("syll");
            if (!tier_syll) continue;

            for (int i = 0; i < tier_syll->count() - 1; ++i) {
                Interval *syll = tier_syll->interval(i);
                Interval *syll_next = tier_syll->interval(i+1);
                if (syll->isPauseSilent() && syll_next->attribute("boundaryForce").toDouble() > 0.0) {
                    bookmarks << new CorpusBookmark(corpus->ID(), com->ID(), com->ID(), syll->tMin(), "");
                }
//                if (syll->attribute("boundary").toString().contains("#")) {
//                    bookmarks << new CorpusBookmark(corpus->ID(), com->ID(), com->ID(), syll->tMin(), "");
//                }
            }
        }
        qDeleteAll(tiers);
        qDebug() << com->ID();
    }
    XMLSerialiserCorpusBookmark::saveCorpusBookmarks(bookmarks, path + "bookmarks_pauses2.xml");
    qDeleteAll(bookmarks);
}

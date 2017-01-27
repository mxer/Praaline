#include "AnnotationGridLayer.h"

#include "base/RealTime.h"
#include "svcore/data/model/Model.h"
#include "svcore/base/Profiler.h"
#include "svgui/layer/ColourDatabase.h"
#include "svgui/view/View.h"

#include "pngui/model/annotation/AnnotationGridModel.h"

#include <QPainter>
#include <QMouseEvent>
#include <QInputDialog>
#include <QTextStream>
#include <QMessageBox>

#include <iostream>
#include <cmath>

AnnotationGridLayer::AnnotationGridLayer() :
    SingleColourLayer(),
    m_model(0),
    m_editing(false),
    m_plotStyle(PlotSpeakersThenLevelAttributes)
{
}

bool AnnotationGridLayer::trySetModel(Model *model)
{
    if (trySetModelHelper<AnnotationGridLayer, AnnotationGridModel>(this, model))
        return true;
    return false;
}

void
AnnotationGridLayer::setModel(AnnotationGridModel *model)
{
    if (m_model == model) return;
    m_model = model;
    connectSignals(m_model);
    if (m_plotStyle == PlotBlendedSpeakers || m_plotStyle == PlotLevelAttributesThenSpeakers)
        m_tierTuples = m_model->tierTuples(AnnotationGridModel::LayoutLevelAttributesThenSpeakers);
    else
        m_tierTuples = m_model->tierTuples(AnnotationGridModel::LayoutSpeakersThenLevelAttributes);
    // cerr << "AnnotationGridLayer::setModel(" << model << ")" << endl;
    emit modelReplaced();
}

// ==============================================================================================================================
// Properties
// ==============================================================================================================================

Layer::PropertyList
AnnotationGridLayer::getProperties() const
{
    PropertyList list = SingleColourLayer::getProperties();
    list.push_back("Structure");
    return list;
}

QString
AnnotationGridLayer::getPropertyLabel(const PropertyName &name) const
{
    if (name == "Structure") return tr("Structure");
    return SingleColourLayer::getPropertyLabel(name);
}

Layer::PropertyType
AnnotationGridLayer::getPropertyType(const PropertyName &name) const
{
    if (name == "Structure") return ValueProperty;
    return SingleColourLayer::getPropertyType(name);
}

int
AnnotationGridLayer::getPropertyRangeAndValue(const PropertyName &name,
                                              int *min, int *max, int *deflt) const
{
    int val = 0;
    if (name == "Structure") {
        if (min) *min = 0;
        if (max) *max = 2;
        if (deflt) *deflt = int(PlotBlendedSpeakers);
        val = int(m_plotStyle);
    } else {
        val = SingleColourLayer::getPropertyRangeAndValue(name, min, max, deflt);
    }
    return val;
}

QString
AnnotationGridLayer::getPropertyValueLabel(const PropertyName &name,
                                           int value) const
{
    if (name == "Structure") {
        switch (value) {
        default:
        case 0: return tr("Blend speakers together");
        case 1: return tr("Speaker, then Level/Attribute");
        case 2: return tr("Level/Attribute, then Speaker");
        }
    }
    return SingleColourLayer::getPropertyValueLabel(name, value);
}

void
AnnotationGridLayer::setProperty(const PropertyName &name, int value)
{
    if (name == "Structure") {
        setPlotStyle(PlotStyle(value));
    } else {
        SingleColourLayer::setProperty(name, value);
    }
}

bool
AnnotationGridLayer::getValueExtents(double &, double &, bool &, QString &) const
{
    return false;
}

// ==============================================================================================================================

bool
AnnotationGridLayer::isLayerScrollable(const View *v) const
{
    QPoint discard;
    return !v->shouldIlluminateLocalFeatures(this, discard);
}

void AnnotationGridLayer::setPlotStyle(PlotStyle style)
{
    if (m_plotStyle == style) return;
    m_plotStyle = style;
    if (m_plotStyle == PlotLevelAttributesThenSpeakers)
        m_tierTuples = m_model->tierTuples(AnnotationGridModel::LayoutLevelAttributesThenSpeakers);
    else // including PlotBlendedSpeakers
        m_tierTuples = m_model->tierTuples(AnnotationGridModel::LayoutSpeakersThenLevelAttributes);
    emit layerParametersChanged();
}

int AnnotationGridLayer::getYForTierIndex(View *v, int tierIndex) const
{
    int h = v->height();
    if ((m_plotStyle == PlotBlendedSpeakers) && m_model) {
        int i = m_tierTuples.at(tierIndex).indexLevelAttributePair;
        int c = m_model->countLevelsAttributes();
        int y = i * h / c;
        qDebug() << "Tier index " << tierIndex << " height " << y;
        return y;
    }
    return int(tierIndex * h / m_tierTuples.count());
}

int AnnotationGridLayer::getTierIndexForY(View *v, int y) const
{
    int h = v->height();
    int tierIndex(0);
    if ((m_plotStyle == PlotBlendedSpeakers) && m_model)
        tierIndex = int(y * m_model->countLevelsAttributes() / h);
    else
        tierIndex = (y * m_tierTuples.count() / h);
    if (tierIndex < 0) tierIndex = 0;
    if (tierIndex >= m_tierTuples.count()) tierIndex = m_tierTuples.count() - 1;
    return tierIndex;
}

AnnotationGridPointModel::PointList AnnotationGridLayer::getLocalPoints(View *v, int x, int y) const
{
    if (!m_model) return AnnotationGridPointModel::PointList();
    // Info for the tier at the y-coordinate
    int tierIndex = getTierIndexForY(v, y);
    if (tierIndex < 0 || tierIndex >= m_tierTuples.count()) return AnnotationGridPointModel::PointList();
    QString levelID = m_tierTuples.at(tierIndex).levelID;
    // Frame from the x-coordinate
    sv_frame_t frame = v->getFrameForX(x);
    // Points for this tier
    QPointer<AnnotationGridPointModel> boundaryModel = m_model->boundariesForLevel(levelID);
    if (!boundaryModel) return AnnotationGridPointModel::PointList();

    AnnotationGridPointModel::PointList onPoints = boundaryModel->getPoints(frame);
    if (!onPoints.empty()) return onPoints;

    AnnotationGridPointModel::PointList prevPoints = boundaryModel->getPreviousPoints(frame);
    return prevPoints;
}

//bool
//AnnotationGridLayer::getPointToDrag(View *v, int x, int y, AnnotationGridModel::Point &p) const
//{
//    if (!m_model) return false;

//    sv_frame_t a = v->getFrameForX(x - 120);
//    sv_frame_t b = v->getFrameForX(x + 10);
//    AnnotationGridModel::PointList onPoints = m_model->getPoints(a, b);
//    if (onPoints.empty()) return false;

//    double nearestDistance = -1;

//    for (AnnotationGridModel::PointList::const_iterator i = onPoints.begin();
//         i != onPoints.end(); ++i) {

//        double yd = getYForHeight(v, (*i).height) - y;
//        double xd = v->getXForFrame((*i).frame) - x;
//        double distance = sqrt(yd*yd + xd*xd);

//        if (nearestDistance == -1 || distance < nearestDistance) {
//            nearestDistance = distance;
//            p = *i;
//        }
//    }

//    return true;
//}

QString AnnotationGridLayer::getFeatureDescription(View *v, QPoint &pos) const
{    
    if (!m_model || !m_model->getSampleRate()) return "";

    int x = pos.x();
    AnnotationGridPointModel::PointList points = getLocalPoints(v, x, pos.y());

    if (points.empty()) {
        if (!m_model->isReady()) {
            return tr("In progress");
        } else {
            return "";
        }
    }

    sv_frame_t frame = points.begin()->frame;
    sv_frame_t duration = points.begin()->duration;
    RealTime rt = RealTime::frame2RealTime(frame, m_model->getSampleRate());
    RealTime rd = RealTime::frame2RealTime(duration, m_model->getSampleRate());

    QString text = QString(tr("Time:\t%1\nDuration:\t%2"))
            .arg(rt.toText(true).c_str()).arg(rd.toText(true).c_str());

    pos = QPoint(v->getXForFrame(frame), pos.y());
    return text;
}


bool AnnotationGridLayer::snapToFeatureFrame(View *v, sv_frame_t &frame, int &resolution, SnapType snap, int y) const
{
    if (!m_model) return Layer::snapToFeatureFrame(v, frame, resolution, snap, y);

    int tierIndex = getTierIndexForY(v, y);
    if (tierIndex < 0 || tierIndex >= m_tierTuples.count())
        return Layer::snapToFeatureFrame(v, frame, resolution, snap, y);

    QString levelID = m_tierTuples.at(tierIndex).levelID;
    QPointer<AnnotationGridPointModel> boundaryModel = m_model->boundariesForLevel(levelID);
    if (!boundaryModel) return Layer::snapToFeatureFrame(v, frame, resolution, snap, y);

    resolution = boundaryModel->getResolution();

    AnnotationGridPointModel::PointList points;

    if (snap == SnapNeighbouring) {
        points = getLocalPoints(v, v->getXForFrame(frame), y);
        if (points.empty()) return false;
        frame = points.begin()->frame;
        return true;
    }

    points = boundaryModel->getPoints(frame, frame);
    sv_frame_t snapped = frame;
    bool found = false;

    for (AnnotationGridPointModel::PointList::const_iterator i = points.begin(); i != points.end(); ++i) {
        if (snap == SnapRight) {
            // The best frame to snap to is the end frame of whichever feature we would have snapped to the start frame of if
            // we had been snapping left.
            if (i->frame <= frame) {
                if (i->frame + i->duration > frame) {
                    snapped = i->frame + i->duration;
                    found = true; // don't break, as the next may be better
                }
            } else {
                if (!found) {
                    snapped = i->frame;
                    found = true;
                }
                break;
            }
        } else if (snap == SnapLeft) {
            if (i->frame <= frame) {
                snapped = i->frame;
                found = true; // don't break, as the next may be better
            } else {
                break;
            }
        } else { // nearest
            AnnotationGridPointModel::PointList::const_iterator j = i;
            ++j;
            if (j == points.end()) {
                snapped = i->frame;
                found = true;
                break;
            } else if (j->frame >= frame) {
                if (j->frame - frame < frame - i->frame) {
                    snapped = j->frame;
                } else {
                    snapped = i->frame;
                }
                found = true;
                break;
            }
        }
    }

    frame = snapped;
    return found;
}

void AnnotationGridLayer::paint(View *v, QPainter &paint, QRect rect) const
{
    if (!m_model || !m_model->isOK()) return;

    sv_samplerate_t sampleRate = m_model->getSampleRate();
    if (!sampleRate) return;

    int x0 = rect.left(), x1 = rect.right();
    sv_frame_t frame0 = v->getFrameForX(x0);
    sv_frame_t frame1 = v->getFrameForX(x1);

    int tierCount = m_tierTuples.count();

    // Draw horizontal tier lines
    // ----------------------------------------------------------------------------------------------------------------
    QColor penColour;
    penColour = v->getForeground();
    paint.setPen(penColour);
    paint.setRenderHint(QPainter::Antialiasing, false);
    for (int tierIndex = 1; tierIndex < tierCount; ++ tierIndex) {
        int y = getYForTierIndex(v, tierIndex);
        paint.drawLine(rect.left(), y, rect.right(), y);
    }

    QPoint localPos;
    AnnotationGridPointModel::Point pointToIlluminate(0);
    if (v->shouldIlluminateLocalFeatures(this, localPos)) {
        AnnotationGridPointModel::PointList localPoints = getLocalPoints(v, localPos.x(), localPos.y());
        if (!localPoints.empty()) pointToIlluminate = *(localPoints.begin());
    }

    // Draw boundaries and data
    // ----------------------------------------------------------------------------------------------------------------
    for (int tierIndex = 0; tierIndex < tierCount; ++ tierIndex) {
        // Top and bottom for boundary lines
        int y0 = getYForTierIndex(v, tierIndex) + 1;
        int y1; // bottom boundary, calculate as follows
        if (m_plotStyle == PlotBlendedSpeakers) {
            // attention, in the Blended Speakers plot mode, the tiers wrap around
            y1 = (tierIndex + 1 < m_model->countLevelsAttributes()) ? getYForTierIndex(v, tierIndex + 1) : rect.bottom();
        } else {
            y1 = (tierIndex + 1 < tierCount) ? getYForTierIndex(v, tierIndex + 1) : rect.bottom();
        }

        int boundaryHeight = y1 - y0;

        // Info for this tier
        QString speakerID = m_tierTuples.at(tierIndex).speakerID;
        QString levelID = m_tierTuples.at(tierIndex).levelID;
        QString attributeID = m_tierTuples.at(tierIndex).attributeID;

        // Points for this tier
        QPointer<AnnotationGridPointModel> boundaryModel = m_model->boundariesForLevel(levelID);
        if (!boundaryModel) continue;
        AnnotationGridPointModel::PointList boundaries = boundaryModel->getPoints(frame0, frame1);

        QColor brushColour(getBaseQColor());
        brushColour.setAlpha(100);

        for (AnnotationGridPointModel::PointList::const_iterator i = boundaries.begin(); i != boundaries.end(); ++i) {
            const AnnotationGridPointModel::Point &p(*i);
            if (p.speakerID != speakerID) continue;
            // Find the next point by the same speaker
            AnnotationGridPointModel::PointList::const_iterator j = i;
            while ((j != boundaries.end()) && (j->speakerID == speakerID)) ++j;
            // Calculate bounds
            int x = v->getXForFrame(p.frame);
            int boundaryWidth = v->getXForFrame(p.frame + 1) - x;
            if (boundaryWidth < 2) {
                if (boundaryWidth < 1) {
                    boundaryWidth = 2;
                    if (j != boundaries.end()) {
                        int nx = v->getXForFrame(j->frame);
                        if (nx < x + 3) boundaryWidth = 1;
                    }
                } else {
                    boundaryWidth = 2;
                }
            }
            // Illuminate goes here
            bool shouldIlluminate =  (!AnnotationGridPointModel::Point::Comparator()(pointToIlluminate, p) &&
                                      !AnnotationGridPointModel::Point::Comparator()(p, pointToIlluminate));
//            if (AnnotationGridPointModel::Point::Comparator()(pointToIlluminate, p) || AnnotationGridPointModel::Point::Comparator()(p, pointToIlluminate)) {
                paint.setPen(penColour);
                paint.setBrush(brushColour);
//            } else {
//                paint.setBrush(penColour);
//                paint.setPen(v->getBackground());
//            }
//            if (AnnotationGridPointModel::Point::Comparator(pointToIlluminate, p)) {
//                paint.setPen(getForegroundQColor(v));
//                paint.setBrush(getForegroundQColor(v));
//            } else {
//                paint.setPen(getBaseQColor());
//                paint.setBrush(brushColour);
//            }
            // Draw boundary
            if (boundaryWidth > 1) {
                paint.drawRect(x, y0, boundaryWidth - 1, boundaryHeight);
            } else {
                paint.drawLine(x, y0, x, boundaryHeight);
            }
            // Contents rectangle
            int boxMaxWidth = v->getXForFrame(p.frame + p.duration) - x - 6;
            int boxMaxHeight = y1 - y0 - 4;
            QRect textRect = QRect(x + 3, y0 + 2, boxMaxWidth, boxMaxHeight);
//            if (shouldIlluminate) {
//                paint.setBrush(Qt::yellow);
//                paint.drawRect(textRect);
//            }
            // Label
            QString label = m_model->data(speakerID, levelID, attributeID, p.itemNo).toString();
            if (label.isEmpty()) continue;
            QRect boundingRect = paint.fontMetrics().boundingRect(QRect(0, 0, boxMaxWidth, boxMaxHeight),
                                                                  Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap,
                                                                  label);
            paint.setPen(v->getForeground());
            paint.setRenderHint(QPainter::Antialiasing, true);
            if (boundingRect.width() > textRect.width())
                paint.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, label);
            else
                paint.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, label);
            paint.setRenderHint(QPainter::Antialiasing, false);
            // cerr << tierIndex << " " << speakerID << " " << levelID << " " << attributeID << " " << y0 << " " << y1 << " " << label << endl;
        }
    }

}

void
AnnotationGridLayer::drawStart(View *v, QMouseEvent *e)
{
    //    cerr << "AnnotationGridLayer::drawStart(" << e->x() << "," << e->y() << ")" << endl;

    if (!m_model) {
        // cerr << "AnnotationGridLayer::drawStart: no model" << endl;
        return;
    }

    sv_frame_t frame = v->getFrameForX(e->x());
    if (frame < 0) frame = 0;
//    frame = frame / m_model->getResolution() * m_model->getResolution();

//    double height = getHeightForY(v, e->y());

//    m_editingPoint = AnnotationGridModel::Point(frame, float(height), "");
//    m_originalPoint = m_editingPoint;

//    if (m_editingCommand) finish(m_editingCommand);
//    m_editingCommand = new AnnotationGridModel::EditCommand(m_model, "Add Label");
//    m_editingCommand->addPoint(m_editingPoint);

//    m_editing = true;
}

void
AnnotationGridLayer::drawDrag(View *v, QMouseEvent *e)
{
    //    cerr << "AnnotationGridLayer::drawDrag(" << e->x() << "," << e->y() << ")" << endl;

    if (!m_model || !m_editing) return;

    sv_frame_t frame = v->getFrameForX(e->x());
    if (frame < 0) frame = 0;
//    frame = frame / m_model->getResolution() * m_model->getResolution();

//    double height = getHeightForY(v, e->y());

//    m_editingCommand->deletePoint(m_editingPoint);
//    m_editingPoint.frame = frame;
//    m_editingPoint.height = float(height);
//    m_editingCommand->addPoint(m_editingPoint);
}

void
AnnotationGridLayer::drawEnd(View *v, QMouseEvent *)
{
    //    cerr << "AnnotationGridLayer::drawEnd(" << e->x() << "," << e->y() << ")" << endl;
    if (!m_model || !m_editing) return;

    bool ok = false;
    QString label = QInputDialog::getText(v, tr("Enter label"),
                                          tr("Please enter a new label:"),
                                          QLineEdit::Normal, "", &ok);

//    if (ok) {
//        AnnotationGridModel::RelabelCommand *command =
//                new AnnotationGridModel::RelabelCommand(m_model, m_editingPoint, label);
//        m_editingCommand->addCommand(command);
//    } else {
//        m_editingCommand->deletePoint(m_editingPoint);
//    }

//    finish(m_editingCommand);
//    m_editingCommand = 0;
    m_editing = false;
}

void
AnnotationGridLayer::eraseStart(View *v, QMouseEvent *e)
{
    if (!m_model) return;

//    if (!getPointToDrag(v, e->x(), e->y(), m_editingPoint)) return;

//    if (m_editingCommand) {
//        finish(m_editingCommand);
//        m_editingCommand = 0;
//    }

    m_editing = true;
}

void
AnnotationGridLayer::eraseDrag(View *, QMouseEvent *)
{
}

void
AnnotationGridLayer::eraseEnd(View *v, QMouseEvent *e)
{
    if (!m_model || !m_editing) return;

    m_editing = false;

//    AnnotationGridModel::Point p(0);
//    if (!getPointToDrag(v, e->x(), e->y(), p)) return;
//    if (p.frame != m_editingPoint.frame || p.height != m_editingPoint.height) return;

//    m_editingCommand = new AnnotationGridModel::EditCommand
//            (m_model, tr("Erase Point"));

//    m_editingCommand->deletePoint(m_editingPoint);

//    finish(m_editingCommand);
//    m_editingCommand = 0;
    m_editing = false;
}

void
AnnotationGridLayer::editStart(View *v, QMouseEvent *e)
{
    //    cerr << "AnnotationGridLayer::editStart(" << e->x() << "," << e->y() << ")" << endl;

    if (!m_model) return;

//    if (!getPointToDrag(v, e->x(), e->y(), m_editingPoint)) {
//        return;
//    }

//    m_editOrigin = e->pos();
//    m_originalPoint = m_editingPoint;

//    if (m_editingCommand) {
//        finish(m_editingCommand);
//        m_editingCommand = 0;
//    }

    m_editing = true;
}

void
AnnotationGridLayer::editDrag(View *v, QMouseEvent *e)
{
    if (!m_model || !m_editing) return;

//    sv_frame_t frameDiff = v->getFrameForX(e->x()) - v->getFrameForX(m_editOrigin.x());
//    double heightDiff = getHeightForY(v, e->y()) - getHeightForY(v, m_editOrigin.y());

//    sv_frame_t frame = m_originalPoint.frame + frameDiff;
//    double height = m_originalPoint.height + heightDiff;

//    //    sv_frame_t frame = v->getFrameForX(e->x());
//    if (frame < 0) frame = 0;
//    frame = (frame / m_model->getResolution()) * m_model->getResolution();

//    //    double height = getHeightForY(v, e->y());

//    if (!m_editingCommand) {
//        m_editingCommand = new AnnotationGridModel::EditCommand(m_model, tr("Drag Label"));
//    }

//    m_editingCommand->deletePoint(m_editingPoint);
//    m_editingPoint.frame = frame;
//    m_editingPoint.height = float(height);
//    m_editingCommand->addPoint(m_editingPoint);
}

void
AnnotationGridLayer::editEnd(View *, QMouseEvent *)
{
    //    cerr << "AnnotationGridLayer::editEnd(" << e->x() << "," << e->y() << ")" << endl;
    if (!m_model || !m_editing) return;

//    if (m_editingCommand) {

//        QString newName = m_editingCommand->getName();

//        if (m_editingPoint.frame != m_originalPoint.frame) {
//            if (m_editingPoint.height != m_originalPoint.height) {
//                newName = tr("Move Label");
//            } else {
//                newName = tr("Move Label Horizontally");
//            }
//        } else {
//            newName = tr("Move Label Vertically");
//        }

//        m_editingCommand->setName(newName);
//        finish(m_editingCommand);
//    }

//    m_editingCommand = 0;
    m_editing = false;
}

bool
AnnotationGridLayer::editOpen(View *v, QMouseEvent *e)
{
    if (!m_model) return false;

    int tierIndex = getTierIndexForY(v, e->y());
    sv_frame_t frame = v->getFrameForX(e->x());

    // Info for this tier
    QString speakerID = m_tierTuples.at(tierIndex).speakerID;
    QString levelID = m_tierTuples.at(tierIndex).levelID;
    QString attributeID = m_tierTuples.at(tierIndex).attributeID;

    // Points for this tier
    QPointer<AnnotationGridPointModel> boundaryModel = m_model->boundariesForLevel(levelID);
    if (!boundaryModel) return false;
    AnnotationGridPointModel::PointList prevPoints = boundaryModel->getPreviousPoints(frame);

    if (!prevPoints.empty()) {
        QString label = m_model->data(speakerID, levelID, attributeID, prevPoints.begin()->itemNo).toString();

        QLineEdit *edit = new QLineEdit(v);


        edit->move(v->getXForFrame(prevPoints.begin()->frame), getYForTierIndex(v, tierIndex));
        edit->show();

//        QMessageBox::information(v, "test", QString("speakerID: %1\nlevelID: %2\nattributeID: %3\nframe: %4\nlabel: %5\nitemNo: %6")
//                                 .arg(speakerID).arg(levelID).arg(attributeID).arg(frame)
//                                 .arg(label).arg(prevPoints.begin()->itemNo));
    }


//    AnnotationGridModel::Point text(0);
//    if (!getPointToDrag(v, e->x(), e->y(), text)) return false;

//    QString label = text.label;

//    bool ok = false;
//    label = QInputDialog::getText(v, tr("Enter label"),
//                                  tr("Please enter a new label:"),
//                                  QLineEdit::Normal, label, &ok);
//    if (ok && label != text.label) {
//        AnnotationGridModel::RelabelCommand *command =
//                new AnnotationGridModel::RelabelCommand(m_model, text, label);
//        CommandHistory::getInstance()->addCommand(command);
//    }

    return true;
}

void
AnnotationGridLayer::moveSelection(Selection s, sv_frame_t newStartFrame)
{
    if (!m_model) return;

//    AnnotationGridModel::EditCommand *command =
//            new AnnotationGridModel::EditCommand(m_model, tr("Drag Selection"));

//    AnnotationGridModel::PointList points =
//            m_model->getPoints(s.getStartFrame(), s.getEndFrame());

//    for (AnnotationGridModel::PointList::iterator i = points.begin();
//         i != points.end(); ++i) {

//        if (s.contains(i->frame)) {
//            AnnotationGridModel::Point newPoint(*i);
//            newPoint.frame = i->frame + newStartFrame - s.getStartFrame();
//            command->deletePoint(*i);
//            command->addPoint(newPoint);
//        }
//    }

//    finish(command);
}

void
AnnotationGridLayer::resizeSelection(Selection s, Selection newSize)
{
    if (!m_model) return;

//    AnnotationGridModel::EditCommand *command =
//            new AnnotationGridModel::EditCommand(m_model, tr("Resize Selection"));

//    AnnotationGridModel::PointList points =
//            m_model->getPoints(s.getStartFrame(), s.getEndFrame());

//    double ratio =
//            double(newSize.getEndFrame() - newSize.getStartFrame()) /
//            double(s.getEndFrame() - s.getStartFrame());

//    for (AnnotationGridModel::PointList::iterator i = points.begin();
//         i != points.end(); ++i) {

//        if (s.contains(i->frame)) {

//            double target = double(i->frame);
//            target = double(newSize.getStartFrame()) +
//                    target - double(s.getStartFrame()) * ratio;

//            AnnotationGridModel::Point newPoint(*i);
//            newPoint.frame = lrint(target);
//            command->deletePoint(*i);
//            command->addPoint(newPoint);
//        }
//    }

//    finish(command);
}

void
AnnotationGridLayer::deleteSelection(Selection s)
{
    if (!m_model) return;

//    AnnotationGridModel::EditCommand *command =
//            new AnnotationGridModel::EditCommand(m_model, tr("Delete Selection"));

//    AnnotationGridModel::PointList points =
//            m_model->getPoints(s.getStartFrame(), s.getEndFrame());

//    for (AnnotationGridModel::PointList::iterator i = points.begin();
//         i != points.end(); ++i) {
//        if (s.contains(i->frame)) command->deletePoint(*i);
//    }

//    finish(command);
}

void
AnnotationGridLayer::copy(View *v, Selection s, Clipboard &to)
{
    if (!m_model) return;

//    AnnotationGridModel::PointList points =
//            m_model->getPoints(s.getStartFrame(), s.getEndFrame());

//    for (AnnotationGridModel::PointList::iterator i = points.begin();
//         i != points.end(); ++i) {
//        if (s.contains(i->frame)) {
//            Clipboard::Point point(i->frame, i->height, i->label);
//            point.setReferenceFrame(alignToReference(v, i->frame));
//            to.addPoint(point);
//        }
//    }
}

bool
AnnotationGridLayer::paste(View *v, const Clipboard &from, sv_frame_t /* frameOffset */, bool /* interactive */)
{
//    if (!m_model) return false;

//    const Clipboard::PointList &points = from.getPoints();

//    bool realign = false;

//    if (clipboardHasDifferentAlignment(v, from)) {

//        QMessageBox::StandardButton button =
//                QMessageBox::question(v, tr("Re-align pasted items?"),
//                                      tr("The items you are pasting came from a layer with different source material from this one.  Do you want to re-align them in time, to match the source material for this layer?"),
//                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
//                                      QMessageBox::Yes);

//        if (button == QMessageBox::Cancel) {
//            return false;
//        }

//        if (button == QMessageBox::Yes) {
//            realign = true;
//        }
//    }

//    AnnotationGridModel::EditCommand *command =
//            new AnnotationGridModel::EditCommand(m_model, tr("Paste"));

//    double valueMin = 0.0, valueMax = 1.0;
//    for (Clipboard::PointList::const_iterator i = points.begin();
//         i != points.end(); ++i) {
//        if (i->haveValue()) {
//            if (i->getValue() < valueMin) valueMin = i->getValue();
//            if (i->getValue() > valueMax) valueMax = i->getValue();
//        }
//    }
//    if (valueMax < valueMin + 1.0) valueMax = valueMin + 1.0;

//    for (Clipboard::PointList::const_iterator i = points.begin();
//         i != points.end(); ++i) {

//        if (!i->haveFrame()) continue;
//        sv_frame_t frame = 0;

//        if (!realign) {

//            frame = i->getFrame();

//        } else {

//            if (i->haveReferenceFrame()) {
//                frame = i->getReferenceFrame();
//                frame = alignFromReference(v, frame);
//            } else {
//                frame = i->getFrame();
//            }
//        }

//        AnnotationGridModel::Point newPoint(frame);

//        if (i->haveValue()) {
//            newPoint.height = float((i->getValue() - valueMin) / (valueMax - valueMin));
//        } else {
//            newPoint.height = 0.5f;
//        }

//        if (i->haveLabel()) {
//            newPoint.label = i->getLabel();
//        } else if (i->haveValue()) {
//            newPoint.label = QString("%1").arg(i->getValue());
//        } else {
//            newPoint.label = tr("New Point");
//        }

//        command->addPoint(newPoint);
//    }

//    finish(command);
//    return true;
}

int
AnnotationGridLayer::getDefaultColourHint(bool darkbg, bool &impose)
{
    impose = false;
    return ColourDatabase::getInstance()->getColourIndex
            (QString(darkbg ? "Bright Orange" : "Orange"));
}

void
AnnotationGridLayer::toXml(QTextStream &stream,
                           QString indent, QString extraAttributes) const
{
    SingleColourLayer::toXml(stream, indent, extraAttributes);
}

void
AnnotationGridLayer::setProperties(const QXmlAttributes &attributes)
{
    SingleColourLayer::setProperties(attributes);
}


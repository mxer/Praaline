#ifndef GROUPINGANNOTATIONEDITORWIDGET_H
#define GROUPINGANNOTATIONEDITORWIDGET_H

#include <QObject>
#include <QPointer>
#include <QMap>
#include <QWidget>
#include <QItemSelection>
#include <QIdentityProxyModel>
#include <QModelIndex>
#include "base/RealTime.h"
#include "model/annotation/AnnotationGroupingTierTableModel.h"
#include "TimelineEditorWidgetBase.h"
#include "SelectionListDataProviderProxy.h"

struct GroupingAnnotationEditorWidgetData;

class GroupingAnnotationEditorWidget : public TimelineEditorWidgetBase
{
    Q_OBJECT

public:
    explicit GroupingAnnotationEditorWidget(QWidget *parent = 0);
    ~GroupingAnnotationEditorWidget();

    void setData(QMap<QString, QPointer<Praaline::Core::AnnotationTierGroup> > &tierGroups,
                 const QString &tiernameGrouping, const QString &tiernameItems,
                 const QStringList &attributesGrouping = QStringList(),
                 const QStringList &attributesItems = QStringList());

    QString tiernameGrouping() const;
    void setTiernameGrouping(const QString &tiernameGrouping);
    QString tiernameItems() const;
    void setTiernameItems(const QString tiernameItems);
    QStringList attributesGrouping() const;
    void setAttributesGrouping(const QStringList &attributesGrouping);
    QStringList attributesItems() const;
    void setAttributesItems(const QStringList &attributesItems);

    AnnotationGroupingTierTableModel *model();

    void moveToTime(const RealTime &time) override;
    void moveToTime(const RealTime &time, double &tMin_msec, double &tMax_msec) override;
    RealTime currentTime() const override;

public slots:
    void removeSorting();

protected:
    void clearModel() override;
    void initModel() override;

protected slots:
    void groupingSplit(int itemIndex);
    void groupingMergeWithPrevious();
    void groupingMergeWithNext();

private:
    GroupingAnnotationEditorWidgetData *d;
};

#endif // GROUPINGANNOTATIONEDITORWIDGET_H

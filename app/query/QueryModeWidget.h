#ifndef QUERYMODEWIDGET_H
#define QUERYMODEWIDGET_H

#include <QWidget>

namespace Ui {
class QueryModeWidget;
}

struct QueryModeWidgetData;

class QueryModeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QueryModeWidget(QWidget *parent = 0);
    ~QueryModeWidget();

signals:
    void activateMode();

private slots:
    void showConcordancer();
    void showCreateDataset();
    void showAnnotationBrowser();
    void showAdvancedQueries();
    void showExtractSamples();

private:
    Ui::QueryModeWidget *ui;
    QueryModeWidgetData *d;

    void setupActions();
};

#endif // QUERYMODEWIDGET_H

#ifndef EXPORTVISUALISATIONDIALOG_H
#define EXPORTVISUALISATIONDIALOG_H

#include <QDialog>
#include <QMap>

namespace Ui {
class ExportVisualisationDialog;
}

#include "pncore/annotation/AnnotationTierGroup.h"
using namespace Praaline::Core;

struct ExportVisualisationDialogData;
class Document;
class PaneStack;
class ViewManager;

class ExportVisualisationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportVisualisationDialog(Document *document, PaneStack *paneStack, ViewManager *viewManager,
                                       QMap<QString, QPointer<AnnotationTierGroup> > &tiers,
                                       QWidget *parent = 0);
    ~ExportVisualisationDialog();

private slots:
    void selectExportDirectory();
    void doExport();

private:
    Ui::ExportVisualisationDialog *ui;
    ExportVisualisationDialogData *d;
};

#endif // EXPORTVISUALISATIONDIALOG_H

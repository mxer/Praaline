#ifndef EXPORTANNOTATIONSWIZARD_H
#define EXPORTANNOTATIONSWIZARD_H

#include <QPointer>
#include <QWizard>
#include "pncore/corpus/Corpus.h"
using namespace Praaline::Core;

namespace Ui {
class ExportAnnotationsWizard;
}

struct ExportAnnotationsWizardData;

class ExportAnnotationsWizard : public QWizard
{
    Q_OBJECT

public:
    explicit ExportAnnotationsWizard(QWidget *parent = 0);
    ~ExportAnnotationsWizard();

private slots:
    void selectFileFolder();
    void exportFormatChanged(int);
    void corpusRepositoryChanged(const QString &repositoryID);

private:
    Ui::ExportAnnotationsWizard *ui;
    ExportAnnotationsWizardData *d;

    // Override methods from QWizard
    bool validateCurrentPage() override;

    void doExport();
};

#endif // EXPORTANNOTATIONSWIZARD_H

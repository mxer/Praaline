#include <QString>
#include <QList>
#include <QSet>
#include <QHash>
#include <QToolBar>
#include <QFileDialog>
#include <QMenuBar>
#include <QTreeView>
#include <QProgressDialog>

#include "CorpusExplorerWidget.h"
#include "ui_CorpusExplorerWidget.h"

#include <QtilitiesProjectManagement>
using namespace QtilitiesProjectManagement;

#include "pncore/corpus/Corpus.h"
#include "pncore/datastore/CorpusRepository.h"
#include "pncore/datastore/FileDatastore.h"
#include "pncore/datastore/MetadataDatastore.h"
#include "pncore/datastore/AnnotationDatastore.h"

#include "pngui/model/corpus/CorpusExplorerTreeModel.h"
#include "pngui/widgets/SelectionDialog.h"
#include "pngui/widgets/MetadataEditorWidget.h"
#include "pngui/widgets/CorpusItemPreview.h"
#include "pngui/observers/CorpusObserver.h"

#include "CorpusRepositoriesManager.h"

#include "AddNewCorpusItemDialog.h"
#include "CorpusExplorerOptionsDialog.h"
#include "CheckMediaFilesDialog.h"
#include "SplitCommunicationsDialog.h"

#include "importmetadatawizard/ImportMetadataWizard.h"
#include "exportmetadatawizard/ExportMetadataWizard.h"
#include "importcorpusitemswizard/ImportCorpusItemsWizard.h"
#include "exportannotationswizard/ExportAnnotationsWizard.h"

struct CorpusExplorerWidgetData {
    CorpusExplorerWidgetData() :
        projectItem(0), corpusRepositoriesManager(0), corporaTopLevelNode(0), corporaObserverWidget(0),
        metadataEditorPrimary(0), metadataEditorSecondary(0), preview(0)
    { }

    QAction *actionSaveMetadata;
    QAction *actionAddCorpus;
    QAction *actionAddCommunication;
    QAction *actionAddSpeaker;
    QAction *actionAddRecording;
    QAction *actionAddAnnotation;
    QAction *actionAddParticipation;
    QAction *actionRemoveCorpusItems;
    QAction *actionRelinkCorpusItem;
    QAction *actionAddItemsFromFolder;
    QAction *actionImportMetadata;
    QAction *actionExportMetadata;
    QAction *actionExportAnnotations;
    QAction *actionAttributesAndGroupings;
    QAction *actionCheckMediaFiles;
    QAction *actionCreateAnnotationsFromRecordings;
    QAction *actionCreateSpeakersFromAnnotations;
    QAction *actionSplitCommunications;
    QAction *actionCleanUpParticipations;
    QAction *actionMetadataEditorPrimaryStyleTree;
    QAction *actionMetadataEditorPrimaryStyleGroupBox;
    QAction *actionMetadataEditorPrimaryStyleButton;
    QAction *actionMetadataEditorSecondaryStyleTree;
    QAction *actionMetadataEditorSecondaryStyleGroupBox;
    QAction *actionMetadataEditorSecondaryStyleButton;

    ObserverProjectItemWrapper* projectItem;

    CorpusRepositoriesManager *corpusRepositoriesManager;
    QHash<QString, CorpusObserver *> observersForCorpusRepositories;
    QPointer<TreeNode> corporaTopLevelNode;
    QPointer<Corpus> activeCorpus;

    ObserverWidget* corporaObserverWidget;
    MetadataEditorWidget *metadataEditorPrimary;
    MetadataEditorWidget *metadataEditorSecondary;

    QToolBar *toolbarCorpusExplorer;
    QMenu *menuMetadataEditorStyles;

    CorpusItemPreview *preview;
};

CorpusExplorerWidget::CorpusExplorerWidget(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CorpusExplorerWidget), d(new CorpusExplorerWidgetData)
{
    ui->setupUi(this);

    // Get Corpus Repositories Manager from global object list. When corpus repositories are opened or closed (i.e. added or removed from the manager)
    // we will receive a signal, and update the corpus explorer tree accordingly.
    QList<QObject *> list;
    list = OBJECT_MANAGER->registeredInterfaces("CorpusRepositoriesManager");
    foreach (QObject* obj, list) {
        CorpusRepositoriesManager *manager = qobject_cast<CorpusRepositoriesManager *>(obj);
        if (manager) d->corpusRepositoriesManager = manager;
    }
    if (d->corpusRepositoriesManager) {
        connect(d->corpusRepositoriesManager, SIGNAL(corpusRepositoryAdded(QString)),
                this, SLOT(corpusRepositoryAdded(QString)));
        connect(d->corpusRepositoriesManager, SIGNAL(corpusRepositoryRemoved(QString)),
                this, SLOT(corpusRepositoryRemoved(QString)));
    }

    d->corporaTopLevelNode = new TreeNode(tr("Corpus Explorer"));
    d->corporaTopLevelNode->enableNamingControl(ObserverHints::EditableNames, NamingPolicyFilter::ProhibitDuplicateNames,
                                                NamingPolicyFilter::AutoRename);
    d->corporaTopLevelNode->displayHints()->setActionHints(ObserverHints::ActionNoHints);
    d->corporaTopLevelNode->displayHints()->setDisplayFlagsHint(ObserverHints::NoDisplayFlagsHint);
    d->corporaTopLevelNode->displayHints()->setDragDropHint(ObserverHints::AllDragDrop);
    // Uncomment these to play around with root display formats.
    //d->corporaTopLevelNode->displayHints()->setRootIndexDisplayHint(ObserverHints::RootIndexDisplayUndecorated);
    //d->corporaTopLevelNode->displayHints()->setRootIndexDisplayHint(ObserverHints::RootIndexDisplayDecorated);
    d->corporaTopLevelNode->displayHints()->setRootIndexDisplayHint(ObserverHints::RootIndexHide); // This is the default

    OBJECT_MANAGER->registerObject(d->corporaTopLevelNode, QtilitiesCategory("Corpus"));

    // Project Item
    // d->projectItem = new ObserverProjectItemWrapper(d->top_level_node, this);

    // Menu and Toolbar actions
    d->toolbarCorpusExplorer = new QToolBar(tr("Corpus Explorer"), this);
    d->toolbarCorpusExplorer->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    d->toolbarCorpusExplorer->setIconSize(QSize(24, 24));
    setupActions();
    this->addToolBar(d->toolbarCorpusExplorer);

    // Set up corpora observer widget
    d->corporaObserverWidget = new ObserverWidget(Qtilities::TreeView);
    d->corporaObserverWidget->setRefreshMode(ObserverWidget::RefreshModeShowTree);
    d->corporaObserverWidget->setGlobalMetaType("Corpus Tree Meta Type");
    d->corporaObserverWidget->setAcceptDrops(true);
    connect(d->corporaObserverWidget, SIGNAL(selectedObjectsChanged(QList<QObject*>)), SLOT(selectionChanged(QList<QObject*>)));
    d->corporaObserverWidget->setObserverContext(d->corporaTopLevelNode);
    d->corporaObserverWidget->layout()->setMargin(0);
    d->corporaObserverWidget->initialize();

    // Set up corpus item preview widget
    d->preview = new CorpusItemPreview(this);
    d->preview->layout()->setMargin(0);
    ui->dockCorpusItemPreview->setWidget(d->preview);

    // Create layout of the Corpus Explorer
    ui->gridLayout->setMargin(0);
    ui->dockCorpusExplorer->setWidget(d->corporaObserverWidget);
    d->metadataEditorPrimary = new MetadataEditorWidget(MetadataEditorWidget::TreeStyle, this);
    d->metadataEditorPrimary->layout()->setMargin(0);
    ui->dockMetadataPrimary->setWidget(d->metadataEditorPrimary);
    d->metadataEditorSecondary = new MetadataEditorWidget(MetadataEditorWidget::TreeStyle, this);
    d->metadataEditorSecondary->layout()->setMargin(0);
    ui->dockMetadataSecondary->setWidget(d->metadataEditorSecondary);

    // Layout
    ui->splitterEditorsAndPreview->setStretchFactor(0, 2);
    ui->splitterEditorsAndPreview->setStretchFactor(1, 1);
}

CorpusExplorerWidget::~CorpusExplorerWidget()
{
    delete ui;
    delete d;
}

void CorpusExplorerWidget::setupActions()
{
    // Context and menu bar
    QList<int> context;
    context.push_front(CONTEXT_MANAGER->contextID(qti_def_CONTEXT_STANDARD));

    bool existed;
    Command* command;

    ActionContainer* menu_bar = ACTION_MANAGER->menuBar(qti_action_MENUBAR_STANDARD);
    ActionContainer* view_menu = ACTION_MANAGER->createMenu(qti_action_VIEW, existed);
    if (!existed) menu_bar->addMenu(view_menu, qti_action_HELP);
    ActionContainer* corpus_menu = ACTION_MANAGER->createMenu(tr("&Corpus"), existed);
    if (!existed) menu_bar->addMenu(corpus_menu, qti_action_HELP);

    // ------------------------------------------------------------------------------------------------------
    // CORPUS MENU
    // ------------------------------------------------------------------------------------------------------
    d->actionSaveMetadata = new QAction(QIcon(":icons/actions/save.png"), tr("Save Metadata"), this);
    connect(d->actionSaveMetadata, SIGNAL(triggered()), SLOT(saveMetadata()));
    command = ACTION_MANAGER->registerAction("Corpus.SaveMetadata", d->actionSaveMetadata, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);
    d->toolbarCorpusExplorer->addAction(d->actionSaveMetadata);

    d->actionAddCorpus = new QAction(QIcon(":icons/actions/list_add.png"), tr("Add Corpus..."), this);
    connect(d->actionAddCorpus, SIGNAL(triggered()), SLOT(addCorpus()));
    command = ACTION_MANAGER->registerAction("Corpus.AddCorpus", d->actionAddCorpus, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);
    d->toolbarCorpusExplorer->addAction(d->actionAddCorpus);

    d->actionAddCommunication = new QAction(QIcon(":icons/actions/list_add.png"), tr("Add Communication..."), this);
    connect(d->actionAddCommunication, SIGNAL(triggered()), SLOT(addCommunication()));
    command = ACTION_MANAGER->registerAction("Corpus.AddCommunication", d->actionAddCommunication, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);
    d->toolbarCorpusExplorer->addAction(d->actionAddCommunication);

    d->actionAddSpeaker = new QAction(QIcon(":icons/actions/list_add.png"), tr("Add Speaker..."), this);
    connect(d->actionAddSpeaker, SIGNAL(triggered()), SLOT(addSpeaker()));
    command = ACTION_MANAGER->registerAction("Corpus.AddSpeaker", d->actionAddSpeaker, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);
    d->toolbarCorpusExplorer->addAction(d->actionAddSpeaker);

    d->actionAddRecording = new QAction(QIcon(":icons/actions/list_add.png"), tr("Add Media Recording..."), this);
    connect(d->actionAddRecording, SIGNAL(triggered()), SLOT(addRecording()));
    command = ACTION_MANAGER->registerAction("Corpus.AddRecording", d->actionAddRecording, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);
    d->toolbarCorpusExplorer->addAction(d->actionAddRecording);

    d->actionAddAnnotation = new QAction(QIcon(":icons/actions/list_add.png"), tr("Add Annotation..."), this);
    connect(d->actionAddAnnotation, SIGNAL(triggered()), SLOT(addAnnotation()));
    command = ACTION_MANAGER->registerAction("Corpus.AddAnnotation", d->actionAddAnnotation, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);
    d->toolbarCorpusExplorer->addAction(d->actionAddAnnotation);

    d->actionAddParticipation = new QAction(QIcon(":icons/actions/list_add.png"), tr("Add Participation..."), this);
    connect(d->actionAddParticipation, SIGNAL(triggered()), SLOT(addParticipation()));
    command = ACTION_MANAGER->registerAction("Corpus.AddParticipation", d->actionAddParticipation, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);
    d->toolbarCorpusExplorer->addAction(d->actionAddParticipation);

    d->actionRemoveCorpusItems = new QAction(QIcon(":icons/actions/list_remove.png"), tr("Remove Corpus Item(s)"), this);
    connect(d->actionRemoveCorpusItems, SIGNAL(triggered()), SLOT(removeCorpusItems()));
    command = ACTION_MANAGER->registerAction("Corpus.RemoveCorpusItems", d->actionRemoveCorpusItems, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);
    d->toolbarCorpusExplorer->addAction(d->actionRemoveCorpusItems);

    d->actionRelinkCorpusItem = new QAction(tr("Link Item to another Communication..."), this);
    connect(d->actionRelinkCorpusItem, SIGNAL(triggered()), SLOT(relinkCorpusItem()));
    command = ACTION_MANAGER->registerAction("Corpus.RelinkCorpusItem", d->actionRelinkCorpusItem, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);

    corpus_menu->addSeparator();

    // Import - export functionality

    d->actionImportMetadata = new QAction(tr("Import corpus metadata..."), this);
    connect(d->actionImportMetadata, SIGNAL(triggered()), SLOT(importMetadata()));
    command = ACTION_MANAGER->registerAction("Corpus.ImportMetadata", d->actionImportMetadata, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);

    d->actionExportMetadata = new QAction(tr("Export corpus metadata..."), this);
    connect(d->actionExportMetadata, SIGNAL(triggered()), SLOT(exportMetadata()));
    command = ACTION_MANAGER->registerAction("Corpus.ExportMetadata", d->actionExportMetadata, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);

    d->actionAddItemsFromFolder = new QAction(tr("Add corpus items from folder..."), this);
    connect(d->actionAddItemsFromFolder, SIGNAL(triggered()), SLOT(addItemsFromFolder()));
    command = ACTION_MANAGER->registerAction("Corpus.AddItemsFromFolder", d->actionAddItemsFromFolder, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);

    d->actionExportAnnotations = new QAction(tr("Export annotations..."), this);
    connect(d->actionExportAnnotations, SIGNAL(triggered()), SLOT(exportAnnotations()));
    command = ACTION_MANAGER->registerAction("Corpus.ExportAnnotations", d->actionExportAnnotations, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);

    corpus_menu->addSeparator();

    d->actionCheckMediaFiles = new QAction(tr("Check files of Media Recordings..."), this);
    connect(d->actionCheckMediaFiles, SIGNAL(triggered()), SLOT(checkMediaFiles()));
    command = ACTION_MANAGER->registerAction("Corpus.CheckMediaFiles", d->actionCheckMediaFiles, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);

    d->actionSplitCommunications = new QAction(tr("Split Communications based on annotation..."), this);
    connect(d->actionSplitCommunications, SIGNAL(triggered()), SLOT(splitCommunications()));
    command = ACTION_MANAGER->registerAction("Corpus.SplitCommunications", d->actionSplitCommunications, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);

    d->actionCreateAnnotationsFromRecordings = new QAction(tr("Create Annotations for Recordings not having one..."), this);
    connect(d->actionCreateAnnotationsFromRecordings, SIGNAL(triggered()), SLOT(createAnnotationsFromRecordings()));
    command = ACTION_MANAGER->registerAction("Corpus.AnnotationsFromRecordings", d->actionCreateAnnotationsFromRecordings, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);

    d->actionCreateSpeakersFromAnnotations = new QAction(tr("Create Speakers and Participations from Annotations..."), this);
    connect(d->actionCreateSpeakersFromAnnotations, SIGNAL(triggered()), SLOT(createSpeakersFromAnnotations()));
    command = ACTION_MANAGER->registerAction("Corpus.SpeakersParticipationsFromAnnotations", d->actionCreateSpeakersFromAnnotations, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);

    d->actionCleanUpParticipations = new QAction(tr("Clean-up Participations based on Annotations..."), this);
    connect(d->actionCleanUpParticipations, SIGNAL(triggered()), SLOT(cleanUpParticipationsFromAnnotations()));
    command = ACTION_MANAGER->registerAction("Corpus.CleanUpParticipationsFromAnnotations", d->actionCleanUpParticipations, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    corpus_menu->addAction(command);

    // ------------------------------------------------------------------------------------------------------
    // VIEW MENU
    // ------------------------------------------------------------------------------------------------------
    d->actionAttributesAndGroupings = new QAction(tr("Select Attributes and Groupings"), this);
    connect(d->actionAttributesAndGroupings, SIGNAL(triggered()), SLOT(attributesAndGroupings()));
    command = ACTION_MANAGER->registerAction("Corpus.AttributesAndGroupings", d->actionAttributesAndGroupings, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    view_menu->addAction(command);

    // Metadata editor styling menus (inserted on the toolbar)
    setupMetadataEditorsStylingMenu();
}

void CorpusExplorerWidget::setupMetadataEditorsStylingMenu()
{
    QList<int> context;
    context.push_front(CONTEXT_MANAGER->contextID(qti_def_CONTEXT_STANDARD));
    Command* command;

    d->menuMetadataEditorStyles = new QMenu(this);

    d->actionMetadataEditorPrimaryStyleTree = new QAction(tr("Primary Metadata Editor: Tree View"), this);
    connect(d->actionMetadataEditorPrimaryStyleTree, SIGNAL(triggered()), SLOT(metadataEditorPrimaryStyleTree()));
    command = ACTION_MANAGER->registerAction("Corpus.Explorer.PrimaryMetadataEditor.Tree", d->actionMetadataEditorPrimaryStyleTree, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    d->menuMetadataEditorStyles->addAction(d->actionMetadataEditorPrimaryStyleTree);

    d->actionMetadataEditorPrimaryStyleGroupBox = new QAction(tr("Primary Metadata Editor: Form View"), this);
    connect(d->actionMetadataEditorPrimaryStyleGroupBox, SIGNAL(triggered()), SLOT(metadataEditorPrimaryStyleGroupBox()));
    command = ACTION_MANAGER->registerAction("Corpus.Explorer.PrimaryMetadataEditor.GroupBox", d->actionMetadataEditorPrimaryStyleGroupBox, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    d->menuMetadataEditorStyles->addAction(d->actionMetadataEditorPrimaryStyleGroupBox);

    d->actionMetadataEditorPrimaryStyleButton = new QAction(tr("Primary Metadata Editor: Compact View"), this);
    connect(d->actionMetadataEditorPrimaryStyleButton, SIGNAL(triggered()), SLOT(metadataEditorPrimaryStyleButton()));
    command = ACTION_MANAGER->registerAction("Corpus.Explorer.PrimaryMetadataEditor.Button", d->actionMetadataEditorPrimaryStyleButton, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    d->menuMetadataEditorStyles->addAction(d->actionMetadataEditorPrimaryStyleButton);

    d->menuMetadataEditorStyles->addSeparator();

    d->actionMetadataEditorSecondaryStyleTree = new QAction(tr("Secondary Metadata Editor: Tree View"), this);
    connect(d->actionMetadataEditorSecondaryStyleTree, SIGNAL(triggered()), SLOT(metadataEditorSecondaryStyleTree()));
    command = ACTION_MANAGER->registerAction("Corpus.Explorer.SecondaryMetadataEditor.Tree", d->actionMetadataEditorSecondaryStyleTree, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    d->menuMetadataEditorStyles->addAction(d->actionMetadataEditorSecondaryStyleTree);

    d->actionMetadataEditorSecondaryStyleGroupBox = new QAction(tr("Secondary Metadata Editor: Form View"), this);
    connect(d->actionMetadataEditorSecondaryStyleGroupBox, SIGNAL(triggered()), SLOT(metadataEditorSecondaryStyleGroupBox()));
    command = ACTION_MANAGER->registerAction("Corpus.Explorer.SecondaryMetadataEditor.GroupBox", d->actionMetadataEditorSecondaryStyleGroupBox, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    d->menuMetadataEditorStyles->addAction(d->actionMetadataEditorSecondaryStyleGroupBox);

    d->actionMetadataEditorSecondaryStyleButton = new QAction(tr("Secondary Metadata Editor: Compact View"), this);
    connect(d->actionMetadataEditorSecondaryStyleButton, SIGNAL(triggered()), SLOT(metadataEditorSecondaryStyleButton()));
    command = ACTION_MANAGER->registerAction("Corpus.Explorer.SecondaryMetadataEditor.Button", d->actionMetadataEditorSecondaryStyleButton, context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    d->menuMetadataEditorStyles->addAction(d->actionMetadataEditorSecondaryStyleButton);

    // Add the pop-up menu to the corpus explorer toolbar
    QToolButton* toolButtonMetadataEditorStyles = new QToolButton();
    toolButtonMetadataEditorStyles->setMenu(d->menuMetadataEditorStyles);
    toolButtonMetadataEditorStyles->setPopupMode(QToolButton::InstantPopup);
    QWidgetAction* toolButtonActionMetadataEditorStyles = new QWidgetAction(this);
    toolButtonActionMetadataEditorStyles->setDefaultWidget(toolButtonMetadataEditorStyles);
    d->toolbarCorpusExplorer->addAction(toolButtonActionMetadataEditorStyles);
}

// ==============================================================================================================================
// Respond to changes in the corpus repositories manager (opening and closing corpus repositories)
// ==============================================================================================================================
void CorpusExplorerWidget::corpusRepositoryAdded(const QString &repositoryID)
{
    if (!d->corpusRepositoriesManager) return;
    QPointer<CorpusRepository> repository = d->corpusRepositoriesManager->corpusRepositoryByID(repositoryID);
    if (!repository) return;
    CorpusObserver *obs = new CorpusObserver(repository);
    OBJECT_MANAGER->registerObject(obs, QtilitiesCategory("CorpusObserver"));
    d->observersForCorpusRepositories.insert(repositoryID, obs);
    d->corporaTopLevelNode->addNode(obs->nodeRepository());
}

void CorpusExplorerWidget::corpusRepositoryRemoved(const QString &repositoryID)
{
    if (!d->corpusRepositoriesManager) return;
    d->corporaTopLevelNode->removeItem(repositoryID);
    CorpusObserver *obs = d->observersForCorpusRepositories.value(repositoryID);
    if (obs) {
        OBJECT_MANAGER->removeObject(obs);
        delete obs;
    }
    // Metadata editors
    d->metadataEditorPrimary->clear();
    d->metadataEditorSecondary->clear();
}

// ==============================================================================================================================
// Update metadata editors when the user selects a communication or speaker
// ==============================================================================================================================

void CorpusExplorerWidget::updateMetadataEditorsForCom(CorpusCommunication *communication)
{
    if (!communication) return;
    if (!communication->repository()) return;
    if (!communication->corpus()) return;
    // Primary Editor: Communication (+ Recordings, Annotations)
    // Secondary Editor: Participations + Speaker
    QList<QPointer<CorpusObject> > itemsMain;
    itemsMain << static_cast<CorpusObject *>(communication);
    foreach(QPointer<CorpusRecording> rec, communication->recordings())
        if (rec) itemsMain << static_cast<CorpusObject *>(rec);
    foreach(QPointer<CorpusAnnotation> annot, communication->annotations())
        if (annot) itemsMain << static_cast<CorpusObject *>(annot);
    d->metadataEditorPrimary->rebind(communication->repository()->metadataStructure(), itemsMain);
    // Speakers participating in Communication
    QList<QPointer<CorpusObject> > itemsSecondary;
    foreach (QPointer<CorpusParticipation> part, communication->corpus()->participationsForCommunication(communication->ID()))
        if (part) itemsSecondary << static_cast<CorpusObject *>(part);
    d->metadataEditorSecondary->rebind(communication->repository()->metadataStructure(), itemsSecondary, true, false);
}

void CorpusExplorerWidget::updateMetadataEditorsForSpk(CorpusSpeaker *speaker)
{
    if (!speaker) return;
    if (!speaker->repository()) return;
    if (!speaker->corpus()) return;
    // Primary Editor: Speaker
    // Secondary Editor: Participations + Communications
    QList<QPointer<CorpusObject> > itemsMain;
    itemsMain << static_cast<CorpusObject *>(speaker);
    d->metadataEditorPrimary->rebind(speaker->repository()->metadataStructure(), itemsMain);
    // Communication where Speaker participates
    QList<QPointer<CorpusObject> > itemsSecondary;
    foreach (QPointer<CorpusParticipation> part, speaker->corpus()->participationsForSpeaker(speaker->ID()))
        if (part) itemsSecondary << static_cast<CorpusObject *>(part);
    d->metadataEditorSecondary->rebind(speaker->repository()->metadataStructure(), itemsSecondary, false, true);
}

void CorpusExplorerWidget::updateMetadataEditorsForCorpus(Corpus *corpus)
{
    if (!corpus) return;
    if (!corpus->repository()) return;
    // Primary Editor: Corpus - Secondary Editor: None, show corpus statistics widget
    QList<QPointer<CorpusObject> > itemsMain;
    itemsMain << static_cast<CorpusObject *>(corpus);
    d->metadataEditorPrimary->rebind(corpus->repository()->metadataStructure(), itemsMain);
    QList<QPointer<CorpusObject> > itemsSecondary;
    d->metadataEditorSecondary->rebind(corpus->repository()->metadataStructure(), itemsSecondary, false, false);
}

void CorpusExplorerWidget::selectionChanged(QList<QObject*> selected)
{
    if (selected.isEmpty()) {
        return;
        d->preview->openCommunication(Q_NULLPTR);
    }
    QObject *obj = selected.first();
    CorpusExplorerTreeNodeCorpus *nodeCorpus = qobject_cast<CorpusExplorerTreeNodeCorpus *>(obj);
    if (nodeCorpus && nodeCorpus->corpus()) {
        d->activeCorpus = nodeCorpus->corpus();
        updateMetadataEditorsForCorpus(nodeCorpus->corpus());
        return;
    }
    CorpusExplorerTreeNodeCommunication *nodeCom = qobject_cast<CorpusExplorerTreeNodeCommunication *>(obj);
    if (nodeCom && nodeCom->communication) {
        d->activeCorpus = nodeCom->communication->corpus();
        updateMetadataEditorsForCom(nodeCom->communication);
        d->preview->openCommunication(nodeCom->communication);
        return;
    }
    CorpusExplorerTreeNodeSpeaker *nodeSpk = qobject_cast<CorpusExplorerTreeNodeSpeaker *>(obj);
    if (nodeSpk && nodeSpk->speaker) {
        d->activeCorpus = nodeSpk->speaker->corpus();
        updateMetadataEditorsForSpk(nodeSpk->speaker);
        return;
    }
    CorpusExplorerTreeNodeRecording *nodeRec = qobject_cast<CorpusExplorerTreeNodeRecording *>(obj);
    if (nodeRec && nodeRec->recording) {
        d->activeCorpus = nodeRec->recording->corpus();
        CorpusCommunication *communication = qobject_cast<CorpusCommunication *>(nodeRec->recording->parent());
        updateMetadataEditorsForCom(communication);
        d->preview->openCommunication(communication);
        return;
    }
    CorpusExplorerTreeNodeAnnotation *nodeAnnot = qobject_cast<CorpusExplorerTreeNodeAnnotation *>(obj);
    if (nodeAnnot && nodeAnnot->annotation) {
        d->activeCorpus = nodeAnnot->annotation->corpus();
        CorpusCommunication *communication = qobject_cast<CorpusCommunication *>(nodeAnnot->annotation->parent());
        updateMetadataEditorsForCom(communication);
        d->preview->openCommunication(communication);
        return;
    }
}

// =========================================================================================================================================
// Save
// =========================================================================================================================================
void CorpusExplorerWidget::saveMetadata()
{
    d->activeCorpus->save();
}

// =========================================================================================================================================
// Add and remove corpus objects
// =========================================================================================================================================

void CorpusExplorerWidget::addCorpus()
{

}

void CorpusExplorerWidget::addCommunication()
{
    if (!d->activeCorpus) {
        QMessageBox::warning(this, tr("Add Communication to Corpus"),
                             tr("Please select the Corpus to which the Communication will be added."),
                             QMessageBox::Ok);
        return;
    }
    if (!d->activeCorpus->repository()) return;
    bool ok;
    QString communicationID = QInputDialog::getText(this, tr("Add new Communication"),
                                                    tr("Communication ID:"), QLineEdit::Normal, "", &ok);
    if (!ok || communicationID.isEmpty()) return;
    d->activeCorpus->addCommunication(new CorpusCommunication(communicationID, d->activeCorpus->repository(), d->activeCorpus));
}

void CorpusExplorerWidget::addSpeaker()
{
    if (!d->activeCorpus) {
        QMessageBox::warning(this, tr("Add Speaker to Corpus"),
                             tr("Please select the Corpus to which the Speaker will be added."),
                             QMessageBox::Ok);
        return;
    }
    if (!d->activeCorpus->repository()) return;
    bool ok;
    QString speakerID = QInputDialog::getText(this, tr("Add new Speaker"),
                                                    tr("Speaker ID:"), QLineEdit::Normal, "", &ok);
    if (!ok || speakerID.isEmpty()) return;
    d->activeCorpus->addSpeaker(new CorpusSpeaker(speakerID, d->activeCorpus->repository(), d->activeCorpus));
}

void CorpusExplorerWidget::addRecording()
{
    CorpusExplorerTreeNodeCommunication *nodeCom = 0;
    if (d->corporaObserverWidget->selectedObjects().count() == 1) {
        nodeCom = qobject_cast<CorpusExplorerTreeNodeCommunication *>(d->corporaObserverWidget->selectedObjects().first());
    }
    if ((!nodeCom) || ((nodeCom) && (!nodeCom->communication))) {
        QMessageBox::warning(this, tr("Add Recording to Communication"),
                             tr("Please select the corpus Communication to which the Media Recordings will be added."),
                             QMessageBox::Ok);
        return;
    }
    Corpus *corpus = qobject_cast<Corpus *>(nodeCom->communication->parent());
    if (!corpus) return;
    QFileDialog::Options options;
    QString selectedFilter;
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Add Media Recordings to Corpus"),
                            corpus->repository()->files()->basePath(), tr("Wave Files (*.wav);;All Files (*)"),
                            &selectedFilter, options);
    if (fileNames.count() == 0) return;
    foreach(QString fileName, fileNames) {
        QFileInfo info(fileName);
        if (info.suffix() == "wav") {
            CorpusRecording *rec = new CorpusRecording(info.baseName(), corpus->repository(), nodeCom->communication);
            rec->setFilename(corpus->repository()->files()->getRelativeToBasePath(fileName));
            nodeCom->communication->addRecording(rec);
        }
    }
}

void CorpusExplorerWidget::addAnnotation()
{
    CorpusExplorerTreeNodeCommunication *nodeCom = 0;
    if (d->corporaObserverWidget->selectedObjects().count() == 1) {
        nodeCom = qobject_cast<CorpusExplorerTreeNodeCommunication *>(d->corporaObserverWidget->selectedObjects().first());
    }
    if ((!nodeCom) || ((nodeCom) && (!nodeCom->communication))) {
        QMessageBox::warning(this, tr("Add Recording to Communication"),
                             tr("Please select the corpus Communication to which the Annotation will be added."),
                             QMessageBox::Ok);
        return;
    }
    Corpus *corpus = qobject_cast<Corpus *>(nodeCom->communication->parent());
    if (!corpus) return;
    bool ok;
    QString annotationID = QInputDialog::getText(this, tr("Add new Annotation"),
                                                 tr("Annotation ID:"), QLineEdit::Normal, "", &ok);
    if (!ok || annotationID.isEmpty()) return;
    CorpusAnnotation *annot = new CorpusAnnotation(annotationID,corpus->repository(), nodeCom->communication);
    if (nodeCom->communication->hasRecording(annot->ID()))
        annot->setRecordingID(annot->ID());
    nodeCom->communication->addAnnotation(annot);
}

void CorpusExplorerWidget::addParticipation()
{
    CorpusExplorerTreeNodeCommunication *nodeCom(Q_NULLPTR);
    CorpusExplorerTreeNodeSpeaker *nodeSpk(Q_NULLPTR);
    if (d->corporaObserverWidget->selectedObjects().count() != 2) {
        QMessageBox::warning(this, tr("Add Participation"),
                             tr("Please select exactly one Communication and one Speaker belonging to the same corpus."),
                             QMessageBox::Ok);
        return;
    }
    nodeCom = qobject_cast<CorpusExplorerTreeNodeCommunication *>(d->corporaObserverWidget->selectedObjects().first());
    nodeSpk = qobject_cast<CorpusExplorerTreeNodeSpeaker *>(d->corporaObserverWidget->selectedObjects().last());
    if ((!nodeCom) || (!nodeSpk)) {
        nodeCom = qobject_cast<CorpusExplorerTreeNodeCommunication *>(d->corporaObserverWidget->selectedObjects().last());
        nodeSpk = qobject_cast<CorpusExplorerTreeNodeSpeaker *>(d->corporaObserverWidget->selectedObjects().first());
    }
    if ((!nodeCom) || ((nodeCom) && (!nodeCom->communication)) || (!nodeSpk) || ((nodeSpk) && (!nodeSpk->speaker))) {
        QMessageBox::warning(this, tr("Add Participation"),
                             tr("Please select exactly one Communication and one Speaker belonging to the same corpus."),
                             QMessageBox::Ok);
        return;
    }
    Corpus *corpusCom = qobject_cast<Corpus *>(nodeCom->communication->parent());
    Corpus *corpusSpk = qobject_cast<Corpus *>(nodeSpk->speaker->parent());
    if ((!corpusCom) || (!corpusSpk)) return;
    if (corpusCom != corpusSpk) {
        QMessageBox::warning(this, tr("Add Participation"),
                             tr("The Communication and the Speaker must belong to the same corpus."),
                             QMessageBox::Ok);
        return;
    }
    corpusCom->addParticipation(nodeCom->communication->ID(), nodeSpk->speaker->ID());
    d->corporaObserverWidget->selectObject(nodeCom);
}

QList<CorpusObject *> CorpusExplorerWidget::selectedCorpusItems()
{
    QList<CorpusObject *> selected;
    foreach (QObject *obj, d->corporaObserverWidget->selectedObjects()) {
        CorpusExplorerTreeNodeCorpus *nodeCorpus = 0;
        nodeCorpus = qobject_cast<CorpusExplorerTreeNodeCorpus *>(obj);
        if (nodeCorpus && nodeCorpus->corpus()) { selected << nodeCorpus->corpus(); continue; }
        CorpusExplorerTreeNodeCommunication *nodeCom = 0;
        nodeCom = qobject_cast<CorpusExplorerTreeNodeCommunication *>(obj);
        if (nodeCom && nodeCom->communication) { selected << nodeCom->communication; continue; }
        CorpusExplorerTreeNodeSpeaker *nodeSpk = 0;
        nodeSpk = qobject_cast<CorpusExplorerTreeNodeSpeaker *>(obj);
        if (nodeSpk && nodeSpk->speaker) { selected << nodeSpk->speaker; continue; }
        CorpusExplorerTreeNodeRecording *nodeRec = 0;
        nodeRec = qobject_cast<CorpusExplorerTreeNodeRecording *>(obj);
        if (nodeRec && nodeRec->recording) { selected << nodeRec->recording; continue; }
        CorpusExplorerTreeNodeAnnotation *nodeAnnot = 0;
        nodeAnnot = qobject_cast<CorpusExplorerTreeNodeAnnotation *>(obj);
        if (nodeAnnot && nodeAnnot->annotation) { selected << nodeAnnot->annotation; continue; }
    }
    return selected;
}

void CorpusExplorerWidget::removeCorpusItems()
{
    QList<CorpusObject *> selected = selectedCorpusItems();
    if (selected.count() == 0) return;
    if (selected.count() == 1) {
        CorpusCommunication *com = qobject_cast<CorpusCommunication *>(selected.first());
        if (com) {
            Corpus *corpus = qobject_cast<Corpus *>(com->parent());
            if (!corpus) return;
            bool alsoDeleteData = false;
            if (QMessageBox::warning(this, tr("Remove communication from corpus?"),
                                     QString(tr("Do you want to remove Communication %1 from Corpus %2?"))
                                     .arg(com->name()).arg(corpus->ID()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) return;
            if (QMessageBox::warning(this, tr("Permanently delete data?"),
                                     QString(tr("Do you also want to delete all the annotation data associated with Communication %1 from Corpus %2?"))
                                     .arg(com->name()).arg(corpus->ID()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                alsoDeleteData = true;
            }
            if (alsoDeleteData) {
                foreach (QPointer<CorpusAnnotation> annot, com->annotations()) {
                    if (annot) {
                        corpus->repository()->annotations()->deleteAllTiersAllSpeakers(annot->ID());
                        com->removeAnnotation(annot->ID());
                    }
                }
            }
            corpus->removeCommunication(com->ID());
            return;
        }
        CorpusSpeaker *spk = qobject_cast<CorpusSpeaker *>(selected.first());
        if (spk) {
            Corpus *corpus = qobject_cast<Corpus *>(spk->parent());
            if (!corpus) return;
            if (QMessageBox::warning(this, tr("Remove speaker from corpus?"),
                                     QString(tr("Do you want to remove Speaker %1 from Corpus %2?"))
                                     .arg(spk->name()).arg(corpus->ID()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) return;
            corpus->removeSpeaker(spk->ID());
            return;
        }
        CorpusRecording *rec = qobject_cast<CorpusRecording *>(selected.first());
        if (rec) {
            CorpusCommunication *com = qobject_cast<CorpusCommunication *>(rec->parent());
            if (!com) return;
            if (QMessageBox::warning(this, tr("Remove recording from communication?"),
                                     QString(tr("Do you want to remove Recording %1 from Communication %2?"))
                                     .arg(rec->name()).arg(rec->communicationID()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) return;
            com->removeRecording(rec->ID());
            return;
        }
        CorpusAnnotation *annot = qobject_cast<CorpusAnnotation *>(selected.first());
        if (annot) {
            CorpusCommunication *com = qobject_cast<CorpusCommunication *>(annot->parent());
            if (!com) return;
            bool alsoDeleteData = false;
            if (QMessageBox::warning(this, tr("Remove annotation from communication?"),
                                     QString(tr("Do you want to remove Annotation %1 from Communication %2?"))
                                     .arg(annot->name()).arg(annot->communicationID()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) return;
            if (QMessageBox::warning(this, tr("Permanently delete data?"),
                                     QString(tr("Do you also want to delete all the annotation data associated with Annotation %1 in Communication %2?"))
                                     .arg(annot->name()).arg(annot->communicationID()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                alsoDeleteData = true;
            }
            if (alsoDeleteData) {
                Corpus *corpus = qobject_cast<Corpus *>(com->parent());
                if (corpus) {
                    corpus->repository()->annotations()->deleteAllTiersAllSpeakers(annot->ID());
                }
            }
            com->removeAnnotation(annot->ID());
            return;
        }
    }
    else {
        // Show dialogue to confirm multiple corpus item deletion
        QPointer<QStandardItemModel> modelDel = new QStandardItemModel(this);
        modelDel->setColumnCount(2);
        modelDel->setRowCount(selected.count());
        modelDel->setHorizontalHeaderLabels(QStringList() << tr("ID") << tr("Type"));
        int i = 0;
        foreach (CorpusObject *cobj, selected) {
            QStandardItem *item = new QStandardItem(cobj->ID());
            item->setCheckable(true);
            item->setCheckState(Qt::Checked);
            modelDel->setItem(i, 0, item);
            modelDel->setItem(i, 1, new QStandardItem(CorpusObject::typeToString(cobj->type())));
            i++;
        }
        // Confirmations
        QPointer<SelectionDialog> sel = new SelectionDialog(tr("Confirm deletion of corpus items"), modelDel, this);
        if (sel->exec() == QDialog::Rejected)
            return;
        bool alsoDeleteData = false;
        if (QMessageBox::warning(this, tr("Permanently delete data?"),
                                 QString(tr("Do you also want to delete the associated annotation data?")),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            alsoDeleteData = true;
        }
        // delete those confirmed
        d->corporaTopLevelNode->startTreeProcessingCycle();
        for (int i = 0; i < modelDel->rowCount(); ++i) {
            QStandardItem *item = modelDel->item(i, 0);
            if (item->checkState() == Qt::Checked) {
                // delete corpus item at position i
                CorpusObject *cobj = selected.at(i);
                if (cobj->type() == CorpusObject::Type_Communication) {
                    Corpus *corpus = qobject_cast<Corpus *>(cobj->parent());
                    if (corpus) {
                        if (alsoDeleteData) {
                            CorpusCommunication *com = qobject_cast<CorpusCommunication *>(cobj);
                            if (com) {
                                foreach (QPointer<CorpusAnnotation> annot, com->annotations()) {
                                    if (annot) {
                                        corpus->repository()->annotations()->deleteAllTiersAllSpeakers(annot->ID());
                                        com->removeAnnotation(annot->ID());
                                    }
                                }
                            }
                        }
                        corpus->removeCommunication(cobj->ID());
                    }
                }
                else if (cobj->type() == CorpusObject::Type_Speaker) {
                    Corpus *corpus = qobject_cast<Corpus *>(cobj->parent());
                    if (corpus) corpus->removeSpeaker(cobj->ID());
                }
                else if (cobj->type() == CorpusObject::Type_Recording) {
                    CorpusCommunication *com = qobject_cast<CorpusCommunication *>(cobj->parent());
                    if (com) com->removeRecording(cobj->ID());
                }
                else if (cobj->type() == CorpusObject::Type_Annotation) {
                    CorpusCommunication *com = qobject_cast<CorpusCommunication *>(cobj->parent());
                    if (com) com->removeAnnotation(cobj->ID());
                    if (alsoDeleteData) {
                        Corpus *corpus = qobject_cast<Corpus *>(com->parent());
                        if (corpus) {
                            corpus->repository()->annotations()->deleteAllTiersAllSpeakers(cobj->ID());
                        }
                    }
                }
            }
        }
        d->corporaTopLevelNode->endTreeProcessingCycle();
    }
}

void CorpusExplorerWidget::relinkCorpusItem()
{
//    QPointer<Corpus> corpus = d->corporaManager->activeCorpus();
//    if (!corpus) return;
//    QList<CorpusObject *> selected = selectedCorpusItems();

//    if (selected.count() < 1) {
//        QMessageBox::warning(this, tr("Relink Corpus Items"),
//                             tr("Please select Recording(s) and Annotation(s) to change their association with a Communication."),
//                             QMessageBox::Ok);
//        return;
//    }
//    // Get destination communication
//    bool ok;
//    QString communicationID = QInputDialog::getItem (this, tr("Relink Corpus Item"),
//                                                    tr("Please select the ID of the Communication to which you want to move the selected Recording"),
//                                                    corpus->communicationIDs(), 0, false, &ok);
//    if (!ok || communicationID.isEmpty()) return;
//    // Relink
//    foreach (CorpusObject *cobj, selected) {
//        if (cobj->type() == CorpusObject::Type_Recording) {
//            CorpusRecording *rec = qobject_cast<CorpusRecording *>(cobj);
//            QPointer<CorpusCommunication> com = corpus->communication(rec->communicationID());
//            if (!com) return;
////            com->unlinkRecording(rec->ID());
////            com = corpus->communication(communicationID);
////            com->addRecording(rec);
//        }
//        else if (cobj->type() == CorpusObject::Type_Annotation) {
//            CorpusAnnotation *annot = qobject_cast<CorpusAnnotation *>(cobj);
//            QPointer<CorpusCommunication> com = corpus->communication(annot->communicationID());
//            if (!com) return;
////            com->unlinkAnnotation(annot->ID());
////            com = corpus->communication(communicationID);
////            com->addAnnotation(annot);
//        }
//    }
}

void CorpusExplorerWidget::addItemsFromFolder()
{
    QPointer<CorpusRepository> repository = d->corpusRepositoriesManager->activeCorpusRepository();
    if (!repository) return;
    d->corporaTopLevelNode->startTreeProcessingCycle();
    ImportCorpusItemsWizard *wizard = new ImportCorpusItemsWizard(repository, this);
    wizard->exec(); // MODAL!
    d->corporaTopLevelNode->endTreeProcessingCycle();
}

void CorpusExplorerWidget::importMetadata()
{
    QPointer<CorpusRepository> repository = d->corpusRepositoriesManager->activeCorpusRepository();
    if (!repository) return;
    QFileDialog::Options options;
    QString selectedFilter;
    QString filename = QFileDialog::getOpenFileName(this, tr("Import Metadata Wizard - Select text file"), "",
                                                    tr("Text File (*.txt);;All Files (*)"),
                                                    &selectedFilter, options);
    if (filename.isEmpty()) return;
//    d->corporaTopLevelNode->startTreeProcessingCycle();
//    ImportMetadataWizard *wizard = new ImportMetadataWizard(filename, repository, this);
//    wizard->exec(); // MODAL!
//    d->corporaTopLevelNode->endTreeProcessingCycle();
}

void CorpusExplorerWidget::exportMetadata()
{
//    QPointer<Corpus> corpus = d->corporaManager->activeCorpus();
//    if (!corpus) return;
//    ExportMetadataWizard *wizard = new ExportMetadataWizard(corpus, this);
//    wizard->exec(); // MODAL!
}

void CorpusExplorerWidget::exportAnnotations()
{
//    QPointer<Corpus> corpus = d->corporaManager->activeCorpus();
//    if (!corpus) return;
//    ExportAnnotationsWizard *wizard = new ExportAnnotationsWizard(corpus, this);
//    wizard->exec(); // MODAL!
}

void CorpusExplorerWidget::checkMediaFiles()
{
//    QPointer<Corpus> corpus = d->corporaManager->activeCorpus();
//    if (!corpus) return;
//    CheckMediaFilesDialog *dialog = new CheckMediaFilesDialog(corpus, this);
//    dialog->exec();
//    delete dialog;
}

void CorpusExplorerWidget::splitCommunications()
{
//    QPointer<Corpus> corpus = d->corporaManager->activeCorpus();
//    if (!corpus) return;
//    SplitCommunicationsDialog *dialog = new SplitCommunicationsDialog(corpus, this);
//    dialog->exec();
//    delete dialog;
}

void CorpusExplorerWidget::createAnnotationsFromRecordings()
{
//    QPointer<Corpus> corpus = d->corporaManager->activeCorpus();
//    if (!corpus) return;
//    d->corporaTopLevelNode->startTreeProcessingCycle();
//    foreach (CorpusCommunication *com, corpus->communications()) {
//        if (!com) continue;
//        if (com->hasAnnotations()) continue;
//        foreach (CorpusRecording *rec, com->recordings()) {
//            if (!rec) continue;
//            CorpusAnnotation *annot = new CorpusAnnotation(rec->ID());
//            com->addAnnotation(annot);
//        }
//    }
//    d->corporaTopLevelNode->endTreeProcessingCycle();
}

void CorpusExplorerWidget::createSpeakersFromAnnotations()
{
//    QPointer<Corpus> corpus = d->corporaManager->activeCorpus();
//    if (!corpus) return;
//    d->corporaTopLevelNode->startTreeProcessingCycle();
//    foreach (CorpusCommunication *com, corpus->communications()) {
//        if (!com) continue;
//        foreach (CorpusAnnotation *annot, com->annotations()) {
//            if (!annot) continue;
//            QList<QString> speakerIDs = corpus->datastoreAnnotations()->getSpeakersInAnnotation(annot->ID());
//            foreach (QString speakerID, speakerIDs) {
//                if (!corpus->hasSpeaker(speakerID))
//                    corpus->addSpeaker(new CorpusSpeaker(speakerID));
//                if (!corpus->hasParticipation(com->ID(), speakerID))
//                    corpus->addParticipation(com->ID(), speakerID);
//            }
//        }
//    }
//    d->corporaTopLevelNode->endTreeProcessingCycle();
}

void CorpusExplorerWidget::cleanUpParticipationsFromAnnotations()
{
//    QPointer<Corpus> corpus = d->corporaManager->activeCorpus();
//    if (!corpus) return;

//    bool deleteSpeakers = false;
//    if (QMessageBox::question(this, tr("Clean up Speakers as well?"), tr("When a speaker does not participate in a Communication, based on its Annotations, "
//                              "the Participation will be deleted. Should the Speaker metadata also be deleted? (Caution: this operation cannot be undone!)"))
//            == QMessageBox::Yes) {
//        deleteSpeakers = true;
//    }
//    QSet<QString> allActiveSpeakerIDs;

//    d->corporaTopLevelNode->startTreeProcessingCycle();

//    QProgressDialog progressDialog(this);
//    progressDialog.setCancelButtonText(tr("&Cancel"));
//    progressDialog.setRange(0, corpus->communicationsCount());
//    progressDialog.setWindowTitle(tr("Analysing Corpus Data"));

//    int i = 1;
//    foreach (CorpusCommunication *com, corpus->communications()) {
//        progressDialog.setValue(i);
//        progressDialog.setLabelText(tr("Analysing Communication %1 of %2...")
//                                    .arg(i).arg(corpus->communicationsCount()));
//        ++i;
//        qApp->processEvents();

//        if (!com) continue;
//        QStringList activeSpeakerIDs;
//        foreach (CorpusAnnotation *annot, com->annotations()) {
//            if (!annot) continue;
//            QList<QString> speakerIDs = corpus->datastoreAnnotations()->getSpeakersActiveInAnnotation(annot->ID());
//            foreach (QString speakerID, speakerIDs)
//                if (!activeSpeakerIDs.contains(speakerID)) activeSpeakerIDs << speakerID;
//        }
//        allActiveSpeakerIDs.unite(QSet<QString>::fromList(activeSpeakerIDs));
//        QStringList inactiveSpeakerIDs;
//        foreach (QPointer<CorpusParticipation> participation, corpus->participationsForCommunication(com->ID())) {
//            if (!participation) continue;
//            if (!activeSpeakerIDs.contains(participation->speakerID()))
//                if (!inactiveSpeakerIDs.contains(participation->speakerID()))
//                    inactiveSpeakerIDs << participation->speakerID();
//        }
//        foreach (QString speakerID, inactiveSpeakerIDs) {
//            corpus->removeParticipation(com->ID(), speakerID);
//        }
//    }
//    if (deleteSpeakers) {
//        foreach (QString speakerID, corpus->speakerIDs()) {
//            if (!allActiveSpeakerIDs.contains(speakerID)) {
//                corpus->removeSpeaker(speakerID);
//            }
//        }
//    }
//    d->corporaTopLevelNode->endTreeProcessingCycle();
}

void CorpusExplorerWidget::attributesAndGroupings()
{
//    QPointer<CorpusObserver> obs = d->corporaManager->activeCorpusObserver();
//    if (!obs) return;
//    QPointer<Corpus> corpus = d->corporaManager->activeCorpus();
//    if (!corpus) return;
//    CorpusExplorerOptionsDialog *dialog = new CorpusExplorerOptionsDialog(corpus->metadataStructure(), this);
//    if (dialog->exec() != QDialog::Accepted) return;

//    obs->setCommunicationsGrouping(dialog->groupAttributesForCommunications());
//    obs->setSpeakersGrouping(dialog->groupAttributesForSpeakers());
}

// ==============================================================================================================================
// Metadata editors (primary and secondary) styling
// ==============================================================================================================================

void CorpusExplorerWidget::metadataEditorPrimaryStyleTree()
{
    d->metadataEditorPrimary->setEditorStyle(MetadataEditorWidget::TreeStyle);
}

void CorpusExplorerWidget::metadataEditorPrimaryStyleGroupBox()
{
    d->metadataEditorPrimary->setEditorStyle(MetadataEditorWidget::GroupBoxStyle);
}

void CorpusExplorerWidget::metadataEditorPrimaryStyleButton()
{
    d->metadataEditorPrimary->setEditorStyle(MetadataEditorWidget::ButtonStyle);
}

void CorpusExplorerWidget::metadataEditorSecondaryStyleTree()
{
    d->metadataEditorSecondary->setEditorStyle(MetadataEditorWidget::TreeStyle);
}

void CorpusExplorerWidget::metadataEditorSecondaryStyleGroupBox()
{
    d->metadataEditorSecondary->setEditorStyle(MetadataEditorWidget::GroupBoxStyle);
}

void CorpusExplorerWidget::metadataEditorSecondaryStyleButton()
{
    d->metadataEditorSecondary->setEditorStyle(MetadataEditorWidget::ButtonStyle);
}


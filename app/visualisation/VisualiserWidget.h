#ifndef VISUALISERWIDGET_H
#define VISUALISERWIDGET_H

#include <QPointer>
#include "SimpleVisualiserWidget.h"

class LayerTreeDialog;
class QFileSystemWatcher;
class QScrollArea;
class KeyReference;
class ActivityLog;

class CorpusCommunication;
class CorpusRecording;
class AnnotationTierModel;
class IntervalTier;
class AnnotationTierGroup;

class VisualiserWidget : public SimpleVisualiserWidget
{
    Q_OBJECT

public:
    VisualiserWidget(const QString &contextStringID = "Context.VisualisationMode",
                     bool withAudioOutput = true, bool withOSCSupport = true);
    virtual ~VisualiserWidget();

    void addLayerTimeInstantsFromIntevalTier(IntervalTier *tier);

signals:
    void canChangeSolo(bool);
    void canAlign(bool);

    void userScrolledToTime(RealTime);
    void playbackScrolledToTime(RealTime);

public slots:
    virtual bool commitData(bool mayAskUser);

    void goFullScreen();
    void endFullScreen();

    void newSessionWithCommunication(QPointer<CorpusCommunication> com);
    void addRecordingToSession(QPointer<CorpusRecording> rec);
    void addAnnotationPaneToSession(QMap<QString, QPointer<AnnotationTierGroup> > &tiers,
                                    const QList<QPair<QString, QString> > &attributes);
    void addProsogramPaneToSession(QPointer<CorpusRecording> rec);
    void addTappingDataPane(QMap<QString, QPointer<AnnotationTierGroup> > &tiers);
    void exportPDF(const QString &filename);

protected slots:
    virtual void importAudio();
    virtual void importMoreAudio();
    virtual void replaceMainAudio();
    virtual void openSomething();
    virtual void openLocation();
    virtual void applyTemplate();
    virtual void exportAudio();
    virtual void exportAudioData();
    virtual void importLayer();
    virtual void exportLayer();
    virtual void exportImage();
    virtual void saveSession();
    virtual void saveSessionAs();
    virtual void newSession();
    virtual void closeSession();

    virtual void toolNavigateSelected();
    virtual void toolSelectSelected();
    virtual void toolEditSelected();
    virtual void toolDrawSelected();
    virtual void toolEraseSelected();
    virtual void toolMeasureSelected();

    virtual void updateMenuStates();

    virtual void setInstantsNumbering();
    virtual void setInstantsCounterCycle();
    virtual void setInstantsCounters();
    virtual void resetInstantsCounters();

    virtual void propertyStacksResized(int);

    virtual void addPane();
    virtual void addLayer();
    virtual void addLayer(QString transformId);
    virtual void renameCurrentLayer();

    virtual void findTransform();

    virtual void paneDropAccepted(Pane *, QStringList);
    virtual void paneDropAccepted(Pane *, QString);

    virtual void setupRecentTransformsMenu();
    virtual void setupTemplatesMenu();

    virtual void playSpeedChanged(int);
    virtual void alignToggled();

    virtual void modelAdded(Model *);
    virtual void layerRemoved(Layer *);
    virtual void layerInAView(Layer *, bool);

    virtual void showLayerTree();

    virtual void handleOSCMessage(const OSCMessage &);
    virtual void midiEventsAvailable();
    virtual void playStatusChanged(bool);

    virtual void saveSessionAsTemplate();
    virtual void manageSavedTemplates();

protected:
    bool                     m_mainMenusCreated;
    QMenu                   *m_paneMenu;
    QMenu                   *m_layerMenu;
    QMenu                   *m_transformsMenu;
    QMenu                   *m_existingLayersMenu;
    QMenu                   *m_sliceMenu;
    QMenu                   *m_recentTransformsMenu;
    QMenu                   *m_templatesMenu;
    QMenu                   *m_rightButtonLayerMenu;
    QMenu                   *m_rightButtonTransformsMenu;

    QAction                 *m_deleteSelectedAction;
    QAction                 *m_manageTemplatesAction;
    QAction                 *m_zoomInAction;
    QAction                 *m_zoomOutAction;
    QAction                 *m_zoomFitAction;
    QAction                 *m_scrollLeftAction;
    QAction                 *m_scrollRightAction;
    QAction                 *m_showPropertyBoxesAction;

    QFrame                  *m_playControlsSpacer;
    int                      m_playControlsWidth;

    QPointer<LayerTreeDialog>   m_layerTreeDialog;

    QFileSystemWatcher      *m_templateWatcher;

    struct LayerConfiguration {
        LayerConfiguration(LayerFactory::LayerType _layer
                           = LayerFactory::Type("TimeRuler"),
                           Model *_source = 0,
                           int _channel = -1) :
            layer(_layer), sourceModel(_source), channel(_channel) { }
        LayerFactory::LayerType layer;
        Model *sourceModel;
        int channel;
    };

    typedef std::map<QAction *, LayerConfiguration> PaneActionMap;
    PaneActionMap m_paneActions;

    typedef std::map<QAction *, LayerConfiguration> LayerActionMap;
    LayerActionMap m_layerActions;

    typedef std::map<QAction *, TransformId> TransformActionMap;
    TransformActionMap m_transformActions;

    typedef std::map<TransformId, QAction *> TransformActionReverseMap;
    TransformActionReverseMap m_transformActionsReverse;

    typedef std::map<QAction *, Layer *> ExistingLayerActionMap;
    ExistingLayerActionMap m_existingLayerActions;
    ExistingLayerActionMap m_sliceActions;

    typedef std::map<ViewManager::ToolMode, QAction *> ToolActionMap;
    ToolActionMap m_toolActions;

    typedef std::map<QAction *, int> NumberingActionMap;
    NumberingActionMap m_numberingActions;

    virtual void setupMenus();
    virtual void setupFileMenu();
    virtual void setupEditMenu();
    virtual void setupAnnotationMenu();
    virtual void setupViewMenu();
    virtual void setupPaneAndLayerMenus();
    virtual void setupTransformsMenu();
    virtual void setupExistingLayersMenus();
    virtual void setupToolbars();

    virtual void addPane(const LayerConfiguration &configuration, QString text);

    virtual void closeEvent(QCloseEvent *e);
    virtual bool checkSaveModified();

    virtual void exportAudio(bool asData);

    virtual bool shouldCreateNewSessionForRDFAudio(bool *cancel);

    virtual void connectLayerEditDialog(ModelDataTableDialog *);
};

#endif // VISUALISERWIDGET_H

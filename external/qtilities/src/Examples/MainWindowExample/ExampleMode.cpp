/****************************************************************************
**
** Copyright (c) 2009-2010, Jaco Naude
**
** This file is part of Qtilities which is released under the following
** licensing options.
**
** Option 1: Open Source
** Under this license Qtilities is free software: you can
** redistribute it and/or modify it under the terms of the GNU General
** Public License as published by the Free Software Foundation, either
** version 3 of the License, or (at your option) any later version.
**
** Qtilities is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Qtilities. If not, see http://www.gnu.org/licenses/.
**
** Option 2: Commercial
** Alternatively, this library is also released under a commercial license
** that allows the development of closed source proprietary applications
** without restrictions on licensing. For more information on this option,
** please see the project website's licensing page:
** http://www.qtilities.org/licensing.html
**
** If you are unsure which license is appropriate for your use, please
** contact support@qtilities.org.
**
****************************************************************************/

#include "ExampleMode.h"
#include "ui_ExampleMode.h"

#include <QtilitiesProjectManagement>
#include <QtGui>

using namespace QtilitiesProjectManagement;

struct Qtilities::Examples::MainWindow::ExampleModePrivateData {
    ExampleModePrivateData() : initialized(false),
        side_viewer_dock(0),
        side_viewer_widget(0),
        main_splitter(0),
        actionShowDock(0),
        code_editor_widget(0),
        project_item(0),
        mode_icon_toggle(false) {}

    bool                            initialized;
    QDockWidget*                    side_viewer_dock;
    DynamicSideWidgetViewer*        side_viewer_widget;
    QSplitter*                      main_splitter;
    QAction*                        actionShowDock;
    CodeEditorWidget*               code_editor_widget;
    CodeEditorProjectItemWrapper*   project_item;
    QIcon                           mode_icon;
    bool                            mode_icon_toggle;
};

Qtilities::Examples::MainWindow::ExampleMode::ExampleMode(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ExampleMode)
{
    ui->setupUi(this);
    d = new ExampleModePrivateData;
    d->mode_icon = QIcon(qti_icon_QTILITIES_SYMBOL_64x64);

    // Create and dock the dynamic side widget viewer:
    d->side_viewer_dock = new QDockWidget(tr("Dynamic Widgets"));
    d->side_viewer_widget = new DynamicSideWidgetViewer(MODE_EXAMPLE_ID);
    connect(d->side_viewer_widget,SIGNAL(toggleVisibility(bool)),SLOT(toggleDock(bool)));
    d->side_viewer_dock->setWidget(d->side_viewer_widget);
    Qt::DockWidgetAreas allowed_areas = 0;
    allowed_areas |= Qt::LeftDockWidgetArea;
    allowed_areas |= Qt::RightDockWidgetArea;
    d->side_viewer_dock->setAllowedAreas(allowed_areas);
    addDockWidget(Qt::RightDockWidgetArea,d->side_viewer_dock);
    d->side_viewer_dock->installEventFilter(this);

    // Create text editor:
    d->code_editor_widget = new CodeEditorWidget;
    d->code_editor_widget->codeEditor()->setPlainText("This is an example mode with a code editor, and a set of dynamically loaded widgets in the dock window.");
    if (d->code_editor_widget->searchBoxWidget())
        d->code_editor_widget->searchBoxWidget()->setWidgetMode(SearchBoxWidget::SearchOnly);

    // Create project item:
    d->project_item = new CodeEditorProjectItemWrapper(d->code_editor_widget,this);
    // Add project item to global object pool:
    OBJECT_MANAGER->registerObject(d->project_item,QtilitiesCategory("Core::Project Items (IProjectItem)","::"));

    // Create splitters:
    if (ui->splitterParent->layout())
        delete ui->splitterParent->layout();

    // Create new layout with new widget:
    d->main_splitter = new QSplitter(Qt::Horizontal);
    QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight,ui->splitterParent);
    layout->addWidget(d->code_editor_widget);
    layout->setMargin(0);
    layout->setSpacing(0);

    // Actions:
    d->actionShowDock = new QAction(QIcon(),"Example Dynamic Dock Widget",this);
    d->actionShowDock->setCheckable(true);
    d->actionShowDock->setChecked(true);
    connect(d->side_viewer_widget,SIGNAL(toggleVisibility(bool)),d->actionShowDock,SLOT(setChecked(bool)));
    connect(d->actionShowDock,SIGNAL(triggered(bool)),SLOT(toggleDock(bool)));

    QList<int> context;
    context.push_front(CONTEXT_MANAGER->contextID(qti_def_CONTEXT_STANDARD));
    Command* command = ACTION_MANAGER->registerAction("ExampleMode.DynamicDockWidget",d->actionShowDock,context);
    command->setCategory(QtilitiesCategory(QApplication::applicationName()));
    bool existed;
    ActionContainer* view_menu = ACTION_MANAGER->createMenu(qti_action_VIEW,existed);
    view_menu->addAction(command);
}

bool Qtilities::Examples::MainWindow::ExampleMode::eventFilter(QObject *object, QEvent *event) {
    if (object == d->side_viewer_dock && event->type() == QEvent::Close) {
        d->actionShowDock->setChecked(false);
    }

    return false;
}

void Qtilities::Examples::MainWindow::ExampleMode::toggleDock(bool toggle) {
    if (toggle) {
        d->side_viewer_dock->show();
    } else {
        d->side_viewer_dock->hide();
    }
}

void Qtilities::Examples::MainWindow::ExampleMode::loadFile(const QString& file_name) {
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QString file_string = file.readAll();
    d->code_editor_widget->codeEditor()->setPlainText(file_string);
}

Qtilities::Examples::MainWindow::ExampleMode::~ExampleMode()
{
    delete ui;
    delete d;
}

QWidget* Qtilities::Examples::MainWindow::ExampleMode::modeWidget() {
    return this;
}

void Qtilities::Examples::MainWindow::ExampleMode::initializeMode() {
    if (d->initialized)
        return;

    // Check all objects in the global object pool.
    QMap<QString, ISideViewerWidget*> text_iface_map;
    QList<QObject*> widgets = OBJECT_MANAGER->registeredInterfaces("ISideViewerWidget");
    for (int i = 0; i < widgets.count(); i++) {
        ISideViewerWidget* side_viewer_widget = qobject_cast<ISideViewerWidget*> (widgets.at(i));
        if (side_viewer_widget) {
            text_iface_map[side_viewer_widget->widgetLabel()] = side_viewer_widget;
        }
    }

    d->side_viewer_widget->setIFaceMap(text_iface_map,true);
    d->side_viewer_widget->show();
    d->initialized = true;
}

QIcon Qtilities::Examples::MainWindow::ExampleMode::modeIcon() const {
    return d->mode_icon;
}

QString Qtilities::Examples::MainWindow::ExampleMode::modeName() const {
    return tr("Example Mode");
}

bool Examples::MainWindow::ExampleMode::setModeIcon(QIcon icon) {
    d->mode_icon = icon;
    emit modeIconChanged();
    return true;
}

void Qtilities::Examples::MainWindow::ExampleMode::handleNewFileSystemWidget(QWidget* widget) {
    // Connect it to the load file slot.
    SideWidgetFileSystem* file_system_widget = qobject_cast<SideWidgetFileSystem*> (widget);
    if (file_system_widget) {
        file_system_widget->toggleDoubleClickFileOpen(false);
        connect(file_system_widget,SIGNAL(requestEditor(QString)),SLOT(loadFile(QString)));
    }
}

void Examples::MainWindow::ExampleMode::toggleModeIcon() {
    if (d->mode_icon_toggle)
        setModeIcon(QIcon(qti_icon_QTILITIES_SYMBOL_64x64));
    else
        setModeIcon(QIcon(qti_icon_QTILITIES_SYMBOL_WHITE_64x64));
    d->mode_icon_toggle = !d->mode_icon_toggle;
}

void Qtilities::Examples::MainWindow::ExampleMode::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

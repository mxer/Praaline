#pragma once

#include <memory>

#include <QtWidgets/QWidget>

#include "PortType.h"
#include "NodeData.h"
#include "Serializable.h"

#include "Export.h"

class NODE_EDITOR_PUBLIC NodeDataModel
        : public QObject
        , public Serializable
{
    Q_OBJECT

public:

    virtual
    ~NodeDataModel() {}

    /// Caption is used in GUI
    virtual QString
    caption() const = 0;

    /// It is possible to hide caption in GUI
    virtual bool
    captionVisible() const { return true; }

    /// Name makes this model unique
    virtual QString
    name() const = 0;

    /// Function creates instances of a model stored in DataModelRegistry
    virtual std::unique_ptr<NodeDataModel>
    clone() const = 0;

public:

    virtual
    unsigned int
    nPorts(PortType portType) const = 0;

    virtual
    NodeDataType
    dataType(PortType portType, PortIndex portIndex) const = 0;

public:

    /// Triggers the algorithm
    virtual
    void
    setInData(std::shared_ptr<NodeData> nodeData,
              PortIndex port) = 0;

    virtual
    std::shared_ptr<NodeData>
    outData(PortIndex port) = 0;

    virtual
    QWidget *
    embeddedWidget() = 0;

    virtual
    bool
    resizable() const { return false; }

signals:

    void
    dataUpdated(PortIndex index);

    void
    dataInvalidated(PortIndex index);

    void
    computingStarted();

    void
    computingFinished();
};

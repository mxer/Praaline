#ifndef DATASETEDITORMODELS_H
#define DATASETEDITORMODELS_H

#include <QtCore/QObject>
#include "nodeeditor/NodeData.h"
#include "nodeeditor/NodeDataModel.h"
#include <memory>

/// The class can potentially encapsulate any user data which need to be transferred within the Node Editor graph
class MyNodeData : public NodeData
{
public:
    NodeDataType
    type() const override { return NodeDataType {"MyNodeData", "My Node Data"}; }
};

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class MyDataModel : public NodeDataModel
{
    Q_OBJECT
public:
    virtual ~MyDataModel() {}

public:
    QString caption() const override {
        return QString("My Data Model");
    }

    QString name() const override {
        return QString("MyDataModel");
    }

    std::unique_ptr<NodeDataModel> clone() const override {
        return std::make_unique<MyDataModel>();
    }

    void save(Properties &p) const override {
        p.put("model_name", MyDataModel::name());
    }

    unsigned int nPorts(PortType) const override {
        return 3;
    }

    NodeDataType dataType(PortType, PortIndex) const override {
        return MyNodeData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex) override {
        return std::make_shared<MyNodeData>();
    }

    void setInData(std::shared_ptr<NodeData>, int) override {
        //
    }

    QWidget * embeddedWidget() override { return nullptr; }
};


#endif // DATASETEDITORMODELS_H

#include <QDebug>
#include <QString>
#include <QList>
#include <QVariant>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "QSqlMigrator/QSqlMigrator.h"
using namespace QSqlMigrator;
using namespace QSqlMigrator::Structure;
using namespace QSqlMigrator::Commands;

#include "SQLSerialiserSystem.h"
#include "SQLSerialiserBase.h"
#include "SQLSerialiserMetadataStructure.h"

namespace Praaline {
namespace Core {

// static
bool SQLSerialiserMetadataStructure::initialiseMetadataStructureSchema(QSqlDatabase &db)
{
    Migrations::Migration initializeMetadataStructure;
    Table::Builder tableMetadataSections("praalineMetadataSections");
    tableMetadataSections
            << Column("objectType", SqlType(SqlType::Char, 1), "", Column::Primary)
            << Column("sectionID", SqlType(SqlType::VarChar, 32), "", Column::Primary)
            << Column("name", SqlType(SqlType::VarChar, 255))
            << Column("description", SqlType(SqlType::VarChar, 1024))
            << Column("itemOrder", SqlType::Integer);
    initializeMetadataStructure.add(new CreateTable(tableMetadataSections));
    Table::Builder tableMetadataAttributes("praalineMetadataAttributes");
    tableMetadataAttributes
            << Column("objectType", SqlType(SqlType::Char, 1), "", Column::Primary)
            << Column("attributeID", SqlType(SqlType::VarChar, 32), "", Column::Primary)
            << Column("sectionID", SqlType(SqlType::VarChar, 32))
            << Column("name", SqlType(SqlType::VarChar, 255))
            << Column("description", SqlType(SqlType::VarChar, 1024))
            << Column("datatype", SqlType(SqlType::VarChar, 32))
            << Column("length", SqlType::Integer)
            << Column("isIndexed", SqlType::Boolean)
            << Column("nameValueList", SqlType(SqlType::VarChar, 32))
            << Column("mandatory", SqlType::Boolean)
            << Column("itemOrder", SqlType::Integer);
    initializeMetadataStructure.add(new CreateTable(tableMetadataAttributes));

    bool result = SQLSerialiserBase::applyMigration("initializeMetadataStructure", &initializeMetadataStructure, db);
    if (result) {
        setPraalineSchemaVersion(1, db);
    }
    return result;
}

// static
bool SQLSerialiserMetadataStructure::upgradeMetadataStructureSchema(QSqlDatabase &db)
{
    int schemaVersion = getPraalineSchemaVersion(db);
    if (schemaVersion == 0) {
        if (db.tables().contains("praalineMetadataSections") && db.tables().contains("praalineMetadataAttributes")) {
            bool result = true;
            result = result && addColumnToTable("praalineMetadataSections", "itemOrder", DataType::Integer, db);
            result = result && addColumnToTable("praalineMetadataAttributes", "mandatory", DataType::Boolean, db);
            result = result && addColumnToTable("praalineMetadataAttributes", "itemOrder", DataType::Integer, db);
            if (result) setPraalineSchemaVersion(1, db);
        }
        else {
            return initialiseMetadataStructureSchema(db);
        }
    }
    return true;
}

bool createNewSchema(MetadataStructure *structure, CorpusObject::Type what, QSqlDatabase &db)
{
    Migrations::Migration initializeTable;
    QString tableName;
    ColumnList columns;
    if (what == CorpusObject::Type_Corpus) {
        tableName = "corpus";
        columns << Column("corpusID", SqlType(SqlType::VarChar, 64), "", Column::Primary) <<
                   Column("corpusName", SqlType(SqlType::VarChar, 128)) <<
                   Column("description", SqlType(SqlType::VarChar, 128));
    }
    else if (what == CorpusObject::Type_Communication) {
        tableName = "communication";
        columns << Column("communicationID", SqlType(SqlType::VarChar, 64), "", Column::Primary) <<
                   Column("corpusID", SqlType(SqlType::VarChar, 64)) <<
                   Column("communicationName", SqlType(SqlType::VarChar, 128));
    }
    else if (what == CorpusObject::Type_Speaker) {
        tableName = "speaker";
        columns << Column("speakerID", SqlType(SqlType::VarChar, 64), "", Column::Primary) <<
                   Column("corpusID", SqlType(SqlType::VarChar, 64)) <<
                   Column("speakerName", SqlType(SqlType::VarChar, 128));
    }
    else if (what == CorpusObject::Type_Recording) {
        tableName = "recording";
        columns << Column("recordingID", SqlType(SqlType::VarChar, 64), "", Column::Primary) <<
                   Column("communicationID", SqlType(SqlType::VarChar, 64)) <<
                   Column("recordingName", SqlType(SqlType::VarChar, 128)) <<
                   Column("filename", SqlType(SqlType::VarChar, 256)) <<
                   Column("format", SqlType(SqlType::VarChar, 32)) <<
                   Column("duration", SqlType::BigInt) <<
                   Column("channels", SqlType::SmallInt, "1") <<
                   Column("sampleRate", SqlType::Integer) <<
                   Column("precisionBits", SqlType::SmallInt) <<
                   Column("bitRate", SqlType::Integer) <<
                   Column("encoding", SqlType(SqlType::VarChar, 256)) <<
                   Column("fileSize", SqlType::BigInt) <<
                   Column("checksumMD5", SqlType(SqlType::VarChar, 64));
    }
    else if (what == CorpusObject::Type_Annotation) {
        tableName = "annotation";
        columns << Column("annotationID", SqlType(SqlType::VarChar, 64), "", Column::Primary) <<
                   Column("communicationID", SqlType(SqlType::VarChar, 64)) <<
                   Column("recordingID", SqlType(SqlType::VarChar, 64)) <<
                   Column("annotationName", SqlType(SqlType::VarChar, 64));
    }
    else if (what == CorpusObject::Type_Participation) {
        tableName = "participation";
        columns << Column("corpusID", SqlType(SqlType::VarChar, 64), "", Column::Primary) <<
                   Column("communicationID", SqlType(SqlType::VarChar, 64), "", Column::Primary) <<
                   Column("speakerID", SqlType(SqlType::VarChar, 64), "", Column::Primary) <<
                   Column("role", SqlType(SqlType::VarChar, 128));
    }
    else return false;

    foreach (MetadataStructureSection *section, structure->sections(what)) {
        foreach (MetadataStructureAttribute *attribute, section->attributes()) {
            columns << Column(attribute->ID(), SqlType(attribute->datatype()));
        }
    }

    Table::Builder table(tableName, columns);
    initializeTable.add(new CreateTable(table));
    return SQLSerialiserBase::applyMigration(QString("initialize_%1").arg(tableName),
                                             &initializeTable, db);
}

// static
bool SQLSerialiserMetadataStructure::createMetadataSchema(QPointer<MetadataStructure> structure, QSqlDatabase &db)
{
    db.transaction();
    if (!createNewSchema(structure, CorpusObject::Type_Corpus, db))         { db.rollback(); return false; }
    if (!createNewSchema(structure, CorpusObject::Type_Communication, db))  { db.rollback(); return false; }
    if (!createNewSchema(structure, CorpusObject::Type_Speaker, db))        { db.rollback(); return false; }
    if (!createNewSchema(structure, CorpusObject::Type_Recording, db))      { db.rollback(); return false; }
    if (!createNewSchema(structure, CorpusObject::Type_Annotation, db))     { db.rollback(); return false; }
    if (!createNewSchema(structure, CorpusObject::Type_Participation, db))  { db.rollback(); return false; }
    db.commit();
    return true;
}

// static
bool SQLSerialiserMetadataStructure::loadMetadataStructure(QPointer<MetadataStructure> structure, QSqlDatabase &db)
{
    if (!structure) return false;
    QSqlQuery q1(db), q2(db);
    q1.setForwardOnly(true);
    q1.prepare("SELECT * FROM praalineMetadataSections ORDER BY itemOrder");
    q2.setForwardOnly(true);
    q2.prepare("SELECT * FROM praalineMetadataAttributes WHERE sectionID = :sectionID ORDER BY itemOrder");
    //
    q1.exec();
    if (q1.lastError().isValid()) { qDebug() << q1.lastError(); return false; }
    structure->clearAll();
    while (q1.next()) {
        CorpusObject::Type objectType = SQLSerialiserSystem::corpusObjectTypeFromCode(q1.value("objectType").toString());
        MetadataStructureSection *section = new MetadataStructureSection(q1.value("sectionID").toString(),
                                                                         q1.value("name").toString(),
                                                                         q1.value("description").toString(),
                                                                         q1.value("itemOrder").toInt());
        q2.bindValue(":sectionID", section->ID());
        q2.exec();
        while (q2.next()) {
            MetadataStructureAttribute *attribute = new MetadataStructureAttribute();
            attribute->setID(q2.value("attributeID").toString());
            attribute->setName(q2.value("name").toString());
            attribute->setDescription(q2.value("description").toString());
            attribute->setDatatype(DataType(q2.value("datatype").toString()));
            attribute->setDatatype(DataType(attribute->datatype().base(), q2.value("length").toInt()));
            if (q2.value("isIndexed").toInt() > 0) attribute->setIndexed(true); else attribute->setIndexed(false);
            attribute->setNameValueList(q2.value("nameValueList").toString());
            attribute->setMandatory(q2.value("mandatory").toBool());
            attribute->setItemOrder(q2.value("itemOrder").toInt());
            attribute->setParent(section);
            section->addAttribute(attribute);
        }
        section->setParent(structure);
        structure->addSection(objectType, section);
    }
    return true;
}

// static
bool SQLSerialiserMetadataStructure::createMetadataSection(CorpusObject::Type type, QPointer<MetadataStructureSection> newSection, QSqlDatabase &db)
{
    if (!newSection) return false;
    QSqlQuery q(db);
    q.prepare("INSERT INTO praalineMetadataSections (objectType, sectionID, name, description, itemOrder) "
              "VALUES (:objectType, :sectionID, :name, :description, :itemOrder) ");
    q.bindValue(":objectType", SQLSerialiserSystem::corpusObjectCodeFromType(type));
    q.bindValue(":sectionID", newSection->ID());
    q.bindValue(":name", newSection->name());
    q.bindValue(":description", newSection->description());
    q.bindValue(":itemOrder", newSection->itemOrder());
    q.exec();
    if (q.lastError().isValid()) { qDebug() << q.lastError(); return false; }
    return true;
}

// static
bool SQLSerialiserMetadataStructure::updateMetadataSection(CorpusObject::Type type, QPointer<MetadataStructureSection> updatedSection, QSqlDatabase &db)
{
    if (!updatedSection) return false;
    QSqlQuery q(db);
    q.prepare("UPDATE praalineMetadataSections SET name=:name, description=:description, itemOrder=:itemOrder "
              "WHERE objectType=:objectType AND sectionID=:sectionID ");
    q.bindValue(":objectType", SQLSerialiserSystem::corpusObjectCodeFromType(type));
    q.bindValue(":sectionID", updatedSection->ID());
    q.bindValue(":name", updatedSection->name());
    q.bindValue(":description", updatedSection->description());
    q.bindValue(":itemOrder", updatedSection->itemOrder());
    q.exec();
    if (q.lastError().isValid()) { qDebug() << q.lastError(); return false; }
    return true;
}

// static
bool SQLSerialiserMetadataStructure::deleteMetadataSection(CorpusObject::Type type, const QString &sectionID, QSqlDatabase &db)
{
    if (sectionID.isEmpty()) return false;
    db.transaction();
    QSqlQuery qupd(db), qdel(db);
    qupd.prepare("UPDATE praalineMetadataAttributes SET sectionID = :defaultSectionID WHERE objectType=:objectType AND sectionID = :sectionID ");
    qupd.bindValue(":objectType", SQLSerialiserSystem::corpusObjectCodeFromType(type));
    qupd.bindValue(":sectionID", sectionID);
    qupd.bindValue(":defaultSectionID", SQLSerialiserSystem::defaultSectionID(type));
    qupd.exec();
    if (qupd.lastError().isValid()) { qDebug() << qupd.lastError(); db.rollback(); return false; }
    qdel.prepare("DELETE FROM praalineMetadataSections WHERE objectType=:objectType AND sectionID = :sectionID ");
    qdel.bindValue(":objectType", SQLSerialiserSystem::corpusObjectCodeFromType(type));
    qdel.bindValue(":sectionID", sectionID);
    qdel.exec();
    if (qdel.lastError().isValid()) { qDebug() << qdel.lastError(); db.rollback(); return false; }
    db.commit();
    return false;
}

// static
bool SQLSerialiserMetadataStructure::createMetadataAttribute(CorpusObject::Type type, QPointer<MetadataStructureAttribute> newAttribute,
                                                             QSqlDatabase &db)
{
    QString tableName = SQLSerialiserSystem::tableNameForCorpusObjectType(type);
    if (tableName.isEmpty()) return false;
    if (!newAttribute) return false;
    bool result = addColumnToTable(tableName, newAttribute->ID(), newAttribute->datatype(), db);
    if (!result) return false;
    QSqlQuery q(db);
    q.prepare("INSERT INTO praalineMetadataAttributes "
              "       (objectType, attributeID, sectionID, name, description, datatype, length, isIndexed, nameValueList, mandatory, itemOrder) "
              "VALUES (:objectType, :attributeID, :sectionID, :name, :description, :datatype, :length, :isIndexed, :nameValueList, :mandatory, :itemOrder) ");
    q.bindValue(":objectType", SQLSerialiserSystem::corpusObjectCodeFromType(type));
    q.bindValue(":attributeID", newAttribute->ID());
    q.bindValue(":sectionID", newAttribute->sectionID());
    q.bindValue(":name", newAttribute->name());
    q.bindValue(":description", newAttribute->description());
    q.bindValue(":datatype", newAttribute->datatypeString());
    q.bindValue(":length", newAttribute->datatypePrecision());
    q.bindValue(":isIndexed", (newAttribute->indexed()) ? 1 : 0);
    q.bindValue(":nameValueList", newAttribute->nameValueList());
    q.bindValue(":mandatory", (newAttribute->mandatory()) ? 1 : 0);
    q.bindValue(":itemOrder", newAttribute->itemOrder());
    q.exec();
    if (q.lastError().isValid()) { qDebug() << q.lastError(); return false; }
    return true;
}

// static
bool SQLSerialiserMetadataStructure::updateMetadataAttribute(CorpusObject::Type type, QPointer<MetadataStructureAttribute> updatedAttribute,
                                                             QSqlDatabase &db)
{
    if (!updatedAttribute) return false;
    QSqlQuery q(db);
    q.prepare("UPDATE praalineMetadataAttributes SET sectionID=:sectionID, name=:name, description=:description, "
              "nameValueList=:nameValueList, mandatory=:mandatory, itemOrder=:itemOrder "
              "WHERE objectType=:objectType AND attributeID=:attributeID");
    q.bindValue(":objectType", SQLSerialiserSystem::corpusObjectCodeFromType(type));
    q.bindValue(":attributeID", updatedAttribute->ID());
    q.bindValue(":sectionID", updatedAttribute->sectionID());
    q.bindValue(":name", updatedAttribute->name());
    q.bindValue(":description", updatedAttribute->description());
    q.bindValue(":nameValueList", updatedAttribute->nameValueList());
    q.bindValue(":mandatory", (updatedAttribute->mandatory()) ? 1 : 0);
    q.bindValue(":itemOrder", updatedAttribute->itemOrder());
    q.exec();
    if (q.lastError().isValid()) { qDebug() << q.lastError(); return false; }
    return true;
}

// static
bool SQLSerialiserMetadataStructure::renameMetadataAttribute(CorpusObject::Type type, const QString &attributeID, const QString &newAttributeID,
                                                             QSqlDatabase &db)
{
    QString tableName = SQLSerialiserSystem::tableNameForCorpusObjectType(type);
    if (tableName.isEmpty()) return false;
    bool result = renameColumn(tableName, attributeID, newAttributeID, db);
    if (!result) return false;
    QSqlQuery q(db);
    q.prepare("UPDATE praalineMetadataAttributes SET attributeID=:newAttributeID WHERE objectType=:objectType AND attributeID=:attributeID ");
    q.bindValue(":objectType", SQLSerialiserSystem::corpusObjectCodeFromType(type));
    q.bindValue(":attributeID", attributeID);
    q.bindValue(":newAttributeID", newAttributeID);
    q.exec();
    if (q.lastError().isValid()) { qDebug() << q.lastError(); return false; }
    return true;
}

// static
bool SQLSerialiserMetadataStructure::retypeMetadataAttribute(CorpusObject::Type type, const QString &attributeID,
                                                             const DataType &oldDataType, const DataType &newDataType, QSqlDatabase &db)
{
    QString tableName = SQLSerialiserSystem::tableNameForCorpusObjectType(type);
    if (tableName.isEmpty()) return false;
    bool result = retypeColumn(tableName, attributeID, oldDataType, newDataType, db);
    if (!result) return false;
    QSqlQuery q(db);
    q.prepare("UPDATE praalineMetadataAttributes SET datatype=:datatype, length=:length "
              "WHERE objectType=:objectType AND attributeID=:attributeID ");
    q.bindValue(":objectType", SQLSerialiserSystem::corpusObjectCodeFromType(type));
    q.bindValue(":attributeID", attributeID);
    q.bindValue(":datatype", newDataType.string());
    q.bindValue(":length", newDataType.precision());
    q.exec();
    if (q.lastError().isValid()) { qDebug() << q.lastError(); return false; }
    return true;
}

// static
bool SQLSerialiserMetadataStructure::deleteMetadataAttribute(CorpusObject::Type type, const QString &attributeID, QSqlDatabase &db)
{
    QString tableName = SQLSerialiserSystem::tableNameForCorpusObjectType(type);
    if (tableName.isEmpty()) return false;
    bool result = deleteColumn(tableName, attributeID, db);
    if (!result) return false;
    QSqlQuery q(db);
    q.prepare("DELETE FROM praalineMetadataAttributes WHERE objectType=:objectType AND attributeID=:attributeID ");
    q.bindValue(":objectType", SQLSerialiserSystem::corpusObjectCodeFromType(type));
    q.bindValue(":attributeID", attributeID);
    q.exec();
    if (q.lastError().isValid()) { qDebug() << q.lastError(); return false; }
    return true;
}

} // namespace Core
} // namespace Praaline
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

#include "ExtendedObserverTableModel.h"

Qtilities::Examples::Clipboard::ExtendedObserverTableModel::ExtendedObserverTableModel(QObject* parent) : ObserverTableModel(parent) {

}

QVariant Qtilities::Examples::Clipboard::ExtendedObserverTableModel::data(const QModelIndex &index, int role) const {
    if ((index.column() == columnCount() - 1) && (role == Qt::DisplayRole)) {
        return QString("Custom Data");
    } else {
        return ObserverTableModel::data(index,role);
    }
}

Qt::ItemFlags Qtilities::Examples::Clipboard::ExtendedObserverTableModel::flags(const QModelIndex &index) const {
    if (index.column() == columnCount() - 1) {
        Qt::ItemFlags item_flags = 0;
        item_flags |= Qt::ItemIsEnabled;
        item_flags |= Qt::ItemIsSelectable;
        return item_flags;
    } else {
        return ObserverTableModel::flags(index);
    }
}

QVariant Qtilities::Examples::Clipboard::ExtendedObserverTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if ((section == columnCount() - 1) && (role == Qt::DisplayRole)) {
        return QString("Custom Extension Header");
    } else {
        return ObserverTableModel::headerData(section,orientation,role);
    }
}

bool Qtilities::Examples::Clipboard::ExtendedObserverTableModel::setData(const QModelIndex &index, const QVariant &value, int role){
    if (index.column() == columnCount() - 1) {
        return false;
    } else {
        return ObserverTableModel::setData(index,value,role);
    }
}

int Qtilities::Examples::Clipboard::ExtendedObserverTableModel::rowCount(const QModelIndex &parent) const {
    return ObserverTableModel::rowCount(parent);
}

int Qtilities::Examples::Clipboard::ExtendedObserverTableModel::columnCount(const QModelIndex &parent) const {
    return ObserverTableModel::columnCount(parent) + 1;
}

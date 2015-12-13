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

#include "AvailablePropertyProvider.h"

#include <QtDebug>

using namespace Qtilities::Core;

QList<Qtilities::Core::PropertySpecification> Qtilities::Examples::ObserverWidgetExample::AvailablePropertyProvider::availableProperties() const {
    QList<Qtilities::Core::PropertySpecification> properties;

    // -----------------------------------------
    // Properties for all QObjects
    // -----------------------------------------
    PropertySpecification prop1("Global Property","Test description 1",QVariant::String);
    properties << prop1;

    // -----------------------------------------
    // Properties for Qtilities::CoreGui::TreeNode
    // -----------------------------------------
    PropertySpecification prop2("TreeNode Property","Test description 2",QVariant::Bool);
    prop2.d_read_only = true;
    prop2.d_removable = false;
    prop2.d_default_value = QVariant(true);
    prop2.d_class_name = "Qtilities::CoreGui::TreeNode";
    properties << prop2;

    // -----------------------------------------
    // Properties for Qtilities::CoreGui::TreeItem
    // -----------------------------------------
    PropertySpecification prop3("TreeItem Property","Test description 3",QVariant::Int);
    prop3.d_class_name = "Qtilities::CoreGui::TreeItem";
    properties << prop3;

    // -----------------------------------------
    // Properties for our ExampleObject
    // -----------------------------------------
    PropertySpecification example_prop1("Example Int Property","",QVariant::Int);
    example_prop1.d_class_name = "Qtilities::Examples::ObserverWidgetExample::ExampleObject";
    example_prop1.d_default_value = 10;
    example_prop1.d_add_during_construction = false;
    properties << example_prop1;

    PropertySpecification example_prop2("Example Bool Property","",QVariant::Bool);
    example_prop2.d_class_name = "Qtilities::Examples::ObserverWidgetExample::ExampleObject";
    example_prop2.d_default_value = true;
    example_prop2.d_add_during_construction = true;
    example_prop2.d_internal = true;
    properties << example_prop2;

    return properties;
}

QStringList Qtilities::Examples::ObserverWidgetExample::AvailablePropertyProvider::prohibitedCustomPropertiesClasses() const {
    QStringList classes;

    return classes;
}

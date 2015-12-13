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

#ifndef AVAILABLE_PROPERTY_PROVIDER_H
#define AVAILABLE_PROPERTY_PROVIDER_H

#include <IAvailablePropertyProvider>

#include <QObject>

namespace Qtilities {
    namespace Examples {
        namespace ObserverWidgetExample {
            using namespace Qtilities::Core;
            using namespace Qtilities::Core::Interfaces;

            /*!
            \class AvailablePropertyProvider
            \brief An example implementation of Qtilities::CoreGui::Interfaces::IAvailablePropertyProvider.
              */
            class AvailablePropertyProvider : public QObject, public IAvailablePropertyProvider
            {
                Q_OBJECT
                Q_INTERFACES(Qtilities::Core::Interfaces::IAvailablePropertyProvider)

                public:
                    AvailablePropertyProvider(QObject* parent = 0) : QObject(parent) {}
                    ~AvailablePropertyProvider() {}

                    // --------------------------------
                    // IObjectBase Implementation
                    // --------------------------------
                    QObject* objectBase() { return this; }
                    const QObject* objectBase() const { return this; }

                    // --------------------------------------------
                    // IAvailablePropertyProvider Implementation
                    // --------------------------------------------
                    QList<PropertySpecification> availableProperties() const;
                    QStringList prohibitedCustomPropertiesClasses() const;
            };
        }
    }
}

#endif // AVAILABLE_PROPERTY_PROVIDER_H

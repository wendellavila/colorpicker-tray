/*
 *   Copyright (C) 2009 Ivan Cukic <ivan.cukic+kde@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser/Library General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser/Library General Public License for more details
 *
 *   You should have received a copy of the GNU Lesser/Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "PlasmaServiceListModel.h"
#include <Plasma/Service>
#include <Plasma/DataEngineManager>
#include <KIcon>
#include <QDebug>

namespace Lancelot {

class PlasmaServiceListModel::Private {
public:
    Plasma::Service * service;
    Plasma::DataEngine * engine;
    Plasma::DataEngine::Data data;
};

PlasmaServiceListModel::PlasmaServiceListModel(QString dataEngine)
    : d(new Private())
{
    d->engine = Plasma::DataEngineManager::self()->loadEngine(dataEngine);

    if (!d->engine->sources().contains("lancelotModel")) {
        qDebug() << "PlasmaServiceListModel:"
                 << dataEngine
                 << "is not a lancelot model";
        d->engine = NULL;
        return;
    }

    d->engine->connectSource("data", this);
}

PlasmaServiceListModel::~PlasmaServiceListModel()
{
    delete d;
}

QString PlasmaServiceListModel::title(int index) const
{
    if (index < 0 || index >= size()) {
        return QString();
    }

    QStringList list = d->data["title"].toStringList();
    return list.at(index);
}

QString PlasmaServiceListModel::description(int index) const
{
    if (index < 0 || index >= size()) {
        return QString();
    }

    QStringList list = d->data["description"].toStringList();
    return list.at(index);
}

QIcon PlasmaServiceListModel::icon(int index) const
{
    if (index < 0 || index >= size()) {
        return QIcon();
    }

    QStringList list = d->data["icon"].toStringList();
    return KIcon(list.at(index));
}

bool PlasmaServiceListModel::isCategory(int index) const
{
    return false;
}

int PlasmaServiceListModel::size() const
{
    return d->data["title"].toStringList().size();
}

void PlasmaServiceListModel::dataUpdated(const QString & name,
        const Plasma::DataEngine::Data & data)
{
    if (name == "data") {
        d->data = data;
        emit updated();
    }
}


} // namespace Lancelot


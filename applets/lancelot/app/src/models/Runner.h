/*
 *   Copyright (C) 2007 Ivan Cukic <ivan.cukic+kde@gmail.com>
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

#ifndef LANCELOTAPP_MODELS_RUNNER_H_
#define LANCELOTAPP_MODELS_RUNNER_H_

#include "BaseModel.h"
#include <plasma/abstractrunner.h>

namespace Models {

class Runner : public BaseModel {
    Q_OBJECT
public:
    Runner(QString search = QString());
    virtual ~Runner();

    QString searchString();

public Q_SLOTS:
    void setSearchString(const QString & search);

protected:
    void activate(int index);
    void load();

private:
    QString m_searchString;
    Plasma::AbstractRunner::List m_runners;
    Plasma::RunnerContext m_context;
};

} // namespace Models

#endif /* LANCELOTAPP_MODELS_DEVICES_H_ */

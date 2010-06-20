/*
 *   Copyright 2009 by Giulio Camuffo <giuliocamuffo@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GRIDGROUP_H
#define GRIDGROUP_H

#include "abstractgroup.h"

namespace Plasma
{
    class Applet;
    class IconWidget;
}

class Position;
class Spacer;

struct LayoutItem {
    int row;
    int column;
    QGraphicsWidget *widget;

    bool operator==(const LayoutItem &i)
    {
        return (widget == i.widget);
    }
};

class GridGroup : public AbstractGroup
{
    Q_OBJECT
    public:
        explicit GridGroup(QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0);
        ~GridGroup();

        void saveChildGroupInfo(QGraphicsWidget *child, KConfigGroup group) const;
        void restoreChildGroupInfo(QGraphicsWidget *child, const KConfigGroup& group);
        QString pluginName() const;
        virtual void showDropZone(const QPointF &pos);

    protected:
        void layoutChild(QGraphicsWidget *child, const QPointF &pos);
        void resizeEvent(QGraphicsSceneResizeEvent *event);

    private slots:
        void onInitCompleted();
        void onAppletRemoved(Plasma::Applet *applet, AbstractGroup *group);
        void onSubGroupRemoved(AbstractGroup *subGroup, AbstractGroup *group);
        void onWidgetStartsMoving(QGraphicsWidget *widget);

    private:
        enum Orientation {
            Horizontal = 0,
            Vertical = 1
        };

        QGraphicsWidget *itemAt(int row, int column) const;
        QGraphicsWidget *removeItemAt(int row, int column, bool fillLayout = true);
        QGraphicsWidget *removeItemAt(const Position &position, bool fillLayout = true);
        void removeItem(QGraphicsWidget *item, bool fillLayout = true);
        void insertItemAt(QGraphicsWidget *item, int row, int column);
        void insertItemAt(QGraphicsWidget *item, int row, int column, Orientation orientation);
        Position itemPosition(QGraphicsWidget *item) const;
        int nearestBoundair(qreal pos, qreal size) const;
        void adjustCells();
        int columnCount() const;
        int rowCount() const;

        Spacer *m_spacer;
        QList<LayoutItem> m_layoutItems;

        friend class Spacer;
};

#endif // GRIDGROUP_H

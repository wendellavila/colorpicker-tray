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

#include "abstractgroup.h"
#include "abstractgroup_p.h"

#include <QtCore/QTimer>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsSceneResizeEvent>

#include <kservice.h>
#include <kservicetypetrader.h>

#include <Plasma/Containment>
#include <Plasma/FrameSvg>
#include <Plasma/Animator>
#include <Plasma/Animation>

#include "groupingcontainment.h"

Q_DECLARE_METATYPE(AbstractGroup *)

AbstractGroupPrivate::AbstractGroupPrivate(AbstractGroup *group)
    : q(group),
      destroying(false),
      containment(0),
      immutability(Plasma::Mutable),
      groupType(AbstractGroup::FreeGroup),
      interestingGroup(0),
      isMainGroup(false),
      m_mainConfig(0)
{
    background = new Plasma::FrameSvg(q);
    background->setImagePath("widgets/translucentbackground");
    background->setEnabledBorders(Plasma::FrameSvg::AllBorders);
}

AbstractGroupPrivate::~AbstractGroupPrivate()
{
    delete m_mainConfig;
}

KConfigGroup *AbstractGroupPrivate::mainConfigGroup()
{
    if (m_mainConfig) {
        return m_mainConfig;
    }

    KConfigGroup containmentGroup = containment->config();
    KConfigGroup groupsConfig = KConfigGroup(&containmentGroup, "Groups");
    KConfigGroup *mainConfig = new KConfigGroup(&groupsConfig, QString::number(id));

    return mainConfig;
}

void AbstractGroupPrivate::destroyGroup()
{
    mainConfigGroup()->deleteGroup();
    emit q->configNeedsSaving();

    Plasma::Animation *anim = Plasma::Animator::create(Plasma::Animator::DisappearAnimation, q);
    if (anim) {
    anim->setTargetWidget(q);
    anim->start();
    }

    q->scene()->removeItem(q);
    delete q;
}

void AbstractGroupPrivate::appletDestroyed(Plasma::Applet *applet)
{
    if (applets.contains(applet)) {
        kDebug()<<"removed applet"<<applet->id()<<"from group"<<id<<"of type"<<q->pluginName();

        applets.removeAll(applet);

        if (destroying && (q->children().count() == 0)) {
            destroyGroup();
            destroying = false;
        }

//         emit q->appletRemovedFromGroup(applet, q);
    }
}

void AbstractGroupPrivate::subGroupDestroyed(AbstractGroup *subGroup)
{
    if (subGroups.contains(subGroup)) {
        kDebug()<<"removed sub group"<<subGroup->id()<<"from group"<<id<<"of type"<<q->pluginName();

        subGroups.removeAll(subGroup);

        if (destroying && (q->children().count() == 0)) {
            destroyGroup();
            destroying = false;
        }

//         emit q->appletRemovedFromGroup(applet, q);
    }
}

void AbstractGroupPrivate::addChild(QGraphicsWidget *child, bool layoutChild)
{
    QPointF newPos = q->mapFromScene(child->scenePos());
    child->setParentItem(q);
    child->setProperty("group", QVariant::fromValue(q));

    if (layoutChild) {
        child->setPos(newPos);
        q->layoutChild(child, newPos);
        child->installEventFilter(q);
    } else {
//         child->installEventFilter(q);
    }

    emit q->configNeedsSaving();
}

void AbstractGroupPrivate::removeChild(QGraphicsWidget *child)
{
    QPointF newPos = child->scenePos();
    child->setParentItem(q->parentItem());
    child->setPos(child->parentItem()->mapFromScene(newPos));
}

//-----------------------------AbstractGroup------------------------------

AbstractGroup::AbstractGroup(QGraphicsItem *parent, Qt::WindowFlags wFlags)
             : QGraphicsWidget(parent, wFlags),
               d(new AbstractGroupPrivate(this))
{
    setAcceptHoverEvents(true);
    setAcceptDrops(true);
//     setContentsMargins(0, 10, 10, 10);
}

AbstractGroup::~AbstractGroup()
{
    emit groupDestroyed(this);

    delete d;
}

void AbstractGroup::setImmutability(Plasma::ImmutabilityType immutability)
{
    if (!isMainGroup()) {
        setFlag(QGraphicsItem::ItemIsMovable, immutability == Plasma::Mutable);
    }
    d->immutability = immutability;
}

Plasma::ImmutabilityType AbstractGroup::immutability() const
{
    return d->immutability;
}

uint AbstractGroup::id() const
{
    return d->id;
}

void AbstractGroup::addApplet(Plasma::Applet *applet, bool layoutApplet)
{
    if (!applet) {
        return;
    }

    if (applets().contains(applet)) {
        if (applet->parentItem() != this) {
            QPointF p(mapFromScene(applet->scenePos()));
            applet->setParentItem(this);
            applet->setPos(p);
        }

        return;
    }

    kDebug()<<"adding applet"<<applet->id()<<"in group"<<id()<<"of type"<<pluginName();

    QVariant pGroup = applet->property("group");
    if (pGroup.isValid()) {
        pGroup.value<AbstractGroup *>()->removeApplet(applet);
    }

    d->applets << applet;
    d->addChild(applet, layoutApplet);

    connect(applet, SIGNAL(appletDestroyed(Plasma::Applet*)),
            this, SLOT(appletDestroyed(Plasma::Applet*)));

    emit appletAddedInGroup(applet, this);
}

void AbstractGroup::addSubGroup(AbstractGroup *group, bool layoutGroup)
{
    if (!group) {
        return;
    }

    if (subGroups().contains(group)) {
        if (group->parentItem() != this) {
            QPointF p(mapFromScene(group->scenePos()));
            group->setParentItem(this);
            group->setPos(p);
        }

        return;
    }

    kDebug()<<"adding sub group"<<group->id()<<"in group"<<id()<<"of type"<<pluginName();

    QVariant pGroup = group->property("group");
    if (pGroup.isValid()) {
        pGroup.value<AbstractGroup *>()->removeSubGroup(group);
    }

    d->subGroups << group;
    d->addChild(group, layoutGroup);

    connect(group, SIGNAL(groupDestroyed(AbstractGroup*)),
            this, SLOT(subGroupDestroyed(AbstractGroup*)));

    emit subGroupAddedInGroup(group, this);
}

Plasma::Applet::List AbstractGroup::applets() const
{
    return d->applets;
}

QList<AbstractGroup *> AbstractGroup::subGroups() const
{
    return d->subGroups;
}

QList<QGraphicsWidget *> AbstractGroup::children() const
{
    QList<QGraphicsWidget *> list;
    foreach (Plasma::Applet *applet, d->applets) {
        list << applet;
    }
    foreach (AbstractGroup *group, d->subGroups) {
        list << group;
    }

    return list;
}

void AbstractGroup::removeApplet(Plasma::Applet *applet, AbstractGroup *newGroup)
{
    kDebug()<<"removing applet"<<applet->id()<<"from group"<<id()<<"of type"<<pluginName();

    d->applets.removeAll(applet);
    KConfigGroup appletConfig = applet->config().parent();
    KConfigGroup groupConfig(&appletConfig, QString("GroupInformation"));
    groupConfig.deleteGroup();
    emit configNeedsSaving();

    if (newGroup) {
        newGroup->addApplet(applet);
    } else {
        d->removeChild(applet);
    }

    emit appletRemovedFromGroup(applet, this);
}

void AbstractGroup::removeSubGroup(AbstractGroup *subGroup, AbstractGroup *newGroup)
{
    kDebug()<<"removing sub group"<<subGroup->id()<<"from group"<<id()<<"of type"<<pluginName();

    d->subGroups.removeAll(subGroup);
    KConfigGroup subGroupConfig = subGroup->config().parent();
    KConfigGroup groupConfig(&subGroupConfig, QString("GroupInformation"));
    groupConfig.deleteGroup();
    emit configNeedsSaving();

    if (newGroup) {
        newGroup->addSubGroup(subGroup);
    } else {
        d->removeChild(subGroup);
    }

    emit subGroupRemovedFromGroup(subGroup, this);
}

void AbstractGroup::destroy()
{
    kDebug()<<"destroying group"<<id()<<"of type"<<pluginName();

    if (applets().count() == 0) {
        d->destroyGroup();
        return;
    }

    d->destroying = true;

    foreach (AbstractGroup *group, subGroups()) {
        group->destroy();
    }
    foreach (Plasma::Applet *applet, applets()) {
        applet->destroy();
    }
}

QGraphicsView *AbstractGroup::view() const
{
    // It's assumed that we won't be visible on more than one view here.
    // Anything that actually needs view() should only really care about
    // one of them anyway though.
    if (!scene()) {
        return 0;
    }

    QGraphicsView *found = 0;
    QGraphicsView *possibleFind = 0;
    //kDebug() << "looking through" << scene()->views().count() << "views";
    foreach (QGraphicsView *view, scene()->views()) {
        //kDebug() << "     checking" << view << view->sceneRect()
        //         << "against" << sceneBoundingRect() << scenePos();
        if (view->sceneRect().intersects(sceneBoundingRect()) ||
            view->sceneRect().contains(scenePos())) {
            //kDebug() << "     found something!" << view->isActiveWindow();
            if (view->isActiveWindow()) {
                found = view;
            } else {
                possibleFind = view;
            }
        }
    }

    return found ? found : possibleFind;
}

GroupingContainment *AbstractGroup::containment() const
{
    return d->containment;
}

KConfigGroup AbstractGroup::config() const
{
    KConfigGroup config = KConfigGroup(d->mainConfigGroup(), "Configuration");

    return config;
}

void AbstractGroup::save(KConfigGroup &group) const
{
    if (!group.isValid()) {
        group = *d->mainConfigGroup();
    }

    group.writeEntry("zvalue", zValue());
}

void AbstractGroup::restore(KConfigGroup &group)
{
    qreal z = group.readEntry("zvalue", 0);

    if (z > 0) {
        setZValue(z);
    }
}

void AbstractGroup::showDropZone(const QPointF& pos)
{
    Q_UNUSED(pos)

    //base implementation does nothing
}

void AbstractGroup::raise()
{
    containment()->raise(this);
}

void AbstractGroup::setGroupType(AbstractGroup::GroupType type)
{
    d->groupType = type;
}

AbstractGroup::GroupType AbstractGroup::groupType() const
{
    return d->groupType;
}

void AbstractGroup::setIsMainGroup(bool isMainGroup)
{
    d->isMainGroup = isMainGroup;
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setZValue(0);
kDebug()<<flags();
}

bool AbstractGroup::isMainGroup() const
{
    return d->isMainGroup;
}

bool AbstractGroup::eventFilter(QObject *obj, QEvent *event)
{
    AbstractGroup *group = qobject_cast<AbstractGroup *>(obj);
    Plasma::Applet *applet = qobject_cast<Plasma::Applet *>(obj);

    QGraphicsWidget *widget = 0;
    if (applet) {
        widget = applet;
    } else if (group) {
        widget = group;
    }
//     kDebug()<<d->subGroups.count();
    if (widget) {
//         switch (event->type()) {
//             case QEvent::GraphicsSceneMove:
//                 foreach (AbstractGroup *parentGroup, d->subGroups) {
//                     if (!parentGroup->children().contains(widget) && (parentGroup != group)) {
//                         QRectF rect = parentGroup->contentsRect();
//                         rect.translate(parentGroup->pos());
//
//                         QRectF intersected(rect.intersected(widget->geometry()));
//                         kDebug()<<this<<intersected;
//                         if (intersected.isValid()) {
//                             parentGroup->showDropZone(mapToItem(parentGroup, intersected.center()));
//                             d->interestingGroup = parentGroup;
//                             break;
//                         } else {
//                             if (parentGroup == d->interestingGroup) {
//                                 parentGroup->showDropZone(QPointF());
//                                 d->interestingGroup = 0;
//                             }
//                         }
//                     }
//                 }
//                 if (children().contains(widget) && !contentsRect().contains(widget->geometry()) && !isMainGroup()) {
//                     AbstractGroup *parentGroup = qgraphicsitem_cast<AbstractGroup *>(parentItem());
//                     if (applet) {
//                         removeApplet(applet, parentGroup);
//                     } else {
//                         removeSubGroup(group, parentGroup);
//                     }
//                 }
//                 break;

//             case QEvent::GraphicsSceneMouseRelease:
//                 if (d->interestingGroup) {
//                     if (applet) {
//                         d->applets.removeAll(applet);
//                         d->interestingGroup->addApplet(applet);
//                     } else if (!group->isAncestorOf(d->interestingGroup)) {
//                         d->subGroups.removeAll(group);
//                         d->interestingGroup->addSubGroup(group);
//                     }
//                     widget->removeEventFilter(this);
//                     d->interestingGroup = 0;
//                 }
//                 break;
//
//             default:
//                 break;
//         }
    }

    return QGraphicsWidget::eventFilter(obj, event);
}

void AbstractGroup::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
    showDropZone(event->pos());
}

void AbstractGroup::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
    event->ignore();

    QPointF pos(event->pos());
    if (pos.y() < 1 || abs(pos.y() - size().height()) < 2 ||
        pos.x() < 1 || abs(pos.x() - size().width()) < 2) {
        showDropZone(QPointF());
    }
}

void AbstractGroup::dropEvent(QGraphicsSceneDragDropEvent *event)
{
    scene()->sendEvent(d->containment, event);
}

void AbstractGroup::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    d->background->resizeFrame(event->newSize());

    emit geometryChanged();
}

int AbstractGroup::type() const
{
    return Type;
}

QVariant AbstractGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && isMainGroup()) {
        return pos();
    }

    return QGraphicsWidget::itemChange(change, value);
}

void AbstractGroup::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (isMainGroup()) {
        return;
    }

//     if (d->background && (d->containment->formFactor() != Plasma::Vertical) &&
//                          (d->containment->formFactor() != Plasma::Horizontal)) {
        d->background->paintFrame(painter);
//     } else {
        //TODO draw a halo, something
//     }
}

#include "abstractgroup.moc"
/***************************************************************************
 *   Copyright (C) 2007 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2008-2010 Matthias Fuchs <mat69@gmx.net>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "comicupdater.h"
#include "comicmodel.h"

#include <QtCore/QTimer>
#include <QtGui/QSortFilterProxyModel>

#include <KConfigDialog>
#include <KNS3/DownloadDialog>
#include <KNS3/DownloadManager>

ComicUpdater::ComicUpdater( QObject *parent )
  : QObject( parent ),
    mDownloadManager( 0 ),
    mUpdateIntervall( 3 ),
    m_updateTimer( 0 )
{
}

ComicUpdater::~ComicUpdater()
{
}

void ComicUpdater::init(const KConfigGroup &group)
{
    mGroup = group;
}

void ComicUpdater::load()
{
    //check when the last update happened and update if necessary
    mUpdateIntervall = mGroup.readEntry( "updateInterval", 3 );
    if ( mUpdateIntervall ) {
        mLastUpdate = mGroup.readEntry( "lastUpdate", QDateTime() );
        checkForUpdate();
    }
}

void ComicUpdater::save()
{
    mGroup.writeEntry( "updateInterval", mUpdateIntervall );
}

void ComicUpdater::setInterval( int interval )
{
    mUpdateIntervall = interval;
}

int ComicUpdater::interval() const
{
    return mUpdateIntervall;
}

void ComicUpdater::checkForUpdate()
{
    //start a timer to check each hour, if KNS3 should look for updates
    if ( !m_updateTimer ) {
        m_updateTimer = new QTimer(this);
        connect(m_updateTimer, SIGNAL(timeout()), this, SLOT(checkForUpdate()));
        m_updateTimer->start( 1 * 60 * 60 * 1000 );
    }

    if ( !mLastUpdate.isValid() || ( mLastUpdate.addDays( mUpdateIntervall ) < QDateTime::currentDateTime() ) ) {
        mGroup.writeEntry( "lastUpdate", QDateTime::currentDateTime() );
        downloadManager()->checkForUpdates();
    }
}

void ComicUpdater::slotUpdatesFound( const KNS3::Entry::List &entries )
{
    for ( int i = 0; i < entries.count(); ++i ) {
        downloadManager()->installEntry( entries[ i ] );
    }
}

KNS3::DownloadManager *ComicUpdater::downloadManager()
{
    if ( !mDownloadManager ) {
        mDownloadManager = new KNS3::DownloadManager( "comic.knsrc", this );
        connect(mDownloadManager, SIGNAL(searchResult(KNS3::Entry::List)), this, SLOT(slotUpdatesFound(KNS3::Entry::List)));
    }

    return mDownloadManager;
}

#include "comicupdater.moc"

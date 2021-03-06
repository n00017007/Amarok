/****************************************************************************************
 * Copyright (c) 2011 Stefan Derkits <stefan@derkits.at>                                *
 * Copyright (c) 2011 Christian Wagner <christian.wagner86@gmx.at>                      *
 * Copyright (c) 2011 Felix Winter <ixos01@gmail.com>                                   *
 * Copyright (c) 2011 Lucas Lira Gomes <x8lucas8x@gmail.com>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef GPODDERPODCASTPROVIDER_H
#define GPODDERPODCASTPROVIDER_H

#include "core/podcasts/PodcastProvider.h"
#include "core/podcasts/PodcastReader.h"
#include "GpodderPodcastMeta.h"
#include <mygpo-qt/ApiRequest.h>
#include <mygpo-qt/EpisodeActionList.h>
#include "playlistmanager/file/KConfigSyncRelStore.h"
#include "playlistmanager/PlaylistManager.h"

#include <KDialog>
#include <KLocale>

#include <QCheckBox>
#include <QPair>
#include <QQueue>

using namespace mygpo;

class QAction;

namespace Podcasts
{
class GpodderProvider : public PodcastProvider
{
    Q_OBJECT
public:
    GpodderProvider( const QString& username, const QString& devicename, ApiRequest *apiRequest );
    ~GpodderProvider();

    bool possiblyContainsTrack( const KUrl &url ) const;
    Meta::TrackPtr trackForUrl( const KUrl &url );
    /** Special function to get an episode for a given guid.
     *
     * note: this functions is required because KUrl does not preserve every possible guids.
     * This means we can not use trackForUrl().
     * Problematic guids contain non-latin characters, percent encoded parts, capitals, etc.
     */
    virtual PodcastEpisodePtr episodeForGuid( const QString &guid );

    virtual void addPodcast( const KUrl &url );

    virtual Podcasts::PodcastChannelPtr addChannel( Podcasts::PodcastChannelPtr channel );
    virtual Podcasts::PodcastEpisodePtr addEpisode( Podcasts::PodcastEpisodePtr episode );

    virtual Podcasts::PodcastChannelList channels();

    // PlaylistProvider methods
    virtual QString prettyName() const;
    virtual KIcon icon() const;
    virtual Playlists::PlaylistList playlists();
    virtual void completePodcastDownloads();

    /** Copy a playlist to the provider.
    */
    virtual Playlists::PlaylistPtr addPlaylist( Playlists::PlaylistPtr playlist );
    QList<QAction *> channelActions( PodcastChannelList episodes );
    QList<QAction *> playlistActions( Playlists::PlaylistPtr playlist );

signals:
    //PlaylistProvider signals
    void updated();
    void playlistAdded( Playlists::PlaylistPtr playlist );
    void playlistRemoved( Playlists::PlaylistPtr playlist );

private slots:
    void requestDeviceUpdates();
    void deviceUpdatesFinished();
    void continueDeviceUpdatesFinished();
    void deviceUpdatesParseError();
    void deviceUpdatesRequestError( QNetworkReply::NetworkError error );

    void requestEpisodeActionsInCascade();
    void episodeActionsInCascadeFinished();
    void episodeActionsInCascadeParseError();
    void episodeActionsInCascadeRequestError( QNetworkReply::NetworkError error );

    void timerGenerateEpisodeAction();
    void timerSynchronizeStatus();
    void timerSynchronizeSubscriptions();
    void timerPrepareToSyncPodcastStatus();

    void slotRemoveChannels();
    void synchronizeStatusParseError();
    void synchronizeStatusRequestError( QNetworkReply::NetworkError error );
    void slotSuccessfulStatusSynchronisation();
    void slotSuccessfulSubscriptionSynchronisation();

    void slotSyncPlaylistAdded( Playlists::PlaylistPtr playlist );
    void slotSyncPlaylistRemoved( Playlists::PlaylistPtr playlist );

    void slotPaused();
    void slotTrackChanged( Meta::TrackPtr track );
    void slotTrackPositionChanged( qint64 position, bool userSeek );

    void requestUrlResolve( GpodderPodcastChannelPtr channel );
    void urlResolvePermanentRedirection ( KIO::Job *job, const KUrl &fromUrl,
            const KUrl &toUrl );
    void urlResolveFinished( KJob * );

private:
    ApiRequest *m_apiRequest;
    const QString m_username;
    const QString m_devicename;
    PodcastChannelList m_channels;
    KIO::TransferJob *m_resolveUrlJob;

    AddRemoveResultPtr m_addRemoveResult;
    DeviceUpdatesPtr m_deviceUpdatesResult;
    AddRemoveResultPtr m_episodeActionsResult;
    EpisodeActionListPtr m_episodeActionListResult;

    qulonglong m_timestampStatus;
    qulonglong m_timestampSubscription;

    qulonglong subscriptionTimestamp();
    void setSubscriptionTimestamp( qulonglong newTimestamp );

    void removeChannel( const QUrl &url );
    void createPlayStatusBookmark();

    void synchronizeStatus();
    void synchronizeSubscriptions();
    void updateLocalPodcasts( const QList< QPair<QUrl,QUrl> > updatedUrls );

    KConfigGroup gpodderActionsConfig() const;
    void loadEpisodeActions();
    void saveEpisodeActions();

    QAction *m_removeAction; //remove a subscription
    QList<QUrl> m_addList;
    QList<QUrl> m_removeList;

    QMap<KUrl,KUrl> m_redirectionUrlMap;
    QQueue<QUrl> m_channelsToRequestActions;
    QQueue<GpodderPodcastChannelPtr> m_channelsToAdd;
    QMap<QUrl,EpisodeActionPtr> m_episodeStatusMap;
    QMap<QUrl,EpisodeActionPtr> m_uploadEpisodeStatusMap;
    QMap<KIO::TransferJob *,GpodderPodcastChannelPtr> m_resolvedPodcasts;

    QTimer *m_timerGenerateEpisodeAction;
    QTimer *m_timerSynchronizeStatus;
    QTimer *m_timerSynchronizeSubscriptions;

    Meta::TrackPtr m_trackToSyncStatus;
};

}

#endif

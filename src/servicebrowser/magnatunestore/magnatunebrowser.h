/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 

#ifndef AMAROKMAGNATUNEBROWSER_H
#define AMAROKMAGNATUNEBROWSER_H


#include "amarok.h"
//#include "magnatunepurchasedialog.h"
//#include "magnatunepurchasehandler.h"
//#include "magnatuneredownloadhandler.h"
#include "magnatunexmlparser.h"
#include "magnatunedatabasehandler.h"
//#include "../databasedrivencontentmodel.h"
#include "../servicebase.h"
#include "servicesqlcollection.h"

#include <kio/job.h>
#include <kio/jobclasses.h>

#include <QCheckBox>
#include <QComboBox>
#include <khbox.h>
#include <q3popupmenu.h>
#include <QPushButton>
#include <kvbox.h>



/**
Amarok browser that displays all the music available at magnatune.com and makes it available for previewing and purchasing.
Implemented as a singleton

@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MagnatuneBrowser : public ServiceBase
{
    Q_OBJECT

public:
    
     /**
     * Constructor
     */
    MagnatuneBrowser( const char *name );
    /**
     * Destructor
     */
    ~MagnatuneBrowser() { }

     /**
     * Do not do expensive initializations before we are actually shown
     */
    void polish();
   // bool updateContextView();

private slots:

    /**
     * Slot for recieving aboutToShow signals from the right click menu.
     * Inserts items in the menu based on the type of the current selection
     */
    void menuAboutToShow();

    /**
     * Slot called when the purchase album button is clicked. Starts a purchase
     */
   // void purchaseButtonClicked();

    /**
     * Slot for recieving notification from the right click menu that the user
     * has chosen to purchase an album. Starts a purchase
     */
   // void purchaseSelectedAlbum();

    /**
     * Slot for recieving notification from the right click menu that the user 
     * has chosen to purchase the album contining the selected track.
     * Starts a purchase
     */
    //void purchaseAlbumContainingSelectedTrack();

     /**
     * Slot for recieving notification from the right click menu that the user 
     * has selected "add to playlist" for the currently selected item,
     */
    //void addSelectionToPlaylist();

    /**
     * Slot for recieving notification that the user has double clicked an 
     * item in the list view. Ads item to playlist.
     * @param item The item that was double clicked
     */
   // void itemExecuted( QListWidgetItem * item);

    /**
     * Slot for recieving notification when a new item in the list is selected.
     * Adds the corrosponding artist or album info to the info view (if visible)
     * @param item The selected item
     */
    //void selectionChanged( QListWidgetItem * item);

    /**
     * Slot for recieving notifications about right clicks in the list view.
     * if selection is valid the popup menu is shown
     * @param item The item that was right clicked
     * @param pos The position of the cursor at the time of thre right click
     * @param column The column of the item that was right clicked (unused)
     */
    //void showPopupMenu( QListWidgetItem * item, const QPoint & pos, int column );

    /**
     * Slot for recieving notification that the update button has been clicked.
     */
    void updateButtonClicked();

    /**
     * Toggles the info area on and off
     * @param show If true the info box is shown, if false it is hidden
     */
    //void showInfo(bool show);

    /**
     * Slot for recieving notification when the Magnatune xml file has been downloaded. 
     * Triggers a parse of the file to get the info added to the databse
     * @param downLoadJob The calling download Job
     */
    void listDownloadComplete( KJob* downLoadJob);

    /**
     * Slot for catching cancelled list downloads
     */
    void listDownloadCancelled();

    /**
     * Slot called when the genre combo box selection changes. Triggers an update of the list view.
     */
   // void genreChanged( QString genre);

    /**
     * Slot called when the parsing of the Magnatuin xml file is completed.
     * Triggers an update of the list view and the genre combo box
     */
    void doneParsing();

    /**
     * Starts the process of redownloading a previously bought album
     */
    //void processRedownload();

    /**
     * Slot for recieving notifications of completed purchase operations
     * @param success Was the operation a success?
     */
    //void purchaseCompleted( bool success );

  
    /**
     * Adds all tracks with a common mood to the playlist
     * @param mood The mood of the tracks to add
     */
    //void addMoodyTracksToPlaylist( QString mood );

private:
    /**
     * Helper function that initializes the button panel below the list view
     */
    void initBottomPanel();

    /**
     * Helper function that initializes the genre selection panel above the list view
     */
    void initTopPanel();

    /**
     * Starts downloading an updated track list xml file from 
     * http://magnatune.com/info/album_info.xml
     * @return Currently always returns true
     */
    bool updateMagnatuneList();

    /**
     * Adds a magnatune preview track to the playlist.
     * @param item The track to add
     */
    void addTrackToPlaylist ( MagnatuneTrack  *item );

    /**
     * Adds all preview tracks on a magnatune album to the playlist
     * @param item The album to add
     */
    //void addAlbumToPlaylist ( MagnatuneAlbum  *item );

    /**
     * Adds all preview tracks on all albums by a given artist to the playlist
     * @param item the artist to add
     */
    //void addArtistToPlaylist( MagnatuneArtist *item );

    /**
     * Clears the genre combo box and inserts all genres from the database
     */
    //void updateGenreBox();


    static MagnatuneBrowser *s_instance;

    //MagnatuneListView         *m_listView;
    //MagnatuneArtistInfoBox    *m_artistInfobox;
    QString                    m_currentInfoUrl;
    QMenu                *m_popupMenu;
    //MagnatunePurchaseHandler  *m_purchaseHandler;
    //MagnatuneRedownloadHandler *m_redownloadHandler;
    //MagnatuneDatabaseHandler * m_dbHandler;

    QPushButton *m_advancedFeaturesButton;
    QPushButton *m_updateListButton;
    QPushButton *m_purchaseAlbumButton;
    QPushButton *m_showInfoToggleButton;

    QComboBox   *m_genreComboBox;
    bool         m_purchaseInProgress;

    //MagnatuneContentModel * m_model;
   

    KIO::FileCopyJob * m_listDownloadJob;

    ServiceSqlCollection * m_collection;

   // DatabaseDrivenContentItem * m_currentlySelectedItem;
};


#endif

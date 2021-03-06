 /****************************************************************************************
 * Copyright (c) 2002-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2002 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2002 Gabor Lehel <illissius@gmail.com>                                 *
 * Copyright (c) 2002 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Artur Szymiec <artur.szymiec@gmail.com>                           *
 * Copyright (c) 2010 Téo Mrnjavac <teo@kde.org>                                        *
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

#define DEBUG_PREFIX "MainWindow"

#include "MainWindow.h"

#include "aboutdialog/ExtendedAboutDialog.h"
#include "ActionClasses.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "EngineController.h" //for actions in ctor
#include "KNotificationBackend.h"
#include "Osd.h"
#include "PaletteHandler.h"
#include "amarokconfig.h"
#include "aboutdialog/OcsData.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "amarokurls/BookmarkManager.h"
#include "browsers/collectionbrowser/CollectionWidget.h"
#include "browsers/filebrowser/FileBrowser.h"
#include "browsers/playlistbrowser/PlaylistBrowser.h"
#include "browsers/servicebrowser/ServiceBrowser.h"
#include "context/ContextDock.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "covermanager/CoverManager.h" // for actions
#include "dialogs/DiagnosticDialog.h"
#include "dialogs/EqualizerDialog.h"
#include "likeback/LikeBack.h"
#include "moodbar/MoodbarManager.h"
#include "network/NetworkAccessManagerProxy.h"
#ifdef DEBUG_BUILD_TYPE
#include "network/NetworkAccessViewer.h"
#endif // DEBUG_BUILD_TYPE
#include "playlist/layouts/LayoutConfigAction.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/PlaylistDock.h"
#include "playlist/ProgressiveSearchWidget.h"
#include "playlistmanager/file/PlaylistFileProvider.h"
#include "playlistmanager/PlaylistManager.h"
#include "PodcastCategory.h"
#include "services/scriptable/ScriptableService.h"
#include "toolbar/SlimToolbar.h"
#include "toolbar/MainToolbar.h"
#include "SvgHandler.h"
#include "PluginManager.h"
//#include "mediabrowser.h"

#include <KAction>          //m_actionCollection
#include <KActionCollection>
#include <KApplication>     //kapp
#include <KFileDialog>      //openPlaylist()
#include <KInputDialog>     //slotAddStream()
#include <KMessageBox>
#include <KLocale>
#include <KMenu>
#include <KMenuBar>
#include <KBugReport>
#include <KStandardAction>
#include <KStandardDirs>
#include <KWindowSystem>
#include <kabstractfilewidget.h>

#include <plasma/plasma.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QDockWidget>
#include <QList>
#include <QStyle>
#include <QVBoxLayout>
#include <QDebug>
#include <QDeclarativeView>
#include <iostream>

#ifdef Q_WS_X11
#include <fixx11h.h>
#endif

#ifdef Q_WS_MAC
#include "mac/GrowlInterface.h"
#endif

#define AMAROK_CAPTION I18N_NOOP( "Amarok" )

extern OcsData ocsData;

QWeakPointer<MainWindow> MainWindow::s_instance;

namespace The {
    MainWindow* mainWindow() { return MainWindow::s_instance.data(); }
}

MainWindow::MainWindow()
    : KMainWindow( 0 )
    , m_showMenuBar( 0 )
    , m_lastBrowser( 0 )
    , m_waitingForCd( false )
{
    DEBUG_BLOCK

    setObjectName( "MainWindow" );
    s_instance = this;

#ifdef Q_WS_MAC
    GrowlInterface* growl = new GrowlInterface( qApp->applicationName() );
#endif

    PERF_LOG( "Instantiate Collection Manager" )
    CollectionManager::instance();
    PERF_LOG( "Started Collection Manager instance" )

    /* The PluginManager needs to be loaded before the playlist model
    * (which gets started by "statusBar::connectPlaylist" below so that it can handle any
    * tracks in the saved playlist that are associated with services. Eg, if
    * the playlist has a Magnatune track in it when Amarok is closed, then the
    * Magnatune service needs to be initialized before the playlist is loaded
    * here. */
    PERF_LOG( "Instantiate Plugin Manager" )
    The::pluginManager();
    PERF_LOG( "Started Plugin Manager instance" )

    // Sets caption and icon correctly (needed e.g. for GNOME)
//     kapp->setTopWidget( this );
    PERF_LOG( "Set Top Widget" )
    createActions();
    PERF_LOG( "Created actions" )

    //new K3bExporter();

    The::paletteHandler()->setPalette( palette() );
    setPlainCaption( i18n( AMAROK_CAPTION ) );

    init();  // We could as well move the code from init() here, but meh.. getting a tad long

    //restore active category ( as well as filters and levels and whatnot.. )
    const QString path = Amarok::config().readEntry( "Browser Path", QString() );
    if( !path.isEmpty() )
        m_browserDock.data()->list()->navigate( path );

    setAutoSaveSettings();

    m_showMenuBar->setChecked(!menuBar()->isHidden());  // workaround for bug #171080

    EngineController *engine = The::engineController();
    connect( engine, SIGNAL( stopped( qint64, qint64 ) ),
             this, SLOT( slotStopped() ) );
    connect( engine, SIGNAL( paused() ),
             this, SLOT( slotPaused() ) );
    connect( engine, SIGNAL( trackPlaying( Meta::TrackPtr ) ),
             this, SLOT( slotNewTrackPlaying() ) );
    connect( engine, SIGNAL( trackMetadataChanged( Meta::TrackPtr ) ),
             this, SLOT( slotMetadataChanged( Meta::TrackPtr ) ) );
    connect( engine, SIGNAL( trackPlaying( Meta::TrackPtr ) ),
			 this, SLOT( currentTrackPlaying( Meta::TrackPtr  ) ) );

    KGlobal::locale()->insertCatalog( "libplasma" );
    win = new QWidget;
    hLayout = new QHBoxLayout;
    win->setLayout( hLayout );
    view = new QDeclarativeView;
    ctxt = view->rootContext();
    hLayout->addWidget( view );
}

void
MainWindow::currentTrackPlaying( Meta::TrackPtr track )
{
	currentModel.setProperty( "trackName", track.data()->prettyName() );
    currentModel.setProperty( "artistName", track.data()->artist()->prettyName() );
    ctxt->setContextProperty( "currentModel", &currentModel );
    view->setSource( QUrl( "/home/saurabh/kde/src/amarok/src/current.qml" ) );
	win->show();
}

MainWindow::~MainWindow()
{
    DEBUG_BLOCK

    //save currently active category
    Amarok::config().writeEntry( "Browser Path", m_browserDock.data()->list()->path() );

#ifdef DEBUG_BUILD_TYPE
    delete m_networkViewer.data();
#endif // DEBUG_BUILD_TYPE
    delete The::svgHandler();
    delete The::paletteHandler();
}


///////// public interface

/**
 * This function will initialize the main window.
 */
void
MainWindow::init()
{
    DEBUG_BLOCK

    layout()->setContentsMargins( 0, 0, 0, 0 );
    layout()->setSpacing( 0 );

    //create main toolbar
    m_mainToolbar = new MainToolbar( 0 );
    m_mainToolbar.data()->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
    m_mainToolbar.data()->setMovable ( true );
    addToolBar( Qt::TopToolBarArea, m_mainToolbar.data() );

    //create slim toolbar
    m_slimToolbar = new SlimToolbar( 0 );
    m_slimToolbar.data()->setAllowedAreas( Qt::TopToolBarArea | Qt::BottomToolBarArea );
    m_slimToolbar.data()->setMovable ( true );
    addToolBar( Qt::TopToolBarArea, m_slimToolbar.data() );
    m_slimToolbar.data()->hide();

    //BEGIN Creating Widgets
    PERF_LOG( "Create sidebar" )
    m_browserDock = new BrowserDock( this );
    m_browserDock.data()->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );

    m_browserDock.data()->installEventFilter( this );
    PERF_LOG( "Sidebar created" )

    PERF_LOG( "Create Playlist" )
    m_playlistDock = new Playlist::Dock( this );
    m_playlistDock.data()->installEventFilter( this );
    //HACK, need to connect after because of order in MainWindow()
    connect( Amarok::actionCollection()->action( "playlist_edit_queue" ),
             SIGNAL( triggered( bool ) ), m_playlistDock.data(), SLOT( slotEditQueue() ) );
    PERF_LOG( "Playlist created" )

    PERF_LOG( "Creating ContextWidget" )
    m_contextDock = new ContextDock( this );
    m_contextDock.data()->installEventFilter( this );

    PERF_LOG( "ContextScene created" )
    //END Creating Widgets

    createMenus();

    PERF_LOG( "Loading default contextScene" )

    PERF_LOG( "Loaded default contextScene" )

    setDockOptions( QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks | QMainWindow::AnimatedDocks | QMainWindow::VerticalTabs );

    addDockWidget( Qt::LeftDockWidgetArea, m_browserDock.data() );
    addDockWidget( Qt::LeftDockWidgetArea, m_contextDock.data(), Qt::Horizontal );
    addDockWidget( Qt::LeftDockWidgetArea, m_playlistDock.data(), Qt::Horizontal );

    setLayoutLocked( AmarokConfig::lockLayout() );

    //<Browsers>
    {
        Debug::Block block( "Creating browsers. Please report long start times!" );

        PERF_LOG( "Creating CollectionWidget" )
        m_collectionBrowser = new CollectionWidget( "collections", 0 );
        m_collectionBrowser->setPrettyName( i18n( "Local Music" ) );
        m_collectionBrowser->setIcon( KIcon( "collection-amarok" ) );
        m_collectionBrowser->setShortDescription( i18n( "Local sources of content" ) );
        m_browserDock.data()->list()->addCategory( m_collectionBrowser );
        PERF_LOG( "Created CollectionWidget" )


        PERF_LOG( "Creating ServiceBrowser" )
        ServiceBrowser *internetContentServiceBrowser = ServiceBrowser::instance();
        internetContentServiceBrowser->setParent( 0 );
        internetContentServiceBrowser->setPrettyName( i18n( "Internet" ) );
        internetContentServiceBrowser->setIcon( KIcon( "applications-internet" ) );
        internetContentServiceBrowser->setShortDescription( i18n( "Online sources of content" ) );
        m_browserDock.data()->list()->addCategory( internetContentServiceBrowser );
        PERF_LOG( "Created ServiceBrowser" )

        PERF_LOG( "Creating PlaylistBrowser" )
        m_playlistBrowser = new PlaylistBrowserNS::PlaylistBrowser( "playlists", 0 );
        m_playlistBrowser->setPrettyName( i18n("Playlists") );
        m_playlistBrowser->setIcon( KIcon( "view-media-playlist-amarok" ) );
        m_playlistBrowser->setShortDescription( i18n( "Various types of playlists" ) );
        m_browserDock.data()->list()->addCategory( m_playlistBrowser );
        PERF_LOG( "CreatedPlaylsitBrowser" )

        PERF_LOG( "Creating FileBrowser" )
        FileBrowser * fileBrowserMkII = new FileBrowser( "files", 0 );
        fileBrowserMkII->setPrettyName( i18n("Files") );
        fileBrowserMkII->setIcon( KIcon( "folder-amarok" ) );
        fileBrowserMkII->setShortDescription( i18n( "Browse local hard drive for content" ) );
        m_browserDock.data()->list()->addCategory( fileBrowserMkII );
        PERF_LOG( "Created FileBrowser" )

        internetContentServiceBrowser->setScriptableServiceManager( The::scriptableServiceManager() );
        PERF_LOG( "ScriptableServiceManager done" )

        PERF_LOG( "Creating Podcast Category" )
        m_browserDock.data()->list()->addCategory( The::podcastCategory() );
        PERF_LOG( "Created Podcast Category" )

        PERF_LOG( "finished MainWindow::init" )
    }

    The::amarokUrlHandler(); //Instantiate
    The::coverFetcher(); //Instantiate

    // Runtime check for Qt 4.6 here.
    // We delete the layout file once, because of binary incompatibility with older Qt version.
    // @see: https://bugs.kde.org/show_bug.cgi?id=213990
    const QChar major = qVersion()[0];
    const QChar minor = qVersion()[2];
    if( major.digitValue() >= 4 && minor.digitValue() > 5 )
    {
        KConfigGroup config = Amarok::config();
        if( !config.readEntry( "LayoutFileDeleted", false ) )
        {
            QFile::remove( Amarok::saveLocation() + "layout" );
            config.writeEntry( "LayoutFileDeleted", true );
            config.sync();
        }
    }

    // we must filter ourself to get mouseevents on the "splitter" - what is us, but filtered by the layouter
    installEventFilter( this );

    // restore the layout on app start
    restoreLayout();
}


QMenu*
MainWindow::createPopupMenu()
{
    QMenu* menu = new QMenu( this );

    // Show/hide menu bar
    if (!menuBar()->isVisible())
        menu->addAction( m_showMenuBar );

    menu->addSeparator();

    addViewMenuItems(menu);

    return menu;
}

void
MainWindow::addViewMenuItems(QMenu* menu)
{
    DEBUG_BLOCK
    menu->setTitle( i18nc("@item:inmenu", "&View" ) );

    // Layout locking:
    QAction* lockAction = new QAction( i18n( "Lock Layout" ), this );
    lockAction->setCheckable( true );
    lockAction->setChecked( AmarokConfig::lockLayout() );
    connect( lockAction, SIGNAL( toggled( bool ) ), SLOT( setLayoutLocked( bool ) ) );
    menu->addAction( lockAction );

    menu->addSeparator();

    // Dock widgets:
    QList<QDockWidget *> dockwidgets = qFindChildren<QDockWidget *>( this );

    foreach( QDockWidget* dockWidget, dockwidgets )
    {
        if( dockWidget->parentWidget() == this )
            menu->addAction( dockWidget->toggleViewAction() );
    }

    menu->addSeparator();

    // Toolbars:
    QList<QToolBar *> toolbars = qFindChildren<QToolBar *>( this );
    QActionGroup* toolBarGroup = new QActionGroup( this );
    toolBarGroup->setExclusive( true );

    foreach( QToolBar* toolBar, toolbars )
    {
        if( toolBar->parentWidget() == this )
        {
            QAction* action = toolBar->toggleViewAction();
            connect( action, SIGNAL( toggled( bool ) ), toolBar, SLOT( setVisible( bool ) ) );
            toolBarGroup->addAction( action );
            menu->addAction( action );
        }
    }
}

void
MainWindow::showBrowser( const QString &name )
{
    Q_UNUSED( name );
    // showBrowser( index ); // FIXME
}

void
MainWindow::showDock( AmarokDockId dockId )
{
    QString name;
    switch( dockId )
    {
        case AmarokDockNavigation:
            name = m_browserDock.data()->windowTitle();
            break;
        case AmarokDockContext:
            name = m_contextDock.data()->windowTitle();
            break;
        case AmarokDockPlaylist:
            name = m_playlistDock.data()->windowTitle();
            break;
    }

    QList < QTabBar * > tabList = findChildren < QTabBar * > ();

    foreach( QTabBar *bar, tabList )
    {
        for( int i = 0; i < bar->count(); i++ )
        {
            if( bar->tabText( i ) == name )
            {
                bar->setCurrentIndex( i );
                break;
            }
        }
    }
}

void
MainWindow::saveLayout()  //SLOT
{
    DEBUG_BLOCK

    // Do not save the layout if the main window is hidden
    // Qt takes widgets out of the layout if they're not visible.
    // So this is not going to work. Also see bug 244583
    if (!isVisible())
        return;

    //save layout to file. Does not go into to rc as it is binary data.
    QFile file( Amarok::saveLocation() + "layout" );

    if( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
    {
        file.write( saveState( LAYOUT_VERSION ) );
        file.flush();
    }
}

void
MainWindow::closeEvent( QCloseEvent *e )
{
    saveLayout();

#ifdef Q_WS_MAC
    Q_UNUSED( e );
    hide();
#else

    //KDE policy states we should hide to tray and not quit() when the
    //close window button is pushed for the main widget
    if( AmarokConfig::showTrayIcon() && e->spontaneous() && !kapp->sessionSaving() )
    {
        KMessageBox::information( this,
                i18n( "<qt>Closing the main window will keep Amarok running in the System Tray. "
                      "Use <B>Quit</B> from the menu, or the Amarok tray icon to exit the application.</qt>" ),
                i18n( "Docking in System Tray" ), "hideOnCloseInfo" );

        hide();
        e->ignore();
        return;
    }

    e->accept();
    kapp->quit();
#endif
}

bool
MainWindow::queryExit()
{
    DEBUG_BLOCK

    // save layout on app exit
    saveLayout();

    return true; // KMainWindow API expects us to always return true
}

void
MainWindow::exportPlaylist() const //SLOT
{
    DEBUG_BLOCK

    KFileDialog fileDialog( KUrl("kfiledialog:///amarok-playlist-export"), QString(), 0 );
    QCheckBox *saveRelativeCheck = new QCheckBox( i18n("Use relative path for &saving") );
    saveRelativeCheck->setChecked( AmarokConfig::relativePlaylist() );

    QStringList supportedMimeTypes;
    supportedMimeTypes << "audio/x-mpegurl"; //M3U
    supportedMimeTypes << "audio/x-scpls"; //PLS
    supportedMimeTypes << "application/xspf+xml"; //XSPF

    fileDialog.setMimeFilter( supportedMimeTypes, supportedMimeTypes.first() );
    fileDialog.fileWidget()->setCustomWidget( saveRelativeCheck );
    fileDialog.setOperationMode( KFileDialog::Saving );
    fileDialog.setMode( KFile::File );
    fileDialog.setCaption( i18n("Save As") );
    fileDialog.setObjectName( "PlaylistExport" );

    fileDialog.exec();

    QString playlistPath = fileDialog.selectedFile();

    if( !playlistPath.isEmpty() )
        The::playlist()->exportPlaylist( playlistPath, saveRelativeCheck->isChecked() );
}

void
MainWindow::slotShowActiveTrack() const
{
    m_playlistDock.data()->showActiveTrack();
}

void
MainWindow::slotEditTrackInfo() const
{
    m_playlistDock.data()->editTrackInfo();
}

void
MainWindow::slotShowCoverManager() const //SLOT
{
    CoverManager::showOnce();
}

void
MainWindow::slotShowDiagnosticsDialog() const
{
    DiagnosticDialog *dialog = new DiagnosticDialog( KGlobal::mainComponent().aboutData() );
    dialog->show();
}

void MainWindow::slotShowBookmarkManager() const
{
    The::bookmarkManager()->showOnce();
}

void MainWindow::slotShowEqualizer() const
{
    The::equalizer()->showOnce();
}

void
MainWindow::slotPlayMedia() //SLOT
{
    // Request location and immediately start playback
    slotAddLocation( true );
}

void
MainWindow::slotAddLocation( bool directPlay ) //SLOT
{
    static KUrl lastDirectory;

    // open a file selector to add media to the playlist
    KUrl::List files;
    KFileDialog dlg( KUrl(QDesktopServices::storageLocation(QDesktopServices::MusicLocation) ), QString("*.*|"), this );

    if( !lastDirectory.isEmpty() )
        dlg.setUrl( lastDirectory );

    dlg.setCaption( directPlay ? i18n("Play Media (Files or URLs)") : i18n("Add Media (Files or URLs)") );
    dlg.setMode( KFile::Files | KFile::Directory );
    dlg.setObjectName( "PlayMedia" );
    dlg.exec();
    files = dlg.selectedUrls();

    lastDirectory = dlg.baseUrl();

    if( files.isEmpty() )
        return;

    The::playlistController()->insertOptioned( files, directPlay ? Playlist::AppendAndPlayImmediately : Playlist::AppendAndPlay );
}

void
MainWindow::slotAddStream() //SLOT
{
    bool ok;
    QString url = KInputDialog::getText( i18n("Add Stream"), i18n("Enter Stream URL:"), QString(), &ok, this );

    if( !ok )
        return;

    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( url ) );

    The::playlistController()->insertOptioned( track, Playlist::Append );
}

void
MainWindow::slotFocusPlaylistSearch()
{
    showDock( AmarokDockPlaylist );  // ensure that the dock is visible if tabbed
    m_playlistDock.data()->searchWidget()->focusInputLine();
}

void
MainWindow::slotFocusCollectionSearch()
{
    // ensure collection browser is activated within navigation dock:
    browserDock()->list()->navigate( QString("collections") );
    showDock( AmarokDockNavigation );  // ensure that the dock is visible if tabbed
    m_collectionBrowser->focusInputLine();
}

#ifdef DEBUG_BUILD_TYPE
void
MainWindow::showNetworkRequestViewer() //SLOT
{
    if( !m_networkViewer )
    {
        m_networkViewer = new NetworkAccessViewer( this );
        The::networkAccessManager()->setNetworkAccessViewer( m_networkViewer.data() );

    }
    The::networkAccessManager()->networkAccessViewer()->show();
}
#endif // DEBUG_BUILD_TYPE

/**
 * "Toggle Main Window" global shortcut connects to this slot
 */
void
MainWindow::showHide() //SLOT
{
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), 0, 0 );
    const int currentDesktop = KWindowSystem::currentDesktop();

    if( !isVisible() )
    {
        setVisible( true );
    }
    else
    {
        if( !isMinimized() )
        {
            if( !isActiveWindow() ) // not minimised and without focus
            {
                KWindowSystem::setOnDesktop( winId(), currentDesktop );
                KWindowSystem::activateWindow( winId() );
            }
            else // Amarok has focus
            {
                setVisible( false );
            }
        }
        else // Amarok is minimised
        {
            setWindowState( windowState() & ~Qt::WindowMinimized );
            KWindowSystem::setOnDesktop( winId(), currentDesktop );
            KWindowSystem::activateWindow( winId() );
        }
    }
}

void
MainWindow::showNotificationPopup() // slot
{
    if( Amarok::KNotificationBackend::instance()->isEnabled()
            && !Amarok::OSD::instance()->isEnabled() )
        Amarok::KNotificationBackend::instance()->showCurrentTrack();
    else
        Amarok::OSD::instance()->forceToggleOSD();
}

void
MainWindow::slotFullScreen() // slot
{
    setWindowState( windowState() ^ Qt::WindowFullScreen );
}

void
MainWindow::slotLoveTrack()
{
    emit loveTrack( The::engineController()->currentTrack() );
}

void
MainWindow::activate()
{
#ifdef Q_WS_X11
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), 0, 0 );

    if( KWindowSystem::activeWindow() != winId() )
        setVisible( true );
    else if( !info.isMinimized() )
        setVisible( true );
    if( !isHidden() )
        KWindowSystem::activateWindow( winId() );
#else
    setVisible( true );
#endif
}

void
MainWindow::createActions()
{
    KActionCollection* const ac = Amarok::actionCollection();
    const EngineController* const ec = The::engineController();
    const Playlist::Actions* const pa = The::playlistActions();
    const Playlist::Controller* const pc = The::playlistController();

    KStandardAction::keyBindings( kapp, SLOT( slotConfigShortcuts() ), ac );
    m_showMenuBar = KStandardAction::showMenubar(this, SLOT(slotShowMenuBar()), ac);
    KStandardAction::preferences( kapp, SLOT( slotConfigAmarok() ), ac );
    ac->action( KStandardAction::name( KStandardAction::KeyBindings ) )->setIcon( KIcon( "configure-shortcuts-amarok" ) );
    ac->action( KStandardAction::name( KStandardAction::Preferences ) )->setIcon( KIcon( "configure-amarok" ) );
    ac->action( KStandardAction::name( KStandardAction::Preferences ) )->setMenuRole(QAction::PreferencesRole); // Define OS X Prefs menu here, removes need for ifdef later

    KStandardAction::quit( kapp, SLOT( quit() ), ac );

    KAction *action = new KAction( KIcon( "folder-amarok" ), i18n("&Add Media..."), this );
    ac->addAction( "playlist_add", action );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotAddLocation() ) );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_A ) );

    action = new KAction( KIcon( "edit-clear-list-amarok" ), i18nc( "clear playlist", "&Clear Playlist" ), this );
    connect( action, SIGNAL( triggered( bool ) ), pc, SLOT( clear() ) );
    ac->addAction( "playlist_clear", action );

    action = new KAction( KIcon( "format-list-ordered" ),
                          i18nc( "edit play queue of playlist", "Edit &Queue" ), this );
    //Qt::META+Qt::Key_Q is taken by Plasma as a global
    action->setShortcut( KShortcut( Qt::META + Qt::Key_U ) );
    ac->addAction( "playlist_edit_queue", action );;

    action = new KAction( i18nc( "Remove duplicate and dead (unplayable) tracks from the playlist", "Re&move Duplicates" ), this );
    connect( action, SIGNAL( triggered( bool ) ), pc, SLOT( removeDeadAndDuplicates() ) );
    ac->addAction( "playlist_remove_dead_and_duplicates", action );

    action = new Playlist::LayoutConfigAction( this );
    ac->addAction( "playlist_layout", action );

    action = new KAction( KIcon( "folder-amarok" ), i18n("&Add Stream..."), this );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( slotAddStream() ) );
    ac->addAction( "stream_add", action );

    action = new KAction( KIcon( "document-export-amarok" ), i18n("&Export Playlist As..."), this );
    connect( action, SIGNAL( triggered( bool ) ), this, SLOT( exportPlaylist() ) );
    ac->addAction( "playlist_export", action );

    action = new KAction( KIcon( "bookmark-new" ), i18n( "Bookmark Media Sources View" ), this );
    ac->addAction( "bookmark_browser", action );
    connect( action, SIGNAL( triggered() ), The::amarokUrlHandler(), SLOT( bookmarkCurrentBrowserView() ) );

    action = new KAction( KIcon( "bookmarks-organize" ), i18n( "Bookmark Manager" ), this );
    ac->addAction( "bookmark_manager", action );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( slotShowBookmarkManager() ) );

    action = new KAction( KIcon( "view-media-equalizer" ), i18n( "Equalizer" ), this );
    ac->addAction( "equalizer_dialog", action );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( slotShowEqualizer() ) );

    action = new KAction( KIcon( "bookmark-new" ), i18n( "Bookmark Playlist Setup" ), this );
    ac->addAction( "bookmark_playlistview", action );
    connect( action, SIGNAL( triggered() ), The::amarokUrlHandler(), SLOT( bookmarkCurrentPlaylistView() ) );

    action = new KAction( KIcon( "bookmark-new" ), i18n( "Bookmark Context Applets" ), this );
    ac->addAction( "bookmark_contextview", action );
    connect( action, SIGNAL( triggered() ), The::amarokUrlHandler(), SLOT( bookmarkCurrentContextView() ) );

    action = new KAction( KIcon( "media-album-cover-manager-amarok" ), i18n( "Cover Manager" ), this );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( slotShowCoverManager() ) );
    ac->addAction( "cover_manager", action );

    action = new KAction( KIcon("folder-amarok"), i18n("Play Media..."), this );
    ac->addAction( "playlist_playmedia", action );
    action->setShortcut( Qt::CTRL + Qt::Key_O );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( slotPlayMedia() ) );

    action = new KAction( KIcon("media-track-edit-amarok"), i18n("Edit Details of Currently Selected Track"), this );
    ac->addAction( "trackdetails_edit", action );
    action->setShortcut( Qt::CTRL + Qt::Key_E );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( slotEditTrackInfo() ) );

    action = new KAction( KIcon( "media-seek-forward-amarok" ), i18n("&Seek Forward"), this );
    ac->addAction( "seek_forward", action );
    action->setShortcut( Qt::Key_Right );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_Plus ) );
    connect( action, SIGNAL( triggered( bool ) ), ec, SLOT( seekForward() ) );

    action = new KAction( KIcon( "media-seek-backward-amarok" ), i18n("&Seek Backward"), this );
    ac->addAction( "seek_backward", action );
    action->setShortcut( Qt::Key_Left );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::SHIFT + Qt::Key_Minus ) );
    connect( action, SIGNAL( triggered( bool ) ), ec, SLOT( seekBackward() ) );

    PERF_LOG( "MainWindow::createActions 6" )
    action = new KAction( KIcon("collection-refresh-amarok"), i18n( "Update Collection" ), this );
    connect ( action, SIGNAL( triggered( bool ) ), CollectionManager::instance(), SLOT( checkCollectionChanges() ) );
    ac->addAction( "update_collection", action );

    action = new KAction( this );
    ac->addAction( "prev", action );
    action->setIcon( KIcon("media-skip-backward-amarok") );
    action->setText( i18n( "Previous Track" ) );
    action->setGlobalShortcut( KShortcut( Qt::Key_MediaPrevious ) );
    connect( action, SIGNAL( triggered( bool ) ), pa, SLOT( back() ) );

    action = new KAction( this );
    ac->addAction( "replay", action );
    action->setIcon( KIcon("media-playback-start") );
    action->setText( i18n( "Restart current track" ) );
    action->setGlobalShortcut( KShortcut() );
    connect( action, SIGNAL( triggered( bool ) ), ec, SLOT(replay()) );

    action = new KAction( this );
    ac->addAction( "repopulate", action );
    action->setText( i18n( "Repopulate Playlist" ) );
    action->setIcon( KIcon("view-refresh-amarok") );
    connect( action, SIGNAL( triggered( bool ) ), pa, SLOT( repopulateDynamicPlaylist() ) );

    action = new KAction( this );
    ac->addAction( "disable_dynamic", action );
    action->setText( i18n( "Disable Dynamic Playlist" ) );
    action->setIcon( KIcon("edit-delete-amarok") );
    //this is connected inside the dynamic playlist category

    action = new KAction( KIcon("media-skip-forward-amarok"), i18n( "Next Track" ), this );
    ac->addAction( "next", action );
    action->setGlobalShortcut( KShortcut( Qt::Key_MediaNext ) );
    connect( action, SIGNAL( triggered( bool ) ), pa, SLOT( next() ) );

    action = new KAction( i18n( "Increase Volume" ), this );
    ac->addAction( "increaseVolume", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_Plus ) );
    action->setShortcut( Qt::Key_Plus );
    connect( action, SIGNAL( triggered() ), ec, SLOT( increaseVolume() ) );

    action = new KAction( i18n( "Decrease Volume" ), this );
    ac->addAction( "decreaseVolume", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_Minus ) );
    action->setShortcut( Qt::Key_Minus );
    connect( action, SIGNAL( triggered() ), ec, SLOT( decreaseVolume() ) );

    action = new KAction( i18n( "Toggle Main Window" ), this );
    ac->addAction( "toggleMainWindow", action );
    action->setGlobalShortcut( KShortcut() );
    connect( action, SIGNAL( triggered() ), SLOT( showHide() ) );

    action = new KAction( i18n( "Toggle Full Screen" ), this );
    ac->addAction( "toggleFullScreen", action );
    action->setShortcut( KShortcut( Qt::CTRL + Qt::SHIFT + Qt::Key_F ) );
    connect( action, SIGNAL( triggered() ), SLOT( slotFullScreen() ) );

    action = new KAction( i18n( "Search playlist" ), this );
    ac->addAction( "searchPlaylist", action );
    action->setShortcut( KShortcut( Qt::CTRL + Qt::Key_J ) );
    connect( action, SIGNAL( triggered() ), SLOT( slotFocusPlaylistSearch()) );

    action = new KAction( i18n( "Search collection" ), this );
    ac->addAction( "searchCollection", action );
    action->setShortcut( KShortcut( Qt::CTRL + Qt::Key_F ) );
    connect( action, SIGNAL( triggered() ), SLOT( slotFocusCollectionSearch()) );

    action = new KAction( KIcon( "music-amarok" ), i18n("Show active track"), this );
    ac->addAction( "show_active_track", action );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( slotShowActiveTrack() ) );

    action = new KAction( i18n( "Show Notification Popup" ), this );
    ac->addAction( "showNotificationPopup", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_O ) );
    connect( action, SIGNAL( triggered() ), SLOT( showNotificationPopup() ) );

    action = new KAction( i18n( "Mute Volume" ), this );
    ac->addAction( "mute", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_M ) );
    connect( action, SIGNAL( triggered() ), ec, SLOT( toggleMute() ) );

    action = new KAction( i18n( "Last.fm: Love Current Track" ), this );
    ac->addAction( "loveTrack", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_L ) );
    connect( action, SIGNAL( triggered() ), SLOT( slotLoveTrack() ) );

    action = new KAction( i18n( "Last.fm: Ban Current Track" ), this );
    ac->addAction( "banTrack", action );
    //action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_B ) );
    connect( action, SIGNAL( triggered() ), SIGNAL( banTrack() ) );

    action = new KAction( i18n( "Last.fm: Skip Current Track" ), this );
    ac->addAction( "skipTrack", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_S ) );
    connect( action, SIGNAL( triggered() ), SIGNAL( skipTrack() ) );

    action = new KAction( KIcon( "media-track-queue-amarok" ), i18n( "Queue Track" ), this );
    ac->addAction( "queueTrack", action );
    action->setShortcut( KShortcut( Qt::CTRL + Qt::Key_D ) );
    connect( action, SIGNAL( triggered() ), SIGNAL( switchQueueStateShortcut() ) );

    action = new KAction( i18n( "Rate Current Track: 1" ), this );
    ac->addAction( "rate1", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_1 ) );
    connect( action, SIGNAL( triggered() ), SLOT( setRating1() ) );

    action = new KAction( i18n( "Rate Current Track: 2" ), this );
    ac->addAction( "rate2", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_2 ) );
    connect( action, SIGNAL( triggered() ), SLOT( setRating2() ) );

    action = new KAction( i18n( "Rate Current Track: 3" ), this );
    ac->addAction( "rate3", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_3 ) );
    connect( action, SIGNAL( triggered() ), SLOT( setRating3() ) );

    action = new KAction( i18n( "Rate Current Track: 4" ), this );
    ac->addAction( "rate4", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_4 ) );
    connect( action, SIGNAL( triggered() ), SLOT( setRating4() ) );

    action = new KAction( i18n( "Rate Current Track: 5" ), this );
    ac->addAction( "rate5", action );
    action->setGlobalShortcut( KShortcut( Qt::META + Qt::Key_5 ) );
    connect( action, SIGNAL( triggered() ), SLOT( setRating5() ) );

#ifdef DEBUG_BUILD_TYPE
    action = new KAction( i18n( "Network Request Viewer" ), this );
    ac->addAction( "network_request_viewer", action );
    action->setIcon( KIcon( "utilities-system-monitor" ) );
    connect( action, SIGNAL( triggered() ), SLOT( showNetworkRequestViewer() ) );
#endif // DEBUG_BUILD_TYPE

    action = KStandardAction::redo( pc, SLOT( redo() ), this);
    ac->addAction( "playlist_redo", action );
    action->setEnabled( false );
    action->setIcon( KIcon( "edit-redo-amarok" ) );
    connect( pc, SIGNAL( canRedoChanged( bool ) ), action, SLOT( setEnabled( bool ) ) );

    action = KStandardAction::undo( pc, SLOT( undo() ), this);
    ac->addAction( "playlist_undo", action );
    action->setEnabled( false );
    action->setIcon( KIcon( "edit-undo-amarok" ) );
    connect( pc, SIGNAL( canUndoChanged( bool ) ), action, SLOT( setEnabled( bool ) ) );

    action = new KAction( KIcon( "amarok" ), i18n( "&About Amarok" ), this );
    ac->addAction( "extendedAbout", action );
    connect( action, SIGNAL( triggered() ), SLOT( showAbout() ) );

    action = new KAction ( KIcon( "info-amarok" ), i18n( "&Diagnostics" ), this );
    ac->addAction( "diagnosticDialog", action );
    connect( action, SIGNAL( triggered() ), SLOT( slotShowDiagnosticsDialog() ) );

    action = new KAction( KIcon( "tools-report-bug" ), i18n("&Report Bug..."), this );
    ac->addAction( "reportBug", action );
    connect( action, SIGNAL( triggered() ), SLOT( showReportBug() ) );

    LikeBack *likeBack = new LikeBack( LikeBack::AllButBugs,
        LikeBack::isDevelopmentVersion( KGlobal::mainComponent().aboutData()->version() ) );
    likeBack->setServer( "likeback.kollide.net", "/send.php" );
    likeBack->setAcceptedLanguages( QStringList( "en" ) );
    likeBack->setWindowNamesListing( LikeBack::WarnUnnamedWindows );    //Notify if a window has no name

    KActionCollection *likeBackActions = new KActionCollection( this, KGlobal::mainComponent() );
    likeBackActions->addAssociatedWidget( this );
    likeBack->createActions( likeBackActions );

    ac->addAction( "likeBackSendComment", likeBackActions->action( "likeBackSendComment" ) );
    ac->addAction( "likeBackShowIcons", likeBackActions->action( "likeBackShowIcons" ) );

    PERF_LOG( "MainWindow::createActions 8" )
    new Amarok::MenuAction( ac, this );
    new Amarok::StopAction( ac, this );
    new Amarok::StopPlayingAfterCurrentTrackAction( ac, this );
    new Amarok::PlayPauseAction( ac, this );
    new Amarok::ReplayGainModeAction( ac, this );
    new Amarok::EqualizerAction( ac, this);

    ac->addAssociatedWidget( this );
    foreach( QAction* action, ac->actions() )
        action->setShortcutContext( Qt::WindowShortcut );
}

void
MainWindow::setRating( int n )
{
    n *= 2;

    Meta::TrackPtr track = The::engineController()->currentTrack();
    if( track )
    {
        // if we're setting an identical rating then we really must
        // want to set the half-star below rating
        if( track->rating() == n )
            n -= 1;

        track->setRating( n );
        Amarok::OSD::instance()->OSDWidget::ratingChanged( track->rating() );
    }
}

void
MainWindow::createMenus()
{
    m_menubar = menuBar();

    //BEGIN Actions menu
    KMenu *actionsMenu = new KMenu( m_menubar.data() );
#ifdef Q_WS_MAC
    // Add these functions to the dock icon menu in OS X
    //extern void qt_mac_set_dock_menu(QMenu *);
    //qt_mac_set_dock_menu(actionsMenu);
    // Change to avoid duplicate menu titles in OS X
    actionsMenu->setTitle( i18n("&Music") );
#else
    actionsMenu->setTitle( i18n("&Amarok") );
#endif
    actionsMenu->addAction( Amarok::actionCollection()->action("playlist_playmedia") );
    actionsMenu->addSeparator();
    actionsMenu->addAction( Amarok::actionCollection()->action("prev") );
    actionsMenu->addAction( Amarok::actionCollection()->action("play_pause") );
    actionsMenu->addAction( Amarok::actionCollection()->action("stop") );
    actionsMenu->addAction( Amarok::actionCollection()->action("stop_after_current") );
    actionsMenu->addAction( Amarok::actionCollection()->action("next") );


#ifndef Q_WS_MAC    // Avoid duplicate "Quit" in OS X dock menu
    actionsMenu->addSeparator();
    actionsMenu->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::Quit ) ) );
#endif
    //END Actions menu

    //BEGIN View menu
    QMenu* viewMenu = new QMenu(this);
    addViewMenuItems(viewMenu);
    //END View menu

    //BEGIN Playlist menu
    KMenu *playlistMenu = new KMenu( m_menubar.data() );
    playlistMenu->setTitle( i18n("&Playlist") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_add") );
    playlistMenu->addAction( Amarok::actionCollection()->action("stream_add") );
    //playlistMenu->addAction( Amarok::actionCollection()->action("playlist_save") ); //FIXME: See FIXME in PlaylistDock.cpp
    playlistMenu->addAction( Amarok::actionCollection()->action( "playlist_export" ) );
    playlistMenu->addSeparator();
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_undo") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_redo") );
    playlistMenu->addSeparator();
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_clear") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_remove_dead_and_duplicates") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_layout") );
    playlistMenu->addAction( Amarok::actionCollection()->action("playlist_edit_queue") );
    //END Playlist menu

    //BEGIN Tools menu
    m_toolsMenu = new KMenu( m_menubar.data() );
    m_toolsMenu.data()->setTitle( i18n("&Tools") );

    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("bookmark_manager") );
    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("cover_manager") );
    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("equalizer_dialog") );
#ifdef QTSCRIPTQTBINDINGS_FOUND
    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("script_manager") );
#endif
#ifdef DEBUG_BUILD_TYPE
    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("network_request_viewer") );
#endif // DEBUG_BUILD_TYPE
    m_toolsMenu.data()->addSeparator();
    m_toolsMenu.data()->addAction( Amarok::actionCollection()->action("update_collection") );
    //END Tools menu

    //BEGIN Settings menu
    m_settingsMenu = new KMenu( m_menubar.data() );
    m_settingsMenu.data()->setTitle( i18n("&Settings") );

    m_settingsMenu.data()->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::ShowMenubar ) ) );

    //TODO use KStandardAction or KXmlGuiWindow

    // the phonon-coreaudio  backend has major issues with either the VolumeFaderEffect itself
    // or with it in the pipeline. track playback stops every ~3-4 tracks, and on tracks >5min it
    // stops at about 5:40. while we get this resolved upstream, don't make playing amarok such on osx.
    // so we disable replaygain on osx

#ifndef Q_WS_MAC
    m_settingsMenu.data()->addAction( Amarok::actionCollection()->action("replay_gain_mode") );
    m_settingsMenu.data()->addSeparator();
#endif

    m_settingsMenu.data()->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::KeyBindings ) ) );
    m_settingsMenu.data()->addAction( Amarok::actionCollection()->action( KStandardAction::name( KStandardAction::Preferences ) ) );
    //END Settings menu

    m_menubar.data()->addMenu( actionsMenu );
    m_menubar.data()->addMenu( viewMenu );
    m_menubar.data()->addMenu( playlistMenu );
    m_menubar.data()->addMenu( m_toolsMenu.data() );
    m_menubar.data()->addMenu( m_settingsMenu.data() );

    KMenu *helpMenu = Amarok::Menu::helpMenu();
    helpMenu->insertAction( helpMenu->actions().first(),
                            Amarok::actionCollection()->action( "reportBug" ) );
    helpMenu->insertAction( helpMenu->actions().last(),
                            Amarok::actionCollection()->action( "extendedAbout" ) );
    helpMenu->insertAction( helpMenu->actions().last(),
                            Amarok::actionCollection()->action( "diagnosticDialog" ) );
    helpMenu->insertAction( helpMenu->actions().at( 4 ),
                            Amarok::actionCollection()->action( "likeBackSendComment" ) );
    helpMenu->insertAction( helpMenu->actions().at( 5 ),
                            Amarok::actionCollection()->action( "likeBackShowIcons" ) );

    m_menubar.data()->addSeparator();
    m_menubar.data()->addMenu( helpMenu );
}

void
MainWindow::slotShowMenuBar()
{
    if (!m_showMenuBar->isChecked())
    {
        //User have chosen to hide a menu. Lets warn him
        if (KMessageBox::warningContinueCancel(this,
            i18n("You have chosen to hide the menu bar.\n\nPlease remember that you can always use the shortcut \"%1\" to bring it back.")
                .arg(m_showMenuBar->shortcut().toString()),
            i18n("Hide Menu"), KStandardGuiItem::cont(), KStandardGuiItem::cancel(), "showMenubar") != KMessageBox::Continue)
        {
            //Cancel menu hiding. Revert menu item to checked state.
            m_showMenuBar->setChecked(true);
            return;
        }
    }
    menuBar()->setVisible(m_showMenuBar->isChecked());
}

void
MainWindow::showAbout()
{
    ExtendedAboutDialog dialog( KGlobal::mainComponent().aboutData(), &ocsData );
    dialog.exec();
}

void
MainWindow::showReportBug()
{
    KBugReport * rbDialog = new KBugReport( this, true, KGlobal::mainComponent().aboutData() );
    rbDialog->setObjectName( "KBugReport" );
    rbDialog->exec();
}

void
MainWindow::paletteChange( const QPalette & oldPalette )
{
    Q_UNUSED( oldPalette )

    The::paletteHandler()->setPalette( palette() );
}

void
MainWindow::slotStopped()
{
    setPlainCaption( i18n( AMAROK_CAPTION ) );
}

void
MainWindow::slotPaused()
{
    setPlainCaption( i18n( "Paused  ::  %1", i18n( AMAROK_CAPTION ) ) );
}

void
MainWindow::slotNewTrackPlaying()
{
    slotMetadataChanged( The::engineController()->currentTrack() );
}

void
MainWindow::slotMetadataChanged( Meta::TrackPtr track )
{
    if( track )
        setPlainCaption( i18n( "%1 - %2  ::  %3", track->artist() ? track->artist()->prettyName() : i18n( "Unknown" ), track->prettyName(), i18n( AMAROK_CAPTION ) ) );
}

CollectionWidget *
MainWindow::collectionBrowser()
{
    return m_collectionBrowser;
}

QString
MainWindow::activeBrowserName()
{
    if( m_browserDock.data()->list()->activeCategory() )
        return m_browserDock.data()->list()->activeCategory()->name();
    else
        return QString();
}

void
MainWindow::setLayoutLocked( bool locked )
{
    DEBUG_BLOCK

    if( locked )
    {
        m_browserDock.data()->setMovable( false );
        m_contextDock.data()->setMovable( false );
        m_playlistDock.data()->setMovable( false );

        m_slimToolbar.data()->setFloatable( false );
        m_slimToolbar.data()->setMovable( false );

        m_mainToolbar.data()->setFloatable( false );
        m_mainToolbar.data()->setMovable( false );
    }
    else
    {
        m_browserDock.data()->setMovable( true );
        m_contextDock.data()->setMovable( true );
        m_playlistDock.data()->setMovable( true );

        m_slimToolbar.data()->setFloatable( true );
        m_slimToolbar.data()->setMovable( true );

        m_mainToolbar.data()->setFloatable( true );
        m_mainToolbar.data()->setMovable( true );
    }

    AmarokConfig::setLockLayout( locked );
    AmarokConfig::self()->writeConfig();
    m_layoutLocked = locked;
}

bool
MainWindow::isLayoutLocked() const
{
    return m_layoutLocked;
}

void
MainWindow::restoreLayout()
{
    DEBUG_BLOCK

    // Do not restore the layout if the main window is hidden
    // Qt takes widgets out of the layout if they're not visible.
    // So this is not going to work. Also see bug 244583
    if (!isVisible())
        return;

    QFile file( Amarok::saveLocation() + "layout" );
    QByteArray layout;
    if( file.open( QIODevice::ReadOnly ) )
    {
        layout = file.readAll();
        file.close();
    }

    if( !restoreState( layout, LAYOUT_VERSION ) )
    {
        //since no layout has been loaded, we know that the items are all placed next to each other in the main window
        //so get the combined size of the widgets, as this is the space we have to play with. Then figure out
        //how much to give to each. Give the context view any pixels leftover from the integer division.

        //int totalWidgetWidth = m_browsersDock->width() + m_contextView->width() + m_playlistDock->width();
        int totalWidgetWidth = contentsRect().width();

        //get the width of the splitter handles, we need to subtract these...
        const int splitterHandleWidth = style()->pixelMetric( QStyle::PM_DockWidgetSeparatorExtent, 0, 0 );
        debug() << "splitter handle widths " << splitterHandleWidth;

        totalWidgetWidth -= ( splitterHandleWidth * 2 );

        debug() << "mainwindow width" <<  contentsRect().width();
        debug() << "totalWidgetWidth" <<  totalWidgetWidth;

        const int widgetWidth = totalWidgetWidth / 3;
        const int leftover = totalWidgetWidth - 3*widgetWidth;

        //We need to set fixed widths initially, just until the main window has been properly layed out. As soon as this has
        //happened, we will unlock these sizes again so that the elements can be resized by the user.
        const int mins[3] = { m_browserDock.data()->minimumWidth(), m_contextDock.data()->minimumWidth(), m_playlistDock.data()->minimumWidth() };
        const int maxs[3] = { m_browserDock.data()->maximumWidth(), m_contextDock.data()->maximumWidth(), m_playlistDock.data()->maximumWidth() };

        m_browserDock.data()->setFixedWidth( widgetWidth );
        m_contextDock.data()->setFixedWidth( widgetWidth + leftover );
        m_playlistDock.data()->setFixedWidth( widgetWidth );
        this->layout()->activate();

        m_browserDock.data()->setMinimumWidth( mins[0] ); m_browserDock.data()->setMaximumWidth( maxs[0] );
        m_contextDock.data()->setMinimumWidth( mins[1] ); m_contextDock.data()->setMaximumWidth( maxs[1] );
        m_playlistDock.data()->setMinimumWidth( mins[2] ); m_playlistDock.data()->setMaximumWidth( maxs[2] );
    }

    // Ensure that only one toolbar is visible
    if( !m_mainToolbar.data()->isHidden() && !m_slimToolbar.data()->isHidden() )
        m_slimToolbar.data()->hide();

    // Ensure that we don't end up without any toolbar (can happen after upgrading)
    if( m_mainToolbar.data()->isHidden() && m_slimToolbar.data()->isHidden() )
        m_mainToolbar.data()->show();
}


bool
MainWindow::playAudioCd()
{
    DEBUG_BLOCK
    //drop whatever we are doing and play auidocd

    QList<Collections::Collection*> collections = CollectionManager::instance()->viewableCollections();

    // Search a non-empty MemoryCollection with the id: AudioCd
    foreach( Collections::Collection *collection, collections )
    {
        if( collection->collectionId() == "AudioCd" )
        {

            debug() << "got audiocd collection";

            Collections::MemoryCollection * cdColl = dynamic_cast<Collections::MemoryCollection *>( collection );

            if( !cdColl || cdColl->trackMap().count() == 0 )
            {
                debug() << "cd collection not ready yet (track count = 0 )";
                m_waitingForCd = true;
                return false;
            }

            The::engineController()->stop( true );
            The::playlistController()->clear();
            The::playlistController()->insertOptioned( cdColl->trackMap().values(), Playlist::DirectPlay );
            m_waitingForCd = false;
            return true;
        }
    }

    debug() << "waiting for cd...";
    m_waitingForCd = true;
    return false;
}

bool
MainWindow::isWaitingForCd() const
{
    DEBUG_BLOCK
    debug() << "waiting?: " << m_waitingForCd;
    return m_waitingForCd;
}

#include "MainWindow.moc"

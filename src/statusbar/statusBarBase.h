/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef KDE_STATUSBAR_H
#define KDE_STATUSBAR_H

#include "progressBar.h" //convenience
#include <qframe.h>      //baseclass
#include <qmap.h>        //stack allocated
#include <qvaluelist.h>  //stack allocated

class QLabel;
class QTimer;

namespace KIO { class Job; }

//TODO
// * concept of a temporary message that is removed when a qobject parent is deleted

namespace KDE
{
    class OverlayWidget;
    typedef QMap<const QObject*, ProgressBar*> ProgressMap;

    /**
     * @class KDE::StatusBar
     *
     * Caveats:
     *   This only looks sensible if the statusbar is at the bottom of the screen
     */

    class StatusBar : public QFrame
    {
        Q_OBJECT

    public:
        StatusBar( QWidget *parent, const char *name = "mainStatusBar" );
       ~StatusBar();

        enum MessageType { Information, Question, Sorry, Warning, Error, ShowAgainCheckBox };

        /**
         * Start a progress operation, with @param owner
         * if owner is 0, the return value is undefined, the application will prolly crash
         */
        ProgressBar &newProgressOperation( QObject *owner );

        /**
         * Monitor progress for a KIO::Job, very handy.
         */
        ProgressBar &newProgressOperation( KIO::Job* );

        void setProgress( const QObject *owner, int steps );
        void incrementProgress( const QObject *owner );

        void setProgressStatus( const QObject *owner, const QString &text );

    public slots:
        /**
         * The statusbar has a region where you can display a mainMessage.
         * It persists after all other message-types are displayed
         */
        void setMainText( const QString &text );

        /// resets mainText if you've done a shortMessage
        void resetMainText();

        /**
         * Shows a non-invasive messgeBox style message that the user has to dismiss
         * but it doesn't interupt whatever he is doing at the current time.
         * Generally you should use these, as it is very easy for a user to not notice
         * statusBar messages.
         */
        void longMessage( const QString &text, int type = Information );

        void longMessageThreadSafe( const QString &text, int type = Information );


        /**
         * Shows a short message, with a button that can be pushed to show a long
         * message
         */
        void shortLongMessage( const QString &_short, const QString &_long );

        /**
         * Set a temporary message over the mainText label, for 2 seconds.
         * ONLY USE FOR STATUS MESSAGES! ie "Buffering...", "Connecting to source..."
         */
        void shortMessage( const QString &text );

        /** Stop anticipating progress from sender() */
        void endProgressOperation();

        /** Stop anticipating progress from @param owner */
        void endProgressOperation( QObject *owner );

        /**
         * Convenience function works like setProgress( QObject*, int )
         * Uses the return value from sender() to determine the owner of
         * the progress bar in question
         */
        void setProgress( int steps );

        /**
         * Convenience function works like setTotalSteps( QObject*, int )
         * Uses the return value from sender() to determine the owner of
         * the progress bar in question
         */
        //void setTotalSteps( int totalSteps );

        /**
         * Convenience function works like incrementProgress( QObject* )
         * Uses the return value from sender() to determine the owner of
         * the progress bar in question
         */
        void incrementProgress();

    public slots:
        void toggleProgressWindow( bool show );
        void abortAllProgressOperations();

    private slots:
        /** For internal use against KIO::Jobs */
        void setProgress( KIO::Job*, unsigned long percent );
        void hideMainProgressBar();

    protected:
        virtual void polish();
        virtual void customEvent( QCustomEvent* );

        QLabel *m_mainTextLabel;

    private:
        struct Message
        {
            Message() {}
            Message( const QString &_text, const MessageType _type ) : text( _text ), type( _type ), offset( 0 ) {}

            QString text;
            MessageType type;

            int offset;
        };

        void updateTotalProgress();

        QWidget *cancelButton() { return ( QWidget* ) child( "cancelButton" ); }
        QWidget *progressBox()  { return ( QWidget* ) child( "progressBox" ); }

        OverlayWidget *m_popupProgress;
        QProgressBar  *m_mainProgressBar;
        QTimer        *m_tempMessageTimer;

        ProgressMap m_progressMap;
        QValueList<QWidget*> m_messageQueue;
        QString m_mainText;
    };
}
#endif

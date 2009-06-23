/***************************************************************************
* copyright   : (C) 2009 Simon Esneault <simon.esneault@gmail.com>        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "PhotosScrollWidget.h"

// Amarok
#include "Debug.h"
#include "SvgHandler.h"

// KDE
#include <Plasma/Animator>

// QT
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneHoverEvent>
#include <QList>
#include <QPixmap>

#define DEBUG_PREFIX "PhotosScrollWidget"

PhotosScrollWidget::PhotosScrollWidget( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
    , m_id( 0 )
    , m_animating( false )
    , m_speed( 1. )
    , m_margin( 5 )
    , m_scrollmax( 0 )
{
    setAcceptHoverEvents( true );
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
}

void PhotosScrollWidget::setPixmapList (QList < QPixmap * > list)
{
    if ( !m_id )
    {
        Plasma::Animator::self()->stopCustomAnimation( m_id );
        m_id = 0;
    }
    
    foreach( QGraphicsItem *it, this->childItems() )
        delete it;

    m_scrollmax = 0;
    foreach (QPixmap *pix, list )
    {
        
        // scale the cover
        QGraphicsPixmapItem *item = new QGraphicsPixmapItem( this );
        item->setPixmap( The::svgHandler()->addBordersToPixmap(
            pix->scaledToHeight( (int) size().height() - 4 * m_margin,  Qt::SmoothTransformation ), 5, "", true ) );
        item->setPos( m_scrollmax, 0 );
        item->show();
        m_scrollmax += item->boundingRect().width() + m_margin;
    }
}

void PhotosScrollWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
    DEBUG_BLOCK
    Plasma::Animator::self()->stopCustomAnimation( m_id );
    m_id = 0;
}

void PhotosScrollWidget::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
 //   DEBUG_BLOCK
    m_speed = ( event->pos().x() - ( size().width() / 2 ) ) / size().width();
    m_speed*=20;
//    debug() << " speed " << m_speed;
    
    // if m_id = 0, we don't have an animator !
    if ( !m_id )
    {
        m_id = Plasma::Animator::self()->customAnimation( m_scrollmax / 2, m_scrollmax*10, Plasma::Animator::LinearCurve, this, "animate" );
    }
}

void PhotosScrollWidget::animate( qreal anim )
{
    Q_UNUSED( anim );

    // check if we can move or not
    if ( this->childItems().last()->pos().x() + 10 > m_scrollmax )
    {
        debug()<< "PhotosScrollWidget:: > m_scrollmax " << this->childItems().first()->pos().x() - abs( m_speed ) ;
        Plasma::Animator::self()->stopCustomAnimation( m_id );
        m_id = 0;
        return;
    }

    if ( this->childItems().last()->pos().x() - 10 < 0 )
    {
        debug()<< "PhotosScrollWidget:: < 0 " << this->childItems().first()->pos().x() - m_speed ;
        Plasma::Animator::self()->stopCustomAnimation( m_id );
        m_id = 0;
        return;
    }
    
    foreach( QGraphicsItem *it, this->childItems() )
    {
        qreal x = it->pos().x() - m_speed;
        it->setPos( x, it->pos().y() );
        it->update();
    }
}

#include "PhotosScrollWidget.moc"

/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#ifndef BREADCRUMBITEM_H
#define BREADCRUMBITEM_H

#include "widgets/ElidingButton.h"

#include <KHBox>

#include <QPushButton>

class BrowserCategory;

/**
A widget representing a single "breadcrumb" item

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/

class BreadcrumbItem : public KHBox
{
    Q_OBJECT
public:
    BreadcrumbItem( BrowserCategory * category );
    ~BreadcrumbItem();

    void setBold( bool bold );

    QSizePolicy sizePolicy () const;

protected slots:
    void updateSizePolicy();

private:
    BrowserCategory * m_category;
    QPushButton * m_menuButton;
    ElidingButton * m_mainButton;
};

#endif
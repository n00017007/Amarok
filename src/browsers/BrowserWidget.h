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
 
#ifndef BROWSERWIDGET_H
#define BROWSERWIDGET_H

#include "BreadcrumbWidget.h"
#include "BrowserCategoryList.h"

#include <KVBox>

#include <QPointer>

/**
The base widget that contains all other browsers, organized in a dig down interface

	@author 
*/
class BrowserWidget : public KVBox
{
    Q_OBJECT
public:
    BrowserWidget( QWidget * parent );

    ~BrowserWidget();

    BrowserCategoryList *list() const;
    void navigate( const QString &target );

private slots:
    void home();

private:

    BreadcrumbWidget * m_breadcrumbWidget;
    QPointer<BrowserCategoryList> m_categoryList;
};

#endif
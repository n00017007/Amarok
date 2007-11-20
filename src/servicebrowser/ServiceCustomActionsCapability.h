/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#ifndef SERVICECUSTOMACTIONSCAPABILITY_H
#define SERVICECUSTOMACTIONSCAPABILITY_H

#include "meta/CustomActionsCapability.h"

class CustomActionsProvider;

/**
	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class ServiceCustomActionsCapability : public Meta::CustomActionsCapability {
    Q_OBJECT
    public:
        ServiceCustomActionsCapability( CustomActionsProvider * customActionsProvider  );

        virtual ~ServiceCustomActionsCapability();

        virtual QList< QAction * > customActions();

    private:
        CustomActionsProvider * m_customActionsProvider;
};


#endif
/***************************************************************************
 *   Copyright (C) 2010 by Jonas Lundqvist                                 *
 *   jonas@gannon.se                                                 	   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include <Qt>

#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kstandarddirs.h>

#include <torrent/globals.h>

#include "magnetgeneratorprefwidget.h"
#include "magnetgeneratorpluginsettings.h"


using namespace bt;

namespace kt
{

	MagnetGeneratorPrefWidget::MagnetGeneratorPrefWidget(QWidget *parent)
		: PrefPageInterface(MagnetGeneratorPluginSettings::self(),i18n("Magnet Generator"),"kt-magnet",parent)
	{
		setupUi(this);
		connect(kcfg_tracker,SIGNAL(toggled(bool)),this,SLOT(trackerToggled(bool)));
		kcfg_tr->setEnabled(MagnetGeneratorPluginSettings::tracker());
	}

	MagnetGeneratorPrefWidget::~MagnetGeneratorPrefWidget()
	{}

	void MagnetGeneratorPrefWidget::trackerToggled(bool on)
	{
		kcfg_tr->setEnabled(on);
	}


}

#include "magnetgeneratorprefwidget.moc"

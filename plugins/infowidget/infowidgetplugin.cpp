/***************************************************************************
 *   Copyright (C) 2005-2007 by Joris Guisson                              *
 *   joris.guisson@gmail.com                                               *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include <kgenericfactory.h>
#include <kglobal.h>
#include <klocale.h>
#include <util/log.h>
#include <util/logsystemmanager.h>
#include <interfaces/guiinterface.h>
#include <interfaces/coreinterface.h>
#include <interfaces/torrentinterface.h>
#include <settings.h>

#include "infowidgetplugin.h"
#include "iwprefpage.h"
#include "statustab.h"
#include "fileview.h"
#include "chunkdownloadview.h"
#include "peerview.h"
#include "trackerview.h"		
#include "infowidgetpluginsettings.h"
#include "monitor.h"
#include "webseedstab.h"
#include "geoipmanager.h"



K_EXPORT_COMPONENT_FACTORY(ktinfowidgetplugin,KGenericFactory<kt::InfoWidgetPlugin>("ktinfowidgetplugin"))
		
using namespace bt;

namespace kt
{
	

	InfoWidgetPlugin::InfoWidgetPlugin(QObject* parent,const QStringList& ) : Plugin(parent)
	{
		pref = 0;
		peer_view = 0;
		cd_view = 0;
		tracker_view = 0;
		file_view = 0;
		status_tab = 0;
		monitor = 0;
		webseeds_tab = 0;
	}


	InfoWidgetPlugin::~InfoWidgetPlugin()
	{}


	void InfoWidgetPlugin::load()
	{
		LogSystemManager::instance().registerSystem(i18n("Info Widget"),SYS_INW);
		connect(getCore(),SIGNAL(settingsChanged()),this,SLOT(applySettings()));
		
		status_tab = new StatusTab(0);
		file_view = new FileView(0);
		file_view->loadState(KGlobal::config());
		connect(getCore(),SIGNAL(torrentRemoved(bt::TorrentInterface*)),file_view,SLOT(onTorrentRemoved(bt::TorrentInterface*)));

		pref = new IWPrefPage(0);
		TorrentActivityInterface* ta = getGUI()->getTorrentActivity();
		ta->addViewListener(this);
		ta->addToolWidget(status_tab,i18n("Status"),"dialog-information",
			   i18n("Displays status information about a torrent"));
		ta->addToolWidget(file_view,i18n("Files"),"folder",
			   i18n("Shows all the files in a torrent"));
	
		applySettings();
				
		getGUI()->addPrefPage(pref);
		currentTorrentChanged(const_cast<bt::TorrentInterface*>(ta->getCurrentTorrent()));
	}

	void InfoWidgetPlugin::unload()
	{
		LogSystemManager::instance().unregisterSystem(i18n("Bandwidth Scheduler"));
		disconnect(getCore(),SIGNAL(settingsChanged()),this,SLOT(applySettings()));
		disconnect(getCore(),SIGNAL(torrentRemoved(bt::TorrentInterface*)),file_view,SLOT(onTorrentRemoved(bt::TorrentInterface*)));
		if (cd_view)
			cd_view->saveState(KGlobal::config()); 
		if (peer_view)
			peer_view->saveState(KGlobal::config());
		if (file_view)
			file_view->saveState(KGlobal::config());
		if (webseeds_tab)
			webseeds_tab->saveState(KGlobal::config());
		if (tracker_view)
			tracker_view->saveState(KGlobal::config());
		KGlobal::config()->sync();
		
		TorrentActivityInterface* ta = getGUI()->getTorrentActivity();
		ta->removeViewListener(this);
		getGUI()->removePrefPage(pref);
		ta->removeToolWidget(status_tab);
		ta->removeToolWidget(file_view);
		if (cd_view)
			ta->removeToolWidget(cd_view);
		if (tracker_view)
			ta->removeToolWidget(tracker_view); 
		if (peer_view)
			ta->removeToolWidget(peer_view);
		if (webseeds_tab)
			ta->removeToolWidget(webseeds_tab);

		delete monitor;
		monitor = 0;
		delete status_tab;
		status_tab = 0;
		delete file_view;
		file_view = 0;
		delete cd_view;
		cd_view = 0;
		delete peer_view;
		peer_view = 0;
		delete tracker_view;
		tracker_view = 0;
		delete webseeds_tab;
		webseeds_tab = 0;
		pref = 0;
	}

	void InfoWidgetPlugin::guiUpdate()
	{
		if (status_tab && status_tab->isVisible())
			status_tab->update();
		
		if (file_view && file_view->isVisible())
			file_view->update();
		
		if (peer_view && peer_view->isVisible())
			peer_view->update();
		
		if (cd_view && cd_view->isVisible())
			cd_view->update();
		
		if (tracker_view && tracker_view->isVisible())
			tracker_view->update();
		
		if (webseeds_tab && webseeds_tab->isVisible())
			webseeds_tab->update();
	}

	void InfoWidgetPlugin::currentTorrentChanged(bt::TorrentInterface* tc)
	{
		if (status_tab)
			status_tab->changeTC(tc);
		if (file_view)
			file_view->changeTC(tc);
		if (cd_view)
			cd_view->changeTC(tc);
		if (tracker_view)
			tracker_view->changeTC(tc);
		if (webseeds_tab)
			webseeds_tab->changeTC(tc);
		
		if (peer_view)
			peer_view->setEnabled(tc != 0);

		createMonitor(tc);
	}
	
	bool InfoWidgetPlugin::versionCheck(const QString & version) const
	{
		return version == KT_VERSION_MACRO;
	}

	void InfoWidgetPlugin::applySettings()
	{
		// if the colors are invalid, set the default colors
		bool save = false;
		if (!InfoWidgetPluginSettings::firstColor().isValid())
		{
			save = true;
			InfoWidgetPluginSettings::setFirstColor(Qt::green);
		}
		
		if (!InfoWidgetPluginSettings::normalColor().isValid())
		{
			save = true;
			InfoWidgetPluginSettings::setNormalColor(QPalette().color(QPalette::Text));
		}
		
		if (!InfoWidgetPluginSettings::lastColor().isValid())
		{
			save = true;
			InfoWidgetPluginSettings::setLastColor(Qt::red);
		}
		
		if (save)
			InfoWidgetPluginSettings::self()->writeConfig();
		
		showWebSeedsTab( InfoWidgetPluginSettings::showWebSeedsTab());
		showPeerView( InfoWidgetPluginSettings::showPeerView() );
		showChunkView( InfoWidgetPluginSettings::showChunkView() );
		showTrackerView( InfoWidgetPluginSettings::showTrackersView() );
		file_view->setShowListOfFiles( Settings::useFileList(),KGlobal::config());
	}
	
	void InfoWidgetPlugin::showPeerView(bool show)
	{
		TorrentActivityInterface* ta = getGUI()->getTorrentActivity();
		bt::TorrentInterface* tc = ta->getCurrentTorrent();
		
		if (show && !peer_view)
		{
			peer_view = new PeerView(0);
			ta->addToolWidget(peer_view,i18n("Peers"),"system-users",
				   i18n("Displays all the peers you are connected to for a torrent"));
			peer_view->loadState(KGlobal::config());
			createMonitor(tc);
		}
		else if (!show && peer_view)
		{
			peer_view->saveState(KGlobal::config());
			ta->removeToolWidget(peer_view);
			delete peer_view; peer_view = 0;
			createMonitor(tc);
		}
	}
	
	void InfoWidgetPlugin::showChunkView(bool show)
	{
		TorrentActivityInterface* ta = getGUI()->getTorrentActivity();
		bt::TorrentInterface* tc = ta->getCurrentTorrent();
		
		if (show && !cd_view)
		{
			cd_view = new ChunkDownloadView(0);
			ta->addToolWidget(cd_view,i18n("Chunks"),"kt-chunks",
				   i18n("Displays all the chunks you are downloading, of a torrent"));
			
			cd_view->loadState(KGlobal::config());
			cd_view->changeTC(tc);
			createMonitor(tc);	
		}
		else if (!show && cd_view)
		{
			cd_view->saveState(KGlobal::config());
			ta->removeToolWidget(cd_view);
			delete cd_view; cd_view = 0;
			createMonitor(tc);
		}
	}
	
	void InfoWidgetPlugin::showTrackerView(bool show)
	{
		TorrentActivityInterface* ta = getGUI()->getTorrentActivity();
		if (show && !tracker_view)
		{
			tracker_view = new TrackerView(0);
			ta->addToolWidget(tracker_view,i18n("Trackers"),"network-server",
					i18n("Displays information about all the trackers of a torrent"));
			tracker_view->loadState(KGlobal::config());
			tracker_view->changeTC(ta->getCurrentTorrent());
		}
		else if (!show && tracker_view)
		{
			tracker_view->saveState(KGlobal::config());
			ta->removeToolWidget(tracker_view);
			delete tracker_view; tracker_view = 0;
		}
	}
	
	void InfoWidgetPlugin::showWebSeedsTab(bool show)
	{
		TorrentActivityInterface* ta = getGUI()->getTorrentActivity();
		if (show && !webseeds_tab)
		{
			webseeds_tab = new WebSeedsTab(0);
			ta->addToolWidget(webseeds_tab,i18n("Webseeds"),"network-server",
				   i18n("Displays all the webseeds of a torrent"));
			webseeds_tab->loadState(KGlobal::config());
			webseeds_tab->changeTC(ta->getCurrentTorrent());
		}
		else if (!show && webseeds_tab)
		{
			webseeds_tab->saveState(KGlobal::config());
			ta->removeToolWidget(webseeds_tab);
			delete webseeds_tab; webseeds_tab = 0;
		}
	}
	
	void InfoWidgetPlugin::createMonitor(bt::TorrentInterface* tc)
	{	
		if (monitor)
		{
			delete monitor; 
			monitor = 0;
		}
			
		if (peer_view)
			peer_view->removeAll();
		if (cd_view)
			cd_view->removeAll();
			
		if (tc && (peer_view || cd_view))
			monitor = new Monitor(tc,peer_view,cd_view,file_view);
	}
}

#include "infowidgetplugin.moc"

/***************************************************************************
 *   Copyright (C) 2006 by Ivan Vasić                                      *
 *   ivasic@gmail.com                                                      *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.           *
 ***************************************************************************/
#include <kgenericfactory.h>
#include <ktoolbar.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kmainwindow.h>
#include <ktoggleaction.h>

#include <interfaces/coreinterface.h>
#include <interfaces/guiinterface.h>
#include <util/constants.h>
#include <util/log.h>
#include <util/logsystemmanager.h>
#include <util/error.h>
#include <net/socketmonitor.h>
#include <interfaces/functions.h>
#include <settings.h>

#include <qstring.h>
#include <qtimer.h>
#include <qdatetime.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kstdaction.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <solid/networking.h>

#include "scheduleeditor.h"
#include "schedule.h"
#include "bwschedulerplugin.h"
#include "bwprefpage.h"

#include <torrent/globals.h>
#include <peer/peermanager.h>
#include <bwschedulerpluginsettings.h>

using namespace bt;

K_EXPORT_COMPONENT_FACTORY(ktbwschedulerplugin,KGenericFactory<kt::BWSchedulerPlugin>("ktbwschedulerplugin"))

namespace kt
{	

	BWSchedulerPlugin::BWSchedulerPlugin(QObject* parent, const QStringList& args) : Plugin(parent)
	{
		Q_UNUSED(args);
		connect(&m_timer, SIGNAL(timeout()), this, SLOT(timerTriggered()));
		m_editor = 0;
		m_pref = 0;
		QString interface("org.freedesktop.ScreenSaver");
		screensaver = new org::freedesktop::ScreenSaver(interface, "/ScreenSaver",QDBusConnection::sessionBus(),this);
		connect(screensaver,SIGNAL(ActiveChanged(bool)),this,SLOT(screensaverActivated(bool)));
		screensaver_on = screensaver->GetActive();
		
		Solid::Networking::Notifier* notifier = Solid::Networking::notifier();
		connect(notifier,SIGNAL(statusChanged(Solid::Networking::Status)),
				this,SLOT(networkStatusChanged(Solid::Networking::Status)));
	}


	BWSchedulerPlugin::~BWSchedulerPlugin()
	{
	}

	void BWSchedulerPlugin::load()
	{
		LogSystemManager::instance().registerSystem(i18n("Scheduler"),SYS_SCD);
		m_schedule = new Schedule();
		m_pref = new BWPrefPage(0);
		connect(m_pref,SIGNAL(colorsChanged()),this,SLOT(colorsChanged()));
		getGUI()->addPrefPage(m_pref);
		
		connect(getCore(),SIGNAL(settingsChanged()),this,SLOT(colorsChanged()));
		
		try
		{
			m_schedule->load(kt::DataDir() + "current.sched");
		}
		catch (bt::Error & err)
		{
			Out(SYS_SCD|LOG_NOTICE) << "Failed to load current.sched : " << err.toString() << endl;
			m_schedule->clear();
		}
		
		m_editor = new ScheduleEditor(0);
		connect(m_editor,SIGNAL(loaded(Schedule*)),this,SLOT(onLoaded(Schedule*)));
		connect(m_editor,SIGNAL(scheduleChanged()),this,SLOT(timerTriggered()));
		getGUI()->addActivity(m_editor);
		m_editor->setSchedule(m_schedule);
		
		// make sure that schedule gets applied again if the settings change
		connect(getCore(),SIGNAL(settingsChanged()),this,SLOT(timerTriggered()));
		timerTriggered();
	}

	void BWSchedulerPlugin::unload()
	{
		setNormalLimits();
		LogSystemManager::instance().unregisterSystem(i18n("Bandwidth Scheduler"));
		m_timer.stop();
	
		getGUI()->removeActivity(m_editor);
		delete m_editor;
		m_editor = 0;
		
		getGUI()->removePrefPage(m_pref);
		delete m_pref;
		m_pref = 0;
		
		try
		{
			m_schedule->save(kt::DataDir() + "current.sched");
		}
		catch (bt::Error & err)
		{
			Out(SYS_SCD|LOG_NOTICE) << "Failed to save current.sched : " << err.toString() << endl;
		}
		
		delete m_schedule;
		m_schedule = 0;
	}
	
	void BWSchedulerPlugin::setNormalLimits() 
	{
		int ulim = Settings::maxUploadRate();
		int dlim = Settings::maxDownloadRate();
		if (screensaver_on && SchedulerPluginSettings::screensaverLimits())
		{
			ulim = SchedulerPluginSettings::screensaverUploadLimit();
			dlim = SchedulerPluginSettings::screensaverDownloadLimit();
		}
		
		Out(SYS_SCD|LOG_NOTICE) << QString("Changing schedule to normal values : %1 down, %2 up")
		.arg(dlim).arg(ulim) << endl;
		// set normal limits
		getCore()->setSuspendedState(false);
		net::SocketMonitor::setDownloadCap(1024 * dlim);
		net::SocketMonitor::setUploadCap(1024 * ulim);
		if (m_editor)
			m_editor->updateStatusText(ulim,dlim,false,m_schedule->isEnabled());
		
		PeerManager::connectionLimits().setLimits(Settings::maxTotalConnections(), Settings::maxConnections());
	}

	
	void BWSchedulerPlugin::timerTriggered()
	{
		QDateTime now = QDateTime::currentDateTime();
		ScheduleItem* item = m_schedule->getCurrentItem(now);
		if (!item || !m_schedule->isEnabled())
		{
			setNormalLimits();
			restartTimer();
			return;
		}
		
		if (item->suspended)
		{
			Out(SYS_SCD|LOG_NOTICE) << QString("Changing schedule to : PAUSED") << endl;
			if (!getCore()->getSuspendedState())
			{
				getCore()->setSuspendedState(true);
				net::SocketMonitor::setDownloadCap(1024 * Settings::maxDownloadRate());
				net::SocketMonitor::setUploadCap(1024 * Settings::maxUploadRate());
				if (m_editor)
					m_editor->updateStatusText(Settings::maxUploadRate(),Settings::maxDownloadRate(),true,m_schedule->isEnabled());
			}
		}
		else
		{
			int ulim = item->upload_limit;
			int dlim = item->download_limit;
			if (screensaver_on && SchedulerPluginSettings::screensaverLimits())
			{
				ulim = item->ss_upload_limit;
				dlim = item->ss_download_limit;
			}
			
			Out(SYS_SCD|LOG_NOTICE) << QString("Changing schedule to : %1 down, %2 up")
					.arg(dlim).arg(ulim) << endl;
			getCore()->setSuspendedState(false);
			
			net::SocketMonitor::setDownloadCap(1024 * dlim);
			net::SocketMonitor::setUploadCap(1024 * ulim);
			if (m_editor)
				m_editor->updateStatusText(ulim,dlim,false,m_schedule->isEnabled());
		}
		
		if (item->set_conn_limits)
		{
			Out(SYS_SCD|LOG_NOTICE) << QString("Setting connection limits to : %1 per torrent, %2 global")
			.arg(item->torrent_conn_limit).arg(item->global_conn_limit) << endl;
			
			PeerManager::connectionLimits().setLimits(item->global_conn_limit, item->torrent_conn_limit);
		}
		else
		{
			PeerManager::connectionLimits().setLimits(Settings::maxTotalConnections(), Settings::maxConnections());
		}
		
		restartTimer();
	}
	
	void BWSchedulerPlugin::restartTimer()
	{
		QDateTime now = QDateTime::currentDateTime();
		// now calculate the new interval
		int wait_time = m_schedule->getTimeToNextScheduleEvent(now) * 1000;
		Out(SYS_SCD|LOG_NOTICE) << "Timer will fire in " << wait_time << " ms" << endl;
		if (wait_time < 1000)
			wait_time = 1000;
		m_timer.stop();
		m_timer.start(wait_time);
	}
	
	void BWSchedulerPlugin::onLoaded(Schedule* ns)
	{
		delete m_schedule;
		m_schedule = ns;
		m_editor->setSchedule(ns);
		timerTriggered();
	}
		
	
	bool BWSchedulerPlugin::versionCheck(const QString & version) const
	{
		return version == KT_VERSION_MACRO;
	}
	
	void BWSchedulerPlugin::colorsChanged()
	{
		if (m_editor)
		{
			m_editor->setSchedule(m_schedule);
			m_editor->colorsChanged();
		}
	}
	
	void BWSchedulerPlugin::screensaverActivated(bool on) 
	{
		screensaver_on = on;
		timerTriggered();
	}
	
	void BWSchedulerPlugin::networkStatusChanged(Solid::Networking::Status status)
	{
		if (status == Solid::Networking::Connected)
		{
			Out(SYS_SCD|LOG_NOTICE) << "Network is up, setting schedule" << endl;
			timerTriggered();
		}
	}

}

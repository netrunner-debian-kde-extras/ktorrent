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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#include <stdlib.h>
#include <util/log.h>
#include <util/sha1hash.h>
#include <torrent/globals.h>
#include <torrent/server.h>
#include <peer/peerid.h>
#include <interfaces/torrentinterface.h>
#include "torrentservice.h"

using namespace bt;

namespace kt
{
	TorrentService::TorrentService(TorrentInterface* tc) : tc(tc),srv(0),browser(0)
	{
	}
	
	TorrentService::~TorrentService()
	{
		stop(0);
	}
	
	void TorrentService::onPublished(bool ok)
	{
		if (ok)
			Out(SYS_ZCO|LOG_NOTICE) << "ZC: " << tc->getStats().torrent_name << " was published" << endl;
		else
			Out(SYS_ZCO|LOG_NOTICE) << "ZC: failed to publish " << tc->getStats().torrent_name  << endl;
	}
	
	void TorrentService::stop(bt::WaitJob* wjob)
	{
		Q_UNUSED(wjob);
		if (srv)
		{
			srv->stop();
			srv->deleteLater();
			srv = 0;
		}

		if (browser)
		{
			browser->deleteLater();
			browser = 0;
		}
	}
	
	void TorrentService::start()
	{
		bt::Uint16 port = bt::ServerInterface::getPort();
		QString name = QString("%1__%2%3").arg(tc->getOwnPeerID().toString()).arg((rand() % 26) + 65).arg((rand() % 26) + 65);
		QStringList subtypes;
		subtypes << QString("_" + tc->getInfoHash().toString() + "._sub._bittorrent._tcp");
		
		if (!srv)
		{
			srv = new DNSSD::PublicService();
			
			srv->setPort(port);
			srv->setServiceName(name);
			srv->setType("_bittorrent._tcp");
			srv->setSubTypes(subtypes);	
			
			connect(srv,SIGNAL(published(bool)),this,SLOT(onPublished(bool)));
			srv->publishAsync();
		}
		
		
		if (!browser)
		{
			browser = new DNSSD::ServiceBrowser(QString("_" + tc->getInfoHash().toString() + "._sub._bittorrent._tcp"),true);
			connect(browser,SIGNAL(serviceAdded(DNSSD::RemoteService::Ptr)),this,SLOT(onServiceAdded(DNSSD::RemoteService::Ptr)));
			browser->startBrowse();
		}
	}
	
	void TorrentService::onServiceAdded(DNSSD::RemoteService::Ptr ptr)
	{
		// lets not connect to ourselve
		if (!ptr->serviceName().startsWith(tc->getOwnPeerID().toString()))
		{
			QString host = ptr->hostName();
			bt::Uint16 port = ptr->port();
			Out(SYS_ZCO|LOG_NOTICE) << "ZC: found local peer " << host << ":" << port << endl;
			// resolve host name before adding it 
			net::AddressResolver::resolve(host, port, this, SLOT(hostResolved(net::AddressResolver*)));
		}
	}
	
	void TorrentService::hostResolved(net::AddressResolver* ar)
	{
		if (ar->succeeded())
		{
			addPeer(ar->address(), true);
			peersReady(this); 
		}
	}
	
	void TorrentService::aboutToBeDestroyed()
	{
		serviceDestroyed(this);
	}
}

#include "torrentservice.moc"

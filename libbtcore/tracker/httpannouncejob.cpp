/*
    Copyright (C) 2009 by Joris Guisson (joris.guisson@gmail.com)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "httpannouncejob.h"
#include <btversion.h>

namespace bt
{

	HTTPAnnounceJob::HTTPAnnounceJob(const KUrl& url) : url(url),get_id(0),proxy_port(-1)
	{
		output.setBuffer(&reply_data);
		http = new QHttp(this);
		connect(http,SIGNAL(requestFinished(int,bool)),this,SLOT(requestFinished(int,bool)));
	}
	
	HTTPAnnounceJob::~HTTPAnnounceJob()
	{
	}

	void HTTPAnnounceJob::start()
	{
		http->setHost(url.host(),url.protocol() == "https" ? QHttp::ConnectionModeHttps : QHttp::ConnectionModeHttp,url.port(80));
		if (!proxy_host.isEmpty() && proxy_port > 0)
			http->setProxy(proxy_host,proxy_port);
		
		QHttpRequestHeader hdr("GET",url.encodedPathAndQuery(),1,1);
		hdr.setValue("User-Agent",bt::GetVersionString());
		hdr.setValue("Connection","close");
		hdr.setValue("Host",QString("%1:%2").arg(url.host()).arg(url.port(80)));
		get_id = http->request(hdr,0,&output);
	}
	
	void HTTPAnnounceJob::kill(bool quietly)
	{
		http->abort();
		if (!quietly)
			emitResult();
	}

	void HTTPAnnounceJob::requestFinished(int id, bool err)
	{
		if (get_id != id)
			return;
		
		if (err)
		{
			setErrorText(http->errorString());
		}
		else
		{
			switch (http->lastResponse().statusCode())
			{
				case 403:
					setError(KIO::ERR_ACCESS_DENIED);
					break;
				case 404:
					setError(KIO::ERR_DOES_NOT_EXIST);
					break;
				case 500:
					setError(KIO::ERR_INTERNAL_SERVER);
					break;
			}
		}
		
		emitResult();
	}
	
	
	void HTTPAnnounceJob::setProxy(const QString& host, int port)
	{
		proxy_host = host;
		proxy_port = port;
	}



}
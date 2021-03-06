/***************************************************************************
 *   Copyright (C) 2008 by Joris Guisson and Ivan Vasic                    *
 *   joris.guisson@gmail.com                                               *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#ifndef KTDOWNLOADANDCONVERTJOB_H
#define KTDOWNLOADANDCONVERTJOB_H

#include <kio/job.h>

namespace kt
{
	class ConvertDialog;
	
	/**
		Job to download and convert a filter file
	*/
	class DownloadAndConvertJob : public KIO::Job
	{
		Q_OBJECT
	public:
		enum Mode
		{
			Verbose,Quietly
		};
		DownloadAndConvertJob(const KUrl & url,Mode mode);
		virtual ~DownloadAndConvertJob();
		
		enum ErrorCode
		{
			CANCELED = 100,DOWNLOAD_FAILED,UNZIP_FAILED,MOVE_FAILED,BACKUP_FAILED
		};
		
		virtual void kill(KJob::KillVerbosity v);
		void start();
		
		bool isAutoUpdate() const {return mode == Quietly;}
		
	signals:
		/// Emitted when the job needs to show a notification
		void notification(const QString & msg);
		
	private slots:
		void downloadFileFinished(KJob*);
		void convert(KJob*);
		void extract(KJob*);
		void makeBackupFinished(KJob* );
		void revertBackupFinished(KJob*);
		void convertAccepted();
		void convertRejected();
			
	private:
		void convert();
		void cleanUp(const QString & path);
		void cleanUpFiles();
		
	private:
		KUrl url;
		KJob* active_job;
		bool unzip;
		ConvertDialog* convert_dlg;
		Mode mode;
	};

}

#endif

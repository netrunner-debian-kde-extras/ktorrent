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
#include <kmimetype.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kzip.h>
#include <kio/jobuidelegate.h>
#include <util/log.h>
#include <util/functions.h>
#include <util/fileops.h>
#include <util/error.h>
#include <util/decompressfilejob.h>
#include <util/extractfilejob.h>
#include <interfaces/functions.h>
#include "convertdialog.h"
#include "downloadandconvertjob.h"

using namespace bt;


namespace kt
{

	DownloadAndConvertJob::DownloadAndConvertJob(const KUrl & url,Mode mode)
			: url(url),unzip(false),convert_dlg(0),mode(mode)
	{
	}


	DownloadAndConvertJob::~DownloadAndConvertJob()
	{
	}

	void DownloadAndConvertJob::start()
	{
		QString temp = kt::DataDir() + "tmp-" + url.fileName();
		if (bt::Exists(temp))
			bt::Delete(temp,true);
		
		active_job = KIO::file_copy(url,temp,-1,KIO::Overwrite);
		connect(active_job,SIGNAL(result(KJob*)),this,SLOT(downloadFileFinished(KJob*)));
	}
	
	void DownloadAndConvertJob::kill(KJob::KillVerbosity )
	{
		if (active_job)
			active_job->kill(KJob::EmitResult);
		else if (convert_dlg)
			convert_dlg->reject();
	}
	
	void DownloadAndConvertJob::convert(KJob* j)
	{
		active_job = 0;
		if (j->error())
		{
			Out(SYS_IPF|LOG_NOTICE) << "IP filter update failed: " << j->errorString() << endl;
			if (mode == Verbose)
			{
				((KIO::Job*)j)->ui()->showErrorMessage();
			}
			else
			{
				QString msg = i18n("Automatic update of IP filter failed: %1", j->errorString());
				notification(msg);
			}
			setError(unzip ? UNZIP_FAILED : MOVE_FAILED);
			emitResult();
		}
		else
			convert();
	}
	
	void DownloadAndConvertJob::downloadFileFinished(KJob* j)
	{
		active_job = 0;
		if (j->error())
		{
			Out(SYS_IPF|LOG_NOTICE) << "IP filter update failed: " << j->errorString() << endl;
			if (mode == Verbose)
			{
				((KIO::Job*)j)->ui()->showErrorMessage();
			}
			else
			{
				QString msg = i18n("Automatic update of IP filter failed: %1", j->errorString());
				notification(msg);
			}
			
			setError(DOWNLOAD_FAILED); 
			emitResult();
			return;
		}
		
		QString temp = kt::DataDir() + "tmp-" + url.fileName();
		
		//now determine if it's ZIP or TXT file
		KMimeType::Ptr ptr = KMimeType::findByPath(temp);
		if (ptr->name() == "application/zip")
		{
			active_job = KIO::file_move(temp,QString(kt::DataDir() + QLatin1String("level1.zip")),-1,KIO::HideProgressInfo|KIO::Overwrite);
			connect(active_job,SIGNAL(result(KJob*)),this,SLOT(extract(KJob*)));
		}
		else if (ptr->name() == "application/x-gzip" || ptr->name() == "application/x-bzip")
		{
			active_job = new bt::DecompressFileJob(temp,QString(kt::DataDir() + "level1.txt"));
			connect(active_job,SIGNAL(result(KJob*)),this,SLOT(convert(KJob*)));
			active_job->start();
		}
		else
		{
			active_job = KIO::file_move(temp,QString(kt::DataDir() + "level1.txt"),-1, KIO::HideProgressInfo|KIO::Overwrite);
			connect(active_job,SIGNAL(result(KJob*)),this,SLOT(convert(KJob*)));
		}
	}
	
	void DownloadAndConvertJob::extract(KJob* j)
	{
		active_job = 0;
		if (j->error())
		{
			Out(SYS_IPF|LOG_NOTICE) << "IP filter update failed: " << j->errorString() << endl;
			if (mode == Verbose)
			{
				((KIO::Job*)j)->ui()->showErrorMessage();
			}
			else
			{
				QString msg = i18n("Automatic update of IP filter failed: %1", j->errorString());
				notification(msg);
			}
			setError(MOVE_FAILED);
			emitResult();
			return;
		}
		
		QString zipfile = kt::DataDir() + "level1.zip";
		KZip* zip = new KZip(zipfile);
		if (!zip->open(QIODevice::ReadOnly) || !zip->directory())
		{
			Out(SYS_IPF|LOG_NOTICE) << "IP filter update failed: cannot open zip file " << zipfile << endl;
			if (mode == Verbose)
			{
				KMessageBox::error(0,i18n("Cannot open zip file %1.",zipfile));
			}
			else
			{
				QString msg = i18n("Automatic update of IP filter failed: cannot open zip file %1", zipfile);
				notification(msg);
			}
			
			setError(UNZIP_FAILED);
			emitResult();
			delete zip;
			return;
		}
		
		QString destination = kt::DataDir() + "level1.txt";
		if (zip->directory()->entries().contains("splist.txt"))
		{
			active_job = new bt::ExtractFileJob(zip,"splist.txt",destination);
			connect(active_job,SIGNAL(result(KJob*)),this,SLOT(convert(KJob*)));
			unzip = true;
			active_job->start();
		}
		else if (zip->directory()->entries().contains("level1.txt"))
		{
			active_job = new bt::ExtractFileJob(zip,"level1.txt",destination);
			connect(active_job,SIGNAL(result(KJob*)),this,SLOT(convert(KJob*)));
			unzip = true;
			active_job->start();
		}
		else if (zip->directory()->entries().contains("ipfilter.dat"))
		{
			active_job = new bt::ExtractFileJob(zip,"ipfilter.dat",destination);
			connect(active_job,SIGNAL(result(KJob*)),this,SLOT(convert(KJob*)));
			unzip = true;
			active_job->start();
		}
		else
		{
			Out(SYS_IPF|LOG_NOTICE) << "IP filter update failed: no blocklist found in zipfile " << zipfile << endl;
			if (mode == Verbose)
			{
				KMessageBox::error(0,i18n("Cannot find blocklist in zip file %1.",zipfile));
			}
			else
			{
				QString msg = i18n("Automatic update of IP filter failed: cannot find blocklist in zip file %1", zipfile);
				notification(msg);
			}
			
			setError(UNZIP_FAILED);
			emitResult();
			delete zip;
		}
	}
	
	void DownloadAndConvertJob::revertBackupFinished(KJob*)
	{
		active_job = 0;
		cleanUpFiles();
		setError(CANCELED);
		emitResult();
	}
	
	void DownloadAndConvertJob::makeBackupFinished(KJob* j)
	{
		if (j && j->error())
		{
			Out(SYS_IPF|LOG_NOTICE) << "IP filter update failed: " << j->errorString() << endl;
			if (mode == Verbose)
			{
				((KIO::Job*)j)->ui()->showErrorMessage();
			}
			else
			{
				QString msg = i18n("Automatic update of IP filter failed: %1", j->errorString());
				notification(msg);
			}
			setError(BACKUP_FAILED);
			emitResult();
		}
		else
		{		
			convert_dlg = new ConvertDialog(0);
			if (mode == Verbose)
				convert_dlg->show();
			connect(convert_dlg,SIGNAL(accepted()),this,SLOT(convertAccepted()));
			connect(convert_dlg,SIGNAL(rejected()),this,SLOT(convertRejected()));
		}
	}
	
	void DownloadAndConvertJob::convertAccepted()
	{
		convert_dlg->deleteLater();
		convert_dlg = 0;
		cleanUpFiles();
		setError(0);
		emitResult();
	}
	
	void DownloadAndConvertJob::convertRejected()
	{
		convert_dlg->deleteLater();
		convert_dlg = 0;
		// shit happened move back backup stuff
		QString dat_file = kt::DataDir() + "level1.dat";
		QString tmp_file = kt::DataDir() + "level1.dat.tmp";
		
		if (bt::Exists(tmp_file))
		{
			active_job = KIO::file_copy(tmp_file,dat_file,-1,KIO::HideProgressInfo|KIO::Overwrite);
			connect(active_job,SIGNAL(result(KJob*)),this,SLOT(revertBackupFinished(KJob*)));	
		}
		else
		{
			cleanUpFiles();
			setError(CANCELED);
			emitResult();
		}
	}
	
	void DownloadAndConvertJob::convert()
	{
		if (bt::Exists(kt::DataDir() + "level1.dat"))
		{
			// make backup of data file, if stuff fails we can always go back
			QString dat_file = kt::DataDir() + "level1.dat";
			QString tmp_file = kt::DataDir() + "level1.dat.tmp";
		
			
			KIO::Job* job = KIO::file_copy(dat_file,tmp_file,-1,KIO::HideProgressInfo|KIO::Overwrite);
			connect(job,SIGNAL(result(KJob*)),this,SLOT(makeBackupFinished(KJob*)));	
		}
		else
			makeBackupFinished(0);
	}
	
	void DownloadAndConvertJob::cleanUpFiles()
	{
		// cleanup temp files
		cleanUp(kt::DataDir() + "level1.zip");
		cleanUp(kt::DataDir() + "level1.txt");
		cleanUp(kt::DataDir() + "level1.tmp");
		cleanUp(kt::DataDir() + "level1.dat.tmp");
	}
	
	void DownloadAndConvertJob::cleanUp(const QString & path)
	{
		if (bt::Exists(path))
			bt::Delete(path,true);
	}
}

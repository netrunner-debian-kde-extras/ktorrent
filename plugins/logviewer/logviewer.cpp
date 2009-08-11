/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
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
#include <QBoxLayout>
#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <qapplication.h>
#include "logviewer.h"
#include "logflags.h"
#include "logviewerpluginsettings.h"
#include <QMenu>
#include <KIcon>


namespace kt
{
	const int LOG_EVENT_TYPE = 65432;
	
	class LogEvent : public QEvent
	{
		QString str;
	public:
		LogEvent(const QString & str) : QEvent((QEvent::Type)LOG_EVENT_TYPE),str(str)
		{}
		
		virtual ~LogEvent()
		{}
		
		const QString & msg() const {return str;}
	};

	LogViewer::LogViewer(LogFlags* flags,QWidget *parent) : Activity(i18n("Log"),"utilities-log-viewer",100,parent),use_rich_text(true),flags(flags),paused(false),menu(0)
	{
		setToolTip(i18n("View the logging output generated by KTorrent")); 
		QVBoxLayout* layout = new QVBoxLayout(this);
		output = new QTextBrowser(this);
		layout->setMargin(0);
		layout->setSpacing(0);
		layout->addWidget(output);
		output->document()->setMaximumBlockCount(200);
		output->setContextMenuPolicy(Qt::CustomContextMenu);
		connect(output,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showMenu(QPoint)));
		
		pause_action = new QAction(KIcon("media-playback-pause"),i18n("Suspend Output"),this);
		pause_action->setCheckable(true);
		connect(pause_action,SIGNAL(toggled(bool)),this,SLOT(pause(bool)));
	}


	LogViewer::~LogViewer()
	{
	}


	void LogViewer::message(const QString& line, unsigned int arg)
	{
		/*
			IMPORTANT: because QTextBrowser is not thread safe, we must use the Qt event mechanism 
			to add strings to it, this will ensure that strings will only be added in the main application
			thread.
		*/
		if(arg==0x00 || flags->checkFlags(arg))
		{
			if(use_rich_text)
			{
				QString tmp = line;
				LogEvent* le = new LogEvent(flags->getFormattedMessage(arg, tmp));
				QApplication::postEvent(this,le);
			}
			else
			{
				LogEvent* le = new LogEvent(line);
				QApplication::postEvent(this,le);
			}
		}
	}
	
	void LogViewer::customEvent(QEvent* ev)
	{
		if (ev->type() == LOG_EVENT_TYPE)
		{
			LogEvent* le = (LogEvent*)ev;
			if (!paused)
			{
				QTextCharFormat fm = output->currentCharFormat();
				output->append(le->msg());
				output->setCurrentCharFormat(fm);
			}
		}
	}
	
	void LogViewer::setRichText(bool val)
	{
		use_rich_text = val;
	}
		
	void LogViewer::showMenu(const QPoint& pos)
	{
		if (!menu)
		{
			menu = output->createStandardContextMenu();
			QAction* first = menu->actions().at(0);
			QAction* sep = menu->insertSeparator(first);
			menu->insertAction(sep,pause_action);
		}
		menu->popup(output->mapToGlobal(pos));
	}
	
	void LogViewer::pause(bool on)
	{
		paused = on;
		QTextCharFormat fm = output->currentCharFormat();
		if (on)
			output->append(i18n("<font color=\"#FF0000\">Logging output suspended</font>"));
		else
			output->append(i18n("<font color=\"#00FF00\">Logging output resumed</font>"));
		output->setCurrentCharFormat(fm);
	}


}
#include "logviewer.moc"

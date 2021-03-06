/***************************************************************************
 *   Copyright (C) 2009 by Joris Guisson                                   *
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

#ifndef KT_MAGNETVIEW_H
#define KT_MAGNETVIEW_H

#include <QTreeView>
#include <KSharedConfig>

class KMenu;

namespace kt
{
	class MagnetModel;
	/**
		View which displays a list of magnet links being downloaded
	*/
	class MagnetView : public QTreeView
	{
		Q_OBJECT
	public:
		MagnetView(MagnetModel* magnet_model,QWidget* parent = 0);
		virtual ~MagnetView();
		
		void saveState(KSharedConfigPtr cfg);
		void loadState(KSharedConfigPtr cfg);
		
		virtual void keyPressEvent(QKeyEvent* event);
		
	private slots:
		void showContextMenu(QPoint p);
		void removeMagnetDownload();
		void startMagnetDownload();
		void stopMagnetDownload();
		
	private:
		MagnetModel* magnet_model;
		KMenu* menu;
		QAction* start;
		QAction* stop;
		QAction* remove;
	};

}

#endif // KT_MAGNETVIEW_H

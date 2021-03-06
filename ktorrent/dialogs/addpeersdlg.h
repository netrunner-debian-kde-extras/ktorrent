/***************************************************************************
 *   Copyright (C) 2007 by Joris Guisson and Ivan Vasic                    *
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
#ifndef KTADDPEERSDLG_H
#define KTADDPEERSDLG_H

#include <qdialog.h>
#include <interfaces/peersource.h>
#include "ui_addpeersdlg.h"

namespace bt
{
	class TorrentInterface;
}

namespace kt
{
	class ManualPeerSource;

	/**
		Dialog to manually add peers to a torrent
	*/
	class AddPeersDlg : public QDialog,public Ui_AddPeersDlg
	{
		Q_OBJECT
	public:
		AddPeersDlg(bt::TorrentInterface* tc,QWidget* parent);
		virtual ~AddPeersDlg();

	private slots:
		void addPressed();
		
	private:
		bt::TorrentInterface* tc;
		ManualPeerSource* mps;
	};

}

#endif

/* 

                          Firewall Builder

                 Copyright (C) 2003 NetCitadel, LLC

  Author:  Vadim Kurland     vadim@fwbuilder.org

  $Id$

  This program is free software which we release under the GNU General Public
  License. You may redistribute and/or modify this program under the terms
  of that license as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  To get a copy of the GNU General Public License, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef __RCSFILEPREVIEW_H_
#define __RCSFILEPREVIEW_H_

#include <ui_rcsfilepreview_q.h>
#include "RCS.h"

#include <qfiledialog.h>
#include <qlistwidget.h>

#include <map>


class RCSViewItem : public QTreeWidgetItem {

 public:

    RCSViewItem(QTreeWidget *parent)     : QTreeWidgetItem(parent) {}
    RCSViewItem(QTreeWidgetItem *parent) : QTreeWidgetItem(parent) {}
    virtual int compare(QTreeWidgetItem *i, int col, bool ascending) const;

};

class RCSFilePreview : public QDialog
{
    Q_OBJECT

    Ui::RCSFilePreview_q     *m_widget;
    RCS                      *rcs;
    QString                   current_file;
    std::map<QString,QString> rcsComments;
    bool                      RO;
    
 public:
    RCSFilePreview(QWidget *parent);
    ~RCSFilePreview();
    RCS*     getSelectedRev();
    bool     showFileRLog( const QString &filename );

public slots:

 virtual void openReadOnly();
 virtual void selectedRevision(QTreeWidgetItem *itm);
 virtual void openFile();

};

#endif

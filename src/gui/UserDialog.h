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


#ifndef __USERDIALOG_H_
#define __USERDIALOG_H_

#include "config.h"
#include <ui_userdialog_q.h>
#include <QWidget>

#include "fwbuilder/FWObject.h"


class ProjectPanel;
class UserDialog : public QWidget
{
    Q_OBJECT

    libfwbuilder::FWObject *obj;
    Ui::UserDialog_q *m_dialog;
    bool init;
    ProjectPanel *m_project;

 public:
    UserDialog(ProjectPanel *project, QWidget *parent);
    ~UserDialog();
    
public slots:
    virtual void changed();
    virtual void libChanged();
    virtual void applyChanges();
    virtual void discardChanges();
    virtual void loadFWObject(libfwbuilder::FWObject *obj);
    virtual void validate(bool*);
    virtual void isChanged(bool*);
    virtual void closeEvent(QCloseEvent *e);

 signals:
/**
 * This signal is emitted from closeEvent, ObjectEditor connects
 * to this signal to make checks before the object editor can be closed
 * and to store its position on the screen
 */
    void close_sign(QCloseEvent *e);
    void changed_sign();
    
};

#endif // UserDialog_H

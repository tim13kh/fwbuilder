/* 

                          Firewall Builder

                 Copyright (C) 2006 NetCitadel, LLC

  Author:  Illiya Yalovoy <yalovoy@gmail.com>

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


#ifndef __INSTBATCHOPTIONSDIALOG_H_
#define __INSTBATCHOPTIONSDIALOG_H_

#include "config.h"
#include <instOptionsDialog.h>

class instConf;

class instBatchOptionsDialog : public instOptionsDialog
{
    Q_OBJECT
 private:
 
 public:
    instBatchOptionsDialog(QWidget *parent, instConf *_cnf);

    
};

#endif 

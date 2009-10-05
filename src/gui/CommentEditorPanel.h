/* 

                          Firewall Builder

                 Copyright (C) 2004 NetCitadel, LLC

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

#ifndef __COMMENTEDITORPANEL_H__
#define __COMMENTEDITORPANEL_H__

#include "../../config.h"
#include "BaseObjectDialog.h"
#include <ui_commenteditorpanel_q.h>
#include <QWidget>

#include "fwbuilder/FWObject.h"
#include "fwbuilder/Rule.h"
#include "fwbuilder/Firewall.h"


class CommentEditorPanel : public BaseObjectDialog
{
    Q_OBJECT;
    Ui::CommentEditorPanel_q *m_widget;

 public:

    CommentEditorPanel(QWidget *p,bool enableLoadFromFile);
    ~CommentEditorPanel();
    libfwbuilder::Rule *rule;

    QString text();
    void setText(QString s);
    void setTitle(QString s);

public slots:
     virtual void loadFromFile();
    

    virtual void changed();
    virtual void applyChanges();
    virtual void discardChanges();
    virtual void loadFWObject(libfwbuilder::FWObject *obj);
    virtual void validate(bool*);
    virtual void isChanged(bool*);
    virtual void closeEvent(QCloseEvent *e);
    virtual void getHelpName(QString*);
    
 signals:
/**
 * This signal is emitted from closeEvent, ObjectEditor connects
 * to this signal to make checks before the object editor can be closed
 * and to store its position on the screen
 */
    void close_sign(QCloseEvent *e);
    void changed_sign();
    void notify_changes_applied_sign();
};

#endif

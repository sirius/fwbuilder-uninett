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


#ifndef __TEMPLATEGROUPDIALOG_H_
#define __TEMPLATEGROUPDIALOG_H_

#include "config.h"
#include <ui_templategroupdialog_q.h>

#include <qtooltip.h>
#include <vector>

#include "fwbuilder/FWObject.h"
#include "ObjectListView.h"
#include "ObjectIconView.h"
#include "BaseObjectDialog.h"
#include <QWidget>

class ObjectIconViewItem;
class ObjectListViewItem;
class QMenu;
class ProjectPanel;

class TemplateGroupDialog : public BaseObjectDialog
{
    Q_OBJECT;


    Ui::TemplateGroupDialog_q *m_dialog;
    ObjectIconView *iconView;
    ObjectListView *listView;
    QMenu *new_object_menu;

    std::vector<int> selectedObjects;

    libfwbuilder::FWObject *selectedObject;


    void addIcon(libfwbuilder::FWObject *o);
    void addIcon(libfwbuilder::FWObject *o, bool ref);

    void setupPopupMenu(const QPoint&);
    void saveColumnWidths();

 public:
    TemplateGroupDialog(QWidget *parent);
    ~TemplateGroupDialog();

    enum viewType { Icon, List };

    // making insertObject() public so we can use it in unit tests
    void insertObject(libfwbuilder::FWObject *o);
    
 public slots:
    virtual void applyChanges();
    virtual void loadFWObject(libfwbuilder::FWObject *obj);
    virtual void validate(bool*);

    virtual void openObject();
    virtual void dropped(QDropEvent *ev);
    virtual void iconContextMenu(const QPoint & pos);
    virtual void listContextMenu(const QPoint & pos);
    
    void copyObj();
    void cutObj();
    void pasteObj();
    void deleteObj();

    void iconViewCurrentChanged(QListWidgetItem *itm);
    void listViewCurrentChanged(QTreeWidgetItem *itm);
    
    void iconViewSelectionChanged();
    void listViewSelectionChanged();
    
    void selectObject(libfwbuilder::FWObject *o);

    void newObject();

    void itemDoubleClicked(QListWidgetItem*);
    void itemDoubleClicked(QTreeWidgetItem*, int);
    
 private:

    static enum viewType vt;

 public:

    
};

#endif // __TEMPLATEGROUPDIALOG_H_

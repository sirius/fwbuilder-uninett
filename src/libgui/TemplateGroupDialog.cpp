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


#include "config.h"
#include "global.h"
#include "utils.h"
#include "events.h"

#include "ProjectPanel.h"
#include "FWBTree.h"
#include "FWBSettings.h"
#include "FWObjectPropertiesFactory.h"
#include "TemplateGroupDialog.h"
#include "FWObjectDrag.h"
#include "FWObjectClipboard.h"
#include "ObjectTreeView.h"
#include "FWWindow.h"
#include "FWCmdChange.h"

#include "fwbuilder/FWObjectDatabase.h"
#include "fwbuilder/Library.h"
#include "fwbuilder/Group.h"
#include "fwbuilder/Resources.h"
#include "fwbuilder/ServiceGroup.h"
#include "fwbuilder/FWObjectReference.h"
#include "fwbuilder/FWServiceReference.h"
#include "fwbuilder/FWIntervalReference.h"
#include "fwbuilder/Service.h"
#include "fwbuilder/IPv6.h"
#include "fwbuilder/AddressRange.h"
#include "fwbuilder/Network.h"
#include "fwbuilder/NetworkIPv6.h"

#include <memory>

#include <qlineedit.h>
#include <qtextedit.h>
#include <qlistwidget.h>
#include <qtreewidget.h>
#include <qtoolbutton.h>
#include <qcombobox.h>
#include <qevent.h>
#include <qstackedwidget.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qmenu.h>
#include <qtooltip.h>
#include <qscrollarea.h>
#include <qpixmapcache.h>
#include <QCoreApplication>
#include <QUndoStack>
#include <QtDebug>
#include <QHeaderView>

#include <iostream>
#include <algorithm>


using namespace std;
using namespace libfwbuilder;

class TemplateGroupWidgetItem: public QTreeWidgetItem
{
    FWObjectDatabase * db;
public:
    TemplateGroupWidgetItem(QTreeWidget *parent, FWObjectDatabase *db): QTreeWidgetItem(parent)
    {
        this->db = db;
    }

};

enum TemplateGroupDialog::viewType TemplateGroupDialog::vt = TemplateGroupDialog::Icon;

#define LIST_VIEW_MODE "list"
#define ICON_VIEW_MODE "icon"


TemplateGroupDialog::TemplateGroupDialog(QWidget *parent) :
        BaseObjectDialog(parent)
{
    m_dialog = new Ui::TemplateGroupDialog_q;
    m_dialog->setupUi(this);

    obj = NULL;
    selectedObject = NULL;
    new_object_menu = NULL;

    listView = new ObjectListView( m_dialog->objectViewsStack, "listView" );
    QStringList sl;
    sl << "Name" << "Properties";
    listView->setHeaderLabels (sl);
    listView->setAcceptDrops( true );
    listView->setDragDropMode( QAbstractItemView::DragDrop );
    listView->setContextMenuPolicy ( Qt::CustomContextMenu );

    iconView = new ObjectIconView( m_dialog->objectViewsStack, "iconView" );
    iconView->setContextMenuPolicy ( Qt::CustomContextMenu );

    m_dialog->objectViewsStack->addWidget(iconView);
    m_dialog->objectViewsStack->addWidget(listView);

    if (st->getGroupViewMode() == ICON_VIEW_MODE)
        m_dialog->objectViewsStack->setCurrentWidget(iconView);
    else
        m_dialog->objectViewsStack->setCurrentWidget(listView);

    listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    iconView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    m_dialog->iconViewBtn->setCheckable(true);
    m_dialog->listViewBtn->setCheckable(true);

    //listView->hide();
    //iconView->show();


    m_dialog->iconViewBtn->setAutoRaise(false);
    m_dialog->listViewBtn->setAutoRaise(false);

    connect( iconView, SIGNAL( currentItemChanged(QListWidgetItem*,QListWidgetItem*) ),
             this,     SLOT( iconViewCurrentChanged(QListWidgetItem*) ) );

    connect( iconView, SIGNAL (itemSelectionChanged()),
            this,      SLOT (iconViewSelectionChanged()));

    connect( iconView, SIGNAL( dropped(QDropEvent*) ),
             this,     SLOT( dropped(QDropEvent*) ) );

    connect( iconView, SIGNAL( customContextMenuRequested(const QPoint&) ),
             this,     SLOT( iconContextMenu(const QPoint&) ) );

    connect( iconView, SIGNAL( delObject_sign() ),
             this,     SLOT( deleteObj() ) );

    connect( iconView, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
             this, SLOT(itemDoubleClicked(QListWidgetItem*)));


    connect( listView, SIGNAL( currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*) ),
             this,     SLOT( listViewCurrentChanged(QTreeWidgetItem*) ) );
    connect( listView, SIGNAL (itemSelectionChanged()),
            this,      SLOT (listViewSelectionChanged()));

    connect( listView, SIGNAL( dropped(QDropEvent*) ),
             this,     SLOT( dropped(QDropEvent*) ) );
    connect( listView, SIGNAL( customContextMenuRequested(const QPoint&) ),
             this,     SLOT( listContextMenu(const QPoint&) ) );
    connect( listView, SIGNAL( delObject_sign() ),
             this,     SLOT( deleteObj() ) );

    connect( listView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
             this, SLOT(itemDoubleClicked(QTreeWidgetItem*, int)));

    QString s    = st->getGroupViewColumns();
    int     col0 = s.section(',',0,0).toInt();
    int     col1 = s.section(',',1,1).toInt();

    if (col0 == 0)
        col0 = listView->width()/2;

    listView->setColumnWidth(0,col0);
    listView->setColumnWidth(1,col1);



    connectSignalsOfAllWidgetsToSlotChange();
}

TemplateGroupDialog::~TemplateGroupDialog()
{
    delete m_dialog;
}

void TemplateGroupDialog::iconViewSelectionChanged()
{
    if (fwbdebug) qDebug("GroupObjectDialog::iconViewSelectionChanged()");

    selectedObjects.clear();

    for (int it=0; it<iconView->count(); ++it)
    {
        QListWidgetItem *itm = iconView->item(it);
        if (itm->isSelected())
        {
            int obj_id = itm->data(Qt::UserRole).toInt();
            if (fwbdebug) qDebug("obj_id=%d", obj_id);
            selectedObjects.push_back(obj_id);
        }
    }
}

void TemplateGroupDialog::listViewSelectionChanged()
{
    if (fwbdebug) qDebug("GroupObjectDialog::listViewSelectionChanged()");

    selectedObjects.clear();

    for (int it=0; it<listView->topLevelItemCount(); ++it)
    {
        QTreeWidgetItem *itm = listView->topLevelItem(it);
        if (itm->isSelected())
        {
            int obj_id = itm->data(0, Qt::UserRole).toInt();
            if (fwbdebug) qDebug("obj_id=%d", obj_id);
            selectedObjects.push_back(obj_id);
        }
    }
}

void TemplateGroupDialog::iconViewCurrentChanged(QListWidgetItem *itm)
{
    if (itm==NULL)
    {
        selectedObject=NULL;
        return;
    }
    int obj_id = itm->data(Qt::UserRole).toInt();
    FWObject *o = m_project->db()->findInIndex(obj_id);
    selectedObject = o;
}


void TemplateGroupDialog::listViewCurrentChanged(QTreeWidgetItem *itm)
{
    if (itm==NULL)
    {
        selectedObject=NULL;
        return;
    }
    int obj_id = itm->data(0, Qt::UserRole).toInt();
    FWObject *o = m_project->db()->findInIndex(obj_id);
    selectedObject = o;
}

/*
 * used to add an object for paste and drop operations
 */
void TemplateGroupDialog::insertObject(FWObject *o)
{
    assert(o!=NULL);
    Group *g = dynamic_cast<Group*>(obj);
    assert(g!=NULL);

    if ( ! g->validateChild(o) || g->isReadOnly() ) return;
    // see #1976 do not allow pasting object that has been deleted
    // note that we call insertObject() from dropEvent(), not only from paste()
    if (o->getLibrary()->getId() == FWObjectDatabase::DELETED_OBJECTS_ID)
        return;

    if (fwbdebug)
        qDebug("Adding object %s to the group %s",
               o->getName().c_str(), g->getName().c_str());

/* avoid duplicates */
    int cp_id = o->getId();

    for (int it=0; it<listView->topLevelItemCount(); ++it)
    {
        QTreeWidgetItem *itm = listView->topLevelItem(it);
        int obj_id = itm->data(0, Qt::UserRole).toInt();
	if(obj_id==cp_id) return;
    }

    addIcon(o, ! FWBTree().isSystem(obj) );

    changed();
}

void TemplateGroupDialog::addIcon(FWObject *fwo)
{
    FWObject *o=fwo;
    bool      ref=false;
    if (FWReference::cast(o)!=NULL)
    {
        o=FWReference::cast(o)->getPointer();
        ref=true;
    }

    addIcon(o,ref);
}

void TemplateGroupDialog::addIcon(FWObject *o, bool ref)
{
    if (Resources::global_res->getResourceBool(
            string("/FWBuilderResources/Type/") +
            o->getTypeName() + "/hidden") ) return;

    QString obj_name=QString::fromUtf8(o->getName().c_str());

    QString icn_filename =
            (":/Icons/"+o->getTypeName()+((ref)?"/icon-ref":"/icon")).c_str();

    QPixmap pm;
    if ( ! QPixmapCache::find( icn_filename, pm) )
    {
        pm.load( icn_filename );
        QPixmapCache::insert( icn_filename, pm);
    }

    QListWidgetItem *list_item = new QListWidgetItem(QIcon(pm),
                                                     obj_name, iconView);
    list_item->setData(Qt::UserRole, QVariant(o->getId()));
    iconView->addItem(list_item);

    TemplateGroupWidgetItem *tree_item = new TemplateGroupWidgetItem(listView, m_project->db());
    tree_item->setText(0, obj_name);
    tree_item->setText(1, FWObjectPropertiesFactory::getObjectProperties(o) );
    tree_item->setIcon(0, QIcon(pm) );
    tree_item->setData(0, Qt::UserRole, QVariant(o->getId()));
    listView->addTopLevelItem(tree_item);
}

void TemplateGroupDialog::loadFWObject(FWObject *o)
{
    obj = o;
    Group *g = Group::cast(obj);
    assert(g!=NULL);

    init=true;

    m_dialog->obj_name->setText( QString::fromUtf8(g->getName().c_str()) );
    m_dialog->commentKeywords->loadFWObject(o);

    m_dialog->obj_name->setEnabled( !FWBTree().isSystem(obj) );

    listView->clear();
    iconView->clear();

    listView->setDB(o->getRoot());
    iconView->setDB(o->getRoot());

    iconView->setResizeMode( QListWidget::Adjust );
    iconView->setGridSize( QSize(50, 40) );

    switch (vt)
    {
    case Icon:
        if ( ! m_dialog->iconViewBtn->isChecked() ) m_dialog->iconViewBtn->toggle();
        iconView->raise();
        break;

    case List:
        if ( ! m_dialog->listViewBtn->isChecked() ) m_dialog->listViewBtn->toggle();
        listView->raise();
        break;
    }

    for (FWObject::iterator i=g->begin(); i!=g->end(); i++)
        addIcon( *i );

    listView->header()->resizeSections(QHeaderView::ResizeToContents);

    //apply->setEnabled( false );

    m_dialog->obj_name->setEnabled(!o->isReadOnly() && !FWBTree().isSystem(o));
    setDisabledPalette(m_dialog->obj_name);

    m_dialog->newButton->setEnabled(!o->isReadOnly());

//    listView->setEnabled(!o->isReadOnly());
    setDisabledPalette(listView);

//    iconView->setEnabled(!o->isReadOnly());
    setDisabledPalette(iconView);

    init=false;

    if (FWBTree().isSystem(g))
    {
        m_dialog->newButton->hide();
        return;
    }

    if (new_object_menu)
    {
        new_object_menu->clear();
        m_dialog->newButton->setMenu(NULL);
        delete new_object_menu;
    }

    new_object_menu = new QMenu(this);
    new_object_menu->setObjectName("GroupObjectDialog_newObjectMenu");

    int add_to_group_id = g->getId();

    list<string> types_list;
    g->getAllowedTypesOfChildren(types_list);
    foreach(string tn, types_list)
    {
        if (tn == FWObjectReference::TYPENAME ||
            tn == FWServiceReference::TYPENAME ||
            tn == FWIntervalReference::TYPENAME) continue;
        if (fwbdebug)
            qDebug() << "Adding type" << tn.c_str() << "to the new object menu";
        m_project->m_panel->om->addNewObjectMenuItem(
            new_object_menu, tn.c_str(), "", add_to_group_id);
    }

    m_dialog->newButton->setMenu( new_object_menu );
    m_dialog->newButton->show();
}

void TemplateGroupDialog::validate(bool *res)
{
    *res=true;
    if (!validateName(this, obj, m_dialog->obj_name->text()))
    {
        *res=false;
        return;
    }
}



void TemplateGroupDialog::applyChanges()
{
    std::auto_ptr<FWCmdChange> cmd( new FWCmdChange(m_project, obj));
    FWObject* new_state = cmd->getNewState();

    string oldname = obj->getName();
    string newname = string(m_dialog->obj_name->text().toUtf8().constData());
    if (oldname != newname)
    {
        if (fwbdebug)
            qDebug() << "oldname=" << oldname.c_str()
                     << "newname=" << newname.c_str();
        new_state->setName(newname);
    }

    m_dialog->commentKeywords->applyChanges(new_state);

    set<int> oldobj;
    set<int> newobj;

    for (int it=0; it<listView->topLevelItemCount(); ++it)
    {
        QTreeWidgetItem *itm = listView->topLevelItem(it);
        int obj_id = itm->data(0, Qt::UserRole).toInt();
        newobj.insert(obj_id);
    }

    for (FWObject::iterator j=obj->begin(); j!=obj->end(); ++j)
    {
        FWObject *o = *j;
        if (FWReference::cast(o)!=NULL) o=FWReference::cast(o)->getPointer();
        oldobj.insert(o->getId());
    }

    set<int> diff;

    set_difference( oldobj.begin(), oldobj.end(),
                    newobj.begin(), newobj.end(),
                    inserter(diff,diff.begin()));
/* diff contains objects present in oldobj but not in newobj - these objects
   were deleted from the group */

    for (set<int>::iterator k=diff.begin(); k!=diff.end(); ++k)
    {
        FWObject *o = m_project->db()->findInIndex(*k);
        // Note: FWBTree::isSystem() would not work for new_state because
        // it is not part of the tree and isSystem() relies on the tree path
        if (FWBTree().isSystem(obj))
        {
            m_project->m_panel->om->deleteObject(o);
        } else
        {
            new_state->removeRef(o);
        }
    }

    diff.clear();

    set_difference( newobj.begin(), newobj.end(),
                    oldobj.begin(), oldobj.end(),
                    inserter(diff,diff.begin()));
/* diff contains objects present in newobj but not in oldobj - these objects
   were added to the group */

    for (set<int>::iterator k1=diff.begin(); k1!=diff.end(); ++k1)
    {
        FWObject *o = m_project->db()->findInIndex(*k1);
        if (FWBTree().isSystem(obj))
        {
            m_project->pasteTo(new_state, o);
        } else
        {
            new_state->addRef(o);
        }
    }

    saveColumnWidths();

    if (!cmd->getOldState()->cmp(new_state, true))
    {
        if (obj->isReadOnly()) return;
        m_project->undoStack->push(cmd.release());
    }
}



// This method is attached to the context menu item "Edit"
void TemplateGroupDialog::openObject()
{
    if (selectedObject!=NULL)
    {
        QCoreApplication::postEvent(
            m_project, new showObjectInTreeEvent(selectedObject->getRoot()->getFileName().c_str(),
                                                 selectedObject->getId()));
        QCoreApplication::postEvent(
            mw, new openObjectInEditorEvent(selectedObject->getRoot()->getFileName().c_str(),
                                            selectedObject->getId()));
    }
}

void TemplateGroupDialog::dropped(QDropEvent *ev)
{
    if (fwbdebug) qDebug("GroupObjectDialog::dropped");

    list<FWObject*> ol;
    if (FWObjectDrag::decode(ev, ol))
    {
        if (ol.size()==0) return;
        for (list<FWObject*>::iterator i=ol.begin(); i!=ol.end(); ++i)
            insertObject( *i );
        if (fwbdebug) qDebug("GroupObjectDialog::dropped  ev->acceptAction()");
        ev->setAccepted(true);

        // see comment in ObjectTreeView.cpp explaining the purpose of
        // flag process_mouse_release_event
        ObjectTreeView *otv = m_project->getCurrentObjectTree();
        otv->ignoreNextMouseReleaseEvent();

    }
    if (fwbdebug) qDebug("GroupObjectDialog::dropped  done");
}

void TemplateGroupDialog::iconContextMenu(const QPoint & pos)
{
    FWObject *o = NULL;
    QListWidgetItem *itm = iconView->itemAt(pos);
    if (itm)
    {
        int obj_id = itm->data(Qt::UserRole).toInt();
        o = m_project->db()->findInIndex(obj_id);
        selectedObject = o;
    }
    setupPopupMenu(iconView->mapToGlobal(pos));
}


void TemplateGroupDialog::listContextMenu(const QPoint & pos)
{
    FWObject *o=NULL;
    QTreeWidgetItem *itm = listView->itemAt(pos);
    if (itm)
    {
        int obj_id = itm->data(0, Qt::UserRole).toInt();
        o = m_project->db()->findInIndex(obj_id);
        selectedObject = o;
    }
    setupPopupMenu(listView->viewport()->mapToGlobal(pos));
}

void TemplateGroupDialog::setupPopupMenu(const QPoint &pos)
{
    QMenu *popup = new QMenu(this);

    if (selectedObject!=NULL)
    {
        if (selectedObject->isReadOnly() )
            popup->addAction(tr("Open"), this, SLOT(openObject()));
        else
            popup->addAction(tr("Edit"), this, SLOT(openObject()));
    }

    QAction *copyID = popup->addAction(tr("Copy"), this, SLOT(copyObj()));
    QAction *cutID = popup->addAction(tr("Cut"), this, SLOT(cutObj()));
    QAction *pasteID = popup->addAction(tr("Paste"), this, SLOT(pasteObj()));
    QAction *delID = popup->addAction(tr("Delete"),this, SLOT(deleteObj()));

    copyID->setEnabled(selectedObject!=NULL &&
                          ! FWBTree().isSystem(selectedObject) );
    cutID->setEnabled(selectedObject!=NULL &&
                          ! FWBTree().isSystem(obj) &&
                          ! obj->isReadOnly() );

    // see #1976 do not allow pasting object that has been deleted
    FWObject *obj_in_clipboard = FWObjectClipboard::obj_clipboard->getObject();
    bool obj_deleted = (obj_in_clipboard &&
                        obj_in_clipboard->getParent()->getId() ==
                        FWObjectDatabase::DELETED_OBJECTS_ID);

    pasteID->setEnabled(! FWBTree().isSystem(obj) &&
                        ! obj->isReadOnly() && ! obj_deleted);

    delID->setEnabled(selectedObject!=NULL &&
                          ! FWBTree().isSystem(obj) &&
                          ! obj->isReadOnly() );

    popup->exec( pos );
}

void TemplateGroupDialog::copyObj()
{
    FWObjectClipboard::obj_clipboard->clear();
    for(vector<int>::iterator it=selectedObjects.begin(); 
        it!=selectedObjects.end(); ++it)
    {
        FWObject* selectedObject = m_project->db()->findInIndex(*it);

        if (selectedObject!=NULL && ! FWBTree().isSystem(selectedObject) )
        {
            FWObjectClipboard::obj_clipboard->add(selectedObject,
                                                  this->m_project );
        }

    }
}

void TemplateGroupDialog::cutObj()
{
    copyObj();
    deleteObj();
}

void TemplateGroupDialog::pasteObj()
{
    vector<std::pair<int,ProjectPanel*> >::iterator i;

    for (i= FWObjectClipboard::obj_clipboard->begin();
         i!=FWObjectClipboard::obj_clipboard->end(); ++i)
    {
        insertObject( m_project->db()->findInIndex(i->first) );
    }
}

void TemplateGroupDialog::deleteObj()
{
    // make a copy of the list of selected objects because selection
    // changes when we delete items 
    vector<int> tv;
    copy(selectedObjects.begin(),selectedObjects.end(),inserter(tv,tv.begin()));

    for(vector<int>::iterator it=tv.begin(); it!=tv.end(); ++it)
    {
        if (fwbdebug)
            qDebug("GroupObjectDialog::deleteObj()  (*it)=%d", (*it));

        FWObject* selectedObject = m_project->db()->findInIndex(*it);
        int o_id = selectedObject->getId();

        for (int it=0; it<listView->topLevelItemCount(); ++it)
        {
            QTreeWidgetItem *itm = listView->topLevelItem(it);
            if (o_id == itm->data(0, Qt::UserRole).toInt())
            {
                listView->takeTopLevelItem(it);
                break;
            }
        }

        for (int it=0; it<iconView->count(); ++it)
        {
            QListWidgetItem *itm = iconView->item(it);
            if (o_id == itm->data(Qt::UserRole).toInt())
            {
                iconView->takeItem(it);
                break;
            }
        }
    }
    changed();
}

void TemplateGroupDialog::saveColumnWidths()
{
    if (fwbdebug)
        qDebug("GroupObjectDialog::saveColumnWidths()");

    QString s = QString("%1,%2")
        .arg(listView->columnWidth(0))
        .arg(listView->columnWidth(1));

    st->setGroupViewColumns(s);
}

void TemplateGroupDialog::selectObject(FWObject *o)
{
    int o_id = o->getId();

    for (int it=0; it<listView->topLevelItemCount(); ++it)
    {
        QTreeWidgetItem *itm = listView->topLevelItem(it);
        if (o_id == itm->data(0, Qt::UserRole).toInt())
        {
            listView->setCurrentItem(itm);
            break;
        }
    }

    for (int it=0; it<iconView->count(); ++it)
    {
        QListWidgetItem *itm = iconView->item(it);
        if (o_id == itm->data(Qt::UserRole).toInt())
        {
            iconView->setCurrentItem(itm);
            break;
        }
    }
}

void TemplateGroupDialog::newObject()
{
    m_dialog->newButton->showMenu();
}

void TemplateGroupDialog::itemDoubleClicked(QListWidgetItem*)
{
    openObject();
}

void TemplateGroupDialog::itemDoubleClicked(QTreeWidgetItem*, int)
{
    openObject();
}

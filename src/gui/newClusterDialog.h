/*
 * newClusterDialog.h - new cluster wizard dialog
 *
 * Copyright (c) 2008 secunet Security Networks AG
 * Copyright (c) 2008 Adrian-Ken Rueegsegger <rueegsegger@swiss-it.ch>
 * Copyright (c) 2008 Reto Buerki <buerki@swiss-it.ch>
 *
 * This work is dual-licensed under:
 *
 * o The terms of the GNU General Public License as published by the Free
 *   Software Foundation, either version 2 of the License, or (at your option)
 *   any later version.
 *
 * o The terms of NetCitadel End User License Agreement
 */

#ifndef __NEWCLUSTERDIALOG_H_
#define __NEWCLUSTERDIALOG_H_

#include <ui_newclusterdialog_q.h>

#include "fakeWizard.h"
#include <QListWidgetItem>


namespace libfwbuilder
{
    class FWObject;
    class FWObjectDatabase;
    class ObjectGroup;
    class Cluster;
    class Interface;
};

class newClusterDialog : public QDialog, public FakeWizard
{
    Q_OBJECT;

    Ui::newClusterDialog_q *m_dialog;

    libfwbuilder::Cluster *ncl;
    libfwbuilder::ObjectGroup *fwlist;
    libfwbuilder::FWObject *parent;
    libfwbuilder::FWObjectDatabase *db;
    libfwbuilder::FWObjectDatabase *tmpldb;
    bool unloadTemplatesLib;
    QMap<QRadioButton*, libfwbuilder::Firewall*> copy_rules_from_buttons;
    QMap<int, bool> visited;
    std::list<libfwbuilder::Firewall*> firewallList;
    bool useFirewallList;
    QVBoxLayout policies;
    QRadioButton noPolicy;

    void copyRuleSets(const std::string &type, libfwbuilder::Firewall *src);
    void deleteRuleSets(const std::string &type, libfwbuilder::Firewall *fw);
    void createNewCluster();

public:
    newClusterDialog(libfwbuilder::FWObject *parent);
    virtual ~newClusterDialog();

    libfwbuilder::Cluster* getNewCluster() { return ncl; };

    void setFirewallList(std::vector<libfwbuilder::FWObject*>);

    void showPage(const int page);

protected slots:
    virtual void finishClicked();
    virtual void cancelClicked();
    virtual void nextClicked();
    virtual void backClicked();
    virtual void changed();
};

#endif /* __NEWCLUSTERDIALOG_H */


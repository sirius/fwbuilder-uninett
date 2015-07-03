/*

                          Firewall Builder

                 Copyright (C) 2015 UNINETT AS

  Author:  Sirius Bakke <sirius.bakke@uninett.no>

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
#include "fwbuilder/libfwbuilder-config.h"

#include "ClusterGroup.h"
#include "TemplateGroup.h"
#include "FWObjectDatabase.h"
#include "FWOptions.h"

using namespace std;
using namespace libfwbuilder;

const char *TemplateGroup::TYPENAME = {"TemplateGroup"};

TemplateGroup::TemplateGroup() : ObjectGroup()
{
    setBool("inactive", false);
}

void TemplateGroup::init(FWObjectDatabase *root)
{
    FWObject *gopt = getFirstByType(TemplateGroupOptions::TYPENAME);
    if (NULL == gopt) {
        gopt = root->create(TemplateGroupOptions::TYPENAME);
        add(gopt);
    }
}

bool TemplateGroup::validateChild(FWObject *o)
{
    string otype = o->getTypeName();

    return (FWObject::validateChild(o) &&
            (otype == TemplateGroupOptions::TYPENAME ||
             otype == FWObjectReference::TYPENAME));
}

void TemplateGroup::fromXML(xmlNodePtr parent) throw(FWException)
{
    FWObject::fromXML(parent);

    const char *n;
    n = FROMXMLCAST(xmlGetProp(parent, TOXMLCAST("inactive")));
    if (NULL != n) {
        setStr("inactive", n);
        FREEXMLBUFF(n);
    }
}

xmlNodePtr TemplateGroup::toXML(xmlNodePtr parent) throw(FWException)
{
    xmlNodePtr me = FWObject::toXML(parent, false);
    xmlNewProp(me, TOXMLCAST("name"), STRTOXMLCAST(getName()));
    xmlNewProp(me, TOXMLCAST("comment"), STRTOXMLCAST(getComment()));

    FWObject *o;

    for (FWObjectTypedChildIterator it = findByType(FWObjectReference::TYPENAME);
         it != it.end(); ++it)
    {
        o = *it;
        if (o) o->toXML(me);
    }

    /*for (FWObjectTypedChildIterator it = findByType(TemplateGroupOptions::TYPENAME);
         it != it.end(); ++it)
    {
        o = *it;
        if (o) o->toXML(me);
    }*/

    if ( (o=getFirstByType( TemplateGroupOptions::TYPENAME )) != NULL )
        o->toXML(me);

    return me;
}

TemplateGroupOptions* TemplateGroup::getOptionsObject()
{
    TemplateGroupOptions *gopt = TemplateGroupOptions::cast(
                getFirstByType(TemplateGroupOptions::TYPENAME));

    if (NULL == gopt)
    {
        gopt = TemplateGroupOptions::cast(
            getRoot()->create(TemplateGroupOptions::TYPENAME));
        gopt->setName("TemplateView");
        add(gopt);
    }

    return gopt;
}

FWObject& TemplateGroup::duplicateForUndo(const FWObject *obj) throw(FWException)
{
    if (NULL == ClusterGroup::constcast(obj)) return *this;

    setRO(false);

    TemplateGroupOptions *their_opts = TemplateGroupOptions::cast(
        obj->getFirstByType(TemplateGroupOptions::TYPENAME));

    TemplateGroupOptions *mine_opts = TemplateGroupOptions::cast(
        getFirstByType(TemplateGroupOptions::TYPENAME));

    list<FWObject *> all_refs = getByType(FWObjectReference::TYPENAME);
    while (all_refs.size())
    {
        remove(all_refs.front(), false);
        all_refs.pop_front();
    }

    for (list<FWObject *>::const_iterator m = obj->begin(); m != obj->end(); ++m)
    {
        if (FWReference::cast(*m))
        {
            FWObject *object = FWReference::getObject(*m);
            addRef(object);
        }
    }

    if (their_opts && mine_opts) mine_opts->duplicate(their_opts);
    if (their_opts && NULL == mine_opts) addCopyOf(their_opts);

    shallowDuplicate(obj);

    return *this;
}

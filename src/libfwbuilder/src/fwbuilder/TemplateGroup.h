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

#ifndef TEMPLATEGROUP_H
#define TEMPLATEGROUP_H

#include "fwbuilder/ObjectGroup.h"

namespace libfwbuilder {

    class TemplateGroupOptions;

    class TemplateGroup : public ObjectGroup
    {
public:
        TemplateGroup();
        virtual ~TemplateGroup() {}

        // Create mandatory child objects
        virtual void init(FWObjectDatabase *root);

        DECLARE_FWOBJECT_SUBTYPE(TemplateGroup)
        DECLARE_DISPATCH_METHODS(TemplateGroup)

        virtual void fromXML(xmlNodePtr parent) throw(FWException);
        virtual xmlNodePtr toXML(xmlNodePtr parent) throw(FWException);

        // Verify given object is of appropriate child type
        virtual bool validateChild(FWObject *o);

        virtual TemplateGroupOptions* getOptionsObject();

        virtual FWObject& duplicateForUndo(const FWObject *obj) throw(FWException);

        virtual bool getInactive() { return getBool("inactive"); }
        virtual void setInactive(bool b) { setBool("inactive", b); }
    };

}

#endif // TEMPLATEGROUP_H

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

#include "../../config.h"
#include "../../definitions.h"
#include "global.h"
#include "utils_no_qt.h"
#include "utils.h"

#include <qobject.h>
#include <qtextstream.h>
#include <qdatetime.h>
#include <qregexp.h>

#include "FWObjectPropertiesFactory.h"
#include "platforms.h"

#include "fwbuilder/AddressRange.h"
#include "fwbuilder/AddressTable.h"
#include "fwbuilder/CustomService.h"
#include "fwbuilder/DNSName.h"
#include "fwbuilder/FWException.h"
#include "fwbuilder/FWObjectDatabase.h"
#include "fwbuilder/FWReference.h"
#include "fwbuilder/Firewall.h"
#include "fwbuilder/Group.h"
#include "fwbuilder/Host.h"
#include "fwbuilder/ICMP6Service.h"
#include "fwbuilder/ICMPService.h"
#include "fwbuilder/IPService.h"
#include "fwbuilder/IPv4.h"
#include "fwbuilder/IPv6.h"
#include "fwbuilder/Interface.h"
#include "fwbuilder/Interval.h"
#include "fwbuilder/Library.h"
#include "fwbuilder/Network.h"
#include "fwbuilder/NetworkIPv6.h"
#include "fwbuilder/ObjectGroup.h"
#include "fwbuilder/Policy.h"
#include "fwbuilder/Resources.h"
#include "fwbuilder/Rule.h"
#include "fwbuilder/TCPService.h"
#include "fwbuilder/TagService.h"
#include "fwbuilder/UDPService.h"
#include "fwbuilder/UserService.h"
#include "fwbuilder/physAddress.h"
#include "fwbuilder/StateSyncClusterGroup.h"
#include "fwbuilder/FailoverClusterGroup.h"
#include "fwbuilder/Cluster.h"

#include <sstream>
#include <iostream>
#include <time.h>


using namespace std;
using namespace libfwbuilder;

/*
 * This method returns brief summary of properties, guaranteed to be 1
 * line of text
 */
QString FWObjectPropertiesFactory::getObjectPropertiesBrief(FWObject *obj)
{
    QString res;
    QTextStream str(&res, QIODevice::WriteOnly);
    FWObject *parent_obj = obj->getParent();

    try
    {
        if (Library::isA(obj))
        {
            if (obj->isReadOnly()) str << "(read only)";

        } else if (IPv4::isA(obj))
        {
            str <<  IPv4::cast(obj)->getAddressPtr()->toString().c_str();
            if (parent_obj && Interface::isA(parent_obj))
            {
                str << "/";
                str << IPv4::cast(obj)->getNetmaskPtr()->toString().c_str();
            }
        } else if (IPv6::isA(obj))
        {
            str <<  IPv6::cast(obj)->getAddressPtr()->toString().c_str();
            if (parent_obj && Interface::isA(parent_obj))
            {
                str << "/";
                str << QString("%1").arg(IPv6::cast(obj)->getNetmaskPtr()->getLength());
            }
        } else if (physAddress::isA(obj))
        {
            str <<  physAddress::cast(obj)->getPhysAddress().c_str();
        } else if (DNSName::isA(obj))
        {
            str << DNSName::cast(obj)->getSourceName().c_str();
        } else if (AddressTable::isA(obj))  
        {
            str << AddressTable::cast(obj)->getSourceName().c_str();
        } else if (AddressRange::isA(obj))
        {
            AddressRange *ar=AddressRange::cast(obj);
            str << ar->getRangeStart().toString().c_str();
            str << " - ";
            str << ar->getRangeEnd().toString().c_str();
        } else if (Firewall::cast(obj))
        {
            if (Firewall::cast(obj)->needsInstall()) str << " * ";
            QString platform = obj->getStr("platform").c_str();
            QString version  = obj->getStr("version").c_str();
            QString readableVersion = getVersionString(platform,version);
            QString hostOS = obj->getStr("host_OS").c_str();
            str <<  platform << "(" << readableVersion << ") / " << hostOS;

        } else if (Host::isA(obj))
        {
            const InetAddr *addr = Address::cast(obj)->getAddressPtr();
            if (addr)
                str <<  addr->toString().c_str();
            else
                str << "(no ip address)";
        } else if (Network::isA(obj))
        {
            Network *n=Network::cast(obj);
            str << n->getAddressPtr()->toString().c_str();
            str << "/";
            str << n->getNetmaskPtr()->toString().c_str();

        } else if (NetworkIPv6::isA(obj))
        {
            NetworkIPv6 *n=NetworkIPv6::cast(obj);
            str << n->getAddressPtr()->toString().c_str();
            str << "/";
            str << QString("%1").arg(n->getNetmaskPtr()->getLength());

        } else if (ClusterGroup::cast(obj)!=NULL)
        {
            ClusterGroup *g = ClusterGroup::cast(obj);
            str << QObject::tr("type: ") << g->getStr("type").c_str();
        } else if (Group::cast(obj)!=NULL)   // just any group
        {
            Group *g=Group::cast(obj);
            str << g->size() << " " << QObject::tr(" objects");

        } else if (Interface::isA(obj))
        {
            Interface *intf = Interface::cast(obj);
            // trigger late initialization of options object
            // if its read-only or part of the read-only tree, I can't help it.
            if (!obj->isReadOnly()) intf->getOptionsObject();
            str << intf->getLabel().c_str();

            QString q;
            if (intf->isDyn())         q =" dyn";
            if (intf->isUnnumbered())  q =" unnum";
            if (intf->isBridgePort())  q =" bridge port";
            if (intf->isSlave())       q =" slave";
            if (intf->isUnprotected()) q = q + " unp";
            if (q!="") str << " (" + q + ")";

        } else if (IPService::isA(obj))
        {
            str << QObject::tr("protocol: %1").arg(obj->getStr("protocol_num").c_str());

        } else if (ICMPService::isA(obj) || ICMP6Service::isA(obj))
        {
            str << QObject::tr("type: %1").arg(obj->getStr("type").c_str())
                << "  "
                << QObject::tr("code: %1").arg(obj->getStr("code").c_str());

        } else if (TCPService::isA(obj) || UDPService::isA(obj))
        {
            int sps,spe,dps,dpe;

            sps=TCPUDPService::cast(obj)->getSrcRangeStart();
            spe=TCPUDPService::cast(obj)->getSrcRangeEnd();
            dps=TCPUDPService::cast(obj)->getDstRangeStart();
            dpe=TCPUDPService::cast(obj)->getDstRangeEnd();

            str << sps << ":" << spe << " / ";
            str << dps << ":" << dpe;
        } else if (TagService::isA(obj)) 
        {
            str << "Pattern: \"" << obj->getStr("tagcode").c_str() << "\"" ;
        } else if (UserService::isA(obj)) 
        {
            const UserService* user_srv = UserService::constcast(obj);
            str << "User id: \"" << user_srv->getUserId().c_str() << "\"" ;
        } else if (Interval::isA(obj))
        {

        }
    } catch (FWException &ex)
    {
        cerr << ex.toString() << endl;
    }

    return res;
}


QString FWObjectPropertiesFactory::getObjectProperties(FWObject *obj)
{
    QString res;
    QTextStream str(&res, QIODevice::WriteOnly);
    FWObject *parent_obj = obj->getParent();

    try
    {
        if (IPv4::isA(obj))
        {
            str <<  IPv4::cast(obj)->getAddressPtr()->toString().c_str();
            if (parent_obj && Interface::isA(parent_obj))
            {
                str << "/";
                str << IPv4::cast(obj)->getNetmaskPtr()->toString().c_str();
            }
        } else if (IPv6::isA(obj))
        {
            str <<  IPv6::cast(obj)->getAddressPtr()->toString().c_str();
            if (parent_obj && Interface::isA(parent_obj))
            {
                str << "/";
                str << QString("%1").arg(IPv6::cast(obj)->getNetmaskPtr()->getLength());
            }
        } else if (physAddress::isA(obj))
        {
            str <<  physAddress::cast(obj)->getPhysAddress().c_str();
        } else if (DNSName::isA(obj))
        {
            str << QObject::tr("DNS record: ")
                << DNSName::cast(obj)->getSourceName().c_str();
        } else if (AddressTable::isA(obj))  
        {
            str << QObject::tr("Address Table: ")
                << AddressTable::cast(obj)->getSourceName().c_str();
        } else if (AddressRange::isA(obj))
        {
            AddressRange *ar=AddressRange::cast(obj);
            str << ar->getRangeStart().toString().c_str();
            str << " - ";
            str << ar->getRangeEnd().toString().c_str();
        } else if (Firewall::cast(obj))
        {
            QString platform = obj->getStr("platform").c_str();
            QString version  = obj->getStr("version").c_str();
            QString readableVersion = getVersionString(platform,version);
            QString hostOS = obj->getStr("host_OS").c_str();

            QDateTime dt;
            time_t t;
            
            t = obj->getInt("lastModified");dt.setTime_t(t);
            QString t_modified  = (t)? dt.toString():"-";
            
            t = obj->getInt("lastCompiled");dt.setTime_t(t);
            QString t_compiled  = (t)? dt.toString():"-";
            
            t = obj->getInt("lastInstalled");dt.setTime_t(t);
            QString t_installed = (t)? dt.toString():"-";
            
            str <<  platform << "(" << readableVersion << ") / " << hostOS;

        } else if (Host::isA(obj))
        {
            const InetAddr *addr = Address::cast(obj)->getAddressPtr();
            if (addr)
                str <<  addr->toString().c_str();
            else
                str << "(no ip address)";

            FWObject *co=obj->getFirstByType("Interface");
            if (co!=NULL) 
            {
                physAddress *paddr=(Interface::cast(co))->getPhysicalAddress();
                if (paddr!=NULL) 
                    str << "    " <<  paddr->getPhysAddress().c_str();
            }

        } else if (Network::isA(obj))
        {
            Network *n=Network::cast(obj);
            str << n->getAddressPtr()->toString().c_str();
            str << "/";
            str << n->getNetmaskPtr()->toString().c_str();

        } else if (NetworkIPv6::isA(obj))
        {
            NetworkIPv6 *n=NetworkIPv6::cast(obj);
            str << n->getAddressPtr()->toString().c_str();
            str << "/";
            str << QString("%1").arg(n->getNetmaskPtr()->getLength());

        } else if (ClusterGroup::cast(obj)!=NULL)
        {
            ClusterGroup *g = ClusterGroup::cast(obj);
            str << QObject::tr("type: ") << g->getStr("type").c_str() << "<br>";
            FWObjectTypedChildIterator j = obj->findByType(FWObjectReference::TYPENAME);
            for ( ; j!=j.end(); ++j)
            {
                FWObject *obj = FWReference::getObject(*j);
                if (Interface::cast(obj))
                {
                    FWObject *fw = obj->getParent();
                    str << QObject::tr("Group member")
                        << " " << fw->getName().c_str() 
                        << ":" << obj->getName().c_str()
                        << "<br>";
                }
            }
        } else if (Group::cast(obj)!=NULL)   // just any group
        {
            Group *g=Group::cast(obj);
            str << g->size() << " " << QObject::tr(" objects");

        } else if (Firewall::cast(obj))
        {

        } else if (Interface::isA(obj))
        {
            Interface *intf = Interface::cast(obj);
            FWObjectTypedChildIterator j = obj->findByType(IPv4::TYPENAME);
            for ( ; j!=j.end(); ++j)
            {
                str << getObjectProperties(*j);
                str << "<br>";
            }
            physAddress *paddr = intf->getPhysicalAddress();
            if (paddr!=NULL) 
                str << " MAC: " << paddr->getPhysAddress().c_str() << "<br>";
            string intf_type = intf->getOptionsObject()->getStr("type");
            if (!intf_type.empty())
                str << " Interface Type: " << intf_type.c_str();

        } else if (IPService::isA(obj))
        {
            str << QObject::tr("protocol: %1").arg(obj->getStr("protocol_num").c_str());

        } else if (ICMPService::isA(obj) || ICMP6Service::isA(obj))
        {
            str << QObject::tr("type: %1").arg(obj->getStr("type").c_str())
                << "  "
                << QObject::tr("code: %1").arg(obj->getStr("code").c_str());

        } else if (TCPService::isA(obj) || UDPService::isA(obj))
        {
            int sps,spe,dps,dpe;

            sps=TCPUDPService::cast(obj)->getSrcRangeStart();
            spe=TCPUDPService::cast(obj)->getSrcRangeEnd();
            dps=TCPUDPService::cast(obj)->getDstRangeStart();
            dpe=TCPUDPService::cast(obj)->getDstRangeEnd();

            str << sps << ":" << spe << " / ";
            str << dps << ":" << dpe;
        } else if (TagService::isA(obj)) 
        {
            str << "Pattern: \"" << obj->getStr("tagcode").c_str() << "\"" ;
        } else if (UserService::isA(obj)) 
        {
            const UserService* user_srv = UserService::constcast(obj);
            str << "User id: \"" << user_srv->getUserId().c_str() << "\"" ;
        } else if (Interval::isA(obj))
        {

        }
    } catch (FWException &ex)
    {
        cerr << ex.toString() << endl;
    }

    return res;
}


QString FWObjectPropertiesFactory::stripHTML(const QString &str)
{
    // note that str may contain multiple lines
    // separated by <br> and/or '\n'

    QRegExp htmltag1 = QRegExp("<[^>]+>");
    QRegExp htmltag2 = QRegExp("</[^>]+>");
    QRegExp htmltd   = QRegExp("</td><td>");

    QString res = str;
    res = res.replace(htmltd,": ");
    res = res.remove(htmltag1);
    res = res.remove(htmltag2);
    return res;
}


QString FWObjectPropertiesFactory::getObjectPropertiesDetailed(FWObject *obj,
                                                               bool showPath,
                                                               bool tooltip,
                                                               bool accentName,
                                                               bool richText)
{
    QString str;
    FWObject *parent_obj = obj->getParent();

    QString path = obj->getPath().c_str();
    path = path.section('/',2,-1);

    if (showPath)
    {
        str += QObject::tr("<b>Library:</b> ");
        str += QString::fromUtf8(obj->getLibrary()->getName().c_str()) + "<br>\n";

        if (!tooltip)
        {
            str += QObject::tr("<b>Object Id:</b> ");
            str += QString(FWObjectDatabase::getStringId(obj->getId()).c_str()) + "<br>\n";
        }
    }

    str += QObject::tr("<b>Object Type:</b> ");
    string d = Resources::global_res->getObjResourceStr(obj,"description");
    str += QString(d.c_str()) + "<br>\n";

    str += QObject::tr("<b>Object Name:</b> ");
    if (accentName) str += "<font color=\"red\">";
    str += QString::fromUtf8(obj->getName().c_str());
    if (accentName) str += "</font>";
    str += "<br>\n";

    try
    {
        if (IPv4::isA(obj))
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            str +=  IPv4::cast(obj)->getAddressPtr()->toString().c_str();
            if (parent_obj && Interface::isA(parent_obj))
            {
                str += "/";
                str += IPv4::cast(obj)->getNetmaskPtr()->toString().c_str();
            }
        } else if (IPv6::isA(obj))
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            str +=  IPv6::cast(obj)->getAddressPtr()->toString().c_str();
            if (parent_obj && Interface::isA(parent_obj))
            {
                str += "/";
                str += QString("%1").arg(IPv6::cast(obj)->getNetmaskPtr()->getLength());
            }
        } else if (physAddress::isA(obj))
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            str += physAddress::cast(obj)->getPhysAddress().c_str();
        } else if (DNSName::isA(obj))
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            str += QObject::tr("<b>DNS record:</b>");
            str += MultiAddress::cast(obj)->getSourceName().c_str();
            str += "<br>\n";
            str += (MultiAddress::cast(obj)->isRunTime())?QObject::tr("Run-time"):QObject::tr("Compile-time");

        } else if (AddressTable::isA(obj))
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            str += QObject::tr("<b>Table file:</b>");
            str += MultiAddress::cast(obj)->getSourceName().c_str();
            str += "<br>\n";
            str += (MultiAddress::cast(obj)->isRunTime())?QObject::tr("Run-time"):QObject::tr("Compile-time");
            
        } else if (AddressRange::isA(obj))
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            AddressRange *ar=AddressRange::cast(obj);
            str += ar->getRangeStart().toString().c_str();
            str += " - ";
            str += ar->getRangeEnd().toString().c_str();
        } else if (Host::isA(obj))
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";

            FWObjectTypedChildIterator j = obj->findByType(
                Interface::TYPENAME);
            for ( ; j!=j.end(); ++j)
            {
                str += (*j)->getName().c_str();
                str += ": ";
                str += getObjectProperties(*j);
                str += "<br>";
            }
        } else if (Network::isA(obj))
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            Network *n=Network::cast(obj);
            str += n->getAddressPtr()->toString().c_str();
            str += "/";
            str += n->getNetmaskPtr()->toString().c_str();

        } else if (NetworkIPv6::isA(obj))
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            NetworkIPv6 *n=NetworkIPv6::cast(obj);
            str += n->getAddressPtr()->toString().c_str();
            str += "/";
            str += QString("%1").arg(n->getNetmaskPtr()->getLength());
        } else if (ClusterGroup::cast(obj)!=NULL)
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            ClusterGroup *g = ClusterGroup::cast(obj);
            str += QObject::tr("type: %1<br>").arg(g->getStr("type").c_str());
            FWObjectTypedChildIterator j = obj->findByType(FWObjectReference::TYPENAME);
            for ( ; j!=j.end(); ++j)
            {
                FWObject *obj = FWReference::getObject(*j);
                if (Interface::cast(obj))
                {
                    FWObject *fw = obj->getParent();
                    str += QObject::tr("Group member %1:%2<br>").
                        arg(fw->getName().c_str()).arg(obj->getName().c_str());
                }
            }
        } else if (Group::cast(obj)!=NULL)   // just any group
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            Group *g = Group::cast(obj);
            str += QObject::tr("%1 objects<br>\n").arg(g->size());
            int n = 0;
            list<FWObject*> ll = *g;
            ll.sort(FWObjectNameCmpPredicate());

            for (FWObject::iterator i=ll.begin(); i!=ll.end(); ++i,++n)
            {
                if (n>20)  // arbitrary number
                {
                    str += "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;.&nbsp;.&nbsp;.&nbsp;";
                    break;
                } else 
                {
                    FWObject *o1=*i;
                    if (FWReference::cast(o1)!=NULL)
                        o1=FWReference::cast(o1)->getPointer();
                    str += QString(o1->getTypeName().c_str())
                        + "  <b>" + QString::fromUtf8(o1->getName().c_str()) + "</b><br>\n";
                }
            }
        } else if (Firewall::cast(obj)) 
        {
            // Note: Firewall::cast(obj) matched Firewall and Cluster
            QString platform = obj->getStr("platform").c_str();
            QString version  = obj->getStr("version").c_str();
            QString readableVersion = getVersionString(platform,version);
            QString hostOS = obj->getStr("host_OS").c_str();
            
            QDateTime dt;
            time_t lm=obj->getInt("lastModified");
            time_t lc=obj->getInt("lastCompiled");
            time_t li=obj->getInt("lastInstalled");
            
            dt.setTime_t(lm);
            QString t_modified  = (lm)? dt.toString():"-";
            if (lm>lc && lm>li) t_modified=QString("<b>")+t_modified+"</b>";
                    
            dt.setTime_t(lc);
            QString t_compiled  = (lc)? dt.toString():"-";
            if (lc>lm && lc>li) t_compiled=QString("<b>")+t_compiled+"</b>";
            
            dt.setTime_t(li);
            QString t_installed = (li)? dt.toString():"-";
            if (li>lc && li>lm) t_installed=QString("<b>")+t_installed+"</b>";
            
               

            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            str += "<table cellspacing=\"0\" cellpadding=\"0\">";
            str += QString("<tr><td>Platform:</td><td>") +
                platform + "</td></tr>\n";
            str += QString("<tr><td>Version:</td><td>")  +
                readableVersion + "</td></tr>\n";
            str += QString("<tr><td>Host OS:</td><td>")  +
                hostOS + "</td></tr>\n";
            
            str += QString("<tr><td>Modified:</td><td>")  +
                t_modified + "</td></tr>\n";
            str += QString("<tr><td>Compiled:</td><td>")  +
                t_compiled + "</td></tr>\n";
            str += QString("<tr><td>Installed:</td><td>")  +
                t_installed + "</td></tr>\n";
            
            str += "</table>";
        } else if (Interface::isA(obj))
        {
            Interface *intf = Interface::cast(obj);
            //str += QObject::tr("<b>Path:</b> ")+ path +"<br>\n";

            FWObjectTypedChildIterator j = obj->findByType(IPv4::TYPENAME);
            for ( ; j!=j.end(); ++j)
            {
                str += getObjectProperties(*j);
                str += "<br>";
            }

            string intf_type = intf->getOptionsObject()->getStr("type");
            if (!intf_type.empty())
            {
                str += "<b>Interface Type: </b>";
                str += intf_type.c_str();
                if (intf_type == "8021q")
                {
                    int vlan_id = intf->getOptionsObject()->getInt("vlan_id");
                    str += QString(" VLAN ID=%1").arg(vlan_id);
                }
                str += "<br>";
            }

            physAddress *paddr = intf->getPhysicalAddress();
            if (paddr!=NULL) 
            {
                str += "MAC: ";
                str +=  paddr->getPhysAddress().c_str() ;
                str += "<br>";
            }

            QString q;
            if (intf->isDyn())        q=" dyn";
            if (intf->isUnnumbered()) q=" unnum";
            if (intf->isBridgePort()) q=" bridge port";
            
            FWObject *p=obj;
            while (p!=NULL && !Firewall::cast(p)) p=p->getParent();
            if (p!=NULL && (p->getStr("platform")=="pix" || p->getStr("platform")=="fwsm"))
            {
                int sl = intf->getSecurityLevel();
                q=q+QString("sec.level %1").arg(sl);
            }

            if (intf->isUnprotected())    q=q+" unp";
            
            if (q!="") str += " (" + q + ")";
            str += "<br>\n";
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";

        } else if (CustomService::isA(obj))
        {

            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";

            CustomService *s = dynamic_cast<CustomService*>(obj);
            bool first=true;

            map<string,string> platforms = Resources::getPlatforms();
            for (map<string,string>::iterator i=platforms.begin(); i!=platforms.end(); i++)
            {
                string c=s->getCodeForPlatform( (*i).first );
                if ( c!="" )
                {
                    if (first)
                    {
                        str += "<table cellspacing=\"0\" cellpadding=\"0\">";
                        first=false;
                    }
                    str += QString("<tr><td>%1</td><td>%2</td></tr>\n")
                        .arg((*i).second.c_str()).arg(c.c_str());
                }
            }
            if (!first) str += "</table>";

        } else if (IPService::isA(obj))
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            str += QObject::tr("protocol ") + obj->getStr("protocol_num").c_str();

        } else if (ICMPService::isA(obj) || ICMP6Service::isA(obj))
        {
            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            str += QObject::tr("type: ") + obj->getStr("type").c_str()
                + "  "
                + QObject::tr("code: ") + obj->getStr("code").c_str();

        } else if (TCPService::isA(obj) || UDPService::isA(obj))
        {
            int sps,spe,dps,dpe;

            sps=TCPUDPService::cast(obj)->getSrcRangeStart();
            spe=TCPUDPService::cast(obj)->getSrcRangeEnd();
            dps=TCPUDPService::cast(obj)->getDstRangeStart();
            dpe=TCPUDPService::cast(obj)->getDstRangeEnd();

            if (showPath && !tooltip) str += "<b>Path: </b>" + path + "<br>\n";
            str += "<table cellspacing=\"0\" cellpadding=\"0\">";
            str += QString("<tr><td>source port range</td><td>%1:%2</td></tr>\n")
                .arg(sps).arg(spe);
            str += QString("<tr><td>destination port range</td><td>%1:%2</td></tr>\n")
                .arg(dps).arg(dpe);
            str += "</table>";
        } else if (TagService::isA(obj)) 
        {
            str += QObject::tr("Pattern: \"%1\"").arg(obj->getStr("tagcode").c_str());
        } else if (UserService::isA(obj)) 
        {
            const UserService* user_srv = UserService::constcast(obj);
            str += QObject::tr("User id: \"%1\"").arg(user_srv->getUserId().c_str());
        } else if (Interval::isA(obj))
        {

        }
    } catch (FWException &ex)
    {
        cerr << ex.toString() << endl;
    }

    if (richText) return str;

    return FWObjectPropertiesFactory::stripHTML(str);
}

/*
 * Do not translate literals 'pipe', 'queue', 'divert' below, these refer
 * to actual ipfw parameters and should not be localized.
 */
QString FWObjectPropertiesFactory::getRuleActionProperties(PolicyRule *rule)
{
    QString par = "";

    if (rule!=NULL)
    {
        string act = rule->getActionAsString();
        
        FWObject *o = rule;
        while (o!=NULL && Firewall::cast(o)==NULL) o=o->getParent();
        if (o==NULL) return "";

        Firewall *f=Firewall::cast(o);
        string platform=f->getStr("platform");
        
        FWOptions *ropt = rule->getOptionsObject();
        string editor=Resources::getActionEditor(platform,act);

        if (editor!="None")
        {
            switch (rule->getAction())
            {
            case PolicyRule::Reject:
                par = ropt->getStr("action_on_reject").c_str();
                break;
            case PolicyRule::Tag:
            {
                FWObject *tag_object = rule->getTagObject();
                if (tag_object)
                    par = tag_object->getName().c_str();
                else
                    par = rule->getTagValue().c_str();
                break;
            }
            case PolicyRule::Accounting :
                par = ropt->getStr("rule_name_accounting").c_str();
                break;
            case PolicyRule::Custom:
                par = ropt->getStr("custom_str").c_str();
                break;
            case PolicyRule::Branch:
            {
                FWObject *branch_ruleset = rule->getBranch();
                if (branch_ruleset)
                    par = branch_ruleset->getName().c_str();
                // ropt->getStr("branch_name").c_str();
                break;
            }
            case PolicyRule::Classify:
                if (platform=="ipfw")
                {
                    if (ropt->getInt("ipfw_classify_method") == DUMMYNETPIPE)
                    {
                        par = "pipe";
                    } else {
                        par = "queue";
                    }
                    par = par + " " + ropt->getStr("ipfw_pipe_queue_num").c_str();
                } else
                {
                    par = ropt->getStr("classify_str").c_str();
                }
                break;
            case PolicyRule::Pipe :
                if (platform=="ipfw")
                {
                    par = QString("divert ") + 
                        ropt->getStr("ipfw_pipe_port_num").c_str();
                }
                break;
            case PolicyRule::Route :
                if (platform=="iptables")
                {
                    string a;
                    a = ropt->getStr("ipt_gw");
                    if (!a.empty()) par = par + " gw: " + a.c_str();
                    a = ropt->getStr("ipt_iif");
                    if (!a.empty()) par = par + " iif: " + a.c_str();
                    a = ropt->getStr("ipt_oif");
                    if (!a.empty()) par = par + " oif: " + a.c_str();
                }
                if (platform=="ipf")
                {
                    string a;
                    a = ropt->getStr("ipf_route_option");
                    if (!a.empty())
                    {
                        par = par + " "+ 
                            getScreenName(a.c_str(),
                                          getRouteOptions_pf_ipf( platform.c_str() ));
                    }
                    a = ropt->getStr("ipf_route_opt_if");
                    if (!a.empty()) par = par + " "+ a.c_str();
                    a = ropt->getStr("ipf_route_opt_addr");
                    if (!a.empty()) par = par + " "+ a.c_str();
                }
                if (platform=="pf")
                {
                    string a;
                    a = ropt->getStr("pf_route_option");
                    if (!a.empty()) par = par + " "+ a.c_str();
                    a = ropt->getStr("pf_route_opt_if");
                    if (!a.empty()) par = par + " "+ a.c_str();
                    a = ropt->getStr("pf_route_opt_addr");
                    if (!a.empty()) par = par + " "+ a.c_str();
                }
                break;


            default : {}
            }
        }
        
    }
    
    return par;
}

QString FWObjectPropertiesFactory::getRuleActionPropertiesRich(PolicyRule *rule)
{
    FWObject *p=rule;
    while (p!=NULL && !Firewall::cast(p)) p=p->getParent();
    assert(p!=NULL);
    string platform=p->getStr("platform"); 
    QString act = getActionNameForPlatform(rule->getAction(),platform.c_str());

    QString par = getRuleActionProperties(rule);
    QString res = QObject::tr("<b>Action   :</b> ")+act+"<br>\n";
    if (!par.isEmpty())
    {
        res+=QObject::tr("<b>Parameter:</b> ")+par;
    }
    return res;
}

QString FWObjectPropertiesFactory::getPolicyRuleOptions(Rule *rule)
{
    QString res;
    
    if (rule!=NULL)
    {
        res="";
        FWObject *o = rule;
        while (o!=NULL && Firewall::cast(o)==NULL) o=o->getParent();
        assert(o!=NULL);
        Firewall *f=Firewall::cast(o);
        string platform=f->getStr("platform");
        FWOptions *ropt = rule->getOptionsObject();
        
        if (platform=="iptables")
        {
            if (!ropt->getStr("log_prefix").empty())
            {
                res+=QObject::tr("<b>Log prefix    :</b> ");
                res+=QString(ropt->getStr("log_prefix").c_str())+"<br>\n";
            }

            if (!ropt->getStr("log_level").empty())
            {
                res+=QObject::tr("<b>Log Level     :</b> ");
                res+=getScreenName(ropt->getStr("log_level").c_str(),
                        getLogLevels(platform.c_str()))+"<br>\n";
            }

            if (ropt->getInt("ulog_nlgroup")>1)
            {
                res+=QObject::tr("<b>Netlink group :</b> ");
                res+=QString(ropt->getStr("ulog_nlgroup").c_str())+"<br>\n";
            }
            
            if (ropt->getInt("limit_value")>0)
            {
                res+=QObject::tr("<b>Limit Value   :</b> ");
                res+=QString(ropt->getStr("limit_value").c_str())+"<br>\n";
            }
            
            if (!ropt->getStr("limit_suffix").empty())
            {
                res+=QObject::tr("<b>Limit suffix  :</b> ");
                res+=getScreenName(ropt->getStr("limit_suffix").c_str(),
                        getLimitSuffixes(platform.c_str()))+"<br>\n";
            }
            
            if (ropt->getInt("limit_burst")>0)
            {
                res+=QObject::tr("<b>Limit burst   :</b> ");
                res+=QString(ropt->getStr("limit_burst").c_str())+"<br>\n";
            }
            
            res+="<ul>";
            if (ropt->getBool("firewall_is_part_of_any_and_networks"))
            {
                res+=QObject::tr("<li><b>Part of Any</b></li>");
                res+="<br>\n";
            }

            if (ropt->getBool("stateless"))
            {
                res+=QObject::tr("<li><b>Stateless</b></li> ");
                res+="<br>\n";
            }
            res+="</ul>";
            
        }else if (platform=="ipf") 
        {
            if (!ropt->getStr("ipf_log_facility").empty())
            {
                res+=QObject::tr("<b>Log facility:</b> ");
                res+=getScreenName(ropt->getStr("ipf_log_facility").c_str(),
                    getLogFacilities(platform.c_str()))+"<br>\n";
            }
            
            if (!ropt->getStr("log_level").empty())
            {
                res+=QObject::tr("<b>Log level   :</b> ");
                res+=getScreenName(ropt->getStr("log_level").c_str(),
                    getLogLevels(platform.c_str()))+"<br>\n";
            }
            
            res+="<ul>";
            if (ropt->getBool("ipf_return_icmp_as_dest"))
            {
                res+=QObject::tr("<li><b>Send 'unreachable'</b></li>");
                res+="<br>\n";
            }

            if (ropt->getBool("stateless"))
            {
                res+=QObject::tr("<li><b>Stateless</b></li> ");
                res+="<br>\n";
            }

            if (ropt->getBool("ipf_keep_frags"))
            {
                res+=QObject::tr("<li><b>Keep information on fragmented packets</b></li> ");
                res+="<br>\n";
            }
            res+="</ul>";
            
        }else if (platform=="pf")
        {
            
            if (!ropt->getStr("log_prefix").empty())
            {
                res+=QObject::tr("<b>Log prefix :</b> ");
                res+=QString(ropt->getStr("log_prefix").c_str())+"<br>\n";
            }
            
            if (ropt->getInt("pf_rule_max_state")>0)
            {
                res+=QObject::tr("<b>Max state  :</b> ");
                res+=QString(ropt->getStr("pf_rule_max_state").c_str())+"<br>\n";
            }
            
            res+="<ul>";
            if (ropt->getBool("stateless"))
            {
                res+=QObject::tr("<li><b>Stateless</b></li> ");
                res+="<br>\n";
            }
            
            if (ropt->getBool("pf_source_tracking"))
            {
                res+=QObject::tr("<li><b>Source tracking</b></li> ");
                res+="<br>\n";
                
                res+=QObject::tr("<b>Max src nodes :</b> ");
                res+=QString(ropt->getStr("pf_max_src_nodes").c_str())+"<br>\n";
                
                res+=QObject::tr("<b>Max src states:</b> ");
                res+=QString(ropt->getStr("pf_max_src_states").c_str())+"<br>\n";
                
            }
            res+="</ul>";
            
        }else if (platform=="ipfw")
        {
            res+="<ul>";
            if (ropt->getBool("stateless"))
            {
                res+=QObject::tr("<li><b>Stateless</b></li> ");
                res+="<br>\n";
            }
            res+="</ul>";
            
        }else if (platform=="pix" || platform=="fwsm")
        {
            string vers="version_"+f->getStr("version");
            
            res+=QObject::tr("<u><b>Ver:%1</b></u><br>\n").arg(vers.c_str());
            
            if ( Resources::platform_res[platform]->getResourceBool(
                  "/FWBuilderResources/Target/options/"+vers+"/pix_rule_syslog_settings"))
            {
                
                if (!ropt->getStr("log_level").empty())
                {
                    res+=QObject::tr("<b>Log level   :</b> ");
                    res+=getScreenName(ropt->getStr("log_level").c_str(),
                        getLogLevels(platform.c_str()))+"<br>\n";
                }
                if (ropt->getInt("log_interval")>0)
                {
                    res+=QObject::tr("<b>Log interval  :</b> ");
                    res+=QString(ropt->getStr("log_interval").c_str())+"<br>\n";
                }
                
                res+="<ul>";
                if (ropt->getBool("disable_logging_for_this_rule"))
                {
                    res+=QObject::tr("<li><b>Disable logging for this rule</b></li> ");
                    res+="<br>\n";
                }
                res+="</ul>";
                
            } 
        }
        
    }
    
    return res;
}

QString FWObjectPropertiesFactory::getNATRuleOptions(Rule *rule)
{
    QString res;
    
    if (rule!=NULL)
    {
        res="";
        FWObject *o = rule;
        while (o!=NULL && Firewall::cast(o)==NULL) o=o->getParent();
        assert(o!=NULL);
        Firewall *f=Firewall::cast(o);
        string platform=f->getStr("platform");
        FWOptions *ropt = rule->getOptionsObject();

        if (fwbdebug)
            qDebug(QString("getNATRuleOptions: platform: %2").arg(platform.c_str()).toAscii().constData());
            
        if (platform=="pf")
        {
            if (ropt->getBool("pf_bitmask"))      res+=QObject::tr("bitmask");
            if (ropt->getBool("pf_random"))       res+=QObject::tr("random");
            if (ropt->getBool("pf_source_hash"))  res+=QObject::tr("source-hash");
            if (ropt->getBool("pf_round_robin"))  res+=QObject::tr("round-robin");
            if (!res.isEmpty()) res += ",";
            if (ropt->getBool("pf_static_port"))  res+=QObject::tr("static-port");
        }
    }
    
    return res;
}


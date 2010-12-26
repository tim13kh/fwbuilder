/* 

                  Firewall Builder Routing add-on

                 Copyright (C) 2004 Compal GmbH, Germany

  Author:  Tidei Maurizio     <fwbuilder-routing at compal.de>
  
  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/

#include <assert.h>

#include "RoutingCompiler.h"

#include "fwbuilder/AddressRange.h"
#include "fwbuilder/RuleElement.h"
#include "fwbuilder/Network.h"
#include "fwbuilder/IPService.h"
#include "fwbuilder/ICMPService.h"
#include "fwbuilder/TCPService.h"
#include "fwbuilder/UDPService.h"
#include "fwbuilder/CustomService.h"
#include "fwbuilder/Routing.h"
#include "fwbuilder/Rule.h"
#include "fwbuilder/Firewall.h"
#include "fwbuilder/Cluster.h"
#include "fwbuilder/RuleSet.h"
#include "fwbuilder/InetAddr.h"
#include "fwbuilder/IPRoute.h"
#include "fwbuilder/Interface.h"
#include "fwbuilder/IPv4.h"
#include "fwbuilder/FWObjectDatabase.h"
#include "fwbuilder/XMLTools.h"
#include "fwbuilder/FWException.h"
#include "fwbuilder/Group.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <assert.h>

 
using namespace fwcompiler;
using namespace libfwbuilder;
using namespace std;

int RoutingCompiler::prolog()
{
    Compiler::prolog();
    
    Routing *routing = Routing::cast(fw->getFirstByType(Routing::TYPENAME));
    assert(routing);

    combined_ruleset = new Routing();   // combined ruleset
    fw->add( combined_ruleset );

    temp_ruleset = new Routing();   // working copy of the routing
    fw->add( temp_ruleset );

    combined_ruleset->setName(routing->getName());
    temp_ruleset->setName(routing->getName());

    routing->renumberRules();

    list<FWObject*> l = routing->getByType(RoutingRule::TYPENAME);
    for (list<FWObject*>::iterator j=l.begin(); j!=l.end(); ++j) 
    {
	Rule *r= Rule::cast(*j);
        if (r == NULL) continue; // skip RuleSetOptions object

        /*
         * do not remove disabled rules just yet because some
         * compilers might use RuleSet::insertRuleAtTop() and other
         * similar methods from prolog() or
         * addPredefinedPolicyRules()() and these methods renumber
         * rules (labels stop matching rule positions when this is
         * done because labels are configured in prolog() method of
         * the base class. See fwbuilder ticket 1173)
         */
	//if (r->isDisabled()) continue;

	r->setInterfaceId(-1);
	r->setLabel( createRuleLabel("", "main", r->getPosition()) );
	combined_ruleset->add( r );
    }

    initialized=true;
    
    return combined_ruleset->size();
}


bool RoutingCompiler::cmpRules(const RoutingRule &r1,
                              const RoutingRule &r2)
{
    if (r1.getRDst()!=r2.getRDst()) return false;
    if (r1.getRGtw()!=r2.getRGtw()) return false;
    if (r1.getRItf()!=r2.getRItf()) return false;

    return true;
}


string RoutingCompiler::debugPrintRule(Rule *r)
{
    RoutingRule *rule=RoutingRule::cast(r);

    RuleElementRDst *dstrel=rule->getRDst();
    RuleElementRItf *itfrel=rule->getRItf();
    RuleElementRGtw *gtwrel=rule->getRGtw();

    ostringstream str;

//    str << setw(70) << setfill('-') << "-";

    string dst, itf, gtw;
   
    FWObject *obj = FWReference::getObject(itfrel->front());
    itf = obj->getName();

    obj = FWReference::getObject(gtwrel->front());
    gtw = obj->getName();    
     
    
    int no=0;
    FWObject::iterator i1=dstrel->begin();
    while ( i1!=dstrel->end())
    {
        str << endl;

        dst = " ";

        if (i1!=dstrel->end())
        {
            FWObject *o = FWReference::getObject(*i1);
            dst = o->getName();
        }

        int w=0;
        if (no==0)
        {
            str << rule->getLabel();
            w = rule->getLabel().length();
        }
        
        str <<  setw(10-w)  << setfill(' ') << " ";

        str <<  setw(18) << setfill(' ') << dst.c_str() << " ";
        str <<  setw(18) << setfill(' ') << itf.c_str() << " ";
        str <<  setw(18) << setfill(' ') << gtw.c_str() << " ";
        str <<  setw(18) << setfill(' ') << " ";

        ++no;

        if ( i1!=dstrel->end() ) ++i1;
    }
    return str.str();
}

bool RoutingCompiler::ExpandMultipleAddresses::processNext()
{
    RoutingRule *rule=getNext(); if (rule==NULL) return false;
    tmp_queue.push_back(rule);

    RuleElementRDst *dst = rule->getRDst();    assert(dst);
    RuleElementRGtw *gtw = rule->getRGtw();    assert(gtw);

    compiler->_expand_addr(rule, dst, true);
    compiler->_expand_addr(rule, gtw, false);
    return true;
}

bool RoutingCompiler::ConvertToAtomicForDST::processNext()
{
    RoutingRule *rule=getNext(); if (rule==NULL) return false;

    //RuleElementSrc *src=rule->getSrc();    assert(src);
    RuleElementRDst *dst=rule->getRDst();    assert(dst);


    for (FWObject::iterator it=dst->begin(); it!=dst->end(); ++it)
    {
        RoutingRule *r = compiler->dbcopy->createRoutingRule();
        r->duplicate(rule);
        compiler->temp_ruleset->add(r);

        FWObject *s;
        //s=r->getSrc();	assert(s);
        //s->clearChildren();
        //s->add( *i1 );

        s=r->getRDst();	assert(s);
        s->clearChildren();
        s->add( *it );

        tmp_queue.push_back(r);
    }

    return true;
}


bool RoutingCompiler::ExpandGroups::processNext()
{
    RoutingRule *rule=getNext(); if (rule==NULL) return false;
    tmp_queue.push_back(rule);

    RuleElementRDst *dst=rule->getRDst();   assert(dst);
    compiler->expandGroupsInRuleElement(dst);
    return true;
}


bool RoutingCompiler::emptyRDstAndRItf::processNext()
{
    RoutingRule *rule=getNext(); if (rule==NULL) return false;
    tmp_queue.push_back(rule);
    
    
    RuleElementRGtw *gtwrel=rule->getRGtw();
    RuleElementRItf *itfrel=rule->getRItf();

    if ( (FWReference::cast(itfrel->front())->getPointer())->getName()=="Any" &&\
         (FWReference::cast(gtwrel->front())->getPointer())->getName()=="Any")
    {
        string msg;
        msg = "Gateway and interface are both empty in the rule";
        compiler->abort(rule, msg.c_str());
    }

    return true;
}


bool RoutingCompiler::singleAdressInRGtw::processNext()
{
    RoutingRule *rule=getNext(); if (rule==NULL) return false;
    tmp_queue.push_back(rule);
    
    RuleElementRGtw *gtwrel=rule->getRGtw();

    FWObject *o = FWReference::getObject(gtwrel->front());
              
    if( gtwrel->checkSingleIPAdress(o) == false)
    {       
        string msg;
        msg = "Object \"" + o->getName() +
            "\" used as a gateway in the routing rule " +
            rule->getLabel() + " has multiple ip adresses";
        compiler->abort(rule, msg.c_str());
    }
    return true;
}

// recursive network validity check
bool RoutingCompiler::validateNetwork::checkValidNetwork(FWObject *o) {
    
    if( Network::cast(o) != NULL) {
        return ((Network *)o)->isValidRoutingNet();
    }
    
    /* if we have a group containing networks and groups, we want to check them too */
    if( ObjectGroup::cast(o) != NULL) {
        
        FWObjectTypedChildIterator child_i = o->findByType(FWObjectReference::TYPENAME);
        for ( ; child_i != child_i.end(); ++child_i) {
            FWObjectReference *child_r = FWObjectReference::cast(*child_i);
            assert(child_r);
            FWObject *child = child_r->getPointer();
            
            Network *network;
            ObjectGroup *group;
            
            // Network
            if ((network=Network::cast(child)) != NULL) {
                if (checkValidNetwork(network) == false) {
                    return false;
                }
            } else if ((group=ObjectGroup::cast(child)) != NULL) { // Group
                if (checkValidNetwork(group) == false) {
                    return false;
                }
            }
        }
    }
    
    return true;    
}

// Invalid routing destination network: network address and netmask mismatch. 
bool RoutingCompiler::validateNetwork::processNext()
{
    RoutingRule *rule=getNext(); if (rule==NULL) return false;
    
    tmp_queue.push_back(rule);
    
    RuleElementRDst *dstrel=rule->getRDst();
    FWObject *o = FWReference::cast(dstrel->front())->getPointer();
     
    if( checkValidNetwork(o) == false) {
    
        string msg;
        msg = "Object \"" + o->getName() +
            "\" used as destination in the routing rule " +
            rule->getLabel() + " has invalid netmask";
        compiler->abort(rule, msg.c_str());
    }
    return true;
    
}

// the IP address of the gateway has to be in a local network of the firewall
bool RoutingCompiler::reachableAddressInRGtw::checkReachableIPAddress(FWObject *o)
{
    // let's walk over all interfaces of this firewall
    list<FWObject*> interfaces = compiler->fw->getByTypeDeep(Interface::TYPENAME);
    list<FWObject*>::iterator intf;

    if( Host::cast(o) != NULL)
    {
        Host *host=Host::cast(o);
        const InetAddr *ip_host = host->getAddressPtr();
        for (intf = interfaces.begin(); intf!=interfaces.end(); ++intf)
        {
            Interface *i_firewall = Interface::cast(*intf);
            for(FWObjectTypedChildIterator fw_ips = 
                    i_firewall->findByType(IPv4::TYPENAME);
                fw_ips!=fw_ips.end(); ++fw_ips)
            {
                IPv4 *ipv4_obj_firewall = IPv4::cast(*fw_ips);

                const InetAddr *addr = ipv4_obj_firewall->getAddressPtr();
                const InetAddr *netm = ipv4_obj_firewall->getNetmaskPtr();
                if (addr)
                {
                    InetAddrMask fw_net(*addr, *netm);
                    if (fw_net.belongs(*ip_host))
                        return true;
                }
            }
        }
        return false;

    } else if( Interface::cast(o) != NULL)
    {
        Interface *gw_interface=Interface::cast(o);
        const InetAddr *ip_gateway = gw_interface->getAddressPtr();

        // walk over all interfaces of this firewall
        for (intf = interfaces.begin(); intf!=interfaces.end(); ++intf)
        {
            Interface *if_firewall=Interface::cast(*intf);
            FWObjectTypedChildIterator addresses =
                if_firewall->findByType(IPv4::TYPENAME);

            // check all IPv4 addresses of this firewall interface
            for ( ; addresses!=addresses.end(); ++addresses )
            {
                IPv4 *ipv4_obj_firewall = IPv4::cast(*addresses);
                const InetAddr *addr = ipv4_obj_firewall->getAddressPtr();
                const InetAddr *netm = ipv4_obj_firewall->getNetmaskPtr();
                if (addr)
                {
                    InetAddrMask fw_net(*addr, *netm);
                    if (fw_net.belongs(*ip_gateway))
                        return true;
                }
            }
        }
        return false;

    } else if( IPv4::cast(o) != NULL)
    {
        IPv4 *ipv4=IPv4::cast(o);
        const InetAddr *ip_ipv4 = ipv4->getAddressPtr();

        for (intf = interfaces.begin(); intf!=interfaces.end(); ++intf)
        {
            Interface *if_firewall=Interface::cast(*intf);
            FWObjectTypedChildIterator addresses =
                if_firewall->findByType(IPv4::TYPENAME);

            // check all IPv4 addresses of this firewall interface
            for ( ; addresses!=addresses.end(); ++addresses ) {
                IPv4 *ipv4_obj_firewall = IPv4::cast(*addresses);
                const InetAddr *addr = ipv4_obj_firewall->getAddressPtr();
                const InetAddr *netm = ipv4_obj_firewall->getNetmaskPtr();
                if (addr)
                {
                    InetAddrMask fw_net(*addr, *netm);
                    if (fw_net.belongs(*ip_ipv4))
                        return true;
                }
            }
        } return false;

    } else return true;

    return false;
}


//  the IP address of the gateway has to be in a local network of the firewall
bool RoutingCompiler::reachableAddressInRGtw::processNext()
{
    RoutingRule *rule=getNext(); if (rule==NULL) return false;
    tmp_queue.push_back(rule);
    
    RuleElementRGtw *gtwrel=rule->getRGtw();
    FWObject *o = FWReference::cast(gtwrel->front())->getPointer();
              
    if( checkReachableIPAddress(o) == false)
    {
        string msg;
        msg = "Object \"" + o->getName() +
            "\" used as gateway in the routing rule " +
            rule->getLabel() +
            " is not reachable because it is not in any local network of the firewall";
        compiler->abort(rule, msg.c_str());
    }
    return true;
}

// the IP address of the gateway RGtw has to be in a network of the interface RItf
bool RoutingCompiler::contradictionRGtwAndRItf::processNext()
{
    RoutingRule *rule=getNext(); if (rule==NULL) return false;
    
    tmp_queue.push_back(rule);
    
    RuleElementRGtw *gtwrel=rule->getRGtw();
    RuleElementRItf *itfrel=rule->getRItf();
    
    FWObject *oRGtw = FWReference::cast(gtwrel->front())->getPointer();
    FWObject *oRItf = FWReference::cast(itfrel->front())->getPointer();
    
    if (oRItf->getName() == "Any") { return true; }
    
    
    if (Host::cast(oRGtw) != NULL ||
        Interface::cast(oRGtw) != NULL ||
        Address::cast(oRGtw)->dimension()==1)
    {

        const InetAddr* ip_interface = NULL;

        if ( Host::cast(oRGtw) != NULL)
        {
            Host *host=Host::cast(oRGtw);
            ip_interface = host->getAddressPtr();
        } else if (Interface::cast(oRGtw) != NULL)
        {
            Interface *intf=Interface::cast(oRGtw);
            ip_interface = intf->getAddressPtr();
        } else if (Address::cast(oRGtw)->dimension()==1)
        {
            Address *ipv4 = Address::cast(oRGtw);
            ip_interface = ipv4->getAddressPtr();
        }

        if (ip_interface)
        {
            list<FWObject*> obj_list = oRItf->getByType(IPv4::TYPENAME);
            for (list<FWObject*>::iterator i=obj_list.begin();
                 i!=obj_list.end(); ++i) 
            {
                Address *addr = Address::cast(*i);
                if (addr->belongs(*ip_interface))
                    return true;
            }
        }

        string msg;
        msg = "Object \"" + oRGtw->getName() +
            "\" used as gateway in the routing rule " +
            rule->getLabel() +
            " is not in the same local network as interface " +
            oRItf->getName();
        compiler->abort(rule, msg.c_str());
    }

    return true;

}

bool RoutingCompiler::rItfChildOfFw::processNext()
{
    RoutingRule *rule=getNext(); if (rule==NULL) return false;
    tmp_queue.push_back(rule);
    
    RuleElementRItf *itfrel = rule->getRItf();

    if (itfrel->isAny()) return true;

    FWObject *o = FWReference::cast(itfrel->front())->getPointer();
    if (o->isChildOf(compiler->fw)) return true;

    // the interface is not a child of the firewall. Could be
    // cluster interface though. In that case make sure the
    // firewall is a member of that cluster.
    Interface *iface = Interface::cast(o);
    if (iface)
    {
        Cluster *cluster = Cluster::cast(iface->getParentHost());
        if (cluster)
        {            
            list<Firewall*> members;
            cluster->getMembersList(members);
            if (std::find(members.begin(), members.end(), compiler->fw) != members.end())
                return true;
        }
    }
    string msg;
    msg = "Object \"" + o->getName() + 
        "\" used as interface in the routing rule " +
        rule->getLabel() +
        " is not a child of the firewall the rule belongs to";
    compiler->abort(rule, msg.c_str());
    // even though we call abort() here, it does not actually stop the
    // program if it runs in the test mode.
    return true;
}


bool RoutingCompiler::competingRules::processNext()
{
    RoutingRule *rule=getNext(); if (rule==NULL) return false;

        
    RuleElementRItf *itfrel=rule->getRItf();
    FWObject *itf = FWReference::cast(itfrel->front())->getPointer();
    
    RuleElementRGtw *gtwrel=rule->getRGtw();
    FWObject *gtw = FWReference::cast(gtwrel->front())->getPointer();
     
    string metric = rule->getMetricAsString();
    string label  = rule->getSortedDstIds();
    ostringstream ostr;
    ostr << gtw->getId() << "_" << itf->getId();
    string combiId = ostr.str();
    
    if( label == "") compiler->abort(
        
            rule,         
            "Place 'createSortedDstIdsLabel()' before 'competingRules()' "
            "in the rule processor chain");
    
    dest_it = rules_seen_so_far.find(label);
    if( dest_it != rules_seen_so_far.end()) {
        
        // a rule with the same destination was already seen
        ///std::cout << "NO NEW DEST" << std::endl;
        
        gtwitf_it = dest_it->second.find(combiId);
        if( gtwitf_it != dest_it->second.end() )
        {
            // ... this gateway and interface combination were already
            // seen for this destination
            ///std::cout << "NO NEW GTWITF" << std::endl;
            
            if( gtwitf_it->second.first == metric) {
                
                // ... and same metric => rule already exists, skip
                ///std::cout << "SAME METRIC" << std::endl;
                
                string msg;
                msg = "Routing rules " + gtwitf_it->second.second +
                    " and " + rule->getLabel() +
                    " are identical, skipping the second one. " +
                    "Delete one of them to avoid this warning";
                compiler->warning(rule,  msg.c_str());
            } else {
                
                // ... but different metric => what metric should I use? => abort
                ///std::cout << "DIFFERENT METRIC" << std::endl;
                
                string msg;
                msg = "Routing rules " + gtwitf_it->second.second +
                    " and " + rule->getLabel() +
                    " are identical except for the metric, " +
                    "please delete one of them";
                compiler->abort(rule,  msg.c_str());
            }
        
        } else
        {
            // ... this gateway and interface combination is new for
            // this destination
            ///std::cout << "NEW GTWITF" << std::endl;///
            
            if(false)
            {
                // TODO_lowPrio: if ( !compiler->fw->getOptionsObject()->getBool ("equal_cost_multi_path") ) ...If multipath is turned off, perform this check.
                //               iterate all gtwitf combis in the map dest_it->second and search for the current metric
                
                // ... but has the same metric => what route should I use for this destination? => abort
                    
                string msg;
                msg = "Routing rules " + gtwitf_it->second.second + " and " +
                    rule->getLabel() +
                    " have the same destination and same metric,"
                    "but different gateway and interface combination. "
                    "Set the metrics to different values or "
                    "enable ECMP (Equal Cost MultiPath) routing";
                compiler->abort( msg.c_str() );
            
            } else {
                
                // ... and different metric OR equal_cost_multi_path enabled => OK
                
                tmp_queue.push_back(rule);
            }
            
            dest_it->second[combiId] = pair< string, string>( metric, rule->getLabel());
        }
        
    } else {
        
        // this destination is new
        //std::cout << "NEW DEST" << std::endl;
        ///
        
        //ruleinfo tmpRuleInfo = { gtw->getStr("id") + itf->getStr("id"), metric, rule->getLabel()};
        //rules_seen_so_far[label] = tmpRuleInfo;
        
        map< string, pair< string, string> > gtw_itf_tmp;
        gtw_itf_tmp[combiId] = pair< string, string>( metric, rule->getLabel());
        
        rules_seen_so_far[label] = gtw_itf_tmp;
        
        
        tmp_queue.push_back(rule);
    }

    return true;
}


bool RoutingCompiler::classifyRoutingRules::processNext()
{

    assert(compiler!=NULL);
    assert(prev_processor!=NULL);

    slurp();

    if (tmp_queue.size()==0) return false;
    
    
    for (std::deque<libfwbuilder::Rule *>::iterator tmp_queue_it=tmp_queue.begin(); tmp_queue_it!=tmp_queue.end(); ++tmp_queue_it)
    {
        RoutingRule *rule = RoutingRule::cast( *tmp_queue_it);
        rule->setRuleType( RoutingRule::SinglePath);
        
        RuleElementRItf *itfrel=rule->getRItf();
        FWObject *itf = FWReference::cast(itfrel->front())->getPointer();
        
        RuleElementRGtw *gtwrel=rule->getRGtw();
        FWObject *gtw = FWReference::cast(gtwrel->front())->getPointer();
        
        string metric  = rule->getMetricAsString();
        string label   = rule->getSortedDstIds();
        ostringstream ostr;
        ostr << gtw->getId() << "_" << itf->getId();
        string combiId = ostr.str();
        
        if( label == "")
            compiler->abort(
                
                    rule,
                    "Place 'createSortedDstIdsLabel()' right before "
                    "'classifyRoutingRules()' in the rule processor chain");
        
        dest_it = rules_seen_so_far.find(label);
        if( dest_it != rules_seen_so_far.end()) {
            
            // a rule with the same destination was already seen
            //std::cout << "classifyRoutingRules:NO NEW DEST" << std::endl;///
            
            gtwitf_it = dest_it->second.find(combiId);
            if( gtwitf_it == dest_it->second.end() ) {
                
                // ... this gateway and interface combination is new for this destination
                //std::cout << "classifyRoutingRules:NEW GTWITF" << std::endl;///
                
                
                for( gtwitf_it = dest_it->second.begin(); gtwitf_it != dest_it->second.end(); gtwitf_it++) {
                    
                    if( gtwitf_it->second.first == metric) {
                        // ... but has the same metric as another rule with this Dst => multipath or abort
                        //std::cout << "classifyRoutingRules:SAME METRIC" << std::endl;///
                        
                        if(true) { //TODO: if ( compiler->fw->getOptionsObject()->getBool ("equal_cost_multi_path") )
                            
                            rule->setRuleType( RoutingRule::MultiPath);
                            gtwitf_it->second.second->setRuleType( RoutingRule::MultiPath);
                            
                            //std::cout << "classifyRoutingRules:the rules " << rule->getLabel() << " and " << gtwitf_it->second.second->getLabel() << " were set to multipath." << std::endl;///
                        }
                    }
                }
                
                dest_it->second[combiId] = pair< string, RoutingRule*>( metric, rule);
            }
            
        } else {
            
            // this destination is new
            //std::cout << "classifyRoutingRules:NEW DEST" << std::endl;///
             
            map< string, pair< string, RoutingRule*> > gtw_itf_tmp;
            gtw_itf_tmp[combiId] = pair< string, RoutingRule*>( metric, rule);
            rules_seen_so_far[label] = gtw_itf_tmp;
        }
    }
    
    return true;
}


bool RoutingCompiler::createSortedDstIdsLabel::processNext()
{
    RoutingRule *rule=getNext(); if (rule==NULL) return false;
    
    tmp_queue.push_back(rule);
    
    // create a label with a sorted dst-id-list, to find identical destinations even if the order 
    // of the dst objects differs within one rule
    
    RuleElementRDst *dstrel=rule->getRDst();
              
    string label = rule->getLabel();
    int bracepos = label.find("(");
    label.erase(0, bracepos);   
             
    std::list<string> idList;
    for (FWObject::iterator it=dstrel->begin(); it!=dstrel->end(); ++it) {
        
        idList.insert(idList.end(),
                      FWObjectDatabase::getStringId(
                          (FWReference::cast(*it)->getPointer())->getId()));
    }
    idList.sort();
    for (std::list<string>::iterator it=idList.begin(); it!=idList.end(); ++it) {
        
        label += " " + *it;
    }
    ///std::cout << "createDstLabel:LABEL: '" << label << "'" << endl;
    
    rule->setSortedDstIds( label);
    
    return true;
}


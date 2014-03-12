#include "ns3/log.h"
#include <stdlib.h>
#include "dccontroller.h"

NS_LOG_COMPONENT_DEFINE("ECMPController");

ECMPController::~ECMPController() {
    NS_LOG_INFO("Deconstruct ECMPController");
    map<Mac48Address, struct edge_host_info *>::iterator it = host_states.begin();
    while (it != host_states.end()) {
        free(it->second);
        it++;
    }
}

void ECMPController::Init(size_t num_of_switches, FattreeTopo *topo) {
    this->num_of_switches = num_of_switches;
    this->topo = topo;
}

void ECMPController::ReceiveFromSwitch(Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer) {
    if (m_switches.size() != num_of_switches) {
        NS_LOG_INFO("Ignore packets from switch before all switches are up.");
        return;
    }
    if (m_switches.find(swtch) == m_switches.end()) {
        NS_LOG_ERROR("Receive packets from unregistered switch.");
        return;
    }

    uint8_t type = GetPacketType(buffer);
    if (type == OFPT_PACKET_IN) {
        ofp_packet_in *opi = (ofp_packet_in *) ofpbuf_try_pull(buffer, offsetof(ofp_packet_in, data));
        int port = ntohs(opi->in_port);
        NS_LOG_INFO("Packet total length: " << ntohs(opi->header.length));

        // Create matching key.
        sw_flow_key key;
        key.wildcards = OFPFW_IN_PORT;
        flow_extract(buffer, port != -1 ? port : OFPP_NONE, &key.flow);

        uint16_t in_port = ntohs(key.flow.in_port);
        Mac48Address src_addr;
        // Ipv4Address src_nw_addr;
        src_addr.CopyFrom(key.flow.dl_src);
        // src_nw_addr.Set(key.flow.nw_src);
        map<Mac48Address, struct edge_host_info*>::iterator it;

        Mac48Address dst_addr;
        // Ipv4Address dst_nw_addr;
        dst_addr.CopyFrom(key.flow.dl_dst);
        // dst_nw_addr.Set(key.flow.nw_dst);
        NS_LOG_INFO("L2: " << topo->ToString(swtch) << " receive packet in from " << src_addr << " to " << dst_addr);
        // NS_LOG_INFO("L3: Receive packet in from " << src_nw_addr << " to " << dst_nw_addr);
        it = host_states.find(dst_addr);
        if (it != host_states.end()) {
            NS_LOG_INFO("dst: " << dst_addr << " is already learned.");
            struct edge_host_info *pair = it->second;
            Ptr<OpenFlowSwitchNetDevice> dstEdgeSwitch = pair->edgeSwitch;
            // uint16_t port = pair->port;
            // Install default path to dst 
            vector<Ptr<NetDevice> > defaultPath = topo->GetDefaultPathBetweenSwitches(swtch, dstEdgeSwitch);
            vector<Ptr<NetDevice> >::iterator it2 = defaultPath.begin();
            Ptr<NetDevice> prev = *it2;
            it2++;
            bool is_first = true;
            while (it2 != defaultPath.end()) {
                ofp_action_output action_forward[1];
                action_forward[0].type = htons(OFPAT_OUTPUT);
                action_forward[0].len = htons(sizeof(ofp_action_output));
                action_forward[0].port = topo->GetPortToSwitch(prev, *it2);
                key.flow.in_port = OFPP_NONE;
                key.wildcards = OFPFW_IN_PORT;
                ofp_flow_mod *ofm = BuildFlow(key, is_first ? opi->buffer_id : -1, OFPFC_ADD, action_forward, sizeof(action_forward), OFP_FLOW_PERMANENT, OFP_FLOW_PERMANENT);
                if (is_first)
                    is_first = false;
                SendToSwitch(DynamicCast<OpenFlowSwitchNetDevice, NetDevice>(prev), ofm, ntohs(ofm->header.length));
                // free(ofm);
                // NS_LOG_INFO(topo->ToString(prev));
                prev = *it2;
                it2++;
            }
        } else {
            // Flood packet on downstream ports of all edge switches
            NS_LOG_INFO("Flood packet from " << src_addr << " to " << dst_addr);
            Flood(swtch, key, opi);
            NS_LOG_INFO("Flood complete.");
        }

        // Learn src Mac address
        it = host_states.find(src_addr);
        if (it == host_states.end()) {
            NS_LOG_INFO("Learn src mac address: " << src_addr);
            struct edge_host_info *pair = (struct edge_host_info *) malloc(sizeof(struct edge_host_info));
            pair->edgeSwitch = swtch;
            pair->port = in_port;
            //host_states[src_addr] = pair;
            host_states.insert(std::make_pair(src_addr, pair));
            ofp_action_output action_forward[1];
            action_forward[0].type = htons(OFPAT_OUTPUT);
            action_forward[0].len = htons(sizeof(ofp_action_output));
            action_forward[0].port = in_port;
            src_addr.CopyTo(key.flow.dl_dst);
            dst_addr.CopyTo(key.flow.dl_src);
            key.flow.in_port = OFPP_NONE;
            key.wildcards = OFPFW_IN_PORT;
            ofp_flow_mod* ofm2 = BuildFlow(key, -1, OFPFC_ADD, action_forward, sizeof(action_forward), OFP_FLOW_PERMANENT, OFP_FLOW_PERMANENT);
            SendToSwitch(swtch, ofm2, ntohs(ofm2->header.length));
            // free(ofm2);
            NS_LOG_INFO("Learn src mac address complete.");
        }
    }
}

void ECMPController::Flood(Ptr<OpenFlowSwitchNetDevice> swtch, sw_flow_key& key, struct ofp_packet_in* opi) {
    ofp_action_output x[1];
    x[0].type = htons (OFPAT_OUTPUT);
    x[0].len = htons (sizeof(ofp_action_output));
    x[0].port = OFPP_FLOOD;

    ofp_flow_mod* ofm = BuildFlow(key, opi->buffer_id, OFPFC_ADD, x, sizeof(x), OFP_FLOW_PERMANENT, OFP_FLOW_PERMANENT);
    SendToSwitch(swtch, ofm, ntohs(ofm->header.length));
    // free(ofm);
    /*
    vector<Ptr<NetDevice> > edgeSwitches = topo->GetEdgeSwitches();
    int dstHostIndex = 0;
    for (vector<Ptr<NetDevice> >::iterator it = edgeSwitches.begin(); it != edgeSwitches.end(); it++) {
        uint16_t out_port;
        while ((out_port = topo->GetPortToHost(*it, topo->GetHostById(dstHostIndex))) != PORT_UNKNOWN) {
            NS_LOG_INFO("dstIndex: " << dstHostIndex << " out_port: " << out_port);
            uint16_t packet_length = sizeof(struct ofp_packet_out) + sizeof(struct ofp_action_output) + ntohs(opi->header.length);
            NS_LOG_INFO("Flood packet length: " << packet_length);
            struct ofp_packet_out *flood_packet = (struct ofp_packet_out *) malloc(packet_length);
            flood_packet->header.version = OFP_VERSION;
            flood_packet->header.type = OFPT_PACKET_OUT;
            flood_packet->header.length = htons(packet_length);
            flood_packet->buffer_id = htonl(-1);
            flood_packet->in_port = OFPP_NONE;
            flood_packet->actions_len = htons(sizeof(struct ofp_action_output));
            struct ofp_action_output action_flood;
            action_flood.type = htons(OFPAT_OUTPUT);
            action_flood.len = htons(sizeof(struct ofp_action_output));
            action_flood.port = out_port;
            memcpy(flood_packet->actions, &action_flood, sizeof(struct ofp_action_output));
            memcpy((uint8_t *)(flood_packet->actions) + sizeof(struct ofp_action_output), opi, ntohs(opi->header.length));
            NS_LOG_INFO("Before");
            SendToSwitch(DynamicCast<OpenFlowSwitchNetDevice, NetDevice>(*it), flood_packet, packet_length);
            NS_LOG_INFO("After");
            dstHostIndex++;
            //free(flood_packet);
        }
    }
    */
}

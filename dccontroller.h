#ifndef DCCONTROLLER_H
#define DCCONTROLLER_H
#include "topo.h"
#include "ns3/openflow-interface.h"
#include "ns3/openflow-module.h"
#include "ns3/openflow-switch-net-device.h"
#include <map>

using namespace std;

struct edge_host_info {
    Ptr<OpenFlowSwitchNetDevice> edgeSwitch;
    uint16_t port;
};


class ECMPController : public ns3::ofi::Controller {
public:
    ~ECMPController();

    void Init(size_t num_of_switches, FattreeTopo *topo);

    void Flood(Ptr<OpenFlowSwitchNetDevice> swtch, sw_flow_key& key, struct ofp_packet_in *opi);

    void ReceiveFromSwitch(Ptr<OpenFlowSwitchNetDevice> swtch, ofpbuf* buffer);

protected:
    size_t num_of_switches;
    FattreeTopo *topo;
    map<Mac48Address, struct edge_host_info*> host_states;
};
#endif

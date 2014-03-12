#ifndef TOPO_H
#define TOPO_H
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/openflow-module.h"

#include <vector>
#include <string>

#define PORT_UNKNOWN 65535

using namespace std;
using namespace ns3;

class FattreeTopo {
public:
    FattreeTopo(int k, unsigned long rate, unsigned long d, Ptr<ns3::ofi::Controller> c);
    ~FattreeTopo();
    uint16_t GetPortToSwitch(Ptr<NetDevice> src, Ptr<NetDevice> dst);
    uint16_t GetPortToHost(Ptr<NetDevice> src, Ptr<NetDevice> dst);
    vector<vector<Ptr<NetDevice> > > GetPaths(Ptr<NetDevice> src, Ptr<NetDevice> dst);
    vector<Ptr<NetDevice> > GetDefaultPath(Ptr<NetDevice> src, Ptr<NetDevice> dst);
    vector<vector<Ptr<NetDevice> > > GetPathsBetweenSwitches(Ptr<NetDevice> src, Ptr<NetDevice> dst);
    vector<Ptr<NetDevice> > GetDefaultPathBetweenSwitches(Ptr<NetDevice> src, Ptr<NetDevice> dst);
    Ptr<NetDevice> GetHostById(int hostIndex);
    Ptr<Node> GetHostNodeById(int hostIndex);
    string ToString(Ptr<NetDevice> dev);
    vector<Ptr<NetDevice> > GetEdgeSwitches(); 

private:
    void Create();
    int size;
    unsigned long dataRate;
    unsigned long delay;
    NetDeviceContainer hostDevices;
    NetDeviceContainer edgeSwitchDevices;
    NetDeviceContainer aggSwitchDevices;
    NetDeviceContainer coreSwitchDevices;
    NodeContainer hosts;
    NodeContainer edgeSwitches;
    NodeContainer aggSwitches;
    NodeContainer coreSwitches;
    Ptr<ns3::ofi::Controller> controller;
};

#endif

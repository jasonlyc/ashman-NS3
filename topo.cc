#include "topo.h"
#include "ns3/log.h"
#include <sstream>

NS_LOG_COMPONENT_DEFINE("FattreeTopo");

FattreeTopo::FattreeTopo(int k, unsigned long rate, unsigned long d, Ptr<ns3::ofi::Controller> c) {
    size = k;
    dataRate = rate;
    delay = d;
    controller = c;
    Create();
}

FattreeTopo::~FattreeTopo() {
    
}

vector<vector<Ptr<NetDevice> > > FattreeTopo::GetPaths(Ptr<NetDevice> src, Ptr<NetDevice> dst) {
    vector<vector<Ptr<NetDevice> > > result;
    int srcIndex, dstIndex;
    for (int i = 0; i != 2 * size * size * size; i++) {
        if (hostDevices.Get(i) == src) {
            srcIndex = i;
            continue;
        }
        if (hostDevices.Get(i) == dst) {
            dstIndex = i;
            continue;
        }
    }
    int srcPod = srcIndex / size / size;
    int dstPod = dstIndex / size / size;
    if (srcPod == dstPod) { // in the same pod
        int srcEdgeIndex = srcIndex / size;
        int dstEdgeIndex = dstIndex / size;
        if (srcEdgeIndex == dstEdgeIndex) { // under the same edge switch
            vector<Ptr<NetDevice> > path;
            path.push_back(edgeSwitchDevices.Get(dstEdgeIndex));
            path.push_back(dst);
            result.push_back(path);
            return result;
        } else { // but different edge switch
            for (int aggIndex  = srcPod * size; aggIndex != (srcPod + 1) * size; aggIndex++) {
                vector<Ptr<NetDevice> > path;
                path.push_back(edgeSwitchDevices.Get(srcEdgeIndex));
                path.push_back(aggSwitchDevices.Get(aggIndex));
                path.push_back(edgeSwitchDevices.Get(dstEdgeIndex));
                path.push_back(dst);
                result.push_back(path);
            }
            return result;
        }
    } else { // in different pod
       int srcEdgeIndex = srcIndex / size;
       int dstEdgeIndex = dstIndex / size;
       for (int srcAggIndex = srcPod * size; srcAggIndex != (srcPod + 1) * size; srcAggIndex++) {
           for (int coreIndex = (srcAggIndex % size) * size; coreIndex != (srcAggIndex % size + 1) * size; coreIndex++) {
               int dstAggIndex = dstPod * size + coreIndex / size;
               vector<Ptr<NetDevice> > path;
               path.push_back(edgeSwitchDevices.Get(srcEdgeIndex));
               path.push_back(aggSwitchDevices.Get(srcAggIndex));
               path.push_back(coreSwitchDevices.Get(coreIndex));
               path.push_back(aggSwitchDevices.Get(dstAggIndex));
               path.push_back(edgeSwitchDevices.Get(dstEdgeIndex));
               path.push_back(dst);
               result.push_back(path);
           }
       }
       return result;
    }
}

vector<vector<Ptr<NetDevice> > > FattreeTopo::GetPathsBetweenSwitches(Ptr<NetDevice> src, Ptr<NetDevice> dst) {
    vector<vector<Ptr<NetDevice> > > result;
    int srcEdgeIndex, dstEdgeIndex;
    for (int i = 0; i != 2 * size * size; i++) {
        if (edgeSwitchDevices.Get(i) == src) {
            srcEdgeIndex = i;
            continue;
        }
        if (edgeSwitchDevices.Get(i) == dst) {
            dstEdgeIndex = i;
            continue;
        }
    }
    int srcPod = srcEdgeIndex / size;
    int dstPod = dstEdgeIndex / size;
    if (srcPod == dstPod) { // in the same pod
        if (srcEdgeIndex == dstEdgeIndex) { // under the same edge switch
            vector<Ptr<NetDevice> > path;
            path.push_back(edgeSwitchDevices.Get(dstEdgeIndex));
            result.push_back(path);
            return result;
        } else { // but different edge switch
            for (int aggIndex  = srcPod * size; aggIndex != (srcPod + 1) * size; aggIndex++) {
                vector<Ptr<NetDevice> > path;
                path.push_back(edgeSwitchDevices.Get(srcEdgeIndex));
                path.push_back(aggSwitchDevices.Get(aggIndex));
                path.push_back(edgeSwitchDevices.Get(dstEdgeIndex));
                result.push_back(path);
            }
            return result;
        }
    } else { // in different pod
       for (int srcAggIndex = srcPod * size; srcAggIndex != (srcPod + 1) * size; srcAggIndex++) {
           for (int coreIndex = (srcAggIndex % size) * size; coreIndex != (srcAggIndex % size + 1) * size; coreIndex++) {
               int dstAggIndex = dstPod * size + coreIndex / size;
               vector<Ptr<NetDevice> > path;
               path.push_back(edgeSwitchDevices.Get(srcEdgeIndex));
               path.push_back(aggSwitchDevices.Get(srcAggIndex));
               path.push_back(coreSwitchDevices.Get(coreIndex));
               path.push_back(aggSwitchDevices.Get(dstAggIndex));
               path.push_back(edgeSwitchDevices.Get(dstEdgeIndex));
               result.push_back(path);
           }
       }
       return result;
    }
}

vector<Ptr<NetDevice> > FattreeTopo::GetDefaultPathBetweenSwitches(Ptr<NetDevice> src, Ptr<NetDevice> dst) {
    return GetPathsBetweenSwitches(src, dst)[0];
}

vector<Ptr<NetDevice> > FattreeTopo::GetDefaultPath(Ptr<NetDevice> src, Ptr<NetDevice> dst) {
    return GetPaths(src, dst)[0];
}

vector<Ptr<NetDevice> > FattreeTopo::GetEdgeSwitches() {
    vector<Ptr<NetDevice> > result;
    NetDeviceContainer::Iterator i;
    for (i = edgeSwitchDevices.Begin(); i != edgeSwitchDevices.End(); i++) {
        result.push_back(*i);
    }
    return result;
}

void FattreeTopo::Create() {
    NS_LOG_INFO("Create Nodes.");
    hosts.Create(2 * size * size * size);
    edgeSwitches.Create(2 * size * size);
    aggSwitches.Create(2 * size * size);
    coreSwitches.Create(size  * size);
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(dataRate));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(delay)));
    OpenFlowSwitchHelper ofSwitchHelper;

    NS_LOG_INFO("Build Fattree Topology.");
    for (int i = 0; i != 2 * size; i++) { // for each pod
        for (int j = i * size; j != (i + 1) * size; j++) { // for each edge switch in the pod
            Ptr<Node> edgeSwitch = edgeSwitches.Get(j);
            NetDeviceContainer edgeSwitchDevice;
            for (int k = j * size; k != (j + 1) * size; k++) { // for each host under the edge switch
                Ptr<Node> host = hosts.Get(k);
                NetDeviceContainer link = csma.Install(NodeContainer(edgeSwitch, host));
                edgeSwitchDevice.Add(link.Get(0));
                // add host into container
                hostDevices.Add(link.Get(1));
            }
            // add partially installed edge switch into container
            edgeSwitchDevices.Add(ofSwitchHelper.Install(edgeSwitch, edgeSwitchDevice));
        }
        for (int j = i * size; j != (i + 1) * size; j++) { // for each agg switch in the pod
            Ptr<Node> aggSwitch = aggSwitches.Get(j);
            NetDeviceContainer aggSwitchDevice;
            for (int k = i * size; k != (i + 1) * size; k++) { // for each edge switch in the pod
                Ptr<Node> edgeSwitch = edgeSwitches.Get(k);
                NetDeviceContainer link = csma.Install(NodeContainer(aggSwitch, edgeSwitch));
                aggSwitchDevice.Add(link.Get(0));
                // add port connected with agg switch to edge switch
                DynamicCast<OpenFlowSwitchNetDevice, NetDevice>(edgeSwitchDevices.Get(k))->AddSwitchPort(link.Get(1));
            }
            // add partially installed agg switch into container
            aggSwitchDevices.Add(ofSwitchHelper.Install(aggSwitch, aggSwitchDevice));
        }
    }

    for (int i = 0; i != size * size; i++) { // for each core switch
        Ptr<Node> coreSwitch = coreSwitches.Get(i);
        NetDeviceContainer coreSwitchDevice;
        for (int j = 0; j != 2 * size; j++) { // for each pod
            int aggId = j * size + i / size;
            Ptr<Node> aggSwitch = aggSwitches.Get(aggId);
            NetDeviceContainer link = csma.Install(NodeContainer(coreSwitch, aggSwitch));
            coreSwitchDevice.Add(link.Get(0));
            DynamicCast<OpenFlowSwitchNetDevice, NetDevice>(aggSwitchDevices.Get(aggId))->AddSwitchPort(link.Get(1));
        }
        // add fully installed core switch into container
        coreSwitchDevices.Add(ofSwitchHelper.Install(coreSwitch, coreSwitchDevice, controller));
    }
    for (int i = 0; i != 2 * size * size; i++) {
        DynamicCast<OpenFlowSwitchNetDevice, NetDevice>(edgeSwitchDevices.Get(i))->SetController(controller);
        DynamicCast<OpenFlowSwitchNetDevice, NetDevice>(aggSwitchDevices.Get(i))->SetController(controller);
        NS_LOG_INFO("Edge switch " << i << " has " << (DynamicCast<OpenFlowSwitchNetDevice, NetDevice>(edgeSwitchDevices.Get(i))->GetNSwitchPorts()));
        NS_LOG_INFO("Agg switch " << i << " has " << (DynamicCast<OpenFlowSwitchNetDevice, NetDevice>(aggSwitchDevices.Get(i))->GetNSwitchPorts()));
    }

    NS_LOG_INFO("Assign IP Addresses.");
    InternetStackHelper internet;
    internet.Install(hosts);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");
    ipv4.Assign(hostDevices);
}

uint16_t FattreeTopo::GetPortToSwitch(Ptr<NetDevice> src, Ptr<NetDevice> dst) {
    int srcIndex, dstIndex;
    for (srcIndex = 0; srcIndex != 2 * size * size; srcIndex++) {
        if (src == edgeSwitchDevices.Get(srcIndex)) { // src is edge switch, dst should be agg switch
            int pod = srcIndex / size;
            for (dstIndex = pod * size; dstIndex != (pod + 1) * size; dstIndex++) {
                if (dst == aggSwitchDevices.Get(dstIndex)) {
                    return size + dstIndex - pod * size;
                }
            }
            return PORT_UNKNOWN;
        }
        if (src == aggSwitchDevices.Get(srcIndex)) { // src is agg switch , dst is either core switch or edge switch
            for (dstIndex = (srcIndex % size) * size; dstIndex != (srcIndex % size + 1) * size; dstIndex++) { // search core switches first
                if (dst == coreSwitchDevices.Get(dstIndex)) {
                    return size + dstIndex - (srcIndex % size) * size;
                }
            }
            int pod = srcIndex / size;
            for (dstIndex = pod * size; dstIndex != (pod + 1) * size; dstIndex++) { // then search edge switches
                if (dst == edgeSwitchDevices.Get(dstIndex)) {
                    return dstIndex - pod * size;
                }
            }
            return PORT_UNKNOWN;
        }
    }
    for (srcIndex = 0; srcIndex != size * size; srcIndex++) {
        if (src == coreSwitchDevices.Get(srcIndex)) { // src is core switch, dst should be agg switch
            int offset = srcIndex / size;
            for (dstIndex = offset; dstIndex != 2 * size * size + offset; dstIndex += size) {
                if (dst == aggSwitchDevices.Get(dstIndex)) {
                    return (dstIndex - offset) / size;
                }
            }
            return PORT_UNKNOWN;
        }
    }
    return PORT_UNKNOWN;
}

uint16_t FattreeTopo::GetPortToHost(Ptr<NetDevice> src, Ptr<NetDevice> dst) {
    int srcIndex, dstIndex;
    for (srcIndex = 0; srcIndex != 2 * size * size; srcIndex++) {
        if (src == edgeSwitchDevices.Get(srcIndex)) {
            for (dstIndex = srcIndex * size; dstIndex != (srcIndex + 1) * size; dstIndex++) {
                if (dst == hostDevices.Get(dstIndex)) {
                    return dstIndex - srcIndex * size;
                }
            }
            return PORT_UNKNOWN;
        }
    }
    return PORT_UNKNOWN;
}

Ptr<NetDevice> FattreeTopo::GetHostById(int hostIndex) {
    return hostDevices.Get(hostIndex);
}

Ptr<Node> FattreeTopo::GetHostNodeById(int hostIndex) {
    return hosts.Get(hostIndex);
}

string FattreeTopo::ToString(Ptr<NetDevice> dev) {
    for (int i = 0; i != 2 * size * size * size; i++) {
        if (hostDevices.Get(i) == dev) {
            stringstream ss;
            ss << i;
            return string("host-") + ss.str();
        }
    }
    for (int i = 0; i != 2 * size * size; i++) {
        if (edgeSwitchDevices.Get(i) == dev) {
            stringstream ss;
            ss << i;
            return string("edge-") + ss.str();
        }
        if (aggSwitchDevices.Get(i) == dev) {
            stringstream ss;
            ss << i;
            return string("agg-") + ss.str();
        }
    }
    for (int i = 0; i != size * size; i++) {
        if (coreSwitchDevices.Get(i) == dev) {
            stringstream ss;
            ss << i;
            return string("core-") + ss.str();
        }
    }
    return string("unknown device");
}

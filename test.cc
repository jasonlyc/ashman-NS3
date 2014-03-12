#include "topo.h"
#include "ns3/log.h"
#include <iostream>
#include "dccontroller.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("TopoTest");

int main(int argc, char *argv[]) {
    LogComponentEnable("TopoTest", LOG_LEVEL_INFO);
    LogComponentEnable("FattreeTopo", LOG_LEVEL_INFO);
    LogComponentEnable("ECMPController", LOG_LEVEL_INFO);
    // LogComponentEnable("OpenFlowSwitchNetDevice", LOG_LEVEL_INFO);
    LogComponentEnable("OpenFlowInterface", LOG_LEVEL_INFO);
    // LogComponentEnableAll(LOG_INFO);
    Ptr<ECMPController> controller = CreateObject<ECMPController>();
    FattreeTopo ft(2, 100000000, 5, controller);
    controller->Init(20, &ft);
    /*
    Ptr<NetDevice> src = ft.GetHostById(8);
    Ptr<NetDevice> dst = ft.GetHostById(7);
    vector<vector<Ptr<NetDevice> > > paths = ft.GetPaths(src, dst);
    vector<vector<Ptr<NetDevice> > >::iterator i1;
    for (i1 = paths.begin(); i1 != paths.end(); i1++) {
        vector<Ptr<NetDevice> > path = *i1;
        vector<Ptr<NetDevice> >::iterator i2;
        for (i2 = path.begin(); i2 != path.end(); i2++) {
            cout << ft.ToString(*i2) << " ";
        }
        cout << endl;
    }
    */
    NS_LOG_INFO("Create Applications");
    uint16_t port = 8765;
    OnOffHelper onoff("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address("10.0.0.9"), port)));
    onoff.SetConstantRate(DataRate("500kb/s"));
    ApplicationContainer app = onoff.Install(ft.GetHostNodeById(0));
    app.Start(Seconds(1.0));
    app.Stop(Seconds(5.0));

    PacketSinkHelper sink("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
    app= sink.Install(ft.GetHostNodeById(8));
    app.Start(Seconds(0.0));

    NS_LOG_INFO ("Configure Tracing.");

    //
    // Configure tracing of all enqueue, dequeue, and NetDevice receive events.
    // Trace output will be sent to the file "openflow-switch.tr"
    //
    AsciiTraceHelper ascii;
    CsmaHelper csma;
    csma.EnableAsciiAll (ascii.CreateFileStream ("ecmp-controller.tr"));

    //
    // Also configure some tcpdump traces; each interface will be traced.
    // The output files will be named:
    //     ecmp-controller-<nodeId>-<interfaceId>.pcap
    // and can be read by the "tcpdump -r" command (use "-tt" option to
    // display timestamps correctly)
    //
    csma.EnablePcapAll ("ecmp-controller", false);

    //
    // Now, do the actual simulation.
    //
    NS_LOG_INFO ("Run Simulation.");
    Simulator::Run ();
    Simulator::Destroy ();
    NS_LOG_INFO ("Done.");


}

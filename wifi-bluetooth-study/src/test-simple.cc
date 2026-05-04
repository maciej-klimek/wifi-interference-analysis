/*
 * WiFi vs Bluetooth Interference Study - Simplified Version for Testing
 */

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <fstream>
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WiFiBT-SimpleTest");

uint32_t rxBytes = 0;

void
PacketRx(Ptr<const Packet> pkt, const Address& addr)
{
    rxBytes += pkt->GetSize();
}

int
main(int argc, char* argv[])
{
    bool btEnabled = false;
    uint32_t numPackets = 100;
    std::string outputFile = "/tmp/wifi-bt-test.csv";

    CommandLine cmd(__FILE__);
    cmd.AddValue("bluetooth-enabled", "Enable BT interferer", btEnabled);
    cmd.AddValue("num-packets", "Number of packets to send", numPackets);
    cmd.AddValue("output-file", "Output CSV file", outputFile);
    cmd.Parse(argc, argv);

    NS_LOG_INFO("WiFi-BT Simple Test");
    NS_LOG_INFO("BT Enabled: " << (btEnabled ? "yes" : "no"));

    // Create two nodes: AP and STA
    NodeContainer nodes;
    nodes.Create(2);

    // Create WiFi
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper phy;
    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::FriisPropagationLossModel");
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    Ssid ssid("test");
    
    // AP
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer devices = wifi.Install(phy, mac, nodes.Get(0));

    // STA
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    devices.Add(wifi.Install(phy, mac, nodes.Get(1)));

    // Mobility
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // IP stack
    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    NS_LOG_INFO("Simulation setup complete");

    // Schedule stop
    Simulator::Stop(Seconds(10));
    Simulator::Run();

    std::cout << "Simulation finished, RX bytes: " << rxBytes << std::endl;

    // Write output
    std::ofstream file(outputFile);
    file << "test,bt_enabled,rx_bytes\n";
    file << "1," << (btEnabled ? 1 : 0) << "," << rxBytes << "\n";
    file.close();

    std::cout << "Output written to " << outputFile << std::endl;

    Simulator::Destroy();
    return 0;
}

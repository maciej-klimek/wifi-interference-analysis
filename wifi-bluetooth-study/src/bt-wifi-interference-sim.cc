#include "ns3/applications-module.h"
#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/spectrum-module.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/waveform-generator-helper.h"
#include "ns3/waveform-generator.h"
#include "ns3/wifi-module.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WiFiBT");

uint64_t g_rxBytes = 0;
uint32_t g_rxPackets = 0;

void PacketRx(Ptr<const Packet> p, const Address& a) {
    g_rxBytes += p->GetSize();
    g_rxPackets++;
}

Ptr<SpectrumValue>
CreateBluetoothHopPsd(double centerFrequencyMhz, double txPowerDbm)
{
    Ptr<SpectrumModel> model = SpectrumModelIsm2400MhzRes1Mhz();
    Ptr<SpectrumValue> psd = Create<SpectrumValue>(model);

    const double txPowerW = std::pow(10.0, txPowerDbm / 10.0) / 1000.0;
    const uint32_t bandIndex = static_cast<uint32_t>(std::llround(centerFrequencyMhz - 2400.0));

    if (bandIndex < psd->GetValuesN())
    {
        const auto band = *(psd->ConstBandsBegin() + bandIndex);
        const double bandWidthHz = band.fh - band.fl;
        (*psd)[bandIndex] = txPowerW / bandWidthHz;
    }

    return psd;
}

int main(int argc, char* argv[])
{
    bool btEnabled = false;
    uint32_t rngRun = 1;
    Time simTime("10s");
    double distance = 15.0;
    std::string outputFile = "results/wifi-bluetooth-results.csv";
    std::string dataRateStr = "10Mbps";
    std::string wifiStandardStr = "802.11g";
    std::string deviceProfileStr = "baseline";
    double wifiChannelWidthMhz = 20.0;
    double wifiChannelNumber = 0.0;
    double btBurstOnMs = 2.0;
    double btBurstPeriodMs = 50.0;
    double btDistance = 0.1;
    double btPowerDbm = 0.0;
    double btHopDwellUs = 625.0;
    uint32_t btHopCount = 79;

    CommandLine cmd(__FILE__);
    cmd.AddValue("bluetooth-enabled", "Enable BT", btEnabled);
    cmd.AddValue("rng-run", "RNG run", rngRun);
    cmd.AddValue("simulation-time", "Sim time", simTime);
    cmd.AddValue("distance", "Distance (m)", distance);
    cmd.AddValue("output-csv", "Output CSV", outputFile);
    cmd.AddValue("data-rate", "AP offered data rate (e.g. 150Mbps)", dataRateStr);
    cmd.AddValue("wifi-standard", "WiFi standard to use: 802.11b, 802.11g, 802.11n (2.4GHz)", wifiStandardStr);
    cmd.AddValue("device-profile", "Device profile preset (baseline, s24-liberty4, iphone-liberty4)", deviceProfileStr);
    cmd.AddValue("wifi-channel-number", "WiFi channel number", wifiChannelNumber);
    cmd.AddValue("wifi-channel-width-mhz", "WiFi channel width in MHz", wifiChannelWidthMhz);
    cmd.AddValue("bt-burst-on-ms", "BT burst ON duration in milliseconds (default 2ms)", btBurstOnMs);
    cmd.AddValue("bt-burst-period-ms", "BT burst period in milliseconds (default 50ms)", btBurstPeriodMs);
    cmd.AddValue("bt-distance", "BT interferer distance from the STA in meters", btDistance);
    cmd.AddValue("bt-power-dbm", "BT interferer power in dBm", btPowerDbm);
    cmd.AddValue("bt-hop-dwell-us", "BT hop dwell time in microseconds", btHopDwellUs);
    cmd.AddValue("bt-hop-count", "Number of BT hop channels in 2.4 GHz", btHopCount);
    cmd.Parse(argc, argv);

    if (deviceProfileStr == "s24") {
        // Original Liberty 4 preset used before the physical calibration.
        wifiStandardStr = "802.11ax";
        wifiChannelNumber = 0.0;
        wifiChannelWidthMhz = 40.0;
        dataRateStr = "50Mbps";
        btBurstOnMs = 2.5;
        btBurstPeriodMs = 7.5;
        btDistance = 0.02;
        btPowerDbm = -3.0;
        btHopDwellUs = 625.0;
        btHopCount = 79;
    } else if (deviceProfileStr == "iphone") {
        wifiStandardStr = "802.11n";
        wifiChannelNumber = 1.0;
        wifiChannelWidthMhz = 20.0;
        dataRateStr = "50Mbps";
        btBurstOnMs = 5.0;
        btBurstPeriodMs = 7.5;
        btDistance = 0.01;
        btPowerDbm = 3.0;
        btHopDwellUs = 625.0;
        btHopCount = 1;
    }

    RngSeedManager::SetRun(rngRun);
    NS_LOG_INFO("WiFi-BT: BT=" << (btEnabled ? "on" : "off"));

    NodeContainer nodes;
    nodes.Create(3);

    WifiHelper wifi;
    // Allow choosing WiFi standard via command line
    if (wifiStandardStr == "802.11b" || wifiStandardStr == "b") {
        wifi.SetStandard(WIFI_STANDARD_80211b);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", StringValue("DsssRate11Mbps"),
                                     "ControlMode", StringValue("DsssRate1Mbps"));
    } else if (wifiStandardStr == "802.11g" || wifiStandardStr == "g") {
        wifi.SetStandard(WIFI_STANDARD_80211g);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", StringValue("ErpOfdmRate54Mbps"),
                                     "ControlMode", StringValue("ErpOfdmRate6Mbps"));
    } else if (wifiStandardStr == "802.11n" || wifiStandardStr == "n") {
        // Use 802.11n to allow higher PHY rates (HT modes)
        wifi.SetStandard(WIFI_STANDARD_80211n);
        // Use a high MCS rate (HtMcs7) with ConstantRate manager so PHY doesn't auto-downshift
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", StringValue("HtMcs7"),
                                     "ControlMode", StringValue("HtMcs0"));
    } else if (wifiStandardStr == "802.11ax" || wifiStandardStr == "ax") {
        wifi.SetStandard(WIFI_STANDARD_80211ax);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", StringValue("HeMcs11"),
                                     "ControlMode", StringValue("ErpOfdmRate54Mbps"));
    } else {
        // Default to 802.11g
        wifi.SetStandard(WIFI_STANDARD_80211g);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", StringValue("ErpOfdmRate54Mbps"),
                                     "ControlMode", StringValue("ErpOfdmRate6Mbps"));
    }

    SpectrumWifiPhyHelper phy(1);
    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    lossModel->SetFrequency(2.412e9);
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);
    phy.AddChannel(spectrumChannel, WIFI_SPECTRUM_2_4_GHZ);
    std::ostringstream channelSettings;
    channelSettings << "{" << wifiChannelNumber << ", " << wifiChannelWidthMhz << ", BAND_2_4GHZ, 0}";
    phy.Set(0, "ChannelSettings", StringValue(channelSettings.str()));
    phy.Set(0, "TxPowerStart", DoubleValue(20.0));
    phy.Set(0, "TxPowerEnd", DoubleValue(20.0));

    WifiMacHelper mac;
    Ssid ssid("wifi");

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer devs = wifi.Install(phy, mac, nodes.Get(0));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    devs.Add(wifi.Install(phy, mac, nodes.Get(1)));

    mac.SetType("ns3::AdhocWifiMac");
    devs.Add(wifi.Install(phy, mac, nodes.Get(2)));

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator>();
    pos->Add(Vector(0, 0, 0));
    pos->Add(Vector(distance, 0, 0));
    pos->Add(Vector(distance + btDistance, 0, 0));
    mobility.SetPositionAllocator(pos);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    InternetStackHelper inet;
    inet.Install(nodes);

    Ipv4AddressHelper addr;
    addr.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaces = addr.Assign(devs);

    Ipv4Address staAddr = ifaces.GetAddress(1);

    // Packet sink on STA (UDP)
    uint16_t port = 9;
    PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(simTime);

    // AP -> STA traffic using OnOff (UDP) to provide a configurable offered load
    OnOffHelper source("ns3::UdpSocketFactory", InetSocketAddress(staAddr, port));
    source.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    source.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    source.SetAttribute("DataRate", DataRateValue(DataRate(dataRateStr)));
    source.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer sourceApps = source.Install(nodes.Get(0));
    sourceApps.Start(Seconds(0.5));
    sourceApps.Stop(simTime);

    // Optional BT interferer
    if (btEnabled) {
        const double burstPeriodSeconds = btBurstPeriodMs / 1000.0;
        const double burstOnSeconds = btBurstOnMs / 1000.0;
        const double hopDwellSeconds = btHopDwellUs / 1e6;
        const uint32_t hopsPerBurst = std::max<uint32_t>(1, static_cast<uint32_t>(burstOnSeconds / hopDwellSeconds));
        const uint32_t btHopSequenceStride = 17;
        const double btBaseFrequencyMhz = 2402.0;

        for (double burstStart = 0.1; burstStart < simTime.GetSeconds(); burstStart += burstPeriodSeconds)
        {
            for (uint32_t hopInBurst = 0; hopInBurst < hopsPerBurst; ++hopInBurst)
            {
                const double hopStart = burstStart + (hopInBurst * hopDwellSeconds);
                if (hopStart >= simTime.GetSeconds())
                {
                    break;
                }

                const uint32_t hopIndex = (rngRun * 13 + static_cast<uint32_t>(burstStart * 1000.0) +
                                           hopInBurst * btHopSequenceStride) % btHopCount;
                const double hopFrequencyMhz = btBaseFrequencyMhz + hopIndex;
                Ptr<SpectrumValue> btPsd = CreateBluetoothHopPsd(hopFrequencyMhz, btPowerDbm);

                WaveformGeneratorHelper waveformGeneratorHelper;
                waveformGeneratorHelper.SetChannel(spectrumChannel);
                waveformGeneratorHelper.SetTxPowerSpectralDensity(btPsd);
                waveformGeneratorHelper.SetPhyAttribute("Period", TimeValue(Seconds(hopDwellSeconds)));
                waveformGeneratorHelper.SetPhyAttribute("DutyCycle", DoubleValue(1.0));

                NetDeviceContainer btDevices = waveformGeneratorHelper.Install(nodes.Get(2));
                Ptr<WaveformGenerator> btWaveform = btDevices.Get(0)
                                                        ->GetObject<NonCommunicatingNetDevice>()
                                                        ->GetPhy()
                                                        ->GetObject<WaveformGenerator>();
                Simulator::Schedule(Seconds(hopStart), &WaveformGenerator::Start, btWaveform);
                Simulator::Schedule(Seconds(hopStart + hopDwellSeconds), &WaveformGenerator::Stop, btWaveform);
            }
        }
    }

    Ptr<PacketSink> sinkPtr = DynamicCast<PacketSink>(sinkApps.Get(0));
    sinkPtr->TraceConnectWithoutContext("Rx", MakeCallback(&PacketRx));

    Simulator::Stop(simTime);
    Simulator::Run();

    double throughput = (g_rxBytes * 8.0) / (simTime.GetSeconds() * 1e6);

    std::ofstream csv(outputFile, std::ios::app);
    if (csv.tellp() == 0)
        csv << "rng_run,bluetooth_enabled,distance_m,simulation_time_s,rx_packets,rx_bytes,throughput_mbps\n";
    csv << rngRun << "," << (btEnabled ? 1 : 0) << "," << distance << ","
        << simTime.GetSeconds() << "," << g_rxPackets << "," << g_rxBytes << ","
        << std::fixed << std::setprecision(4) << throughput << "\n";
    csv.close();

    std::cout << "Done: BT=" << (btEnabled ? "on" : "off") << " throughput=" << std::fixed << std::setprecision(2) << throughput << " Mbps\n";

    Simulator::Destroy();
    return 0;
}

#include "ns3/applications-module.h"
#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/network-module.h"
#include "ns3/non-communicating-net-device.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/spectrum-module.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/waveform-generator-helper.h"
#include "ns3/waveform-generator.h"
#include "ns3/wifi-module.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WiFiMicrowave");

struct RxSample
{
    double timeSeconds;
    uint32_t bytes;
};

std::array<std::vector<RxSample>, 2> g_rxSamples;

void PacketRx(uint32_t stationId, Ptr<const Packet> p, const Address& a)
{
    (void)a;
    g_rxSamples[stationId].push_back({Simulator::Now().GetSeconds(), p->GetSize()});
}

Ptr<SpectrumValue>
CreateMicrowavePsd(double centerFrequencyMhz, double txPowerDbm, double bandwidthMhz)
{
    Ptr<SpectrumModel> model = SpectrumModelIsm2400MhzRes1Mhz();
    Ptr<SpectrumValue> psd = Create<SpectrumValue>(model);

    const double txPowerW = std::pow(10.0, txPowerDbm / 10.0) / 1000.0;
    const double lowerFrequencyMhz = centerFrequencyMhz - (bandwidthMhz / 2.0);
    const double upperFrequencyMhz = centerFrequencyMhz + (bandwidthMhz / 2.0);

    double activeBandwidthHz = 0.0;
    auto bandIterator = psd->ConstBandsBegin();
    for (uint32_t index = 0; index < psd->GetValuesN(); ++index, ++bandIterator)
    {
        const auto& band = *bandIterator;
        const double bandCenterMhz = (band.fl + band.fh) / 2.0 / 1e6;
        const bool overlaps = (bandCenterMhz >= lowerFrequencyMhz) && (bandCenterMhz <= upperFrequencyMhz);
        if (overlaps)
        {
            activeBandwidthHz += (band.fh - band.fl);
        }
    }

    if (activeBandwidthHz <= 0.0)
    {
        return psd;
    }

    bandIterator = psd->ConstBandsBegin();
    for (uint32_t index = 0; index < psd->GetValuesN(); ++index, ++bandIterator)
    {
        const auto& band = *bandIterator;
        const double bandCenterMhz = (band.fl + band.fh) / 2.0 / 1e6;
        const bool overlaps = (bandCenterMhz >= lowerFrequencyMhz) && (bandCenterMhz <= upperFrequencyMhz);
        if (overlaps)
        {
            (*psd)[index] = txPowerW / activeBandwidthHz;
        }
    }

    return psd;
}

std::array<double, 3>
ComputePhaseThroughput(const std::vector<RxSample>& samples,
                      double simulationTimeSeconds,
                      double microwaveOnStartSeconds,
                      double microwaveOnStopSeconds)
{
    std::array<uint64_t, 3> bytesByPhase = {0, 0, 0};
    for (const auto& sample : samples)
    {
        const uint32_t phaseIndex = (sample.timeSeconds < microwaveOnStartSeconds)
                                        ? 0
                                        : (sample.timeSeconds < microwaveOnStopSeconds ? 1 : 2);
        bytesByPhase[phaseIndex] += sample.bytes;
    }

    const std::array<double, 3> phaseDurations = {
        microwaveOnStartSeconds,
        std::max(0.0, microwaveOnStopSeconds - microwaveOnStartSeconds),
        std::max(0.0, simulationTimeSeconds - microwaveOnStopSeconds),
    };

    std::array<double, 3> throughputMbps = {0.0, 0.0, 0.0};
    for (uint32_t phaseIndex = 0; phaseIndex < throughputMbps.size(); ++phaseIndex)
    {
        if (phaseDurations[phaseIndex] > 0.0)
        {
            throughputMbps[phaseIndex] = (bytesByPhase[phaseIndex] * 8.0) / (phaseDurations[phaseIndex] * 1e6);
        }
    }

    return throughputMbps;
}

std::string
PhaseName(double binMidpointSeconds, double microwaveOnStartSeconds, double microwaveOnStopSeconds)
{
    if (binMidpointSeconds < microwaveOnStartSeconds)
    {
        return "pre";
    }
    if (binMidpointSeconds < microwaveOnStopSeconds)
    {
        return "microwave-on";
    }
    return "post";
}

int main(int argc, char* argv[])
{
    bool microwaveEnabled = true;
    uint32_t rngRun = 1;
    Time simTime("12s");
    Time binWidth("100ms");
    std::string outputFile = "results/microwave/microwave-sweep.csv";
    std::string dataRateStr = "80Mbps";
    std::string wifiStandardStr = "802.11n";
    std::string deviceProfileStr = "baseline";
    double wifiChannelNumber = 6.0;
    double wifiChannelWidthMhz = 20.0;
    double station1DistanceM = 8.0;
    double station2DistanceM = 12.0;
    double station2OffsetYM = 3.0;
    double microwaveX = 5.5;
    double microwaveY = 1.5;
    double microwaveOnStartSeconds = 4.0;
    double microwaveOnStopSeconds = 8.0;
    double microwavePowerDbm = 0.0;
    double microwaveCenterFrequencyMhz = 2450.0;
    double microwaveBandwidthMhz = 20.0;

    CommandLine cmd(__FILE__);
    cmd.AddValue("microwave-enabled", "Enable microwave interferer", microwaveEnabled);
    cmd.AddValue("rng-run", "RNG run", rngRun);
    cmd.AddValue("simulation-time", "Simulation time", simTime);
    cmd.AddValue("bin-width", "Time bin width for throughput traces", binWidth);
    cmd.AddValue("output-csv", "Output CSV", outputFile);
    cmd.AddValue("data-rate", "AP offered data rate to each station", dataRateStr);
    cmd.AddValue("wifi-standard", "WiFi standard to use", wifiStandardStr);
    cmd.AddValue("device-profile", "Device profile preset (baseline, kitchen-microwave)", deviceProfileStr);
    cmd.AddValue("wifi-channel-number", "WiFi channel number", wifiChannelNumber);
    cmd.AddValue("wifi-channel-width-mhz", "WiFi channel width in MHz", wifiChannelWidthMhz);
    cmd.AddValue("station1-distance-m", "Distance from AP to station 1", station1DistanceM);
    cmd.AddValue("station2-distance-m", "Distance from AP to station 2", station2DistanceM);
    cmd.AddValue("station2-offset-y-m", "Y-axis offset for station 2", station2OffsetYM);
    cmd.AddValue("microwave-x-m", "Microwave x-position", microwaveX);
    cmd.AddValue("microwave-y-m", "Microwave y-position", microwaveY);
    cmd.AddValue("mw-on-start-s", "Microwave activation time", microwaveOnStartSeconds);
    cmd.AddValue("mw-on-stop-s", "Microwave deactivation time", microwaveOnStopSeconds);
    cmd.AddValue("mw-power-dbm", "Microwave leakage power in dBm", microwavePowerDbm);
    cmd.AddValue("mw-center-frequency-mhz", "Microwave center frequency in MHz", microwaveCenterFrequencyMhz);
    cmd.AddValue("mw-bandwidth-mhz", "Microwave occupied bandwidth in MHz", microwaveBandwidthMhz);
    cmd.Parse(argc, argv);

    if (deviceProfileStr == "kitchen-microwave")
    {
        wifiStandardStr = "802.11n";
        wifiChannelNumber = 6.0;
        wifiChannelWidthMhz = 20.0;
        dataRateStr = "80Mbps";
        station1DistanceM = 8.0;
        station2DistanceM = 12.0;
        station2OffsetYM = 3.0;
        microwaveX = 5.5;
        microwaveY = 1.5;
        microwaveOnStartSeconds = 4.0;
        microwaveOnStopSeconds = 8.0;
        microwavePowerDbm = 0.0;
        microwaveCenterFrequencyMhz = 2450.0;
        microwaveBandwidthMhz = 20.0;
    }

    if (microwaveOnStopSeconds <= microwaveOnStartSeconds)
    {
        NS_FATAL_ERROR("Microwave stop time must be greater than start time");
    }

    if (microwaveOnStopSeconds > simTime.GetSeconds())
    {
        NS_FATAL_ERROR("Microwave stop time must be within the simulation time");
    }

    RngSeedManager::SetRun(rngRun);
    NS_LOG_INFO("WiFi microwave simulation: microwave=" << (microwaveEnabled ? "on" : "off"));

    NodeContainer nodes;
    nodes.Create(4);

    WifiHelper wifi;
    if (wifiStandardStr == "802.11b" || wifiStandardStr == "b")
    {
        wifi.SetStandard(WIFI_STANDARD_80211b);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", StringValue("DsssRate11Mbps"),
                                     "ControlMode", StringValue("DsssRate1Mbps"));
    }
    else if (wifiStandardStr == "802.11g" || wifiStandardStr == "g")
    {
        wifi.SetStandard(WIFI_STANDARD_80211g);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", StringValue("ErpOfdmRate54Mbps"),
                                     "ControlMode", StringValue("ErpOfdmRate6Mbps"));
    }
    else if (wifiStandardStr == "802.11n" || wifiStandardStr == "n")
    {
        wifi.SetStandard(WIFI_STANDARD_80211n);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", StringValue("HtMcs7"),
                                     "ControlMode", StringValue("HtMcs0"));
    }
    else if (wifiStandardStr == "802.11ax" || wifiStandardStr == "ax")
    {
        wifi.SetStandard(WIFI_STANDARD_80211ax);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", StringValue("HeMcs11"),
                                     "ControlMode", StringValue("ErpOfdmRate54Mbps"));
    }
    else
    {
        wifi.SetStandard(WIFI_STANDARD_80211n);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode", StringValue("HtMcs7"),
                                     "ControlMode", StringValue("HtMcs0"));
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
    Ssid ssid("microwave-study");

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer devs = wifi.Install(phy, mac, nodes.Get(0));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    devs.Add(wifi.Install(phy, mac, nodes.Get(1)));
    devs.Add(wifi.Install(phy, mac, nodes.Get(2)));

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator>();
    pos->Add(Vector(0.0, 0.0, 0.0));
    pos->Add(Vector(station1DistanceM, 0.0, 0.0));
    pos->Add(Vector(station2DistanceM, station2OffsetYM, 0.0));
    pos->Add(Vector(microwaveX, microwaveY, 0.0));
    mobility.SetPositionAllocator(pos);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    InternetStackHelper inet;
    inet.Install(nodes.Get(0));
    inet.Install(nodes.Get(1));
    inet.Install(nodes.Get(2));

    Ipv4AddressHelper addr;
    addr.SetBase("10.2.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ifaces = addr.Assign(devs);

    Ipv4Address sta1Addr = ifaces.GetAddress(1);
    Ipv4Address sta2Addr = ifaces.GetAddress(2);

    uint16_t port1 = 9;
    uint16_t port2 = 10;

    PacketSinkHelper sink1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port1));
    PacketSinkHelper sink2("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port2));
    ApplicationContainer sinkApps1 = sink1.Install(nodes.Get(1));
    ApplicationContainer sinkApps2 = sink2.Install(nodes.Get(2));
    sinkApps1.Start(Seconds(0.0));
    sinkApps1.Stop(simTime);
    sinkApps2.Start(Seconds(0.0));
    sinkApps2.Stop(simTime);

    OnOffHelper source1("ns3::UdpSocketFactory", InetSocketAddress(sta1Addr, port1));
    source1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    source1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    source1.SetAttribute("DataRate", DataRateValue(DataRate(dataRateStr)));
    source1.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer sourceApps1 = source1.Install(nodes.Get(0));
    sourceApps1.Start(Seconds(0.5));
    sourceApps1.Stop(simTime);

    OnOffHelper source2("ns3::UdpSocketFactory", InetSocketAddress(sta2Addr, port2));
    source2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    source2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    source2.SetAttribute("DataRate", DataRateValue(DataRate(dataRateStr)));
    source2.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer sourceApps2 = source2.Install(nodes.Get(0));
    sourceApps2.Start(Seconds(0.5));
    sourceApps2.Stop(simTime);

    if (microwaveEnabled)
    {
        Ptr<SpectrumValue> microwavePsd = CreateMicrowavePsd(microwaveCenterFrequencyMhz, microwavePowerDbm, microwaveBandwidthMhz);
        WaveformGeneratorHelper waveformGeneratorHelper;
        waveformGeneratorHelper.SetChannel(spectrumChannel);
        waveformGeneratorHelper.SetTxPowerSpectralDensity(microwavePsd);
        waveformGeneratorHelper.SetPhyAttribute("Period", TimeValue(Seconds(1.0)));
        waveformGeneratorHelper.SetPhyAttribute("DutyCycle", DoubleValue(1.0));

        NetDeviceContainer microwaveDevices = waveformGeneratorHelper.Install(nodes.Get(3));
        Ptr<WaveformGenerator> microwaveWaveform = microwaveDevices.Get(0)
                                                       ->GetObject<NonCommunicatingNetDevice>()
                                                       ->GetPhy()
                                                       ->GetObject<WaveformGenerator>();
        Simulator::Schedule(Seconds(microwaveOnStartSeconds), &WaveformGenerator::Start, microwaveWaveform);
        Simulator::Schedule(Seconds(microwaveOnStopSeconds), &WaveformGenerator::Stop, microwaveWaveform);
    }

    Ptr<PacketSink> sinkPtr1 = DynamicCast<PacketSink>(sinkApps1.Get(0));
    Ptr<PacketSink> sinkPtr2 = DynamicCast<PacketSink>(sinkApps2.Get(0));
    sinkPtr1->TraceConnectWithoutContext("Rx", MakeBoundCallback(&PacketRx, 0));
    sinkPtr2->TraceConnectWithoutContext("Rx", MakeBoundCallback(&PacketRx, 1));

    Simulator::Stop(simTime);
    Simulator::Run();

    const double simulationTimeSeconds = simTime.GetSeconds();
    const double binWidthSeconds = binWidth.GetSeconds();
    const uint32_t numBins = static_cast<uint32_t>(std::ceil(simulationTimeSeconds / binWidthSeconds));

    std::array<std::vector<double>, 2> binThroughputMbps;
    std::array<std::vector<std::string>, 2> binPhase;
    std::array<std::vector<uint8_t>, 2> binMicrowaveActive;
    std::array<std::vector<uint64_t>, 2> binBytes;
    for (auto stationIndex = 0u; stationIndex < 2; ++stationIndex)
    {
        binThroughputMbps[stationIndex].assign(numBins, 0.0);
        binPhase[stationIndex].assign(numBins, "pre");
        binMicrowaveActive[stationIndex].assign(numBins, 0);
        binBytes[stationIndex].assign(numBins, 0);
    }

    for (uint32_t stationIndex = 0; stationIndex < 2; ++stationIndex)
    {
        for (const auto& sample : g_rxSamples[stationIndex])
        {
            uint32_t binIndex = static_cast<uint32_t>(sample.timeSeconds / binWidthSeconds);
            if (binIndex >= numBins)
            {
                binIndex = numBins - 1;
            }
            binBytes[stationIndex][binIndex] += sample.bytes;
        }

        for (uint32_t binIndex = 0; binIndex < numBins; ++binIndex)
        {
            const double binStartSeconds = binIndex * binWidthSeconds;
            const double binEndSeconds = std::min(simulationTimeSeconds, binStartSeconds + binWidthSeconds);
            const double effectiveBinWidthSeconds = std::max(1e-9, binEndSeconds - binStartSeconds);
            const double binMidpointSeconds = (binStartSeconds + binEndSeconds) / 2.0;
            binThroughputMbps[stationIndex][binIndex] = (binBytes[stationIndex][binIndex] * 8.0) / (effectiveBinWidthSeconds * 1e6);
            binPhase[stationIndex][binIndex] = PhaseName(binMidpointSeconds, microwaveOnStartSeconds, microwaveOnStopSeconds);
            binMicrowaveActive[stationIndex][binIndex] = (binPhase[stationIndex][binIndex] == "microwave-on") ? 1 : 0;
        }
    }

    std::array<double, 3> sta1PhaseThroughput = ComputePhaseThroughput(g_rxSamples[0], simulationTimeSeconds, microwaveOnStartSeconds, microwaveOnStopSeconds);
    std::array<double, 3> sta2PhaseThroughput = ComputePhaseThroughput(g_rxSamples[1], simulationTimeSeconds, microwaveOnStartSeconds, microwaveOnStopSeconds);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Run " << rngRun << ": "
              << "STA1 pre=" << sta1PhaseThroughput[0] << " Mbps, "
              << "microwave-on=" << sta1PhaseThroughput[1] << " Mbps, "
              << "post=" << sta1PhaseThroughput[2] << " Mbps; "
              << "STA2 pre=" << sta2PhaseThroughput[0] << " Mbps, "
              << "microwave-on=" << sta2PhaseThroughput[1] << " Mbps, "
              << "post=" << sta2PhaseThroughput[2] << " Mbps" << std::endl;

    std::ofstream csv(outputFile, std::ios::app);
    if (csv.tellp() == 0)
    {
        csv << "rng_run,station_id,station_label,bin_index,bin_start_s,bin_end_s,microwave_active,phase,rx_bytes,throughput_mbps\n";
    }

    for (uint32_t stationIndex = 0; stationIndex < 2; ++stationIndex)
    {
        const std::string stationLabel = (stationIndex == 0) ? "sta1" : "sta2";
        for (uint32_t binIndex = 0; binIndex < numBins; ++binIndex)
        {
            const double binStartSeconds = binIndex * binWidthSeconds;
            const double binEndSeconds = std::min(simulationTimeSeconds, binStartSeconds + binWidthSeconds);
            csv << rngRun << ","
                << stationIndex << ","
                << stationLabel << ","
                << binIndex << ","
                << std::fixed << std::setprecision(4) << binStartSeconds << ","
                << std::fixed << std::setprecision(4) << binEndSeconds << ","
                << static_cast<uint32_t>(binMicrowaveActive[stationIndex][binIndex]) << ","
                << binPhase[stationIndex][binIndex] << ","
                << binBytes[stationIndex][binIndex] << ","
                << std::fixed << std::setprecision(4) << binThroughputMbps[stationIndex][binIndex] << "\n";
        }
    }

    csv.close();

    Simulator::Destroy();
    return 0;
}

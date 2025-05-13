#include <fstream>
#include <string>
#include <cmath>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/inet-socket-address.h"
#include "ns3/traffic-control-module.h"
#include <iostream>
#include <algorithm>
#include <random>

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("ns3simulator");

double flow_time = 30.0;
uint maxBytes = 50 * 1024 * 1024; 
int port = 8331;

std::vector<double> tputsexp1, tputsexp2, tputs2exp2, tputsexp3, tputsexp4, tputs2exp4, tputsexp5, tputs2exp5;
std::vector<double> ftimeexp1, ftimeexp2, ftime2exp2, ftimeexp3, ftimeexp4, ftime2exp4, ftimeexp5, ftime2exp5;

float calculate_sd(float a, float b, float c)
{
  float mean = (a+b+c)/3.0;
  float summation = pow(a-mean,2) + pow(b-mean, 2) + pow(c-mean, 2);
  float sd = sqrt(summation/3.0);
  return sd;
}

void runExperimentWithFlows(int expNum, int iteration, std::string tcp1, std::string tcp2 = "") 
{
    std::cout << "\n======= Running Experiment " << expNum << ", Iteration " << iteration << " =======\n";
    
    Config::Reset();
    
    RngSeedManager::SetSeed(12345);
    RngSeedManager::SetRun(expNum * 10 + iteration);
    
    NodeContainer source, destination, router;
    source.Create(2);
    destination.Create(2);
    router.Create(2);
    
    InternetStackHelper internet;
    internet.Install(source);
    internet.Install(destination);
    internet.Install(router);
    
    if (tcp1 == "TcpBic") {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpBic::GetTypeId()));
    } 
    else if (tcp1 == "TcpDctcp") {
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpDctcp::GetTypeId()));
        Config::SetDefault("ns3::TcpSocketBase::UseEcn", StringValue("On"));
    }
    
    PointToPointHelper pointToPointEdge;
    pointToPointEdge.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    pointToPointEdge.SetChannelAttribute("Delay", StringValue("0ms"));
    
    PointToPointHelper pointToPointBottleneck;
    pointToPointBottleneck.SetDeviceAttribute("DataRate", StringValue("1Gbps"));
    pointToPointBottleneck.SetChannelAttribute("Delay", StringValue("0ms"));
    pointToPointBottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("100p"));
    
    NodeContainer link1 = NodeContainer(source.Get(0), router.Get(0));
    NodeContainer link2 = NodeContainer(source.Get(1), router.Get(0));
    NodeContainer link3 = NodeContainer(router.Get(0), router.Get(1));
    NodeContainer link4 = NodeContainer(router.Get(1), destination.Get(1));
    NodeContainer link5 = NodeContainer(router.Get(1), destination.Get(0));
    
    NetDeviceContainer s1linkr1 = pointToPointEdge.Install(link1);
    NetDeviceContainer s2linkr1 = pointToPointEdge.Install(link2);
    NetDeviceContainer r1linkr2 = pointToPointBottleneck.Install(link3);
    NetDeviceContainer r2linkd1 = pointToPointEdge.Install(link5);
    NetDeviceContainer r2linkd2 = pointToPointEdge.Install(link4);
    
    if (tcp1 == "TcpDctcp" || tcp2 == "TcpDctcp") {
        TrafficControlHelper tch;
        tch.SetRootQueueDisc("ns3::RedQueueDisc",
                           "UseEcn", BooleanValue(true));
        
        tch.Install(r1linkr2);
    }
    
    Ipv4AddressHelper address;
    
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i1 = address.Assign(s1linkr1);
    
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i2 = address.Assign(s2linkr1);
    
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i3 = address.Assign(r1linkr2);
    
    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i4 = address.Assign(r2linkd1);
    
    address.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i5 = address.Assign(r2linkd2);
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    Address destinationIP1 = InetSocketAddress(i4.GetAddress(1), port);
    Address destinationIP2 = InetSocketAddress(i5.GetAddress(1), port);
    
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(2 * 1024 * 1024));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(2 * 1024 * 1024));
    
    pointToPointEdge.SetDeviceAttribute("Mtu", UintegerValue(1500));
    pointToPointBottleneck.SetDeviceAttribute("Mtu", UintegerValue(1500));
    
    BulkSendHelper sourceHelper1("ns3::TcpSocketFactory", destinationIP1);
    sourceHelper1.SetAttribute("MaxBytes", UintegerValue(maxBytes));
    sourceHelper1.SetAttribute("SendSize", UintegerValue(1400));
    
    PacketSinkHelper packetSinkHelper1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer destApp1 = packetSinkHelper1.Install(destination.Get(0));
    ApplicationContainer sourceApp1 = sourceHelper1.Install(source.Get(0));
    
    destApp1.Start(Seconds(0.0));
    destApp1.Stop(Seconds(flow_time + 1.0));
    sourceApp1.Start(Seconds(0.1));
    sourceApp1.Stop(Seconds(flow_time));
    
    ApplicationContainer destApp2, sourceApp2;
    if (!tcp2.empty()) {
        if (tcp2 != tcp1) {
            if (tcp2 == "TcpBic") {
                Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpBic::GetTypeId()));
            } 
            else if (tcp2 == "TcpDctcp") {
                Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpDctcp::GetTypeId()));
                Config::SetDefault("ns3::TcpSocketBase::UseEcn", StringValue("On"));
            }
        }
        
        BulkSendHelper sourceHelper2("ns3::TcpSocketFactory", destinationIP2);
        sourceHelper2.SetAttribute("MaxBytes", UintegerValue(maxBytes));
        sourceHelper2.SetAttribute("SendSize", UintegerValue(1400));
        
        PacketSinkHelper packetSinkHelper2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
        destApp2 = packetSinkHelper2.Install(destination.Get(1));
        sourceApp2 = sourceHelper2.Install(source.Get(1));
        
        destApp2.Start(Seconds(0.0));
        destApp2.Stop(Seconds(flow_time + 1.0));
        sourceApp2.Start(Seconds(0.1));
        sourceApp2.Stop(Seconds(flow_time));
    }
    
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    
    std::cout << "Running simulation for " << flow_time << " seconds..." << std::endl;
    
    Simulator::Stop(Seconds(flow_time + 2.0));
    Simulator::Run();
    
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
    
    std::cout << "Number of flows detected: " << stats.size() << std::endl;
    
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i) {
        FlowId flowId = i->first;
        FlowMonitor::FlowStats fs = i->second;
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flowId);
        
        std::cout << "Flow " << flowId << " (" << t.sourceAddress << ":" << t.sourcePort 
                  << " -> " << t.destinationAddress << ":" << t.destinationPort << ")" << std::endl;
        std::cout << "  Tx Bytes: " << fs.txBytes << ", Rx Bytes: " << fs.rxBytes << std::endl;
        std::cout << "  Tx Packets: " << fs.txPackets << ", Rx Packets: " << fs.rxPackets << std::endl;
        std::cout << "  Lost Packets: " << fs.lostPackets << std::endl;
        
        if (fs.rxBytes > 1000000) {
            double time_taken = fs.timeLastRxPacket.GetSeconds() - fs.timeFirstTxPacket.GetSeconds();
            double curr_throughput = 0.0;
            
            if (time_taken > 0) {
                curr_throughput = fs.rxBytes * 8.0 / time_taken / (1024 * 1024);
            }
            
            std::cout << "  Time: " << time_taken << " seconds" << std::endl;
            std::cout << "  Throughput: " << curr_throughput << " Mbps" << std::endl;
            
            if (t.sourceAddress == i1.GetAddress(0)) {
                switch(expNum) {
                    case 1: tputsexp1.push_back(curr_throughput); ftimeexp1.push_back(time_taken); break;
                    case 2: tputsexp2.push_back(curr_throughput); ftimeexp2.push_back(time_taken); break;
                    case 3: tputsexp3.push_back(curr_throughput); ftimeexp3.push_back(time_taken); break;
                    case 4: tputsexp4.push_back(curr_throughput); ftimeexp4.push_back(time_taken); break;
                    case 5: tputsexp5.push_back(curr_throughput); ftimeexp5.push_back(time_taken); break;
                }
            } 
            else if (t.sourceAddress == i2.GetAddress(0)) {
                switch(expNum) {
                    case 2: tputs2exp2.push_back(curr_throughput); ftime2exp2.push_back(time_taken); break;
                    case 4: tputs2exp4.push_back(curr_throughput); ftime2exp4.push_back(time_taken); break;
                    case 5: tputs2exp5.push_back(curr_throughput); ftime2exp5.push_back(time_taken); break;
                }
            }
        } else {
            std::cout << "  Flow has low data transfer, skipping..." << std::endl;
        }
    }
    
    Simulator::Destroy();
    std::cout << "Experiment " << expNum << ", Iteration " << iteration << " completed" << std::endl;
}

int main(int argc, char *argv[])
{
    std::cout << "======= NS3 TCP Simulation Starting =======" << std::endl;
    
    for (int i = 0; i < 3; i++) {
        runExperimentWithFlows(1, i+1, "TcpBic");
    }
    
    for (int i = 0; i < 3; i++) {
        runExperimentWithFlows(2, i+1, "TcpBic", "TcpBic");
    }
    
    for (int i = 0; i < 3; i++) {
        runExperimentWithFlows(3, i+1, "TcpDctcp");
    }
    
    for (int i = 0; i < 3; i++) {
        runExperimentWithFlows(4, i+1, "TcpDctcp", "TcpDctcp");
    }
    
    for (int i = 0; i < 3; i++) {
        runExperimentWithFlows(5, i+1, "TcpBic", "TcpDctcp");
    }
    
    std::cout << "\n=========== Calculating Statistics ===========" << std::endl;
    
    if (tputsexp1.size() != 3 || tputsexp2.size() != 3 || tputs2exp2.size() != 3 ||
        tputsexp3.size() != 3 || tputsexp4.size() != 3 || tputs2exp4.size() != 3 ||
        tputsexp5.size() != 3 || tputs2exp5.size() != 3) {
        std::cout << "ERROR: Incorrect number of samples collected!" << std::endl;
        std::cout << "Exp1: " << tputsexp1.size() << ", Exp2: " << tputsexp2.size() << "/" << tputs2exp2.size() 
                 << ", Exp3: " << tputsexp3.size() << ", Exp4: " << tputsexp4.size() << "/" << tputs2exp4.size()
                 << ", Exp5: " << tputsexp5.size() << "/" << tputs2exp5.size() << std::endl;
        
        while (tputsexp1.size() < 3) tputsexp1.push_back(0.0);
        while (tputsexp2.size() < 3) tputsexp2.push_back(0.0);
        while (tputs2exp2.size() < 3) tputs2exp2.push_back(0.0);
        while (tputsexp3.size() < 3) tputsexp3.push_back(0.0);
        while (tputsexp4.size() < 3) tputsexp4.push_back(0.0);
        while (tputs2exp4.size() < 3) tputs2exp4.push_back(0.0);
        while (tputsexp5.size() < 3) tputsexp5.push_back(0.0);
        while (tputs2exp5.size() < 3) tputs2exp5.push_back(0.0);
        
        while (ftimeexp1.size() < 3) ftimeexp1.push_back(0.0);
        while (ftimeexp2.size() < 3) ftimeexp2.push_back(0.0);
        while (ftime2exp2.size() < 3) ftime2exp2.push_back(0.0);
        while (ftimeexp3.size() < 3) ftimeexp3.push_back(0.0);
        while (ftimeexp4.size() < 3) ftimeexp4.push_back(0.0);
        while (ftime2exp4.size() < 3) ftime2exp4.push_back(0.0);
        while (ftimeexp5.size() < 3) ftimeexp5.push_back(0.0);
        while (ftime2exp5.size() < 3) ftime2exp5.push_back(0.0);
    }
    
    float th_means[9];
    th_means[1] = (tputsexp1[0] + tputsexp1[1] + tputsexp1[2]) / 3;
    th_means[2] = (tputsexp2[0] + tputsexp2[1] + tputsexp2[2]) / 3;
    th_means[3] = (tputs2exp2[0] + tputs2exp2[1] + tputs2exp2[2]) / 3;
    th_means[4] = (tputsexp3[0] + tputsexp3[1] + tputsexp3[2]) / 3;
    th_means[5] = (tputsexp4[0] + tputsexp4[1] + tputsexp4[2]) / 3;
    th_means[6] = (tputs2exp4[0] + tputs2exp4[1] + tputs2exp4[2]) / 3;
    th_means[7] = (tputsexp5[0] + tputsexp5[1] + tputsexp5[2]) / 3;
    th_means[8] = (tputs2exp5[0] + tputs2exp5[1] + tputs2exp5[2]) / 3;

    float th_sds[9];
    th_sds[1] = calculate_sd(tputsexp1[0], tputsexp1[1], tputsexp1[2]);
    th_sds[2] = calculate_sd(tputsexp2[0], tputsexp2[1], tputsexp2[2]);
    th_sds[3] = calculate_sd(tputs2exp2[0], tputs2exp2[1], tputs2exp2[2]);
    th_sds[4] = calculate_sd(tputsexp3[0], tputsexp3[1], tputsexp3[2]);
    th_sds[5] = calculate_sd(tputsexp4[0], tputsexp4[1], tputsexp4[2]);
    th_sds[6] = calculate_sd(tputs2exp4[0], tputs2exp4[1], tputs2exp4[2]);
    th_sds[7] = calculate_sd(tputsexp5[0], tputsexp5[1], tputsexp5[2]);
    th_sds[8] = calculate_sd(tputs2exp5[0], tputs2exp5[1], tputs2exp5[2]);

    float afct_means[9];
    afct_means[1] = (ftimeexp1[0] + ftimeexp1[1] + ftimeexp1[2]) / 3;
    afct_means[2] = (ftimeexp2[0] + ftimeexp2[1] + ftimeexp2[2]) / 3;
    afct_means[3] = (ftime2exp2[0] + ftime2exp2[1] + ftime2exp2[2]) / 3;
    afct_means[4] = (ftimeexp3[0] + ftimeexp3[1] + ftimeexp3[2]) / 3;
    afct_means[5] = (ftimeexp4[0] + ftimeexp4[1] + ftimeexp4[2]) / 3;
    afct_means[6] = (ftime2exp4[0] + ftime2exp4[1] + ftime2exp4[2]) / 3;
    afct_means[7] = (ftimeexp5[0] + ftimeexp5[1] + ftimeexp5[2]) / 3;
    afct_means[8] = (ftime2exp5[0] + ftime2exp5[1] + ftime2exp5[2]) / 3;

    float afct_sds[9];
    afct_sds[1] = calculate_sd(ftimeexp1[0], ftimeexp1[1], ftimeexp1[2]);
    afct_sds[2] = calculate_sd(ftimeexp2[0], ftimeexp2[1], ftimeexp2[2]);
    afct_sds[3] = calculate_sd(ftime2exp2[0], ftime2exp2[1], ftime2exp2[2]);
    afct_sds[4] = calculate_sd(ftimeexp3[0], ftimeexp3[1], ftimeexp3[2]);
    afct_sds[5] = calculate_sd(ftimeexp4[0], ftimeexp4[1], ftimeexp4[2]);
    afct_sds[6] = calculate_sd(ftime2exp4[0], ftime2exp4[1], ftime2exp4[2]);
    afct_sds[7] = calculate_sd(ftimeexp5[0], ftimeexp5[1], ftimeexp5[2]);
    afct_sds[8] = calculate_sd(ftime2exp5[0], ftime2exp5[1], ftime2exp5[2]);

    std::cout << "\n=========== Writing Results to CSV ===========" << std::endl;
    std::ofstream outputFile("tcp_adevali_udiyora_tshah6.csv");
    
    outputFile<<"exp,r1_s1,r2_s1,r3_s1,avg_s1,std_s1,unit_s1,r1_s2,r2_s2,r3_s2,avg_s2,std_s2,unit_s2,";
    outputFile<<"\n";

    outputFile<<"th_1,"<<tputsexp1[0]<<","<<tputsexp1[1]<<","<<tputsexp1[2]<<","<<th_means[1]<<","<<th_sds[1]<<","<<"Mbps,"<<",,,,,,\n";
    outputFile<<"th_2,"<<tputsexp2[0]<<","<<tputsexp2[1]<<","<<tputsexp2[2]<<","<<th_means[2]<<","<<th_sds[2]<<","<<"Mbps,"<<tputs2exp2[0]<<","<<tputs2exp2[1]<<","<<tputs2exp2[2]<<","<<th_means[3]<<","<<th_sds[3]<<",Mbps,\n";
    outputFile<<"th_3,"<<tputsexp3[0]<<","<<tputsexp3[1]<<","<<tputsexp3[2]<<","<<th_means[4]<<","<<th_sds[4]<<","<<"Mbps,"<<",,,,,,\n";
    outputFile<<"th_4,"<<tputsexp4[0]<<","<<tputsexp4[1]<<","<<tputsexp4[2]<<","<<th_means[5]<<","<<th_sds[5]<<","<<"Mbps,"<<tputs2exp4[0]<<","<<tputs2exp4[1]<<","<<tputs2exp4[2]<<","<<th_means[6]<<","<<th_sds[6]<<",Mbps,\n";
    outputFile<<"th_5,"<<tputsexp5[0]<<","<<tputsexp5[1]<<","<<tputsexp5[2]<<","<<th_means[7]<<","<<th_sds[7]<<","<<"Mbps,"<<tputs2exp5[0]<<","<<tputs2exp5[1]<<","<<tputs2exp5[2]<<","<<th_means[8]<<","<<th_sds[8]<<",Mbps,\n";

    outputFile<<"afct_1,"<<ftimeexp1[0]<<","<< ftimeexp1[1]<<","<< ftimeexp1[2]<<","<< afct_means[1]<<","<<afct_sds[1]<<","<<"Seconds,"<<",,,,,,\n";
    outputFile<<"afct_2,"<<ftimeexp2[0]<<","<< ftimeexp2[1]<<","<< ftimeexp2[2]<<","<< afct_means[2]<<","<<afct_sds[2]<<","<<"Seconds,"<<ftime2exp2[0]<<","<<ftime2exp2[1]<<","<<ftime2exp2[2]<<","<<afct_means[3]<<","<<afct_sds[3]<<",Seconds,\n";
    outputFile<<"afct_3,"<<ftimeexp3[0]<<","<<ftimeexp3[1]<<","<<ftimeexp3[2]<<","<<afct_means[4]<<","<<afct_sds[4]<<","<<"Seconds,"<<",,,,,,\n";
    outputFile<<"afct_4,"<<ftimeexp4[0]<<","<<ftimeexp4[1]<<","<<ftimeexp4[2]<<","<<afct_means[5]<<","<<afct_sds[5]<<","<<"Seconds,"<<ftime2exp4[0]<<","<<ftime2exp4[1]<<","<<ftime2exp4[2]<<","<<afct_means[6]<<","<<afct_sds[6]<<",Seconds,\n";
    outputFile<<"afct_5,"<<ftimeexp5[0]<<","<<ftimeexp5[1]<<","<<ftimeexp5[2]<<","<<afct_means[7]<<","<<afct_sds[7]<<","<<"Seconds,"<<ftime2exp5[0]<<","<<ftime2exp5[1]<<","<<ftime2exp5[2]<<","<<afct_means[8]<<","<<afct_sds[8]<<",Seconds,\n";

    std::cout << "CSV file written successfully" << std::endl;
    std::cout << "======= Simulation Complete =======" << std::endl;
    
    return 0;
}
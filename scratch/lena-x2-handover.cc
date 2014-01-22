/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/lte-global-pathloss-database.h"
#include <math.h>

using namespace ns3;

#define kilo 1000
double simTime = 100;   //simulation time for EACH application
static double scheduler_timer=0; //timer to schedule position tracking


int isPedestrian = -1; //-1=no fading trace and no mobility, 0=vehicular trace, 1=pedestrian trace.
uint16_t traceTime = 100;       //trace file period, in seconds
std::string P_TRACE_FILE = "/home/binhn/ln/fading_traces/EPA_3kmh_100_dl.fad";
std::string V_TRACE_FILE = "/home/binhn/ln/fading_traces/EVA_60kmh_100_dl.fad";
std::string traceFile = P_TRACE_FILE;   //location of trace file.
uint16_t isFading = 1;



/*******Simulation******/
uint16_t radioUlBandwidth = 25;  
uint16_t radioDlBandwidth = 25;  //same as above, for downlink.
std::string SACK="1";
std::string TIME_STAMP="0";
std::string WINDOW_SCALING="1";
std::string TCP_VERSION="cubic"; //reno,westwood,vegas,veno,yeah,illinois,htcp,hybla
double X2_path_delay = 19; //X2 forwarding path delay in ms.
double S1_u_link_delay = 15;	//S1u between SGW and eNB in ms.
uint16_t isCost231 = 0;
double moving_bound = 50000;
static uint16_t is_random_allocation = 0;  //UEs fixed position allocation by default
double moving_speed = 3; //UE moving speed in km/h (3km/h for pedestrian, 60km/h for vehicular)



/****Application******/
static uint32_t isTcp=1;
uint32_t packetSize = 900;
double samplingInterval = 0.005;    /*getTcp() function invoke for each x second*/
uint16_t PUT_SAMPLING_INTERVAL = 40; /*sample a TCP throughput for each x pkts*/
double t = 0.0;
Ptr<ns3::FlowMonitor> monitor;
FlowMonitorHelper flowHelper;
Ptr<ns3::Ipv4FlowClassifier> classifier;
std::map <FlowId, FlowMonitor::FlowStats> stats;
std::string dataRate = "150Mb/s";
//LogLevel logLevel = (LogLevel) (LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC);
LogLevel logLevel = (LogLevel) (LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC | LOG_LEVEL_DEBUG);
uint32_t isAutoHo = 0;
double speed = 20; //20m/s

std::map<Ipv4Address, double> last_tx_time;
std::map<Ipv4Address, double> last_rx_time ;
std::map<Ipv4Address, double> last_tx_bytes ;
std::map<Ipv4Address, double> last_rx_bytes ;
std::map<Ipv4Address, double> tcp_delay ;
std::map<Ipv4Address, double> last_delay_sum ;
std::map<Ipv4Address, double> last_rx_pkts ;
std::map<Ipv4Address, double> last_tx_pkts ;
std::map<Ipv4Address, uint16_t> init_map ;
std::map<Ipv4Address, double> last_put_sampling_time;
/**sending flowS stats***/
std::map<Ipv4Address, double> meanTxRate_send;
std::map<Ipv4Address, double> meanRxRate_send;
std::map<Ipv4Address, double> meanTcpDelay_send;
std::map<Ipv4Address, uint64_t> numOfLostPackets_send;
std::map<Ipv4Address, uint64_t> numOfTxPacket_send;

const uint32_t ONEBIL = 1000000000;

uint16_t numberOfUes = 1;
uint16_t numberOfEnbs = 2;
uint16_t numBearersPerUe = 1;
double distance = 500.0;

/********DEBUGGING mode********/
uint32_t isDebug = 0;

/********* Ascii output files name *********/
static std::string DIR = "/var/tmp/ln_result/radio/";
static std::string macro = DIR+"macro_output.dat";
static std::string put_send;
static std::string debugger = DIR+"debugger.dat";
static std::string course_change = DIR+"course_change.dat";
static std::string overall = "overall.out";
static std::string position_tracking = DIR+"position_tracking.dat";

/********wrappers**********/
static AsciiTraceHelper asciiTraceHelper;
Ptr<OutputStreamWrapper> put_send_wp;
Ptr<OutputStreamWrapper> macro_wp;
Ptr<OutputStreamWrapper> debugger_wp;
Ptr<OutputStreamWrapper> ue_positions_wp;
Ptr<OutputStreamWrapper> overall_wp;
Ptr<OutputStreamWrapper> position_tracking_wp;



static void getTcpPut();
void EnableLogComponents();
void SetDefaultConfigs();
void CommandlineParameters(int argc, char* argv[]);
void InstallMobility(NodeContainer ueNodes, NodeContainer enbNodes);
void InstallFading(Ptr<LteHelper> lteHelper);
void EnablePositionTracking(NetDeviceContainer enbLteDevs, NodeContainer ueNodes);
static void init_wrappers();
static void pos_tracking (Ptr<OutputStreamWrapper> position_tracking_wp, Ptr<const MobilityModel> model);
 


void ConfigStoreOutput(std::string);
void ConfigStoreInput(std::string in_f);

//******Handover monitoring********//
Ptr<OutputStreamWrapper> ho_wp = asciiTraceHelper.CreateFileStream(DIR+"handover.dat");
void 
NotifyConnectionEstablishedUe (std::string context, 
                               uint64_t imsi, 
                               uint16_t cellid, 
                               uint16_t rnti)
{
  *ho_wp->GetStream() << Simulator::Now().GetSeconds() 
	    << " : " << context 
            << " UE IMSI " << imsi 
            << ": connected to CellId " << cellid 
            << " with RNTI " << rnti 
            << std::endl;
}

void 
NotifyHandoverStartUe (std::string context, 
                       uint64_t imsi, 
                       uint16_t cellid, 
                       uint16_t rnti, 
                       uint16_t targetCellId)
{
   *ho_wp->GetStream() << Simulator::Now().GetSeconds() 
	    << " : " << context 
            << " UE IMSI " << imsi 
            << ": previously connected to CellId " << cellid 
            << " with RNTI " << rnti 
            << ", doing handover to CellId " << targetCellId 
            << std::endl;
}

void 
NotifyHandoverEndOkUe (std::string context, 
                       uint64_t imsi, 
                       uint16_t cellid, 
                       uint16_t rnti)
{
  *ho_wp->GetStream() << Simulator::Now().GetSeconds() 
	    << " : " << context 
            << " UE IMSI " << imsi 
            << ": successful handover to CellId " << cellid 
            << " with RNTI " << rnti 
            << std::endl;
}

void 
NotifyConnectionEstablishedEnb (std::string context, 
                                uint64_t imsi, 
                       uint16_t cellid,
                       uint16_t rnti)
{
   *ho_wp->GetStream() << Simulator::Now().GetSeconds()
            << " : " << context
            << " eNB CellId " << cellid
            << ": successful connection of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti,
                        uint16_t targetCellId)
{
  *ho_wp->GetStream() << Simulator::Now().GetSeconds()
            << " : " << context
            << " eNB CellId " << cellid
            << ": start handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << " to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti)
{
  *ho_wp->GetStream() << Simulator::Now().GetSeconds()
            << " : " << context
            << " eNB CellId " << cellid
            << ": completed handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

static void
CourseChange (Ptr<OutputStreamWrapper> ue_positions_wp, Ptr<const MobilityModel> model);


/**
 * Sample simulation script for a X2-based handover.
 * It instantiates two eNodeB, attaches one UE to the 'source' eNB and
 * triggers a handover of the UE towards the 'target' eNB.
 */
NS_LOG_COMPONENT_DEFINE ("EpcX2HandoverExample");
int
main (int argc, char *argv[])
{
  // change some default attributes so that they are reasonable for
  // this scenario, but do this before processing command line
  // arguments, so that the user is allowed to override these settings 
  SetDefaultConfigs();
  ConfigStoreInput("lte.in");


  // Command line arguments
  CommandlineParameters(argc, argv);
  init_wrappers();
  EnableLogComponents();
  
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");

  Ptr<Node> pgw = epcHelper->GetPgwNode ();
	epcHelper->SetAttribute("S1uLinkDataRate", DataRateValue (DataRate ("1Gb/s")));
  epcHelper->SetAttribute("S1uLinkDelay", TimeValue (Seconds (0.015)));
  epcHelper->SetAttribute("S1uLinkMtu", UintegerValue (1500));

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
    internet.SetTcp("ns3::NscTcpL4Protocol", "Library", StringValue("liblinux2.6.26.so"));
    internet.Install (remoteHost);
   //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_rmem", StringValue ("4096 87380 8338608"));
;

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.015)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);


  // Routing of the Internet Host (towards the LTE network)
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  // interface 0 is localhost, 1 is the p2p device
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  if (isAutoHo==1){
	numberOfEnbs = 5;
	simTime = numberOfEnbs*1000/speed;
  }
  enbNodes.Create(numberOfEnbs);
  ueNodes.Create(numberOfUes);

  // Install Mobility Model
  InstallMobility(ueNodes, enbNodes);

  // Install Fading Model
  InstallFading(lteHelper);

  // Install LTE Devices in eNB and UEs
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  EnablePositionTracking(enbLteDevs, ueNodes);

    // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_sack", StringValue (SACK));
    Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_timestamps", StringValue (TIME_STAMP));
    Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_window_scaling", StringValue (WINDOW_SCALING));
    Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue (TCP_VERSION));
    //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.core.wmem_max", StringValue ("8338608"));
    //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.core.rmem_max", StringValue ("8338608"));
    Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_rmem", StringValue ("4096 8000000 8338608"));
    //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_wmem", StringValue ("4096 5000000 8338608"));
   
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }


  // Attach all UEs to the first eNodeB
  for (uint16_t i = 0; i < numberOfUes; i++)
    {
      lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));
    }


  NS_LOG_LOGIC ("setting up applications");
    
  // Install and start applications on UEs and remote host
  uint16_t dlPort = 10000;
  uint16_t ulPort = 20000;

  // randomize a bit start times to avoid simulation artifacts
  // (e.g., buffer overflows due to packet transmissions happening
  // exactly at the same time) 
  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0.5));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.6));
     
  for (uint32_t u = 0; u < numberOfUes; ++u)
    {
      Ptr<Node> ue = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

      for (uint32_t b = 0; b < numBearersPerUe; ++b)
        {
          ++dlPort;
          ++ulPort;

          ApplicationContainer clientApps;
          ApplicationContainer serverApps;


        if (isTcp == 1){
		LogComponentEnable("Queue",logLevel);    //Only enable Queue monitoring for TCP to accelerate experiment speed.
		PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
		serverApps.Add(sink.Install(ueNodes.Get(u)));

		OnOffHelper onOffHelper("ns3::TcpSocketFactory", Address ( InetSocketAddress(ueIpIfaces.GetAddress(u) , dlPort) ));
		onOffHelper.SetConstantRate( DataRate(dataRate), packetSize );
		clientApps.Add(onOffHelper.Install(remoteHost));
        }
        else{
						PUT_SAMPLING_INTERVAL = PUT_SAMPLING_INTERVAL*20;
						PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
						serverApps.Add(sink.Install(ueNodes.Get(u)));

						OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address ( InetSocketAddress(ueIpIfaces.GetAddress(u), dlPort) ));
						onOffHelper.SetConstantRate( DataRate(dataRate), packetSize );
						clientApps.Add(onOffHelper.Install(remoteHost));
						/*
						PacketSinkHelper ul_sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
						serverApps.Add(ul_sink.Install(remoteHost));

						OnOffHelper ul_onOffHelper("ns3::UdpSocketFactory", Address ( InetSocketAddress(remoteHostAddr, ulPort) ));
						ul_onOffHelper.SetConstantRate( DataRate(dataRate), packetSize );
						clientApps.Add(ul_onOffHelper.Install(ueNodes.Get(u)));          
						*/
	}
 	/*

	PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
	serverApps.Add(sink.Install(ueNodes.Get(u)));

	OnOffHelper onOffHelper("ns3::TcpSocketFactory", Address ( InetSocketAddress(ueIpIfaces.GetAddress(u) , dlPort) ));
	onOffHelper.SetConstantRate( DataRate("150Mb/s"), 900 );
	clientApps.Add(onOffHelper.Install(remoteHost));
	*/
	/*
          NS_LOG_LOGIC ("installing UDP DL app for UE " << u);
          UdpClientHelper dlClientHelper (ueIpIfaces.GetAddress (u), dlPort);
          clientApps.Add (dlClientHelper.Install (remoteHost));
          PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", 
                                               InetSocketAddress (Ipv4Address::GetAny (), dlPort));
          serverApps.Add (dlPacketSinkHelper.Install (ue));
              
          NS_LOG_LOGIC ("installing UDP UL app for UE " << u);
          UdpClientHelper ulClientHelper (remoteHostAddr, ulPort);
          clientApps.Add (ulClientHelper.Install (ue));
          PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", 
                                               InetSocketAddress (Ipv4Address::GetAny (), ulPort));
          serverApps.Add (ulPacketSinkHelper.Install (remoteHost));  
          */   
          Ptr<EpcTft> tft = Create<EpcTft> ();
          EpcTft::PacketFilter dlpf;
          dlpf.localPortStart = dlPort;
          dlpf.localPortEnd = dlPort;
          tft->Add (dlpf); 
          EpcTft::PacketFilter ulpf;
          ulpf.remotePortStart = ulPort;
          ulpf.remotePortEnd = ulPort;
          tft->Add (ulpf);
          EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
          lteHelper->ActivateDedicatedEpsBearer (ueLteDevs.Get (u), bearer, tft);
          
          Time startTime = Seconds (startTimeSeconds->GetValue ());
          serverApps.Start (startTime);
          clientApps.Start (startTime);

        } // end for b
    }


  // Add X2 inteface
  lteHelper->AddX2Interface (enbNodes);

  // X2-based Handover
  if (isAutoHo == 1){
	*debugger_wp->GetStream ()<< "Auto Handover..., ServingCellHandoverThreshold = 30, NeighbourCellHandoverOffset = 1\n" ;
	Config::SetDefault("ns3::A2A4RsrqHandoverAlgorithm::ServingCellThreshold",UintegerValue (30));
	Config::SetDefault("ns3::A2A4RsrqHandoverAlgorithm::NeighbourCellOffset",UintegerValue (1));
  }
  else{
	  *debugger_wp->GetStream () << "Manual Handover at 20,50,80 second\n" ;
	  lteHelper->HandoverRequest (Seconds (10.00), ueLteDevs.Get (0), enbLteDevs.Get (0), enbLteDevs.Get (1));
	  lteHelper->HandoverRequest (Seconds (20.00), ueLteDevs.Get (0), enbLteDevs.Get (1), enbLteDevs.Get (0));
	  //lteHelper->HandoverRequest (Seconds (70.00), ueLteDevs.Get (0), enbLteDevs.Get (0), enbLteDevs.Get (1));
	  //lteHelper->HandoverRequest (Seconds (100.00), ueLteDevs.Get (0), enbLteDevs.Get (1), enbLteDevs.Get (0));
	  //lteHelper->HandoverRequest (Seconds (130.00), ueLteDevs.Get (0), enbLteDevs.Get (0), enbLteDevs.Get (1));
  }
    monitor = flowHelper.Install(ueNodes);
    monitor = flowHelper.Install(remoteHost);
    monitor = flowHelper.GetMonitor();  

  // Uncomment to enable PCAP tracing
  p2ph.EnablePcapAll(DIR+"lena-x2-handover");

  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  lteHelper->EnablePdcpTraces ();
  Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
  rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (0.01)));
  Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats ();
  pdcpStats->SetAttribute ("EpochDuration", TimeValue (Seconds (0.01)));


  // connect custom trace sinks for RRC connection establishment and handover notification
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkUe));

 /*=============schedule to get TCP throughput============*/
  Time t = Seconds(0.0);
  Simulator::ScheduleWithContext (0 ,Seconds (0.0), &getTcpPut);

 
  ConfigStoreOutput("lte.out");

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  monitor->CheckForLostPackets();
  Ptr<ns3::Ipv4FlowClassifier> classifier = DynamicCast<ns3::Ipv4FlowClassifier> (flowHelper.GetClassifier());
  std::map <FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter){
	    ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);

	   *macro_wp->GetStream()  << "***Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << ":" << t.sourcePort 
			<< " Dst Addr " << t.destinationAddress << ":" << t.destinationPort  << std::endl
	    << "Tx Packets " << iter->second.txPackets << std::endl
	    << "Rx Packets " << iter->second.rxPackets << std::endl
	    << "Lost packets " << iter->second.lostPackets << std::endl
	    << "Lost ratio " << double (iter->second.lostPackets)/(iter->second.lostPackets+iter->second.rxPackets) << std::endl;
	    double ONEBIL=1000000000;
	    if (iter->second.rxPackets > 1){
     		*macro_wp->GetStream()   << "Average delay received " 
		<< iter->second.delaySum/iter->second.rxPackets/1000000 << std::endl
        	<< "Mean received bitrate " 
		<< 8*iter->second.rxBytes/(iter->second.timeLastRxPacket-iter->second.timeFirstRxPacket)*ONEBIL/(1024) 
		<< std::endl
        	<< "Mean transmitted bitrate " 
		<< 8*iter->second.txBytes/(iter->second.timeLastTxPacket-iter->second.timeFirstTxPacket)*ONEBIL/(1024) 
		<< std::endl;
   	    }	
	}
	

  // GtkConfigStore config;
  // config.ConfigureAttributes();

  Simulator::Destroy();
  return 0;

}


static void
getTcpPut(){
    monitor->CheckForLostPackets();
    classifier = DynamicCast<ns3::Ipv4FlowClassifier> (flowHelper.GetClassifier());
    stats = monitor->GetFlowStats();

    /*==============Get flows information============*/
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter){
    ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
        if (init_map[t.destinationAddress] != 1){
                init_map[t.destinationAddress] = 1;
                meanTxRate_send[t.destinationAddress] = 0;
                meanRxRate_send[t.destinationAddress] = 0;
                last_tx_time[t.destinationAddress] = 0;
                last_tx_bytes[t.destinationAddress] = 0;
                last_rx_time[t.destinationAddress] = 0;
                last_rx_bytes[t.destinationAddress] = 0;
                meanTcpDelay_send[t.destinationAddress] = 0;
                last_tx_pkts[t.destinationAddress] = 0;
                last_put_sampling_time[t.destinationAddress] = 0;
                tcp_delay[t.destinationAddress] = 0;
                last_delay_sum[t.destinationAddress] = 0;
                last_rx_pkts[t.destinationAddress] = 0;
        }
        /*sending/receiving rate*/
      if (iter->second.txPackets > last_tx_pkts[t.destinationAddress] + PUT_SAMPLING_INTERVAL && iter->second.timeLastTxPacket > last_tx_time[t.destinationAddress]){
                meanTxRate_send[t.destinationAddress] = 8*(iter->second.txBytes-last_tx_bytes[t.destinationAddress])/(iter->second.timeLastTxPacket.GetDouble()-last_tx_time[t.destinationAddress])*ONEBIL/kilo;
                meanRxRate_send[t.destinationAddress] = 8*(iter->second.rxBytes-last_rx_bytes[t.destinationAddress])/(iter->second.timeLastRxPacket.GetDouble()-last_rx_time[t.destinationAddress])*ONEBIL/kilo;
		last_tx_time[t.destinationAddress] = iter->second.timeLastTxPacket.GetDouble();
                last_tx_bytes[t.destinationAddress] = iter->second.txBytes;
                last_rx_time[t.destinationAddress] = iter->second.timeLastRxPacket.GetDouble();
                last_rx_bytes[t.destinationAddress] = iter->second.rxBytes;
                last_tx_pkts[t.destinationAddress] = iter->second.txPackets;
                last_put_sampling_time[t.destinationAddress] = Simulator::Now().GetSeconds();
        }
      numOfLostPackets_send[t.destinationAddress] = iter->second.lostPackets;
        /*end-to-end delay sampling*/
      if (iter->second.rxPackets > last_rx_pkts[t.destinationAddress]){
                    tcp_delay[t.destinationAddress] = (iter->second.delaySum.GetDouble() - last_delay_sum[t.destinationAddress]) / (iter->second.rxPackets - last_rx_pkts[t.destinationAddress])/(kilo*kilo);
                    last_delay_sum[t.destinationAddress] = iter->second.delaySum.GetDouble();
                    last_rx_pkts[t.destinationAddress] = iter->second.rxPackets;
        }

      numOfTxPacket_send[t.destinationAddress] = iter->second.txPackets;
  }

    std::map<Ipv4Address,double>::iterator it1 = meanRxRate_send.begin();
    std::map<Ipv4Address,uint64_t>::iterator it3 = numOfLostPackets_send.begin();
    std::map<Ipv4Address,uint64_t>::iterator it4 = numOfTxPacket_send.begin();
    std::map<Ipv4Address,double>::iterator it5 = meanTxRate_send.begin();
    std::map<Ipv4Address,double>::iterator it6 = tcp_delay.begin();

    for (;it1 != meanRxRate_send.end(); ){
      *put_send_wp->GetStream() << Simulator::Now().GetSeconds() << "\t\t"
                  << (*it1).first << "\t\t"
                  << (*it1).second << "\t\t"
                  << "x" << "\t\t"
                  << (*it3).second << "\t\t"
                  << (*it4).second << "\t\t"
                  << "x" << "\t\t"
                  << "x" << "\t\t"
		  << "x" << "\t\t"
                  << "x" << "\t"
                  << (*it5).second << "\t"
                  << (*it6).second <<  "\n";
                  ++it1;
                  ++it3;
                  ++it4;
                  ++it5;
                  ++it6;
    }

    while (t < simTime){
        t += samplingInterval;
        Simulator::Schedule(Seconds(t),&getTcpPut);
    }
}


void ConfigStoreOutput(std::string out_f){
    /****ConfigStore setting****/
    Config::SetDefault("ns3::ConfigStore::Filename", StringValue(out_f));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("RawText"));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Save"));
    ConfigStore outputConfig;
    outputConfig.ConfigureDefaults();
    outputConfig.ConfigureAttributes();
}

void ConfigStoreInput(std::string in_f){
    Config::SetDefault("ns3::ConfigStore::Filename", StringValue("lte.in"));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("RawText"));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Load"));
    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();
    inputConfig.ConfigureAttributes();
}

void EnableLogComponents(){
	if (isTcp==1){
		if (isDebug==0){
			LogLevel logLevel = (LogLevel) (LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC);
			//LogComponentEnable ("LteHelper", logLevel);
  		//LogComponentEnable ("EpcHelper", logLevel);
  		LogComponentEnable ("EpcEnbApplication", logLevel);
  		LogComponentEnable ("EpcX2", logLevel);
  		LogComponentEnable ("EpcSgwPgwApplication", logLevel);
  		LogComponentEnable ("NscTcpSocketImpl",LOG_LEVEL_DEBUG);
  		LogComponentEnable ("LteEnbNetDevice", logLevel);
  		LogComponentEnable ("LteUeRrc", logLevel);
  		LogComponentEnable ("LteUeNetDevice", logLevel);
		}
  	LogComponentEnable ("LteUeRrc", logLevel);
  	LogComponentEnable ("LteRlcUm", logLevel);
  	LogComponentEnable ("LteRlcAm", logLevel);
  	LogComponentEnable ("LteEnbRrc", logLevel);
  	LogComponentEnable ("LtePdcp", logLevel);
  	LogComponentEnable ("EpcX2", logLevel);
 }
}
void SetDefaultConfigs(){
  Config::SetDefault ("ns3::UdpClient::Interval", TimeValue (MicroSeconds(10000)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets", UintegerValue(1000000));
  Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue(false));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue(10000));
  Config::SetDefault ("ns3::DropTailQueue::MaxBytes", UintegerValue(999999));
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(150000));
  Config::SetDefault ("ns3::PointToPointEpcHelper::X2LinkDelay", TimeValue (MilliSeconds(X2_path_delay)));
  Config::SetDefault ("ns3::PointToPointEpcHelper::S1uLinkDelay", TimeValue (MilliSeconds(S1_u_link_delay)));

  //if (isFading==1)
  //	Config::SetDefault("ns3::LteHelper::FadingModel", StringValue("ns3::TraceFadingLossModel"));
}
void CommandlineParameters(int argc, char* argv[]){
  CommandLine cmd;
    cmd.AddValue("numberOfUes", "Number of UeNodes", numberOfUes);
    cmd.AddValue("numberOfEnbs", "Number of eNodebs", numberOfEnbs);
    cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
    cmd.AddValue("distance", "Distance between eNBs [m]", distance);
    cmd.AddValue("packetSize", "Size of each packet", packetSize);
    /*
    cmd.AddValue("s1uLinkDataRate", "S1u Link Data Rate", s1uLinkDataRate);
    cmd.AddValue("s1uLinkDelay", "S1u Link Delay", s1uLinkDelay);
    cmd.AddValue("s1uLinkMtu", "S1u Link Mtu", s1uLinkMtu);
    cmd.AddValue("p2pLinkDataRate", "p2p Link Data Rate", p2pLinkDataRate);
    cmd.AddValue("p2pLinkDelay", "p2p Link Delay", p2pLinkDelay);
    cmd.AddValue("p2pLinkMtu", "p2p Link Mtu", p2pLinkMtu);
    */
    cmd.AddValue("radioUlBandwidth", "Uplink radio bandwidth [RBs] (6,15,25,50,75,100)", radioUlBandwidth);
    cmd.AddValue("radioDlBandwidth", "Downlink radio bandwidth [RBs] (6,15,25,50,75,100)", radioDlBandwidth);
    cmd.AddValue("isFading", "Whether enabling trace fading", isFading);
    cmd.AddValue("dataRate", "TCP application data rate", dataRate);
    cmd.AddValue("isTcp", "TCP application if true, Udp if false", isTcp);
    cmd.AddValue("isPedestrian", "Whether using pedestrian fading trace and mobility (-1 for no fading/mobility, 0 for vehicular, 1 for pedestrian", isPedestrian);
    cmd.AddValue("SACK", "TCP SACK", SACK);
    cmd.AddValue("TIME_STAMP", "TCP TIME_STAMP", TIME_STAMP);
    cmd.AddValue("WINDOW_SCALING", "TCP WINDOW_SCALING", WINDOW_SCALING);
    cmd.AddValue("isAutoHo", "Whether doing auto handover", isAutoHo);
    cmd.AddValue("isDebug", "Whether in debug mode (turn on/off log)", isDebug);

  cmd.Parse(argc, argv);
}

void InstallMobility(NodeContainer ueNodes, NodeContainer enbNodes){
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                    "MinX", DoubleValue (0.0),  //zero point
                    "MinY", DoubleValue (0.0),  //zero point
                    "DeltaX", DoubleValue (300.0),  //distance among ENB nodes
                    "DeltaY", DoubleValue (300.0),
                    "GridWidth", UintegerValue (10), //number of nodes on a line
                    "LayoutType", StringValue ("RowFirst"));
    enbMobility.Install (enbNodes); //===ENB #1 placed at (0.0)====//

    MobilityHelper ueMobility;
   // Vector enbPosition = (*enbLteDevIt)->GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
    if (is_random_allocation == 0){ //fixed position allocation
        *debugger_wp->GetStream() << "Allocating UEs with FIXED positions ....\n";
        ueMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
	    "MinX", DoubleValue (distance),  //1st UE is put at a location which is "distance" meters away from eNB.
	    "MinY", DoubleValue (0),  
	    "DeltaX", DoubleValue (100.0),  //distance among UE nodes
	    "DeltaY", DoubleValue (100.0),
	    "GridWidth", UintegerValue (3), //number of nodes on a line
	    "LayoutType", StringValue ("RowFirst"));
    }
    else{       //random distributed allocation
            *debugger_wp->GetStream() << "Allocating UEs with RANDOM DISTRIBUTED positions ....\n ";
            ueMobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", //nodes are put randomly inside a circle with the central point is (x,y).
		     "X", DoubleValue (0),
		     "Y", DoubleValue (0),
		     "rho", DoubleValue (distance));  //radius of the circle.
        }

    if (isPedestrian == -1){ //constant position
	ueMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	*debugger_wp->GetStream() << "NO MOBILITY (ConstantPositionMobilityModel) .... \n";
    }
    else {      //randomwalking enabled
	    if (isPedestrian==0) moving_speed = 50;     //vehicular mobility
	    double speed = double (moving_speed)*1000/3600; //kmph to meter per second.
	    *debugger_wp->GetStream() << "RandomWalk2dMobilityModel MOBILITY ... speed= " << speed << std::endl;
	    std::stringstream mss;
	    mss << speed;
	    std::string ms = mss.str();
	    ueMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
		     "Mode", StringValue ("Time"),  //change distance and speed based on TIME.
		     "Time", StringValue ("200s"), //change direction and speed after each 2s.
		     "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),  //m/s
			   "Speed", StringValue ("ns3::ConstantRandomVariable[Constant="+ms+"]"),  //m/s
			   "Bounds", RectangleValue (Rectangle (-moving_bound, moving_bound, -moving_bound, moving_bound)));  //bound
    }
    ueMobility.Install(ueNodes);

}

void InstallFading(Ptr<LteHelper> lteHelper){
    if(isPedestrian==1 && isFading ==1)
        traceFile = P_TRACE_FILE;
    else if (isPedestrian==0 && isFading == 1) traceFile = V_TRACE_FILE;

    if (isPedestrian >= 0 && isFading == 1){ //enable trace fading if isPedestrian=1 or vehicular (isPedestrian=0)
      *debugger_wp->GetStream() << "Trace fading enabled....\n";
      Config::SetDefault("ns3::LteHelper::FadingModel", StringValue("ns3::TraceFadingLossModel"));
      lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
      lteHelper->SetFadingModelAttribute("TraceLength",TimeValue(Seconds(traceTime)));
      lteHelper->SetFadingModelAttribute("SamplesNum",UintegerValue(traceTime*1000));  /*1sample/1ms*/
      lteHelper->SetFadingModelAttribute("WindowSize",TimeValue(Seconds(0.5)));
      lteHelper->SetFadingModelAttribute("RbNum",UintegerValue(radioDlBandwidth));
      lteHelper->SetFadingModelAttribute("TraceFilename", StringValue(traceFile));
      NS_LOG_UNCOND("Trace fading:\n"
                  << "========================"
                  << "\nisPedestrian= " << isPedestrian
                  << "\ntraceTime= " << traceTime
                  << "\nradioDlBandwidth= " << radioDlBandwidth
                  << "\ntraceFile= " << traceFile);
    }
    else *debugger_wp->GetStream() << "Trace fading disabled....\n";
}

void EnablePositionTracking(NetDeviceContainer enbLteDevs, NodeContainer ueNodes){
    NetDeviceContainer::Iterator enbLteDevIt = enbLteDevs.Begin ();
    for (NetDeviceContainer::Iterator enbLteDevIt = enbLteDevs.Begin(); enbLteDevIt != enbLteDevs.End(); enbLteDevIt++){
    	Vector enbPosition = (*enbLteDevIt)->GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
    	*debugger_wp->GetStream() << "eNB(x,y)= " << enbPosition.x << ", " << enbPosition.y << std::endl;
		}
		for (uint16_t i = 0; i < ueNodes.GetN(); i++ ){
			Ptr<MobilityModel> ue_mobility_model = ueNodes.Get(i)->GetObject<MobilityModel>();
    	double x = ue_mobility_model->GetPosition().x;
    	double y = ue_mobility_model->GetPosition().y;
    	*debugger_wp->GetStream() << "UE (x,y)= " << x << ", " << y << " d= " << sqrt(x*x+y*y) << std::endl;
    	ue_mobility_model->TraceConnectWithoutContext("CourseChange", MakeBoundCallback(&CourseChange, ue_positions_wp));
			//Tracking.
    	Simulator::Schedule(Seconds(1), &pos_tracking, position_tracking_wp, ue_mobility_model);
		}
}


static void
CourseChange (Ptr<OutputStreamWrapper> ue_positions_wp, Ptr<const MobilityModel> model){
        Vector position = model->GetPosition();
        Vector vel = model->GetVelocity();
        *ue_positions_wp->GetStream() << Simulator::Now().GetSeconds() << " (x,y)= " << position.x << " , " 
                                   << position.y
                                   << " d= " << sqrt(position.x*position.x+position.y*position.y) 
                                   << " v= "
                                   << sqrt (vel.x*vel.x + vel.y*vel.y)
                                   << std::endl;
}


static void
pos_tracking (Ptr<OutputStreamWrapper> position_tracking_wp, Ptr<const MobilityModel> model){
        Vector position = model->GetPosition();
        Vector vel = model->GetVelocity();
        *position_tracking_wp->GetStream() << Simulator::Now().GetSeconds() << " (x,y)= " << position.x << " , "
                                   << position.y
                                   << " d= " << sqrt(position.x*position.x+position.y*position.y)
                                   << " v= "
                                   << sqrt (vel.x*vel.x + vel.y*vel.y)
                                   << std::endl;
        while (scheduler_timer <= simTime){
                scheduler_timer += 3;
                Simulator::Schedule(Seconds(scheduler_timer), &pos_tracking, position_tracking_wp, model);
        }
}
static void
init_wrappers(){
    /* create files for wrappers */
    debugger_wp = asciiTraceHelper.CreateFileStream(debugger);
    overall_wp = asciiTraceHelper.CreateFileStream(overall, std::ios::app);
    ue_positions_wp = asciiTraceHelper.CreateFileStream(course_change);
    position_tracking_wp = asciiTraceHelper.CreateFileStream(position_tracking);
    *ue_positions_wp->GetStream() << "========================\n";
    *position_tracking_wp->GetStream() << "========================\n";
    //********************Initialize wrappers*********************/
    if (isTcp==1){
      put_send = DIR + "tcp-put.dat";
      macro = DIR + "macro_tcp.dat";
    } else{
      put_send = DIR + "udp-put.dat";
      macro = DIR + "macro_udp.dat";
    }

    macro_wp = asciiTraceHelper.CreateFileStream(macro);
    put_send_wp = asciiTraceHelper.CreateFileStream(put_send);

    *put_send_wp->GetStream() << "#DestinationIp\t"
                  << "Time\t"
                  << "Send Tcp throughput\t"
                  << "Send Tcp delay\t"
                  << "Number of Lost Pkts\t"
                  << "Number of Tx Pkts\t"
                  << "ErrorUlTx\t"
                  << "ErrorDlTx\t"
                  << "HarqUlTx\t"
                  << "HarqDlTx\n";
    
}   


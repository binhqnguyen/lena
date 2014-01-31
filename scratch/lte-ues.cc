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
#include "ns3/lte-enb-net-device.h"
#include <math.h>

using namespace ns3;

#define kilo 1000
#define LONG_LIVED_UE "7.0.0.2"
#define COARSE_GRAIN_SAMPLING 40

double simTime = 100;   //simulation time for EACH application
static double ue_position_tracking_timer = 0; //timer to schedule position tracking


int isPedestrian = -1; //-1=no fading trace and no mobility, 0=vehicular trace, 1=pedestrian trace.
uint16_t traceTime = 100;       //trace file period, in seconds
std::string P_TRACE_FILE = "/home/binhn/ln/fading_traces/EPA_3kmh_100_dl.fad";
std::string V_TRACE_FILE = "/home/binhn/ln/fading_traces/EVA_60kmh_100_dl.fad";
std::string traceFile = P_TRACE_FILE;   //location of trace file.
uint16_t isFading = 1;

/***** Mobility parameters******/
uint16_t is_random_allocation = 0;  //UEs fixed position allocation by default
//UE grid allocation parameters.
double distance_ue_root_x = 50;
double distance_ue_root_y = 50;
double distance_among_ues_x = 50;
double distance_among_ues_y = 50;
//ENB grid allocation parameters.
double distance_among_enbs_x = 350;
double distance_among_enbs_y = 350;
//UE circle random allocation radius.
double distance_rho = 100;
//Moving
double moving_bound = 50000;
double VEH_VE = 60; //60km/h for vehicular
double PED_VE = 3; //3km/h for pedestrian
double moving_speed = 3; //UE moving speed in km/h.
double speed_mps = 20; //20m/s

/*******Simulation******/
uint16_t radioUlBandwidth = 25;  
uint16_t radioDlBandwidth = 25;  //same as above, for downlink.
std::string SACK="1";
std::string TIME_STAMP="0";
std::string WINDOW_SCALING="1";
std::string TCP_VERSION="cubic"; //reno,westwood,vegas,veno,yeah,illinois,htcp,hybla
std::string configure_input = "lte-ues.in";
std::string configure_output = "lte-ues.out";


/****Static node/nodecontainer/dev***/
static NodeContainer remoteHostContainer;
static Ptr<Node> remoteHost;
static NetDeviceContainer enbLteDevs;  //All eNB devices.
static Ptr<NetDevice> testingEnbDev;  //eNB that of the testing site.
static NetDeviceContainer neighborEnbDevs; //neighbor eNBs used for UE leaving (Hand over).
static Ipv4Address remoteHostAddr;
//Helpers
static Ipv4StaticRoutingHelper ipv4RoutingHelper;
static Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
static Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
static InternetStackHelper internet;
static NodeContainer enbNodes;
//static NodeContainer ueNodes;

/*HO related
uint32_t isAutoHo = 0;
*/
double X2_path_delay = 19; //X2 forwarding path delay in ms.
NodeContainer current_ue_nodes; //list that store the current UEs in the testing cell (ENB1).
NodeContainer gone_ue_nodes;  //Ues that handovered to the neighbor cell (ENB2).
uint32_t gone_ue_cnt = 0;


//MAC
std::string mac_scheduler = "ns3::RrFfMacScheduler";

//CORE
std::string s1uLinkDataRate = "1Gb/s";
double s1uLinkDelay = 0.015;
double s1uLinkMtu = 1500;
std::string p2pLinkDataRate = "1Gb/s";
double p2pLinkDelay = 0.015;
double p2pLinkMtu = 1500;

/****Application******/
uint32_t isTcp=1;
uint32_t packetSize = 900;
double samplingInterval = 0.005;    /*getTcp() function invoke for each x second*/
uint16_t PUT_SAMPLING_INTERVAL = 0; /*sample a TCP throughput for each x pkts*/
double put_sampling_timer = 0.0;    /*getTcpPut() invoked each period*/
double ue_joining_timer = 0.0;
Ptr<ns3::FlowMonitor> monitor;
FlowMonitorHelper flowHelper;
Ptr<ns3::Ipv4FlowClassifier> classifier;
std::map <FlowId, FlowMonitor::FlowStats> stats;
std::string dataRate = "150Mb/s";
uint16_t dlPort = 10000;
uint16_t ulPort = 20000;
LogLevel logLevel = (LogLevel) (LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC | LOG_LEVEL_DEBUG);

//Routing
Ptr<Ipv4StaticRouting> remoteHostStaticRouting;
Ptr<Ipv4StaticRouting> ueStaticRouting;

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
std::map<uint32_t, uint32_t> source_destination_port;

const double ONEBIL = 1000000000;

uint16_t numberOfUes = 3;
uint16_t numberOfEnbs = 2;
uint16_t numBearersPerUe = 1;

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

/** Random seeds set up **/
Ptr<ParetoRandomVariable> flow_per_ue_pareto = CreateObject<ParetoRandomVariable>();
double flow_per_ue_pa_mean = 1.5;
double flow_per_ue_pa_shape = 3;

Ptr<ExponentialRandomVariable> flow_duration_B_exp  = CreateObject<ExponentialRandomVariable> ();
double flow_duration_B_exp_mean = 8000; //mean 8KB
double flow_duration_B_exp_bound = 0;

Ptr<ParetoRandomVariable> ontime_s_pareto  = CreateObject<ParetoRandomVariable> ();
double ontime_s_pa_mean = 2;
double ontime_s_pa_shape = 0.5; 


Ptr<ParetoRandomVariable> thinktime_s_pareto  = CreateObject<ParetoRandomVariable> ();
double thinktime_s_pa_mean = 20;
double thinktime_s_pa_shape = 0.5;

Ptr<UniformRandomVariable> uniform = CreateObject<UniformRandomVariable>();
double uniform_min = 0.0;
double uniform_max = 10.0;


void SetUpRandomSeed();
/*********/


void getTcpPut();
void get_average_result();
void EnableLogComponents();
void SetDefaultConfigs();
void CommandlineParameters (int argc, char* argv[]);
void InstallMobilityUe (NodeContainer ueNodes, uint16_t is_pedestrian);
void InstallLocationUe(NodeContainer ueNodes, uint32_t distance_rho);
void InstallLocationUe(NodeContainer ueNodes, double distance_ue_root_x, double distance_ue_root_y, double distance_among_ues_x, double distance_among_ues_y);
void InstallMobilityEnb(NodeContainer enbNodes, double distance_among_enbs_x, double distance_among_enbs_y);
void InstallFading(Ptr<LteHelper> lteHelper);
void EnablePositionTrackingEnb(NetDeviceContainer enbLteDevs);
void EnablePositionTrackingUes(NodeContainer ueNodes);
void init_wrappers();
void pos_tracking (Ptr<OutputStreamWrapper> position_tracking_wp, Ptr<const MobilityModel> model, uint32_t ue_id);
void CourseChange (Ptr<OutputStreamWrapper> ue_positions_wp, uint32_t ue_id, Ptr<const MobilityModel> model);
void ConfigStoreOutput(std::string out_f);
void ConfigStoreInput(std::string in_f);
void set_up_enbs();
void set_up_remote_host_and_sgw_and_p2p_link();
void enable_lte_traces();
void hook_ho_callbacks();

//******Handover monitoring********//
Ptr<OutputStreamWrapper> ho_wp = asciiTraceHelper.CreateFileStream(DIR+"handover.dat");
void NotifyConnectionEstablishedUe (std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti);
void NotifyHandoverStartUe (std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti, uint16_t targetCellId);
void NotifyHandoverEndOkUe (std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti);
void NotifyConnectionEstablishedEnb (std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti);
void NotifyHandoverStartEnb (std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti, uint16_t targetCellId);
void NotifyHandoverEndOkEnb (std::string context, uint64_t imsi, uint16_t cellid, uint16_t rnti);

/**** UE leaving or joining ****/
void UeJoining(uint32_t number_of_ues, Ptr<NetDevice> joining_enb_dev);
void UeLeaving(uint32_t number_of_ues, Node target_enb);
void InstallWorkload (NodeContainer ueNodes, Ptr<NetDevice> joining_enb_dev);

/**
 * Sample simulation script for a X2-based handover.
 * It instantiates two eNodeB, attaches one UE to the 'source' eNB and
 * triggers a handover of the UE towards the 'target' eNB.
 */
NS_LOG_COMPONENT_DEFINE ("EpcX2HandoverExample");
int
main (int argc, char *argv[])
{
SetUpRandomSeed();
// change some default attributes so that they are reasonable for
// this scenario, but do this before processing command line
// arguments, so that the user is allowed to override these settings 
SetDefaultConfigs();
ConfigStoreInput(configure_input);


// Command line arguments
CommandlineParameters(argc, argv);
init_wrappers();
EnableLogComponents();

/**
  * Create a remote host.
  * Set up link and routing between SPGW and the remote host.
  */
set_up_remote_host_and_sgw_and_p2p_link();

//*********** ENBs settings *********//
set_up_enbs();


/**** UE level actions here *****/

/**********UE**********
  *Create UE nodes
  *Allocate routing
  *Install mobility
  *Attach UEs to the specified eNB
  */
UeJoining(1, testingEnbDev);	//
UeJoining(numberOfUes, testingEnbDev);

/** Manual HOs used for joining and leaving UEs**/
//*debugger_wp->GetStream () << "Manual Handover at 20,50,80 second\n" ;
//lteHelper->HandoverRequest (Seconds (10.00), ueLteDevs.Get (0), enbLteDevs.Get (0), enbLteDevs.Get (1));
//lteHelper->HandoverRequest (Seconds (20.00), ueLteDevs.Get (0), enbLteDevs.Get (1), enbLteDevs.Get (0));
/** end HOs ***/

monitor = flowHelper.Install(remoteHost);
monitor = flowHelper.GetMonitor();  


enable_lte_traces();

hook_ho_callbacks();

/**Schedule the first get_tcp_put invoke*/
Simulator::ScheduleWithContext (0 ,Seconds (0.0), &getTcpPut);

ConfigStoreOutput(configure_output);

Simulator::Stop(Seconds(simTime));
Simulator::Run();

//Final average result summary
get_average_result();

// GtkConfigStore config;
// config.ConfigureAttributes();

Simulator::Destroy();
return 0;

}

void
set_up_enbs(){
  enbNodes.Create(numberOfEnbs);
  InstallMobilityEnb(enbNodes, distance_among_enbs_x, distance_among_enbs_y); 
  enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  testingEnbDev = enbLteDevs.Get(0);
  for (uint16_t i = 1; i < enbLteDevs.GetN(); ++i){
    neighborEnbDevs.Add(enbLteDevs.Get(i));
  }
  EnablePositionTrackingEnb(enbLteDevs); 
  //Install Fading Model
  InstallFading(lteHelper);
  //Add X2 inteface
  lteHelper->AddX2Interface (enbNodes);
}

void
set_up_remote_host_and_sgw_and_p2p_link(){
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType(mac_scheduler);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();
  epcHelper->SetAttribute("S1uLinkDataRate", DataRateValue (DataRate (s1uLinkDataRate)));
  epcHelper->SetAttribute("S1uLinkDelay", TimeValue (Seconds (s1uLinkDelay)));
  epcHelper->SetAttribute("S1uLinkMtu", UintegerValue (s1uLinkMtu));

  // Create a single RemoteHost
  remoteHostContainer.Create (1);
  remoteHost = remoteHostContainer.Get (0);
  internet.SetTcp("ns3::NscTcpL4Protocol", "Library", StringValue("liblinux2.6.26.so"));
  internet.Install (remoteHost);
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_sack", StringValue (SACK));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_timestamps", StringValue (TIME_STAMP));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_window_scaling", StringValue (WINDOW_SCALING));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue (TCP_VERSION));
  //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.core.wmem_max", StringValue ("8338608"));
  //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.core.rmem_max", StringValue ("8338608"));
  //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_rmem", StringValue ("4096 8000000 8338608"));
  //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_wmem", StringValue ("4096 5000000 8338608"));
  //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_rmem", StringValue ("4096 87380 8338608"));

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (p2pLinkDataRate)));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (p2pLinkMtu));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (p2pLinkDelay)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  remoteHostAddr = internetIpIfaces.GetAddress (1);
  // Routing of the Internet Host (towards the LTE network)
  remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  // interface 0 is localhost, 1 is the p2p device
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
}

void
hook_ho_callbacks(){
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
}

void 
enable_lte_traces(){
  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  lteHelper->EnablePdcpTraces ();
  Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
  rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (0.01)));
  Ptr<RadioBearerStatsCalculator> pdcpStats = lteHelper->GetPdcpStats ();
  pdcpStats->SetAttribute ("EpochDuration", TimeValue (Seconds (0.01)));
}
void 
get_average_result(){
  monitor->CheckForLostPackets();
  Ptr<ns3::Ipv4FlowClassifier> classifier = DynamicCast<ns3::Ipv4FlowClassifier> (flowHelper.GetClassifier());
  std::map <FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter){
    ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
    *macro_wp->GetStream()  << "***Flow ID: " << iter->first 
          << " Src Addr " << t.sourceAddress << ":" << t.sourcePort 
          << " Dst Addr " << t.destinationAddress << ":" << t.destinationPort 
          << "\nTx Packets " << iter->second.txPackets
          << "\nRx Packets " << iter->second.rxPackets
          << "\nLost packets " << iter->second.lostPackets
          << "\nLost ratio " << double (iter->second.lostPackets)/(iter->second.lostPackets+iter->second.rxPackets) << std::endl;
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
}

void
InstallWorkload (NodeContainer ueNodes, Ptr<NetDevice> joining_enb_dev){
	// Install LTE Devices on UEs
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
  EnablePositionTrackingUes(ueNodes);

  // Install the IP stack on the UEs
  internet.SetTcp("ns3::NscTcpL4Protocol", "Library", StringValue("liblinux2.6.26.so"));
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
	
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_sack", StringValue (SACK));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_timestamps", StringValue (TIME_STAMP));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_window_scaling", StringValue (WINDOW_SCALING));
 	Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue (TCP_VERSION)); 
	//Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.core.wmem_max", StringValue ("8338608"));
  //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.core.rmem_max", StringValue ("8338608"));
  //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_rmem", StringValue ("4096 8000000 8338608"));
  //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_wmem", StringValue ("4096 5000000 8338608"));
 
	*debugger_wp->GetStream() << "xxx\n";
  //Attach all Ues to the specified eNB.
  //This attach process is MANUAL and not going through the 
  //proper cell selection but jump to CONNECTED state and create a default BEARER immediately.
  lteHelper->Attach(NetDeviceContainer(ueLteDevs), joining_enb_dev);

  //Randomize the starting time of clientApps and serverApps.
  Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable> ();
  startTimeSeconds->SetAttribute ("Min", DoubleValue (0.5));
  startTimeSeconds->SetAttribute ("Max", DoubleValue (0.6));

  *debugger_wp->GetStream() << "Workload for UEs:\n"
      << "-------------------\n";
  /**UE workload**/
	uint16_t number_of_bearers_per_ue = uint16_t (ceil(flow_per_ue_pareto->GetValue()));
	uint32_t flow_duration_B = 0;
	//Workload:
	//1. number of beaer per ue.
	//2. short flow duration in KB (pareto, mean=9KB, shape=0.5).
	//3. long flow duration in KB (pareto, mean=1MB, shape=0.5).
	//4. percentage between short/long flow (0.6% long).
	//5. flow on time in second (2s).??? SPDY
	//6. think time (flow off time) in second (2s).??? SPDY
  for (uint32_t u = 0; u < ueNodes.GetN(); ++u){
		number_of_bearers_per_ue = flow_per_ue_pareto->GetValue();
    Ptr<Node> ue = ueNodes.Get (u);
    // Set the default gateway for the UE
    ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ue->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
		*debugger_wp->GetStream() << "UE IP " << ue->GetObject<Ipv4>() 
														<< " Flows per UE: " << number_of_bearers_per_ue << std::endl;
    for (uint32_t b = 0; b < number_of_bearers_per_ue; ++b){
      ++dlPort;
      ++ulPort;
			
      ApplicationContainer clientApps;
      ApplicationContainer serverApps;

      if (isTcp == 1){
        LogComponentEnable("Queue",logLevel);    //Only enable Queue monitoring for TCP to accelerate experiment speed.
        PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        serverApps.Add(sink.Install(ueNodes.Get(u)));

				//Generate a random value for flow duration.
				flow_duration_B = uint32_t (flow_duration_B_exp->GetValue());

				*debugger_wp->GetStream() << "\n Flow duration (B) " << flow_duration_B << std::endl;

        OnOffHelper onOffHelper("ns3::TcpSocketFactory", Address ( InetSocketAddress(ueIpIfaces.GetAddress(u) , dlPort) ));
				//onOffHelper.SetAttribute("MaxBytes", UintegerValue(300000));
				onOffHelper.SetAttribute("OnTime", StringValue("ns3::ParetoRandomVariable[Mean=2,Shape=0.5]"));
				onOffHelper.SetAttribute("OffTime", StringValue("ns3::ParetoRandomVariable[Mean=30,Shape=0.5]"));
        onOffHelper.SetConstantRate( DataRate(dataRate), packetSize );
        clientApps.Add(onOffHelper.Install(remoteHost));

				/*
				PacketSinkHelper ul_sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
      	serverApps.Add(ul_sink.Install(remoteHost));

        OnOffHelper ul_onOffHelper("ns3::TcpSocketFactory", Address ( InetSocketAddress(remoteHostAddr, ulPort) ));
        ul_onOffHelper.SetConstantRate( DataRate(dataRate), packetSize );
        clientApps.Add(ul_onOffHelper.Install(ueNodes.Get(u)));
				*/
      }
      else{
        PUT_SAMPLING_INTERVAL = PUT_SAMPLING_INTERVAL*20;
        PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        serverApps.Add(sink.Install(ueNodes.Get(u)));

        OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address ( InetSocketAddress(ueIpIfaces.GetAddress(u), dlPort) ));
        onOffHelper.SetConstantRate( DataRate(dataRate), packetSize );
        clientApps.Add(onOffHelper.Install(remoteHost));

        PacketSinkHelper ul_sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
        serverApps.Add(ul_sink.Install(remoteHost));

        OnOffHelper ul_onOffHelper("ns3::UdpSocketFactory", Address ( InetSocketAddress(remoteHostAddr, ulPort) ));
        ul_onOffHelper.SetConstantRate( DataRate(dataRate), packetSize );
        clientApps.Add(ul_onOffHelper.Install(ueNodes.Get(u)));
      }
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
      *debugger_wp->GetStream() << "UE-ID # " << u
          << " dlport/ulport = " << dlPort << " , " << ulPort
          << " TCP = " << isTcp << std::endl;
    } // end for b
  }
	
	//for (NetDeviceContainer::Iterator devIt = ueLteDevs.Begin(); devIt != ueLteDevs.End(); ++devIt){
		//pcapHelper.GetFilenameFromDevice(DIR+"lte-ues",(*devIt),true);
		//*debugger_wp->GetStream() << "aaa" << (*devIt)->GetNode()->GetObject<Ipv4>();
		//Ptr<PcapHelperForDevice> pcapHelper;
		//pcapHelper->EnablePcapInternal(DIR+"UE-",(*devIt),1,1);
	//}

}
/**
  * Create and attach X Ues to a specific eNB.
  */
void
UeJoining(uint32_t number_of_ues, Ptr<NetDevice> joining_enb_dev){

  *debugger_wp->GetStream() << "UEJoining:\n----------" 
              << "\nNumber of joining Ues: " << number_of_ues
              << "\nJoining event is at (s): " << Simulator::Now().GetSeconds()
              //<< "\n# of current ues in testing cell: " << current_ues.size()
              << "\n# of ues left so far: " << gone_ue_cnt << std::endl;
	NodeContainer ueNodes;
  //ues.Create(number_of_ues);
  ueNodes.Create(number_of_ues);

  // Install Mobility Model
  InstallMobilityUe(ueNodes, isPedestrian); 
	InstallLocationUe(ueNodes, distance_rho);
	// Install workload for each UE 
	InstallWorkload (ueNodes, joining_enb_dev);	//TODO
	monitor = flowHelper.Install(ueNodes);
}

void
UeLeaving(uint32_t number_of_ues, LteEnbNetDevice target_enb){
//TODO:
//  - X UEs HO to the neighor eNB.
//  - Schedule for the next UE leaving event after y seconds.
}

void
SetUpRandomSeed(){
  /** Pareto Random **/
	//TODO:
	//1. number of beaer per ue.
	//2. short flow duration in KB (pareto, mean=9KB, shape=0.5).
	//3. long flow duration in KB (pareto, mean=1MB, shape=0.5).
	//4. percentage between short/long flow (0.6% long).
	//5. flow on time in second (2s).??? SPDY
	//6. think time (flow off time) in second (2s).??? SPDY
	//Flow per UE
 	flow_per_ue_pareto->SetAttribute("Mean", DoubleValue (flow_per_ue_pa_mean));
  flow_per_ue_pareto->SetAttribute("Shape", DoubleValue (flow_per_ue_pa_shape));
	
	//Flow duration in Bytes
	flow_duration_B_exp->SetAttribute("Mean", DoubleValue (flow_duration_B_exp_mean));
	flow_duration_B_exp->SetAttribute("Bound", DoubleValue (flow_duration_B_exp_bound));

	//On time
	ontime_s_pareto->SetAttribute("Mean", DoubleValue(ontime_s_pa_mean));
	ontime_s_pareto->SetAttribute("Shape", DoubleValue(ontime_s_pa_shape));

	//Think time
	thinktime_s_pareto->SetAttribute("Mean", DoubleValue(thinktime_s_pa_mean));
	thinktime_s_pareto->SetAttribute("Shape", DoubleValue(thinktime_s_pa_shape));

  /** Uniform Random **/
  uniform->SetAttribute("Min", DoubleValue(uniform_min));
  uniform->SetAttribute("Max", DoubleValue(uniform_max));
}

void
getTcpPut(){
  monitor->CheckForLostPackets();
  classifier = DynamicCast<ns3::Ipv4FlowClassifier> (flowHelper.GetClassifier());
  stats = monitor->GetFlowStats();

    /*==============Get flows information============*/
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter){
    ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
    NS_LOG_DEBUG ("getTcpPut destinationAddress = " << t.destinationAddress << " port = " << t.destinationPort << std::endl);
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

		source_destination_port[t.sourcePort] = t.destinationPort;

		//Adjust sampling rate based on destination IP. Only the long-lived UE is measured using COARSE_GRAIN_SAMPLING. 
		PUT_SAMPLING_INTERVAL = (t.destinationAddress == LONG_LIVED_UE)? COARSE_GRAIN_SAMPLING : 0;
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
	std::map<Ipv4Address,double>::iterator it7 = last_rx_bytes.begin();
	std::map<Ipv4Address,double>::iterator it8 = last_tx_bytes.begin();
	std::map<uint32_t,uint32_t>::iterator it9 = source_destination_port.begin();
  for (;it1 != meanRxRate_send.end(); ){
    *put_send_wp->GetStream() << Simulator::Now().GetSeconds() << "\t\t"
        << (*it1).first << "\t\t"
        << (*it1).second << "\t\t"
        << (*it7).second << "\t\t"
        << (*it3).second << "\t\t"
        << (*it4).second << "\t\t"
        << (*it9).second << "\t\t"
        << "x" << "\t\t"
        << "x" << "\t\t"
        << (*it8).second << "\t"
        << (*it5).second << "\t"
        << (*it6).second <<  "\n";
    ++it1;
    ++it3;
    ++it4;
    ++it5;
    ++it6;
		++it7;
		++it8;
		++it9;
  }

  while (put_sampling_timer < simTime){
      put_sampling_timer += samplingInterval;
      Simulator::Schedule(Seconds(put_sampling_timer),&getTcpPut);
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
  Config::SetDefault("ns3::ConfigStore::Filename", StringValue(in_f));
  Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("RawText"));
  Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Load"));
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();
  inputConfig.ConfigureAttributes();
}

void EnableLogComponents(){
	if (isTcp==1){
		if (isDebug==0){
			//LogLevel logLevel = (LogLevel) (LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC);
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
		LogComponentEnable ("NscTcpL4Protocol", (LogLevel) (LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC));
		LogComponentEnable ("Ipv4", logLevel);
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

  if (isFading==1)
  	Config::SetDefault("ns3::LteHelper::FadingModel", StringValue("ns3::TraceFadingLossModel"));
}
void CommandlineParameters(int argc, char* argv[]){
  CommandLine cmd;
  cmd.AddValue("numberOfUes", "Number of UeNodes", numberOfUes);
  cmd.AddValue("numberOfEnbs", "Number of eNodebs", numberOfEnbs);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance_among_ues_x", "Distance among ues x[m]", distance_among_ues_x);
  cmd.AddValue("distance_among_ues_y", "Distance among ues y [m]", distance_among_ues_y);
  cmd.AddValue("distance_ue_root_x", "Distance among ues x[m]", distance_ue_root_x);
  cmd.AddValue("distance_ue_root_y", "Distance among ues x[m]", distance_ue_root_y);
  cmd.AddValue("packetSize", "Size of each packet", packetSize);
  cmd.AddValue("s1uLinkDataRate", "S1u Link Data Rate", s1uLinkDataRate);
  cmd.AddValue("s1uLinkDelay", "S1u Link Delay", s1uLinkDelay);
  cmd.AddValue("s1uLinkMtu", "S1u Link Mtu", s1uLinkMtu);
  cmd.AddValue("p2pLinkDataRate", "p2p Link Data Rate", p2pLinkDataRate);
  cmd.AddValue("p2pLinkDelay", "p2p Link Delay", p2pLinkDelay);
  cmd.AddValue("p2pLinkMtu", "p2p Link Mtu", p2pLinkMtu);
  cmd.AddValue("radioUlBandwidth", "Uplink radio bandwidth [RBs] (6,15,25,50,75,100)", radioUlBandwidth);
  cmd.AddValue("radioDlBandwidth", "Downlink radio bandwidth [RBs] (6,15,25,50,75,100)", radioDlBandwidth);
  cmd.AddValue("isFading", "Whether enabling trace fading", isFading);
  cmd.AddValue("dataRate", "TCP application data rate", dataRate);
  cmd.AddValue("isTcp", "TCP application if true, Udp if false", isTcp);
  cmd.AddValue("isPedestrian", "Whether using pedestrian fading trace and mobility (-1 for no fading/mobility, 0 for vehicular, 1 for pedestrian", isPedestrian);
  cmd.AddValue("SACK", "TCP SACK", SACK);
  cmd.AddValue("TIME_STAMP", "TCP TIME_STAMP", TIME_STAMP);
  cmd.AddValue("WINDOW_SCALING", "TCP WINDOW_SCALING", WINDOW_SCALING);
  cmd.AddValue("isDebug", "Whether in debug mode (turn on/off log)", isDebug);
  cmd.Parse(argc, argv);
}

void InstallMobilityEnb(NodeContainer enbNodes, double distance_among_enbs_x, double distance_among_enbs_y){
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                  "MinX", DoubleValue (0.0),  //zero point
                  "MinY", DoubleValue (0.0),  //zero point
                  "DeltaX", DoubleValue (distance_among_enbs_x),  //distance among ENB nodes
                  "DeltaY", DoubleValue (distance_among_enbs_y),
                  "GridWidth", UintegerValue (10), //number of nodes on a line
                  "LayoutType", StringValue ("RowFirst"));
  enbMobility.Install (enbNodes); //===ENB #1 placed at (0.0)====//
  *debugger_wp->GetStream() << "ENBs allocation:\n-----------\n"
                            << "Grid position, root <0,0>, "
                            << "distance_among_enbs <x,y> = < " 
                            << distance_among_enbs_x << " , " << distance_among_enbs_y << " >\n";
}

void InstallLocationUe(NodeContainer ueNodes, uint32_t distance_rho){
  	MobilityHelper ueMobility;
  	*debugger_wp->GetStream() << "UEs allocation:\n------------\n";
    ueMobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", //nodes are put randomly inside a circle with the central point is (x,y).
    "X", DoubleValue (0),
    "Y", DoubleValue (0),
    "rho", DoubleValue (distance_rho));  //radius of the circle.
		ueMobility.Install(ueNodes);
    *debugger_wp->GetStream() << "Random position allocation: circle center = <0,0>. "
        << "circle radius = " << distance_rho << std::endl;
}
void InstallLocationUe(NodeContainer ueNodes, double distance_ue_root_x, double distance_ue_root_y, double distance_among_ues_x, double distance_among_ues_y){
	MobilityHelper ueMobility;
  *debugger_wp->GetStream() << "UEs allocation:\n------------\n";
  ueMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
        "MinX", DoubleValue (distance_ue_root_x),  //1st UE is put at a location which is "distance" meters away from eNB.
        "MinY", DoubleValue (distance_ue_root_y),  
        "DeltaX", DoubleValue (distance_among_ues_x),  //distance among UE nodes
        "DeltaY", DoubleValue (distance_among_ues_y),
        "GridWidth", UintegerValue (5), //number of nodes on a line
        "LayoutType", StringValue ("RowFirst"));
	ueMobility.Install(ueNodes);
  *debugger_wp->GetStream() << "Fixed grid allocation: root = < "
        << distance_ue_root_x << " , " << distance_ue_root_y << " >. "
        << "Among ues <distance_x, distance_y> = < " 
        << distance_among_ues_x << " , " << distance_among_ues_y << " >.\n";
}
void InstallMobilityUe(NodeContainer ueNodes, uint16_t is_pedestrian){
	MobilityHelper ueMobility;
  *debugger_wp->GetStream() << "UEs Mobility:\n------------\n";
	if (!is_pedestrian) 
		moving_speed = VEH_VE;     //vehicular mobility
	else moving_speed = PED_VE;
	double speed_mps = double (moving_speed)*1000/3600; //kmph to meter per second.
	std::stringstream mss;
	mss << speed_mps;
	std::string ms = mss.str();
	ueMobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
			"Mode", StringValue ("Time"),  //change distance and speed based on TIME.
			"Time", StringValue ("200s"), //change direction and speed after each 2s.
			"Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),  //m/s
			"Speed", StringValue ("ns3::ConstantRandomVariable[Constant="+ms+"]"),  //m/s
			"Bounds", RectangleValue (Rectangle (-moving_bound, moving_bound, -moving_bound, moving_bound)));  //bound
	ueMobility.Install(ueNodes);
	*debugger_wp->GetStream() << "UEs mobility: RandomWalk2dMobilityModel."
			<< " Speed = " << speed_mps
			<< " Moving bound = square " << moving_bound << std::endl;
}

void InstallFading(Ptr<LteHelper> lteHelper){
  if(isPedestrian == 1 && isFading == 1)
    traceFile = P_TRACE_FILE;
  else if (isPedestrian==0 && isFading == 1) 
    traceFile = V_TRACE_FILE;

  if (isPedestrian >= 0 && isFading == 1){ //enable trace fading if isPedestrian=1 or vehicular (isPedestrian=0)
    Config::SetDefault("ns3::LteHelper::FadingModel", StringValue("ns3::TraceFadingLossModel"));
    lteHelper->SetFadingModel("ns3::TraceFadingLossModel");
    lteHelper->SetFadingModelAttribute("TraceLength",TimeValue(Seconds(traceTime)));
    lteHelper->SetFadingModelAttribute("SamplesNum",UintegerValue(traceTime*1000));  /*1sample/1ms*/
    lteHelper->SetFadingModelAttribute("WindowSize",TimeValue(Seconds(0.5)));
    lteHelper->SetFadingModelAttribute("RbNum",UintegerValue(radioDlBandwidth));
    lteHelper->SetFadingModelAttribute("TraceFilename", StringValue(traceFile));
    *debugger_wp->GetStream() << "Trace fading ENABLED:\n"
        << "--------------------"
        << "\nPesdestrian = " << isPedestrian
        << "\nTrace time = " << traceTime
        << "\nTrace file= " << traceFile << std::endl;
  }
  else *debugger_wp->GetStream() << "Trace fading DISABLED.\n";
}

void EnablePositionTrackingEnb(NetDeviceContainer enbLteDevs){
  NetDeviceContainer::Iterator enbLteDevIt = enbLteDevs.Begin ();
  uint16_t i = 0;
  *debugger_wp->GetStream() << "ENBs postion:\n"
      << "------------\n";
  for (NetDeviceContainer::Iterator enbLteDevIt = enbLteDevs.Begin(); enbLteDevIt != enbLteDevs.End(); enbLteDevIt++){
  	Vector enbPosition = (*enbLteDevIt)->GetNode ()->GetObject<MobilityModel> ()->GetPosition ();
    *debugger_wp->GetStream() << "eNB " << i << ": (x,y) = " 
        << enbPosition.x << ", " << enbPosition.y << std::endl;
    i++;
	}
}

void EnablePositionTrackingUes(NodeContainer ueNodes){
  *debugger_wp->GetStream() << "UEs position:\n"
      << "--------------\n";
  for (uint32_t i = 0; i < ueNodes.GetN(); i++ ){
    Ptr<MobilityModel> ue_mobility_model = ueNodes.Get(i)->GetObject<MobilityModel>();
    double x = ue_mobility_model->GetPosition().x;
    double y = ue_mobility_model->GetPosition().y;
    *debugger_wp->GetStream() << "UE-ID = " << i << std::endl
        << "(x,y)= " << x << ", " << y 
        << " distance to root (0,0) = " << sqrt(x*x+y*y) << std::endl;
    //Coonect to CourseChange call back for position updating.
    ue_mobility_model->TraceConnectWithoutContext("CourseChange", MakeBoundCallback(&CourseChange, ue_positions_wp, i));
    //Tracking UE location periodically, start from 1s.
    Simulator::Schedule(Seconds(1), &pos_tracking, position_tracking_wp, ue_mobility_model, i);
    ue_position_tracking_timer = 1;
  }
}

void
CourseChange (Ptr<OutputStreamWrapper> ue_positions_wp, uint32_t ue_id, Ptr<const MobilityModel> model){
  Vector position = model->GetPosition();
  Vector vel = model->GetVelocity();
  *ue_positions_wp->GetStream() << Simulator::Now().GetSeconds()
      << "UE-ID " << ue_id
      << " (x,y) = " << position.x << " , " << position.y
      << " distance to root (0,0) = " << sqrt(position.x*position.x+position.y*position.y) 
      << " velocity = " << sqrt (vel.x*vel.x + vel.y*vel.y) << std::endl;
}


void
pos_tracking (Ptr<OutputStreamWrapper> position_tracking_wp, Ptr<const MobilityModel> model, uint32_t ue_id){
  Vector position = model->GetPosition();
  Vector vel = model->GetVelocity();
  *position_tracking_wp->GetStream() << Simulator::Now().GetSeconds() 
        << "UE-ID = " << ue_id 
        << " (x,y) = " << position.x << " , " << position.y
        << " distance to root (0,0) = " << sqrt(position.x*position.x+position.y*position.y)
        << " velocity = " << sqrt (vel.x*vel.x + vel.y*vel.y) << std::endl;
  while (ue_position_tracking_timer <= simTime){
    ue_position_tracking_timer += 3;
    Simulator::Schedule(Seconds(ue_position_tracking_timer), &pos_tracking, position_tracking_wp, model, ue_id);
  }
}
void
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

  *put_send_wp->GetStream() << "Time\t"
                << "Destination IP\t"
                << "Tcp Goodput\t"
                << "Received bytes\t"
                << "Number of Lost Pkts\t"
                << "Number of Tx Pkts\t"
                << "x\t"
                << "x\t"
                << "x\t"
                << "Transmitted bytes\t"
								<< "Tcp throughput\t"
								<< "Tcp delay\n";
}  



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


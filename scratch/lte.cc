

/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 The university of Utah
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
 * Author: Binh Nguyen <binh@cs.utah.edu>
 */


/**
 * NOTE: This code doesn't support MULTIPLE ENODEBs yet (problem might lay on the mobility model, co-location in position of EnodeBs).
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
//#include "ns3/gtk-config-store.h"

#include <map>

using namespace ns3;
/**
 * Theory Topology:                Ues (x numberOfNodes) ---------- EnodeB ---------- |
 *																																	|
 * 							 Ues (x numberOfNodes) ---------- EnodeB ----------	|
 * 							 																										|
 * 							 Ues (x numberOfNodes) ---------- EnodeB ----------	|	SPGW -------------------------- RH
 *
 *                              										n2                 n0                       n1
 */
 /*
  * Experiment topology:
  *     
  *             UE (n3) -------------------- ENB (n2) ==================== SPGW (n0) ==================== End-host (n1)
  *                                                 2:2                  0:3        0:2                 1:1
  *                             radio                  <1Gbps,15ms,1500>               <1Gbps,15ms,1500>
  */
/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */
NS_LOG_COMPONENT_DEFINE ("lte-single-ue");

#define kilo 1000



double simTime = 100;	//simulation time for EACH application
static double scheduler_timer=0; //timer to schedule position tracking
std::ofstream tcpThroughput;
Ptr<ns3::FlowMonitor> monitor;
FlowMonitorHelper flowHelper;
double samplingInterval = 0.005;    /*getTcp() function invoke for each x second*/
uint16_t PUT_SAMPLING_INTERVAL = 50; /*sample a TCP throughput for each x pkts*/
double t = 0.0;
uint16_t isTcp = 1;
//topology
uint16_t numberOfUeNodes = 1;
uint16_t numberOfEnodebs = 1;

//S1uLink (an in-depth paper 50%)
std::string s1uLinkDataRate = "1Gb/s";
double  s1uLinkDelay = 0.015;
uint16_t s1uLinkMtu = 1500;

//p2pLink
std::string p2pLinkDataRate = "1Gb/s";
double p2pLinkDelay = 0.015;
uint16_t p2pLinkMtu = 1500;

//Simulation
uint32_t numberOfPackets = 0;
uint32_t packetSize = 900;
double distance = 1000.0;    //With enbTxPower=5, Noise=37 and UeTxPower=50 (NEED TO BE THAT HIGH TO GUARANTEE UPLINK FOR TCP ACK FLOW), noise=9, we have roughly 1000Kb/s downlink bandwidth.
uint16_t radioUlBandwidth = 25;  //the radio link bandwidth among UEs and EnodeB (in Resource Blocks). This is the configuration on LteEnbDevice.
uint16_t radioDlBandwidth = 25;  //same as above, for downlink.
std::string dataRate = "100Mb/s";
std::string SACK="1";
std::string TIME_STAMP="0";
std::string WINDOW_SCALING="1";
//std::string TCP_RECEIVE_BUFFER="1000000 1000000 1000000"; //min default max
std::string  TCP_RECEIVE_BUFFER="500000"; //min default max
std::string TCP_VERSION="cubic"; //reno,westwood,vegas,veno,yeah,illinois,htcp,hybla
uint16_t isAMRLC = 0;    
uint16_t isCost231 = 0;    
double moving_bound = 50000;


//tracefading
int isPedestrian = -1; //-1=no fading trace and no mobility, 0=vehicular trace, 1=pedestrian trace.
uint16_t traceTime = 100;	//trace file period, in seconds
std::string P_TRACE_FILE = "~/ln/fading_traces/EPA_3kmh_100_dl.fad";
std::string V_TRACE_FILE = "~/ln/fading_traces/EVA_60kmh_100_dl.fad";
std::string traceFile = P_TRACE_FILE;	//location of trace file.
uint16_t isFading = 0;


//Mobility
double moving_speed = 3; //UE moving speed in km/h (3km/h for pedestrian, 60km/h for vehicular)


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
Ptr<ns3::Ipv4FlowClassifier> classifier;
std::map <FlowId, FlowMonitor::FlowStats> stats;


const uint32_t ONEBIL = 1000000000;

/********* Ascii output files name *********/
static std::string DIR = "/var/tmp/ln_result/radio/";
static std::string macro = DIR+"macro_output.dat";
static std::string put_send;
static std::string debugger = "debugger.dat";
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

LogLevel level = (LogLevel) (LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC);
/**************** Functions **************/

static void getTcpPut();
static void init_wrappers();

/*****************NSC*********************/
static std::string nsc_stack="liblinux2.6.26.so";


static uint16_t is_random_allocation = 0;  //UEs fixed position allocation by default

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

void log_component_enable(){
	if (isTcp==1){
	//*************Enable logs********************/
    //To enable all components inside the LTE module.
//      lteHelper->EnableLogComponents();
    
    //	LogComponentEnable("UdpEchoClientApplication",LOG_LEVEL_INFO);
    //	LogComponentEnable("UdpEchoClientApplication",LOG_PREFIX_ALL);
    	// LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    	// LogComponentEnable("UdpClient",LOG_LEVEL_INFO);
    	 // LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
//		LogComponentEnable("OnOffApplication",LOG_LEVEL_INFO);
//		LogComponentEnable("PacketSink",LOG_LEVEL_INFO);
   // LogComponentEnable("TcpTahoe", level);
   // LogComponentEnable("RttEstimator",level);
   // LogComponentEnable("TcpSocketBase",level);
    LogComponentEnable ("LteRlcUm", level);
    LogComponentEnable ("LteRlcAm", level);
    LogComponentEnable ("NscTcpSocketImpl",LOG_LEVEL_DEBUG);
    		// LogComponentEnable("OnOffApplication",level);
//     		LogComponentEnable("PacketSink",level);
    //   LogComponentEnable ("LteHelper",level);
//       LogComponentEnable ("LteUeMac", level);
    //   LogComponentEnable ("LteEnbMac", level);
//       LogComponentEnable ("LtePdcp", level);
//       LogComponentEnable ("LtePhy", level);
	}
}

int
main (int argc, char *argv[])
{

   
    // Command line arguments
    CommandLine cmd;
    cmd.AddValue("numberOfUeNodes", "Number of UeNodes", numberOfUeNodes);
    cmd.AddValue("numberOfEnodebs", "Number of eNodebs", numberOfEnodebs);
    cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
    cmd.AddValue("distance", "Distance between eNBs [m]", distance);
    cmd.AddValue("numberOfPackets", "Number of packets to send", numberOfPackets);
    cmd.AddValue("packetSize", "Size of each packet", packetSize);
    cmd.AddValue("s1uLinkDataRate", "S1u Link Data Rate", s1uLinkDataRate);
    cmd.AddValue("s1uLinkDelay", "S1u Link Delay", s1uLinkDelay);
    cmd.AddValue("s1uLinkMtu", "S1u Link Mtu", s1uLinkMtu);
    cmd.AddValue("p2pLinkDataRate", "p2p Link Data Rate", p2pLinkDataRate);
    cmd.AddValue("p2pLinkDelay", "p2p Link Delay", p2pLinkDelay);
    cmd.AddValue("p2pLinkMtu", "p2p Link Mtu", p2pLinkMtu);
    cmd.AddValue("radioUlBandwidth", "Uplink radio bandwidth [RBs] (6,15,25,50,75,100)", radioUlBandwidth);
    cmd.AddValue("radioDlBandwidth", "Downlink radio bandwidth [RBs] (6,15,25,50,75,100)", radioDlBandwidth);
    cmd.AddValue("isAMRLC", "Whether using AM RLC (UM RLC if false)", isAMRLC);
    cmd.AddValue("isFading", "Whether enabling trace fading", isFading);
    cmd.AddValue("dataRate", "TCP application data rate", dataRate);
    cmd.AddValue("isTcp", "TCP application if true, Udp if false", isTcp);
    cmd.AddValue("isPedestrian", "Whether using pedestrian fading trace and mobility (-1 for no fading/mobility, 0 for vehicular, 1 for pedestrian", isPedestrian);
    cmd.AddValue("SACK", "TCP SACK", SACK);
    cmd.AddValue("TIME_STAMP", "TCP TIME_STAMP", TIME_STAMP);
    cmd.AddValue("WINDOW_SCALING", "TCP WINDOW_SCALING", WINDOW_SCALING);

   


    /**ConfigStore setting*/
    Config::SetDefault("ns3::ConfigStore::Filename", StringValue("lte.in"));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("RawText"));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Load"));
    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();
    inputConfig.ConfigureAttributes();
    
    cmd.Parse(argc, argv);
    //*************************************************/
   
    init_wrappers();

    log_component_enable();
    //************lteHeper, epcHelper**************//
    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
    
    //***************propagation model settings*****//
    //Notice: For Cost231PropagationLossModel, 2 environments can be set: suburban (C=0) and downtown (C=3).
    //See src/propagation/model/Cost231PropagationLossModel.cc
    if (isCost231 == 1){
	    P_TRACE_FILE = "~/ln/fading_traces/EPA_3kmh_100_dl_1575earfcn.fad";
            V_TRACE_FILE = "~/ln/fading_traces/EVA_60kmh_100_dl_1575earfcn.fad";
	    lteHelper->SetPathlossModelAttribute("Frequency",DoubleValue(1.842e9)); //frequency of transmission.
	    lteHelper->SetPathlossModelAttribute("SSAntennaHeight",DoubleValue(1.7));	//mobile station antenna height.
    }
    //*********************Use epcHelper to get the PGW node********************//
    Ptr<Node> pgw = epcHelper->GetPgwNode ();
    epcHelper->SetAttribute("S1uLinkDataRate", DataRateValue (DataRate (s1uLinkDataRate)));
    epcHelper->SetAttribute("S1uLinkDelay", TimeValue (Seconds (s1uLinkDelay)));
    epcHelper->SetAttribute("S1uLinkMtu", UintegerValue (s1uLinkMtu));



    //***********Create a single RemoteHost, install the Internet stack on it*************//
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create (1);
    Ptr<Node> remoteHost = remoteHostContainer.Get (0);
    //Install Internet stack on the remoteHost.
    InternetStackHelper internet;
    internet.SetTcp("ns3::NscTcpL4Protocol", "Library", StringValue(nsc_stack));
    internet.Install (remoteHost);
    Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_sack", StringValue (SACK));
    Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_timestamps", StringValue (TIME_STAMP));
    Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_window_scaling", StringValue (WINDOW_SCALING));
    Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue (TCP_VERSION));
    //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_rmem", StringValue (TCP_RECEIVE_BUFFER));
    Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.core.rmem_max", StringValue("500000"));
    Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.core.wmem_max", StringValue("500000"));


    //***************Create and install a point to point connection between the SPGW and the remoteHost*****************//
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (p2pLinkDataRate)));
    p2ph.SetDeviceAttribute ("Mtu", UintegerValue (p2pLinkMtu));
    p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (p2pLinkDelay)));
    NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);		//The interfaces between the SPGW and remoteHost were saved in internetDevices.

    // Create the Internet
    Ipv4AddressHelper ipv4h;	//Ipv4AddressHelper is used to assign Ip Address for a typical node.
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);		//assign IP addresses in starting at "1.0.0.0" to the SPGW and remoteHost.
    // interface 0 is localhost, 1 is the p2p device
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);


    //***************************Let's the remoteHost know how to route to UE "7.0.0.0"**************************//
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    //get the static routing method to the remoteHost. The parameter for GetStaticRouting() is the Ptr<Ipv4> of the destination.
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());	//remoteHostStaticRouting now knows how to route to the remoteHost.
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);	//Add the routing entry to the remoteHostStaticRouting table.
    //"1" means interface #1 of the remoteHost will route to "7.0.0.0/24" (which is default Ipv4 range for UEs??)

    //**********************************Create Ue nodes, EnodeBs*******************************//
    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numberOfEnodebs);
    ueNodes.Create(numberOfUeNodes);


    //=============================eNB allocation=================//
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
		    "MinX", DoubleValue (0.0),  //zero point
		    "MinY", DoubleValue (0.0),  //zero point
		    "DeltaX", DoubleValue (10000.0),  //distance among ENB nodes
		    "DeltaY", DoubleValue (10000.0),
		    "GridWidth", UintegerValue (3), //number of nodes on a line
		    "LayoutType", StringValue ("RowFirst"));
    enbMobility.Install (enbNodes); /*===ENB #1 placed at (0.0)====*/
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
    NetDeviceContainer::Iterator enbLteDevIt = enbLteDevs.Begin ();
    Vector enbPosition = (*enbLteDevIt)->GetNode ()->GetObject<MobilityModel> ()->GetPosition ();

    //=========================UE allocation=======================//
    MobilityHelper ueMobility;
    if (is_random_allocation == 0){ //fixed position allocation
	*debugger_wp->GetStream() << "Allocating UEs with FIXED positions ....\n";
	ueMobility.SetPositionAllocator ("ns3::GridPositionAllocator",
		    "MinX", DoubleValue (distance/sqrt(2)),  //1st UE is put at a location which is "distance" meters away from eNB.
		    "MinY", DoubleValue (distance/sqrt(2)),  
		    "DeltaX", DoubleValue (100.0),  //distance among UE nodes
		    "DeltaY", DoubleValue (100.0),
		    "GridWidth", UintegerValue (3), //number of nodes on a line
		    "LayoutType", StringValue ("RowFirst"));

    }
    else{ 	//random distributed allocation
	    *debugger_wp->GetStream() << "Allocating UEs with RANDOM DISTRIBUTED positions ....\n ";
	    ueMobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", //nodes are put randomly inside a circle with the central point is (x,y).
						     "X", DoubleValue (enbPosition.x),
						     "Y", DoubleValue (enbPosition.y),
						     "rho", DoubleValue (distance));  //radius of the circle.
	}

    //===========================UEs mobility=====================//
    if (isPedestrian == -1){ //constant position
    	ueMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	*debugger_wp->GetStream() << "NO MOBILITY (ConstantPositionMobilityModel) .... \n";
    }
    else {	//randomwalking enabled
	    //*************UE random walking mobility*************//
	    if (isPedestrian==0) moving_speed = 50;	//vehicular mobility
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
    
        //Install mobility model into UEs. 
    ueMobility.Install (ueNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);   
    //Enable UE position and velocity traking
    Ptr<MobilityModel> ue_mobility_model = ueNodes.Get(0)->GetObject<MobilityModel>();
    double x = ue_mobility_model->GetPosition().x;
    double y = ue_mobility_model->GetPosition().y;
    *debugger_wp->GetStream() << "eNB(x,y)= " << enbPosition.x << ", " << enbPosition.y << std::endl;
    *debugger_wp->GetStream() << "UE (x,y)= " << x << ", " << y << " d= " << sqrt(x*x+y*y) << std::endl;

    ue_mobility_model->TraceConnectWithoutContext("CourseChange", MakeBoundCallback(&CourseChange, ue_positions_wp));
    

    //========================trace fading setup===================//
    if(isPedestrian==1 && isFading ==1)
	traceFile = P_TRACE_FILE;
    else if (isPedestrian==0 && isFading == 1) traceFile = V_TRACE_FILE;

    if (isPedestrian >= 0 && isFading == 1){ //enable trace fading if isPedestrian=1 or vehicular (isPedestrian=0)
      *debugger_wp->GetStream() << "Trace fading enabled....\n";
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
    }else *debugger_wp->GetStream() << "Trace fading disabled....\n";
                                
                                                                                               
  //**********************Assign Ipv4 addresses for UEs. Install the IP stack on the UEs******************//
    internet.Install (ueNodes);	//internet (InternetStackHelper) again be used to install an Internet stack for a node.

    // Assign IP address to UEs, and install applications
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));

    for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
        Ptr<Node> ueNode = ueNodes.Get (u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }


    //**********************Attach all UEs to eNodeB**********************//
    for (uint16_t i = 0; i < numberOfEnodebs; i++)
    {
        for (uint16_t t = 0; t < numberOfUeNodes; ++t){
            lteHelper->Attach (ueLteDevs.Get(t), enbLteDevs.Get(i));    //Attach function takes Interfaces as parameters.
        }
    }

    //*****************************Install and start applications on UEs and remote host****************************//
    uint16_t dlPort = 2000;
    uint16_t ulPort = 7000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;
    for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
        ++dlPort;				//each Ue will contact with the remoteHost by a different dlPort (the remoteHost needs this).
	++ulPort;
        

        if (isTcp == 1){
					/*********TCP Application********/
					//Create a packet sink to receive packet on remoteHost
     			                LogComponentEnable("Queue",level);    //Only enable Queue monitoring for TCP to accelerate experiment speed.
					PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
					serverApps.Add(sink.Install(ueNodes.Get(u)));

					OnOffHelper onOffHelper("ns3::TcpSocketFactory", Address ( InetSocketAddress(ueIpIface.GetAddress(u) , dlPort) ));
					onOffHelper.SetConstantRate( DataRate(dataRate), packetSize );
					if (numberOfPackets != 0)
						onOffHelper.SetAttribute("MaxBytes",UintegerValue(packetSize*numberOfPackets));
					clientApps.Add(onOffHelper.Install(remoteHost));
        }
        else{
					PUT_SAMPLING_INTERVAL = PUT_SAMPLING_INTERVAL*40;
					/*********UDP Application********/
					//Create a packet sink to receive packet on remoteHost
					PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
					serverApps.Add(sink.Install(ueNodes.Get(u)));

					OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address ( InetSocketAddress(ueIpIface.GetAddress(u), dlPort) ));
					onOffHelper.SetConstantRate( DataRate(dataRate), packetSize );
					if (numberOfPackets != 0)
						onOffHelper.SetAttribute("MaxBytes",UintegerValue(packetSize*numberOfPackets));
					clientApps.Add(onOffHelper.Install(remoteHost));

					PacketSinkHelper ul_sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
					serverApps.Add(ul_sink.Install(remoteHost));

					OnOffHelper ul_onOffHelper("ns3::UdpSocketFactory", Address ( InetSocketAddress(remoteHostAddr, ulPort) ));
					ul_onOffHelper.SetConstantRate( DataRate(dataRate), packetSize );
					if (numberOfPackets != 0)
						onOffHelper.SetAttribute("MaxBytes",UintegerValue(packetSize*numberOfPackets));
					clientApps.Add(ul_onOffHelper.Install(ueNodes.Get(u)));

        }

    }




    /*******************Start client and server apps***************/
    serverApps.Start (Seconds (0.01));		//All server start at 0.01s.
    clientApps.Start(Seconds(0.5));
    
    
    /*********Tracing settings***************/
    lteHelper->EnableTraces ();
    
    lteHelper->GetPdcpStats()->SetAttribute("EpochDuration", TimeValue( Seconds (0.010)) );		//set collection interval for PDCP.
    lteHelper->GetRlcStats()->SetAttribute("EpochDuration", TimeValue ( Seconds (0.010)))	;		//same for RLC
 

    if (isTcp==1)
    	// Uncomment to enable PCAP tracing
    	p2ph.EnablePcapAll(DIR+"lte-nsc");


    monitor = flowHelper.Install(ueNodes);
    monitor = flowHelper.Install(remoteHost);
    monitor = flowHelper.GetMonitor();


    /****ConfigStore setting****/
    Config::SetDefault("ns3::ConfigStore::Filename", StringValue("lte.out"));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("RawText"));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Save"));
    ConfigStore outputConfig;
    outputConfig.ConfigureDefaults();
    outputConfig.ConfigureAttributes();

  
    /*=============schedule to get TCP throughput============*/
    Time t = Seconds(0.0);
    Simulator::ScheduleWithContext (0 ,Seconds (0.0), &getTcpPut);
    Simulator::Schedule(Seconds(1), &pos_tracking, position_tracking_wp, ue_mobility_model);


    /*********Start the simulation*****/
    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    

    /**************Simulation stops here. Start printing out information (if needed)***********/
  *overall_wp->GetStream() << "=========Experiment=========\n"
    << "TCP: " << isTcp << std::endl
    << "Distance from eNB: " << distance << std::endl 
    << "Mobility: Pedestrian = " << isPedestrian << ", Fading = " << isFading 
    << ", Cost231 Path loss = " << isCost231 << std::endl;
   
  
  monitor->CheckForLostPackets();
  Ptr<ns3::Ipv4FlowClassifier> classifier = DynamicCast<ns3::Ipv4FlowClassifier> (flowHelper.GetClassifier());
  std::map <FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter){
    ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
    /*sending flows, from endhost (1.0.0.2:49153) to Ues (7.0.0.2:200x)*/
      if (iter->second.rxPackets > 1){
        meanTxRate_send[t.sourceAddress] = 8*iter->second.txBytes/(iter->second.timeLastTxPacket.GetDouble()-iter->second.timeFirstTxPacket.GetDouble())*ONEBIL/(1024);
        meanRxRate_send[t.sourceAddress] = 8*iter->second.rxBytes/(iter->second.timeLastRxPacket.GetDouble()-iter->second.timeFirstRxPacket.GetDouble())*ONEBIL/(1024);
        meanTcpDelay_send[t.sourceAddress] = iter->second.delaySum.GetDouble()/iter->second.rxPackets/1000000;
      }
      numOfLostPackets_send[t.sourceAddress] = iter->second.lostPackets; 
      numOfTxPacket_send[t.sourceAddress] = iter->second.txPackets;
  
    *macro_wp->GetStream() << "***Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << ":" << t.sourcePort << " Dst Addr " << t.destinationAddress << ":" << t.destinationPort  << std::endl
    << "Tx Packets " << iter->second.txPackets << std::endl
    << "Rx Packets " << iter->second.rxPackets << std::endl
    << "Lost packets " << iter->second.lostPackets << std::endl
    << "Lost ratio " << double (iter->second.lostPackets)/(iter->second.lostPackets+iter->second.rxPackets) << std::endl;
    if (iter->second.rxPackets > 1){
        *macro_wp->GetStream() << "Average delay received " << iter->second.delaySum/iter->second.rxPackets/1000000 << std::endl
        << "Mean received bitrate " << 8*iter->second.rxBytes/(iter->second.timeLastRxPacket-iter->second.timeFirstRxPacket)*ONEBIL/(1024) << std::endl
        << "Mean transmitted bitrate " << 8*iter->second.txBytes/(iter->second.timeLastTxPacket-iter->second.timeFirstTxPacket)*ONEBIL/(1024) << std::endl;
    }

    *overall_wp->GetStream()  << "***Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << ":" << t.sourcePort << " Dst Addr " << t.destinationAddress << ":" << t.destinationPort  << std::endl
    << "Tx Packets " << iter->second.txPackets << std::endl
    << "Rx Packets " << iter->second.rxPackets << std::endl
    << "Lost packets " << iter->second.lostPackets << std::endl
    << "Lost ratio " << double (iter->second.lostPackets)/(iter->second.lostPackets+iter->second.rxPackets) << std::endl;
    if (iter->second.rxPackets > 1){
        *overall_wp->GetStream() << "Average delay received " << iter->second.delaySum/iter->second.rxPackets/1000000 << std::endl
        << "Mean received bitrate " << 8*iter->second.rxBytes/(iter->second.timeLastRxPacket-iter->second.timeFirstRxPacket)*ONEBIL/(1024) << std::endl
        << "Mean transmitted bitrate " << 8*iter->second.txBytes/(iter->second.timeLastTxPacket-iter->second.timeFirstTxPacket)*ONEBIL/(1024) << std::endl;
    }

  }

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
		*debugger_wp->GetStream() << "init map ... " << t.destinationAddress << std::endl;
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

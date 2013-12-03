/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

//
// Network topology
//
//           10Mb/s, 10ms       10Mb/s, 10ms
//       (0_0)          (1_0)  (1_1)         (2_0)
//       n0-----------------n1-----------------n2
//	(Ue)		  (Enb, SPGW) 	   (end-host)	
// 	10.1.3.1:3000	10.1.3.2/10.1.2.1    10.1.2.2:49153
//
// 	Device topology:
// 	ue(n0)----------------------------enb(n1)-------------------------------endhost(n2)
// 	(ue_dev)          (enb_radio_dev)         (enb_core_dev)            (endhost_dev)  
//  Usage (e.g.): ./waf --run "scratch/emulated --<parameter1>"


#include <ctype.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/config-store.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("emulated_nsc");

#define kilo 1000
#define KILO 1024
#define TCP_SAMPLING_INTERVAL 0.001 //tcp flow sampling interval in second
#define ONEBIL kilo*kilo*kilo

static double timer = 0;
static double scheduler_timer = 0;
static uint32_t sim_time = 100;
static uint32_t packet_size = 900;
static std::string sending_rate = "100Mb/s"; //sending rate.
static std::string core_network_bandwidth = "1000Mb/s"; 	//core_network_bandwidth.
static uint32_t core_network_delay = 30;	//core_network_delay in millisenconds.
static uint32_t core_network_mtu = 1500; 	//core_network_mte in Bytes.
static double init_radio_bd = 25;
static std::string init_radio_bandwidth = "25Mb/s"; 	//radio_link_bandwidth (init).
static uint32_t init_radio_delay = 5;	//radio_link_delay (init) in millisenconds.
static uint32_t init_radio_mtu = 1500; 	//radio_link_mtu (init) in Bytes.
static uint16_t is_tcp = 1;
static Ptr<ns3::FlowMonitor> monitor;
static FlowMonitorHelper flowHelper;
static Ptr<ns3::Ipv4FlowClassifier> classifier;
static std::map <FlowId, FlowMonitor::FlowStats> stats;
static Ipv4Address ue_ip;
static Ipv4Address endhost_ip;
static Ipv4Address enb_radio_ip;
static Ipv4Address enb_core_ip;
/**sending flowS stats***/
double meanTxRate_send;
double meanRxRate_send;
double meanTcpDelay_send;
uint64_t numOfLostPackets_send;
uint64_t numOfTxPacket_send;
//double last_lost = 0;
static double tcp_delay = 0;
static double last_delay_sum = 0;
static uint32_t last_rx_pkts = 0;

/***acking flowS stats***/
double meanTxRate_ack;
double meanRxRate_ack;
double meanTcpDelay_ack;
uint64_t numOfLostPackets_ack;
uint64_t numOfTxPacket_ack;
static double tcp_delay_ack = 0;
static double last_delay_sum_ack = 0;
static uint32_t last_rx_pkts_ack = 0;

static Ptr<PointToPointNetDevice> enb_radio_dev; 	//device on the radio side of the enb
static Ptr<PointToPointNetDevice> enb_core_dev;		//device on the core side of the enb
static Ptr<PointToPointNetDevice> ue_dev;
static Ptr<PointToPointNetDevice> endhost_dev;
static PointToPointHelper radio_link;	
static double rate_slope = -0.1; //increase the radio link bandwidth by "rate_slope" Mbps per "time_step"
static double p_rate = init_radio_bd;
static std::string current_radio_rate = "";
static double time_step = 0.3; //time step for each increment of link rate.

static double last_tx_time = 0;
static double last_rx_time = 0;
static double last_tx_bytes = 0;
static double last_rx_bytes = 0;

/* Ascii output files name*/
//Note: TCP's traces were obtained from nsc, stored in TCP_LOG file.
static std::string DIR = "/var/tmp/ln_result/emulated/";
static std::string queues = DIR+"queues.txt";
static std::string put;
static std::string debugger = DIR+"debugger.info";

/********wrappers**********/
Ptr<OutputStreamWrapper> dev_queues_wp;
Ptr<OutputStreamWrapper> put_wp;
Ptr<OutputStreamWrapper> debugger_wp;


static AsciiTraceHelper asciiTraceHelper;
/**************NSC************/
static std::string nsc_stack="liblinux2.6.26.so";
static std::string TCP_VERSION="hybla"; //reno,westwood,vegas,veno,yeah,illinois,htcp,hybla 

static void
getTcpPut();


/*
 * Changing the radio link bandwidth every "time_step" second.
 * The changing rate determined by "rate_slope".
 */
static void link_change(){
  //p_rate = init_radio_bd + Simulator::Now().GetSeconds()*rate_slope; 
  if (p_rate > 9 && p_rate < 10 && Simulator::Now().GetSeconds() < 60 )
	p_rate = p_rate;
  else if (p_rate < 0.2 || p_rate > 24)
	p_rate = p_rate;
  else p_rate += rate_slope; 
  std::stringstream ss;
  ss << p_rate;
  current_radio_rate = ss.str()+"Mb/s";
  *debugger_wp->GetStream() << Simulator::Now().GetSeconds() << "s: " <<  current_radio_rate << "\n";
  Config::Set("/NodeList/1/DeviceList/0/$ns3::PointToPointNetDevice/DataRate",StringValue(current_radio_rate));
  while (scheduler_timer < sim_time){
	 scheduler_timer += time_step;
 	 Simulator::Schedule(Seconds(scheduler_timer),&link_change);
  }
}


int main (int argc, char *argv[])
{
   LogLevel level = (LogLevel) (LOG_LEVEL_ALL | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC);
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
  //  LogComponentEnable("TcpL4Protocol", LOG_LEVEL_ALL);
  //  LogComponentEnable("TcpSocketImpl", LOG_LEVEL_ALL);
  //  LogComponentEnable("PacketSink", LOG_LEVEL_ALL);
  //  LogComponentEnable("TcpLargeTransfer", LOG_LEVEL_ALL);
     // LogComponentEnable("TcpNewReno",level);
     // LogComponentEnable("TcpReno",level);
  //LogComponentEnable("TcpTahoe",level);
  //LogComponentEnable("NscTcpL4Protocol",LOG_LEVEL_DEBUG);
  LogComponentEnable("NscTcpSocketImpl",LOG_LEVEL_DEBUG);
  //LogComponentEnable("RttEstimator",level);
  //LogComponentEnable("TcpSocketBase",level);
  
    CommandLine cmd;
    cmd.AddValue("sim_time", "Total duration of the simulation [s])", sim_time);
    cmd.AddValue("packet_size", "Size of each packet", packet_size);
    cmd.AddValue("sending_rate", "Application sending rate", sending_rate);
    cmd.AddValue("core_network_bandwidth", "Core network Data Rate", core_network_bandwidth);
    cmd.AddValue("core_network_delay", "Core network Delay", core_network_delay);
    cmd.AddValue("core_network_mtu", "Core network MTU size", core_network_mtu);
    cmd.AddValue("is_tcp", "Transport protocol used", is_tcp);
 
    /**ConfigStore setting*/
    Config::SetDefault("ns3::ConfigStore::Filename", StringValue("emulated-nsc.in"));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("RawText"));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Load"));
    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();
    inputConfig.ConfigureAttributes();
    

  cmd.Parse (argc, argv);


   /* create files for wrappers */
    dev_queues_wp = asciiTraceHelper.CreateFileStream(queues);
    debugger_wp = asciiTraceHelper.CreateFileStream(debugger);



  NodeContainer n0n1;
  n0n1.Create (2);

  NodeContainer n1n2;
  n1n2.Add (n0n1.Get (1));
  n1n2.Create (1);

  Ptr<Node> remote_host = n1n2.Get(1);
  Ptr<Node> ue = n0n1.Get(0);
  Ptr<Node> enb = n1n2.Get(0); 

  // We create the channels first without any IP addressing information
  // First make and configure the helper, so that it will put the appropriate
  // attributes on the network interfaces and channels we are about to install.
  PointToPointHelper core_network_link;
  core_network_link.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (core_network_bandwidth)));
  core_network_link.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (core_network_delay)));
  core_network_link.SetDeviceAttribute ("Mtu", UintegerValue(core_network_mtu));
 
  radio_link.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (init_radio_bandwidth)));
  radio_link.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (init_radio_delay)));
  radio_link.SetDeviceAttribute ("Mtu", UintegerValue(init_radio_mtu));


  // And then install devices and channels connecting our topology.
  NetDeviceContainer radio_dev = radio_link.Install (n0n1); 	//Radio link devices, n0-Ue, n1-Enb,SPGW
  NetDeviceContainer core_dev = core_network_link.Install (n1n2);		//Core network devices, n2-endhost
  enb_radio_dev = radio_dev.Get(1)->GetObject<ns3::PointToPointNetDevice>();  //radio side enb device
  enb_core_dev = core_dev.Get(0)->GetObject<ns3::PointToPointNetDevice>();    //core side enb device
  endhost_dev = core_dev.Get(1)->GetObject<ns3::PointToPointNetDevice>();
  ue_dev = radio_dev.Get(0)->GetObject<ns3::PointToPointNetDevice>();

  // Now add ip/tcp stack to all nodes.
  InternetStackHelper internet;
  internet.Install (enb);
  internet.SetTcp ("ns3::NscTcpL4Protocol", "Library", StringValue(nsc_stack));
  internet.Install (remote_host);
  internet.Install (ue);
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_sack", StringValue ("1"));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_timestamps", StringValue ("0"));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_window_scaling", StringValue ("1"));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue (TCP_VERSION));
  
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer radio_interfs = ipv4.Assign (radio_dev); 
  ipv4.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer core_interfs = ipv4.Assign (core_dev);
  ue_ip = radio_interfs.GetAddress(0);
  endhost_ip = core_interfs.GetAddress(1);
  enb_radio_ip = radio_interfs.GetAddress(1);
  enb_core_ip = core_interfs.GetAddress(0);
  // and setup ip routing tables to get total ip-level connectivity.
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  //*****************************Install and start applications on UEs and remote host****************************//
    uint16_t ulPort = 3000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

   if (is_tcp == 1){
                LogComponentEnable("Queue",level);    //Only enable Queue monitoring for TCP to accelerate experiment speed.
                put = DIR + "tcp-put.txt";
                put_wp = asciiTraceHelper.CreateFileStream(put);
        				/*********TCP Application********/
       					PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
       					serverApps.Add(sink.Install(ue));

        				OnOffHelper onOffHelper("ns3::TcpSocketFactory", Address ( InetSocketAddress(ue_ip, ulPort) ));
        				onOffHelper.SetConstantRate( DataRate(sending_rate), packet_size );
       					clientApps.Add(onOffHelper.Install(remote_host));
   }
              else{
                put = DIR + "udp-put.txt";
                put_wp = asciiTraceHelper.CreateFileStream(put);
        					/*********UDP Application********/
        				PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), ulPort));
       					serverApps.Add(sink.Install(ue));

        				OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address ( InetSocketAddress( ue_ip, ulPort) ));
        				onOffHelper.SetConstantRate( DataRate(sending_rate), packet_size );
       					clientApps.Add(onOffHelper.Install(remote_host));
    }

  //===========Flow monitor==============//
  monitor = flowHelper.Install(ue);
  monitor = flowHelper.Install(remote_host);
  monitor = flowHelper.GetMonitor();




  /*******************Start client and server apps***************/
  serverApps.Start (Seconds (0.01));		//All server start at 0.01s.
  clientApps.Start (Seconds(0.5));

  Simulator::ScheduleWithContext (0 ,Seconds (0.0), &getTcpPut);
  Simulator::Schedule(Seconds(0.1), &link_change);
  
    /****ConfigStore setting****/
    Config::SetDefault("ns3::ConfigStore::Filename", StringValue("emulated-nsc.out"));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("RawText"));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Save"));
    ConfigStore outputConfig;
    outputConfig.ConfigureDefaults();
    outputConfig.ConfigureAttributes();


  //core_network_link.EnablePcap("core");
  // radio_link.EnablePcapAll("emulated");

  Simulator::Stop (Seconds (sim_time));
  Simulator::Run ();

  monitor->CheckForLostPackets();
  Ptr<ns3::Ipv4FlowClassifier> classifier = DynamicCast<ns3::Ipv4FlowClassifier> (flowHelper.GetClassifier());
  std::map <FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();


  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter){
    ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);

    /*sending flows, from endhost (1.0.0.2:49153) to Ues (7.0.0.2:200x)*/
    if (t.destinationPort >= 3001 && t.destinationPort <= 4000) {
      if (iter->second.rxPackets > 1){
        meanTxRate_send = 8*iter->second.txBytes/(iter->second.timeLastTxPacket.GetDouble()-iter->second.timeFirstTxPacket.GetDouble())*ONEBIL/kilo;
        meanRxRate_send = 8*iter->second.rxBytes/(iter->second.timeLastRxPacket.GetDouble()-iter->second.timeFirstRxPacket.GetDouble())*ONEBIL/kilo;
        meanTcpDelay_send = iter->second.delaySum.GetDouble()/iter->second.rxPackets/1000000;
      }
      numOfLostPackets_send = iter->second.lostPackets; 
      numOfTxPacket_send = iter->second.txPackets;
    }

    /*ack flow, from Ues (7.0.0.2:200x) to endhost (1.0.0.2:49153)*/
    if (t.destinationPort >= 49153){
      if (iter->second.rxPackets > 1){
        meanTxRate_ack = 8*iter->second.txBytes/(iter->second.timeLastTxPacket.GetDouble()-iter->second.timeFirstTxPacket.GetDouble())*ONEBIL/(1024);
        meanRxRate_ack = 8*iter->second.rxBytes/(iter->second.timeLastRxPacket.GetDouble()-iter->second.timeFirstRxPacket.GetDouble())*ONEBIL/(1024);
        meanTcpDelay_ack = iter->second.delaySum.GetDouble()/iter->second.rxPackets/1000000;
      }
      numOfLostPackets_ack = iter->second.lostPackets; 
      numOfTxPacket_ack = iter->second.txPackets;
    }

    NS_LOG_UNCOND("***Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << "Port " << t.sourcePort << " Dst Addr " << t.destinationAddress << "destination port " << t.destinationPort);
    NS_LOG_UNCOND("Tx Packets " << iter->second.txPackets);
    NS_LOG_UNCOND("Rx Packets " << iter->second.rxPackets);
    NS_LOG_UNCOND("Lost packets " << iter->second.lostPackets);
    NS_LOG_UNCOND("Lost ratio " << double (iter->second.lostPackets)/(iter->second.lostPackets+iter->second.rxPackets));
    if (iter->second.rxPackets > 1){
        NS_LOG_UNCOND("Average delay received " << iter->second.delaySum/iter->second.rxPackets/1000000);
        NS_LOG_UNCOND("Mean received bitrate " << 8*iter->second.rxBytes/(iter->second.timeLastRxPacket-iter->second.timeFirstRxPacket)*ONEBIL/(1024));
        NS_LOG_UNCOND("Mean transmitted bitrate " << 8*iter->second.txBytes/(iter->second.timeLastTxPacket-iter->second.timeFirstTxPacket)*ONEBIL/(1024));
    }
  }
  // NS_LOG_UNCOND ("ue ip = " << ue_ip << "endhost ip = " << endhost_ip << "enb radio/core ip = " << enb_radio_ip << "/" << enb_core_ip  );
  Simulator::Destroy ();
}

static void
getTcpPut(){

    monitor->CheckForLostPackets();
    classifier = DynamicCast<ns3::Ipv4FlowClassifier> (flowHelper.GetClassifier());
    stats = monitor->GetFlowStats();
   // NS_LOG_UNCOND ("QQQQ: " << Simulator::Now().GetSeconds() << " " << ue_dev->GetQueue()->GetNBytes() << " " << enb_radio_dev->GetQueue()->GetNBytes() << " " << enb_core_dev->GetQueue()->GetNBytes() << " " << endhost_dev->GetQueue()->GetNBytes()); 
    *dev_queues_wp->GetStream() << "QQQQ: " << Simulator::Now().GetSeconds() << " " << ue_dev->GetQueue()->GetNBytes() << " " << enb_radio_dev->GetQueue()->GetNBytes() << " " << enb_core_dev->GetQueue()->GetNBytes() << " " << endhost_dev->GetQueue()->GetNBytes() << std::endl;
   /*==============Get flows information============*/
   for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter){
    ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);

    /*sending flows, from endhost (1.0.0.2:49153) to Ues (7.0.0.2:200x)*/
    if (t.destinationPort >= 3000 && t.destinationPort <= 4000) {
      if (iter->second.rxPackets > 1){
        if ((last_tx_time + 200000000) < iter->second.timeLastTxPacket.GetDouble()){
            meanTxRate_send = 8*(iter->second.txBytes-last_tx_bytes)/(iter->second.timeLastTxPacket.GetDouble()-last_tx_time)*ONEBIL/kilo;
            meanRxRate_send = 8*(iter->second.rxBytes-last_rx_bytes)/(iter->second.timeLastRxPacket.GetDouble()-last_rx_time)*ONEBIL/kilo;
            last_tx_time = iter->second.timeLastTxPacket.GetDouble();
            last_tx_bytes = iter->second.txBytes;
            last_rx_time = iter->second.timeLastRxPacket.GetDouble();
            last_rx_bytes = iter->second.rxBytes;
        }
    	  if (iter->second.rxPackets > last_rx_pkts){
     	      meanTcpDelay_send = iter->second.delaySum.GetDouble()/iter->second.rxPackets/1000000;
    		    tcp_delay = (iter->second.delaySum.GetDouble() - last_delay_sum) / (iter->second.rxPackets - last_rx_pkts)/(kilo*kilo);
    		    last_delay_sum = iter->second.delaySum.GetDouble();
    		    last_rx_pkts = iter->second.rxPackets;
    	 }
      }
      numOfLostPackets_send = iter->second.lostPackets;
      /*  
      if (iter->second.lostPackets > last_lost){
	NS_LOG_UNCOND(Simulator::Now().GetMilliSeconds() << " Tcp lost= " << iter->second.lostPackets - last_lost);
	last_lost = iter->second.lostPackets;
	}
	*/
      numOfTxPacket_send = iter->second.txPackets;
    }

     /*ack flow, from Ues (7.0.0.2:200x) to endhost (1.0.0.2:49153)*/
    if (t.destinationPort >= 49153){
      if (iter->second.rxPackets > 1){
        meanTxRate_ack = 8*iter->second.txBytes/(iter->second.timeLastTxPacket.GetDouble()-iter->second.timeFirstTxPacket.GetDouble())*ONEBIL/(1024);
        meanRxRate_ack = 8*iter->second.rxBytes/(iter->second.timeLastRxPacket.GetDouble()-iter->second.timeFirstRxPacket.GetDouble())*ONEBIL/(1024);	
	if (iter->second.rxPackets > last_rx_pkts_ack){
 	       	meanTcpDelay_ack = iter->second.delaySum.GetDouble()/iter->second.rxPackets/1000000;
		tcp_delay_ack = (iter->second.delaySum.GetDouble() - last_delay_sum_ack) / (iter->second.rxPackets - last_rx_pkts_ack)/(kilo*kilo);
		last_delay_sum_ack = iter->second.delaySum.GetDouble();
		last_rx_pkts_ack = iter->second.rxPackets;
	}
      }
      numOfLostPackets_ack = iter->second.lostPackets; 
      numOfTxPacket_ack = iter->second.txPackets;
      /*
      if (iter->second.lostPackets > last_lost_ack){
		NS_LOG_UNCOND(Simulator::Now().GetMilliSeconds() << " Tcp_ack lost= " << iter->second.lostPackets - last_lost_ack);
		last_lost_ack = iter->second.lostPackets;
      }
      */
    }
   }
    //    NS_LOG_UNCOND (Simulator::Now().GetSeconds() << "\t"
    //               << ue_ip << "\t"
    //               << meanRxRate_send << "\t"
    //               << meanTcpDelay_send << "\t"
    //               << numOfLostPackets_send << "\t"
    //               << numOfTxPacket_send << "\t"
    //               << "x" << "\t"
    //               << "x" << "\t"
    //               << "x" << "\t"
    //               << "x" << "\t"
		  // << meanTxRate_send << "\t" << tcp_delay << "\t" << tcp_delay_ack);
    *put_wp->GetStream() << Simulator::Now().GetSeconds() << "\t"
                  << ue_ip << "\t"
                  << meanRxRate_send << "\t"
                  << meanTcpDelay_send << "\t"
                  << numOfLostPackets_send << "\t"
                  << numOfTxPacket_send << "\t"
                  << "x" << "\t"
                  << "x" << "\t"
                  << "x" << "\t"
                  << "x" << "\t"
                  << meanTxRate_send << "\t" 
                  << tcp_delay << "\t" 
                  << tcp_delay_ack << std::endl;
    while (timer < sim_time){
        timer += TCP_SAMPLING_INTERVAL;
        Simulator::Schedule(Seconds(timer),&getTcpPut);
    }
}

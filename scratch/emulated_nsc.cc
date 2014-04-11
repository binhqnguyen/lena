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
//	(Ue)		 	 						(Enb) 	   			(end-host)	
// 	10.1.3.1:3000	10.1.3.2/10.1.2.1    10.1.2.2:49153
//
// 	Device topology:
// 	ue(n0)----------------------------enb(n1)-------------------------------endhost(n2)
// 	(ue_dev)          (enb_radio_dev)         (enb_core_dev)            (endhost_dev)  
//  Usage (e.g.): ./waf --run "scratch/emulated --<parameter1>"


#include "emulated_nsc.h"
/*
 * Changing the radio link bandwidth every "time_step" second.
 * The changing rate determined by "rate_slope".
 */
static void link_change(){
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

/*
 * Schedule a radio bandwidth change (kbps) at a moment of the experiment
 */
static void change_radio_bandwidth_at_time(std::string bandwidth, double time_of_change){
	NS_LOG_UNCOND("Change radio bandwidth");
  Simulator::Schedule(Seconds(time_of_change), &set_radio_bandwidth, bandwidth);
}

/*
 * Schedule a link delay change (ms) at a moment of the experiment
 */
static void change_radio_delay_at_time(double delay, double time_of_change){
	NS_LOG_UNCOND("Change radio delay");
  Simulator::Schedule(Seconds(time_of_change), &set_radio_delay, delay);
}


/*
 * Set radio bandwidth
 */
static void set_radio_bandwidth(std::string bandwidth){
  Config::Set("/NodeList/1/DeviceList/0/$ns3::PointToPointNetDevice/DataRate",StringValue(bandwidth));
  *debugger_wp->GetStream() << Simulator::Now().GetSeconds() << "s: new bandwidth = " << bandwidth << "\n";
  NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "s: new bandwidth = " << bandwidth << "\n");
}

/*
 * Set radio link delay (millisecond)
 */
static void set_radio_delay(double delay){
  //Config::Set("/NodeList/1/DeviceList/0/$ns3::PointToPointNetDevice/Delay",TimeValue(MilliSeconds(delay)));
  //Config::Set("/ChannelListPriv/ChannelList/*/$ns3::PointToPointChannel/Delay",StringValue("1000ms"));
  //radio_link.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (5000)));
  //Config::Set("/ChannelListPriv/ChannelList/*/$ns3::PointToPointChannel/Delay",StringValue("1000ms"));
  *debugger_wp->GetStream() << Simulator::Now().GetSeconds() << "s: new delay (ms) = " << delay << "\n";
  NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "s: new delay (ms) = " << delay << "\n");
}



int main (int argc, char *argv[])
{
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
	if (is_tcp==1){
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
  LogComponentEnable("NscTcpSocketImpl",level_info);
  //LogComponentEnable("RttEstimator",level);
  //LogComponentEnable("TcpSocketBase",level);
  }


   /* create files for wrappers */
    dev_queues_wp = asciiTraceHelper.CreateFileStream(queues);
    debugger_wp = asciiTraceHelper.CreateFileStream(debugger);
		macro_wp = asciiTraceHelper.CreateFileStream(macro);



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
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_sack", StringValue (TCP_SACK));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_timestamps", StringValue (TCP_TIMESTAMP));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_window_scaling", StringValue (TCP_WINDOWSCALING));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue (TCP_VERSION));
  Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_frto", StringValue (TCP_FRTO));
  
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
    uint16_t dlPort = 3000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

   if (is_tcp == 1){
                LogComponentEnable("Queue",level_all);    //Only enable Queue monitoring for TCP to accelerate experiment speed.
                put = DIR + "tcp-put.dat";
                put_wp = asciiTraceHelper.CreateFileStream(put);
        				/*********TCP Application********/
       					PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
       					serverApps.Add(sink.Install(ue));

        				OnOffHelper onOffHelper("ns3::TcpSocketFactory", Address ( InetSocketAddress(ue_ip, dlPort) ));
        				onOffHelper.SetConstantRate( DataRate(sending_rate), packet_size );
       					clientApps.Add(onOffHelper.Install(remote_host));
   }
              else{
                put = DIR + "udp-put.dat";
                put_wp = asciiTraceHelper.CreateFileStream(put);
        					/*********UDP Application********/
        				PacketSinkHelper sink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), dlPort));
       					serverApps.Add(sink.Install(ue));

        				OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address ( InetSocketAddress( ue_ip, dlPort) ));
        				onOffHelper.SetConstantRate( DataRate(sending_rate), packet_size );
       					clientApps.Add(onOffHelper.Install(remote_host));

								PacketSinkHelper sinkul("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
       					serverApps.Add(sinkul.Install(remote_host));

        				OnOffHelper onOffHelperul("ns3::UdpSocketFactory", Address ( InetSocketAddress( endhost_ip, 5000) ));
        				onOffHelperul.SetConstantRate( DataRate(sending_rate), packet_size );
       					clientApps.Add(onOffHelperul.Install(ue));

    }

  //===========Flow monitor==============//
  monitor = flowHelper.Install(ue);
  monitor = flowHelper.Install(remote_host);
  monitor = flowHelper.GetMonitor();




  /*******************Start client and server apps***************/
  serverApps.Start (Seconds (0.01));		//All server start at 0.01s.
  clientApps.Start (Seconds(0.5));

  Simulator::ScheduleWithContext (0 ,Seconds (0.0), &getTcpPut);
	change_radio_bandwidth_at_time("30kb/s",7); //change bandwidth at 5s.
	change_radio_delay_at_time(1000000000,300); //change radio link delay
  //Simulator::Schedule(Seconds(0.1), &link_change);
  
    /****ConfigStore setting****/
    Config::SetDefault("ns3::ConfigStore::Filename", StringValue("emulated-nsc.out"));
    Config::SetDefault("ns3::ConfigStore::FileFormat", StringValue("RawText"));
    Config::SetDefault("ns3::ConfigStore::Mode", StringValue("Save"));
    ConfigStore outputConfig;
    outputConfig.ConfigureDefaults();
    outputConfig.ConfigureAttributes();

  Config::Set("/NodeList/1/DeviceList/0/$ns3::PointToPointNetDevice/DataRate",StringValue(init_radio_bandwidth));

  //core_network_link.EnablePcap("core");
  radio_link.EnablePcapAll("emulated");

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
  // NS_LOG_UNCOND ("ue ip = " << ue_ip << "endhost ip = " << endhost_ip << "enb radio/core ip = " << enb_radio_ip << "/" << enb_core_ip  );
  Simulator::Destroy ();
}

static void
getTcpPut(){
    monitor->CheckForLostPackets();
    classifier = DynamicCast<ns3::Ipv4FlowClassifier> (flowHelper.GetClassifier());
    stats = monitor->GetFlowStats();
    *dev_queues_wp->GetStream() << "QQQQ: " << Simulator::Now().GetSeconds() << " " << ue_dev->GetQueue()->GetNBytes() << " " << enb_radio_dev->GetQueue()->GetNBytes() << " " << enb_core_dev->GetQueue()->GetNBytes() << " " << endhost_dev->GetQueue()->GetNBytes() << std::endl;
   /*==============Get flows information============*/
   for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter){
    ns3::Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);

    /*sending flows, from endhost (1.0.0.2:49153) to Ues (7.0.0.2:200x)*/
    if (t.destinationPort >= 3000 && t.destinationPort <= 4000) {
      if (iter->second.rxPackets > 1){
        if ((last_tx_time < iter->second.timeLastTxPacket.GetDouble())){
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
    }
   }
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
                  << tcp_delay_ack << "\t"
                  << last_tx_bytes << "\t" 
                  << last_rx_bytes << std::endl; 
    while (timer < sim_time){
        timer += TCP_SAMPLING_INTERVAL;
        Simulator::Schedule(Seconds(timer),&getTcpPut);
    }
}

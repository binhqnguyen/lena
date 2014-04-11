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
#define TCP_SAMPLING_INTERVAL 0.01//tcp flow sampling interval in second
#define ONEBIL kilo*kilo*kilo

static double timer = 0;
static double scheduler_timer = 0;
static uint32_t sim_time = 100;
static uint32_t packet_size = 900;
static std::string sending_rate = "200Mb/s"; //sending rate.
static std::string core_network_bandwidth = "1000Mb/s"; 	//core_network_bandwidth.
static uint32_t core_network_delay = 18;	//core_network_delay in millisenconds.
static uint32_t core_network_mtu = 1500; 	//core_network_mte in Bytes.
static double init_radio_bd = 1;
static std::string init_radio_bandwidth = "500kb/s"; 	//radio_link_bandwidth (init).
static uint32_t init_radio_delay = 17;	//radio_link_delay (init) in millisenconds.
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
static PointToPointHelper core_network_link;
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
static std::string macro = DIR+"macro_tcp.info";

/********wrappers**********/
Ptr<OutputStreamWrapper> dev_queues_wp;
Ptr<OutputStreamWrapper> put_wp;
Ptr<OutputStreamWrapper> debugger_wp;
Ptr<OutputStreamWrapper> macro_wp;

static AsciiTraceHelper asciiTraceHelper;
/**************NSC************/
static std::string nsc_stack="liblinux2.6.26.so";
static std::string TCP_VERSION="cubic"; //reno,westwood,vegas,veno,yeah,illinois,htcp,hybla 
static std::string TCP_SACK = "1";
static std::string TCP_TIMESTAMP = "1";
static std::string TCP_WINDOWSCALING = "1";
static std::string TCP_FRTO = "2";



static void
getTcpPut();
static void change_radio_bandwidth_at_time(std::string bandwidth, double time_of_change);
static void set_radio_bandwidth(std::string bandwidth);
static void set_radio_delay(double delay);
static void change_radio_delay_at_time(double delay, double time_of_change);

//static void change_link_bandwidth(double link_bd);

LogLevel level_info = (LogLevel) (LOG_LEVEL_INFO| LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC);
LogLevel level_all = (LogLevel) (LOG_LEVEL_ALL| LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC);
LogLevel level_debug = (LogLevel) (LOG_LEVEL_DEBUG| LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_PREFIX_FUNC);


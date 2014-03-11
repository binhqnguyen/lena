#ifndef LTE_UES_H
#define LTE_UES_H

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
#define COARSE_GRAIN_SAMPLING 60

double simTime = 50;   //simulation time for EACH application
static double ue_position_tracking_timer = 0; //timer to schedule position tracking


int isPedestrian = -1; //-1=no fading trace and no mobility, 0=vehicular trace, 1=pedestrian trace.
uint16_t traceTime = 50;       //trace file period, in seconds
std::string P_TRACE_FILE = "/var/tmp/ln_result/fading_traces/fading_trace_DL_EPA_50RB_band4_3kmph.fad";
std::string V_TRACE_FILE = "/var/tmp/ln_result/fading_traces/fading_trace_DL_EPA_50RB_band4_40kmph.fad";
std::string traceFile = P_TRACE_FILE;   //location of trace file.
uint16_t isFading = 1;

/***** Mobility parameters******/
uint16_t is_random_allocation = 0;  //UEs fixed position allocation by default
//UE grid allocation parameters.
double distance_ue_root_x = 100;
double distance_ue_root_y = 100;
double distance_among_ues_x = 20;
double distance_among_ues_y = -100;
//ENB grid allocation parameters.
double distance_among_enbs_x = 300;
double distance_among_enbs_y = 750;
//UE circle random allocation radius.
double distance_rho = 200;
//Moving
double moving_bound = 50000;
double VEH_VE = 40; //40km/h for vehicular
double PED_VE = 3; //3km/h for pedestrian
double moving_speed = 3; //UE moving speed in km/h.
double speed_mps = 11; //20m/s

/*******Simulation******/
uint16_t radioUlBandwidth = 50;  
uint16_t radioDlBandwidth = 50;  //same as above, for downlink.
std::string SACK="1";
std::string TIME_STAMP="0";
std::string WINDOW_SCALING="1";
std::string TCP_VERSION="cubic"; //reno,westwood,vegas,veno,yeah,illinois,htcp,hybla
std::string TCP_FRTO="0"; 
std::string configure_input = "lte-handover.in";
std::string configure_output = "lte-handover.out";


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
uint32_t ho_type = 1;
uint8_t a2_servingcell_threshold = 34; //if current cell signal strength smaller than this, consider HO (default 30) [0-34] as in Section 9.1.7 of [TS36133]
uint8_t a4_neighbourcell_offset = 1; //if neighbour cell signal strength is larger than the source cell by this amount, allow HO. (default 1).

double X2_path_delay = 19; //X2 forwarding path delay in ms.
NodeContainer current_ue_nodes; //list that store the current UEs in the testing cell (ENB1).
NodeContainer gone_ue_nodes;  //Ues that handovered to the neighbor cell (ENB2).
uint32_t gone_ue_cnt = 0;


//MAC
//std::string mac_scheduler = "ns3::RrFfMacScheduler";
std::string mac_scheduler = "ns3::PfFfMacScheduler";

//CORE
std::string s1uLinkDataRate = "1Gb/s";
double s1uLinkDelay = 0.015;
double s1uLinkMtu = 1500;
std::string p2pLinkDataRate = "1Gb/s";
double p2pLinkDelay = 0.020;
double p2pLinkMtu = 1500;

/****Application******/
uint32_t isTcp=1;
uint32_t packetSize = 900;
double samplingInterval = 0.01;    /*getTcp() function invoke for each x second*/
uint16_t PUT_SAMPLING_INTERVAL = 1; /*sample a TCP throughput for each x pkts*/
double put_sampling_timer = 0.0;    /*getTcpPut() invoked each period*/
double ue_joining_timer = 0.0;
Ptr<ns3::FlowMonitor> monitor;
FlowMonitorHelper flowHelper;
Ptr<ns3::Ipv4FlowClassifier> classifier;
std::map <FlowId, FlowMonitor::FlowStats> stats;
std::string dataRate = "150Mb/s";
uint16_t dlPort = 10000;
uint16_t ulPort = 20000;
//LogLevel logLevel = (LogLevel) (LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC | LOG_LEVEL_DEBUG);
LogLevel logLevel = (LogLevel) (LOG_PREFIX_TIME | LOG_PREFIX_NODE| LOG_PREFIX_FUNC | LOG_LEVEL_INFO);

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

uint16_t numberOfEnbs = 2;
uint16_t numBearersPerUe = 1;

/********DEBUGGING mode********/
uint32_t isDebug = 0;

/********* Ascii output files name *********/
static std::string TAG = "512KB";
static std::string DIR = "/var/tmp/ln_result/radio/";
static std::string TOTAL_DIR = "/var/tmp/ln_result/total/lossless_ho/";
static std::string macro = DIR+"macro_output.dat";
static std::string put_send;
static std::string debugger = DIR+"debugger.dat";
static std::string course_change = DIR+"course_change.dat";
static std::string overall = "overall.out";
static std::string position_tracking = DIR+"position_tracking.dat";
static std::string throughput_goodput_delay = TOTAL_DIR+"throughput_goodput_delay"+TAG+".dat";

/********wrappers**********/
static AsciiTraceHelper asciiTraceHelper;
Ptr<OutputStreamWrapper> put_send_wp;
Ptr<OutputStreamWrapper> macro_wp;
Ptr<OutputStreamWrapper> debugger_wp;
Ptr<OutputStreamWrapper> ue_positions_wp;
Ptr<OutputStreamWrapper> overall_wp;
Ptr<OutputStreamWrapper> position_tracking_wp;
Ptr<OutputStreamWrapper> throughput_goodput_delay_wp;

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

double on_time = 30.0;
struct UES{
  NodeContainer ueNodes;
  NetDeviceContainer ueLteDevs;
  Ipv4InterfaceContainer ueIpIfaces;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
};

/*******Function prototype*******/
void SetUpRandomSeed();  
void getTcpPut();
void get_average_result();
void EnableLogComponents();
void SetDefaultConfigs();
void CommandlineParameters (int argc, char* argv[]);
void InstallMobilityUe (NodeContainer ueNodes, uint16_t is_pedestrian);
void InstallLocationUe(NodeContainer ueNodes, uint32_t distance_rho);
//void InstallLocationUe(NodeContainer ueNodes, double distance_ue_root_x, double distance_ue_root_y, double distance_among_ues_x, double distance_among_ues_y);
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
UES CreateUes(uint32_t number_of_ues, double startTime, double on_time);
void UeLeaving(uint32_t number_of_ues, Node target_enb);
void InstallWorkload (UES ueNodes);
void ScheduleUeAttach(double second, UES ues);
void UeAttach(UES ues);
void SetAppsStartTime (double startTime, ApplicationContainer serverApps, ApplicationContainer clientApps);

#endif

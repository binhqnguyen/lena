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
#include "lte-ues.h"

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

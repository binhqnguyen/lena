/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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

#include "ns3/simulator.h"
#include "ns3/log.h"

#include "ns3/lte-rlc-am-header.h"
#include "ns3/lte-pdcp-header.h"
#include "ns3/lte-rlc-am.h"
#include "ns3/lte-rlc-sdu-status-tag.h"
#include "ns3/lte-rlc-tag.h"
#include "ns3/random-variable.h"

NS_LOG_COMPONENT_DEFINE ("LteRlcAm");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (LteRlcAm);


LteRlcAm::LteRlcAm ()
{
  NS_LOG_FUNCTION (this);

  // Buffers
  m_txonBufferSize = 0;
  m_retxBuffer.resize (1024);
  m_retxBufferSize = 0;
  m_txedBuffer.resize (1024);
  m_txedBufferSize = 0;
  m_maxTxBufferSize = 10*1024;  //Binh: added maxTxBufferSize. In Bytes. Default 10KB.
	m_transmittingRlcSduBufferSize = 0;

  m_statusPduRequested = false;
  m_statusPduBufferSize = 0;

  // State variables: transmitting side
  m_windowSize = 512;
  m_vtA  = 0;
  m_vtMs = m_vtA + m_windowSize;
  m_vtS  = 0;
  m_pollSn = 0;

  // State variables: receiving side
  m_vrR  = 0;
  m_vrMr = m_vrR + m_windowSize;
  m_vrX  = 0;
  m_vrMs = 0;
  m_vrH  = 0;

  // Counters
  m_pduWithoutPoll  = 0;
  m_byteWithoutPoll = 0;

  // Configurable parameters
  m_maxRetxThreshold = 5;
  m_pollPdu = 1;
  m_pollByte = 50;

  // SDU reassembling process
  m_reassemblingState = WAITING_S0_FULL;
  m_expectedSeqNumber = 0;
	
	//Reassemble
	m_reassembleExpectedSeqNumber = 0;
  m_assemblingState = WAITING_SI_SF;

  // Timers
  m_pollRetransmitTimerValue = MilliSeconds (100);
  m_reorderingTimerValue = MilliSeconds (20);
  m_statusProhibitTimerValue = MilliSeconds (20);
  m_rbsTimerValue = MilliSeconds (10);

  m_pollRetransmitTimerJustExpired = false;
}

LteRlcAm::~LteRlcAm ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
LteRlcAm::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LteRlcAm")
    .SetParent<LteRlc> ()
    .AddConstructor<LteRlcAm> ()
    .AddAttribute ("PollRetransmitTimer",
                   "Value of the t-PollRetransmit (See section 7.3 of 3GPP TS 36.322)",
                   TimeValue (MilliSeconds (100)),
                   MakeTimeAccessor (&LteRlcAm::m_pollRetransmitTimerValue),
                   MakeTimeChecker ())
    .AddAttribute ("TxOpportunityForRetxAlwaysBigEnough",
                   "If true, always pretend that the size of a TxOpportunity is big enough "
                   "for retransmission. If false (default and realistic behavior), no retx "
                   "is performed unless the corresponding TxOpportunity is big enough.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&LteRlcAm::m_txOpportunityForRetxAlwaysBigEnough),
                   MakeBooleanChecker ())
    .AddAttribute ("MaxTxBufferSize",
                   "Maximum Size of the Transmission Buffer (in Bytes)",
                   UintegerValue (10 * 1024),
                   MakeUintegerAccessor (&LteRlcAm::m_maxTxBufferSize),
                   MakeUintegerChecker<uint32_t> ())

    ;
  return tid;
}

void
LteRlcAm::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_pollRetransmitTimer.Cancel ();
  m_reorderingTimer.Cancel ();
  m_statusProhibitTimer.Cancel ();
  m_rbsTimer.Cancel ();

  m_txonBuffer.clear ();
  m_txonBufferSize = 0;
  m_txedBuffer.clear ();
  m_txedBufferSize = 0;
  m_retxBuffer.clear ();
  m_retxBufferSize = 0;
  m_rxonBuffer.clear ();
  m_sdusBuffer.clear ();
  m_sdusAssembleBuffer.clear ();
  m_keepS0 = 0;
  m_controlPduBuffer = 0;
  m_maxTxBufferSize = 0; 
	m_reassembleExpectedSeqNumber = 0;
  m_expectedSeqNumber = 0;
	m_transmittingRlcSduBufferSize = 0;
	m_transmittingRlcSduBuffer.clear();
  LteRlc::DoDispose ();
}


/**
 * RLC SAP
 */

void
LteRlcAm::DoTransmitPdcpPdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid << p->GetSize ());

  if (m_txonBufferSize + p->GetSize() < m_maxTxBufferSize){
    /** Store arrival time */
    Time now = Simulator::Now ();
    RlcTag timeTag (now);
    p->AddPacketTag (timeTag);

    /** Store PDCP PDU */

    LteRlcSduStatusTag tag;
    tag.SetStatus (LteRlcSduStatusTag::FULL_SDU);
    p->AddPacketTag (tag);

    NS_LOG_LOGIC ("Txon Buffer: New packet added");
    m_txonBuffer.push_back (p);
    m_txonBufferSize += p->GetSize ();
    NS_LOG_LOGIC ("NumOfBuffers = " << m_txonBuffer.size() );
    NS_LOG_LOGIC ("txonBufferSize = " << m_txonBufferSize);
    NS_LOG_DEBUG ("txonBufferSize = " << m_txonBufferSize);
    NS_LOG_DEBUG ("packet size     = " << p->GetSize ());
  }
  else {
      NS_LOG_LOGIC ("TxBuffer is full. RLC SDU discarded");
      NS_LOG_LOGIC ("MaxTxBufferSize = " << m_maxTxBufferSize);
      NS_LOG_LOGIC ("txBufferSize    = " << m_txonBufferSize);
      NS_LOG_LOGIC ("packet size     = " << p->GetSize ());
      NS_LOG_DEBUG ("TxBuffer is full. txonBufferSize = " << m_txonBufferSize);
  }

  /** Report Buffer Status */
  DoReportBufferStatus ();
  m_rbsTimer.Cancel ();
  m_rbsTimer = Simulator::Schedule (m_rbsTimerValue, &LteRlcAm::ExpireRbsTimer, this);
}


/**
 * MAC SAP
 */

void
LteRlcAm::DoNotifyTxOpportunity (uint32_t bytes, uint8_t layer, uint8_t harqId)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid << bytes);

  if (bytes < 4)
    {
      // Stingy MAC: In general, we need more bytes.
      // There are a more restrictive test for each particular case
      NS_LOG_LOGIC ("TxOpportunity (size = " << bytes << ") too small");
      NS_ASSERT_MSG (false, "TxOpportunity (size = " << bytes << ") too small.\n"
                         << "Your MAC scheduler is assigned too few resource blocks.");
      return;
    }

  if ( m_statusPduRequested && ! m_statusProhibitTimer.IsRunning () )
    {
      if (bytes < m_statusPduBufferSize)
        {
          // Stingy MAC: We need more bytes for the STATUS PDU
          NS_LOG_LOGIC ("TxOpportunity (size = " << bytes << ") too small for the STATUS PDU (size = " << m_statusPduBufferSize << ")");
          NS_ASSERT_MSG (false, "TxOpportunity (size = " << bytes << ") too small for the STATUS PDU (size = " << m_statusPduBufferSize << ")\n"
                             << "Your MAC scheduler is assigned too few resource blocks.");
          return;
        }

      NS_LOG_LOGIC ("Sending STATUS PDU");

      Ptr<Packet> packet = Create<Packet> ();
      LteRlcAmHeader rlcAmHeader;
      rlcAmHeader.SetControlPdu (LteRlcAmHeader::STATUS_PDU);
     
      NS_LOG_LOGIC ("Check for SNs to NACK from " << m_vrR.GetValue() << " to " << m_vrMs.GetValue());
      SequenceNumber10 sn;
      sn.SetModulusBase (m_vrR);
      std::map<uint16_t, PduBuffer>::iterator pduIt;
      for (sn = m_vrR; sn < m_vrMs; sn++) 
        {
          NS_LOG_LOGIC ("SN = " << sn);          
          if (!rlcAmHeader.OneMoreNackWouldFitIn (bytes))
            {
              NS_LOG_LOGIC ("Can't fit more NACKs in STATUS PDU");
              break;
            }          
          pduIt = m_rxonBuffer.find (sn.GetValue ());
          if (pduIt == m_rxonBuffer.end () || (!(pduIt->second.m_pduComplete)))
            {
              NS_LOG_LOGIC ("adding NACK_SN " << sn.GetValue ());
              rlcAmHeader.PushNack (sn.GetValue ());              
            }          
        }
      NS_LOG_LOGIC ("SN at end of NACK loop = " << sn);
      // 3GPP TS 36.322 section 6.2.2.1.4 ACK SN
      // find the  SN of the next not received RLC Data PDU 
      // which is not reported as missing in the STATUS PDU. 
      pduIt = m_rxonBuffer.find (sn.GetValue ());
      while ((sn < m_vrMs) && (pduIt != m_rxonBuffer.end ()) && (pduIt->second.m_pduComplete))            
        {
          NS_LOG_LOGIC ("SN = " << sn << " < " << m_vrMs << " = " << (sn < m_vrMs));
          sn++;
          NS_LOG_LOGIC ("SN = " << sn);
          pduIt = m_rxonBuffer.find (sn.GetValue ());
        }
      
      NS_ASSERT_MSG (sn <= m_vrMs, "first SN not reported as missing = " << sn << ", VR(MS) = " << m_vrMs);      
      rlcAmHeader.SetAckSn (sn); 


      NS_LOG_LOGIC ("RLC header: " << rlcAmHeader);
      packet->AddHeader (rlcAmHeader);

      // Send RLC PDU to MAC layer
      LteMacSapProvider::TransmitPduParameters params;
      params.pdu = packet;
      params.rnti = m_rnti;
      params.lcid = m_lcid;
      params.layer = layer;
      params.harqProcessId = harqId;

      m_macSapProvider->TransmitPdu (params);

      m_statusPduRequested = false;
      m_statusPduBufferSize = 0;
      m_statusProhibitTimer = Simulator::Schedule (m_statusProhibitTimerValue,
                                                   &LteRlcAm::ExpireStatusProhibitTimer, this);
      return;
    }
  else if ( m_retxBufferSize > 0 )
    {
      NS_LOG_LOGIC ("retxBufferSize = " << m_retxBufferSize);      
      NS_LOG_LOGIC ("Sending data from Retransmission Buffer");
      NS_ASSERT (m_vtA < m_vtS);
      SequenceNumber10 sn;
      sn.SetModulusBase (m_vtA);
      bool found = false;
      for (sn = m_vtA; sn < m_vtS; sn++) 
        {
          uint16_t seqNumberValue = sn.GetValue ();
          NS_LOG_LOGIC ("SN = " << seqNumberValue << " m_pdu " << m_retxBuffer.at (seqNumberValue).m_pdu);

          if (m_retxBuffer.at (seqNumberValue).m_pdu != 0)
            {            

              Ptr<Packet> packet = m_retxBuffer.at (seqNumberValue).m_pdu->Copy ();
              
              if (( packet->GetSize () <= bytes )
                  || m_txOpportunityForRetxAlwaysBigEnough)
                {
                  found = true;
                  // According to 5.2.1, the data field is left as is, but we rebuild the header
                  LteRlcAmHeader rlcAmHeader;
                  packet->RemoveHeader (rlcAmHeader);
                  NS_LOG_LOGIC ("old AM RLC header: " << rlcAmHeader);

                  // Calculate the Polling Bit (5.2.2.1)
                  rlcAmHeader.SetPollingBit (LteRlcAmHeader::STATUS_REPORT_NOT_REQUESTED);

                  NS_LOG_LOGIC ("polling conditions: m_txonBuffer.empty=" << m_txonBuffer.empty () 
                                << " retxBufferSize="  << m_retxBufferSize
                                << " packet->GetSize ()=" << packet->GetSize ());
                  if (((m_txonBuffer.empty ()) && (m_retxBufferSize == packet->GetSize () + rlcAmHeader.GetSerializedSize ())) 
                      || (m_vtS >= m_vtMs)
                      || m_pollRetransmitTimerJustExpired)
                    {
                      m_pollRetransmitTimerJustExpired = false;
                      rlcAmHeader.SetPollingBit (LteRlcAmHeader::STATUS_REPORT_IS_REQUESTED);
                      m_pduWithoutPoll = 0;
                      m_byteWithoutPoll = 0;

                      m_pollSn = m_vtS - 1;
                      NS_LOG_LOGIC ("New POLL_SN = " << m_pollSn);

                      if (! m_pollRetransmitTimer.IsRunning () )
                        {
                          NS_LOG_LOGIC ("Start PollRetransmit timer");

                          m_pollRetransmitTimer = Simulator::Schedule (m_pollRetransmitTimerValue,
                                                                       &LteRlcAm::ExpirePollRetransmitTimer, this);
                        }
                      else
                        {
                          NS_LOG_LOGIC ("Restart PollRetransmit timer");

                          m_pollRetransmitTimer.Cancel ();
                          m_pollRetransmitTimer = Simulator::Schedule (m_pollRetransmitTimerValue,
                                                                       &LteRlcAm::ExpirePollRetransmitTimer, this);
                        }
                    }

                  packet->AddHeader (rlcAmHeader);
                  NS_LOG_LOGIC ("new AM RLC header: " << rlcAmHeader);
                  
                  // Send RLC PDU to MAC layer
                  LteMacSapProvider::TransmitPduParameters params;
                  params.pdu = packet;
                  params.rnti = m_rnti;
                  params.lcid = m_lcid;
                  params.layer = layer;
                  params.harqProcessId = harqId;
                  
                  m_macSapProvider->TransmitPdu (params);

                  m_retxBuffer.at (seqNumberValue).m_retxCount++;
                  NS_LOG_INFO ("Incr RETX_COUNT for SN = " << seqNumberValue);
                  if (m_retxBuffer.at (seqNumberValue).m_retxCount >= m_maxRetxThreshold)
                    {
                      NS_LOG_INFO ("Max RETX_COUNT for SN = " << seqNumberValue);
                    }

                  NS_LOG_INFO ("Move SN = " << seqNumberValue << " back to txedBuffer");
                  m_txedBuffer.at (seqNumberValue).m_pdu = m_retxBuffer.at (seqNumberValue).m_pdu->Copy ();
                  m_txedBuffer.at (seqNumberValue).m_retxCount = m_retxBuffer.at (seqNumberValue).m_retxCount;
                  m_txedBufferSize += m_txedBuffer.at (seqNumberValue).m_pdu->GetSize ();

                  m_retxBufferSize -= m_retxBuffer.at (seqNumberValue).m_pdu->GetSize ();
                  m_retxBuffer.at (seqNumberValue).m_pdu = 0;
                  m_retxBuffer.at (seqNumberValue).m_retxCount = 0;
									NS_LOG_DEBUG (this << "retxdBuffer.remove SEQ = " << seqNumberValue);
                  
                  NS_LOG_LOGIC ("retxBufferSize = " << m_retxBufferSize);
                  //NS_LOG_DEBUG ("retxBufferSize = " << m_retxBufferSize);

                  return;
                }
              else
                {
                  NS_LOG_LOGIC ("TxOpportunity (size = " << bytes << ") too small for retransmission of the packet (size = " << packet->GetSize () << ")");
                  NS_LOG_LOGIC ("Waiting for bigger TxOpportunity");
                  return;
                }
            }
        }
      NS_ASSERT_MSG (found, "m_retxBufferSize > 0, but no PDU considered for retx found");
    }
  else if ( m_txonBufferSize > 0 )
    {
      if (bytes < 7)
      {
        // Stingy MAC: We need more bytes for new DATA PDUs.
        NS_LOG_LOGIC ("TxOpportunity (size = " << bytes << ") too small for DATA PDU");
        NS_ASSERT_MSG (false, "TxOpportunity (size = " << bytes << ") too small for DATA PDU\n"
                           << "Your MAC scheduler is assigned too few resource blocks.");
        return;
      }

      NS_LOG_LOGIC ("Sending data from Transmission Buffer");
    }
  /* else if ( m_txedBufferSize > 0 )
    {
      NS_LOG_LOGIC ("Sending data from Transmitted Buffer");

      NS_LOG_INFO ("VT(A)     = " << m_vtA);
      NS_LOG_INFO ("VT(S)     = " << m_vtS);

      uint16_t vta = m_vtA.GetValue ();
      Ptr<Packet> packet = m_txedBuffer.at (vta)->Copy ();
      
      if (( packet->GetSize () <= bytes )
          || m_txOpportunityForRetxAlwaysBigEnough)
        {
          NS_LOG_INFO ("Move SN = " << vta << " to retxBuffer");
          m_retxBuffer.at (vta).m_pdu = m_txedBuffer.at (vta)->Copy ();
          m_retxBuffer.at (vta).m_retxCount = 1;
          m_retxBufferSize += m_retxBuffer.at (vta).m_pdu->GetSize ();

          m_txedBufferSize -= m_txedBuffer.at (vta)->GetSize ();
          m_txedBuffer.at (vta) = 0;

          LteRlcAmHeader rlcAmHeader;
          packet->PeekHeader (rlcAmHeader);
          NS_LOG_LOGIC ("RLC header: " << rlcAmHeader);

          // Send RLC PDU to MAC layer
          LteMacSapProvider::TransmitPduParameters params;
          params.pdu = packet;
          params.rnti = m_rnti;
          params.lcid = m_lcid;
          params.layer = layer;
          params.harqProcessId = harqId;

          m_macSapProvider->TransmitPdu (params);
          return;
        }
      else
        {
          NS_LOG_LOGIC ("TxOpportunity (size = " << bytes << ") too small for retransmission of the packet (size = " << packet->GetSize () << ")");
          NS_LOG_LOGIC ("Waiting for bigger TxOpportunity");
          return;
        }
        }*/ 
  else
    {
      NS_LOG_LOGIC ("No data pending");
      return;
    }

  //
  //
  // Build new PDU
  //
  //

  Ptr<Packet> packet = Create<Packet> ();
  LteRlcAmHeader rlcAmHeader;
  rlcAmHeader.SetDataPdu ();

  // Build Data field
  uint32_t nextSegmentSize = bytes - 4;
  uint32_t nextSegmentId = 1;
  uint32_t dataFieldTotalSize = 0;
  uint32_t dataFieldAddedSize = 0;
  std::vector < Ptr<Packet> > dataField;

  // Remove the first packet from the transmission buffer.
  // If only a segment of the packet is taken, then the remaining is given back later
  if ( m_txonBuffer.size () == 0 )
    {
      NS_LOG_LOGIC ("No data pending");
      return;
    }

  NS_LOG_LOGIC ("SDUs in TxonBuffer  = " << m_txonBuffer.size ());
  NS_LOG_LOGIC ("First SDU buffer  = " << *(m_txonBuffer.begin()));
  NS_LOG_LOGIC ("First SDU size    = " << (*(m_txonBuffer.begin()))->GetSize ());
  NS_LOG_LOGIC ("Next segment size = " << nextSegmentSize);
  NS_LOG_LOGIC ("Remove SDU from TxBuffer");
  Ptr<Packet> firstSegment = (*(m_txonBuffer.begin ()))->Copy ();
  m_txonBufferSize -= (*(m_txonBuffer.begin()))->GetSize ();
  NS_LOG_LOGIC ("txBufferSize      = " << m_txonBufferSize );
  m_txonBuffer.erase (m_txonBuffer.begin ());

  while ( firstSegment && (firstSegment->GetSize () > 0) && (nextSegmentSize > 0) )
    {
      NS_LOG_LOGIC ("WHILE ( firstSegment && firstSegment->GetSize > 0 && nextSegmentSize > 0 )");
      NS_LOG_LOGIC ("    firstSegment size = " << firstSegment->GetSize ());
      NS_LOG_LOGIC ("    nextSegmentSize   = " << nextSegmentSize);
      if ( (firstSegment->GetSize () > nextSegmentSize) ||
           // Segment larger than 2047 octets can only be mapped to the end of the Data field
           (firstSegment->GetSize () > 2047)
         )
        {
          // Take the minimum size, due to the 2047-bytes 3GPP exception
          // This exception is due to the length of the LI field (just 11 bits)
          uint32_t currSegmentSize = std::min (firstSegment->GetSize (), nextSegmentSize);

          NS_LOG_LOGIC ("    IF ( firstSegment > nextSegmentSize ||");
          NS_LOG_LOGIC ("         firstSegment > 2047 )");

          // Segment txBuffer.FirstBuffer and
          // Give back the remaining segment to the transmission buffer
          Ptr<Packet> newSegment = firstSegment->CreateFragment (0, currSegmentSize);
          NS_LOG_LOGIC ("    newSegment size   = " << newSegment->GetSize ());

          // Status tag of the new and remaining segments
          // Note: This is the only place where a PDU is segmented and
          // therefore its status can change
          LteRlcSduStatusTag oldTag, newTag;
          firstSegment->RemovePacketTag (oldTag);
          newSegment->RemovePacketTag (newTag);
          if (oldTag.GetStatus () == LteRlcSduStatusTag::FULL_SDU)
            {
              newTag.SetStatus (LteRlcSduStatusTag::FIRST_SEGMENT);
              oldTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
            }
          else if (oldTag.GetStatus () == LteRlcSduStatusTag::LAST_SEGMENT)
            {
              newTag.SetStatus (LteRlcSduStatusTag::MIDDLE_SEGMENT);
              //oldTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
            }

          // Give back the remaining segment to the transmission buffer
          firstSegment->RemoveAtStart (currSegmentSize);
          NS_LOG_LOGIC ("    firstSegment size (after RemoveAtStart) = " << firstSegment->GetSize ());
          if (firstSegment->GetSize () > 0)
            {
              firstSegment->AddPacketTag (oldTag);

              m_txonBuffer.insert (m_txonBuffer.begin (), firstSegment);
              m_txonBufferSize += (*(m_txonBuffer.begin()))->GetSize ();

              NS_LOG_LOGIC ("    Txon buffer: Give back the remaining segment");
              NS_LOG_LOGIC ("    Txon buffers = " << m_txonBuffer.size ());
              NS_LOG_LOGIC ("    Front buffer size = " << (*(m_txonBuffer.begin()))->GetSize ());
              NS_LOG_LOGIC ("    txonBufferSize = " << m_txonBufferSize );
            }
          else
            {
              // Whole segment was taken, so adjust tag
              if (newTag.GetStatus () == LteRlcSduStatusTag::FIRST_SEGMENT)
                {
                  newTag.SetStatus (LteRlcSduStatusTag::FULL_SDU);
                }
              else if (newTag.GetStatus () == LteRlcSduStatusTag::MIDDLE_SEGMENT)
                {
                  newTag.SetStatus (LteRlcSduStatusTag::LAST_SEGMENT);
                }
            }
          // Segment is completely taken or
          // the remaining segment is given back to the transmission buffer
          firstSegment = 0;

          // Put status tag once it has been adjusted
          newSegment->AddPacketTag (newTag);

          // Add Segment to Data field
          dataFieldAddedSize = newSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (newSegment);
          newSegment = 0;

          // ExtensionBit (Next_Segment - 1) = 0
          rlcAmHeader.PushExtensionBit (LteRlcAmHeader::DATA_FIELD_FOLLOWS);

          // no LengthIndicator for the last one

          nextSegmentSize -= dataFieldAddedSize;
          nextSegmentId++;

          // nextSegmentSize MUST be zero (only if segment is smaller or equal to 2047)

          // (NO more segments) ? exit
          // break;
        }
      else if ( (nextSegmentSize - firstSegment->GetSize () <= 2) || (m_txonBuffer.size () == 0) )
        {
          NS_LOG_LOGIC ("    IF nextSegmentSize - firstSegment->GetSize () <= 2 || txonBuffer.size == 0");

          // Add txBuffer.FirstBuffer to DataField
          dataFieldAddedSize = firstSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (firstSegment);
          firstSegment = 0;

          // ExtensionBit (Next_Segment - 1) = 0
          rlcAmHeader.PushExtensionBit (LteRlcAmHeader::DATA_FIELD_FOLLOWS);

          // no LengthIndicator for the last one

          nextSegmentSize -= dataFieldAddedSize;
          nextSegmentId++;

          NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txonBuffer.size ());
          if (m_txonBuffer.size () > 0)
            {
              NS_LOG_LOGIC ("        First SDU buffer  = " << *(m_txonBuffer.begin()));
              NS_LOG_LOGIC ("        First SDU size    = " << (*(m_txonBuffer.begin()))->GetSize ());
            }
          NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);

          // nextSegmentSize <= 2 (only if txBuffer is not empty)

          // (NO more segments) ? exit
          // break;
        }
      else // (firstSegment->GetSize () < m_nextSegmentSize) && (m_txBuffer.size () > 0)
        {
          NS_LOG_LOGIC ("    IF firstSegment < NextSegmentSize && txonBuffer.size > 0");
          // Add txBuffer.FirstBuffer to DataField
          dataFieldAddedSize = firstSegment->GetSize ();
          dataFieldTotalSize += dataFieldAddedSize;
          dataField.push_back (firstSegment);

          // ExtensionBit (Next_Segment - 1) = 1
          rlcAmHeader.PushExtensionBit (LteRlcAmHeader::E_LI_FIELDS_FOLLOWS);

          // LengthIndicator (Next_Segment)  = txBuffer.FirstBuffer.length()
          rlcAmHeader.PushLengthIndicator (firstSegment->GetSize ());

          nextSegmentSize -= ((nextSegmentId % 2) ? (2) : (1)) + dataFieldAddedSize;
          nextSegmentId++;

          NS_LOG_LOGIC ("        SDUs in TxBuffer  = " << m_txonBuffer.size ());
          if (m_txonBuffer.size () > 0)
            {
              NS_LOG_LOGIC ("        First SDU buffer  = " << *(m_txonBuffer.begin()));
              NS_LOG_LOGIC ("        First SDU size    = " << (*(m_txonBuffer.begin()))->GetSize ());
            }
          NS_LOG_LOGIC ("        Next segment size = " << nextSegmentSize);
          NS_LOG_LOGIC ("        Remove SDU from TxBuffer");

          // (more segments)
          firstSegment = (*(m_txonBuffer.begin ()))->Copy ();
          m_txonBufferSize -= (*(m_txonBuffer.begin()))->GetSize ();
          m_txonBuffer.erase (m_txonBuffer.begin ());
          NS_LOG_LOGIC ("        txBufferSize = " << m_txonBufferSize );
        }

    }

  //
  // Build RLC header
  //

  rlcAmHeader.SetSequenceNumber ( m_vtS++ );
  rlcAmHeader.SetResegmentationFlag (LteRlcAmHeader::PDU);
  rlcAmHeader.SetLastSegmentFlag (LteRlcAmHeader::LAST_PDU_SEGMENT);
  rlcAmHeader.SetSegmentOffset (0);

  // Calculate FramingInfo flag according the status of the SDUs in the DataField
  uint8_t framingInfo = 0;
  std::vector< Ptr<Packet> >::iterator it;
  it = dataField.begin ();

  // FIRST SEGMENT
  LteRlcSduStatusTag tag;
  (*it)->RemovePacketTag (tag);
  if ( (tag.GetStatus () == LteRlcSduStatusTag::FULL_SDU) ||
       (tag.GetStatus () == LteRlcSduStatusTag::FIRST_SEGMENT)
     )
    {
      framingInfo |= LteRlcAmHeader::FIRST_BYTE;
    }
  else
    {
      framingInfo |= LteRlcAmHeader::NO_FIRST_BYTE;
    }
  (*it)->AddPacketTag (tag);

  // Add all SDUs (in DataField) to the Packet
  while (it < dataField.end ())
    {
      NS_LOG_LOGIC ("Adding SDU/segment to packet, length = " << (*it)->GetSize ());

      packet->AddAtEnd (*it);
      it++;
    }

  // LAST SEGMENT (Note: There could be only one and be the first one)
  it--;
  (*it)->RemovePacketTag (tag);
  if ( (tag.GetStatus () == LteRlcSduStatusTag::FULL_SDU) ||
        (tag.GetStatus () == LteRlcSduStatusTag::LAST_SEGMENT) )
    {
      framingInfo |= LteRlcAmHeader::LAST_BYTE;
    }
  else
    {
      framingInfo |= LteRlcAmHeader::NO_LAST_BYTE;
    }
  (*it)->AddPacketTag (tag);

  // Set the FramingInfo flag after the calculation
  rlcAmHeader.SetFramingInfo (framingInfo);


  // Calculate the Polling Bit (5.2.2.1)
  rlcAmHeader.SetPollingBit (LteRlcAmHeader::STATUS_REPORT_NOT_REQUESTED);

  m_pduWithoutPoll++;
  NS_LOG_LOGIC ("PDU_WITHOUT_POLL = " << m_pduWithoutPoll);
  m_byteWithoutPoll += packet->GetSize ();
  NS_LOG_LOGIC ("BYTE_WITHOUT_POLL = " << m_byteWithoutPoll);

  if ( (m_pduWithoutPoll >= m_pollPdu) || (m_byteWithoutPoll >= m_pollByte) ||
       ( (m_txonBuffer.empty ()) && (m_retxBufferSize == 0) ) ||
       (m_vtS >= m_vtMs)
       || m_pollRetransmitTimerJustExpired
     )
    {
      m_pollRetransmitTimerJustExpired = false;
      rlcAmHeader.SetPollingBit (LteRlcAmHeader::STATUS_REPORT_IS_REQUESTED);
      m_pduWithoutPoll = 0;
      m_byteWithoutPoll = 0;

      m_pollSn = m_vtS - 1;
      NS_LOG_LOGIC ("New POLL_SN = " << m_pollSn);

      if (! m_pollRetransmitTimer.IsRunning () )
        {
          NS_LOG_LOGIC ("Start PollRetransmit timer");

          m_pollRetransmitTimer = Simulator::Schedule (m_pollRetransmitTimerValue,
                                                       &LteRlcAm::ExpirePollRetransmitTimer, this);
        }
      else
        {
          NS_LOG_LOGIC ("Restart PollRetransmit timer");

          m_pollRetransmitTimer.Cancel ();
          m_pollRetransmitTimer = Simulator::Schedule (m_pollRetransmitTimerValue,
                                                       &LteRlcAm::ExpirePollRetransmitTimer, this);
        }
    }


  // Build RLC PDU with DataField and Header
  NS_LOG_LOGIC ("AM RLC header: " << rlcAmHeader);
  packet->AddHeader (rlcAmHeader);

  // Store new PDU into the Transmitted PDU Buffer
  NS_LOG_LOGIC ("Put transmitted PDU in the txedBuffer");
  m_txedBufferSize += packet->GetSize ();
  m_txedBuffer.at ( rlcAmHeader.GetSequenceNumber ().GetValue () ).m_pdu = packet->Copy ();
  m_txedBuffer.at ( rlcAmHeader.GetSequenceNumber ().GetValue () ).m_retxCount = 0;

  //NS_LOG_DEBUG ("Put " << packet->GetSize() << " PDU in the txedBuffer, size = " << m_txedBufferSize);
  // Sender timestamp
  RlcTag rlcTag (Simulator::Now ());
  packet->AddByteTag (rlcTag);
  m_txPdu (m_rnti, m_lcid, packet->GetSize ());

  // Send RLC PDU to MAC layer
  LteMacSapProvider::TransmitPduParameters params;
  params.pdu = packet;
  params.rnti = m_rnti;
  params.lcid = m_lcid;
  params.layer = layer;
  params.harqProcessId = harqId;

  m_macSapProvider->TransmitPdu (params);
}

void
LteRlcAm::DoNotifyHarqDeliveryFailure ()
{
  NS_LOG_FUNCTION (this);
}

//Packet: RlcPdu
//Assemble RlcPdus in txed/retxBuffer to RlcSdu.
//Note: if handover happens when RlcPdu is smaller than RlcSdu,
//the switch condition often ends up with: NO LOSS S1 11 case 
//and multiple RlcPdus is combined to create a RlcSdu (1500B).
void
LteRlcAm::Reassemble (Ptr<Packet> packet)
{
  LteRlcAmHeader rlcAmHeader;
  packet->RemoveHeader (rlcAmHeader);
  uint8_t framingInfo = rlcAmHeader.GetFramingInfo ();
	SequenceNumber10 currSeqNumber = rlcAmHeader.GetSequenceNumber ();
  bool expectedSnLost;
	NS_LOG_DEBUG("currSeqNumber = " << currSeqNumber << "expected = " << m_reassembleExpectedSeqNumber);
	//Starting state: WITH LOSS.
  if ( currSeqNumber != m_reassembleExpectedSeqNumber )
    {
      expectedSnLost = true;
      NS_LOG_LOGIC ("There are losses. Expected SN = " << m_reassembleExpectedSeqNumber  << ". Current SN = " << currSeqNumber);
      m_reassembleExpectedSeqNumber = currSeqNumber + 1;
    }
  else
    {
      expectedSnLost = false;
      NS_LOG_LOGIC ("No losses. Expected SN = " << m_reassembleExpectedSeqNumber  << ". Current SN = " << currSeqNumber);
      m_reassembleExpectedSeqNumber = m_reassembleExpectedSeqNumber + 1;
    }

  // Build list of SDUs
  uint8_t extensionBit;
  uint16_t lengthIndicator;
  do
    {
      extensionBit = rlcAmHeader.PopExtensionBit ();
      NS_LOG_DEBUG ("E = " << (uint16_t)extensionBit);

      if ( extensionBit == 0 )
        {
          m_sdusAssembleBuffer.push_back (packet);
        }
      else // extensionBit == 1
        {
          lengthIndicator = rlcAmHeader.PopLengthIndicator ();
          NS_LOG_LOGIC ("LI = " << lengthIndicator);
          NS_LOG_DEBUG ("LI = " << lengthIndicator);

          // Check if there is enough data in the packet
          if ( lengthIndicator >= packet->GetSize () )
            {
              NS_LOG_LOGIC ("INTERNAL ERROR: Not enough data in the packet (" << packet->GetSize () << "). Needed LI=" << lengthIndicator);
              NS_LOG_DEBUG ("INTERNAL ERROR: Not enough data in the packet (" << packet->GetSize () << "). Needed LI=" << lengthIndicator);
              /// \todo What to do in this case? Discard packet and continue? Or Assert?
            }

          // Split packet in two fragments
          Ptr<Packet> data_field = packet->CreateFragment (0, lengthIndicator);
          packet->RemoveAtStart (lengthIndicator);

          m_sdusAssembleBuffer.push_back (data_field);
        }
    }
  while ( extensionBit == 1 );

  std::list < Ptr<Packet> >::iterator it;
	
	//m_assemblingState = WAITING_SI_SF;
  // Current reassembling state
  if      (m_assemblingState == WAITING_S0_FULL)  NS_LOG_DEBUG ("Reassembling State = 'WAITING_S0_FULL'");
  else if (m_assemblingState == WAITING_SI_SF)    NS_LOG_DEBUG ("Reassembling State = 'WAITING_SI_SF'");
  else                                              NS_LOG_DEBUG ("Reassembling State = Unknown state");

  // Received framing Info
  NS_LOG_LOGIC ("Framing Info = " << (uint16_t)framingInfo);
  NS_LOG_LOGIC ("m_sdusAssembleBuffer = " << m_sdusAssembleBuffer.size ());
 	NS_LOG_DEBUG ("Framing Info = " << (uint16_t)framingInfo);
  NS_LOG_DEBUG ("m_sdusAssembleBuffer = " << m_sdusAssembleBuffer.size ());
 // Reassemble the list of SDUs (when there is no losses)
  if (!expectedSnLost)
    {
  		NS_LOG_DEBUG ("Reassembling NO LOSS ");
      switch (m_assemblingState)
        {
          case WAITING_S0_FULL: //Not waiting to complete a SDU.
                  switch (framingInfo)
                    {
											//RlcPdu = RlcSdu, without loss -> m_sdusAssembleBuffer contains all RLC SDUs.
                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                              m_assemblingState = WAITING_S0_FULL;

  														NS_LOG_DEBUG (" S0 00 ");
                              /**
                              * Deliver one or multiple PDUs
                              */
                              for ( it = m_sdusAssembleBuffer.begin () ; it != m_sdusAssembleBuffer.end () ; it++ )
                                {
																	NS_LOG_DEBUG ("Received Pdu Size = " << (*it)->GetSize());
                                  m_transmittingRlcSdus.push_back (*it);
																	m_transmittingRlcSduBufferSize += (*it)->GetSize ();
                                }
                              m_sdusAssembleBuffer.clear ();
                      break;
											
                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
															//Received a PDU with the last part is a portion of a SDU.
															//Switch to waiting for complete SDU state.
                              m_assemblingState = WAITING_SI_SF;
  														NS_LOG_DEBUG (" S0 01 ");

                              /**
                              * Deliver full PDUs
                              */
															//Previous recorded SDUs must be complete SDUs. The last PDU could 
															//be: (1) a part of the SDU, (2) several SDUs and a part of the last SDU.
                              while ( m_sdusAssembleBuffer.size () > 1 )
                                {
																	NS_LOG_DEBUG ("Received Pdu Size = " << m_sdusAssembleBuffer.front()->GetSize ());
                                  m_transmittingRlcSdus.push_back (m_sdusAssembleBuffer.front ());
																	m_transmittingRlcSduBufferSize += m_sdusAssembleBuffer.front()->GetSize ();
                                  m_sdusAssembleBuffer.pop_front ();
                                }

                              /**
                              * Keep S0. Keep the last incomplete SDU in the sduBuffer
                              * to wait for its ending portion.
                              */
                              m_keepS0 = m_sdusAssembleBuffer.front ();
                              m_sdusAssembleBuffer.pop_front ();
                      break;
											//This could not happen because in this state, PDU should presents complete 
											//SDU or the last part of PDU is a portion of RLC.
                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
                      default:
                              /**
                              * ERROR: Transition not possible
                              */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                              NS_LOG_DEBUG ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          case WAITING_SI_SF:
                  switch (framingInfo)
                    {
                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::LAST_BYTE): //finish the previous S0.
                              m_assemblingState = WAITING_S0_FULL;	//switch state to not waiting for S0 full.

  														NS_LOG_DEBUG (" SI 10 ");
                              /**
                              * Deliver (Kept)S0 + SN
                              */
															//Received an ending flag, if the state is waiting for the ending, 
															//then switch to complete (S0_full)
                              m_keepS0->AddAtEnd (m_sdusAssembleBuffer.front ());
                              m_sdusAssembleBuffer.pop_front ();
															NS_LOG_DEBUG ("Received Pdu Size = " << m_keepS0->GetSize ());
                              m_transmittingRlcSdus.push_back (m_keepS0);
															m_transmittingRlcSduBufferSize += m_keepS0->GetSize ();

                              /**
                                * Deliver zero, one or multiple PDUs
                                */
															//Deliver the complete SDUs after the first portion of SDU that completes
															//the above S0.
                              while ( ! m_sdusAssembleBuffer.empty () )
                                {
																	NS_LOG_DEBUG ("Received Pdu Size = " << m_sdusAssembleBuffer.front()->GetSize ());
                                  m_transmittingRlcSdus.push_back (m_sdusAssembleBuffer.front ());
																	m_transmittingRlcSduBufferSize += m_sdusAssembleBuffer.front()->GetSize ();
                                  m_sdusAssembleBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
                              m_assemblingState = WAITING_SI_SF;
  														NS_LOG_DEBUG (" SI 11 ");
											
                              /**
                              * Keep SI
                              */
                              if ( m_sdusAssembleBuffer.size () == 1 )
                                {
                                  m_keepS0->AddAtEnd (m_sdusAssembleBuffer.front ());
                                  m_sdusAssembleBuffer.pop_front ();
                                }
                              else // m_sdusAssembleBuffer.size () > 1
                                {
                                  /**
                                  * Deliver (Kept)S0 + SN
                                  */
                                  m_keepS0->AddAtEnd (m_sdusAssembleBuffer.front ());
                                  m_sdusAssembleBuffer.pop_front ();
																	NS_LOG_DEBUG ("Received Pdu Size = " << m_keepS0->GetSize ());
                                  m_transmittingRlcSdus.push_back (m_keepS0);
																	m_transmittingRlcSduBufferSize += m_keepS0->GetSize ();

                                  /**
                                  * Deliver zero, one or multiple PDUs
                                  */
                                  while ( m_sdusAssembleBuffer.size () > 1 )
                                    {
																			NS_LOG_DEBUG ("Received Pdu Size = " << m_sdusAssembleBuffer.front()->GetSize ());
                                      m_transmittingRlcSdus.push_back (m_sdusAssembleBuffer.front ());
																			m_transmittingRlcSduBufferSize += m_sdusAssembleBuffer.front()->GetSize ();
                                      m_sdusAssembleBuffer.pop_front ();
                                    }

                                  /**
                                  * Keep S0
                                  */
                                  m_keepS0 = m_sdusAssembleBuffer.front ();
                                  m_sdusAssembleBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
                      default:
                              /**
                                * ERROR: Transition not possible
                                */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                              NS_LOG_DEBUG ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          default:
                NS_LOG_LOGIC ("INTERNAL ERROR: Wrong reassembling state = " << (uint32_t) m_assemblingState);
          break;
        }
    }
  else // Reassemble the list of SDUs (when there are losses, i.e. the received SN is not the expected one)
    {
		NS_LOG_DEBUG ("Reassembling WITH LOSS");
		switch (m_assemblingState)
        {
          case WAITING_S0_FULL:
                  switch (framingInfo)
                    {
                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                              m_assemblingState = WAITING_S0_FULL;
															NS_LOG_DEBUG (" S0 00");
                              /**
                               * Deliver one or multiple PDUs
                               */
                              for ( it = m_sdusAssembleBuffer.begin () ; it != m_sdusAssembleBuffer.end () ; it++ )
                                {	
                                  m_transmittingRlcSdus.push_back (*it);
																	NS_LOG_DEBUG ("Received Pdu Size = " << (*it)->GetSize ());
																	m_transmittingRlcSduBufferSize += (*it)->GetSize ();
                                }
                              m_sdusAssembleBuffer.clear ();
                      break;

                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
                              m_assemblingState = WAITING_SI_SF;

															NS_LOG_DEBUG (" S0 01 ");
                              /**
                               * Deliver full PDUs
                               */
                              while ( m_sdusAssembleBuffer.size () > 1 )
                                {
																	NS_LOG_DEBUG ("Received Pdu Size = " << m_sdusAssembleBuffer.front()->GetSize ());
                                  m_transmittingRlcSdus.push_back (m_sdusAssembleBuffer.front ());
																	m_transmittingRlcSduBufferSize += m_sdusAssembleBuffer.front()->GetSize ();
                                  m_sdusAssembleBuffer.pop_front ();
                                }

                              /**
                               * Keep S0
                               */
                              m_keepS0 = m_sdusAssembleBuffer.front ();
                              m_sdusAssembleBuffer.pop_front ();
                      break;

                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                              m_assemblingState = WAITING_S0_FULL;
															NS_LOG_DEBUG (" S0 10");
                              /**
                               * Discard the first junk portion of SDU.
                               */
                              m_sdusAssembleBuffer.pop_front ();

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( ! m_sdusAssembleBuffer.empty () )
                                {
																	NS_LOG_DEBUG ("Received Pdu Size = " << m_sdusAssembleBuffer.front()->GetSize ());
                                  m_transmittingRlcSdus.push_back (m_sdusAssembleBuffer.front ());
																	m_transmittingRlcSduBufferSize += m_sdusAssembleBuffer.front()->GetSize ();
                                  m_sdusAssembleBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
															NS_LOG_DEBUG(" S0 11");
															
                              if ( m_sdusAssembleBuffer.size () == 1 )
                                {
                                  m_assemblingState = WAITING_S0_FULL;
                                }
                              else
                                {
                                  m_assemblingState = WAITING_SI_SF;
                                }
                              /**
                               * Discard SI or SN
                               */
                              m_sdusAssembleBuffer.pop_front ();

                              if ( m_sdusAssembleBuffer.size () > 0 )
                                {
                                  /**
                                  * Deliver zero, one or multiple PDUs
                                  */
                                  while ( m_sdusAssembleBuffer.size () > 1 )
                                    {
																			NS_LOG_DEBUG ("Received Pdu Size = " << m_sdusAssembleBuffer.front()->GetSize ());
                                      m_transmittingRlcSdus.push_back (m_sdusAssembleBuffer.front ());
																			m_transmittingRlcSduBufferSize += m_sdusAssembleBuffer.front()->GetSize ();
                                      m_sdusAssembleBuffer.pop_front ();
                                    }

                                  /**
                                  * Keep S0
                                  */
                                  m_keepS0 = m_sdusAssembleBuffer.front ();
                                  m_sdusAssembleBuffer.pop_front ();
                                }
                      break;

                      default:
                              /**
                               * ERROR: Transition not possible
                               */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                              NS_LOG_DEBUG ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          case WAITING_SI_SF:
                  switch (framingInfo)
                    {
                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                              m_assemblingState = WAITING_S0_FULL;

															NS_LOG_DEBUG (" SI 00");
                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Deliver one or multiple PDUs
                               */
                              while ( ! m_sdusAssembleBuffer.empty () )
                                {
                                  m_transmittingRlcSdus.push_back (m_sdusAssembleBuffer.front ());
																	NS_LOG_DEBUG ("Received Pdu Size = " << m_sdusAssembleBuffer.front()->GetSize ());
																	m_transmittingRlcSduBufferSize += m_sdusAssembleBuffer.front()->GetSize ();
                                  m_sdusAssembleBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
                              m_assemblingState = WAITING_SI_SF;

															NS_LOG_DEBUG (" SI 01");
                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( m_sdusAssembleBuffer.size () > 1 )
                                {
																	NS_LOG_DEBUG ("Received Pdu Size = " << m_sdusAssembleBuffer.front()->GetSize ());
                                  m_transmittingRlcSdus.push_back (m_sdusAssembleBuffer.front ());
																	m_transmittingRlcSduBufferSize += m_sdusAssembleBuffer.front()->GetSize ();
                                  m_sdusAssembleBuffer.pop_front ();
                                }

                              /**
                               * Keep S0
                               */
                              m_keepS0 = m_sdusAssembleBuffer.front ();
                              m_sdusAssembleBuffer.pop_front ();

                      break;

                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
															NS_LOG_DEBUG (" SI 10");
                              m_assemblingState = WAITING_S0_FULL;

                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Discard SI or SN
                               */
                              m_sdusAssembleBuffer.pop_front ();

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( ! m_sdusAssembleBuffer.empty () )
                                {
                                  m_transmittingRlcSdus.push_back (m_sdusAssembleBuffer.front ());
																	NS_LOG_DEBUG ("Received Pdu Size = " << m_sdusAssembleBuffer.front()->GetSize ());
																	m_transmittingRlcSduBufferSize += m_sdusAssembleBuffer.front()->GetSize ();
                                  m_sdusAssembleBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):

															NS_LOG_DEBUG(" SI 11");
                              if ( m_sdusAssembleBuffer.size () == 1 )
                                {
                                  m_assemblingState = WAITING_SI_SF;
																	m_reassembleExpectedSeqNumber = currSeqNumber - 1; //With loss + SI_SF: Jump here until sdusBuffer is larger than 1, meaning a new SDU begins.
                                }
                              else
                                {
                                  m_assemblingState = WAITING_SI_SF;
                                }

                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Discard SI or SN
                               */
                              m_sdusAssembleBuffer.pop_front ();

                              if ( m_sdusAssembleBuffer.size () > 0 )
                                {
                                  /**
                                   * Deliver zero, one or multiple PDUs
                                   */
                                  while ( m_sdusAssembleBuffer.size () > 1 )
                                    {
																			NS_LOG_DEBUG ("Received Pdu Size = " << m_sdusAssembleBuffer.front()->GetSize ());
                                      m_transmittingRlcSdus.push_back (m_sdusAssembleBuffer.front ());
																			m_transmittingRlcSduBufferSize += m_sdusAssembleBuffer.front()->GetSize ();
                                      m_sdusAssembleBuffer.pop_front ();
                                    }

                                  /**
                                   * Keep S0
                                   */
                                  m_keepS0 = m_sdusAssembleBuffer.front ();
                                  m_sdusAssembleBuffer.pop_front ();
                                }
                      break;

                      default:
                              /**
                                * ERROR: Transition not possible
                                */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                              NS_LOG_DEBUG ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          default:
                NS_LOG_LOGIC ("INTERNAL ERROR: Wrong reassembling state = " << (uint32_t) m_assemblingState);
                NS_LOG_DEBUG ("INTERNAL ERROR: Wrong reassembling state = " << (uint32_t) m_assemblingState);
          break;
        }
	}
	NS_LOG_DEBUG ("End RlcPdutoRlcSdu... transmittingRlcSduBufferSize= " << m_transmittingRlcSdus.size() << " Bytes= " << m_transmittingRlcSduBufferSize);
}

void 
LteRlcAm::CreateRlcSduBuffer(){
	NS_LOG_DEBUG (this);
	LtePdcpHeader pdcpHeader;
	for (std::vector < Ptr<Packet> >::iterator it = m_transmittingRlcSdus.begin(); it != m_transmittingRlcSdus.end(); ++it){
		(*it)->PeekHeader(pdcpHeader);
		NS_LOG_DEBUG ("SEQ = " << pdcpHeader.GetSequenceNumber());
		m_transmittingRlcSduBuffer[pdcpHeader.GetSequenceNumber()] = (*it)->Copy();
	}
}

void 
LteRlcAm::RlcPdusToRlcSdus (std::vector < RetxPdu > RlcPdus){

	NS_LOG_DEBUG (this << "in RlcPdusTo..." );
	uint16_t isGotExpectedSeqNumber = 0;
	for ( std::vector <RetxPdu>::iterator it = RlcPdus.begin(); it != RlcPdus.end (); it++)
				{
					if (it->m_pdu == 0){
						continue;
					}
					NS_LOG_DEBUG (this << "Pdu = " << it->m_pdu );
					Ptr<Packet> p = it->m_pdu->Copy();

					// Get RLC header parameters
					LteRlcAmHeader rlcAmHeader;
					p->PeekHeader (rlcAmHeader);
					NS_LOG_DEBUG("RLC header: " << rlcAmHeader);
					
					//Always enter the Reassemble function with LOSS mode.
					if (isGotExpectedSeqNumber  == 0){ 
							isGotExpectedSeqNumber = 1;
							m_reassembleExpectedSeqNumber = rlcAmHeader.GetSequenceNumber()+1; //To make the condition WITH LOSS true.
					}
					if ( rlcAmHeader.IsDataPdu () )
						{
							SequenceNumber10 seqNumber = rlcAmHeader.GetSequenceNumber ();

							if ( rlcAmHeader.GetResegmentationFlag () == LteRlcAmHeader::SEGMENT )
								{
									NS_LOG_DEBUG ("PDU segment in Txed/Retx buffer ( *absolute* SN = " << seqNumber << " )");
								}
							else if ( rlcAmHeader.GetResegmentationFlag () == LteRlcAmHeader::PDU )
								{
									NS_LOG_DEBUG ("PDU in Txed/Retx buffer ( *absolute* SN = " << seqNumber << " )");
								}
							else
								{
									NS_ASSERT_MSG (false, "Neither a PDU segment nor a PDU Txed/Retx buffer");
									return ;
								}
							Reassemble(p);
						}
					else if ( rlcAmHeader.IsControlPdu () )
						{
							NS_LOG_DEBUG ("Control AM RLC PDU in Txed/Retx buffer. Discard");
						}
					else
						{
							NS_LOG_DEBUG ("Wrong AM RLC PDU type in Txed/Retx buffer");
							return;
						}
    }
		CreateRlcSduBuffer();
}
void
LteRlcAm::DoReceivePdu (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << m_rnti << (uint32_t) m_lcid << p->GetSize ());

  // Receiver timestamp
  RlcTag rlcTag;
  Time delay;
  if (p->FindFirstMatchingByteTag (rlcTag))
    {
      delay = Simulator::Now() - rlcTag.GetSenderTimestamp ();
    }
  m_rxPdu (m_rnti, m_lcid, p->GetSize (), delay.GetNanoSeconds ());

  // Get RLC header parameters
  LteRlcAmHeader rlcAmHeader;
  p->PeekHeader (rlcAmHeader);
  NS_LOG_LOGIC ("RLC header: " << rlcAmHeader);

  if ( rlcAmHeader.IsDataPdu () )
    {

      // 5.1.3.1   Transmit operations

      // 5.1.3.1.1       General
      //
      // The transmitting side of an AM RLC entity shall prioritize transmission of RLC control PDUs
      // over RLC data PDUs. The transmitting side of an AM RLC entity shall prioritize retransmission
      // of RLC data PDUs over transmission of new AMD PDUs.
      //
      // The transmitting side of an AM RLC entity shall maintain a transmitting window according to
      // state variables VT(A) and VT(MS) as follows:
      // - a SN falls within the transmitting window if VT(A) <= SN < VT(MS);
      // - a SN falls outside of the transmitting window otherwise.
      //
      // The transmitting side of an AM RLC entity shall not deliver to lower layer any RLC data PDU
      // whose SN falls outside of the transmitting window.
      //
      // When delivering a new AMD PDU to lower layer, the transmitting side of an AM RLC entity shall:
      // - set the SN of the AMD PDU to VT(S), and then increment VT(S) by one.
      //
      // The transmitting side of an AM RLC entity can receive a positive acknowledgement (confirmation
      // of successful reception by its peer AM RLC entity) for a RLC data PDU by the following:
      // - STATUS PDU from its peer AM RLC entity.
      //
      // When receiving a positive acknowledgement for an AMD PDU with SN = VT(A), the transmitting
      // side of an AM RLC entity shall:
      // - set VT(A) equal to the SN of the AMD PDU with the smallest SN, whose SN falls within the
      //   range VT(A) <= SN <= VT(S) and for which a positive acknowledgment has not been received yet.
      // - if positive acknowledgements have been received for all AMD PDUs associated with
      //   a transmitted RLC SDU:
      // - send an indication to the upper layers of successful delivery of the RLC SDU.


      // 5.1.3.2 Receive operations
      //
      // 5.1.3.2.1 General
      //
      // The receiving side of an AM RLC entity shall maintain a receiving window according to state
      // variables VR(R) and VR(MR) as follows:
      // - a SN falls within the receiving window if VR(R) <= SN < VR(MR);
      // - a SN falls outside of the receiving window otherwise.
      //
      // When receiving a RLC data PDU from lower layer, the receiving side of an AM RLC entity shall:
      // - either discard the received RLC data PDU or place it in the reception buffer (see sub clause 5.1.3.2.2);
      // - if the received RLC data PDU was placed in the reception buffer:
      // - update state variables, reassemble and deliver RLC SDUs to upper layer and start/stop t-Reordering as needed (see sub clause 5.1.3.2.3).
      //
      // When t-Reordering expires, the receiving side of an AM RLC entity shall:
      // - update state variables and start t-Reordering as needed (see sub clause 5.1.3.2.4).


      SequenceNumber10 seqNumber = rlcAmHeader.GetSequenceNumber ();
      seqNumber.SetModulusBase (m_vrR);

      if ( rlcAmHeader.GetResegmentationFlag () == LteRlcAmHeader::SEGMENT )
        {
          NS_LOG_LOGIC ("PDU segment received ( SN = " << seqNumber << " )");
        }
      else if ( rlcAmHeader.GetResegmentationFlag () == LteRlcAmHeader::PDU )
        {
          NS_LOG_LOGIC ("PDU received ( SN = " << seqNumber << " )");
        }
      else
        {
          NS_ASSERT_MSG (false, "Neither a PDU segment nor a PDU received");
          return ;
        }

      // STATUS PDU is requested
      if ( rlcAmHeader.GetPollingBit () == LteRlcAmHeader::STATUS_REPORT_IS_REQUESTED )
        {
          m_statusPduRequested = true;
          m_statusPduBufferSize = 4;

          if (! m_statusProhibitTimer.IsRunning ())
            {
              DoReportBufferStatus ();
            }
        }

      // 5.1.3.2.2 Actions when a RLC data PDU is received from lower layer
      //
      // When a RLC data PDU is received from lower layer, where the RLC data PDU contains
      // byte segment numbers y to z of an AMD PDU with SN = x, the receiving side of an AM RLC entity shall:
      // - if x falls outside of the receiving window; or
      // - if byte segment numbers y to z of the AMD PDU with SN = x have been received before:
      //     - discard the received RLC data PDU;
      // - else:
      //     - place the received RLC data PDU in the reception buffer;
      //     - if some byte segments of the AMD PDU contained in the RLC data PDU have been received before:
      //         - discard the duplicate byte segments.

      NS_LOG_LOGIC ("VR(R)  = " << m_vrR);
      NS_LOG_LOGIC ("VR(MR) = " << m_vrMr);
      NS_LOG_LOGIC ("VR(X)  = " << m_vrX);
      NS_LOG_LOGIC ("VR(MS) = " << m_vrMs);
      NS_LOG_LOGIC ("VR(H)  = " << m_vrH);

      // - if x falls outside of the receiving window; or
      // - if byte segment numbers y to z of the AMD PDU with SN = x have been received before:
      if ( ! IsInsideReceivingWindow (seqNumber) )
        {
          NS_LOG_LOGIC ("PDU discarded");
          return;
        }
      else
        {
          NS_LOG_LOGIC ("Place PDU in the reception buffer ( SN = " << seqNumber << " )");
          m_rxonBuffer[ seqNumber.GetValue () ].m_byteSegments.push_back (p);
          m_rxonBuffer[ seqNumber.GetValue () ].m_pduComplete = true;

          // - if some byte segments of the AMD PDU contained in the RLC data PDU have been received before:
          //         - discard the duplicate byte segments.
        }

      // 5.1.3.2.3 Actions when a RLC data PDU is placed in the reception buffer
      // When a RLC data PDU with SN = x is placed in the reception buffer,
      // the receiving side of an AM RLC entity shall:

      // - if x >= VR(H)
      //     - update VR(H) to x+ 1;

      if ( seqNumber >= m_vrH )
        {
          m_vrH = seqNumber + 1;
          NS_LOG_LOGIC ("New VR(H)  = " << m_vrH);
        }

      // - if all byte segments of the AMD PDU with SN = VR(MS) are received:
      //     - update VR(MS) to the SN of the first AMD PDU with SN > current VR(MS) for
      //       which not all byte segments have been received;

      std::map <uint16_t, PduBuffer>::iterator it = m_rxonBuffer.find (m_vrMs.GetValue ());
      if ( it != m_rxonBuffer.end () &&
           m_rxonBuffer[ m_vrMs.GetValue () ].m_pduComplete )
        {
          int firstVrMs = m_vrMs.GetValue ();
          while ( it != m_rxonBuffer.end () &&
                  m_rxonBuffer[ m_vrMs.GetValue () ].m_pduComplete )
            {
              m_vrMs++;
              it = m_rxonBuffer.find (m_vrMs.GetValue ());
              NS_LOG_LOGIC ("Incr VR(MS) = " << m_vrMs);

              NS_ASSERT_MSG (firstVrMs != m_vrMs.GetValue (), "Infinite loop in RxonBuffer");
            }
          NS_LOG_LOGIC ("New VR(MS) = " << m_vrMs);
        }

      // - if x = VR(R):
      //     - if all byte segments of the AMD PDU with SN = VR(R) are received:
      //         - update VR(R) to the SN of the first AMD PDU with SN > current VR(R) for which not all byte segments have been received;
      //         - update VR(MR) to the updated VR(R) + AM_Window_Size;
      //     - reassemble RLC SDUs from any byte segments of AMD PDUs with SN that falls outside of the receiving window and in-sequence byte segments of the AMD PDU with SN = VR(R), remove RLC headers when doing so and deliver the reassembled RLC SDUs to upper layer in sequence if not delivered before;

      if ( seqNumber == m_vrR )
        {
          std::map <uint16_t, PduBuffer>::iterator it = m_rxonBuffer.find (seqNumber.GetValue ());
          if ( it != m_rxonBuffer.end () &&
               m_rxonBuffer[ seqNumber.GetValue () ].m_pduComplete )
            {
              it = m_rxonBuffer.find (m_vrR.GetValue ());
              int firstVrR = m_vrR.GetValue ();
              while ( it != m_rxonBuffer.end () &&
                      m_rxonBuffer[ m_vrR.GetValue () ].m_pduComplete )
                {
                  NS_LOG_LOGIC ("Reassemble and Deliver ( SN = " << m_vrR << " )");
                  NS_ASSERT_MSG (m_rxonBuffer[ m_vrR.GetValue () ].m_byteSegments.size () == 1,
                                "Too many segments. PDU Reassembly process didn't work");
                  ReassembleAndDeliver (m_rxonBuffer[ m_vrR.GetValue () ].m_byteSegments.front ());
                  m_rxonBuffer.erase (m_vrR.GetValue ());

                  m_vrR++;
                  m_vrR.SetModulusBase (m_vrR);
                  m_vrX.SetModulusBase (m_vrR);
                  m_vrMs.SetModulusBase (m_vrR);
                  m_vrH.SetModulusBase (m_vrR);
                  it = m_rxonBuffer.find (m_vrR.GetValue ());

                  NS_ASSERT_MSG (firstVrR != m_vrR.GetValue (), "Infinite loop in RxonBuffer");
                }
              NS_LOG_LOGIC ("New VR(R)  = " << m_vrR);
              m_vrMr = m_vrR + m_windowSize;

              NS_LOG_LOGIC ("New VR(MR) = " << m_vrMr);
            }

//           NS_LOG_LOGIC ("Reassemble and Deliver ( SN = " << seqNumber << " )");
//           NS_ASSERT_MSG (m_rxonBuffer[ seqNumber.GetValue () ].m_byteSegments.size () == 1,
//                          "Too many segments. PDU Reassembly process didn't work");
//           ReassembleAndDeliver (m_rxonBuffer[ seqNumber.GetValue () ].m_byteSegments.front ());
//           m_rxonBuffer.erase (seqNumber.GetValue ());
        }

      // - if t-Reordering is running:
      //     - if VR(X) = VR(R); or
      //     - if VR(X) falls outside of the receiving window and VR(X) is not equal to VR(MR):
      //         - stop and reset t-Reordering;

      if ( m_reorderingTimer.IsRunning () )
        {
          NS_LOG_LOGIC ("Reordering timer is running");
          if ( (m_vrX == m_vrR) ||
               ( (! IsInsideReceivingWindow (m_vrX)) && (m_vrX != m_vrMr) )
             )
            {
              /// \todo stop and reset the t-Reordering
              NS_LOG_LOGIC ("Stop reordering timer");
              m_reorderingTimer.Cancel ();
            }
        }

      // - if t-Reordering is not running (includes the case t-Reordering is stopped due to actions above):
      //     - if VR (H) > VR(R):
      //         - start t-Reordering;
      //         - set VR(X) to VR(H).

      if ( ! m_reorderingTimer.IsRunning () )
        {
          NS_LOG_LOGIC ("Reordering timer is not running");
          if ( m_vrH > m_vrR )
            {
              NS_LOG_LOGIC ("Start reordering timer");
              m_reorderingTimer = Simulator::Schedule (m_reorderingTimerValue,
                                                       &LteRlcAm::ExpireReorderingTimer ,this);
              m_vrX = m_vrH;
              NS_LOG_LOGIC ("New VR(X) = " << m_vrX);
            }
        }



      /// \todo To remove

      // 5.1.2.2.3 Actions when an UMD PDU is placed in the reception buffer
      // When an UMD PDU with SN = x is placed in the reception buffer, the receiving UM RLC entity shall:

      // - if x falls outside of the reordering window:
      //    - update VR(UH) to x + 1;
      //    - reassemble RLC SDUs from any UMD PDUs with SN that falls outside of the reordering window, remove
      //      RLC headers when doing so and deliver the reassembled RLC SDUs to upper layer in ascending order of the
      //      RLC SN if not delivered before;
      //    - if VR(UR) falls outside of the reordering window:
      //        - set VR(UR) to (VR(UH) - UM_Window_Size);

//       if ( ! IsInsideReorderingWindow (seqNumber))
//         {
//           NS_LOG_LOGIC ("SN outside the reordering window");
// 
//           m_vrUh = seqNumber + 1;
//           NS_LOG_LOGIC ("New VR(UH) = " << m_vrUh);
// 
//           ReassembleOutsideWindow ();
// 
//           if ( ! IsInsideReorderingWindow (m_vrUr) )
//             {
//               m_vrUr = m_vrUh - m_windowSize;
//               NS_LOG_LOGIC ("VR(UR) outside the reordering window");
//               NS_LOG_LOGIC ("New VR(UR) = " << m_vrUr);
//             }
//         }

      // - if the reception buffer contains an UMD PDU with SN = VR(UR):
      //    - update VR(UR) to the SN of the first UMD PDU with SN > current VR(UR) that has not been received;
      //    - reassemble RLC SDUs from any UMD PDUs with SN < updated VR(UR), remove RLC headers when doing
      //      so and deliver the reassembled RLC SDUs to upper layer in ascending order of the RLC SN if not delivered
      //      before;

//       if ( m_rxBuffer.count (m_vrUr) > 0 )
//         {
//           NS_LOG_LOGIC ("Reception buffer contains SN = " << m_vrUr);
// 
//           std::map <uint16_t, Ptr<Packet> >::iterator it;
//           uint16_t newVrUr;
// 
//           it = m_rxBuffer.find (m_vrUr);
//           newVrUr = (it->first) + 1;
//           while ( m_rxBuffer.count (newVrUr) > 0 )
//             {
//               newVrUr++;
//             }
//           m_vrUr = newVrUr;
//           NS_LOG_LOGIC ("New VR(UR) = " << m_vrUr);
// 
//           ReassembleSnLessThan (m_vrUr);
//         }

      // - if t-Reordering is running:
      //    - if VR(UX) <= VR(UR); or
      //    - if VR(UX) falls outside of the reordering window and VR(UX) is not equal to VR(UH)::
      //        - stop and reset t-Reordering;
//       if ( m_reorderingTimer.IsRunning () )
//         {
//           NS_LOG_LOGIC ("Reordering timer is running");
// 
//           if ( (m_vrUx <= m_vrUr) ||
//               ((! IsInsideReorderingWindow (m_vrUx)) && (m_vrUx != m_vrUh)) )
//             {
//               NS_LOG_LOGIC ("Stop reordering timer");
//               m_reorderingTimer.Cancel ();
//             }
//         }

      // - if t-Reordering is not running (includes the case when t-Reordering is stopped due to actions above):
      //    - if VR(UH) > VR(UR):
      //        - start t-Reordering;
      //        - set VR(UX) to VR(UH).
//       if ( ! m_reorderingTimer.IsRunning () )
//         {
//           NS_LOG_LOGIC ("Reordering timer is not running");
// 
//           if ( m_vrUx > m_vrUr )
//             {
//               NS_LOG_LOGIC ("VR(UX) > VR(UR). " << m_vrUx << " > " << m_vrUr);
//               NS_LOG_LOGIC ("Start reordering timer");
//               m_reorderingTimer = Simulator::Schedule (m_reorderingTimerValue),
//                                                       &LteRlcAm::ExpireReorderingTimer ,this);
//               m_vrUx = m_vrUh;
//               NS_LOG_LOGIC ("New VR(UX) = " << m_vrUx);
//             }
//         }

    }
  else if ( rlcAmHeader.IsControlPdu () )
    {
      NS_LOG_INFO ("Control AM RLC PDU");

      SequenceNumber10 ackSn = rlcAmHeader.GetAckSn ();
      SequenceNumber10 sn;

      NS_LOG_INFO ("ackSn     = " << ackSn);
      NS_LOG_INFO ("VT(A)     = " << m_vtA);
      NS_LOG_INFO ("VT(S)     = " << m_vtS);
      NS_LOG_LOGIC ("retxBufferSize = " << m_retxBufferSize);
      NS_LOG_LOGIC ("txedBufferSize = " << m_txedBufferSize);

      m_vtA.SetModulusBase (m_vtA);
      m_vtS.SetModulusBase (m_vtA);
      ackSn.SetModulusBase (m_vtA);
      sn.SetModulusBase (m_vtA);

      bool incrementVtA = true; 

      for (sn = m_vtA; sn < ackSn && sn < m_vtS; sn++)
        {
          NS_LOG_LOGIC ("sn = " << sn);

          uint16_t seqNumberValue = sn.GetValue ();

          if (m_pollRetransmitTimer.IsRunning () 
              && (seqNumberValue == m_pollSn.GetValue ()))
            {
              m_pollRetransmitTimer.Cancel ();
            }

          if (rlcAmHeader.IsNackPresent (sn))
            {
              NS_LOG_LOGIC ("sn " << sn << " is NACKed");

              incrementVtA = false;

              if (m_txedBuffer.at (seqNumberValue).m_pdu != 0)
                {
                  NS_LOG_INFO ("Move SN = " << seqNumberValue << " to retxBuffer");
                  m_retxBuffer.at (seqNumberValue).m_pdu = m_txedBuffer.at (seqNumberValue).m_pdu->Copy ();
                  m_retxBuffer.at (seqNumberValue).m_retxCount = m_txedBuffer.at (seqNumberValue).m_retxCount;
                  m_retxBufferSize += m_retxBuffer.at (seqNumberValue).m_pdu->GetSize ();

                  m_txedBufferSize -= m_txedBuffer.at (seqNumberValue).m_pdu->GetSize ();
                  m_txedBuffer.at (seqNumberValue).m_pdu = 0;
                  m_txedBuffer.at (seqNumberValue).m_retxCount = 0;
                }

              NS_ASSERT (m_retxBuffer.at (seqNumberValue).m_pdu != 0);
              
            }
          else
            {
              NS_LOG_LOGIC ("sn " << sn << " is ACKed");

              if (m_txedBuffer.at (seqNumberValue).m_pdu)
                {
                  NS_LOG_INFO ("ACKed SN = " << seqNumberValue << " from txedBuffer");
                  //               NS_LOG_INFO ("m_txedBuffer( " << m_vtA << " )->GetSize = " << m_txedBuffer.at (m_vtA.GetValue ())->GetSize ());
                  m_txedBufferSize -= m_txedBuffer.at (seqNumberValue).m_pdu->GetSize ();
                  m_txedBuffer.at (seqNumberValue).m_pdu = 0;
                  NS_ASSERT (m_retxBuffer.at (seqNumberValue).m_pdu == 0);
									/*
									for (std::vector< RetxPdu >::iterator it = m_txedBuffer.begin(); it != m_txedBuffer.end(); ++it){
											if (it->m_pdu != 0){
												NS_LOG_DEBUG (it->m_pdu);
											}
									}
									*/
                }

              if (m_retxBuffer.at (seqNumberValue).m_pdu)
                {
                  NS_LOG_INFO ("ACKed SN = " << seqNumberValue << " from retxBuffer");
                  m_retxBufferSize -= m_retxBuffer.at (seqNumberValue).m_pdu->GetSize ();
                  m_retxBuffer.at (seqNumberValue).m_pdu = 0;
                  m_retxBuffer.at (seqNumberValue).m_retxCount = 0;
  								NS_LOG_DEBUG ("ACKs received, retxBuffer size = " << m_retxBufferSize);
                }

            }

          NS_LOG_LOGIC ("retxBufferSize = " << m_retxBufferSize);
          NS_LOG_LOGIC ("txedBufferSize = " << m_txedBufferSize);      

          if (incrementVtA)
            {
              m_vtA++;
              NS_LOG_INFO ("New VT(A) = " << m_vtA);
              m_vtA.SetModulusBase (m_vtA);
              m_vtS.SetModulusBase (m_vtA);
              ackSn.SetModulusBase (m_vtA);
              sn.SetModulusBase (m_vtA);
            }
          
        } // loop over SN : VT(A) <= SN < ACK SN
      
      return;

    }
  else
    {
      NS_LOG_WARN ("Wrong AM RLC PDU type");
      return;
    }

}


bool
LteRlcAm::IsInsideReceivingWindow (SequenceNumber10 seqNumber)
{
  NS_LOG_FUNCTION (this << seqNumber);
  NS_LOG_LOGIC ("Receiving Window: " <<
                m_vrR << " <= " << seqNumber << " <= " << m_vrMr);

  m_vrR.SetModulusBase (m_vrR);
  m_vrMr.SetModulusBase (m_vrR);
  seqNumber.SetModulusBase (m_vrR);

  if ( (m_vrR <= seqNumber) && (seqNumber < m_vrMr ) )
    {
      NS_LOG_LOGIC (seqNumber << " is INSIDE the receiving window");
      return true;
    }
  else
    {
      NS_LOG_LOGIC (seqNumber << " is OUTSIDE the receiving window");
      return false;
    }
}


void
LteRlcAm::ReassembleAndDeliver (Ptr<Packet> packet)
{
  LteRlcAmHeader rlcAmHeader;
  packet->RemoveHeader (rlcAmHeader);
  uint8_t framingInfo = rlcAmHeader.GetFramingInfo ();
  SequenceNumber10 currSeqNumber = rlcAmHeader.GetSequenceNumber ();
  bool expectedSnLost;

  if ( currSeqNumber != m_expectedSeqNumber )
    {
      expectedSnLost = true;
      NS_LOG_LOGIC ("There are losses. Expected SN = " << m_expectedSeqNumber << ". Current SN = " << currSeqNumber);
      m_expectedSeqNumber = currSeqNumber + 1;
    }
  else
    {
      expectedSnLost = false;
      NS_LOG_LOGIC ("No losses. Expected SN = " << m_expectedSeqNumber << ". Current SN = " << currSeqNumber);
      m_expectedSeqNumber = m_expectedSeqNumber + 1;
    }

  // Build list of SDUs
  uint8_t extensionBit;
  uint16_t lengthIndicator;
  do
    {
      extensionBit = rlcAmHeader.PopExtensionBit ();
      NS_LOG_LOGIC ("E = " << (uint16_t)extensionBit);
      //NS_LOG_DEBUG ("E = " << (uint16_t)extensionBit);

      if ( extensionBit == 0 )
        {
          m_sdusBuffer.push_back (packet);
        }
      else // extensionBit == 1
        {
          lengthIndicator = rlcAmHeader.PopLengthIndicator ();
          NS_LOG_LOGIC ("LI = " << lengthIndicator);
          //NS_LOG_DEBUG ("LI = " << lengthIndicator);

          // Check if there is enough data in the packet
          if ( lengthIndicator >= packet->GetSize () )
            {
              NS_LOG_LOGIC ("INTERNAL ERROR: Not enough data in the packet (" << packet->GetSize () << "). Needed LI=" << lengthIndicator);
              //NS_LOG_DEBUG ("INTERNAL ERROR: Not enough data in the packet (" << packet->GetSize () << "). Needed LI=" << lengthIndicator);
              /// \todo What to do in this case? Discard packet and continue? Or Assert?
            }

          // Split packet in two fragments
          Ptr<Packet> data_field = packet->CreateFragment (0, lengthIndicator);
          packet->RemoveAtStart (lengthIndicator);

          m_sdusBuffer.push_back (data_field);
        }
    }
  while ( extensionBit == 1 );

  std::list < Ptr<Packet> >::iterator it;

  // Current reassembling state
  if      (m_reassemblingState == WAITING_S0_FULL)  NS_LOG_LOGIC ("Reassembling State = 'WAITING_S0_FULL'");
  else if (m_reassemblingState == WAITING_SI_SF)    NS_LOG_LOGIC ("Reassembling State = 'WAITING_SI_SF'");
  else                                              NS_LOG_LOGIC ("Reassembling State = Unknown state");

  // Received framing Info
  NS_LOG_LOGIC ("Framing Info = " << (uint16_t)framingInfo);
  NS_LOG_LOGIC ("m_sdusBuffer = " << m_sdusBuffer.size ());

	//NS_LOG_DEBUG ("Framing Info = " << (uint16_t)framingInfo);
  //NS_LOG_DEBUG ("m_sdusBuffer = " << m_sdusBuffer.size ());


  // Reassemble the list of SDUs (when there is no losses)
  if (!expectedSnLost)
    {
  		NS_LOG_DEBUG ("NO LOSS ");
      switch (m_reassemblingState)
        {
          case WAITING_S0_FULL: //next pdu should complete the S0 SDU.
                  switch (framingInfo)
                    {
                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

  														NS_LOG_DEBUG (" S0 00 ");
                              /**
                              * Deliver one or multiple PDUs
                              */
                              for ( it = m_sdusBuffer.begin () ; it != m_sdusBuffer.end () ; it++ )
                                {
																	NS_LOG_DEBUG ("Received Pdu");
                                  m_rlcSapUser->ReceivePdcpPdu (*it);
                                }
                              m_sdusBuffer.clear ();
                      break;

                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;
  														NS_LOG_DEBUG (" S0 01 ");

                              /**
                              * Deliver full PDUs
                              */
                              while ( m_sdusBuffer.size () > 1 )
                                {
																	NS_LOG_DEBUG ("Received Pdu");
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }

                              /**
                              * Keep S0. Keep the last incomplete SDU in the sduBuffer
                              * to wait for its ending portion.
                              */
                              m_keepS0 = m_sdusBuffer.front ();
                              m_sdusBuffer.pop_front ();
                      break;

                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
                      default:
                              /**
                              * ERROR: Transition not possible
                              */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                              NS_LOG_DEBUG ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          case WAITING_SI_SF:
                  switch (framingInfo)
                    {
                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::LAST_BYTE): //finish the previous S0.
                              m_reassemblingState = WAITING_S0_FULL;	//switch state to not waiting for S0 full.

  														NS_LOG_DEBUG (" SI 10 ");
                              /**
                              * Deliver (Kept)S0 + SN
                              */
                              m_keepS0->AddAtEnd (m_sdusBuffer.front ());
                              m_sdusBuffer.pop_front ();
															NS_LOG_DEBUG ("Received Pdu");
                              m_rlcSapUser->ReceivePdcpPdu (m_keepS0);

                              /**
                                * Deliver zero, one or multiple PDUs
                                */
                              while ( ! m_sdusBuffer.empty () )
                                {
																	NS_LOG_DEBUG ("Received Pdu");
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;
  														NS_LOG_DEBUG (" SI 11 ");
											
                              /**
                              * Keep SI
                              */
                              if ( m_sdusBuffer.size () == 1 )
                                {
                                  m_keepS0->AddAtEnd (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                              else // m_sdusBuffer.size () > 1
                                {
                                  /**
                                  * Deliver (Kept)S0 + SN
                                  */
                                  m_keepS0->AddAtEnd (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
																	NS_LOG_DEBUG ("Received Pdu");
                                  m_rlcSapUser->ReceivePdcpPdu (m_keepS0);

                                  /**
                                  * Deliver zero, one or multiple PDUs
                                  */
                                  while ( m_sdusBuffer.size () > 1 )
                                    {
																			NS_LOG_DEBUG ("Received Pdu");
                                      m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                      m_sdusBuffer.pop_front ();
                                    }

                                  /**
                                  * Keep S0
                                  */
                                  m_keepS0 = m_sdusBuffer.front ();
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
                      default:
                              /**
                                * ERROR: Transition not possible
                                */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                              NS_LOG_DEBUG ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          default:
                NS_LOG_LOGIC ("INTERNAL ERROR: Wrong reassembling state = " << (uint32_t) m_reassemblingState);
          break;
        }
    }
  else // Reassemble the list of SDUs (when there are losses, i.e. the received SN is not the expected one)
    {

      NS_LOG_DEBUG (" WITH LOSS ... ");
      switch (m_reassemblingState)
        {
          case WAITING_S0_FULL:
                  switch (framingInfo)
                    {
                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;
															NS_LOG_DEBUG (" S0 00");
                              /**
                               * Deliver one or multiple PDUs
                               */
                              for ( it = m_sdusBuffer.begin () ; it != m_sdusBuffer.end () ; it++ )
                                {
																	NS_LOG_DEBUG ("Received Pdu");
                                  m_rlcSapUser->ReceivePdcpPdu (*it);
                                }
                              m_sdusBuffer.clear ();
                      break;

                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;

															NS_LOG_DEBUG (" S0 01");
                              /**
                               * Deliver full PDUs
                               */
                              while ( m_sdusBuffer.size () > 1 )
                                {
																	NS_LOG_DEBUG ("Received Pdu");
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }

                              /**
                               * Keep S0
                               */
                              m_keepS0 = m_sdusBuffer.front ();
                              m_sdusBuffer.pop_front ();
                      break;

                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

															NS_LOG_DEBUG (" S0 10");
                              /**
                               * Discard SN
                               */
                              m_sdusBuffer.pop_front ();

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( ! m_sdusBuffer.empty () )
                                {
																	NS_LOG_DEBUG ("Received Pdu");
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
															NS_LOG_DEBUG (" S0 11");
                              if ( m_sdusBuffer.size () == 1 )
                                {
                                  m_reassemblingState = WAITING_S0_FULL;
                                }
                              else
                                {
                                  m_reassemblingState = WAITING_SI_SF;
                                }
                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              if ( m_sdusBuffer.size () > 0 )
                                {
                                  /**
                                  * Deliver zero, one or multiple PDUs
                                  */
                                  while ( m_sdusBuffer.size () > 1 )
                                    {
																			NS_LOG_DEBUG ("Received Pdu");
                                      m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                      m_sdusBuffer.pop_front ();
                                    }

                                  /**
                                  * Keep S0
                                  */
                                  m_keepS0 = m_sdusBuffer.front ();
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      default:
                              /**
                               * ERROR: Transition not possible
                               */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                              NS_LOG_DEBUG ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          case WAITING_SI_SF:
                  switch (framingInfo)
                    {
                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;
															NS_LOG_DEBUG(" SI 00");
                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Deliver one or multiple PDUs
                               */
                              while ( ! m_sdusBuffer.empty () )
                                {
																	NS_LOG_DEBUG ("Received Pdu");
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcAmHeader::FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):
                              m_reassemblingState = WAITING_SI_SF;

															NS_LOG_DEBUG(" SI 01");
                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( m_sdusBuffer.size () > 1 )
                                {
																	NS_LOG_DEBUG ("Received Pdu");
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }

                              /**
                               * Keep S0
                               */
                              m_keepS0 = m_sdusBuffer.front ();
                              m_sdusBuffer.pop_front ();

                      break;

                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::LAST_BYTE):
                              m_reassemblingState = WAITING_S0_FULL;

															NS_LOG_DEBUG(" SI 10");
                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              /**
                               * Deliver zero, one or multiple PDUs
                               */
                              while ( ! m_sdusBuffer.empty () )
                                {
																	NS_LOG_DEBUG ("Received Pdu");
                                  m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      case (LteRlcAmHeader::NO_FIRST_BYTE | LteRlcAmHeader::NO_LAST_BYTE):

															NS_LOG_DEBUG(" SI 11");
                              
                              if ( m_sdusBuffer.size () == 1 )
                                {
                                  m_reassemblingState = WAITING_S0_FULL;
                                }
                              else
                                {
                                  m_reassemblingState = WAITING_SI_SF;
                                }

                              /**
                               * Discard S0
                               */
                              m_keepS0 = 0;

                              /**
                               * Discard SI or SN
                               */
                              m_sdusBuffer.pop_front ();

                              if ( m_sdusBuffer.size () > 0 )
                                {
                                  /**
                                   * Deliver zero, one or multiple PDUs
                                   */
                                  while ( m_sdusBuffer.size () > 1 )
                                    {
																			NS_LOG_DEBUG ("Received Pdu");
                                      m_rlcSapUser->ReceivePdcpPdu (m_sdusBuffer.front ());
                                      m_sdusBuffer.pop_front ();
                                    }

                                  /**
                                   * Keep S0
                                   */
                                  m_keepS0 = m_sdusBuffer.front ();
                                  m_sdusBuffer.pop_front ();
                                }
                      break;

                      default:
                              /**
                                * ERROR: Transition not possible
                                */
                              NS_LOG_LOGIC ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                              NS_LOG_DEBUG ("INTERNAL ERROR: Transition not possible. FI = " << (uint32_t) framingInfo);
                      break;
                    }
          break;

          default:
                NS_LOG_LOGIC ("INTERNAL ERROR: Wrong reassembling state = " << (uint32_t) m_reassemblingState);
          break;
        }
    }

}



void
LteRlcAm::DoReportBufferStatus (void)
{
  NS_LOG_FUNCTION (this);

  Time now = Simulator::Now ();

  NS_LOG_LOGIC ("txonBufferSize = " << m_txonBufferSize);
  NS_LOG_LOGIC ("retxBufferSize = " << m_retxBufferSize);
  NS_LOG_LOGIC ("txedBufferSize = " << m_txedBufferSize);
  NS_LOG_LOGIC ("VT(A) = " << m_vtA);
  NS_LOG_LOGIC ("VT(S) = " << m_vtS);

  // Transmission Queue HOL time
  Time txonQueueHolDelay (0);
  if ( m_txonBufferSize > 0 )
    {
      RlcTag txonQueueHolTimeTag;
      m_txonBuffer.front ()->PeekPacketTag (txonQueueHolTimeTag);
      txonQueueHolDelay = now - txonQueueHolTimeTag.GetSenderTimestamp ();
    }

  // Retransmission Queue HOL time
  Time retxQueueHolDelay;
  RlcTag retxQueueHolTimeTag;
  if ( m_retxBufferSize > 0 )
    {
      if (m_retxBuffer.at (m_vtA.GetValue ()).m_pdu != 0)
        {
          m_retxBuffer.at (m_vtA.GetValue ()).m_pdu->PeekPacketTag (retxQueueHolTimeTag);
        }
      else
        {
          m_txedBuffer.at (m_vtA.GetValue ()).m_pdu->PeekPacketTag (retxQueueHolTimeTag);
        }      
      retxQueueHolDelay = now - retxQueueHolTimeTag.GetSenderTimestamp ();
    }
  else 
    {      
      retxQueueHolDelay = Seconds (0);
    }

  LteMacSapProvider::ReportBufferStatusParameters r;
  r.rnti = m_rnti;
  r.lcid = m_lcid;
  r.txQueueSize = m_txonBufferSize;
  r.txQueueHolDelay = txonQueueHolDelay.GetMilliSeconds ();
  r.retxQueueSize = m_retxBufferSize + m_txedBufferSize;
  r.retxQueueHolDelay = retxQueueHolDelay.GetMilliSeconds ();

  if ( m_statusPduRequested && ! m_statusProhibitTimer.IsRunning () )
    {
      r.statusPduSize = m_statusPduBufferSize;
    }
  else
    {
      r.statusPduSize = 0;
    }

  if ( r.txQueueSize != 0 || r.retxQueueSize != 0 || r.statusPduSize != 0 )
    {
      NS_LOG_INFO ("Send ReportBufferStatus: " << r.txQueueSize << ", " << r.txQueueHolDelay << ", " 
                                               << r.retxQueueSize << ", " << r.retxQueueHolDelay << ", " 
                                               << r.statusPduSize);
      m_macSapProvider->ReportBufferStatus (r);
    }
  else
    {
      NS_LOG_INFO ("ReportBufferStatus don't needed");
    }
}


void
LteRlcAm::ExpireReorderingTimer (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Reordering Timer has expired");

  // 5.1.3.2.4 Actions when t-Reordering expires
  // When t-Reordering expires, the receiving side of an AM RLC entity shall:
  // - update VR(MS) to the SN of the first AMD PDU with SN >= VR(X) for which not all byte segments
  //   have been received;
  // - if VR(H) > VR(MS):
  //    - start t-Reordering;
  //    - set VR(X) to VR(H).

  m_vrMs = m_vrX;
  int firstVrMs = m_vrMs.GetValue ();
  std::map <uint16_t, PduBuffer>::iterator it = m_rxonBuffer.find (m_vrMs.GetValue ());
  while ( it != m_rxonBuffer.end () &&
          m_rxonBuffer[ m_vrMs.GetValue () ].m_pduComplete )
    {
      m_vrMs++;
      it = m_rxonBuffer.find (m_vrMs.GetValue ());

      NS_ASSERT_MSG (firstVrMs != m_vrMs.GetValue (), "Infinite loop in ExpireReorderingTimer");
    }
  NS_LOG_LOGIC ("New VR(MS) = " << m_vrMs);

  if ( m_vrH > m_vrMs )
    {
      NS_LOG_LOGIC ("Start reordering timer");
      m_reorderingTimer = Simulator::Schedule (m_reorderingTimerValue,
                                              &LteRlcAm::ExpireReorderingTimer ,this);
      m_vrX = m_vrH;
      NS_LOG_LOGIC ("New VR(MS) = " << m_vrMs);
    }

    // Section 5.2.3 Status Reporting:
    //   - The receiving side of an AM RLC entity shall trigger a
    //     STATUS report when T_reordering expires. 
  m_statusPduRequested = true;
}

void
LteRlcAm::ExpirePollRetransmitTimer (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("PollRetransmit Timer has expired");

  NS_LOG_LOGIC ("txonBufferSize = " << m_txonBufferSize);
  NS_LOG_LOGIC ("retxBufferSize = " << m_retxBufferSize);
  NS_LOG_LOGIC ("txedBufferSize = " << m_txedBufferSize);
  NS_LOG_LOGIC ("statusPduRequested = " << m_statusPduRequested);

  m_pollRetransmitTimerJustExpired = true;

  if ( m_txonBufferSize == 0 && m_retxBufferSize == 0 )
    {
      NS_LOG_INFO ("txonBuffer and retxBuffer empty. Move PDUs up to = " << m_vtS.GetValue () - 1 << " to retxBuffer");
      uint16_t sn = 0;
      for ( sn = m_vtA.GetValue(); sn < m_vtS.GetValue (); sn++ )
        {
          bool pduAvailable = m_txedBuffer.at (sn).m_pdu != 0;

           if ( pduAvailable )
             {
               NS_LOG_INFO ("Move PDU " << sn << " from txedBuffer to retxBuffer");
               m_retxBuffer.at (sn).m_pdu = m_txedBuffer.at (sn).m_pdu->Copy ();
               m_retxBuffer.at (sn).m_retxCount = m_txedBuffer.at (sn).m_retxCount;
               m_retxBufferSize += m_retxBuffer.at (sn).m_pdu->GetSize ();

               m_txedBufferSize -= m_txedBuffer.at (sn).m_pdu->GetSize ();
               m_txedBuffer.at (sn).m_pdu = 0;
               m_txedBuffer.at (sn).m_retxCount = 0;
             }
        }
    }

  DoReportBufferStatus ();  
}


void 
LteRlcAm::ExpireStatusProhibitTimer (void)
{
  NS_LOG_FUNCTION (this);
}

void
LteRlcAm::ExpireRbsTimer (void)
{
  NS_LOG_LOGIC ("RBS Timer expires");

  if (m_txonBufferSize + m_txedBufferSize + m_retxBufferSize > 0)
    {
      DoReportBufferStatus ();
      m_rbsTimer = Simulator::Schedule (m_rbsTimerValue, &LteRlcAm::ExpireRbsTimer, this);
    }
}

} // namespace ns3

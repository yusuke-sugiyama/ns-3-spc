/* -*- Mode:C++; -*- */
/*
 * Copyright (c) 2014 Yusuke Sugiyama
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
 * Foundation, Inc., Saruwatari Lab, Shizuoka University, Japan
 *
 * Author: Yusuke Sugiyama <sugiyama@aurum.cs.inf.shizuoka.ac.jp>
 */

#include "ns3/core-module.h"
#include "ns3/llc-snap-header.h"
#include "ns3/object.h"
#include "ns3/log.h"
#include "spc-mac-header.h"
#include "spc-mac-trailer.h"
#include "spc-mac.h"
#include "ns3/math.h"

NS_LOG_COMPONENT_DEFINE ("SpcMac");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SpcMac);

class PhySpcMacListener : public ns3::SpcPhyListener
{
public:
  /**
   * Create a PhyMacLowListener for the given SpcMac.
   *
   * \param spcMac
   */
  PhySpcMacListener (ns3::SpcMac *spcMac)
    : m_spcMac (spcMac)
  {
  }
  virtual ~PhySpcMacListener ()
  {
  }
  virtual void NotifyMaybeCcaBusyStart (Time duration)
  {
    m_spcMac->NotifyMaybeCcaBusyStartNow (duration);
  }
  virtual void NotifyTxStart (Time duration)
  {
    m_spcMac->NotifyTxStartNow (duration);
  }
  virtual void NotifyRxStart (Time duration)
  {
    m_spcMac->NotifyRxStartNow (duration);
  }
  virtual void NotifyRxEndOk (Ptr<Packet> packet, double rssi, uint8_t spcNum)
  {
    m_spcMac->ReceiveOk (packet, rssi, spcNum);
  }
  virtual void NotifyRxEndError (Ptr<Packet> packet)
  {
    m_spcMac->ReceiveError (packet);
  }
private:
  ns3::SpcMac *m_spcMac;
};

SpcMac::SpcMac ()
  : m_phySpcMacListener (0),
    m_currentPacket1 (0),
    m_currentPacket2 (0),
    m_rtsSendThreshold (1000),
    m_resendRtsNum (0),
    m_resendRtsMax (7),
    m_resendDataNum (0),
    m_resendDataMax (7),
    m_recvCtsNum (0),
    m_cwMin (15),
    m_cwMax (1023),
    m_cw (m_cwMin),
    m_backoffSlots (0),
    m_sifs (MicroSeconds (16)),
    m_difs (MicroSeconds (34)),
    m_slotTime (MicroSeconds (9)),
    m_backoffStart (0),
    m_lastRxStart (0),
    m_lastRxDuration (0),
    m_lastBusyStart (0),
    m_lastBusyDuration (0),
    m_lastTxStart (0),
    m_lastTxDuration (0),
    m_lastNavStart (0),
    m_lastNavDuration (0),
    m_lastAckTimeoutEnd (0),
    m_lastCtsTimeoutEnd (0),
    m_waitTime (0),
    m_rxing (false),
    m_minRate (6000000 / 8),
    m_measureTrafficInterval (Seconds (0.1)),
    m_restrictionPacketNum (10),
    m_sendCtsAfterRtsEvent (),
    m_sendDataAfterCtsEvent (),
    m_sendAckAfterDataEvent (),
    m_ackTimeoutEvent1 (),
    m_ackTimeoutEvent2 (),
    m_ctsTimeoutEvent (),
    m_backoffTimeoutEvent (),
    m_backoffGrantStartEvent(),
    m_measureTrafficEvent ()
{
  NS_LOG_FUNCTION (this);
  SpcPreamble preamble;
  SpcMacHeader rts;
  rts.SetType (SPC_MAC_RTS);
  SpcMacHeader cts;
  cts.SetType (SPC_MAC_CTS);
  SpcMacHeader ack;
  ack.SetType (SPC_MAC_ACK);
  SpcMacTrailer fcs;

  Time rtsDuration = Seconds (double(rts.GetSize () + fcs.GetSize ()) / preamble.GetRate ()) + preamble.GetDuration ();
  Time ctsDuration = Seconds (double(cts.GetSize () + fcs.GetSize ()) / preamble.GetRate ()) + preamble.GetDuration ();
  Time ackDuration = Seconds (double(ack.GetSize () + fcs.GetSize ()) / preamble.GetRate ()) + preamble.GetDuration ();

  m_maxPropagationDelay = Seconds (1000.0 / 300000000.0);
  m_rtsSendAndSifsTime = rtsDuration + m_maxPropagationDelay + m_sifs;
  m_ctsSendAndSifsTime = ctsDuration + m_maxPropagationDelay + m_sifs;
  m_ackSendAndSifsTime = ackDuration + m_maxPropagationDelay + m_sifs;
  
  m_phy = CreateObject<SpcPhy> ();
  m_queue = CreateObject<SpcMacQueue> ();
  m_nodeTable = CreateObject<NodeInformationTable> ();
  m_queue->SetNodeTable (m_nodeTable);
  m_rng = new SpcRealRandomStream ();
  SetupPhySpcMacListener (m_phy->GetPhyStateHelper ());

  m_measureTrafficEvent = Simulator::Schedule (Seconds (0.01),
					       &SpcMac::MeasureTrafficEnd,
					       this);
}

TypeId
SpcMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpcMac")
    .SetParent<Object> ()
    .AddAttribute ("Rate", "Rate for send.",
                   UintegerValue (6000000 / 8),
                   MakeUintegerAccessor (&SpcMac::m_rate),
                   MakeUintegerChecker<uint32_t>(0))
  ;
  return tid;
}

int64_t
SpcMac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_rng->AssignStreams (stream);
  return 1;
}

void
SpcMac::MeasureTrafficEnd ()
{
  m_nodeTable->UpdateTraffic (m_measureTrafficInterval);
  m_nodeTable->ResetSize ();
  m_measureTrafficEvent = Simulator::Schedule (m_measureTrafficInterval,
					       &SpcMac::MeasureTrafficEnd,
					       this);
}

SpcMac::~SpcMac ()
{
  NS_LOG_FUNCTION (this);
}

void
SpcMac::SetNetDevice (Ptr<SpcNetDevice> device){
  m_device = device;
}

void
SpcMac::SetAddress (Mac48Address address){
  m_address = address;
}

Ptr<SpcPhy>
SpcMac::GetPhy ()
{
  return m_phy;
}

Mac48Address
SpcMac::GetAddress (){
  return m_address;
}

double
SpcMac::GetNoiseFloor (uint32_t bandwidth) const
{
  static const double BOLTZMANN = 1.3803e-23;
  double Nt = BOLTZMANN * 290.0 * bandwidth;
  double noiseFloor = m_phy->GetRxNoiseFigure () * Nt;
  return noiseFloor;
}

void
SpcMac::SetupPhySpcMacListener (Ptr<SpcPhyStateHelper> state)
{
  m_phySpcMacListener = new PhySpcMacListener (this);
  state->RegisterListener (m_phySpcMacListener);
}

void
SpcMac::NotifyMaybeCcaBusyStartNow (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  m_lastBusyStart = Simulator::Now ();
  m_lastBusyDuration = duration;
}

void
SpcMac::NotifyTxStartNow (Time duration)
{
  NS_LOG_FUNCTION (this);
  if (m_rxing)
    {
      // NS_ASSERT (Simulator::Now () - m_lastRxStart <= m_sifs);
      m_lastRxDuration = Simulator::Now () - m_lastRxStart;
      m_rxing = false;
    }
  m_lastTxStart = Simulator::Now ();
  m_lastTxDuration = duration;
}

void
SpcMac::NotifyRxStartNow (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  m_lastRxStart = Simulator::Now ();
  m_lastRxDuration = duration;
  m_rxing = true;
}

Time
SpcMac::GetBackoffGrantStart (void) const
{
  NS_LOG_FUNCTION (this);

  Time rxBackoffStart = m_lastRxStart + m_lastRxDuration + m_difs;
  Time txBackoffStart = m_lastTxStart + m_lastTxDuration + m_difs;
  Time busyBackoffStart = m_lastBusyStart + m_lastBusyDuration + m_difs;
  Time navBackoffStart = m_lastNavStart + m_lastNavDuration + m_difs;
  Time ackTimeoutBackoffStart = m_lastAckTimeoutEnd + m_difs;
  Time ctsTimeoutBackoffStart = m_lastCtsTimeoutEnd + m_difs;
  Time waitTimeBackoffStart   = m_waitTime + m_difs;
  Time backoffGrantedStart = Max (rxBackoffStart, txBackoffStart);
  backoffGrantedStart = Max (backoffGrantedStart, busyBackoffStart);
  backoffGrantedStart = Max (backoffGrantedStart, navBackoffStart);
  backoffGrantedStart = Max (backoffGrantedStart, ackTimeoutBackoffStart);
  backoffGrantedStart = Max (backoffGrantedStart, ctsTimeoutBackoffStart);
  backoffGrantedStart = Max (backoffGrantedStart, waitTimeBackoffStart);
                                        
  NS_LOG_INFO ("access grant start="  << backoffGrantedStart <<
               ", rx start="   << rxBackoffStart   <<
               ", tx start="   << txBackoffStart    <<
               ", busy start=" << busyBackoffStart <<
               ", nav start="  << navBackoffStart  <<
               ", ack timeout start="  << ackTimeoutBackoffStart << 
               ", cts timeout start="  << ackTimeoutBackoffStart <<
	       ", wait time=" << m_waitTime
	       );
  return backoffGrantedStart;
}

Time
SpcMac::GetSendGrantStart (void) const
{
  NS_LOG_FUNCTION (this);

  Time rxSendStart = m_lastRxStart + m_lastRxDuration;
  Time txSendStart = m_lastTxStart + m_lastTxDuration;
  Time busySendStart = m_lastBusyStart + m_lastBusyDuration;
  Time navSendStart = m_lastNavStart + m_lastNavDuration;
  Time ackTimeoutSendStart = m_lastAckTimeoutEnd;
  Time ctsTimeoutSendStart = m_lastCtsTimeoutEnd;
  Time waitTimeSendStart   = m_waitTime;
  Time sendGrantedStart = Max (rxSendStart, txSendStart);
  sendGrantedStart = Max (sendGrantedStart, busySendStart);
  sendGrantedStart = Max (sendGrantedStart, navSendStart);
  sendGrantedStart = Max (sendGrantedStart, ackTimeoutSendStart);
  sendGrantedStart = Max (sendGrantedStart, ctsTimeoutSendStart);
  sendGrantedStart = Max (sendGrantedStart, waitTimeSendStart);
                                        
  NS_LOG_INFO ("access grant start="  << sendGrantedStart <<
               ", rx start="   << rxSendStart   <<
               ", tx start="   << txSendStart    <<
               ", busy start=" << busySendStart <<
               ", nav start="  << navSendStart  <<
               ", ack timeout start="  << ackTimeoutSendStart <<
               ", cts timeout start="  << ctsTimeoutSendStart <<
	       ", wait time=" << m_waitTime
	       );
  return sendGrantedStart;
}

void
SpcMac::UpdateCw ()
{
  m_cw = std::min ( 2 * (m_cw + 1) - 1, m_cwMax);
  m_backoffSlots = m_rng->GetNext (0, m_cw);
  m_backoffStart = Simulator::Now ();
}

void
SpcMac::InitSend ()
{
  NS_LOG_FUNCTION (this);
  m_resendRtsNum  = 0;
  m_resendDataNum = 0;
  m_cw = m_cwMin;
  m_unicast = false;
}

void
SpcMac::ReceiveOk (Ptr<Packet> packet, double rssi, uint8_t spcNum)
{
  NS_LOG_FUNCTION (this << rssi);

  m_rxing = false;

  SpcMacHeader hdr;
  packet->RemoveHeader (hdr);
  NS_LOG_DEBUG (hdr);

  // Set Nav
  if (hdr.GetType () == SPC_MAC_DATA_SPC || hdr.GetType () == SPC_MAC_RTS_SPC)
    {
      if (hdr.GetAddr1 () != GetAddress () && hdr.GetAddr2 () != GetAddress ())
	{
	  SetNav (hdr.GetDuration ());
	}
    }
  else if (hdr.GetType () == SPC_MAC_CTS_SPC)
    {
      // ???
    }
  else
    {
      if (hdr.GetAddr1 () != GetAddress ())
	{
	  SetNav (hdr.GetDuration ());
	}
    }

  switch (hdr.GetType ())
    {
    /** RTS **/
    case SPC_MAC_RTS:
      if (hdr.GetAddr1 () == GetAddress ())
	{
	  m_waitTime = m_waitTime = Max (m_waitTime, Simulator::Now () + m_sifs + m_ctsSendAndSifsTime);
	  m_sendCtsAfterRtsEvent = Simulator::Schedule (m_sifs,
							&SpcMac::SendCtsAfterRts,
							this,
							hdr.GetAddr2 (),
							rssi);
	}
      break;
      
    /** CTS **/
    case SPC_MAC_CTS:
      if (hdr.GetAddr1 () == GetAddress ())
	{
	  NS_ASSERT (m_sendState != SPC);
	  NS_LOG_DEBUG (m_currentHdr1.GetAddr1 ());
	  if (m_sendState == FIRST)
	    {
	      double rssi = ConvertRssiToW (hdr.GetRtsRssi ());
	      NS_LOG_INFO ("Rssi=" << rssi  << " , Addr=" << m_currentHdr1.GetAddr1 ());
	      m_nodeTable->UpdatePassLoss (m_currentHdr1.GetAddr1 (), rssi);
	    }
	  else if (m_sendState == SECOND)
	    {
	      double rssi = ConvertRssiToW (hdr.GetRtsRssi ());
	      NS_LOG_INFO ("Rssi=" << rssi << " , Addr=" << m_currentHdr2.GetAddr1 ());
	      m_nodeTable->UpdatePassLoss (m_currentHdr2.GetAddr1 (), rssi);
	    }
	  m_ctsTimeoutEvent.Cancel ();
	  m_lastCtsTimeoutEnd = Simulator::Now () + m_sifs;
	  m_sendDataAfterCtsEvent = Simulator::Schedule (m_sifs,
							 &SpcMac::SendUnicastDataAfterCts,
							 this);
	}
      break;
  
    /** DATA **/
    case SPC_MAC_DATA:
      if (hdr.GetAddr1 () == GetAddress () && !hdr.GetAddr1 ().IsGroup ())
	{
	  m_sendAckAfterDataEvent = Simulator::Schedule (m_sifs,
							 &SpcMac::SendAckAfterData,
							 this,
							 hdr.GetAddr2 (),
							 SpcMacHeader::FIRST);

	  m_device->Receive (packet, hdr.GetAddr1 (), hdr.GetAddr2 ());
	}
      if (hdr.GetAddr1 ().IsGroup ())
	{
	  m_device->Receive (packet, hdr.GetAddr1 (), hdr.GetAddr2 ());
	}
      break;
      
    /** RTS_SPC **/
    case SPC_MAC_RTS_SPC:
      if (hdr.GetAddr1 () == GetAddress ())
	{
	  NS_LOG_DEBUG ("********** Receive RTS SPC1: Rssi=" << rssi << " **********");
	  m_waitTime = m_waitTime = Max (m_waitTime, Simulator::Now () + m_sifs + m_ctsSendAndSifsTime * 2);
	  m_sendCtsAfterRtsEvent = Simulator::Schedule (m_sifs,
							&SpcMac::SendCtsSpcAfterRtsSpc,
							this,
							hdr.GetAddr1 (),
							rssi);
	}
      else if (hdr.GetAddr2 () == GetAddress ())
	{

	  NS_LOG_DEBUG ("********** Receive RTS SPC2: Rssi=" << rssi << " **********");
	  m_waitTime = m_waitTime = Max (m_waitTime, Simulator::Now () + m_sifs + m_ctsSendAndSifsTime * 2);
	  m_sendCtsAfterRtsEvent = Simulator::Schedule (m_sifs + m_ctsSendAndSifsTime,
							&SpcMac::SendCtsSpcAfterRtsSpc,
							this,
							hdr.GetAddr2 (),
							rssi);
	}
      break;
      
    /** CTS_SPC **/
    case SPC_MAC_CTS_SPC:
      if (!m_ctsTimeoutEvent.IsExpired () &&
	  (m_currentHdr1.GetAddr1 () == hdr.GetAddr1 () || m_currentHdr2.GetAddr1 () == hdr.GetAddr1 ()))
	{
	  NS_ASSERT (m_sendState == SPC);
	  double rssi = ConvertRssiToW (hdr.GetRtsRssi ());
	  NS_LOG_INFO ("Rssi=" << rssi << " recvCtsNum=" << m_recvCtsNum);
	  m_nodeTable->UpdatePassLoss (hdr.GetAddr1 (), rssi);
	  if (++m_recvCtsNum == 2)
	    {
	      m_ctsTimeoutEvent.Cancel ();
	      m_lastCtsTimeoutEnd = Simulator::Now () + m_sifs;
	      m_sendDataAfterCtsEvent = Simulator::Schedule (m_sifs,
							     &SpcMac::SendSpcDataAfterCtsSpc,
							     this);
	    }
	}
      break;

    /** DATA_SPC **/
    case SPC_MAC_DATA_SPC:
      NS_ASSERT(!hdr.GetAddr1 ().IsGroup () && !hdr.GetAddr2 ().IsGroup ());
      if (spcNum == SpcMacHeader::FIRST && hdr.GetAddr1 () == GetAddress ())
	{
	  NS_LOG_DEBUG ("Receive SPC DATA: to=" << hdr.GetAddr1 () <<
			", from=" << hdr.GetAddr3 () <<
			", size=" << packet->GetSize ());
	  PacketInfo packetInfo;
	  packetInfo.SetPacketInfo (packet);
	  m_waitTime = Max (m_waitTime, Simulator::Now () + m_ackSendAndSifsTime * 2 + m_sifs);
	  m_sendAckAfterDataEvent = Simulator::Schedule (m_sifs,
							 &SpcMac::SendAckAfterData,
							 this,
							 hdr.GetAddr3 (),
							 SpcMacHeader::FIRST);
	  m_device->Receive (packet, hdr.GetAddr1 (), hdr.GetAddr3 ());
	}
      else if (spcNum == SpcMacHeader::SECOND && hdr.GetAddr2 () == GetAddress ())
	{
	  NS_LOG_DEBUG ("Receive SPC DATA: to=" << hdr.GetAddr2 () <<
			", from=" << hdr.GetAddr3 () <<
			", size=" << packet->GetSize ());
	  PacketInfo packetInfo;
	  packetInfo.SetPacketInfo (packet);
	  m_waitTime = Max (m_waitTime, Simulator::Now () + m_ackSendAndSifsTime * 2 + m_sifs);
	  m_sendAckAfterDataEvent = Simulator::Schedule (m_ackSendAndSifsTime + m_sifs,
							 &SpcMac::SendAckAfterData,
							 this,
							 hdr.GetAddr3 (),
							 SpcMacHeader::SECOND);
	  m_device->Receive (packet, hdr.GetAddr2 (), hdr.GetAddr3 ());
	}
      break;
      
    /** ACK **/
    case SPC_MAC_ACK:
      if (hdr.GetAddr1 () == GetAddress ())
	{
	  uint8_t spcNum = hdr.GetSpcNum ();
	  if (m_sendState == FIRST)
	    {
	      NS_LOG_DEBUG ("receive Ack: state first");
	      m_ackTimeoutEvent1.Cancel ();
	      m_currentPacket1 = 0;
	    }
	  else if (m_sendState == SECOND)
	    {
	      NS_LOG_DEBUG ("receive Ack: state second");
	      m_ackTimeoutEvent2.Cancel ();
	      m_currentPacket2 = 0;
	    }
	  else if (m_sendState == SPC)
	    {
	      NS_LOG_DEBUG ("receive Ack: state spc");
	      if (spcNum == SpcMacHeader::FIRST)
		{
		  m_ackTimeoutEvent1.Cancel ();
		  m_currentPacket1 = 0;
		}
	      else if(spcNum == SpcMacHeader::SECOND)
		{
		  m_ackTimeoutEvent2.Cancel ();
		  m_currentPacket2 = 0;
		}
	      else
		{
		  NS_ASSERT (false);
		}
	    }
	  InitSend ();
	  StartBackoffIfNeeded ();
	}
	  
      break;
    }
}

void
SpcMac::ReceiveError (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this);
  m_rxing = false;

  SpcMacHeader hdr;
  packet->RemoveHeader (hdr);
  NS_LOG_DEBUG (hdr);
}


struct SpcMac::TimeRate
SpcMac::CalculateTimeRate (double passLoss, uint32_t size, uint32_t bandwidth)
{
  double optRate = bandwidth * log2 (1 + (passLoss / GetNoiseFloor (bandwidth)));
  NS_LOG_DEBUG ("passLoss: "    << passLoss  <<
		", noise: "     << GetNoiseFloor (bandwidth) <<
		", ratio: "     << (passLoss / GetNoiseFloor (bandwidth)) <<
		", size: "      << size      <<
		", bandwidth: " << bandwidth <<
		", optRate: "   << optRate   <<
		", time: "      << Seconds(size / (optRate / 8)));
  struct TimeRate timeRate;
  timeRate.time = Seconds(size / (optRate / 8));
  timeRate.rate = optRate / 8;
  return timeRate;
}

struct SpcMac::PowerTimeRate
SpcMac::CalculatePowerTimeRate (double passLoss1, double passLoss2, uint32_t size1, uint32_t size2, uint32_t bandwidth)
{
  double minEndTime = sizeof(double);
  double optPower = 0;
  uint32_t optRate = 0;

  for (double power = 0.1; power < 1.0; power += 0.01)
    {
      double pow1 = power;
      double pow2 = 1 - power;
      double t1, t2;
      if (pow1 >= 0.5)
	{
	  t1 = (pow1 * passLoss1) / (pow2 * passLoss1 + GetNoiseFloor (bandwidth));
	  t2 = (pow2 * passLoss2) / GetNoiseFloor (bandwidth);
	}
      else
	{
	  t1 = (pow1 * passLoss1) / GetNoiseFloor (bandwidth);
	  t2 = (pow2 * passLoss2) / (pow1 * passLoss2 + GetNoiseFloor (bandwidth));
	}
      double capacity1 = bandwidth * log2 (1 + t1) / 8;
      double capacity2 = bandwidth * log2 (1 + t2) / 8;
      double currentEndTime = std::max (size1 / capacity1, size2 / capacity2);
      if (currentEndTime < minEndTime)
	{
	  optPower = power;
	  minEndTime = currentEndTime;
	  if (size1 / capacity1 > size2 / capacity2)
	    {
	      optRate = capacity1;
	    }
	  else
	    {
	      optRate = capacity2;
	    }
	}
    }

  struct PowerTimeRate powerTimeRate;
  powerTimeRate.power = optPower;
  powerTimeRate.rate  = optRate;
  powerTimeRate.time  = Seconds (minEndTime);
  /*
  NS_LOG_DEBUG ("passLoss1: "    << passLoss1  <<
		", passLoss2: "  << passLoss2  <<
		", size1: "      << size1      <<
		", size2: "      << size2      <<
		", bandwidth: "  << bandwidth  <<
		", optRate: "    << optRate    <<
		", optPower: "   << optPower   <<
		", minTime:"     << Seconds (minEndTime));
  */

  return powerTimeRate;
}

struct SpcMac::TimeNum1Num2
SpcMac::GetWaitTimeForBuffer (void)
{
  double passLoss1 = m_nodeTable->GetPassLoss (m_currentHdr1.GetAddr1 ());
  double passLoss2 = m_nodeTable->GetPassLoss (m_currentHdr2.GetAddr1 ());
  uint32_t size1 = m_currentPacket1->GetSize ();
  uint32_t size2 = m_currentPacket2->GetSize ();
  uint32_t traffic1 = m_nodeTable->GetTraffic (m_currentHdr1.GetAddr1 ());
  uint32_t traffic2 = m_nodeTable->GetTraffic (m_currentHdr2.GetAddr1 ());

  TimeNum1Num2 tnn;
  tnn.num1 = 1;
  tnn.num2 = 2;
  tnn.time = Seconds (0);
  if (passLoss1 == 0 || passLoss2 == 0 ||
      traffic1  == 0 || traffic2  == 0)
    {
      return tnn;
    }

  SpcPreamble preamble;
  SpcMacHeader hdr;
  hdr.SetType (SPC_MAC_DATA_SPC);
  SpcMacTrailer fcs;
  tnn.time = Seconds (10000);
  for (uint32_t i = 1; i <= m_restrictionPacketNum; i++)
    {
      for (uint32_t j = 1; j <= m_restrictionPacketNum; j++)
	{
	  struct SpcMac::PowerTimeRate ptr;
	  uint32_t s1 = size1 * i + hdr.GetSize () + fcs.GetSize (); 
	  uint32_t s2 = size2 * j + hdr.GetSize () + fcs.GetSize (); 
	  ptr = CalculatePowerTimeRate (passLoss1, passLoss2,
					s1, s2,
					preamble.GetBandwidth ());
	  if(ptr.time <= tnn.time)
	    {
	      tnn.num1 = i;
	      tnn.num2 = j;
	      tnn.time   = ptr.time;
	    }
	}
    }

  uint32_t s1 = size1 * tnn.num1 + hdr.GetSize () + fcs.GetSize (); 
  uint32_t s2 = size2 * tnn.num2 + hdr.GetSize () + fcs.GetSize (); 
  tnn.time = std::max(Seconds (double(s1) / traffic1),
		      Seconds (double(s2) / traffic2));

  return tnn;
}

void
SpcMac::SetState ()
{
  // m_currentPacket1, 2が存在するかつSPCの再送でない場合
  if (m_currentPacket1 != 0 && m_currentPacket2 != 0 && !m_unicast)
    {
      // m_currentPacket1, 2のどちらか一方がブロードキャスト送信 or m_currentPacket1, 2が同じ宛先
      if ((m_currentHdr1.GetAddr1 ().IsGroup () || m_currentHdr2.GetAddr1 ().IsGroup ()) ||
	  (m_currentHdr1.GetAddr1 () == m_currentHdr2.GetAddr1 ()))
	{
	  NS_LOG_DEBUG ("first not spc");
	  m_sendState = FIRST;
	}
      // SPCできる場合
      else
	{
	  NS_LOG_DEBUG ("spc send");
	  m_sendState = SPC;
	}
    }
  // m_currentPacket1が存在する場合
  else if (m_currentPacket1 != 0)
    {
      NS_LOG_DEBUG ("first");
      m_sendState = FIRST;
    }
  // m_currentPacket2が存在する場合
  else if (m_currentPacket2 != 0)
    {
      NS_LOG_DEBUG ("second");
      m_sendState = SECOND;
    }
  else
    {
      NS_LOG_DEBUG ("first");
      m_sendState = FIRST;
    }
    
}

void
SpcMac::SendRts ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendState != SPC);
  NS_ASSERT (m_ctsTimeoutEvent.IsExpired ());

  Time timerDelay = m_rtsSendAndSifsTime + m_ctsSendAndSifsTime;
  m_ctsTimeoutEvent = Simulator::Schedule (timerDelay, &SpcMac::CtsTimeout, this);
  m_lastCtsTimeoutEnd = Simulator::Now () + timerDelay;
  NS_LOG_DEBUG ("CTS Time out: " << m_lastCtsTimeoutEnd);

  SpcMacHeader rts;
  rts.SetType (SPC_MAC_RTS);
  if (m_sendState == FIRST)
    {
      rts.SetAddr1 (m_currentHdr1.GetAddr1 ());
    }
  else if (m_sendState == SECOND)
    {
      rts.SetAddr1 (m_currentHdr2.GetAddr1 ());
    }
  rts.SetAddr2 (GetAddress ());
  rts.SetDuration (m_ctsSendAndSifsTime);
  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (rts);
  SpcMacTrailer fcs;
  packet->AddTrailer (fcs);

  SpcPreamble preamble;

  m_phy->StartSend (packet, preamble);
}

void
SpcMac::SendCtsAfterRts (Mac48Address source, double rssi)
{
  int a = ConvertRssiToDbm (rssi);
  NS_LOG_FUNCTION (this << a);

  SpcMacHeader cts;
  cts.SetType (SPC_MAC_CTS);
  cts.SetAddr1 (source);
  cts.SetRtsRssi (ConvertRssiToDbm (rssi));
  cts.SetDuration (m_sifs + m_maxPropagationDelay);
  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (cts);
  SpcMacTrailer fcs;
  packet->AddTrailer (fcs);

  SpcPreamble preamble;

  m_phy->StartSend (packet, preamble); 
}

uint8_t
SpcMac::ConvertRssiToDbm (double rssi) const
{
  int dbm = 10.0 * std::log10 (rssi);
  uint8_t convert;
  if (dbm >= 127)
    {
      convert = 255;
    }
  else if (dbm <= -127)
    {
      convert = 0;
    }
  else
    {
      convert = dbm += 127 - 1;
    }
  return convert;
}

double
SpcMac::ConvertRssiToW (uint8_t rssi) const
{
  double w = std::pow (10.0, (rssi - 127) / 10.0);
  return w;
}

void
SpcMac::SendUnicastDataAfterCts ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendState != SPC);

  Ptr<Packet> packet;
  SpcMacHeader hdr;
  hdr.SetType (SPC_MAC_DATA);
  if (m_sendState == FIRST)
    {
      hdr.SetAddr1 (m_currentHdr1.GetAddr1 ());
      packet = packetInfo1.CreatePacket ();
    }
  else if (m_sendState == SECOND)
    {
      hdr.SetAddr1 (m_currentHdr2.GetAddr1 ());
      packet = packetInfo2.CreatePacket ();
    }
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDuration (m_ackSendAndSifsTime);
  packet->AddHeader (hdr);
  SpcMacTrailer fcs;
  packet->AddTrailer (fcs);
  NS_LOG_DEBUG (hdr);

  SpcPreamble preamble;
  double passLoss = m_nodeTable->GetPassLoss (hdr.GetAddr1 ());
  m_powerRate = 1.0;
  TimeRate uni = CalculateTimeRate (passLoss, packet->GetSize (), preamble.GetBandwidth ());
  m_rate = uni.rate; 
  preamble.SetRate (m_rate);
  preamble.SetPower (m_powerRate);
  preamble.SetSymbols (packet->GetSize ());
  preamble.SetNLength (packet->GetSize () - hdr.GetSize ());
  preamble.SetFLength (packet->GetSize () - hdr.GetSize ());
  NS_LOG_DEBUG ("Rate: "     << m_rate <<
		", size: "   << packet->GetSize () <<
		", symbol: " << preamble.GetSymbols () <<
		", ack"      << m_ackSendAndSifsTime);

  Time txDuration =
    Seconds((double)preamble.GetSymbols () / preamble.GetRate ()) +
    preamble.GetDuration () +
    m_maxPropagationDelay;
  Time timerDelay = txDuration + m_ackSendAndSifsTime;
  m_lastAckTimeoutEnd = Simulator::Now () + timerDelay;
  if (m_sendState == FIRST)
    {
      NS_ASSERT (m_ackTimeoutEvent1.IsExpired ());
      m_ackTimeoutEvent1 = Simulator::Schedule (timerDelay, &SpcMac::AckTimeout1, this);
    }
  else if (m_sendState == SECOND)
    {
      NS_ASSERT (m_ackTimeoutEvent2.IsExpired ());
      m_ackTimeoutEvent2 = Simulator::Schedule (timerDelay, &SpcMac::AckTimeout2, this);
    }
  NS_LOG_DEBUG ("duration=" << txDuration <<
		"symbol="   << preamble.GetSymbols () <<
		"rate="     << preamble.GetRate ());
  NS_LOG_DEBUG ("[ACK Time out] duration=" << timerDelay <<  ", end time=" << m_lastAckTimeoutEnd);

  m_phy->StartSend (packet, preamble); 
}

void
SpcMac::SendRtsSpc ()
{
  NS_LOG_FUNCTION (this << m_currentHdr1.GetAddr1 () << m_currentHdr2.GetAddr1 ());
  NS_ASSERT (m_sendState == SPC);
  NS_ASSERT (m_ctsTimeoutEvent.IsExpired ());

  m_recvCtsNum = 0;
  Time timerDelay = m_rtsSendAndSifsTime + m_ctsSendAndSifsTime * 2;
  m_ctsTimeoutEvent = Simulator::Schedule (timerDelay, &SpcMac::CtsTimeout, this);
  m_lastCtsTimeoutEnd = Simulator::Now () + timerDelay;
  NS_LOG_DEBUG ("CTS Time out: " << m_lastCtsTimeoutEnd);

  SpcMacHeader rts;
  rts.SetType (SPC_MAC_RTS_SPC);
  rts.SetAddr1 (m_currentHdr1.GetAddr1 ());
  rts.SetAddr2 (m_currentHdr2.GetAddr1 ());
  rts.SetDuration (m_ctsSendAndSifsTime * 2);
  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (rts);
  SpcMacTrailer fcs;
  packet->AddTrailer (fcs);

  SpcPreamble preamble;

  m_phy->StartSend (packet, preamble);
}

void
SpcMac::SendCtsSpcAfterRtsSpc (Mac48Address source, double rssi)
{
  int a = ConvertRssiToDbm (rssi);
  NS_LOG_FUNCTION (this << a);

  SpcMacHeader cts;
  cts.SetType (SPC_MAC_CTS_SPC);
  cts.SetAddr1 (source);
  cts.SetRtsRssi (ConvertRssiToDbm (rssi));
  cts.SetDuration (m_maxPropagationDelay + m_sifs);
  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (cts);
  SpcMacTrailer fcs;
  packet->AddTrailer (fcs);

  SpcPreamble preamble;

  m_phy->StartSend (packet, preamble); 
}

void
SpcMac::SendSpcDataAfterCtsSpc ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendState == SPC);
  NS_ASSERT (m_ackTimeoutEvent1.IsExpired ());
  NS_ASSERT (m_ackTimeoutEvent2.IsExpired ());

  Ptr<Packet> packet1 = packetInfo1.CreatePacket ();
  Ptr<Packet> packet2 = packetInfo2.CreatePacket ();
  SpcPreamble preamble;
  SpcMacHeader hdrUni, hdrSpc;
  SpcMacTrailer fcs;
  hdrUni.SetType (SPC_MAC_DATA);
  hdrSpc.SetType (SPC_MAC_DATA_SPC);
  double passLoss1 = m_nodeTable->GetPassLoss (m_currentHdr1.GetAddr1 ());
  double passLoss2 = m_nodeTable->GetPassLoss (m_currentHdr2.GetAddr1 ());
  struct PowerTimeRate spc = CalculatePowerTimeRate (passLoss1,
						     passLoss2,
						     packet1->GetSize () + hdrSpc.GetSize () + fcs.GetSize (),
						     packet2->GetSize () + hdrSpc.GetSize () + fcs.GetSize (),
						     preamble.GetBandwidth ());
  struct TimeRate uni1 = CalculateTimeRate (passLoss1,
					    packet1->GetSize () + hdrUni.GetSize () + fcs.GetSize (),
					    preamble.GetBandwidth ());
  struct TimeRate uni2 = CalculateTimeRate (passLoss2,
					    packet2->GetSize () + hdrUni.GetSize () + fcs.GetSize (),
					    preamble.GetBandwidth ());
  
  if (spc.time <= uni1.time + uni2.time)
    {
      // spc
      SpcMacHeader hdr;
      hdr.SetType (SPC_MAC_DATA_SPC);
      hdr.SetAddr1 (m_currentHdr1.GetAddr1 ());
      hdr.SetAddr2 (m_currentHdr2.GetAddr1 ());
      hdr.SetAddr3 (GetAddress ());
      hdr.SetDuration (m_ackSendAndSifsTime * 2);
      packet1->AddHeader (hdr);
      packet2->AddHeader (hdr);
      SpcMacTrailer fcs;
      packet1->AddTrailer (fcs);
      packet2->AddTrailer (fcs);
      NS_LOG_DEBUG (hdr);
      
      uint32_t maxSymbols = std::max (packet1->GetSize (), packet2->GetSize ());
      m_powerRate = spc.power;
      m_rate = spc.rate;
      preamble.SetRate (m_rate);
      preamble.SetPower (m_powerRate);
      preamble.SetSymbols (maxSymbols);
      preamble.SetNLength (packet1->GetSize () - m_currentHdr1.GetSize ());
      preamble.SetFLength (packet2->GetSize () - m_currentHdr2.GetSize ());
      
      Time txDuration = Seconds((double)preamble.GetSymbols () / preamble.GetRate ()) +
	preamble.GetDuration () + m_maxPropagationDelay;
      Time timerDelay = txDuration + m_ackSendAndSifsTime * 2;
      m_lastAckTimeoutEnd = Simulator::Now () + timerDelay;
      m_ackTimeoutEvent1 = Simulator::Schedule (timerDelay, &SpcMac::AckTimeout1, this);
      m_ackTimeoutEvent2 = Simulator::Schedule (timerDelay, &SpcMac::AckTimeout2, this);
      NS_LOG_DEBUG ("[ACK Time out] duration=" << timerDelay <<  ", end time=" << m_lastAckTimeoutEnd);
      m_phy->StartSend (packet1, packet2, preamble); 
    }
  else
    {
      // unicast
      m_sendState = FIRST;
      SendUnicastDataAfterCts ();
    }
}

void
SpcMac::SendUnicastDataNoAck ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendState != SPC);

  Ptr<Packet> packet;
  SpcMacHeader hdr;
  if (m_sendState == FIRST)
    {
      hdr.SetAddr1 (m_currentHdr1.GetAddr1 ());
      packet = packetInfo1.CreatePacket ();
    }
  else
    {
      hdr.SetAddr1 (m_currentHdr2.GetAddr1 ());
      packet = packetInfo2.CreatePacket ();
    }
  hdr.SetType (SPC_MAC_DATA);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetDuration (Seconds (0));
  packet->AddHeader (hdr);
  SpcMacTrailer fcs;
  packet->AddTrailer (fcs);

  SpcPreamble preamble;
  m_powerRate = 1.0;
  m_rate = m_minRate;
  preamble.SetRate (m_rate);
  preamble.SetPower (m_powerRate);
  preamble.SetSymbols (packet->GetSize ());
  preamble.SetNLength (packet->GetSize () - hdr.GetSize ());
  preamble.SetFLength (packet->GetSize () - hdr.GetSize ());

  m_phy->StartSend (packet, preamble); 

  InitSend ();
  if (m_sendState == FIRST)
    {
      m_currentPacket1 = 0;
    }
  else if (m_sendState == SECOND)
    {
      m_currentPacket2 = 0;
    }
  StartBackoffIfNeeded ();
}



void
SpcMac::SendAckAfterData (Mac48Address source, uint8_t spcNum)
{
  NS_LOG_FUNCTION (this);

  SpcMacHeader ack;
  ack.SetType (SPC_MAC_ACK);
  ack.SetSpcNum (spcNum);
  ack.SetAddr1 (source);
  ack.SetDuration (Seconds (0));
  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (ack);
  SpcMacTrailer fcs;
  packet->AddTrailer (fcs);

  SpcPreamble preamble;

  m_phy->StartSend (packet, preamble); 
}

void
SpcMac::SetNav (Time duration)
{
  m_lastNavStart = Seconds (0);
  if (m_lastNavStart + m_lastNavDuration < Simulator::Now () + duration)
    {
      m_lastNavDuration = duration;
      m_lastNavStart = Simulator::Now ();
    }
}

void
SpcMac::Enqueue (Ptr<Packet const> packet, const SpcMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  m_queue->Enqueue (packet, hdr);
  StartBackoffIfNeeded ();
}

/*
 * キューで(宛先アドレス＋ポート番号)のアグリゲーションをする
 * 2つのパケットが同じ宛先アドレスを持つ場合はユニキャスト
 * 2つのパケットが異なる宛先アドレスを持つ場合はSPC
 * m_currentHdrはここで作成しなおす
 */
void
SpcMac::StartBackoffIfNeeded ()
{
  NS_LOG_FUNCTION (this);

  if (!m_backoffGrantStartEvent.IsExpired ())
    {
      return;
    }

  if (m_currentPacket1 !=0 || m_currentPacket2 != 0)
  {
    BackoffGrantStart ();
  }

  if (m_currentPacket1 == 0 && m_currentPacket2 == 0 &&
      !m_queue->IsEmpty ())
    {
      m_currentPacket1 = m_queue->Dequeue (&m_currentHdr1);
      packetInfo1.SetPacketInfo (m_currentPacket1->Copy ());

      if (!m_queue->IsEmpty ())
      {
	m_currentPacket2 = m_queue->Dequeue (&m_currentHdr2);
	packetInfo2.SetPacketInfo (m_currentPacket2->Copy ());
      }
      BackoffGrantStart ();
    }
}

void
SpcMac::BackoffGrantStart ()
{
  NS_LOG_FUNCTION (this);
  Time backoffGrantStart = GetBackoffGrantStart ();
  if (backoffGrantStart <= Simulator::Now ())
    {
      StartBackoff ();
    }
  else
    {
      Time duration = backoffGrantStart - Simulator::Now ();
      m_backoffGrantStartEvent = Simulator::Schedule (duration, &SpcMac::BackoffGrantStart, this);
    }
}

void
SpcMac::StartBackoff ()
{
  NS_LOG_FUNCTION (this);
  m_backoffSlots = m_rng->GetNext (0, m_cw);
  m_backoffStart = Simulator::Now ();
  Time duration = m_backoffSlots * m_slotTime;
  NS_LOG_DEBUG ("slot: "   << m_backoffSlots <<
		", start: "<< m_backoffStart <<
		", end: "  << m_backoffSlots * m_slotTime + m_backoffStart);
  SetState ();
  if (m_cw == m_cwMin && m_sendState == SPC)
    {
      m_tnn = GetWaitTimeForBuffer ();
      if (m_tnn.time > duration )
	{
	  duration = m_tnn.time - duration;
	}
    }
  m_backoffTimeoutEvent = Simulator::Schedule (duration, &SpcMac::BackoffTimeout, this);
}

void
SpcMac::BackoffTimeout ()
{
  NS_LOG_FUNCTION (this << m_currentPacket1 << m_currentPacket2 << m_unicast);
  Time sendGrantStartTime = GetSendGrantStart ();
  Time backoffGrantStart = GetBackoffGrantStart ();
  if (m_currentPacket1 == 0 && m_currentPacket2 == 0)
    {
      NS_LOG_DEBUG ("Does not have any frames.");
      InitSend ();
      return;
    }
  if (sendGrantStartTime <= Simulator::Now ())
    {
      SetState ();
      if (m_sendState == SPC)
	{
	  uint32_t size1 = m_queue->Aggregation (m_currentHdr1.GetAddr1 (), packetInfo1.GetDestPort (), m_tnn.num1);
	  uint32_t size2 = m_queue->Aggregation (m_currentHdr2.GetAddr1 (), packetInfo2.GetDestPort (), m_tnn.num2);
	  packetInfo1.SetSize (packetInfo1.GetSize () + size1);
	  packetInfo2.SetSize (packetInfo2.GetSize () + size2);
	  SendRtsSpc ();
	}
      else if (m_sendState == FIRST)
	{
	  if (m_currentHdr1.GetAddr1 ().IsGroup ())
	    {
	      SendUnicastDataNoAck ();
	    }
	  else
	    {
	      if (m_rtsSendThreshold <= m_currentPacket1->GetSize ())
		{
		  SendRts ();
		}
	      else
		{
		  SendUnicastDataNoAck ();
		}
	    }
	}
      else if (m_sendState == SECOND)
	{
	  if (m_currentHdr2.GetAddr1 ().IsGroup ())
	    {
	      SendUnicastDataNoAck ();
	    }
	  else
	    {

	      if (m_rtsSendThreshold <= m_currentPacket2->GetSize ())
		{
		  SendRts ();
		}
	      else
		{
		  SendUnicastDataNoAck ();
		}
	    }
	}
    }
  else
    {
      Time duration = backoffGrantStart - Simulator::Now ();
      m_backoffGrantStartEvent = Simulator::Schedule (duration, &SpcMac::BackoffGrantStart, this);
    }
}

void
SpcMac::CtsTimeout ()
{
  NS_LOG_FUNCTION (this << m_resendRtsNum);
  if (m_resendRtsMax > m_resendRtsNum)
    {
      m_resendRtsNum++;
      UpdateCw ();
      BackoffGrantStart ();
    }
  else
    {
      InitSend ();
      m_currentPacket1 = 0;
      m_currentPacket2 = 0;
      StartBackoffIfNeeded ();
    }
}

void
SpcMac::AckTimeout1 ()
{
  NS_LOG_FUNCTION (this << m_resendDataNum);
  if (m_sendState == SPC)
    {
      InitSend ();
      m_unicast = true;
      BackoffGrantStart ();
      return;
    }
  if (m_resendDataMax > m_resendDataNum)
    {
      m_resendDataNum++;
      UpdateCw ();
      BackoffGrantStart ();
    }
  else
    {
      InitSend ();
      m_currentPacket1 = 0;
      StartBackoffIfNeeded ();
    }
}

void
SpcMac::AckTimeout2 ()
{
  NS_LOG_FUNCTION (this << m_resendDataNum);

  if (m_sendState == SPC)
    {
      InitSend ();
      m_unicast = true;
      BackoffGrantStart ();
      return;
    }
  if (m_resendDataMax > m_resendDataNum)
    {
      m_resendDataNum++;
      UpdateCw ();
      BackoffGrantStart ();
    }
  else
    {
      InitSend ();
      m_currentPacket2 = 0;
      StartBackoffIfNeeded ();
    }
}


} // namespace ns3

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
#include "ns3/simulator.h"
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"

#include "spc-phy.h"
#include "spc-preamble.h"

NS_LOG_COMPONENT_DEFINE ("SpcPhy");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SpcPhy);

SpcPhy::SpcPhy ()
  : m_edThresholdW (DbmToW (-96.0)),
    m_ccaMode1ThresholdW (DbmToW (-99.0)),
    m_txGainDb (0),
    m_rxGainDb (0),
    m_txPowerDbm (20),
    m_endRxEvent ()
{
  NS_LOG_FUNCTION (this);
  m_channel = CreateObject<SpcChannel>();
  m_state = CreateObject<SpcPhyStateHelper>();
  m_random = CreateObject<UniformRandomVariable> ();

  m_rxNoiseFigureDb = 7;
  m_interference.SetNoiseFigure (DbToRatio (m_rxNoiseFigureDb));

}
SpcPhy::~SpcPhy ()
{
  NS_LOG_FUNCTION (this);
}
void
SpcPhy::DoDispose (){
  m_channel = 0;
  m_state = 0;
}

TypeId
SpcPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpcPhy")
    .SetParent<Object> ()
    .AddTraceSource ("StartTx", "Start transmission",
                     MakeTraceSourceAccessor (&SpcPhy::m_txTrace))
    ;
  return tid;
}

double
SpcPhy::GetRxNoiseFigure () const
{
  return DbToRatio (m_rxNoiseFigureDb);
}

void
SpcPhy::SetMobility (Ptr<Object> mobility)
{
  m_mobility = mobility;
}

void
SpcPhy::SetDevice (Ptr<Object> device)
{
  m_device = device;
}

Ptr<Object>
SpcPhy::GetMobility ()
{
  return m_mobility;
}

Ptr<SpcPhyStateHelper>
SpcPhy::GetPhyStateHelper () const
{
  return m_state;
}

Ptr<SpcChannel>
SpcPhy::GetChannel () const
{
  return m_channel;
}

Ptr<Object>
SpcPhy::GetDevice (void) const
{
  return m_device;
}

int64_t
SpcPhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_random->SetStream (stream);
  return 1;
}

void
SpcPhy::StartSend (Ptr<Packet> packet, SpcPreamble preamble)
{
  NS_LOG_FUNCTION (this);
  if (m_state->IsStateRx ())
    {
      m_endRxEvent.Cancel ();
      m_interference.NotifyRxEnd ();
    }
  m_txTrace (packet);
  Time txDuration = Seconds((double)packet->GetSize () / preamble.GetRate ()) + preamble.GetDuration ();
  m_state->SwitchToTx (txDuration);
  m_channel->Send (packet, preamble, m_txPowerDbm + m_txGainDb, this);
}

void
SpcPhy::StartSend (Ptr<Packet> packet1, Ptr<Packet> packet2, SpcPreamble preamble)
{
  NS_LOG_FUNCTION (this);
  if (m_state->IsStateRx ())
    {
      m_endRxEvent.Cancel ();
      m_interference.NotifyRxEnd ();
    }
  //  m_txTrace (packet);
  Time txDuration = Seconds((double)preamble.GetSymbols () / preamble.GetRate ()) + preamble.GetDuration ();
  m_state->SwitchToTx (txDuration);
  m_channel->Send (packet1, packet2, preamble, m_txPowerDbm + m_txGainDb, this);
}

void
SpcPhy::StartReceive (Ptr<Packet> packet, SpcPreamble preamble, double rxPowerDbm)
{
  NS_LOG_FUNCTION (this << rxPowerDbm + m_rxGainDb);
  double rxPowerW = DbmToW (rxPowerDbm + m_rxGainDb);
  Time rxDuration = Seconds((double)packet->GetSize () / preamble.GetRate ()) + preamble.GetDuration ();
  Ptr<SpcInterferenceHelper::Event> event;
  event = m_interference.Add (packet->GetSize (), rxDuration, rxPowerW, preamble);
  switch (m_state->GetState ())
    {
    case SpcPhyState::RX:
      NS_LOG_DEBUG ("Can not receive because state is RX");
      goto maybeCcaBusy;
      break;
    case SpcPhyState::TX:
      NS_LOG_DEBUG ("Can not receive because state is TX");
      goto maybeCcaBusy;
    break;
    case SpcPhyState::CCA_BUSY:
    case SpcPhyState::IDLE:
      if (rxPowerW > m_edThresholdW)
	{
	  m_interference.NotifyRxStart ();
	  m_state->SwitchToRx (rxDuration);
	  m_endRxEvent = Simulator::Schedule (rxDuration,
					      &SpcPhy::EndReceive,
					      this,
					      packet,
					      event);
	}
      else
	{
	  NS_LOG_DEBUG ("Can not receive because rxPower is small: " << rxPowerDbm + m_rxGainDb);
	  goto maybeCcaBusy;
	}
      break;
    }
  return;

maybeCcaBusy:
  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaMode1ThresholdW);
  if (!delayUntilCcaEnd.IsZero ())
    {
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

void
SpcPhy::StartReceive (Ptr<Packet> packet1, Ptr<Packet> packet2, SpcPreamble preamble, double rxPowerDbm)
{
  NS_LOG_FUNCTION (this << rxPowerDbm + m_rxGainDb);
  double rxPowerW = DbmToW (rxPowerDbm + m_rxGainDb);
  Time rxDuration = Seconds((double)preamble.GetSymbols () / preamble.GetRate ()) + preamble.GetDuration ();
  Ptr<SpcInterferenceHelper::Event> event;
  event = m_interference.Add (preamble.GetSymbols (), rxDuration, rxPowerW, preamble);
  switch (m_state->GetState ())
    {
    case SpcPhyState::RX:
      NS_LOG_DEBUG ("Can not receive because state is RX");
      goto maybeCcaBusy;
      break;
    case SpcPhyState::TX:
      NS_LOG_DEBUG ("Can not receive because state is TX");
      goto maybeCcaBusy;
    break;
    case SpcPhyState::CCA_BUSY:
    case SpcPhyState::IDLE:
      if (rxPowerW > m_edThresholdW)
	{
	  m_interference.NotifyRxStart ();
	  m_state->SwitchToRx (rxDuration);
	  m_endRxEvent = Simulator::Schedule (rxDuration,
					      &SpcPhy::EndReceive2,
					      this,
					      packet1,
					      packet2,
					      event);
	}
      else
	{
	  NS_LOG_DEBUG ("Can not receive because rxPower is small: " << rxPowerDbm + m_rxGainDb);
	  goto maybeCcaBusy;
	}
      break;
    }
  return;

maybeCcaBusy:
  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaMode1ThresholdW);
  if (!delayUntilCcaEnd.IsZero ())
    {
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

void
SpcPhy::EndReceive (Ptr<Packet> packet,  Ptr<SpcInterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << packet);
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  struct SpcInterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculateSnrPer (event);
  m_interference.NotifyRxEnd ();

  NS_LOG_DEBUG ("rate="   << (event->GetPreamble ().GetRate ()) <<
                ", snr="  << snrPer.snr <<
		", per="  << snrPer.per <<
		", size=" << packet->GetSize () <<
		", rx="   << event->GetRxPowerW ());

  if (m_random->GetValue () > snrPer.per)
    {
      m_state->EndReceiveOk (packet, event->GetRxPowerW (), SpcMacHeader::FIRST);
    }
  else
    {
      m_state->EndReceiveError (packet);
    }
}

void
SpcPhy::EndReceive2 (Ptr<Packet> packet1, Ptr<Packet> packet2, Ptr<SpcInterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << packet1 << packet2);
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  struct SpcInterferenceHelper::SnrPer2 snrPer2;
  snrPer2 = m_interference.CalculateSnrPer2 (event);
  m_interference.NotifyRxEnd ();

  NS_LOG_DEBUG ("rate=" << (event->GetPreamble ().GetRate ()) <<
                ", snr1=" << snrPer2.snr1 << ", per1=" << snrPer2.per1 <<
                ", snr2=" << snrPer2.snr2 << ", per2=" << snrPer2.per2);

  if (m_random->GetValue () > snrPer2.per1)
    {
      m_state->EndReceiveOk (packet1, 0, SpcMacHeader::FIRST);
    }
  else
    {
      m_state->EndReceiveError (packet1);
    }

  if (m_random->GetValue () > snrPer2.per2)
    {
      m_state->EndReceiveOk (packet2, 0, SpcMacHeader::SECOND);
    }
  else
    {
      m_state->EndReceiveError (packet2);
    }
}

double
SpcPhy::DbToRatio (double dB) const
{
  double ratio = std::pow (10.0, dB / 10.0);
  return ratio;
}

double
SpcPhy::DbmToW (double dBm) const
{
  double mW = std::pow (10.0, dBm / 10.0);
  return mW / 1000.0;
}

double
SpcPhy::RatioToDb (double ratio) const
{
  return 10.0 * std::log10 (ratio);
}
} // namespace ns3

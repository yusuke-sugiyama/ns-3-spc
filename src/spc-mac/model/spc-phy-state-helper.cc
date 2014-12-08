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

#include "spc-phy-state-helper.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("SpcPhyStateHelper");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SpcPhyStateHelper);

SpcPhyListener::~SpcPhyListener ()
{
}

SpcPhyStateHelper::SpcPhyStateHelper ()
  : m_startTx (0),
    m_startRx (0),
    m_startCcaBusy (0),
    m_endTx (0),
    m_endRx (0),
    m_endCcaBusy (0),
    m_listeners (0),
    m_rxing (false)
{
  NS_LOG_FUNCTION (this);
}

void
SpcPhyStateHelper::NotifyMaybeCcaBusyStart (Time duration)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyMaybeCcaBusyStart (duration);
    }
}
void
SpcPhyStateHelper::NotifyTxStart (Time duration)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyTxStart (duration);
    }
}
void
SpcPhyStateHelper::NotifyRxStart (Time duration)
{
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyRxStart (duration);
    }
}
bool
SpcPhyStateHelper::IsStateIdle (void)
{
  return (GetState () == SpcPhyState::IDLE);
}
bool
SpcPhyStateHelper::IsStateBusy (void)
{
  return (GetState () != SpcPhyState::IDLE);
}
bool
SpcPhyStateHelper::IsStateCcaBusy (void)
{
  return (GetState () == SpcPhyState::CCA_BUSY);
}
bool
SpcPhyStateHelper::IsStateRx (void)
{
  return (GetState () == SpcPhyState::RX);
}
bool
SpcPhyStateHelper::IsStateTx (void)
{
  return (GetState () == SpcPhyState::TX);
}

enum SpcPhyState::State
SpcPhyStateHelper::GetState (void)
{
  if (m_endTx > Simulator::Now ())
    {
      return SpcPhyState::TX;
    }
  else if (m_rxing)
    {
      return SpcPhyState::RX;
    }
  else if (m_endCcaBusy > Simulator::Now ())
    {
      return SpcPhyState::CCA_BUSY;
    }
  else
    {
      return SpcPhyState::IDLE;
    }
}

void
SpcPhyStateHelper::EndReceiveOk (Ptr<Packet> packet, double rssi, uint8_t spcNum)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      NS_LOG_DEBUG ("kita");
      (*i)->NotifyRxEndOk (packet, rssi, spcNum);
    }
  m_rxing = false;
}

void
SpcPhyStateHelper::EndReceiveError (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyRxEndError (packet);
    }

  m_rxing = false;
}

void
SpcPhyStateHelper::RegisterListener (SpcPhyListener *listener)
{
  m_listeners.push_back (listener);
}

void
SpcPhyStateHelper::SwitchMaybeToCcaBusy (Time duration)
{
  NS_LOG_FUNCTION (this << duration << duration + Simulator::Now ());
  NotifyMaybeCcaBusyStart (duration);
  Time now = Simulator::Now ();

  if (GetState () != SpcPhyState::CCA_BUSY)
    {
      m_startCcaBusy = now;
    }
  m_endCcaBusy = std::max (m_endCcaBusy, now + duration);
}

void
SpcPhyStateHelper::SwitchToTx (Time duration)
{
  NS_LOG_FUNCTION (this << duration << duration + Simulator::Now ());
  NotifyTxStart (duration);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case SpcPhyState::RX:
      /* The packet which is being received as well
       * as its endRx event are cancelled by the caller.
       */
      m_rxing = false;
      m_endRx = now;
      break;
    case SpcPhyState::IDLE:
    case SpcPhyState::TX:
    case SpcPhyState::CCA_BUSY:
      break;
    default:
      NS_FATAL_ERROR ("Invalid SpcPhyState state.");
      break;
    }
  m_endTx = now + duration;
  m_startTx = now;
}
void
SpcPhyStateHelper::SwitchToRx (Time duration)
{
  NS_LOG_FUNCTION (this << duration << duration + Simulator::Now ());
  NS_ASSERT (IsStateIdle () || IsStateCcaBusy ());
  NS_ASSERT (!m_rxing);
  NotifyRxStart (duration);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case SpcPhyState::RX:
    case SpcPhyState::TX:
      NS_FATAL_ERROR ("Invalid SpcPhyState state.");
      break;
    case SpcPhyState::IDLE:
    case SpcPhyState::CCA_BUSY:
      break;
    }
  m_rxing = true;
  m_startRx = now;
  m_endRx = now + duration;
  NS_ASSERT (IsStateRx ());
}
} // namespace ns3

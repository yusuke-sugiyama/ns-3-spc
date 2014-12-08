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

#include "spc-channel.h"
#include "spc-net-device.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/node.h"

NS_LOG_COMPONENT_DEFINE ("SpcChannel");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SpcChannel);

TypeId 
SpcChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpcChannel")
    .SetParent<Channel> ()
    .AddConstructor<SpcChannel> ()
    ;
  return tid;
}
  
SpcChannel::SpcChannel ()
{
  NS_LOG_FUNCTION (this);

  ObjectFactory factoryLoss;
  factoryLoss.SetTypeId ("ns3::LogDistancePropagationLossModel");
  Ptr<PropagationLossModel> loss = factoryLoss.Create<PropagationLossModel> ();
  SetPropagationLossModel (loss);

  ObjectFactory factoryDelay;
  factoryDelay.SetTypeId ("ns3::ConstantSpeedPropagationDelayModel");
  Ptr<PropagationDelayModel> delay = factoryDelay.Create<PropagationDelayModel> ();
  SetPropagationDelayModel (delay);
}

void
SpcChannel::SetPropagationLossModel (Ptr<PropagationLossModel> loss)
{
  m_loss = loss;
}

void
SpcChannel::SetPropagationDelayModel (Ptr<PropagationDelayModel> delay)
{
  m_delay = delay;
}

uint32_t
SpcChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION (this);
  return m_phyList.size ();
}

Ptr<NetDevice>
SpcChannel::GetDevice (uint32_t i) const
{
  return m_phyList[i]->GetDevice ()->GetObject<NetDevice> ();
}

void
SpcChannel::Add (Ptr<SpcPhy> phy)
{
  m_phyList.push_back (phy);
}
  
void
SpcChannel::Send (Ptr<Packet> packet, SpcPreamble preamble, double txPowerDbm, Ptr<SpcPhy> sender) const
{
  NS_LOG_FUNCTION (this);
  Ptr<MobilityModel> senderMobility = sender->GetMobility ()->GetObject<MobilityModel> ();
  uint32_t j = 0;
  for (PhyList::const_iterator i = m_phyList.begin (); i != m_phyList.end (); i++, j++)
    {
      if (sender == (*i))
	{
	  continue;
	}
      Ptr<MobilityModel> receiverMobility = (*i)->GetMobility ()->GetObject<MobilityModel> ();
      Time delay = m_delay->GetDelay (senderMobility, receiverMobility);
      double rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility);
      Ptr<Packet> copy = packet->Copy ();
      Ptr<Object> dstNetDevice = m_phyList[j]->GetDevice ();
      uint32_t dstNode;
      if (dstNetDevice == 0)
	{
	  dstNode = 0xffffffff;
	}
      else
	{
	  dstNode = dstNetDevice->GetObject<NetDevice> ()->GetNode ()->GetId ();
	}
      NS_LOG_DEBUG ("rxPower=" << rxPowerDbm << ", delay=" << delay);
      Simulator::ScheduleWithContext (dstNode,
				      delay,
				      &SpcChannel::Receive,
				      this,
				      copy,
				      preamble,
				      rxPowerDbm,
				      j);
    }
}

void
SpcChannel::Send (Ptr<Packet> packet1, Ptr<Packet> packet2, SpcPreamble preamble, double txPowerDbm, Ptr<SpcPhy> sender) const
{
  NS_LOG_FUNCTION (this);
  Ptr<MobilityModel> senderMobility = sender->GetMobility ()->GetObject<MobilityModel> ();
  uint32_t j = 0;
  for (PhyList::const_iterator i = m_phyList.begin (); i != m_phyList.end (); i++, j++)
    {
      if (sender == (*i))
	{
	  continue;
	}
      Ptr<MobilityModel> receiverMobility = (*i)->GetMobility ()->GetObject<MobilityModel> ();
      Time delay = m_delay->GetDelay (senderMobility, receiverMobility);
      double rxPowerDbm = m_loss->CalcRxPower (txPowerDbm, senderMobility, receiverMobility);
      Ptr<Packet> copy1 = packet1->Copy ();
      Ptr<Packet> copy2 = packet2->Copy ();
      Ptr<Object> dstNetDevice = m_phyList[j]->GetDevice ();
      uint32_t dstNode;
      if (dstNetDevice == 0)
	{
	  dstNode = 0xffffffff;
	}
      else
	{
	  dstNode = dstNetDevice->GetObject<NetDevice> ()->GetNode ()->GetId ();
	}
      NS_LOG_DEBUG ("rxPower=" << rxPowerDbm << ", delay=" << delay);
      Simulator::ScheduleWithContext (dstNode,
				      delay,
				      &SpcChannel::Receive2,
				      this,
				      copy1,
				      copy2,
				      preamble,
				      rxPowerDbm,
				      j);
    }
}

void
SpcChannel::Receive (Ptr<Packet> packet, SpcPreamble preamble, double rxPowerDbm, uint32_t i) const
{
  NS_LOG_FUNCTION (this);
  m_phyList[i]->StartReceive (packet, preamble, rxPowerDbm);
}

void
SpcChannel::Receive2 (Ptr<Packet> packet1, Ptr<Packet> packet2, SpcPreamble preamble, double rxPowerDbm, uint32_t i) const
{
  NS_LOG_FUNCTION (this);
  m_phyList[i]->StartReceive (packet1, packet2, preamble, rxPowerDbm);
}

} // namespace ns3

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

#ifndef SPC_CHANNEL_H
#define SPC_CHANNEL_H

#include "ns3/channel.h"
#include "ns3/spc-phy.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/mobility-model.h"
#include <vector>

#include "spc-preamble.h"
#include "spc-channel.h"

namespace ns3 {
class SpcPhy;
class SpcNetDevice;
class PropagationLossModel;
class PropagationDelayModel;

class SpcChannel : public Channel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  SpcChannel ();

  void SetPropagationLossModel (Ptr<PropagationLossModel> loss);
  void SetPropagationDelayModel (Ptr<PropagationDelayModel> delay);
  void Add (Ptr<SpcPhy> phy);
  virtual uint32_t GetNDevices (void) const;
  virtual Ptr<NetDevice> GetDevice (uint32_t i) const;

  void Send (Ptr<Packet> packet, SpcPreamble preamble, double txPowerDbm, Ptr<SpcPhy> sender) const; 
  void Send (Ptr<Packet> packet1, Ptr<Packet> packet2, SpcPreamble preamble, double txPowerDbm, Ptr<SpcPhy> sender) const; 
  void Receive (Ptr<Packet> packet, SpcPreamble preamble, double rxPowerDbm, uint32_t i) const;
  void Receive2 (Ptr<Packet> packet1, Ptr<Packet> packet2, SpcPreamble preamble, double rxPowerDbm, uint32_t i) const;

private:
  typedef std::vector<Ptr<SpcPhy> > PhyList;
  PhyList m_phyList;
  Ptr<PropagationLossModel> m_loss;
  Ptr<PropagationDelayModel> m_delay;
};

} // namespace ns3

#endif /* SPC_CHANNEL_H */

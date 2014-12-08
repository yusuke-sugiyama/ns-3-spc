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

#ifndef SPC_PHY_H
#define SPC_PHY_H


#include <stdint.h>
#include <string>
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/object.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/mac48-address.h"
#include "ns3/traced-callback.h"
#include "ns3/random-variable-stream.h"
#include "spc-random-stream.h"
#include "spc-channel.h"
#include "spc-mac.h"
#include "spc-phy-state.h"
#include "spc-phy-state-helper.h"
#include "spc-preamble.h"
#include "spc-interference-helper.h"

namespace ns3 {

class SpcPhyStateHelper;
class SpcChannel;

class SpcPhy: public Object
{
public:

  SpcPhy ();
  ~SpcPhy ();

  static TypeId GetTypeId (void);

  typedef enum State
  {
    IDLE,
    TX,
    RX,
    CCA_BUSY
  }State;

  void SetMobility (Ptr<Object> mobility);
  void SetDevice (Ptr<Object> device);
  Ptr<Object> GetMobility ();
  Ptr<SpcPhyStateHelper> GetPhyStateHelper () const;
  Ptr<SpcChannel> GetChannel () const;
  Ptr<Object> GetDevice () const;
  double GetRxNoiseFigure () const;
  int64_t AssignStreams (int64_t stream);

  void StartSend (Ptr<Packet> pacekt, SpcPreamble preamble);
  void StartSend (Ptr<Packet> pacekt1, Ptr<Packet> pacekt2, SpcPreamble preamble);
  void StartReceive (Ptr<Packet> packet, SpcPreamble preamble, double rxPowerDbm);
  void StartReceive (Ptr<Packet> packet1, Ptr<Packet> packet2, SpcPreamble preamble, double rxPowerDbm);
  void EndReceive (Ptr<Packet> packet, Ptr<SpcInterferenceHelper::Event> event);
  void EndReceive2 (Ptr<Packet> packet1, Ptr<Packet> packet2, Ptr<SpcInterferenceHelper::Event> event);
  double DbToRatio (double dB) const;
  double DbmToW (double dBm) const;
  double RatioToDb (double ratio) const;

protected:
  virtual void DoDispose (void);
private:
  Ptr<SpcChannel> m_channel;
  Ptr<Object> m_mobility;
  Ptr<SpcPhyStateHelper> m_state;
  Ptr<Object> m_device;
  Ptr<UniformRandomVariable> m_random;

  SpcInterferenceHelper m_interference;

  double m_edThresholdW;
  double m_ccaMode1ThresholdW;
  double m_txGainDb;
  double m_rxGainDb;
  double m_txPowerDbm;
  double m_rxNoiseFigureDb;

  EventId m_endRxEvent;
  TracedCallback<Ptr<Packet> > m_txTrace;
};

} // namespace ns3

#endif /* SPC_PHY_H */

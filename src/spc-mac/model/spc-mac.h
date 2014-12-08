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

#ifndef SPC_MAC_H
#define SPC_MAC_H

#include <stdint.h>
#include <string>
#include "ns3/event-id.h"
#include "ns3/traced-callback.h"
#include "ns3/ptr.h"
#include "spc-random-stream.h"
#include "spc-preamble.h"
#include "spc-mac-queue.h"
#include "spc-phy.h"
#include "spc-phy-state-helper.h"
#include "spc-net-device.h"
#include "node-information-table.h"
#include "packet-info.h"

#include "ns3/udp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/llc-snap-header.h"
#include "ns3/arp-header.h"
#include "ns3/icmpv4.h"

namespace ns3 {

class SpcRandomStream;
class SpcPhy;
class SpcPhyStateHelper;

class SpcMac: public Object
{
public:
  SpcMac ();
  ~SpcMac ();

  typedef enum SendState
  {
    FIRST,
    SECOND,
    SPC
  }SendState;

  struct PowerTimeRate
  {
    double power;
    Time time;
    double rate;
  };

  struct TimeRate
  {
    Time time;
    double rate;
  };

  struct TimeNum1Num2
  {
    Time time;
    uint32_t num1;
    uint32_t num2;
  };

  static TypeId GetTypeId (void);

  int64_t AssignStreams (int64_t stream);

  Mac48Address GetAddress ();
  double GetNoiseFloor (uint32_t bandwidth) const;
  Ptr<SpcPhy> GetPhy ();
  void SetAddress (Mac48Address);
  void SetNetDevice (Ptr<SpcNetDevice> device);

  void ReceiveOk (Ptr<Packet> packet, double rssi, uint8_t spcNum);
  void ReceiveError (Ptr<Packet> packet);

  void NotifyMaybeCcaBusyStartNow (Time duration);
  void NotifyTxStartNow (Time duration);
  void NotifyRxStartNow (Time duration);
  void SetupPhySpcMacListener (Ptr<SpcPhyStateHelper> state);
  void Enqueue (Ptr<Packet const> packet, const SpcMacHeader &hdr);

  void StartBackoffIfNeeded ();
  void StartBackoff ();
  Time GetBackoffGrantStart (void) const;
  Time GetSendGrantStart (void) const;
  void MakeSpc ();
  uint8_t ConvertRssiToDbm (double rssi) const;
  double ConvertRssiToW (uint8_t rssi) const;

  void UpdateCw ();
  void InitSend ();
  void SetNav (Time duration);

  struct PowerTimeRate CalculatePowerTimeRate (double passLoss1, double passLoss2, uint32_t size1, uint32_t size2, uint32_t bandwidth, bool *isFar);
  struct TimeRate CalculateTimeRate (double passLoss, uint32_t size, uint32_t bandwidth);
  TimeNum1Num2 GetWaitTimeForBuffer (void);
  void SetState (void);

  void SendRts ();
  void SendCtsAfterRts (Mac48Address source, double rssi);
  void SendUnicastDataAfterCts ();

  void SendRtsSpc ();
  void SendCtsSpcAfterRtsSpc (Mac48Address source, double rssi);
  void SendSpcDataAfterCtsSpc ();

  void SendAckAfterData (Mac48Address source, uint8_t spcNum);
  void SendUnicastData ();
  void SendUnicastDataNoAck ();

  void BackoffGrantStart ();
  void BackoffTimeout ();
  void AckTimeout1 ();
  void AckTimeout2 ();
  void CtsTimeout ();

  double CalculateFPowerRate (double passLoss1, double passLoss2, uint32_t size1, uint32_t size2);
  uint32_t CalculateRate (double passLoss1, double passLoss2, uint32_t size1, uint32_t size2);

  void MeasureTrafficEnd ();

private:
  Ptr<NodeInformationTable> m_nodeTable;
  class PhySpcMacListener *m_phySpcMacListener;
  Ptr<SpcPhy> m_phy;
  Ptr<SpcMacQueue> m_queue;
  Ptr<SpcNetDevice> m_device;
  SpcRandomStream *m_rng;

  PacketInfo packetInfo1;
  PacketInfo packetInfo2;

  Mac48Address m_address;
  Ptr<Packet const> m_currentPacket1;
  Ptr<Packet const> m_currentPacket2;
  SpcMacHeader m_currentHdr1;
  SpcMacHeader m_currentHdr2;

  uint32_t m_rtsSendThreshold;

  Time m_maxPropagationDelay;
  Time m_rtsSendAndSifsTime;
  Time m_ctsSendAndSifsTime;
  Time m_ackSendAndSifsTime;
  Time m_rtsNavDuration;

  uint16_t m_resendRtsNum;
  uint16_t m_resendRtsMax;
  uint16_t m_resendDataNum;
  uint16_t m_resendDataMax;
  uint16_t m_recvCtsNum;

  uint32_t m_cwMin;
  uint32_t m_cwMax;
  uint32_t m_cw;
  uint32_t m_backoffSlots;
  Time m_sifs;
  Time m_difs;
  Time m_slotTime;
  Time m_backoffStart;

  Time m_lastRxStart;
  Time m_lastRxDuration;
  Time m_lastBusyStart;
  Time m_lastBusyDuration;
  Time m_lastTxStart;
  Time m_lastTxDuration;
  Time m_lastNavStart;
  Time m_lastNavDuration;
  Time m_lastAckTimeoutEnd;
  Time m_lastCtsTimeoutEnd;
  Time m_waitTime;
  bool m_rxing;

  uint32_t m_rate;
  uint32_t m_minRate;

  Time m_measureTrafficInterval;
  uint32_t m_restrictionPacketNum;

  EventId m_sendCtsAfterRtsEvent;
  EventId m_sendDataAfterCtsEvent;
  EventId m_sendAckAfterDataEvent;
  EventId m_ackTimeoutEvent1;
  EventId m_ackTimeoutEvent2;
  EventId m_ctsTimeoutEvent;
  EventId m_backoffTimeoutEvent;
  EventId m_backoffGrantStartEvent;
  EventId m_measureTrafficEvent;

  double m_powerRate;
  uint8_t m_sendState;
  bool m_unicast;

  TimeNum1Num2 m_tnn;
};

} // namespace ns3

#endif /* SPC_MAC_H */

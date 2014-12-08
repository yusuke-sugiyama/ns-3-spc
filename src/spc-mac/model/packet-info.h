/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef PACKET_INFO_H
#define PACKET_INFO_H


#include <stdint.h>
#include "ns3/packet.h"
#include "ns3/udp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/llc-snap-header.h"
#include "ns3/arp-header.h"
#include "ns3/icmpv4.h"

namespace ns3 {

class PacketInfo
{
public:
  ~PacketInfo ();
  PacketInfo ();
  Ptr<Packet> CreatePacket ();
  void SetPacketInfo (Ptr<Packet> packet);
  uint16_t GetDestPort ();
  uint16_t GetSize ();
  void SetSize (uint16_t size);
private:
  UdpHeader     udp;
  ArpHeader     arp;
  Ipv4Header    ipv4;
  LlcSnapHeader llc;
  Icmpv4Echo    icmpEcho;
  Icmpv4Header  icmp;
  Icmpv4DestinationUnreachable icmpUn;
  Icmpv4TimeExceeded icmpTE;
  bool isIcmp;
  bool isIcmpUn;
  bool isIcmpEcho;
  bool isIcmpTE;
  bool isUdp;
  bool isArp;
  bool isIpv4;
  bool isLlc;
  uint32_t size;
};

} // namespace ns3

#endif /* PACKET_INFO_H */

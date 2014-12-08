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
#include "packet-info.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("PacketInfo");

namespace ns3 {

PacketInfo::~PacketInfo ()
{
}

PacketInfo::PacketInfo ()
{
}

Ptr<Packet>
PacketInfo::CreatePacket ()
{
  Ptr<Packet> packet = Create<Packet>(size);
  uint16_t tmpSize = size;
  if (isUdp){
    NS_LOG_DEBUG (udp);
    packet->AddHeader (udp);
    tmpSize += udp.GetSerializedSize ();
  }

  if (isArp){
    NS_LOG_DEBUG (arp);
    packet->AddHeader (arp);
    tmpSize += arp.GetSerializedSize ();
  }
  if (isIcmp){
    NS_LOG_DEBUG (icmp);
    packet->AddHeader (icmp);
    tmpSize += icmp.GetSerializedSize ();
  }
  if (isIcmpUn){
    NS_LOG_DEBUG (icmpUn);
    packet->AddHeader (icmpUn);
    tmpSize += 4 + 5 * 4 + 8;
  }
  if (isIcmpEcho){
    NS_LOG_DEBUG (icmpEcho);
    packet->AddHeader (icmpEcho);
    tmpSize += icmpEcho.GetSerializedSize ();
  }
  if (isIcmpTE){
    NS_LOG_DEBUG (icmpTE);
    packet->AddHeader (icmpTE);
    tmpSize += icmpTE.GetSerializedSize ();
  }
  if (isIpv4){
    NS_LOG_DEBUG (ipv4);
    ipv4.SetPayloadSize (tmpSize);
    packet->AddHeader (ipv4);
  }
  if (isLlc){
    NS_LOG_DEBUG (llc);
    packet->AddHeader (llc);
  }

  NS_LOG_DEBUG (packet->GetSize ());
  return packet;
}


void
PacketInfo::SetPacketInfo (Ptr<Packet> packet)
{
  PacketMetadata::ItemIterator it = packet->BeginItem ();
  PacketMetadata::Item item;
  
  isUdp  = false;
  isArp  = false;
  isIpv4 = false;
  isLlc  = false;
  isIcmp     = false;
  isIcmpUn   = false;
  isIcmpEcho = false;
  isIcmpTE   = false;
  
  uint32_t headerSize = 0;
  uint32_t packetSize = packet->GetSize ();
  while (it.HasNext ())
    {
      item = it.Next ();
      if (item.type == PacketMetadata::Item::HEADER)
	{
	  NS_LOG_DEBUG (item.tid.GetName ());
	  if (item.tid.GetName () == "ns3::LlcSnapHeader")
	    {
	      NS_LOG_INFO ("LlcSnap header found.");
	      packet->PeekHeader (llc);
	      NS_LOG_DEBUG(llc);
	      headerSize += llc.GetSerializedSize ();
	      isLlc = true;
	    }
	  
	  if (item.tid.GetName () == "ns3::ArpHeader")
	    {
	      NS_LOG_INFO ("Arp header found.");
	      packet->RemoveHeader (llc);
	      packet->RemoveHeader (arp);
	      NS_LOG_DEBUG(arp);
	      headerSize += arp.GetSerializedSize ();
	      isArp = true;
	    }
	  
	  if (item.tid.GetName () == "ns3::Ipv4Header")
	    {
	      NS_LOG_INFO ("IPv4 header found.");
	      ipv4.Deserialize(item.current);
	      NS_LOG_DEBUG(ipv4);
	      headerSize += ipv4.GetSerializedSize ();
	      isIpv4 = true;
	    }
	  
	  if (item.tid.GetName () == "ns3::UdpHeader")
	    {
	      NS_LOG_INFO ("Udp header found.");
	      packet->PeekHeader (udp);
	      NS_LOG_DEBUG(udp);
	      headerSize += udp.GetSerializedSize ();
	      isUdp = true;
	    }

	  if (item.tid.GetName () == "ns3::Icmpv4Header")
	    {
	      NS_LOG_INFO ("Icmpv4 header found.");
	      packet->RemoveHeader (llc);
	      packet->RemoveHeader (ipv4);
	      packet->RemoveHeader (icmp);
	      NS_LOG_DEBUG(icmp);
	      headerSize += icmp.GetSerializedSize ();
	      isIcmp = true;
	    }

	  if (item.tid.GetName () == "ns3::Icmpv4DestinationUnreachable")
	    {
	      NS_LOG_INFO ("Icmpv4DestinationUnreachable found.");
	      packet->PeekHeader (icmpUn);
	      NS_LOG_DEBUG(icmpUn);
	      headerSize += 4 + 5 * 4 + 8;
	      isIcmpUn = true;
	    }

	  if (item.tid.GetName () == "ns3::Icmpv4Echo")
	    {
	      NS_LOG_INFO ("Icmpv4Echo found.");
	      packet->PeekHeader (icmpEcho);
	      NS_LOG_DEBUG(icmpEcho);
	      headerSize += icmpEcho.GetSerializedSize ();
	      isIcmpEcho = true;
	    }

	  if (item.tid.GetName () == "ns3::Icmpv4TimeExceeded")
	    {
	      NS_LOG_INFO ("Icmpv4TimeExceeded found.");
	      packet->PeekHeader (icmpTE);
	      NS_LOG_DEBUG(icmpTE);
	      headerSize += icmpTE.GetSerializedSize ();
	      isIcmpTE = true;
	    }

	}
    }
  size = packetSize -headerSize;
}

uint16_t
PacketInfo::GetDestPort ()
{
  return udp.GetDestinationPort ();
}

uint16_t
PacketInfo::GetSize ()
{
  return size;
}

void
PacketInfo::SetSize (uint16_t s)
{
  size = s;
}

} // namespace ns3

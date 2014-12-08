/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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

#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/llc-snap-header.h"
#include "spc-net-device.h"

NS_LOG_COMPONENT_DEFINE ("SpcNetDevice");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SpcNetDevice);

TypeId 
SpcNetDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpcNetDevice")
    .SetParent<NetDevice> ()
    .AddConstructor<SpcNetDevice> ()
    .AddTraceSource ("PhyRxDrop",
                     "Trace source indicating a packet has been dropped by the device during reception",
                     MakeTraceSourceAccessor (&SpcNetDevice::m_phyRxDropTrace))
  ;
  return tid;
}

SpcNetDevice::SpcNetDevice ()
  : m_node (0),
    m_mtu (0xffff),
    m_ifIndex (0)
{
  NS_LOG_FUNCTION (this);
  SetMac (CreateObject<SpcMac>());
  GetMac ()->SetNetDevice (this);
  SetPhy (GetMac()->GetPhy());
}
void
SpcNetDevice::SetPhy (Ptr<SpcPhy> phy)
{
  m_phy = phy;
}
void
SpcNetDevice::SetMac (Ptr<SpcMac> mac)
{
  m_mac = mac;
}
Ptr<SpcPhy>
SpcNetDevice::GetPhy ()
{
  return m_phy;
}
Ptr<SpcMac>
SpcNetDevice::GetMac ()
{
  return m_mac;
}

void
SpcNetDevice::Receive (Ptr<Packet> packet, Mac48Address to, Mac48Address from)
{
  NS_LOG_FUNCTION (this << packet << to << from);
  NetDevice::PacketType packetType;
  LlcSnapHeader llc;
  packet->RemoveHeader (llc);
  NS_LOG_DEBUG (llc.GetType ());
  
  if (to == m_address)
    {
      packetType = NetDevice::PACKET_HOST;
    }
  else if (to.IsBroadcast ())
    {
      packetType = NetDevice::PACKET_HOST;
    }
  else if (to.IsGroup ())
    {
      packetType = NetDevice::PACKET_MULTICAST;
    }
  else 
    {
      packetType = NetDevice::PACKET_OTHERHOST;
      return;
    }
  m_rxCallback (this, packet, llc.GetType (), from);
  if (!m_promiscCallback.IsNull ())
    {
      m_promiscCallback (this, packet, llc.GetType (), from, to, packetType);
    }
}

void 
SpcNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this << index);
  m_ifIndex = index;
}
uint32_t 
SpcNetDevice::GetIfIndex (void) const
{
  NS_LOG_FUNCTION (this);
  return m_ifIndex;
}
Ptr<Channel>
SpcNetDevice::GetChannel (void) const
{
  return m_phy->GetChannel ();
}
void
SpcNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
  m_mac->SetAddress (m_address);
}
Address 
SpcNetDevice::GetAddress (void) const
{
  //
  // Implicit conversion from Mac48Address to Address
  //
  NS_LOG_FUNCTION (this);
  return m_address;
}
bool 
SpcNetDevice::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}
uint16_t 
SpcNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}
bool 
SpcNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}
void 
SpcNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
 NS_LOG_FUNCTION (this << &callback);
}
bool 
SpcNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}
Address
SpcNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}
bool 
SpcNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}
Address 
SpcNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this << multicastGroup);
  return Mac48Address::GetMulticast (multicastGroup);
}

Address SpcNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address::GetMulticast (addr);
}

bool 
SpcNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool 
SpcNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool 
SpcNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);

  LlcSnapHeader llc;
  llc.SetType (protocolNumber);
  packet->AddHeader (llc);

  SpcMacHeader hdr;
  Mac48Address to = Mac48Address::ConvertFrom (dest);
  hdr.SetType (SPC_MAC_DATA);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (m_mac->GetAddress ());
  m_mac->Enqueue (packet, hdr);

  return true;
}
bool 
SpcNetDevice::SendFrom (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
  //  Mac48Address to = Mac48Address::ConvertFrom (dest);
  //  Mac48Address from = Mac48Address::ConvertFrom (source);
  //  m_mac->Send (packet, protocolNumber, to, from);
  return true;
}

Ptr<Node> 
SpcNetDevice::GetNode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_node;
}
void 
SpcNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this << node);
  m_node = node;
}
bool 
SpcNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}
void 
SpcNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_rxCallback = cb;
}

void
SpcNetDevice::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_node = 0;
  m_mac = 0;
  NetDevice::DoDispose ();
}


void
SpcNetDevice::SetPromiscReceiveCallback (PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (this << &cb);
  m_promiscCallback = cb;
}

bool
SpcNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

} // namespace ns3

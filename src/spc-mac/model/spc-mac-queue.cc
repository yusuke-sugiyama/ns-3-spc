/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

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

#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "spc-mac.h"
#include "spc-mac-queue.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SpcMacQueue);

SpcMacQueue::Item::Item (Ptr<const Packet> packet,
                          const SpcMacHeader &hdr,
                          Time tstamp)
  : packet (packet),
    hdr (hdr),
    tstamp (tstamp)
{
}

TypeId
SpcMacQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpcMacQueue")
    .SetParent<Object> ()
    .AddConstructor<SpcMacQueue> ()
    .AddAttribute ("MaxPacketNumber", "If a packet arrives when there are already this number of packets, it is dropped.",
                   UintegerValue (400),
                   MakeUintegerAccessor (&SpcMacQueue::m_maxSize),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

SpcMacQueue::SpcMacQueue ()
  : m_size (0)
{
}

SpcMacQueue::~SpcMacQueue ()
{
  Flush ();
}

uint32_t
SpcMacQueue::Aggregation (Mac48Address addr, uint16_t port, uint32_t pktNum)
{
  PacketQueueI it = m_queue.begin ();
  uint32_t cutNum = 0;
  uint32_t size = 0;
  pktNum--;
  while (it != m_queue.end ())
    {
      SpcMacHeader hdr = it->hdr;
      PacketInfo packetInfo;
      packetInfo.SetPacketInfo (it->packet->Copy ());
      if (hdr.GetAddr1 () ==  addr && packetInfo.GetDestPort () == port && pktNum > cutNum)
        {
          cutNum ++;
          size += packetInfo.GetSize ();
          m_queue.erase (it++);
          m_size--;
        }
      else{
        it++;
      }
    }
  return size;
}

void
SpcMacQueue::SetMaxSize (uint32_t maxSize)
{
  m_maxSize = maxSize;
}


uint32_t
SpcMacQueue::GetMaxSize (void) const
{
  return m_maxSize;
}

void
SpcMacQueue::Enqueue (Ptr<const Packet> packet, const SpcMacHeader &hdr)
{
  Cleanup ();
  if (m_size == m_maxSize)
    {
      return;
    }
  Time now = Simulator::Now ();
  m_queue.push_back (Item (packet, hdr, now));
  if (!hdr.GetAddr1 ().IsGroup ())
    {
      m_nodeTable->AddSize (hdr.GetAddr1 (), packet->GetSize ());
    }
  m_size++;
}

Ptr<const Packet>
SpcMacQueue::Dequeue (SpcMacHeader *hdr)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      Item i = m_queue.front ();
      m_queue.pop_front ();
      m_size--;
      *hdr = i.hdr;
      return i.packet;
    }
  return 0;
}

Ptr<const Packet>
SpcMacQueue::Peek (SpcMacHeader *hdr)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      Item i = m_queue.front ();
      *hdr = i.hdr;
      return i.packet;
    }
  return 0;
}

bool
SpcMacQueue::IsEmpty (void)
{
  Cleanup ();
  return m_queue.empty ();
}

uint32_t
SpcMacQueue::GetSize (void)
{
  return m_size;
}

void
SpcMacQueue::SetNodeTable (Ptr<NodeInformationTable> nodeTable)
{
  m_nodeTable = nodeTable;
}

void
SpcMacQueue::Flush (void)
{
  m_queue.erase (m_queue.begin (), m_queue.end ());
  m_size = 0;
}

bool
SpcMacQueue::Remove (Ptr<const Packet> packet)
{
  PacketQueueI it = m_queue.begin ();
  for (; it != m_queue.end (); it++)
    {
      if (it->packet == packet)
        {
          m_queue.erase (it);
          m_size--;
          return true;
        }
    }
  return false;
}

} // namespace ns3

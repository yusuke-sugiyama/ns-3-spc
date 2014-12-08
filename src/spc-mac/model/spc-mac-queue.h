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

#ifndef SPC_MAC_QUEUE_H
#define SPC_MAC_QUEUE_H

#include <list>
#include <utility>
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "spc-mac-header.h"
#include "node-information-table.h"


namespace ns3 {
class SpcMac;

class SpcMacQueue : public Object
{
public:
  static TypeId GetTypeId (void);
  SpcMacQueue ();
  ~SpcMacQueue ();

  void SetMaxSize (uint32_t maxSize);
  uint32_t GetMaxSize (void) const;
  void Enqueue (Ptr<const Packet> packet, const SpcMacHeader &hdr);
  void PushFront (Ptr<const Packet> packet, const SpcMacHeader &hdr);
  Ptr<const Packet> Dequeue (SpcMacHeader *hdr);
  Ptr<const Packet> Peek (SpcMacHeader *hdr);
  bool Remove (Ptr<const Packet> packet);
  void Flush (void);
  bool IsEmpty (void);
  uint32_t GetSize (void);

  void SetNodeTable(Ptr<NodeInformationTable> nodeTable);
  uint32_t Aggregation (Mac48Address addr, uint16_t port, uint32_t pktNum);
protected:

  struct Item;

  typedef std::list<struct Item> PacketQueue;
  typedef std::list<struct Item>::reverse_iterator PacketQueueRI;
  typedef std::list<struct Item>::iterator PacketQueueI;

  struct Item
  {
    Item (Ptr<const Packet> packet,
          const SpcMacHeader &hdr,
          Time tstamp);
    Ptr<const Packet> packet;
    SpcMacHeader hdr;
    Time tstamp;
  };

  Ptr<NodeInformationTable> m_nodeTable;
  PacketQueue m_queue;
  uint32_t m_size;
  uint32_t m_maxSize;
};

} // namespace ns3

#endif /* SPC_MAC_QUEUE_H */

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

#include "ns3/assert.h"
#include "ns3/address-utils.h"
#include "spc-mac-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (SpcMacHeader);

enum
{
  TYPE_DATA = 0,
  TYPE_RTS  = 1,
  TYPE_CTS  = 2,
  TYPE_DATA_SPC = 3,
  TYPE_RTS_SPC  = 4,
  TYPE_CTS_SPC  = 5,
  TYPE_ACK  = 6,
};

SpcMacHeader::SpcMacHeader ()
{
}
SpcMacHeader::~SpcMacHeader ()
{
}

void
SpcMacHeader::SetAddr1 (Mac48Address address)
{
  m_addr1 = address;
}

void
SpcMacHeader::SetAddr2 (Mac48Address address)
{
  m_addr2 = address;
}

void
SpcMacHeader::SetAddr3 (Mac48Address address)
{
  m_addr3 = address;
}

void
SpcMacHeader::SetType (enum SpcMacType type)
{
  switch (type)
    {
    case SPC_MAC_DATA:
      m_ctrlType = TYPE_DATA;
      break;
    case SPC_MAC_RTS:
      m_ctrlType = TYPE_RTS;
      break;
    case SPC_MAC_CTS:
      m_ctrlType = TYPE_CTS;
      break;
    case SPC_MAC_DATA_SPC:
      m_ctrlType = TYPE_DATA_SPC;
      break;
    case SPC_MAC_RTS_SPC:
      m_ctrlType = TYPE_RTS_SPC;
      break;
    case SPC_MAC_CTS_SPC:
      m_ctrlType = TYPE_CTS_SPC;
      break;
    case SPC_MAC_ACK:
      m_ctrlType = TYPE_ACK;
      break;
    }
}

void
SpcMacHeader::SetSpcNum (uint8_t spcNum)
{
  m_spcNum = spcNum;
}

void
SpcMacHeader::SetDuration (Time duration)
{
  int64_t duration_us = duration.GetMicroSeconds ();
  NS_ASSERT (duration_us >= 0 && duration_us <= 0x7fff);
  m_duration = static_cast<uint16_t> (duration_us);
}

void
SpcMacHeader::SetRtsRssi (uint8_t rssi)
{
  m_rtsRssi = rssi;
}

Mac48Address
SpcMacHeader::GetAddr1 (void) const
{
  return m_addr1;
}

Mac48Address
SpcMacHeader::GetAddr2 (void) const
{
  return m_addr2;
}

Mac48Address
SpcMacHeader::GetAddr3 (void) const
{
  return m_addr3;
}


enum SpcMacType
SpcMacHeader::GetType (void) const
{
  switch (m_ctrlType)
    {
    case TYPE_DATA:
      return SPC_MAC_DATA;
      break;
    case TYPE_RTS:
      return SPC_MAC_RTS;
      break;
    case TYPE_CTS:
      return SPC_MAC_CTS;
      break;
    case TYPE_DATA_SPC:
      return SPC_MAC_DATA_SPC;
      break;
    case TYPE_RTS_SPC:
      return SPC_MAC_RTS_SPC;
      break;
    case TYPE_CTS_SPC:
      return SPC_MAC_CTS_SPC;
      break;
    case TYPE_ACK:
      return SPC_MAC_ACK;
      break;
    }
  NS_ASSERT (false);
  return (enum SpcMacType)-1;
}

uint8_t
SpcMacHeader::GetSpcNum () const
{
  return m_spcNum;
}

Time
SpcMacHeader::GetDuration (void) const
{
  return MicroSeconds (m_duration);
}

uint8_t
SpcMacHeader::GetRtsRssi (void) const
{
  return m_rtsRssi;
}

uint32_t
SpcMacHeader::GetSize (void) const
{
  uint32_t size = 0;
  switch (m_ctrlType)
    {
    case TYPE_DATA:
      size = 2 + 2 + 6 + 6;
      break;
    case TYPE_RTS:
      size = 2 + 2 + 6 + 6;
      break;
    case TYPE_CTS:
      size = 2 + 2 + 1 + 6;
      break;
    case TYPE_DATA_SPC:
      size = 2 + 2 + 6 + 6 + 6;
      break;
    case TYPE_RTS_SPC:
      size = 2 + 2 + 6 + 6;
      break;
    case TYPE_CTS_SPC:
      size = 2 + 2 + 1 + 6;
      break;
    case TYPE_ACK:
      size = 2 + 2 + 6;
      break;
    }
  return size;
}

TypeId
SpcMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SpcMacHeader")
    .SetParent<Header> ()
    .AddConstructor<SpcMacHeader> ()
  ;
  return tid;
}

TypeId
SpcMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
SpcMacHeader::Print (std::ostream &os) const
{
  os << "type=" << GetType ();
  os << ", Duration/ID=" << m_duration << "us";
  switch (GetType ())
    {
    case TYPE_DATA:
      os << ", DA=" << m_addr1 << ", SA=" << m_addr2;
      break;
    case TYPE_RTS:
      os << ", DA=" << m_addr1 << ", SA=" << m_addr2;
      break;
    case TYPE_CTS:
      os <<  ", DA=" << m_addr1 << ", RSSI=" << m_rtsRssi;
      break;
    case TYPE_DATA_SPC:
      os << ", DA1=" << m_addr1 << ", DA2=" << m_addr2 << ", SA=" << m_addr3;
      break;
    case TYPE_RTS_SPC:
      os << ", DA1=" << m_addr1 << ", DA2=" << m_addr2;
      break;
    case TYPE_CTS_SPC:
      os <<  ", DA=" << m_addr1 << ", RSSI=" << m_rtsRssi;
      break;
    case TYPE_ACK:
      os << ", DA=" << m_addr1;
      break;
    }
}
uint16_t
SpcMacHeader::GetFrameControl (void) const
{
  uint16_t val = 0;
  val |= m_ctrlType & 0x7;
  val |= (m_spcNum << 3) & (0x1 << 3);
  return val;
}
void
SpcMacHeader::SetFrameControl (uint16_t ctrl)
{
  m_ctrlType = ctrl & 0x07;
  m_spcNum   = (ctrl >> 3) & 0x01;
}
uint32_t
SpcMacHeader::GetSerializedSize (void) const
{
  return GetSize ();
}
void
SpcMacHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteHtolsbU16 (GetFrameControl ());
  i.WriteHtolsbU16 (m_duration);
  WriteTo (i, m_addr1);
  switch (m_ctrlType)
    {
    case TYPE_DATA:
      WriteTo (i, m_addr2);
      break;
    case TYPE_RTS:
      WriteTo (i, m_addr2);
      break;
    case TYPE_CTS:
      i.WriteU8 (m_rtsRssi);
      break;
    case TYPE_DATA_SPC:
      WriteTo (i, m_addr2);
      WriteTo (i, m_addr3);
      break;
    case TYPE_RTS_SPC:
      WriteTo (i, m_addr2);
      break;
    case TYPE_CTS_SPC:
      i.WriteU8 (m_rtsRssi);
      break;
    case TYPE_ACK:
      // do nothing
      break;
    }
}

uint32_t
SpcMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t frame_control = i.ReadLsbtohU16 ();
  SetFrameControl (frame_control);
  m_duration = i.ReadLsbtohU16 ();
  ReadFrom (i, m_addr1);
  switch (m_ctrlType)
    {
    case TYPE_DATA:
      ReadFrom (i, m_addr2);
      break;
    case TYPE_RTS:
      ReadFrom (i, m_addr2);
      break;
    case TYPE_CTS:
      m_rtsRssi = i.ReadU8 ();
      break;
    case TYPE_DATA_SPC:
      ReadFrom (i, m_addr2);
      ReadFrom (i, m_addr3);
      break;
    case TYPE_RTS_SPC:
      ReadFrom (i, m_addr2);
      break;
    case TYPE_CTS_SPC:
      m_rtsRssi = i.ReadU8 ();
      break;
    case TYPE_ACK:
      // do nothing
      break;
    }
  return i.GetDistanceFrom (start);
}

} // namespace ns3

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

#ifndef NODE_INFORMATION_TABLE_H
#define NODE_INFORMATION_TABLE_H

#include <stdint.h>
#include <vector>
#include "ns3/object.h"
#include "ns3/mac48-address.h"
#include "ns3/nstime.h"

namespace ns3 {

class NodeInformationItem
{
public:

  NodeInformationItem (Mac48Address address, double passLoss, uint32_t traffic, uint32_t size);
  ~NodeInformationItem ();

  Mac48Address GetAddress (void);
  double GetPassLoss  (void);
  uint32_t GetTraffic (void);
  uint32_t GetSize (void);
  void SetPassLoss(double passLoss);
  void SetTraffic (uint32_t traffic);
  void AddSize (uint32_t size);
  void ResetSize ();
  
private:
  Mac48Address m_address;
  double m_passLoss;
  uint32_t m_traffic;
  uint32_t m_size;
};

class NodeInformationTable : public Object
{
public:
  static TypeId GetTypeId (void);

  NodeInformationTable();
  ~NodeInformationTable();

  void AddItem(Mac48Address address, double passLoss, uint32_t traffic, uint32_t size);
  void InitItem();
  bool IsExistsAddress(Mac48Address address);
  void UpdatePassLoss(Mac48Address address, double passLoss);
  void UpdateTraffic (Mac48Address address, uint32_t traffic);
  void AddSize (Mac48Address address, uint32_t size);
  void ResetSize();
  double GetPassLoss(Mac48Address address);
  uint32_t GetTraffic(Mac48Address address);
  void UpdateTraffic(Time interval);

;
private:
  std::vector<NodeInformationItem*> Items;
};

} // namespace ns3


#endif /* NODE_INFORMATION_TABLE_H */

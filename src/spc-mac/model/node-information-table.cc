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

#include <vector>
#include "ns3/log.h"
#include "node-information-table.h"
#include "ns3/random-variable-stream.h"
#include "ns3/mac48-address.h"
#include "ns3/double.h"


NS_LOG_COMPONENT_DEFINE ("NodeInformationTable");

namespace ns3 {

TypeId
NodeInformationTable::GetTypeId (void)
{
  static TypeId tid = TypeId ("NodeInformationTable")
    .SetParent<Object> ()
    .AddConstructor<NodeInformationTable> ()
    ;
  return tid;
}

NodeInformationTable::NodeInformationTable ()
{
}

NodeInformationTable::~NodeInformationTable ()
{
  InitItem();
}

void
NodeInformationTable::AddItem(Mac48Address address, double passLoss, uint32_t traffic, uint32_t size)
{
  NS_LOG_FUNCTION(this << address << passLoss << traffic);
  Items.push_back(new NodeInformationItem(address, passLoss, traffic, size));
}
  
void
NodeInformationTable::InitItem()
{
  Items.clear();
}
bool
NodeInformationTable::IsExistsAddress(Mac48Address address)
{
  NS_LOG_FUNCTION(this);
  for(unsigned int i = 0; i < Items.size(); i++)
    {
      NS_LOG_DEBUG(this << Items[i]->GetAddress());
      if(Items[i]->GetAddress() == address)
	{
	  return true;
	}
    }
  return false;
}
  
void
NodeInformationTable::UpdatePassLoss(Mac48Address address, double passLoss)
{
  NS_LOG_FUNCTION(this);
  for(uint32_t i = 0; i < Items.size(); i++)
    {
      if(Items[i]->GetAddress() == address)
	{
	  Items[i]->SetPassLoss(passLoss);
	  return;
	}
    }
  AddItem(address, passLoss, 0, 0);
}

void
NodeInformationTable::UpdateTraffic(Mac48Address address, uint32_t traffic)
{
  NS_LOG_FUNCTION(this);
  for(unsigned int i = 0; i < Items.size(); i++)
    {
      if(Items[i]->GetAddress() == address)
	{
	  Items[i]->SetTraffic (traffic);
	  return;
	}
    }
  AddItem(address, 0, traffic, 0);
}

void
NodeInformationTable::AddSize (Mac48Address address, uint32_t size)
{
  NS_LOG_FUNCTION(this);
  for(unsigned int i = 0; i < Items.size(); i++)
    {
      if(Items[i]->GetAddress() == address)
	{
	  Items[i]->AddSize (size);
	  return;
	}
    }
  AddItem(address, 0, 0, size);
}

double
NodeInformationTable::GetPassLoss(Mac48Address address)
{
  NS_LOG_FUNCTION(this << address);
  for(unsigned int i = 0; i < Items.size(); i++)
    {
      NS_LOG_DEBUG(this << Items[i]->GetAddress());
      if(Items[i]->GetAddress() == address)
	{
	  return Items[i]->GetPassLoss ();
	}
    }
  return -1;
}

uint32_t
NodeInformationTable::GetTraffic(Mac48Address address)
{
  NS_LOG_FUNCTION(this << address);
  for(unsigned int i = 0; i < Items.size(); i++)
    {
      NS_LOG_DEBUG(this << Items[i]->GetAddress());
      if(Items[i]->GetAddress() == address)
	{
	  return Items[i]->GetTraffic ();
	}
    }
  return -1;
}

void
NodeInformationTable::ResetSize()
{
  NS_LOG_FUNCTION(this);
  for(uint32_t i = 0; i < Items.size(); i++)
    {
      Items[i]->ResetSize();
    }
}

void
NodeInformationTable::UpdateTraffic(Time interval)
{
  NS_LOG_FUNCTION(this);
  for(uint32_t i = 0; i < Items.size(); i++)
    {
      uint32_t traffic = Items[i]->GetSize() / interval.GetSeconds ();
      UpdateTraffic (Items[i]->GetAddress (), traffic);
    }
}

NodeInformationItem::NodeInformationItem (Mac48Address address, double passLoss, uint32_t traffic, uint32_t size)
{
  m_address  = address;
  m_passLoss = passLoss;
  m_traffic  = traffic;
  m_size     = size;
}

NodeInformationItem::~NodeInformationItem ()
{
}

Mac48Address
NodeInformationItem::GetAddress ()
{
  return m_address;
}

double
NodeInformationItem::GetPassLoss ()
{
  return m_passLoss;
}

uint32_t
NodeInformationItem::GetTraffic ()
{
  return m_traffic;
}

uint32_t
NodeInformationItem::GetSize ()
{
  return m_size;
}

void
NodeInformationItem::SetPassLoss(double passLoss)
{
  m_passLoss = passLoss;
}

void
NodeInformationItem::SetTraffic (uint32_t traffic)
{
  m_traffic = traffic;
}

void
NodeInformationItem::ResetSize()
{
  m_size = 0;
}

void
NodeInformationItem::AddSize(uint32_t size)
{
  m_size += size;
}
} // namespace ns3

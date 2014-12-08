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

#include "spc-preamble.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("SpcPreamble");

namespace ns3 {

SpcPreamble::SpcPreamble ()
  : m_rate (6000000 / 8),
    m_bandwidth (20000000),
    m_duration (MicroSeconds (24))
{
}

SpcPreamble::~SpcPreamble ()
{
}

void
SpcPreamble::SetPower (double power){
  m_power = power;
}

void
SpcPreamble::SetIsFar (bool isFar){
  m_isFar = isFar;
}

void
SpcPreamble::SetRate (uint32_t rate){
  m_rate = rate;
}

void
SpcPreamble::SetBandwidth (uint32_t bandwidth){
  m_bandwidth = bandwidth;
}

void
SpcPreamble::SetDuration (Time duration){
  m_duration = duration;
}

void
SpcPreamble::SetSymbols (uint32_t length){
  m_symbols = length;
}

void
SpcPreamble::SetNLength (uint32_t length){
  m_nLength = length;
}

void
SpcPreamble::SetFLength (uint32_t length){
  m_fLength = length;
}

double
SpcPreamble::GetPower (){
  return m_power;
}

uint32_t
SpcPreamble::GetRate (){
  return m_rate;
}

bool
SpcPreamble::GetIsFar (){
  return m_isFar;
}

uint32_t
SpcPreamble::GetBandwidth (){
  return m_bandwidth;
}

Time
SpcPreamble::GetDuration (){
  return m_duration;
}

uint32_t
SpcPreamble::GetSymbols (){
  return m_symbols;
}

uint32_t
SpcPreamble::GetNLength (){
  return m_nLength;
}

uint32_t
SpcPreamble::GetFLength (){
  return m_fLength;
}

}

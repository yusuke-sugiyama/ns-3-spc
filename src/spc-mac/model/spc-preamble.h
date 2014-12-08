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

#ifndef SPC_PREAMBLE_H
#define SPC_PREAMBLE_H

#include <stdint.h>
#include "ns3/nstime.h"


namespace ns3 {

class SpcPreamble
{
public:
  SpcPreamble ();
  ~SpcPreamble ();
  void SetRate (uint32_t rate);
  void SetPower (double power);
  void SetBandwidth (uint32_t bandwidth);
  void SetSymbols (uint32_t length);
  void SetNLength (uint32_t length);
  void SetFLength (uint32_t length);
  void SetDuration (Time duration);
  uint32_t GetRate ();
  double GetPower ();
  uint32_t GetBandwidth ();
  Time GetDuration ();
  uint32_t GetSymbols ();
  uint32_t GetNLength ();
  uint32_t GetFLength ();
private:
  uint32_t m_rate;
  uint32_t m_bandwidth;
  double m_power;
  /*
    0. preamble + layer 1 header
      preamble      : 12 [symbols] 16 [us]
      layer 1 header:  1 [symbols]  4 [us]

    1. layer 1 header = |Rate|Symbols|N Length|F Length|Tail|
      Rate:      6 [bits]
      Symbols:  12 [bits]
      N Length: 12 [bits]
      F Length: 12 [bits]
      Tail:      6 [bits]
      Total:    48 [bits]
   */
  Time m_duration;
  uint32_t m_symbols;
  uint32_t m_nLength;
  uint32_t m_fLength;
};
}

#endif /* SPC_PREAMBLE_H */

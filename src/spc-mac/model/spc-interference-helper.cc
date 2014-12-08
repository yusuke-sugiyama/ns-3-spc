/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
#include "spc-interference-helper.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <algorithm>
#include "ns3/math.h"

NS_LOG_COMPONENT_DEFINE ("SpcInterferenceHelper");

namespace ns3 {

/****************************************************************
 *       Phy event class
 ****************************************************************/

SpcInterferenceHelper::Event::Event (uint32_t size, Time duration, double rxPower, SpcPreamble preamble)
  : m_size (size),
    m_startTime (Simulator::Now ()),
    m_endTime (m_startTime + duration),
    m_rxPowerW (rxPower),
    m_preamble (preamble)
    
{
}
SpcInterferenceHelper::Event::~Event ()
{
}

Time
SpcInterferenceHelper::Event::GetDuration (void) const
{
  return m_endTime - m_startTime;
}
Time
SpcInterferenceHelper::Event::GetStartTime (void) const
{
  return m_startTime;
}
Time
SpcInterferenceHelper::Event::GetEndTime (void) const
{
  return m_endTime;
}
double
SpcInterferenceHelper::Event::GetRxPowerW (void) const
{
  return m_rxPowerW;
}
uint32_t
SpcInterferenceHelper::Event::GetSize (void) const
{
  return m_size;
}
SpcPreamble
SpcInterferenceHelper::Event::GetPreamble (void) const
{
  return m_preamble;
}

/****************************************************************
 *       Class which records SNIR change events for a
 *       short period of time.
 ****************************************************************/

SpcInterferenceHelper::NiChange::NiChange (Time time, double delta)
  : m_time (time),
    m_delta (delta)
{
}
Time
SpcInterferenceHelper::NiChange::GetTime (void) const
{
  return m_time;
}
double
SpcInterferenceHelper::NiChange::GetDelta (void) const
{
  return m_delta;
}
bool
SpcInterferenceHelper::NiChange::operator < (const SpcInterferenceHelper::NiChange& o) const
{
  return (m_time < o.m_time);
}

/****************************************************************
 *       The actual SpcInterferenceHelper
 ****************************************************************/

SpcInterferenceHelper::SpcInterferenceHelper ()
  : m_firstPower (0.0),
    m_rxing (false)
{
}
SpcInterferenceHelper::~SpcInterferenceHelper ()
{
  EraseEvents ();
}

Ptr<SpcInterferenceHelper::Event>
SpcInterferenceHelper::Add (uint32_t size, Time duration, double rxPowerW, SpcPreamble preamble)
{
  Ptr<SpcInterferenceHelper::Event> event;

  event = Create<SpcInterferenceHelper::Event> (size,
                                             duration,
                                             rxPowerW,
                                             preamble);
  AppendEvent (event);
  return event;
}


void
SpcInterferenceHelper::SetNoiseFigure (double value)
{
  m_noiseFigure = value;
}

double
SpcInterferenceHelper::GetNoiseFigure (void) const
{
  return m_noiseFigure;
}

Time
SpcInterferenceHelper::GetEnergyDuration (double energyW)
{
  Time now = Simulator::Now ();
  double noiseInterferenceW = 0.0;
  Time end = now;
  noiseInterferenceW = m_firstPower;
  for (NiChanges::const_iterator i = m_niChanges.begin (); i != m_niChanges.end (); i++)
    {
      noiseInterferenceW += i->GetDelta ();
      end = i->GetTime ();
      if (end < now)
        {
          continue;
        }
      if (noiseInterferenceW < energyW)
        {
          break;
        }
    }
  return end > now ? end - now : MicroSeconds (0);
}

void
SpcInterferenceHelper::AppendEvent (Ptr<SpcInterferenceHelper::Event> event)
{
  Time now = Simulator::Now ();
  SpcPreamble pre = event->GetPreamble();
  if (!m_rxing)
    {
      NiChanges::iterator nowIterator = GetPosition (now);
      for (NiChanges::iterator i = m_niChanges.begin (); i != nowIterator; i++)
        {
          m_firstPower += i->GetDelta ();
        }
      m_niChanges.erase (m_niChanges.begin (), nowIterator);
      m_niChanges.insert (m_niChanges.begin (), NiChange (event->GetStartTime (), event->GetRxPowerW ()));
    }
  else
    {
      AddNiChangeEvent (NiChange (event->GetStartTime (), event->GetRxPowerW ()));
    }
  AddNiChangeEvent (NiChange (event->GetEndTime (), -event->GetRxPowerW ()));

}


double
SpcInterferenceHelper::CalculateSnr (double signal, double noiseInterference, SpcPreamble preamble) const
{
  // thermal noise at 290K in J/s = W
  static const double BOLTZMANN = 1.3803e-23;
  // Nt is the power of thermal noise in W
  double Nt = BOLTZMANN * 290.0 * preamble.GetBandwidth ();
  // receiver noise Floor (W) which accounts for thermal noise and non-idealities of the receiver
  double noiseFloor = m_noiseFigure * Nt;
  double noise = noiseFloor + noiseInterference;
  double snr = signal / noise;
  NS_LOG_DEBUG ("signal=" << signal <<
                "noise="  << noise  <<
                "rate="   << signal / noise);
  return snr;
}

double
SpcInterferenceHelper::CalculateNoiseInterferenceW (Ptr<SpcInterferenceHelper::Event> event, NiChanges *ni) const
{
  double noiseInterference = m_firstPower;
  NS_ASSERT (m_rxing);
  for (NiChanges::const_iterator i = m_niChanges.begin () + 1; i != m_niChanges.end (); i++)
    {
      if ((event->GetEndTime () == i->GetTime ()) && event->GetRxPowerW () == -i->GetDelta ())
        {
          break;
        }
      ni->push_back (*i);
    }
  ni->insert (ni->begin (), NiChange (event->GetStartTime (), noiseInterference));
  ni->push_back (NiChange (event->GetEndTime (), 0));
  return noiseInterference;
}

bool
SpcInterferenceHelper::CheckChunkShannonCapacity (double snir, Time duration, SpcPreamble preamble,
                                                  uint32_t totalBytes, uint32_t *currentBytes) const
{
  if (duration == NanoSeconds (0))
    {
      return true;
    }

  uint32_t rate = preamble.GetRate ();
  uint64_t nbytes = (uint64_t)(rate * duration.GetSeconds ());
  uint64_t shannonBits = preamble.GetBandwidth () * log2 (1 + snir);
  uint64_t shannonBytes = shannonBits / 8;
  shannonBytes = shannonBytes * duration.GetSeconds ();

  if (*currentBytes == totalBytes)
    {
      NS_LOG_DEBUG ("[mark] equal");
      return true;
    }
  else if (totalBytes >= (*currentBytes + nbytes))
    {
      NS_LOG_DEBUG ("[mark] totalBytes > current + nbytes");
      *currentBytes += nbytes;
    }
  else
    {
      NS_LOG_DEBUG ("[mark] totalBytes < current + nbytes");
      nbytes = totalBytes - *currentBytes;
      *currentBytes = totalBytes;
    }
  
  NS_LOG_DEBUG ("[Slimit]: " << shannonBytes << ", [Bytes]:" << nbytes << ", [SNIR]:" << snir << " , [D]:" << duration);
    
  if (shannonBytes >= nbytes)
    {
      return true;
    }
  else
    {
      return false;
    }
}

bool
SpcInterferenceHelper::CheckChunkShannonCapacity (double snir, Time duration, SpcPreamble preamble) const
{
  if (duration == NanoSeconds (0))
    {
      return true;
    }

  uint32_t rate = preamble.GetRate ();
  uint64_t nbytes = (uint64_t)(rate * duration.GetSeconds ());
  uint64_t shannonBits = preamble.GetBandwidth () * log2 (1 + snir);
  uint64_t shannonBytes = shannonBits / 8;
  shannonBytes = shannonBytes * duration.GetSeconds ();

  NS_LOG_DEBUG ("[Slimit]: " << shannonBytes << ", [Bytes]:" << nbytes << ", [SNIR]:" << snir << " , [D]:" << duration);
    
  if (shannonBytes >= nbytes)
    {
      return true;
    }
  else
    {
      return false;
    }
}

double
SpcInterferenceHelper::CalculatePer (Ptr<const SpcInterferenceHelper::Event> event, NiChanges *ni) const
{
  SpcPreamble preambleHdr;
  double snr;

  NiChanges::iterator j = ni->begin ();
  Time previous = (*j).GetTime ();

  Time preambleStart = (*j).GetTime ();
  Time payloadStart  = (*j).GetTime () + event->GetPreamble ().GetDuration ();
  double noiseInterferenceW = (*j).GetDelta ();
  double powerW = event->GetRxPowerW ();

  j++;

  while (ni->end () != j)
    {
      Time current = (*j).GetTime ();
      if (payloadStart > previous && payloadStart < current)
        {
          
          // Header
          snr = CalculateSnr (powerW, noiseInterferenceW, preambleHdr);
          if (!CheckChunkShannonCapacity (snr, payloadStart - previous, preambleHdr))
            {
              return 1;
            }
                                        
          // Payload
          snr = CalculateSnr (powerW, noiseInterferenceW, event->GetPreamble ());
          if (!CheckChunkShannonCapacity (snr, current - payloadStart, event->GetPreamble ()))
            {
              return 1;
            }
        }
      else if (payloadStart >= current)
        {
          // Header
          snr = CalculateSnr (powerW, noiseInterferenceW, preambleHdr);
          if (!CheckChunkShannonCapacity (snr , current - previous, preambleHdr))
            {
              return 1;
            }
        }
      else if (payloadStart < current)
        {
          // Payload
          snr = CalculateSnr (powerW, noiseInterferenceW, event->GetPreamble ());
          if (!CheckChunkShannonCapacity (snr, current - previous, event->GetPreamble ()))
            {
              return 1;
            }
        }
      noiseInterferenceW += (*j).GetDelta ();
      previous = (*j).GetTime ();
      j++;
    }

  return 0;
}

double
SpcInterferenceHelper::CalculatePer (Ptr<const SpcInterferenceHelper::Event> event, NiChanges *ni, double power, double noise, uint32_t totalBytes) const
{
  SpcPreamble preambleHdr;
  double snr;

  NiChanges::iterator j = ni->begin ();
  Time previous = (*j).GetTime ();

  Time preambleStart = (*j).GetTime ();
  Time payloadStart  = (*j).GetTime () + event->GetPreamble ().GetDuration ();
  double normalNoiseInterferenceW = (*j).GetDelta ();
  double noiseInterferenceW = (*j).GetDelta () + noise;
  double allPowerW = event->GetRxPowerW ();
  double powerW = event->GetRxPowerW () * power;

  j++;
  uint32_t currentBytes = 0;
  NS_LOG_DEBUG ("total Bytes=" << totalBytes);
  while (ni->end () != j)
    {
      Time current = (*j).GetTime ();
      if (payloadStart > previous && payloadStart < current)
        {
          // Header
          snr = CalculateSnr (allPowerW, normalNoiseInterferenceW, preambleHdr);
          if (!CheckChunkShannonCapacity (snr, payloadStart - previous, preambleHdr))
            {
              return 1;
            }
                                        
          // Payload
          snr = CalculateSnr (powerW, noiseInterferenceW, event->GetPreamble ());
          if (!CheckChunkShannonCapacity (snr, current - payloadStart, event->GetPreamble (), totalBytes ,&currentBytes))
            {
              return 1;
            }
        }
      else if (payloadStart >= current)
        {
          // Header
          snr = CalculateSnr (allPowerW, normalNoiseInterferenceW, preambleHdr);
          if (!CheckChunkShannonCapacity (snr , current - previous, preambleHdr))
            {
              return 1;
            }
        }
      else if (payloadStart < current)
        {
          // Payload
          snr = CalculateSnr (powerW, noiseInterferenceW, event->GetPreamble ());
          if (!CheckChunkShannonCapacity (snr, current - previous, event->GetPreamble (), totalBytes ,&currentBytes))
            {
              return 1;
            }
        }
      noiseInterferenceW += (*j).GetDelta ();
      previous = (*j).GetTime ();
      j++;
    }

  return 0;
}


struct SpcInterferenceHelper::SnrPer
SpcInterferenceHelper::CalculateSnrPer (Ptr<SpcInterferenceHelper::Event> event)
{
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);

  double snr = CalculateSnr (event->GetRxPowerW (),
                             noiseInterferenceW,
                             event->GetPreamble ());

  /* calculate the SNIR at the start of the packet and accumulate
   * all SNIR changes in the snir vector.
   */
  double per = CalculatePer (event, &ni);

  struct SnrPer snrPer;
  snrPer.snr = snr;
  snrPer.per = per;
  return snrPer;
}

struct SpcInterferenceHelper::SnrPer2
SpcInterferenceHelper::CalculateSnrPer2 (Ptr<SpcInterferenceHelper::Event> event)
{
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);

  SpcPreamble preamble = event->GetPreamble ();
  double powRate1 = preamble.GetPower ();
  double powRate2 = 1 - powRate1;
  double noise;
  double snr1, snr2;
  double per1, per2;

  if (preamble.GetIsFar ())
    {
      noise = event->GetRxPowerW () * powRate2;
      snr1 = CalculateSnr (event->GetRxPowerW () * powRate1,
                           noiseInterferenceW + noise,
                           event->GetPreamble ());
      snr2 = CalculateSnr (event->GetRxPowerW () * powRate2,
                           noiseInterferenceW,
                           event->GetPreamble ());

      per1 = CalculatePer (event, &ni, powRate1, noise, event->GetPreamble ().GetNLength ());
      if (per1 == 1)
        {
          per2 = 1;
        }
      else
        {
          per2 = CalculatePer (event, &ni, powRate2, 0, event->GetPreamble ().GetFLength ());
        }
    }
  else
    {
      noise = event->GetRxPowerW () * powRate1;
      snr1 = CalculateSnr (event->GetRxPowerW () * powRate1,
                           noiseInterferenceW,
                           event->GetPreamble ());
      snr2 = CalculateSnr (event->GetRxPowerW () * powRate2,
                           noiseInterferenceW + noise,
                           event->GetPreamble ());
      per2 = CalculatePer (event, &ni, powRate2, noise, event->GetPreamble ().GetFLength ());
      if (per2 == 1)
        {
          per1 = 1;
        }
      else
        {
          per1 = CalculatePer (event, &ni, powRate1, 0, event->GetPreamble ().GetNLength ());
        }
    }



  /* calculate the SNIR at the start of the packet and accumulate
   * all SNIR changes in the snir vector.
   */


  struct SnrPer2 snrPer2;
  snrPer2.snr1 = snr1;
  snrPer2.snr2 = snr2;
  snrPer2.per1 = per1;
  snrPer2.per2 = per2;
  return snrPer2;
}

void
SpcInterferenceHelper::EraseEvents (void)
{
  m_niChanges.clear ();
  m_rxing = false;
  m_firstPower = 0.0;
}
SpcInterferenceHelper::NiChanges::iterator
SpcInterferenceHelper::GetPosition (Time moment)
{
  return std::upper_bound (m_niChanges.begin (), m_niChanges.end (), NiChange (moment, 0));
}
void
SpcInterferenceHelper::AddNiChangeEvent (NiChange change)
{
  m_niChanges.insert (GetPosition (change.GetTime ()), change);
}
void
SpcInterferenceHelper::NotifyRxStart ()
{
  m_rxing = true;
}
void
SpcInterferenceHelper::NotifyRxEnd ()
{
  m_rxing = false;
}
} // namespace ns3

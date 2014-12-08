/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Authors: Joe Kopena <tjkopena@cs.drexel.edu>
 *
 * These applications are used in the WiFi Distance Test experiment,
 * described and implemented in test02.cc.  That file should be in the
 * same place as this file.  The applications have two very simple
 * jobs, they just generate and receive packets.  We could use the
 * standard Application classes included in the NS-3 distribution.
 * These have been written just to change the behavior a little, and
 * provide more examples.
 *
 */

#include <ctime>
#include <sstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/spc-mac-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"

#include "spc-apps.h"

#define STARTAN 1
#define ENDAN   2

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("SpcSimulator");

Time startAnalysisTime = Seconds (0);
bool isAnalysisTime = false;
uint32_t totalRecvSize = 0;

/**********************************************************
                   Trace data
 ***********************************************************/

void
AppSenderRx (std::string path,
	     uint32_t numPkts,
             Ptr<Packet> packet)
{
  if (isAnalysisTime){
    totalRecvSize += packet->GetSize ();
  }
  NS_LOG_INFO (path << "[numPkts]=" << numPkts << " [size]=" << totalRecvSize);
}

void
AppSenderTx (std::string path,
             Ptr<const Packet> packet)
{
  if (Simulator::Now() >= Seconds (STARTAN) && !isAnalysisTime) {
    isAnalysisTime = true;
    startAnalysisTime = Simulator::Now();
  }

  if(Simulator::Now() >= Seconds (STARTAN) + Seconds (ENDAN)) {
    Time time = Simulator::Now() - startAnalysisTime;
    std::cout << (totalRecvSize * 8 / time.GetSeconds ()) / 1000000 << std::endl;
    Simulator::Stop();
  }
}

void
StartTxCallback (std::string path,
                 Ptr<const Packet> packet)
{
  NS_LOG_INFO ("Send=" << packet);
}


//----------------------------------------------
//-- main
//----------------------------------------------
int main (int argc, char *argv[]) {

  PacketMetadata::Enable(); 
  ns3::Packet::EnablePrinting();
  int nodeAmount = 3;
  int distance = 50;
  double interval = 0.001;
  int pkSize = 1500;
  double trafficRatio = 0.3;
  uint64_t stream = 0;

  // Set up command line parameters used to control the experiment.
  CommandLine cmd;
  cmd.AddValue ("distance",     "Distance apart to place nodes (in meters).", distance);
  cmd.AddValue ("interval",     "Delay between transmissions", interval);
  cmd.AddValue ("pkSize",       "The size of packets", pkSize);
  cmd.AddValue ("trafficRatio", "Data traffic ratio (First layer / Second layer)", trafficRatio);
  cmd.AddValue ("stream", "random stream", stream);
  cmd.Parse (argc, argv);

  //------------------------------------------------------------
  //-- Create nodes and network stacks
  //--------------------------------------------
  NS_LOG_INFO ("Creating nodes.");
  NodeContainer nodes;
  nodes.Create (nodeAmount);
  InternetStackHelper internet;
  internet.Install (nodes);

  //------------------------------------------------------------
  //-- Create Net device
  //--------------------------------------------
  NS_LOG_INFO ("Create traffic source & sink.");
  Ptr<SpcNetDevice> netDevices[nodeAmount];
  NetDeviceContainer netDeviceContainer;
  for(int i = 0; i < nodeAmount; i++)
    {
      Ptr<Node> node = NodeList::GetNode (i);
      netDevices [i] = CreateObject<SpcNetDevice>();
      netDeviceContainer.Add(netDevices [i]);
      node->AddDevice (netDevices [i]);
    }
  int64_t currentStream = stream = 0;
  for (int i = 0; i < nodeAmount; i++)
    {
      Ptr<Node>         node     = NodeList::GetNode (i);
      Ptr<SpcNetDevice> device   = netDevices [i];
      Ptr<SpcPhy>       phy      = device->GetPhy ();
      phy->SetMobility (node);
      phy->SetDevice (device);
      netDevices [i]->SetAddress (Mac48Address::Allocate ());
      currentStream += netDevices [i]->GetMac ()->AssignStreams (currentStream);
      currentStream += netDevices [i]->GetMac ()->GetPhy ()->AssignStreams (currentStream);
    }
  for (int i = 0; i < nodeAmount; i++)
    {
      Ptr<SpcChannel> channel= netDevices [i]->GetMac ()->GetPhy ()->GetChannel ();
      for (int j = 0; j < nodeAmount; j++)
        {
          Ptr<SpcPhy> phy = netDevices [j]->GetMac ()->GetPhy ();
          channel->Add (phy);
        }
    }
  Ipv4AddressHelper ipAddrs;
  ipAddrs.SetBase ("192.168.0.0", "255.255.255.0");
  ipAddrs.Assign (netDeviceContainer);

  //------------------------------------------------------------
  //-- Setup Mobility
  //--------------------------------------------
  NS_LOG_INFO ("Installing static mobility; distance " << distance << " .");
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  for(int i = 0; i < nodeAmount; i++){
    positionAlloc->Add (Vector (0.0, distance * i, 0.0)); 
  }

  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (nodes);

  //------------------------------------------------------------
  //-- Create Applications
  //--------------------------------------------
  NS_LOG_INFO ("Create traffic source & sink.");
  for (int i = 0; i < nodeAmount; i++)
    {
      Ptr<SpcSender>   sender    = CreateObject<SpcSender>();
      Ptr<SpcReceiver> receiver = CreateObject<SpcReceiver>();
      sender  ->SetStartTime (Seconds (0));
      receiver->SetStartTime (Seconds (0));

      Ptr<Node> node = NodeList::GetNode (i);
      node->AddApplication (sender);
      node->AddApplication (receiver);
    }

  std::ostringstream oss;
  oss << interval;

  std::string Sender0 ("/NodeList/0/ApplicationList/*/$SpcSender/");
  Config::Set (Sender0 + "PacketSize", UintegerValue (pkSize * 2));
  Config::Set (Sender0 + "TrafficRatio", DoubleValue (trafficRatio));
  Config::Set (Sender0 + "Destination1", Ipv4AddressValue ("192.168.0.2"));
  Config::Set (Sender0 + "Destination2", Ipv4AddressValue ("192.168.0.3"));
  Config::Set (Sender0 + "Interval" ,
               StringValue ("ns3::ConstantRandomVariable[Constant="+oss.str ()+"]"));
  
  Config::Connect ("/NodeList/*/ApplicationList/*/$SpcReceiver/Rx", MakeCallback (&AppSenderRx));
  Config::Connect ("/NodeList/*/ApplicationList/*/$SpcSender/Tx", MakeCallback (&AppSenderTx));
  Config::Connect ("/$ns3::SpcPhy/StartTx", MakeCallback (&StartTxCallback));

  //------------------------------------------------------------
  //-- Run the simulation
  //--------------------------------------------
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  
  // end main
}

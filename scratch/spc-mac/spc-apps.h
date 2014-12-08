#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/application.h"

#include "ns3/stats-module.h"

using namespace ns3;

//----------------------------------------------------------------------
//------------------------------------------------------
class SpcSender : public Application {
 public:
  static TypeId GetTypeId (void);
  SpcSender();
  virtual ~SpcSender();

 protected:
  virtual void DoDispose (void);
  
 private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  
  void SendPacket ();
  
  uint32_t        m_pktSize;
  double          m_trafficRatio;
  Ipv4Address     m_destAddr1;
  Ipv4Address     m_destAddr2;
  uint32_t        m_destPort;
  Ptr<ConstantRandomVariable> m_interval;
  Ptr<UniformRandomVariable> m_random;
  
  Ptr<Socket>     m_socket;
  EventId         m_sendEvent;
  
  TracedCallback<Ptr<const Packet> > m_txTrace;
  
  // end class SpcSender
};




//------------------------------------------------------
class SpcReceiver: public Application {
 public:
  static TypeId GetTypeId (void);
  SpcReceiver();
  virtual ~SpcReceiver();
  
  void SetCounter (Ptr<CounterCalculator<> > calc);
  
 protected:
  virtual void DoDispose (void);
  
 private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);
  
  void Receive (Ptr<Socket> socket);

  uint32_t        m_numPkts;  
  Ptr<Socket>     m_socket;
  uint32_t        m_port;
  
  Ptr<CounterCalculator<> > m_calc;
  TracedCallback<uint32_t, Ptr<Packet> > m_rxTrace;
  // end class SpcReceiver
};

#include <ostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/stats-module.h"

#include "spc-apps.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SpcApps");

TypeId
SpcSender::GetTypeId (void)
{
  static TypeId tid = TypeId ("SpcSender")
    .SetParent<Application> ()
    .AddConstructor<SpcSender> ()
    .AddAttribute ("PacketSize", "The size of packets transmitted.",
                   UintegerValue (1500),
                   MakeUintegerAccessor (&SpcSender::m_pktSize),
                   MakeUintegerChecker<uint32_t>(1))
    .AddAttribute ("TrafficRatio", "The data traffic ratio (first layer / second layer).",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&SpcSender::m_trafficRatio),
                   MakeDoubleChecker<double>(0.0))

    .AddAttribute ("Interval", "Delay between transmissions.",
                   StringValue ("ns3::ConstantRandomVariable[Constant=1]"),
                   MakePointerAccessor (&SpcSender::m_interval),
                   MakePointerChecker <RandomVariableStream>())

    .AddAttribute ("Destination1", "Target host address.",
                   Ipv4AddressValue ("255.255.255.255"),
                   MakeIpv4AddressAccessor (&SpcSender::m_destAddr1),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("Destination2", "Target host address.",
                   Ipv4AddressValue ("255.255.255.255"),
                   MakeIpv4AddressAccessor (&SpcSender::m_destAddr2),
                   MakeIpv4AddressChecker ())

    .AddAttribute ("Port", "Destination app port.",
                   UintegerValue (768),
                   MakeUintegerAccessor (&SpcSender::m_destPort),
                   MakeUintegerChecker<uint32_t>())
    .AddAttribute ("Stream", "Random Stream.",
                   StringValue ("ns3::UniformRandomVariable[Stream=-1]"),
                   MakePointerAccessor (&SpcSender::m_random),
                   MakePointerChecker <RandomVariableStream>())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&SpcSender::m_txTrace))
  ;
  return tid;
}


SpcSender::SpcSender()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_interval = CreateObject<ConstantRandomVariable> ();
  m_random    = CreateObject<UniformRandomVariable> ();
  m_socket = 0;
}

SpcSender::~SpcSender()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
SpcSender::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

void SpcSender::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket == 0) {
      Ptr<SocketFactory> socketFactory = GetNode ()->GetObject<SocketFactory>
          (UdpSocketFactory::GetTypeId ());
      m_socket = Socket::CreateSocket (GetNode (), UdpSocketFactory::GetTypeId ());
      m_socket->Bind ();
    }

  Simulator::Cancel (m_sendEvent);
  m_sendEvent = Simulator::ScheduleNow (&SpcSender::SendPacket, this);
 
  // end SpcSender::StartApplication
}

void SpcSender::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Simulator::Cancel (m_sendEvent);
  // end SpcSender::StopApplication
}

void SpcSender::SendPacket ()
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Packet> packet;

  if (m_random->GetValue() >= 0.5)
    {
      NS_LOG_INFO ("Sending packet at " << Simulator::Now () <<
		   ", to " << m_destAddr1);
      packet = Create<Packet>(m_pktSize * m_trafficRatio);
      m_socket->SendTo (packet, 0, InetSocketAddress (m_destAddr1, m_destPort));
    }
  else
    {
      NS_LOG_INFO ("Sending packet at " << Simulator::Now () <<
		   ", to " << m_destAddr2);
      packet = Create<Packet>(m_pktSize * (1 - m_trafficRatio));
      m_socket->SendTo (packet, 0, InetSocketAddress (m_destAddr2, m_destPort));
    }

  // Report the event to the trace.
  m_txTrace (packet);

  double interval = m_interval->GetValue ();
  double logval = -log(m_random->GetValue());
  Time nextTxTime = Seconds (interval * logval);
  NS_LOG_INFO("nextTime:" << nextTxTime << " in:" << interval << " log:" << logval);

  m_sendEvent = Simulator::Schedule (nextTxTime, &SpcSender::SendPacket, this);
}




//----------------------------------------------------------------------
//-- Receiver
//------------------------------------------------------
TypeId
SpcReceiver::GetTypeId (void)
{
  static TypeId tid = TypeId ("SpcReceiver")
    .SetParent<Application> ()
    .AddConstructor<SpcReceiver> ()
    .AddAttribute ("Port", "Listening port.",
                   UintegerValue (768),
                   MakeUintegerAccessor (&SpcReceiver::m_port),
                   MakeUintegerChecker<uint32_t>())
    .AddAttribute ("NumPackets", "Total number of packets to recv.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SpcReceiver::m_numPkts),
                   MakeUintegerChecker<uint32_t>(0))
    .AddTraceSource ("Rx", "SpcReceiver data packet",
                     MakeTraceSourceAccessor (&SpcReceiver::m_rxTrace))
  ;
  return tid;
}

SpcReceiver::SpcReceiver()
  : m_calc (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
}

SpcReceiver::~SpcReceiver()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
SpcReceiver::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

void
SpcReceiver::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket == 0) {
      Ptr<SocketFactory> socketFactory = GetNode ()->GetObject<SocketFactory>
          (UdpSocketFactory::GetTypeId ());
      m_socket = socketFactory->CreateSocket ();
      InetSocketAddress local = 
        InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (local);
    }
  m_socket->SetRecvCallback (MakeCallback (&SpcReceiver::Receive, this));
  // end SpcReceiver::StartApplication
}

void
SpcReceiver::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket != 0) {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }

  // end SpcReceiver::StopApplication
}

void
SpcReceiver::SetCounter (Ptr<CounterCalculator<> > calc)
{
  m_calc = calc;
  // end SpcReceiver::SetCounter
}

void
SpcReceiver::Receive (Ptr<Socket> socket)
{
  // NS_LOG_FUNCTION (this << socket << packet << from);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from))) {
    if (InetSocketAddress::IsMatchingType (from)) {
      NS_LOG_DEBUG(InetSocketAddress::ConvertFrom (from).GetIpv4 ());
    }

    m_rxTrace (m_numPkts, packet);

    if (m_calc != 0) {
      m_calc->Update ();
    }
    
    // end receiving packets
  }
  
  // end SpcReceiver::SpcReceiver
}

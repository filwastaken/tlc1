/**
 *
 *
 * Network topology
 *
 *                        n4
 *                      / |  \
 *                     /  |   \
 * n0   n1   n2 --- n3 -- n5 -- n7    n8     n9
 * |    |    |         \  |   /  |     |      |
 * ===========          \ |  /   ==============
 *     LAN                n6          LAN
 *
 *
 *
 *  # pcap tracking is active on nodes n0, n3 and n7
 *  # ascii is active only on servers and clients (defined in the configuration section)
 *      the traces follow this format:  task1-<configuration>-<id_del_nodo>.<formato_file_richiesto(.pcap|.tr)>
 *
 * -- configuration 0:
 *    TCP sink on n5, port 2300
 *    TCPOnOffClient on n9 {start_send : 3s, stop_send : 15s, packet_size : 1300bytes}
 *
 *    n9 -> n5 (n9 sends to n5)
 *
 * -- configuration 1:
 *    TCP sink on n5, port 2300
 *    TCP sink on n0, port 7457
 *    TCPOnOffClient on n9 {start_send : 5s, stop_send : 15s, packet_size : 3000bytes}
 *    TCPOnOffClient on n8 {start_send : 2s, stop_send : 9s, packet_size : 2500bytes}
 *    n9 -> n5;   n8 -> n0
 *
 * -- configuration 2:
 *    UDP echo on n8 -> n2
 *    TCP sink n9 -> n5
 *    UDP sink n8 -> n0
 *
 *
 *    Fatto da: Group 25
 *    matricole:
 *      - 1946083
 *      - 1962183
 *      - 1931976
 *      - 1943235
 *
*/


#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>
#include <iostream>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Task_1_Team_25");
int main(int argc, char* argv[]) {
  /**
   *
   * Usage: ./ns3 run <pathtofolder>/task1.cc [-- [--configuration=[0|1|2]] [--verbose]]
   *
   *
  */

  int configuration = 0; //by defualt configuration is set to 0.
  bool verbose = false;  //by default verbose is set to false

  CommandLine cmd(__FILE__);
  cmd.AddValue("configuration", "Start with a particular configuration, 0 is default. (0, 1 or 2) are available",configuration);
  cmd.AddValue("verbose", "Start with verbose information active", verbose);
  cmd.Parse(argc, argv);

  if(configuration != 0 && configuration != 1 and configuration != 2){
    perror("Configuration error, chosen value is not valid");
    exit(1);
  }

  if(verbose){
    //Log for CsmaHelper
    LogComponentEnable ("CsmaHelper", LOG_LEVEL_INFO);
    LogComponentEnable ("CsmaHelper", LOG_LEVEL_ALL);
    //Log for Ipv4AddressHelper
    LogComponentEnable ("Ipv4AddressHelper", LOG_LEVEL_INFO);
    LogComponentEnable ("Ipv4AddressHelper", LOG_LEVEL_ALL);
    //Log for OnOffApplication
    LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("OnOffApplication", LOG_LEVEL_ALL);
    //Log for PointToPointHelper
    LogComponentEnable ("PointToPointHelper", LOG_LEVEL_INFO);
    LogComponentEnable ("PointToPointHelper", LOG_LEVEL_ALL);
    //Log for PointToPointStarHelper
    LogComponentEnable ("PointToPointStarHelper", LOG_LEVEL_INFO);
    LogComponentEnable ("PointToPointStarHelper", LOG_LEVEL_ALL);
    //Log for PacketSink
    LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
    //Log for UdpEchoClientApplication
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
    //Log for UdpEchoServerApplication
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);
  }

  /**
   *
   * Node creations
   *
  */
  NS_LOG_INFO("Create nodes.");

  //Creating nodes
  NodeContainer left_nodes;
  left_nodes.Create(3);

  NodeContainer right_nodes;
  right_nodes.Create(2);

  // Creating center hardware (STAR)
  PointToPointHelper starP2P;
  starP2P.SetDeviceAttribute("DataRate", StringValue("80Mbps"));
  starP2P.SetChannelAttribute("Delay", StringValue("8us"));
  PointToPointStarHelper star(4, starP2P); //4 external nodes


  // Taking custom nodes for p2p connection (Get method return pointer to node)
  Ptr<Node> n0 = left_nodes.Get(0);
  Ptr<Node> n1 = left_nodes.Get(1);
  Ptr<Node> n2 = left_nodes.Get(2);
  Ptr<Node> n8 = right_nodes.Get(0);
  Ptr<Node> n9 = right_nodes.Get(1);
  Ptr<Node> n5 = star.GetHub();
  Ptr<Node> n3 = star.GetSpokeNode(0);
  Ptr<Node> n4 = star.GetSpokeNode(1);
  Ptr<Node> n6 = star.GetSpokeNode(2);
  Ptr<Node> n7 = star.GetSpokeNode(3); //pointer to n7 to be added to right_nodes

  right_nodes.Add(n7); //adding n7 to the right lan connection

  InternetStackHelper internet_helper;
  internet_helper.Install(NodeContainer::GetGlobal()); //installing the stack on all nodes

  /**
   *
   * Channel creation (Hardware side)
   *
  */
  NS_LOG_INFO("Create channels.");
  //Left hardware (CMSA)
  CsmaHelper left_csma;
  left_csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(25000000))); //25Mbps
  left_csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(0.01)));

  //Right hardware (CSMA)
  CsmaHelper right_csma;
  right_csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(30000000))); //30Mbps
  right_csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(0.02)));

  /*
  * Creating this p2p connections:
  *   n2 - n3
  *   n3 - n4
  *   n3 - n6
  *   n4 - n7
  *   n6 - n7
  *
  */
  // Creating multiple containers for the p2p network installation

  NodeContainer n2_n3(n2, n3);
  NodeContainer n3_n4(n3, n4);
  NodeContainer n3_n6(n3, n6);
  NodeContainer n4_n7(n4, n7);
  NodeContainer n6_n7(n6, n7);

  //n2-n3 connection
  PointToPointHelper n2n3_connection;
  n2n3_connection.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  n2n3_connection.SetChannelAttribute("Delay", StringValue("15us"));

  //Given that all other (above) connections have the same data rate and delay, i'll use only one PointToPointHelper, but multiple Ipv4AddressHelper
  PointToPointHelper l_connections;
  l_connections.SetDeviceAttribute("DataRate", StringValue("80Mbps"));
  l_connections.SetChannelAttribute("Delay", StringValue("5us"));

  /**
   *
   * Creating Net device containers for the given nodes and p2p instances
   *
  */

  NetDeviceContainer left_container = left_csma.Install(left_nodes);
  NetDeviceContainer right_container = right_csma.Install(right_nodes);
  NetDeviceContainer n2n3_container = n2n3_connection.Install(n2_n3);
  NetDeviceContainer n3n4_container = l_connections.Install(n3_n4);
  NetDeviceContainer n3n6_container = l_connections.Install(n3_n6);
  NetDeviceContainer n4n7_container = l_connections.Install(n4_n7);
  NetDeviceContainer n6n7_container = l_connections.Install(n6_n7);


  /**
   *
   * Ip-Address(ing)
   *
  */
  NS_LOG_INFO("Assign IP Addresses.");
  Ipv4AddressHelper left_ipv4; //Ipv4 left lan connection
  left_ipv4.SetBase("192.128.1.0", "/24");

  Ipv4AddressHelper right_ipv4; //Ipv4 right lan connection
  right_ipv4.SetBase("192.128.2.0", "/24");

  //Ipv4 ptp between n2 and n3
  Ipv4AddressHelper n2n3_address;
  n2n3_address.SetBase("10.1.1.0", "/30");

  //Ipv4 ptp connection between outer star nodes
  Ipv4AddressHelper n3n4_address;
  n3n4_address.SetBase("10.0.1.0", "/30");

  Ipv4AddressHelper n3n6_address;
  n3n6_address.SetBase("10.0.4.0", "/30");

  Ipv4AddressHelper n4n7_address;
  n4n7_address.SetBase("10.0.2.0", "/30");

  Ipv4AddressHelper n6n7_address;
  n6n7_address.SetBase("10.0.3.0", "/30");

  Ipv4AddressHelper star_address;
  star_address.SetBase("10.10.1.0", "/24");

  /**
   *
   * Creating Ipv4 Interface Containers for every connection (other than the star, since it doesn't need it)
   *
  */
  Ipv4InterfaceContainer right_interface = right_ipv4.Assign(right_container);
  Ipv4InterfaceContainer left_interface = left_ipv4.Assign(left_container);
  Ipv4InterfaceContainer n2n3_interface = n2n3_address.Assign(n2n3_container);
  Ipv4InterfaceContainer n3n4_interface = n3n4_address.Assign(n3n4_container);
  Ipv4InterfaceContainer n3n6_interface = n3n6_address.Assign(n3n6_container);
  Ipv4InterfaceContainer n4n7_interface = n4n7_address.Assign(n4n7_container);
  Ipv4InterfaceContainer n6n7_interface = n6n7_address.Assign(n6n7_container);
  star.AssignIpv4Addresses(star_address); //Ipv4 star connection

  //With this the all the nodes will ne able to reach the other reachables
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  /**
  *
  *
  * Containers order (to recap)
  * left: n0, n1, n2
  * star: n3, n4, n6, n7 (outer) - n5 (hub)
  * right: n8, n9, n7 (7 is the last one since it got added last. It is shared between star and right).
  *
  *
  *
  *	ips: left 	192.128.1.x
  *			 right	192.128.2.x
  *			 star	10.10.x.y
  *			 n2n3	10.1.1.x
  *			 n3n4	10.0.1.x
  *			 n4n7	10.0.2.x
  *			 n7n6	10.0.3.x
  *			 n3n6	10.0.4.x
  *
  */

  // Ascii tracing must be done on client and servers only! It must be defined in the configuration part
  if(configuration == 0){
    //TCP - sink on n5, 2300 port
    //TCP OnOff client on n9 {start_send : 3s, stop_send : 15s, packet_size : 1300bytes}

    //Sink on n5 -> center of the star!
    uint16_t port = 2300;
    Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);

    ApplicationContainer sinkApp = sinkHelper.Install(n5);
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(20.0)); // I chose 20 since it's the simulation maximum time

    // Create the OnOff applications to send TCP to the sender on n9
    OnOffHelper clientHelper("ns3::TcpSocketFactory", Address());
    clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    clientHelper.SetAttribute("PacketSize", UintegerValue(1300));

    ApplicationContainer sender;
    AddressValue remoteAddress(InetSocketAddress(star.GetHubIpv4Address(3), port));
    clientHelper.SetAttribute("Remote", remoteAddress);
    sender.Add(clientHelper.Install(n9));

    sender.Start(Seconds(3.0));
    sender.Stop(Seconds(15.0));

    // configure tracing
    AsciiTraceHelper ascii;
    NodeContainer n5_container(n5); starP2P.EnableAscii(ascii.CreateFileStream("task1-0-n5.tr"), n5_container);
    right_csma.EnableAscii(ascii.CreateFileStream("task1-0-n9.tr"), right_container.Get(1));
    right_csma.EnableAscii(ascii.CreateFileStream("task1-0-n7.tr"), right_container.Get(2));

    // Enabling packet tracing for n0, n3, n7
    left_csma.EnablePcap("task1-0-0.pcap", left_container.Get(0), true, true); //n0 is the first one on left_csma
    n2n3_connection.EnablePcap("task1-0-3.pcap", n2n3_container.Get(1), true, true); //n3 is the lastone on the n2n3 connection
    right_csma.EnablePcap("task1-0-7.pcap", right_container.Get(2), true, true); //I'm getting the last one since 7 is the one i added last
  } else if(configuration == 1){
    /* TCP - sink on n5, 2300 port
    * TCP - sink on n0, 7457 port
    * TCP OnOff client on n9 {start_send : 5s, stop_send : 15s, packet_size : 2500bytes}
    * TCP OnOff client on n8 {start_send : 2s, stop_send : 9s, packet_size : 5000bytes}
    * n9 -> n5,    n8 -> n0
    */

    uint16_t n5_portnumber = 2300, n0_portnumber = 7457;
    Address n5_sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(),n5_portnumber));
    Address n0_sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(),n0_portnumber));
    PacketSinkHelper n5_sink("ns3::TcpSocketFactory", n5_sinkLocalAddress);
    PacketSinkHelper n0_sink("ns3::TcpSocketFactory", n0_sinkLocalAddress);
    ApplicationContainer n5_sinkapp = n5_sink.Install(n5);
    ApplicationContainer n0_sinkapp = n0_sink.Install(n0);
    
    //Sink Application Code
    n5_sinkapp.Start(Seconds(0.0));
    n0_sinkapp.Start(Seconds(0.0));
    n5_sinkapp.Stop(Seconds(20.0));
    n0_sinkapp.Stop(Seconds(20.0));
    
    //TCPOnOff Code
    OnOffHelper n9_tcphelper("ns3::TcpSocketFactory", Address());
    OnOffHelper n8_tcphelper("ns3::TcpSocketFactory", Address());

    n9_tcphelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    n9_tcphelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    n9_tcphelper.SetAttribute("PacketSize", UintegerValue(2500));
    
    n8_tcphelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    n8_tcphelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    n8_tcphelper.SetAttribute("PacketSize", UintegerValue(5000));
 
    
    AddressValue n9_remoteAddress(InetSocketAddress(star.GetHubIpv4Address(3), n5_portnumber)); //manda i dati all'indirizzo ip della stella di n5 da n7
    //indica gli indirizzi a cui mandare i dati
    AddressValue n8_remoteAddress(InetSocketAddress(left_interface.GetAddress(0), n0_portnumber)); //manda i dati all'indirizzo ip di n0
    n9_tcphelper.SetAttribute("Remote", n9_remoteAddress);
    n8_tcphelper.SetAttribute("Remote", n8_remoteAddress);
    //OnOff Application Code
    ApplicationContainer n9_onoff = n9_tcphelper.Install(n9);
    ApplicationContainer n8_onoff = n8_tcphelper.Install(n8);

    n9_onoff.Start(Seconds(5.0));
    n8_onoff.Start(Seconds(2.0));
    n9_onoff.Stop(Seconds(15.0));
    n8_onoff.Stop(Seconds(9.0));
    
    AsciiTraceHelper ascii;
    NodeContainer n5_container(n5); starP2P.EnableAscii(ascii.CreateFileStream("task1-2-n5.tr"), n5_container);
    left_csma.EnableAscii(ascii.CreateFileStream("task1-1-n0.tr"), left_container.Get(0));    //Tracing n0
    right_csma.EnableAscii(ascii.CreateFileStream("task1-1-n9.tr"), right_container.Get(1));  //  "     n9
    right_csma.EnableAscii(ascii.CreateFileStream("task1-1-n8.tr"), right_container.Get(0));  //  "     n8
     
    // Enabling packet tracing for n0, n3, n7
    left_csma.EnablePcap("task1-1-0.pcap", left_container.Get(0), true, true);
    n2n3_connection.EnablePcap("task1-1-3.pcap", n2n3_container.Get(1), true, true);
    right_csma.EnablePcap("task1-1-7.pcap", right_container.Get(2), true, true);

  } else if(configuration == 2){
    /*
    *  UDPecho n8 -> n2
    *  TCPsink n9 -> n5
    *  UDPsink n8 -> n0
    */
    uint16_t n2_port = 63;
    UdpEchoServerHelper n2_server(n2_port);
    ApplicationContainer n2_serverapp = n2_server.Install(n2);
    
    n2_serverapp.Start(Seconds(0.0));
    n2_serverapp.Stop(Seconds(20.0));
    
    UdpEchoClientHelper n8_echo(left_interface.GetAddress(2), n2_port);
    n8_echo.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    n8_echo.SetAttribute("MaxPackets", UintegerValue(5)); //it will sends 5 packets every two seconds, starting at 3s
    n8_echo.SetAttribute("PacketSize", UintegerValue(2560));
    ApplicationContainer n8_client_app = n8_echo.Install(n8);

    // Inserting the hex number based on our matriculation number
    int sum = 1931976 + 1946083 + 1962183 + 1943235;
    std::ostringstream ss;
    ss << std::hex << sum;
    std::string result = ss.str();

    char* testo = (char*) malloc(2560);
    for(int i = 0; i < (2560 - (int) result.length()); i++) testo[i] = 0;
    for(int i = (2560 - (int) result.length()); i<2560; i++) testo[i] = result[i];

    n8_echo.SetFill(n8_client_app.Get(0), testo);
    n8_client_app.Start(Seconds(3.0));
    n8_client_app.Stop(Seconds(10.0)); // it won't send anything between 9 and 10
    
    //Sink on/off
    uint16_t n5_portnumber = 2300, n0_portnumber = 7454;
    Address n5_sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(),n5_portnumber));
    Address n0_sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(),n0_portnumber));
    PacketSinkHelper n5_sink("ns3::TcpSocketFactory", n5_sinkLocalAddress);
    PacketSinkHelper n0_sink("ns3::UdpSocketFactory", n0_sinkLocalAddress);
    ApplicationContainer n5_sinkapp = n5_sink.Install(n5);
    ApplicationContainer n0_sinkapp = n0_sink.Install(n0);

    n5_sinkapp.Start(Seconds(0.0));
    n0_sinkapp.Start(Seconds(0.0));
    n5_sinkapp.Stop(Seconds(20.0));
    n0_sinkapp.Stop(Seconds(20.0));
    
    //TCP and On/Off
    OnOffHelper n9_tcphelper("ns3::TcpSocketFactory", Address());
    OnOffHelper n8_tcphelper("ns3::UdpSocketFactory", Address());

    n9_tcphelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    n9_tcphelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    n9_tcphelper.SetAttribute("PacketSize", UintegerValue(3000));
    n8_tcphelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    n8_tcphelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    n8_tcphelper.SetAttribute("PacketSize", UintegerValue(3000));
    
    AddressValue n9_remoteAddress(InetSocketAddress(star.GetHubIpv4Address(3), n5_portnumber)); //sending to n5 from the pov of n7
    AddressValue n8_remoteAddress(InetSocketAddress(left_interface.GetAddress(0), n0_portnumber)); //sending to n0
    n9_tcphelper.SetAttribute("Remote", n9_remoteAddress);
    n8_tcphelper.SetAttribute("Remote", n8_remoteAddress);
    
    ApplicationContainer n9_onoff = n9_tcphelper.Install(n9);
    ApplicationContainer n8_onoff = n8_tcphelper.Install(n8);

    n9_onoff.Start(Seconds(3.0));
    n9_onoff.Stop(Seconds(9.0));
    n8_onoff.Start(Seconds(5.0));
    n8_onoff.Stop(Seconds(15.0));

    AsciiTraceHelper ascii; // Ascii tracing
    NodeContainer n5_container(n5); starP2P.EnableAscii(ascii.CreateFileStream("task1-2-n5.tr"), n5_container);
    left_csma.EnableAscii(ascii.CreateFileStream("task1-2-n0.tr"),left_container.Get(0)); 
    right_csma.EnableAscii(ascii.CreateFileStream("task1-2-n9.tr"), right_container.Get(1));
    right_csma.EnableAscii(ascii.CreateFileStream("task1-2-n8.tr"), right_container.Get(0));
    left_csma.EnableAscii(ascii.CreateFileStream("task1-2-n2.tr"), left_container.Get(2));
    
    // Enabling packet tracing for n0, n3, n7
    left_csma.EnablePcap("task1-2-0.pcap", left_container.Get(0), true, true);
    n2n3_connection.EnablePcap("task1-2-3.pcap", n2n3_container.Get(1), true, true);
    right_csma.EnablePcap("task1-2-7.pcap", right_container.Get(2), true, true);
  }

  /**
   *
   * Running the simulation
   *
  */
  NS_LOG_INFO("Run Simulation.");
  Simulator::Stop(Seconds(20.0));
  Simulator::Run();
  Simulator::Destroy();
  NS_LOG_INFO("Done.");
  return 0;
}
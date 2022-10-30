/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 *     LAN                n6            LAN
 *
 *
 *
 *  # pcap tracking is active on nodes n0, n3 and n7
 *  # ascii is active only on servers and clients (defined in the configuration section)
 *      the traces follow this format:  task1-<configuration>-<id_del_nodo>.<formato_file_richiesto(.pcap|.tr)>
 *
 * -- configuration 0:
 *
 *
 *
 * -- configuration 1:
 *
 *
 *
 * -- configuration 2:
 *
 *
 *
 *
*/

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Task_1_Team_25");
int main(int argc, char* argv[]) {
  /**
   *
   * Usage: ./ns3 run <pathtofolder>/task1.cc [--configuration=[0|1|2]] [--verbose]
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
    LogComponentEnable ("Homework1 (INFO) - ", LOG_LEVEL_INFO);
    LogComponentEnable ("Homework1 (CLIENT) - ", LOG_LEVEL_ALL);
    LogComponentEnable ("Homework1 (SERVER) - ", LOG_LEVEL_ALL);
  }

  /**
   *
   * Node creations
   *
  */
  NS_LOG_INFO("Create nodes.");
  //Creating nodes

  Ptr<Node> n0 = CreateObject<Node>();
  Ptr<Node> n1 = CreateObject<Node>();
  Ptr<Node> n2 = CreateObject<Node>();

  NodeContainer nodes_left(n0, n1, n2);

  Ptr<Node> n8 = CreateObject<Node>();
  Ptr<Node> n9 = CreateObject<Node>();

  NodeContainer nodes_right(n8, n9); //node 7 will be added later

  // Creating center hardware (STAR)
  PointToPointHelper starP2P;
  starP2P.SetDeviceAttribute("DataRate", StringValue("80Mbps"));
  starP2P.SetChannelAttribute("Delay", StringValue("8us"));
  PointToPointStarHelper star(4, starP2P); //4 external nodes

  // Taking custom nodes for p2p connection (Get method return pointer to node)
  Ptr<Node> n5 = star.GetHub();
  Ptr<Node> n3 = star.GetSpokeNode(0);
  Ptr<Node> n4 = star.GetSpokeNode(1);
  Ptr<Node> n6 = star.GetSpokeNode(2);
  Ptr<Node> n7 = star.GetSpokeNode(3); //pointer to n7 to be added to nodes_right

  InternetStackHelper internet_helper;
  internet_helper.Install(nodes_left);
  internet_helper.Install(nodes_right);   // Reversing Internet installation and n7 sharing to prevent internet installation twice on n7
  star.InstallStack(internet_helper);

  // n7 sharing
  NodeContainer shared(n7);
  nodes_right.Add(shared); //added n7 to nodes_right

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
  NetDeviceContainer left_container = left_csma.Install(nodes_left);
  NetDeviceContainer right_container = right_csma.Install(nodes_right);
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
  left_ipv4.SetBase("192.168.1.0", "/24");

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
  star.AssignIpv4Addresses(Ipv4AddressHelper("10.10.1.0", "/24")); //Ipv4 star connection



  // Enabling packet tracing for n0, n3, n7
  left_csma.EnablePcap("n0_pcap_", left_container.Get(0)); //n0 is the first one on left_csma
  n2n3_connection.EnablePcap("n3_pcap_", n2n3_container.Get(1)); //n3 is the lastone on the n2n3 connection
  right_csma.EnablePcap("n7_pcap_", right_container.Get(2)); //I'm getting the last one since 7 is the one i added last

  // Ascii tracing must be done on client and servers only! It must be defined in the configuration part
  if(configuration == 0){
    //TODO HERE

  } else if(configuration == 1){
    //TODO HERE

  } else if(configuration == 2){
    //TODO HERE

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

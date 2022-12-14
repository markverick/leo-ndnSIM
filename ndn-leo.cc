// ndn-leo.cc

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/names.h"
#include "read-data.h"

// for LinkStatusControl::FailLinks and LinkStatusControl::UpLinks
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"
// for FibHelper::AddRoute and FibHelper::RemoveRoute
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

namespace ns3 {

int
main(int argc, char* argv[])
{
  // Loading data
  // Importing TLEs
  vector<leo::Tle> tles = leo::readTles("data/tles.txt");
  
  // Importing ground stations
  vector<leo::GroundStation> ground_stations = 
    leo::readGroundStations("data/ground_stations.txt", tles.size());

  // Importing topology
  // vector<leo::Topo> topos = leo::readIsls("data/isls.txt");
  unordered_set<pair<int, int>, leo::pairhash > topos = leo::populateTopology("data/dynamic_state_100ms_for_500s");

  // Setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", StringValue("20p"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(tles.size() + ground_stations.size());

  // Connecting nodes using imported topology
  const int node1 = 35 + tles.size(); // 35,Krung-Thep-(Bangkok)
  const int node2 = 20 + tles.size(); // 20, Los-Angeles-Long-Beach-Santa-Ana
  std::string prefix1 = "/uid-" + to_string(node2);
  std::string prefix2 = "/uid-" + to_string(node1);
  // cout << prefix1 << prefix2 << endl;

  PointToPointHelper p2p;
  cout << "Loading topology..."  << endl;
  for (pair<int, int> p : topos) {
    p2p.Install(nodes.Get(p.first), nodes.Get(p.second));
  }

  // // TODO: Will be significantly faster to use real connections
  // cout << "Installing ground station <--> satellite links..." << endl;
  // // Attaching all satellite to ground stations
  // for (leo::GroundStation gs : ground_stations) {
  //   for (leo::Tle sat : tles) {
  //     p2p.Install(nodes.Get(gs.m_uid), nodes.Get(sat.m_uid));
  //   }
  // }
  
  // cout << "Installing links between satellites..." << endl;
  // // Attaching links between satellites
  // for (leo::Topo topo : topos) {
  //   p2p.Install(nodes.Get(topo.m_uid_1), nodes.Get(topo.m_uid_2));
  // }
  ndn::StackHelper ndnHelper;
  ndnHelper.Install(nodes);

  // Setting up FIB schedules

  cout << "Setting up FIB schedules..."  << endl;
  // Install NDN stack on all nodes
  leo::importDynamicState(nodes, "data/dynamic_state_100ms_for_500s");

  // For debug
  cout << "press any keys to continue";
  cin.get(); 

  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix(prefix1);
  consumerHelper.SetAttribute("Frequency", StringValue("2")); // 10 interests a second
  consumerHelper.Install(nodes.Get(node1)); // first node

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix(prefix1);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(nodes.Get(node2)); // last node



  
  Simulator::Stop(Seconds(5.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
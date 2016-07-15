#include <fstream>
#include <iostream>
#include <Beam/Network/TcpServerSocket.hpp>
#include <Beam/Threading/TimerThreadPool.hpp>
#include <Beam/Utilities/ApplicationInterrupt.hpp>
#include <Beam/Utilities/YamlConfig.hpp>
#include <Beam/WebServices/HttpServletContainer.hpp>
#include <tclap/CmdLine.h>
#include "HttpFileServer/HttpFileServer/HttpFileServlet.hpp"
#include "HttpFileServer/Version.hpp"

using namespace Beam;
using namespace Beam::HttpFileServer;
using namespace Beam::IO;
using namespace Beam::Network;
using namespace Beam::Threading;
using namespace Beam::WebServices;
using namespace boost;
using namespace boost::posix_time;
using namespace std;
using namespace TCLAP;

namespace {
  using HttpFileServletContainer = HttpServletContainer<HttpFileServlet,
    TcpServerSocket>;

  struct ServerConnectionInitializer {
    IpAddress m_interface;
    vector<IpAddress> m_addresses;

    void Initialize(const YAML::Node& config);
  };

  void ServerConnectionInitializer::Initialize(const YAML::Node& config) {
    m_interface = Extract<IpAddress>(config, "interface");
    vector<IpAddress> addresses;
    addresses.push_back(m_interface);
    m_addresses = Extract<vector<IpAddress>>(config, "addresses", addresses);
  }
}

int main(int argc, const char** argv) {
  string configFile;
  try {
    CmdLine cmd{"", ' ', "0.9-r" HTTP_FILE_SERVER_VERSION
      "\nCopyright (C) 2009 Eidolon Systems Ltd."};
    ValueArg<string> configArg{"c", "config", "Configuration file", false,
      "config.yml", "path"};
    cmd.add(configArg);
    cmd.parse(argc, argv);
    configFile = configArg.getValue();
  } catch(const ArgException& e) {
    cerr << "error: " << e.error() << " for arg " << e.argId() << endl;
    return -1;
  }
  YAML::Node config;
  try {
    ifstream configStream{configFile.c_str()};
    if(!configStream.good()) {
      cerr << configFile << " not found." << endl;
      return -1;
    }
    YAML::Parser configParser{configStream};
    configParser.GetNextDocument(config);
  } catch(const YAML::ParserException& e) {
    cerr << "Invalid YAML at line " << (e.mark.line + 1) << ", " << "column " <<
      (e.mark.column + 1) << ": " << e.msg << endl;
    return -1;
  }
  SocketThreadPool socketThreadPool;
  TimerThreadPool timerThreadPool;
  ServerConnectionInitializer serverConnectionInitializer;
  try {
    serverConnectionInitializer.Initialize(GetNode(config, "server"));
  } catch(const std::exception& e) {
    cerr << "Error parsing section 'server': " << e.what() << endl;
    return -1;
  }
  HttpFileServletContainer server{Initialize(),
    Initialize(serverConnectionInitializer.m_interface, Ref(socketThreadPool))};
  try {
    server.Open();
  } catch(const std::exception& e) {
    cerr << "Error opening server: " << e.what() << endl;
    return -1;
  }
  WaitForKillEvent();
  return 0;
}

#include "sevent/net/InetAddress.h"
#include "sevent/base/Logger.h"
#include "myassert.h"
#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;
using namespace sevent;
using namespace sevent::net;

void testInetAddress() {
  InetAddress addr0(1234);
  myassert(addr0.toStringIp() == string("0.0.0.0"));
  myassert(addr0.toStringIpPort() == string("0.0.0.0:1234"));
  myassert(addr0.getPortHost() == 1234);

  InetAddress addr1(4321, false, true);
  myassert(addr1.toStringIp() == string("127.0.0.1"));
  myassert(addr1.toStringIpPort() == string("127.0.0.1:4321"));
  myassert(addr1.getPortHost() == 4321);

  InetAddress addr2("1.2.3.4", 8888);
  myassert(addr2.toStringIp() == string("1.2.3.4"));
  myassert(addr2.toStringIpPort() == string("1.2.3.4:8888"));
  myassert(addr2.getPortHost() == 8888);

  InetAddress addr3("255.254.253.252", 65535);
  myassert(addr3.toStringIp() == string("255.254.253.252"));
  myassert(addr3.toStringIpPort() == string("255.254.253.252:65535"));
  myassert(addr3.getPortHost() == 65535);
}

void testInetAddressResolve() {
  InetAddress addr(80);
  if (InetAddress::resolve("baidu.com", &addr)) {
    LOG_INFO << "baidu.com resolved to " << addr.toStringIpPort();
  }
  else {
    LOG_ERROR << "Unable to resolve baidu.com";
  }
}
int main() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    testInetAddress();
    testInetAddressResolve();
    cout << "finish InetAddress_test" << endl;

#ifdef _WIN32
WSACleanup();
#endif
    return 0;
}
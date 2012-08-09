#include "netroute.h"
#include "node.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/route.h>

namespace netroute {

using namespace node;
using namespace v8;

Persistent<String> dest_sym;
Persistent<String> gateway_sym;
Persistent<String> netmask_sym;
Persistent<String> genmask_sym;
Persistent<String> ifp_sym;
Persistent<String> ifa_sym;
Persistent<String> interface_sym;
Persistent<String> mtu_sym;
Persistent<String> rtt_sym;
Persistent<String> expire_sym;

static Persistent<String> ip4_sym;
static Persistent<String> ip6_sym;


static Handle<Value> GetInfo(const Arguments& args) {
  HandleScope scope;
  Local<Object> result = Object::New();

  result->Set(ip4_sym, GetInfo(AF_INET));
  result->Set(ip6_sym, GetInfo(AF_INET6));

  return scope.Close(result);
}


static void Init(Handle<Object> target) {
  HandleScope scope;

  ip4_sym = Persistent<String>::New(String::NewSymbol("IPv4"));
  ip6_sym = Persistent<String>::New(String::NewSymbol("IPv6"));
  dest_sym = Persistent<String>::New(String::NewSymbol("destination"));
  gateway_sym = Persistent<String>::New(String::NewSymbol("gateway"));
  netmask_sym = Persistent<String>::New(String::NewSymbol("netmask"));
  genmask_sym = Persistent<String>::New(String::NewSymbol("genmask"));
  interface_sym = Persistent<String>::New(String::NewSymbol("interface"));
  mtu_sym = Persistent<String>::New(String::NewSymbol("mtu"));
  rtt_sym = Persistent<String>::New(String::NewSymbol("rtt"));
  expire_sym = Persistent<String>::New(String::NewSymbol("expire"));

  NODE_SET_METHOD(target, "getInfo", GetInfo);
}

NODE_MODULE(netroute, Init);

} // namespace netroute

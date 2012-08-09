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

static Persistent<String> dest_sym;
static Persistent<String> gateway_sym;
static Persistent<String> netmask_sym;
static Persistent<String> genmask_sym;
static Persistent<String> ifp_sym;
static Persistent<String> ifa_sym;
static Persistent<String> interface_sym;
static Persistent<String> mtu_sym;
static Persistent<String> rtt_sym;
static Persistent<String> expire_sym;
static Persistent<String> pksent_sym;


static Handle<Value> GetInfo(const Arguments& args) {
  HandleScope scope;
  Local<Array> result = Array::New();

  int mib[6] = { CTL_NET, PF_ROUTE, 0, 0, NET_RT_DUMP, 0 };
  size_t size;
  char* addresses;

  // Get buffer size
  if (sysctl(mib, 6, NULL, &size, NULL, 0) == -1) {
    return scope.Close(ThrowException(String::New("sysctl failed (get size)")));
  }

  // Get real addresses
  addresses = reinterpret_cast<char*>(malloc(size));
  if (sysctl(mib, 6, addresses, &size, NULL, 0) == -1) {
    free(addresses);
    return scope.Close(ThrowException(String::New("sysctl failed (read)")));
  }

  // Iterate through received info
  char* current = addresses;
  sockaddr_in* addrs[1024];
  char out[256];

  int flags[6] = { RTA_DST, RTA_GATEWAY, RTA_NETMASK,
                   RTA_GENMASK, RTA_IFP, RTA_IFA };
  int indexes[6] = { RTAX_DST, RTAX_GATEWAY, RTAX_NETMASK,
                     RTAX_GENMASK, RTAX_IFP, RTAX_IFA };
  Persistent<String> keys[6] = { dest_sym, gateway_sym, netmask_sym,
                                 genmask_sym, ifp_sym, ifa_sym };

  int i = 0;
  while (current < addresses + size) {
    rt_msghdr* msg = reinterpret_cast<rt_msghdr*>(current);

    Local<Object> info = Object::New();

    // Copy pointers to socket addresses
    // (each address may be either ip4 or ip6, we should dynamically decide
    //  how far next address is)
    addrs[0] = reinterpret_cast<sockaddr_in*>(msg + 1);
    for (int j = 1; ; j++) {
      size_t prev_size;

      if (addrs[j - 1]->sin_family == AF_INET6) {
        prev_size = sizeof(sockaddr_in6);
      } else {
        prev_size = sizeof(sockaddr_in);
      }

      addrs[j] = reinterpret_cast<sockaddr_in*>(
          reinterpret_cast<char*>(addrs[j - 1]) + prev_size);
      if (reinterpret_cast<char*>(addrs[j]) >= current + msg->rtm_msglen) break;
    }

    // Put every socket address into object
    for (int j = 0; j < 6; j++) {
      if ((msg->rtm_addrs & flags[j]) == 0) continue;

      sockaddr_in* addr = addrs[indexes[j]];
      if (addr->sin_family == AF_INET6) {
        uv_ip6_name(reinterpret_cast<sockaddr_in6*>(addr), out, sizeof(out));
      } else {
        uv_ip4_name(addr, out, sizeof(out));
      }
      info->Set(keys[j], String::New(out));
    }

    // Put metrics
    info->Set(mtu_sym, Number::New(msg->rtm_rmx.rmx_mtu));
    info->Set(rtt_sym, Number::New(msg->rtm_rmx.rmx_rtt));
    info->Set(expire_sym, Number::New(msg->rtm_rmx.rmx_expire));
    info->Set(pksent_sym, Number::New(msg->rtm_rmx.rmx_pksent));

    // Put interface name
    char iface[IFNAMSIZ];
    if_indextoname(msg->rtm_index, iface);
    info->Set(interface_sym, String::New(iface));

    // And put object into resulting array
    result->Set(i, info);
    current += msg->rtm_msglen;
    i++;
  }

  // Finally, free allocated memory
  free(addresses);

  return scope.Close(result);
}


static void Init(Handle<Object> target) {
  HandleScope scope;

  dest_sym = Persistent<String>::New(String::NewSymbol("destination"));
  gateway_sym = Persistent<String>::New(String::NewSymbol("gateway"));
  netmask_sym = Persistent<String>::New(String::NewSymbol("netmask"));
  genmask_sym = Persistent<String>::New(String::NewSymbol("genmask"));
  ifp_sym = Persistent<String>::New(String::NewSymbol("interfaceName"));
  ifa_sym = Persistent<String>::New(String::NewSymbol("interfaceAddr"));
  interface_sym = Persistent<String>::New(String::NewSymbol("interface"));
  mtu_sym = Persistent<String>::New(String::NewSymbol("mtu"));
  rtt_sym = Persistent<String>::New(String::NewSymbol("rtt"));
  expire_sym = Persistent<String>::New(String::NewSymbol("expire"));
  pksent_sym = Persistent<String>::New(String::NewSymbol("pksent"));

  NODE_SET_METHOD(target, "getInfo", GetInfo);
}

NODE_MODULE(netroute, Init);

} // namespace netroute

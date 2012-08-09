#include "node.h"
#include "netroute.h"

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/route.h>

namespace netroute {

using namespace node;
using namespace v8;

Handle<Value> GetInfo(int family) {
  HandleScope scope;
  Local<Array> result = Array::New();

  int mib[6] = { CTL_NET, PF_ROUTE, 0, family, NET_RT_DUMP, 0 };
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

  int flags[4] = { RTA_DST, RTA_GATEWAY, RTA_NETMASK,
                   RTA_GENMASK };
  int indexes[4] = { RTAX_DST, RTAX_GATEWAY, RTAX_NETMASK,
                     RTAX_GENMASK };
  Persistent<String> keys[4] = { dest_sym, gateway_sym, netmask_sym,
                                 genmask_sym };

  int i = 0;
  while (current < addresses + size) {
    rt_msghdr* msg = reinterpret_cast<rt_msghdr*>(current);

    // Skip cloned routes
    if (msg->rtm_flags & RTF_WASCLONED) {
      current += msg->rtm_msglen;
      continue;
    }

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
    for (int j = 0; j < 4; j++) {
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

} // namespace netroute

#include "node.h"
#include "node_object_wrap.h"
#include "netroute.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/sockios.h>

namespace netroute {

using namespace node;
using namespace v8;

#define ASSERT(e)                                                             \
  do {                                                                        \
    if ((e)) break;                                                           \
    fprintf(stderr,                                                           \
            "Assertion `" #e "' failed at %s:%d\n", __FILE__, __LINE__);      \
    abort();                                                                  \
  }                                                                           \
  while (0)

#define THROW(s)                                                              \
  do {                                                                        \
    v8::Local<v8::String> errmsg = v8::String::New((s));                      \
    v8::Local<v8::Value> ex = v8::Exception::Error(errmsg);                   \
    return v8::ThrowException(ex);                                            \
  }                                                                           \
  while (0)


unsigned int hex2bin(unsigned char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  ASSERT(0);
}


void Hex2Bin(char* buf, unsigned int len) {
  unsigned char* p = reinterpret_cast<unsigned char*>(buf);

  for (unsigned int i = 0; i < len; i += 2) {
    unsigned int a = p[i + 0];
    unsigned int b = p[i + 1];
    p[i / 2] = 16 * hex2bin(a) + hex2bin(b);
  }
}


Handle<Value> GetRoutesIPv4() {
  HandleScope scope;
  Local<Array> routes = Array::New();

  FILE* fp = fopen("/proc/net/route", "r");
  if (fp == NULL) return scope.Close(routes);

  char buf[1024];
  char* s = fgets(buf, sizeof(buf), fp); // skip the first line
  ASSERT(s == buf);

  while (!feof(fp)) {
    char iface[256];
    unsigned int dst;
    unsigned int gateway;
    unsigned int flags;
    int refcnt;
    unsigned int use;
    int metric;
    unsigned int mask;
    int mtu;
    unsigned int window;
    unsigned int rtt;

    int nitems = fscanf(fp,
                        "%s %08x %08x %04x %d %u %d %08x %d %u %u\n",
                        iface,
                        &dst,
                        &gateway,
                        &flags,
                        &refcnt,
                        &use,
                        &metric,
                        &mask,
                        &mtu,
                        &window,
                        &rtt);
    ASSERT(nitems == 11);

    char buf[256];
    Local<Object> route = Object::New();
    route->Set(interface_sym,
               String::New(reinterpret_cast<const char*>(iface)));
    route->Set(dest_sym,
               String::New(inet_ntop(AF_INET, &dst, buf, sizeof(buf))));
    route->Set(gateway_sym,
               String::New(inet_ntop(AF_INET, &gateway, buf, sizeof(buf))));
    route->Set(String::New("flags"), Integer::NewFromUnsigned(flags));
    route->Set(String::New("refcnt"), Integer::New(refcnt));
    route->Set(String::New("use"), Integer::NewFromUnsigned(use));
    route->Set(String::New("metric"), Integer::New(metric));
    route->Set(netmask_sym,
               String::New(inet_ntop(AF_INET, &mask, buf, sizeof(buf))));
    route->Set(mtu_sym, Integer::New(mtu));
    route->Set(String::New("window"), Integer::NewFromUnsigned(window));
    route->Set(rtt_sym, Integer::NewFromUnsigned(rtt));
    routes->Set(routes->Length(), route);
  }

  fclose(fp);

  return scope.Close(routes);
}


Handle<Value> GetRoutesIPv6() {
  HandleScope scope;
  Local<Array> routes = Array::New();

  FILE* fp = fopen("/proc/net/ipv6_route", "r");
  if (fp == NULL) return scope.Close(routes);

  while (!feof(fp)) {
    char dst[256];
    unsigned int dst_len;
    char src[256];
    unsigned int src_len;
    char gateway[256];
    unsigned int flags;
    int metric;
    unsigned int refcnt;
    unsigned int use;
    char iface[256];

    int nitems = fscanf(fp,
                        "%32s %02x %32s %02x %32s %08x %08x %08x %08x %s\n",
                        dst,
                        &dst_len,
                        src,
                        &src_len,
                        gateway,
                        &metric,
                        &refcnt,
                        &use,
                        &flags,
                        iface);
    ASSERT(nitems == 10);

    char buf[256];
    Hex2Bin(dst, 32);
    Hex2Bin(src, 32);
    Hex2Bin(gateway, 32);

    inet_ntop(AF_INET6, &dst, buf, sizeof(buf));
    snprintf(dst, sizeof(dst), "%s/%u", buf, dst_len);
    inet_ntop(AF_INET6, &src, buf, sizeof(buf));
    snprintf(src, sizeof(src), "%s/%u", buf, src_len);
    inet_ntop(AF_INET6, &gateway, buf, sizeof(buf));
    snprintf(gateway, sizeof(gateway), "%s", buf);

    Local<Object> route = Object::New();
    route->Set(dest_sym, String::New(dst));
    route->Set(String::New("source"), String::New(src));
    route->Set(gateway_sym, String::New(gateway));
    route->Set(String::New("metric"), Integer::New(metric));
    route->Set(String::New("refcnt"), Integer::NewFromUnsigned(refcnt));
    route->Set(String::New("use"), Integer::NewFromUnsigned(use));
    route->Set(String::New("flags"), Integer::NewFromUnsigned(flags));
    route->Set(interface_sym,
               String::New(reinterpret_cast<const char*>(iface)));
    routes->Set(routes->Length(), route);
  }

  fclose(fp);

  return scope.Close(routes);
}


Handle<Value> GetInfo(int family) {
  if (family == AF_INET) return GetRoutesIPv4();
  if (family == AF_INET6) return GetRoutesIPv6();
  abort();
}

} // namespace netroute

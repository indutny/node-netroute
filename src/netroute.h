#ifndef NETROUTE_H_
#define NETROUTE_H_

#include "v8.h"

namespace netroute {

extern v8::Persistent<v8::String> dest_sym;
extern v8::Persistent<v8::String> gateway_sym;
extern v8::Persistent<v8::String> netmask_sym;
extern v8::Persistent<v8::String> genmask_sym;
extern v8::Persistent<v8::String> ifp_sym;
extern v8::Persistent<v8::String> ifa_sym;
extern v8::Persistent<v8::String> interface_sym;
extern v8::Persistent<v8::String> mtu_sym;
extern v8::Persistent<v8::String> rtt_sym;
extern v8::Persistent<v8::String> expire_sym;

v8::Handle<v8::Value> GetInfo(int family);

} // namespace netroute

#endif // NETROUTE_H_

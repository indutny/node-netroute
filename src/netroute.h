#ifndef _SRC_NETROUTE_H_
#define _SRC_NETROUTE_H_

#include <node.h>
#include <node_object_wrap.h>

namespace netroute {

using namespace node;
using namespace v8;

class Netroute : public ObjectWrap {
 public:
  Netroute();
  ~Netroute();

  static Handle<Value> New(const Arguments& args);
  static Handle<Value> GetGateway(const Arguments& args);

  static void Init(Handle<Object> target);

 protected:
  int fd_;
};

} // namespace netroute

#endif // _SRC_NETROUTE_H_

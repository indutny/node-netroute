#include "node.h"
#include "node_object_wrap.h"
#include "netroute.h"

#include <errno.h>
#include <sys/socket.h> // socket
#include <net/if.h>
#include <net/route.h>
#include <unistd.h> // close

namespace netroute {

using namespace node;
using namespace v8;

Netroute::Netroute() {
  fd_ = socket(PF_ROUTE, SOCK_RAW, AF_INET);
  if (fd_ == -1) abort();
}


Netroute::~Netroute() {
  close(fd_);
}


Handle<Value> Netroute::New(const Arguments& args) {
  HandleScope scope;

  Netroute* r = new Netroute();

  r->Wrap(args.Holder());

  return scope.Close(args.This());
}


Handle<Value> Netroute::GetGateway(const Arguments& args) {
  HandleScope scope;
  Netroute* r = ObjectWrap::Unwrap<Netroute>(args.This());

  size_t raw_size = sizeof(rt_msghdr) + 512;

  rt_msghdr* msg = reinterpret_cast<rt_msghdr*>(malloc(raw_size));
  sockaddr_in* dest = reinterpret_cast<sockaddr_in*>(msg + 1);
  memset(msg, 0, raw_size);

  // Set message
  msg->rtm_msglen = sizeof(rt_msghdr) + sizeof(sockaddr_in);
  msg->rtm_version = RTM_VERSION;
  msg->rtm_type = RTM_GET;
  msg->rtm_addrs = RTA_DST;
  msg->rtm_pid = getpid();
  msg->rtm_seq = 1;

  // XXX: Support ip6 there
  // Set destination
  *dest = uv_ip4_addr("8.8.8.8", 0);

  // Send message to kernel
  ssize_t written = write(r->fd_, msg, msg->rtm_msglen);
  if (written != msg->rtm_msglen) {
    free(msg);
    return scope.Close(ThrowException(String::New(
        "Failed to write to kernel\'s route socket")));
  }

  // Receive answer
  do {
    ssize_t bytes = read(r->fd_, msg, raw_size);
    if (bytes <= 0) {
      free(msg);
      return scope.Close(ThrowException(String::New(
          "Failed to read from kernel\'s route socket")));
    }
  } while (msg->rtm_type != RTM_GET ||
           msg->rtm_seq != 1 ||
           msg->rtm_pid != getpid());

  // XXX: Support ipv6 there somehow?!
  if ((msg->rtm_addrs & RTA_GATEWAY) == 0) {
    free(msg);
    return scope.Close(ThrowException(String::New(
        "No gateway in the kernel response!")));
  }

  char ip[256];
  uv_ip4_name(&dest[RTAX_GATEWAY], ip, sizeof(ip));
  free(msg);

  return scope.Close(String::New(ip));
}


void Netroute::Init(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(Netroute::New);

  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(String::NewSymbol("Netroute"));

  NODE_SET_PROTOTYPE_METHOD(t, "getGateway", Netroute::GetGateway);

  target->Set(String::NewSymbol("Netroute"), t->GetFunction());
}

NODE_MODULE(netroute, Netroute::Init);

} // namespace netroute

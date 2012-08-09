var netroute = require('..'),
    assert = require('assert'),
    net = require('net');

describe('netroute', function() {
  it('should get gateway address', function() {
    assert(net.isIP(netroute.getGateway()));
  });
});

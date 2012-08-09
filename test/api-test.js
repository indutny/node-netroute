var netroute = require('..'),
    assert = require('assert'),
    net = require('net');

describe('netroute', function() {
  it('should get routing table', function() {
    var info = netroute.getInfo();
    assert(Array.isArray(info.IPv4));
    assert(Array.isArray(info.IPv6));

    info.IPv4.forEach(function(item) {
      assert(typeof item.interface === 'string');
      assert(typeof item.destination === 'string');
    });
  });
});

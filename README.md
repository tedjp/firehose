# firehose

Firehose is a program for forwarding multicast traffic, converting multicast
traffic to unicast, or unicast to multicast.

Its full operation involves running it on two different subnets, one to capture
multicast traffic and forward it as unicast to another instance, which
multicasts it to the destination network.

## Build

    make

There's also a sample multicast sender program called `sender`:

    make sender

## Usage

The first is considered the forwarder (fwd), the second is considered the
receiver (recv):

    Usage:
    ./firehose [options] fwd 'mcaddr:port' 'src:port' 'unidst:port' ['mc_local_if_addr:ignored']
    ./firehose [options] recv 'recvaddr:port' 'multidest:port' 'srcaddr:port'

    options:
    -v --verbose Verbose output (log each packet)

The address parameters are generally in traffic order:

`fwd` addresses:

* `mcaddr:port`
the address to subscribe to

* `src:port`
the multicast source address

* `unidst:port`
the unicast destination (receiver address)

* `mc_local_if_addr:ignored` (optional)
address to bind for receiving multicast traffic (port value unused)


`recv` addresses:

* `recvaddr:port`
Unicast address to receive on (should match `fwd`'s `unidst:port`)

* `multidest:port`
Multicast group address to send to

* `srcaddr:port`
Address to send multicast traffic from

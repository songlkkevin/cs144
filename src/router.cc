#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // Your code here.
  _routes.push_back( { route_prefix, prefix_length, next_hop, interface_num } );
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  // Your code here.
  for ( auto& interface : _interfaces ) {
    auto&& datagrams_received { interface->datagrams_received() };
    while ( not datagrams_received.empty() ) {
      int8_t interface_num = -1;
      int8_t longest_prefix_length = -1;
      optional<Address> next_hop;
      auto datagram { std::move( datagrams_received.front() ) };
      auto destination = datagram.header.dst;
      datagrams_received.pop();

      if ( datagram.header.ttl <= 1 ) {
        cerr << "DEBUG: TTL expired\n";
        continue;
      }
      datagram.header.ttl--;
      datagram.header.compute_checksum();

      for ( auto& curr_route : _routes ) {
        auto prefix = curr_route.route_prefix;
        int8_t curr_length = -1;
        if ( curr_route.prefix_length < longest_prefix_length ) {
          continue;
        }
        for ( int i = 0; i < curr_route.prefix_length; i++ ) {
          if ( ( prefix & ( 1 << ( 31 - i ) ) ) == ( destination & ( 1 << ( 31 - i ) ) ) ) {
            curr_length = i;
          } else {
            break;
          }
        }
        curr_length++;
        if ( curr_length >= longest_prefix_length && curr_length == curr_route.prefix_length ) {
          longest_prefix_length = curr_length;
          next_hop = curr_route.next_hop;
          interface_num = curr_route.interface_num;
        }
      }
      _interfaces[interface_num]->send_datagram(
        std::move( datagram ), next_hop.value_or( Address::from_ipv4_numeric( datagram.header.dst ) ) );
    }
  }
}

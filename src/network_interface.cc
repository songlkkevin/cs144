#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

static ARPMessage make_arp( const uint16_t opcode,
                            const EthernetAddress sender_ethernet_address,
                            const uint32_t sender_ip_address,
                            const EthernetAddress target_ethernet_address,
                            const uint32_t target_ip_address )
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ethernet_address = sender_ethernet_address;
  arp.sender_ip_address = sender_ip_address;
  arp.target_ethernet_address = target_ethernet_address;
  arp.target_ip_address = target_ip_address;
  return arp;
}

static EthernetFrame make_frame( const EthernetAddress& src,
                                 const EthernetAddress& dst,
                                 const uint16_t type,
                                 vector<string> payload )
{
  EthernetFrame frame;
  frame.header.src = src;
  frame.header.dst = dst;
  frame.header.type = type;
  frame.payload = std::move( payload );
  return frame;
}

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // Your code here.
  const auto ip = next_hop.ipv4_numeric();
  auto it = arp_cache_.find( ip );
  if ( it != arp_cache_.end() ) {
    transmit(
      make_frame( ethernet_address_, it->second.ethernet_address, EthernetHeader::TYPE_IPv4, serialize( dgram ) ) );
  } else {
    auto waiting_it = waiting_datagrams_.find( ip );

    if ( waiting_it == waiting_datagrams_.end() ) {
      waiting_datagrams_[ip] = queue<InternetDatagram>();
    }
    waiting_datagrams_[ip].push( dgram );
    if ( arp_request_.contains( ip ) ) {
      return;
    }

    arp_request_.insert( ip );
    const uint64_t expire_time = ms_since_start_ + ARP_REQUEST_TIMEOUT;
    if ( expire_time_.find( expire_time ) == expire_time_.end() ) {
      expire_time_[expire_time] = queue<ExpireEntry>();
    }
    expire_time_[expire_time].push( { ExpireEntry::QUERY, ip } );
    transmit( make_frame( ethernet_address_,
                          ETHERNET_BROADCAST,
                          EthernetHeader::TYPE_ARP,
                          serialize( make_arp( ARPMessage::OPCODE_REQUEST,
                                               ethernet_address_,
                                               ip_address_.ipv4_numeric(),
                                               {},
                                               next_hop.ipv4_numeric() ) ) ) );
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // Your code here.
  if ( frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST ) {
    return;
  }
  if ( frame.header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    auto ret = parse( dgram, frame.payload );
    if ( not ret ) {
      return;
    }
    datagrams_received_.push( dgram );
    return;
  }
  if ( frame.header.type == EthernetHeader::TYPE_ARP ) {
    ARPMessage arp;
    auto ret = parse( arp, frame.payload );
    if ( not ret ) {
      return;
    }
    arp_cache_[arp.sender_ip_address] = { arp.sender_ethernet_address, ms_since_start_ + ARP_CACHE_TIMEOUT };
    const uint64_t expire_time = ms_since_start_ + ARP_CACHE_TIMEOUT;
    if ( expire_time_.find( expire_time ) == expire_time_.end() ) {
      expire_time_[expire_time] = queue<ExpireEntry>();
    }
    expire_time_[expire_time].push( { ExpireEntry::CACHE, arp.sender_ip_address } );
    if ( arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == ip_address_.ipv4_numeric() ) {
      transmit( make_frame( ethernet_address_,
                            arp.sender_ethernet_address,
                            EthernetHeader::TYPE_ARP,
                            serialize( make_arp( ARPMessage::OPCODE_REPLY,
                                                 ethernet_address_,
                                                 ip_address_.ipv4_numeric(),
                                                 arp.sender_ethernet_address,
                                                 arp.sender_ip_address ) ) ) );
    }
    auto dgram_it = waiting_datagrams_.find( arp.sender_ip_address );
    if ( dgram_it != waiting_datagrams_.end() ) {
      while ( not dgram_it->second.empty() ) {
        auto dgram = dgram_it->second.front();
        dgram_it->second.pop();
        transmit( make_frame(
          ethernet_address_, arp.sender_ethernet_address, EthernetHeader::TYPE_IPv4, serialize( dgram ) ) );
      }
      waiting_datagrams_.erase( dgram_it );
    }
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  ms_since_start_ += ms_since_last_tick;
  auto it = expire_time_.begin();
  while ( it != expire_time_.end() ) {
    if ( it->first <= ms_since_start_ ) {
      while ( not it->second.empty() ) {
        auto entry = it->second.front();
        it->second.pop();
        if ( entry.type == entry.CACHE ) {
          auto it_arp = arp_cache_.find( entry.ip );
          if ( it_arp != arp_cache_.end() && it_arp->second.expire_time <= ms_since_start_ ) {
            arp_cache_.erase( it_arp );
          }
        } else {
          arp_request_.erase( entry.ip );
        }
      }
      it = expire_time_.erase( it );
    } else {
      break;
    }
  }
}

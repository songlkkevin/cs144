#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  if ( message.SYN ) {
    ISN_ = std::move( message.seqno );
    received_ISN_ = true;
  }
  if ( message.RST ) {
    reader().set_error();
  }
  if ( received_ISN_ == false || reader().has_error() ) {
    return;
  }
  const uint64_t checkpoint = reader().writer().bytes_pushed() + ( message.SYN ? 0 : 1 );
  const uint64_t abs_seqo = message.seqno.unwrap( std::move( ISN_ ), checkpoint );
  const uint64_t stream_index = abs_seqo - ( message.SYN ? 0 : 1 );
  reassembler_.insert( stream_index, std::move( message.payload ), message.FIN );
  return;
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  uint16_t window_size = writer().available_capacity() < UINT16_MAX ? writer().available_capacity() : UINT16_MAX;
  bool RST = reader().has_error();
  const uint64_t abs_seqo = writer().bytes_pushed() + 1 + ( writer().is_closed() ? 1 : 0 );
  const std::optional<Wrap32> ackno
    = received_ISN_ ? std::optional<Wrap32>( Wrap32::wrap( abs_seqo, std::move( ISN_ ) ) ) : std::nullopt;
  return { ackno, window_size, RST };
}

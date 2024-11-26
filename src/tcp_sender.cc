#include "tcp_sender.hh"
#include "tcp_config.hh"
#include "wrapping_integers.hh"
#include <cstdint>

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return bytes_in_flight_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  const uint64_t window_size = window_size_ ? window_size_ : 1;
  while ( bytes_in_flight_ < window_size ) {

    string payload_;
    TCPSenderMessage message = make_empty_message();
    if ( !SYN_sent_ ) {
      SYN_sent_ = true;
      message.SYN = true;
    }

    const uint64_t bytes_read_max = TCPConfig::MAX_PAYLOAD_SIZE - message.SYN;
    uint64_t bytes_valid = window_size > bytes_in_flight_ ? window_size - bytes_in_flight_ : 0;
    bytes_valid = min( bytes_valid, input_.reader().bytes_buffered() );
    const uint64_t bytes_read = min( bytes_read_max, bytes_valid );
    read( input_.reader(), bytes_read, payload_ );
    if ( !FIN_sent_ && reader().is_finished() && bytes_read == bytes_valid ) {
      FIN_sent_ = true;
      message.FIN = true;
    }
    message.payload = std::move( payload_ );
    if ( bytes_in_flight_ + message.sequence_length() > window_size ) {
      FIN_sent_ = false;
      message.FIN = false;
    }
    if ( message.sequence_length() == 0 && !message.RST ) {
      return;
    }
    outstanding_segments_.push_back( message );
    bytes_in_flight_ += message.sequence_length();
    if ( !outstanding_segments_.empty() && !timer_.is_running() ) {
      timer_.start();
    }
    transmit( message );
    if ( reader().bytes_buffered() == 0 ) {
      return;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return { Wrap32::wrap( ackno_abs_seqno_ + bytes_in_flight_, isn_ ), false, {}, false, input_.has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  const uint64_t ackno_abs_seqno = msg.ackno->unwrap( isn_, ackno_abs_seqno_ );
  window_size_ = msg.window_size;
  if ( msg.RST ) {
    input_.set_error();
  }
  if ( ackno_abs_seqno > ackno_abs_seqno_ + bytes_in_flight_ ) {
    return;
  }
  if ( ackno_abs_seqno >= ackno_abs_seqno_ + outstanding_segments_.front().sequence_length() ) {
    while ( ackno_abs_seqno >= ackno_abs_seqno_ + outstanding_segments_.front().sequence_length() ) {
      bytes_in_flight_ -= outstanding_segments_.front().sequence_length();
      ackno_abs_seqno_ += outstanding_segments_.front().sequence_length();
      outstanding_segments_.pop_front();
    }
    consecutive_retransmissions_ = 0;
    timer_.reset();
    if ( !outstanding_segments_.empty() ) {
      timer_.start();
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  timer_.elapsed( ms_since_last_tick );
  if ( timer_.is_expired() ) {
    if ( !outstanding_segments_.empty() ) {
      transmit( outstanding_segments_.front() );
    }
    timer_.start();
    if ( window_size_ != 0 ) {
      consecutive_retransmissions_++;
      timer_.exponential_backoff();
    }
  }
}

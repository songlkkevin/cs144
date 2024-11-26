#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>

class TCPTimer
{
public:
  TCPTimer( uint64_t initial_RTO_ms ) : initial_RTO_ms_( initial_RTO_ms ), RTO_ms_( initial_RTO_ms ) {}
  void start()
  {
    running_ = true;
    culumative_ms_ = 0;
  }
  void stop() { running_ = false; }
  void elapsed( uint64_t ms_since_last_tick ) { culumative_ms_ += ms_since_last_tick; }
  void exponential_backoff() { RTO_ms_ *= 2; }
  void reset() { RTO_ms_ = initial_RTO_ms_; }
  bool is_expired() { return culumative_ms_ >= RTO_ms_ && running_; }
  bool is_running() { return running_; }

private:
  uint64_t initial_RTO_ms_;
  uint64_t RTO_ms_;
  uint64_t culumative_ms_ = 0;

  bool running_ = false;
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), outstanding_segments_()
  {}

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  uint64_t ackno_abs_seqno_ = 0;
  uint64_t window_size_ = 1;

  bool SYN_sent_ = false;
  bool FIN_sent_ = false;

  std::list<TCPSenderMessage> outstanding_segments_;
  TCPTimer timer_ = TCPTimer( initial_RTO_ms_ );

  uint64_t bytes_in_flight_ = 0;
  uint64_t consecutive_retransmissions_ = 0;
};

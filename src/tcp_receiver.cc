#include "tcp_receiver.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include "wrapping_integers.hh"
#include <cstdint>
#include <cstdio>
#include <string>
#include <type_traits>
#include <utility>

using namespace std;

//? wrap32->uint64，似乎无法判断旧包重发的问题
void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  //* 收到syn包
  //* 收到数据包
  //* 收到fin包

  if ( fined_ )
    return;

  if ( message.RST ) {
    reassembler_.reader().set_error();
    // reassembler_.writer().set_error(); 区分
    return;
  }

  if ( message.SYN ) {
    if ( !syned_ ) {
      syned_ = true;
      abosulte_nextSeq++;
      ISN = message.seqno;
    } else {
      return;
    }
  }

  if ( !syned_ )
    return;

  { // 读取load
    uint64_t abosu_seq = message.seqno.unwrap( *ISN, abosulte_nextSeq );
    if ( abosu_seq == 0 ) { //! syn+data or ISN,data
      if ( message.SYN ) {
        abosu_seq = 1;
      } else {
        return;
      }
    }

    bool is_last = false;
    if ( message.FIN ) {
      is_last = true;
    }

    uint64_t stream_id = abosu_seq - 1;
    uint64_t presz = reassembler_.writer().available_capacity();
    reassembler_.insert( stream_id, move( message.payload ), is_last ); //?
    abosulte_nextSeq += presz - reassembler_.writer().available_capacity();
  }

  if ( reassembler_.writer().is_closed() ) {
    fined_ = true;
    abosulte_nextSeq++;
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  //? 所谓的send是向调用者返回一个包
  // x 既然需要send了，说明至少有ISN了，不需考虑确认序列号为空的情况

  TCPReceiverMessage newmes;
  if ( ISN.has_value() ) {
    Wrap32 seq_num = Wrap32::wrap( abosulte_nextSeq, *ISN );
    newmes.ackno = seq_num;
  }

  newmes.window_size = min( reassembler_.writer().available_capacity(), (uint64_t)UINT16_MAX );
  //! 报文的大小为4字节
  newmes.RST = reassembler_.reader().has_error();

  return newmes;
}
//? 旧包怎么处理？
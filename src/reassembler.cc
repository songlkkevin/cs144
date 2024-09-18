#include "reassembler.hh"
#include "byte_stream.hh"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <iostream>
using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.

  auto& op_writer = output_.writer();

  const char EMPTY = char( 0xAA ), UNEMPTY = char( 0xF3 );

  if ( buf_flags.size() != op_writer.available_capacity() ) {
    buf_unorderedByte.resize( op_writer.available_capacity() );
    buf_flags.resize( op_writer.available_capacity(), EMPTY );
  }

  //! 这里需要重点探讨区间位置的情况
  size_t buf_i, data_i;
  if ( first_index < Bstream_nextByte ) {
    buf_i = 0;
    data_i = Bstream_nextByte - first_index;
  } else {
    data_i = 0;
    buf_i = first_index - Bstream_nextByte;
  }

  for ( ; buf_i < buf_unorderedByte.size() && data_i < data.size(); buf_i++, data_i++ ) {
    if ( buf_flags[buf_i] == EMPTY ) {//! 重复过的包
      buf_unorderedByte[buf_i] = data[data_i];
      bytes_pendingNum++;
      buf_flags[buf_i] = UNEMPTY;
    }
  }

  if ( is_last_substring && data_i == data.size() ) {
    //! 关闭的条件是接收到最后一个包，并且该包的字节全部缓存下来，无丢失
    close_ = true;
  }

  if ( buf_flags[0] != EMPTY ) {
    size_t end = min( buf_flags.find( EMPTY ), buf_flags.size() );
    op_writer.push( buf_unorderedByte.substr( 0, end ) );
    Bstream_nextByte += end;
    
    bytes_pendingNum -= end;

    buf_unorderedByte.erase( 0, end );
    buf_flags.erase( 0, end );
  }
  //! 注意空包的情况。数据为空但带有结束位
  if ( close_ && bytes_pendingNum == 0 ) {
    op_writer.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return bytes_pendingNum;
}

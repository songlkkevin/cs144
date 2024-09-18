#include "byte_stream.hh"
#include <cstddef>
#include <assert.h>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
  // Your code here.
  return close_;
}

//! 添加数据的过程中若满，则丢弃后面的数据
void Writer::push( string data ) 
{
  // Your code here.
  size_t sz = min( available_capacity(), data.size() );
  num_push += sz;
  buf_.append( data.substr( 0, sz ) );  
}

void Writer::close()
{
  // Your code here.
  close_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - buf_.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return num_push;
}

bool Reader::is_finished() const
{
  // Your code here.
  return close_ && buf_.size() == 0;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return num_pop;
}

string_view Reader::peek() const
{
  return buf_;  
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  int sz = min( len, buf_.size() );
  num_pop+=sz;
  buf_.erase(0,sz);
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return buf_.size();
}

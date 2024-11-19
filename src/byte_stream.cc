#include "byte_stream.hh"
#include <cstdint>
#include <chrono>
#include <string_view>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), stream_("") {}

bool Writer::is_closed() const
{
  // Your code here.
  return close_;
}

void Writer::push( string data )
{
  // Your code here.
  // print time of the functions
    if (close_)
        return;
    const uint64_t size = bytes_pushed_ - bytes_poped_;
    const uint64_t capacity = capacity_ - size;
    const uint64_t data_size = data.size();
    const uint64_t bytes_pushed = min (data_size, capacity);
    if (data_size == bytes_pushed)
        stream_.append(data);
    else
        stream_.append(data.substr(0, bytes_pushed));
    bytes_pushed_ += bytes_pushed;
    return;
}

void Writer::close()
{
  // Your code here.
    close_ = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
    uint64_t size = bytes_pushed_ - bytes_poped_;
    return capacity_ - size;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
    return bytes_pushed_;
}

bool Reader::is_finished() const
{
  // Your code here.
    return close_ && (bytes_pushed_ == bytes_poped_);
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
    return bytes_poped_;
}

string_view Reader::peek() const
{
  // Your code here.
    const uint64_t size = bytes_pushed_ - bytes_poped_;
    if (size == 0)
        return string_view();
    if (erased_ < stream_.size()) {
        return std::string_view(stream_.data() + erased_, 1);
    } else {
        return std::string_view();
    }
}

void Reader::pop( uint64_t len )
{
  // Your code here.
    uint64_t size = bytes_pushed_ - bytes_poped_;
    len = min (len, size);
    erased_ += len;
    bytes_poped_ += len;
    if (erased_ >= capacity_) {
        stream_.erase(0, erased_);
        erased_ = 0;
    }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
    return bytes_pushed_ - bytes_poped_;
}

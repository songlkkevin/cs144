#include "reassembler.hh"
#include <utility>

using namespace std;

bool Reassembler::is_valid( uint64_t& first_index, string& data, bool& is_last_substring )
{
  const auto last_index = writer().available_capacity() + next_index_;
  if ( first_index >= last_index ) {
    return false;
  }
  if ( first_index < next_index_ ) {
    if ( first_index + data.size() <= next_index_ ) {
      return false;
    }
    const auto overlap = next_index_ - first_index;
    data = data.substr( overlap );
    first_index = next_index_;
  }
  if ( first_index + data.size() > last_index ) {
    const auto overlap = last_index - first_index;
    data = data.substr( 0, overlap );
    is_last_substring = false;
  }
  return true;
}

void Reassembler::colocate( uint64_t first_index )
{
  auto it = pending_.find( first_index );
  if ( it != pending_.begin() ) {
    auto prev = it;
    --prev;
    if ( prev->first + prev->second.first.size() >= it->first ) {
      if ( prev->first + prev->second.first.size() >= it->first + it->second.first.size() ) {
        pending_bytes_ -= it->second.first.size();
        prev->second.second = it->second.second;
        pending_.erase( it );
        return;
      }
      pending_bytes_ -= prev->first + prev->second.first.size() - it->first;
      prev->second.first += it->second.first.substr( prev->first + prev->second.first.size() - it->first );
      prev->second.second = it->second.second;
      pending_.erase( it );
      it = prev;
    }
  }

  auto next = it;
  ++next;
  while ( next != pending_.end() && next->first <= it->first + it->second.first.size() ) {
    if ( next->first + next->second.first.size() > it->first + it->second.first.size() ) {
      pending_bytes_ -= it->first + it->second.first.size() - next->first;
      it->second.first += next->second.first.substr( it->first + it->second.first.size() - next->first );
    } else {
      pending_bytes_ -= next->second.first.size();
    }
    it->second.second = next->second.second;
    pending_.erase( next++ );
  }
}

void Reassembler::write_to_stream()
{
  auto it = pending_.begin();
  if ( it->first == next_index_ ) {
    output_.writer().push( it->second.first );
    next_index_ += it->second.first.size();
    pending_bytes_ -= it->second.first.size();
    if ( it->second.second ) {
      output_.writer().close();
    }
    it = pending_.erase( it );
  }
}

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  if ( is_valid( first_index, data, is_last_substring ) == false ) {
    return;
  }

  auto it = pending_.find( first_index );
  if ( it != pending_.end() ) {
    if ( it->second.first.size() < data.size() ) {
      pending_bytes_ += data.size() - it->second.first.size();
      it->second.first = std::move( data );
    }
  } else {
    pending_bytes_ += data.size();
    pending_.emplace( first_index, std::make_pair( std::move( data ), is_last_substring ) );
  }
  colocate( first_index );
  write_to_stream();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return pending_bytes_;
}

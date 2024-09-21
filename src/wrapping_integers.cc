#include "wrapping_integers.hh"
#include <cstdint>
#include <cstdio>
#include <iostream>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.

  return Wrap32 { zero_point + (uint32_t)n };
}

uint64_t binary_search_target( uint64_t checkpoint, uint64_t offset )
{
  int64_t left = 0, right = (1<<20);
  uint64_t x = ( 1UL << 32 );
  auto check = [checkpoint, offset, x]( uint64_t target ) -> int64_t { return offset + x * target - checkpoint; };
  while ( left <= right ) {
    int64_t mid = ( left + right ) / 2;
    if ( check( mid ) < 0 ) {
      left = mid + 1;
    } else if ( check( mid ) == 0 ) {
      return offset + x * mid;
    } else {
      right = mid - 1;
    }
    printf( "val %lu\n", offset + x * mid );
    printf( "mid %ld left %lu right %lu \n", mid, left, right );
  }
  uint64_t lvalue = offset + x * left;
  uint64_t rvalue = offset + x * right;
  if ( right == -1 )
    return lvalue;
  if ( checkpoint - rvalue < lvalue - checkpoint ) {
    return rvalue;
  }
  return lvalue;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  uint64_t base = ( 1UL << 32 );
  uint64_t offset = ( raw_value_ - zero_point.raw_value_ + base ) % base;

  // offset = binary_search_target( checkpoint, offset );
  if ( offset >= checkpoint ) {
    return offset;
  }
  

  uint64_t mult = ( checkpoint - offset ) / base;
  uint64_t downv = offset + base * mult;
  uint64_t upv = offset + base * ( mult + 1 );
  //printf( "dv %lu upv %lu m %lu \n", downv, upv,mult);
  offset = downv;
  if ( checkpoint - downv > upv - checkpoint ) {
    offset = upv;
  }
  
  return offset;
}

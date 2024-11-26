#include "wrapping_integers.hh"
#include <cstdint>
#define WRAP32_MODULUS 0x100000000
#define WRAP32_MASK 0xFFFFFFFF

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { zero_point + (uint32_t)( n & WRAP32_MASK ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  const uint64_t abs_seqo
    = ( (int64_t)raw_value_ - (int64_t)zero_point.raw_value_ + WRAP32_MODULUS ) % WRAP32_MODULUS;
  const uint64_t nearest = ( (int64_t)checkpoint - (int64_t)abs_seqo ) / WRAP32_MODULUS * WRAP32_MODULUS + abs_seqo;
  const uint64_t complement_nearest = nearest + ( nearest < checkpoint ? WRAP32_MODULUS : -WRAP32_MODULUS );
  const uint64_t diff1 = nearest > checkpoint ? nearest - checkpoint : checkpoint - nearest;
  const uint64_t diff2
    = complement_nearest > checkpoint ? complement_nearest - checkpoint : checkpoint - complement_nearest;
  return diff1 < diff2 ? nearest : complement_nearest;
}

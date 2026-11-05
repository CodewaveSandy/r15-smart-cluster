#pragma once
#include <cstdint>
#include <cstddef>
namespace cluster::binary {
inline std::uint64_t get(const std::uint8_t* b,unsigned at,unsigned n=4){std::uint64_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint64_t(b[at+i])<<(8*i);return v;}
inline void put(std::uint8_t* b,unsigned at,std::uint64_t v,unsigned n=4){for(unsigned i=0;i<n;++i)b[at+i]=std::uint8_t(v>>(8*i));}
inline bool zero(const std::uint8_t* b,unsigned at,unsigned n){for(unsigned i=0;i<n;++i)if(b[at+i])return false;return true;}
inline std::uint32_t crc(const std::uint8_t* b,unsigned n){std::uint32_t c=0xffffffffu;for(unsigned i=0;i<n;++i){c^=b[i];for(unsigned j=0;j<8;++j)c=(c>>1)^((c&1)?0xedb88320u:0u);}return c^0xffffffffu;}
inline void seal(std::uint8_t* b,unsigned n){put(b,n-4,crc(b,n-4));}
inline bool intact(const std::uint8_t* b,unsigned n){return n>=4&&get(b,n-4)==crc(b,n-4);}
inline bool forward(std::uint32_t a,std::uint32_t b){const auto d=a-b;return d&&d<0x80000000u;}
}

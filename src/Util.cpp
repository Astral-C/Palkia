#include <Util.hpp>

namespace Palkia {

uint8_t cv3To8(uint8_t v){
    return (v << 5) | (v << 2) | (v >> 1);
}

uint8_t cv5To8(uint8_t v){
    return (v << 3) | (v >> 2);
}

uint8_t s3tcBlend(uint8_t a, uint8_t b){
    return (((a << 1) + a) + ((b << 2) + b)) >> 3;
}

uint32_t PadTo32(uint32_t x){
    return ((x + (32-1)) & ~(32-1));
}

uint32_t Pad(uint32_t x, uint32_t y){
    return ((x + (y-1)) & ~(y-1));
}


}
#include "../firmware/ToyBLE/ImageBounds.h"
#include <cassert>
#include <vector>
#include <cstdio>
int main(){
 std::vector<unsigned char> b(80);b[0]=0xe9;b[1]=1;b[23]=1;b[28]=8;
 auto read=[&](uint32_t o,uint8_t *p,uint32_t n){if(o>b.size()||n>b.size()-o)return false;memcpy(p,b.data()+o,n);return true;};
 assert(imageBounds(80,read));assert(!imageBounds(39,read));assert(!imageBounds(79,read));
 b[28]=0xff;b[29]=0xff;b[30]=0xff;b[31]=0xff;assert(!imageBounds(80,read));
 b[1]=17;assert(!imageBounds(80,read));b[1]=0;assert(!imageBounds(80,read));b[1]=1;b[0]=0;assert(!imageBounds(80,read));
 puts("PASS OTA image segment/trailer bounds");
}

#include "../firmware/ToyBLE/PeripheralProtocol.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){PeripheralState s;assert(peripheralCommand("screen closed",s)==ACCEPTED&&s.screen==CLOSED);assert(peripheralCommand("screen explosion",s)==INVALID&&s.screen==CLOSED);assert(peripheralCommand("contrast 0",s)==ACCEPTED&&s.contrast==0);assert(peripheralCommand("contrast 256",s)==INVALID&&s.contrast==0);assert(peripheralCommand("touch threshold 1000",s)==ACCEPTED&&s.threshold==1000);assert(peripheralCommand("touch threshold 999",s)==INVALID&&s.threshold==1000);assert(peripheralCommand("touch threshold 2000001",s)==INVALID);assert(peripheralCommand("touch off",s)==ACCEPTED&&!s.touch);assert(peripheralCommand("touch on",s)==ACCEPTED&&s.touch);
assert(peripheralCommand("face -100 100 0 100 happy",s)==ACCEPTED);
assert(s.face.x==-100&&s.face.y==100&&s.face.left==0&&s.face.right==100&&s.face.expression==HAPPY);
for(const char *bad:{"face -101 0 0 0 neutral","face 0 101 0 0 neutral","face 0 0 -1 0 neutral","face 0 0 0 101 neutral","face 0 0 0 0 scary","face 0 0 0 0 neutral extra","face 999999999999999999 0 0 0 neutral","face 0 0 0 neutral","face 0 0 0 0 neutral "}){
 assert(peripheralCommand(bad,s)==INVALID);assert(s.face.x==-100&&s.face.right==100&&s.face.expression==HAPPY);
}
assert(peripheralCommand("face 0 0 25 75 angry",s)==ACCEPTED&&s.face.left==25&&s.face.right==75&&s.face.expression==ANGRY);
assert(peripheralCommand("face 0 -100 100 0 surprised",s)==ACCEPTED&&s.face.y==-100&&s.face.expression==SURPRISED);
assert(peripheralCommand("face 0 0 0 0 neutral",s)==ACCEPTED&&s.face.expression==NEUTRAL&&s.face.left==0);
puts("PASS peripheral bounds, atomic face updates and expressions");}

#include "../firmware/ToyBLE/PeripheralProtocol.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
int main(){
 PeripheralState s;
 assert(peripheralCommand("eyes 0 -100 100 -100 25 75 bottom neutral",s)==ACCEPTED);
 assert(s.face.y==-100&&s.face.pupilX==100&&s.face.pupilY==-100&&s.face.pupils&&s.face.bottomLids);
 for(const char *bad:{"eyes 0 0 101 0 0 0 bottom neutral","eyes 0 0 0 -101 0 0 top neutral","eyes 0 0 0 0 101 0 bottom neutral","eyes 0 0 0 0 0 0 sideways neutral","eyes 0 0 0 0 0 0 top bogus","eyes 0 0 0 0 0 0 top neutral extra","eyes 0 0 0 0 0 0 top","eyes 0 0 0 0 0 0 top neutral "}){
  assert(peripheralCommand(bad,s)==INVALID);assert(s.face.y==-100&&s.face.pupilX==100&&s.face.right==75&&s.face.bottomLids);
 }
 assert(peripheralCommand("eyes 0 0 0 0 50 50 top happy",s)==ACCEPTED&&!s.face.bottomLids);
 assert(peripheralCommand("face 0 0 0 0 neutral",s)==ACCEPTED&&!s.face.pupils);
 puts("PASS independent eye fields, legacy compatibility and atomic invalid-input rejection");
}

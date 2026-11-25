#include "../firmware/ToyBLE/PeripheralProtocol.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
 PeripheralState s;
 assert(peripheralCommand("doodle -100 100 -100 100 100 50 0 nervous",s)==ACCEPTED);
 assert(s.screen==EYES&&!strcmp(expressionName(s.face.expression),"nervous"));
 assert(peripheralCommand("doodle 7 -8 9 -10 11 12 13 sorry",s)==ACCEPTED);
 auto before=s.face;
 for(const char *bad:{"doodle 101 0 0 0 0 0 0 happy","doodle 0 0 0 0 101 0 0 happy","doodle 0 0 0 0 0 101 0 happy","doodle 0 0 0 0 0 0 0 bad","doodle 0 0 0 0 0 0 0 sorry extra"}){
  assert(peripheralCommand(bad,s)==INVALID);assert(s.face.expression==before.expression);
  assert(s.face.eyeBend==7&&s.face.browBend==-8&&s.face.mouthBend==9&&s.face.mouthSkew==-10&&s.face.jaw==11&&s.face.left==12&&s.face.right==13&&s.face.doodle);
 }
 assert(peripheralCommand("eyes 0 0 0 0 0 0 bottom happy",s)==ACCEPTED);
 puts("PASS doodle commands and invalid-input atomicity; legacy eyes accepted");
}

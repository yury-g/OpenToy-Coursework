// Export the preserved renderer's exact 128x64 bitmaps for editable documentation art.
#include "../firmware/FeatherFace/CachedFace.h"
#include "../firmware/FeatherFace/BatteryDiagnostics.h"

#include <cstdio>
int main(){
 unsigned levels[]={5,35,60,85,100};printf("[");
 for(unsigned n=0;n<5;n++){BatteryState b;b.update(3.5f+levels[n]*.007f,levels[n],0,100);auto pose=batteryPose(b);CachedFace face;const auto *p=face.pixels(pose,false);if(n)printf(",");printf("{\"percent\":%u,\"pixels\":[",levels[n]);for(unsigned i=0;i<1024;i++){if(i)printf(",");printf("%u",p[i]);}printf("]}");}printf("]\n");
}

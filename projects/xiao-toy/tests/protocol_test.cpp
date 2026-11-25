// SPDX-License-Identifier: MIT
#include "../firmware/ToyBLE/Protocol.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main() {
 ToyState s; assert(s.period == 1000 && s.duty == 50 && s.mode == BLINK);
 assert(applyCommand("period 250", s) == ACCEPTED && s.period == 250);
 assert(applyCommand("period 99", s) == INVALID && s.period == 250);
 assert(applyCommand("period 10001", s) == INVALID);
 assert(applyCommand("period 250junk", s) == INVALID);
 assert(applyCommand("period -200", s) == INVALID);
 assert(applyCommand("period 9999999999999999999999999", s) == INVALID);
 assert(applyCommand("duty 35", s) == ACCEPTED && s.duty == 35);
 assert(applyCommand("duty 0", s) == INVALID && s.duty == 35);
 assert(applyCommand("duty 100", s) == INVALID);
 assert(applyCommand("mode off", s) == ACCEPTED && s.mode == OFF);
 assert(applyCommand("mode on", s) == ACCEPTED && s.mode == ON);
 assert(applyCommand("mode blink", s) == ACCEPTED && s.mode == BLINK);
 assert(applyCommand("mode explode", s) == INVALID && s.mode == BLINK);
 assert(applyCommand("status", s) == STATUS_QUERY);
 assert(applyCommand("reset", s) == ACCEPTED && s.period == 1000 && s.duty == 50);
 FrameParser p; Frame f;
 for (char c : "period 300") if(c) assert(!p.feed(c,f));
 assert(p.feed(';',f) && f.kind == TEXT && strcmp(f.text,"period 300") == 0);
 assert(p.feed('\n',f) == false);
 // Unknown, oversized and embedded-NUL input must not apply a truncated command.
 for(int i=0;i<100;i++) assert(!p.feed('x',f));
 assert(p.feed(';',f) && f.kind == BAD_FRAME);
 const char *cmd="status;";bool got=false;for(size_t i=0;i<strlen(cmd);i++) got=p.feed(cmd[i],f);
 assert(got && f.kind == TEXT && strcmp(f.text,"status") == 0);
 p.feed('a',f);p.feed(0,f);assert(p.feed(';',f) && f.kind == BAD_FRAME);
 // App control-pad packet !B61 + ones-complement checksum.
 uint8_t b[]={33,66,54,49,static_cast<uint8_t>(~(33+66+54+49))};
 for(int i=0;i<4;i++) assert(!p.feed(b[i],f));
 assert(p.feed(b[4],f) && f.kind == BUTTON && f.button == 6 && f.pressed);
 applyButton(f.button,f.pressed,s);assert(s.period == 1200);
 applyButton(6,false,s);assert(s.period == 1200);
 for(int i=0;i<4;i++)p.feed(b[i],f);assert(p.feed(0,f) && f.kind == BAD_FRAME);
 // Cross-session partial input is discarded.
 p.feed('p',f);p.reset();got=false;for(size_t i=0;i<strlen(cmd);i++)got=p.feed(cmd[i],f);
 assert(got && strcmp(f.text,"status") == 0);
 for(int i=0;i<200;i++)applyButton(5,true,s);assert(s.period==100);
 for(int i=0;i<200;i++)applyButton(6,true,s);assert(s.period==10000);
 for(int i=0;i<200;i++)applyButton(7,true,s);assert(s.duty==1);
 for(int i=0;i<200;i++)applyButton(8,true,s);assert(s.duty==99);
 puts("PASS: bounds, malformed commands, framing, overflow, binary checksum, reset, button limits");
}

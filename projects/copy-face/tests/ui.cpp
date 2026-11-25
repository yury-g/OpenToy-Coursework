// SPDX-License-Identifier: MIT
#include "../core/Ui.h"
#include <cassert>
#include <cstdio>
using namespace copy_face;
int main() {
 Ui ui;TouchMenu touch;
 touch.sample(ui,760,20,1,10);assert(ui.page==Page::Closed);
 touch.sample(ui,760,20,0,30);assert(ui.page==Page::Main);
 ui.page=Page::Closed;
 touch.sample(ui,760,20,1,50);touch.sample(ui,500,250,1,70);touch.sample(ui,760,20,0,90);assert(ui.page==Page::Closed);
 ui.apply(Action::Performance);assert(ui.performance&&ui.page==Page::Closed);
 for(unsigned now=100;now<=900;now+=100)touch.sample(ui,512,20,1,now);
 assert(!ui.performance&&ui.page==Page::Main);
 touch.sample(ui,512,20,0,950);assert(ui.page==Page::Main);
 ui.apply(Action::Performance);
 touch.sample(ui,200,200,1,1000);for(unsigned now=1100;now<=2000;now+=100)touch.sample(ui,512,20,1,now);
 assert(ui.performance);touch.cancel();
 touch.sample(ui,512,20,1,2100);touch.expire(2400);touch.sample(ui,512,20,1,2900);assert(ui.performance);
 touch.cancel();ui.apply(Action::Menu);ui.apply(Action::Display);assert(ui.page==Page::Display);
 ui.apply(Action::Developer);assert(!ui.developer);ui.apply(Action::Back);assert(ui.page==Page::Main);
 ui.apply(Action::Return);assert(ui.page==Page::Closed);assert(!Ui().performance);
 puts("UI: release activation, drag cancellation, restore hold, stale input and menu paths passed");
}

"""Compile the real adapter against only a fake I2C transport; no hardware access."""
import subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ZIG='/Users/example/Documents/Codex/2026-10-01/handle-this-project-end-to-end/work/zig-aarch64-macos-0.16.0/zig'
WIRE=r'''
#pragma once
#include <cstdint>
#include <vector>
struct FakeWire {
 bool present=true,wrongProduct=false,shortRead=false,invalidAdc=false,failWrite=false;
 uint8_t address=0x52,selectedAddress=0,module=0,reg=0;unsigned pos=0;
 std::vector<uint8_t> tx,rx;std::vector<std::vector<uint8_t>> writes;
 void beginTransmission(uint8_t a){selectedAddress=a;tx.clear();}
 void write(uint8_t b){tx.push_back(b);}
 unsigned endTransmission(){
  if(!present||selectedAddress!=address)return 2;
  if(tx.size()==2){module=tx[0];reg=tx[1];return 0;}
  writes.push_back(tx);return failWrite?2:0;
 }
 unsigned requestFrom(uint8_t a,uint8_t count){
  rx.clear();pos=0;if(!present||a!=address)return 0;
  uint32_t value=0;
  if(module==0&&reg==1)value=0x86;
  else if(module==0&&reg==2)value=(wrongProduct?5742u:5743u)<<16;
  else if(module==1&&reg==4)value=(1u<<16)|0x67;
  else if(module==9)value=invalidAdc?65535:512;
  for(int shift=(count-1)*8;shift>=0;shift-=8)rx.push_back(uint8_t(value>>shift));
  if(shortRead&&module==1&&reg==4)rx.resize(2);
  return rx.size();
 }
 int available(){return int(rx.size()-pos);}
 uint8_t read(){return rx.at(pos++);}
};
inline FakeWire Wire;
'''
SOURCE=r'''
#include <cassert>
#include <cstring>
#include <Wire.h>
uint32_t millis(){return 1000;}
void delayMicroseconds(uint16_t){}
struct {template<class... T>void printf(const char *,T...) {}} Serial;
#include "FeatherGamepad.h"
int main(int argc,char**argv){
 FeatherGamepad pad;
 if(!strcmp(argv[1],"identity")){
  assert(pad.begin(0)&&pad.address==0x52&&pad.product==5743&&pad.hardware==0x86);
  assert(Wire.writes.size()==3);
  const uint8_t registers[]={3,0x0b,5};
  for(unsigned i=0;i<3;i++){
   auto w=Wire.writes[i];assert(w.size()==6&&w[0]==1&&w[1]==registers[i]);
   uint32_t mask=uint32_t(w[2])<<24|uint32_t(w[3])<<16|uint32_t(w[4])<<8|w[5];assert(mask==GamepadFace::MASK);
  }
  pad.service(25);assert(pad.ready&&pad.controls.connected&&!pad.controls.manual&&pad.rawX==512&&pad.rawY==512&&pad.samples==1);
 }else if(!strcmp(argv[1],"wrong-product")){
  Wire.wrongProduct=true;assert(!pad.begin(0)&&Wire.writes.empty());
 }else{
  assert(pad.begin(0));pad.service(25);pad.controls.manual=true;pad.controls.lookX=6;
  if(!strcmp(argv[1],"unplug"))Wire.present=false;
  else if(!strcmp(argv[1],"short"))Wire.shortRead=true;
  else {assert(!strcmp(argv[1],"invalid"));Wire.invalidAdc=true;}
  pad.service(50);assert(!pad.ready&&!pad.controls.manual&&!pad.controls.connected&&pad.controls.lookX==0&&pad.errors==1);
  Wire.present=true;Wire.shortRead=Wire.invalidAdc=false;pad.service(1049);assert(!pad.ready);
  pad.service(1050);assert(pad.ready);pad.service(1075);assert(pad.controls.connected&&!pad.controls.manual);
 }
}
'''
class FeatherGamepadAdapter(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();p=Path(cls.tmp.name)
  (p/'Wire.h').write_text(WIRE);(p/'adapter.cpp').write_text(SOURCE);cls.exe=p/'adapter'
  r=subprocess.run([ZIG,'c++','-std=c++17','-I'+str(p),'-I'+str(ROOT/'firmware/FeatherFace'),str(p/'adapter.cpp'),'-o',str(cls.exe)],capture_output=True,text=True)
  if r.returncode:raise RuntimeError(r.stderr)
 @classmethod
 def tearDownClass(cls):cls.tmp.cleanup()
 def run_case(self,case):
  r=subprocess.run([str(self.exe),case],capture_output=True,text=True)
  self.assertEqual(r.returncode,0,r.stderr)
 def test_identity_address_and_only_volatile_button_configuration(self):self.run_case('identity')
 def test_unrelated_product_is_never_configured(self):self.run_case('wrong-product')
 def test_unplug_returns_to_automatic_and_retry_is_bounded(self):self.run_case('unplug')
 def test_short_gpio_read_cannot_become_six_presses(self):self.run_case('short')
 def test_invalid_adc_cannot_control_face(self):self.run_case('invalid')

import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from config_check import validate_config
class ConfigTest(unittest.TestCase):
 def test_rejects_silently_downgraded_memory_clock(self):
  good='CONFIG_IDF_TARGET="esp32p4"\nCONFIG_SPIRAM_SPEED=200\nCONFIG_ESPTOOLPY_FLASHSIZE="16MB"\nCONFIG_PARTITION_TABLE_OFFSET=0x8000\nCONFIG_LV_COLOR_DEPTH=16\n'
  validate_config(good)
  with self.assertRaisesRegex(ValueError,'SPIRAM_SPEED'):validate_config(good.replace('SPEED=200','SPEED=20'))
 def test_missing_target_is_rejected(self):
  with self.assertRaisesRegex(ValueError,'IDF_TARGET'):validate_config('')
if __name__=='__main__':unittest.main()

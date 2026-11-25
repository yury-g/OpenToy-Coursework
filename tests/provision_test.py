import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from provision import parse_identity
class IdentificationTests(unittest.TestCase):
 def test_known_s3_and_eight_megabytes(self):
  self.assertEqual(parse_identity('Chip type: ESP32-S3 (QFN56)\nMAC: 02:00:00:00:00:02','Detected flash size: 8MB'),'02:00:00:00:00:02')
 def test_wrong_chip_rejected(self):
  with self.assertRaises(ValueError):parse_identity('Chip type: ESP32-C3\nMAC: 02:00:00:00:00:02','Detected flash size: 8MB')
 def test_missing_mac_rejected(self):
  with self.assertRaises(ValueError):parse_identity('Chip type: ESP32-S3','Detected flash size: 8MB')
 def test_wrong_flash_rejected(self):
  with self.assertRaises(ValueError):parse_identity('Chip type: ESP32-S3\nMAC: 02:00:00:00:00:02','Detected flash size: 4MB')
if __name__=='__main__':unittest.main()

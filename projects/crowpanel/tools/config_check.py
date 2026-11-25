"""Reject silent Kconfig fallbacks that invalidate this board's build assumptions."""
def validate_config(text):
 values=dict(line.split('=',1) for line in text.splitlines() if line.startswith('CONFIG_') and '=' in line)
 required={'CONFIG_IDF_TARGET':'"esp32p4"','CONFIG_SPIRAM_SPEED':'200',
           'CONFIG_ESPTOOLPY_FLASHSIZE':'"16MB"','CONFIG_PARTITION_TABLE_OFFSET':'0x8000',
           'CONFIG_LV_COLOR_DEPTH':'16'}
 for name,expected in required.items():
  if values.get(name)!=expected:raise ValueError(f'{name}: expected {expected}, got {values.get(name)}')

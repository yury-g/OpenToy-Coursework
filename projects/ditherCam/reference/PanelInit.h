// SPDX-License-Identifier: Apache-2.0
// Waveshare ST7305 initialization, source revision 9e754fdf. See THIRD-PARTY.md.
void Panel::begin()
{
  reset();

  const uint8_t d6[] = {0x17, 0x02};
  const uint8_t d1[] = {0x01};
  const uint8_t c0[] = {0x11, 0x04};
  const uint8_t c1[] = {0x69, 0x69, 0x69, 0x69};
  const uint8_t c2[] = {0x19, 0x19, 0x19, 0x19};
  const uint8_t c4[] = {0x4B, 0x4B, 0x4B, 0x4B};
  const uint8_t d8[] = {0x80, 0xE9};
  const uint8_t b2[] = {0x02};
  const uint8_t b3[] = {0xE5, 0xF6, 0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
  const uint8_t b4[] = {0x05, 0x46, 0x77, 0x77, 0x77, 0x77, 0x76, 0x45};
  const uint8_t g_timing[] = {0x32, 0x03, 0x1F};
  const uint8_t b7[] = {0x13};
  const uint8_t b0[] = {0x64};
  const uint8_t c9[] = {0x00};
  const uint8_t m36[] = {0x48};
  const uint8_t m3a[] = {0x11};
  const uint8_t b9[] = {0x20};
  const uint8_t b8[] = {0x29};
  const uint8_t win_a[] = {0x12, 0x2A};
  const uint8_t win_b[] = {0x00, 0xC7};
  const uint8_t m35[] = {0x00};
  const uint8_t d0[] = {0xFF};

  _cmd_data(0xD6, d6, sizeof(d6));
  _cmd_data(0xD1, d1, sizeof(d1));
  _cmd_data(0xC0, c0, sizeof(c0));
  _cmd_data(0xC1, c1, sizeof(c1));
  _cmd_data(0xC2, c2, sizeof(c2));
  _cmd_data(0xC4, c4, sizeof(c4));
  _cmd_data(0xC5, c2, sizeof(c2));
  _cmd_data(0xD8, d8, sizeof(d8));
  _cmd_data(0xB2, b2, sizeof(b2));
  _cmd_data(0xB3, b3, sizeof(b3));
  _cmd_data(0xB4, b4, sizeof(b4));
  _cmd_data(0x62, g_timing, sizeof(g_timing));
  _cmd_data(0xB7, b7, sizeof(b7));
  _cmd_data(0xB0, b0, sizeof(b0));
  _cmd(0x11);
  delay(120);
  _cmd_data(0xC9, c9, sizeof(c9));
  _cmd_data(0x36, m36, sizeof(m36));
  _cmd_data(0x3A, m3a, sizeof(m3a));
  _cmd_data(0xB9, b9, sizeof(b9));
  _cmd_data(0xB8, b8, sizeof(b8));
  _cmd(0x21);
  _cmd_data(0x2A, win_a, sizeof(win_a));
  _cmd_data(0x2B, win_b, sizeof(win_b));
  _cmd_data(0x35, m35, sizeof(m35));
  _cmd_data(0xD0, d0, sizeof(d0));
  _cmd(0x38);
  _cmd(0x29);
}

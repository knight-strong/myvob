#ifndef _ir_keymap_mod_
#define _ir_keymap_mod_

#define KEY_VOLUMEUP_MOD    KEY_F5
#define KEY_VOLUMEDOWN_MOD  KEY_F6
#define KEY_MUTE_MOD        KEY_F7
#define KEY_POWER_MOD       KEY_F8
#define KEY_SYNC_MOD        KEY_F9
#define KEY_HOME_MOD        KEY_F11

/*
Vendor code:ff00
04  power
01  up
03  menu
47  mute
02  vol-
07  down
41  vol+
40  space
0a  1
0b  2
49  3
48  4
0e  5
0f  6
4d  7
4c  8
12  9
13  0
51  .
50  syn
1a  <
17  ^
59  >
58  del
1b  ok
1f  v
5d  |<<
5c  >>|
10  stop
15  pause/resume
5a  <<
5f  >>
*/
static const unsigned int ir_keycodes_map_common[][2] = {
    {0x01,  KEY_UP},
    {0x02,  KEY_VOLUMEDOWN_MOD},
    {0x03,  KEY_MENU},
    {0x04,  KEY_POWER_MOD},
    {0x0A,  KEY_1},
    {0x0B,  KEY_2},
    {0x0E,  KEY_5},
    {0x0F,  KEY_6},
    {0x10,  KEY_STOPCD},
    {0x12,  KEY_9},
    {0x13,  KEY_0},
    {0x15,  KEY_PLAYPAUSE},
    {0x17,  KEY_UP},
    {0x1A,  KEY_LEFT},
    {0x1B,  KEY_ENTER},
    {0x1F,  KEY_DOWN},
    {0x40,  KEY_SPACE},
    {0x41,  KEY_VOLUMEUP_MOD},
    {0x47,  KEY_MUTE_MOD},
    {0x48,  KEY_4},
    {0x49,  KEY_3},
    {0x4C,  KEY_8},
    {0x4D,  KEY_7},
    {0x50,  KEY_SYNC_MOD},
    {0x51,  KEY_DOT},
    {0x58,  KEY_BACKSPACE},
    {0x59,  KEY_RIGHT},
    {0x5A,  KEY_FRAMEBACK},
    {0x5C,  KEY_NEXTSONG},
    {0x5D,  KEY_PREVIOUSSONG},
    {0x5F,  KEY_FRAMEFORWARD},
};

/*
Vendor code:fe01
17  power
02  mute
18  >||
0d  stop
0e  <<
03  >>
19  home/exit
04  menu
1b  del
06  backspace
0f  up
11  down
1a  left
05  right
10  ok
1c  1
12  2
07  3
1d  4
13  5
08  6
1e  7
14  8
09  9
1f  @:
15  0
0a  #unknown#
 */
static const unsigned int ir_keycodes_map_white[][2] = {
    // {0x02,  KEY_MUTE},
    // {0x03,  KEY_FRAMEFORWARD},
    {0x04,  KEY_F10},
    {0x05,  KEY_RIGHT},
    {0x06,  KEY_F12},
    {0x07,  KEY_3},
    {0x08,  KEY_6},
    {0x09,  KEY_9},
    // {0x0D,  KEY_STOPCD},
    // {0x0E,  KEY_FRAMEBACK},
    {0x0F,  KEY_UP},
    {0x10,  KEY_ENTER},
    {0x11,  KEY_DOWN},
    {0x12,  KEY_2},
    {0x13,  KEY_5},
    {0x14,  KEY_8},
    {0x15,  KEY_0},
    // {0x17,  KEY_POWER_MOD},
    // {0x18,  KEY_PLAYPAUSE},
    {0x19,  KEY_HOME_MOD},
    {0x1A,  KEY_LEFT},
    // {0x1B,  KEY_DELETE},
    {0x1C,  KEY_1},
    {0x1D,  KEY_4},
    {0x1E,  KEY_7},
    // {0x1F,  KEY_DOT},
    // {0x0a,  KEY_ENTER},
};

/*
    vendor code:50af(bd02)
 17 power
 0f up
 11 down
 1a left
 05 right
 10 enter
 89 +
 90 -
 19 undo/home
 04 menu
 06 options/return
 02 home /unused
 * */
static const unsigned int ir_keycodes_map_black[][2] = {
    // {0x02,  KEY_HOME},
    {0x04,  KEY_F10},
    {0x05,  KEY_RIGHT},
    {0x06,  KEY_F12},
    {0x0F,  KEY_UP},
    {0x10,  KEY_ENTER},
    {0x11,  KEY_DOWN},
    {0x19,  KEY_HOME_MOD},
    // {0x17,  KEY_POWER_MOD},
    {0x1A,  KEY_LEFT},
    {0x89,  KEY_VOLUMEUP_MOD},
    {0x90,  KEY_VOLUMEDOWN_MOD},
};


#endif


# Quick Reference Card - Common Keycodes

## Letters (Uppercase)
```
A=65   B=66   C=67   D=68   E=69   F=70   G=71   H=72
I=73   J=74   K=75   L=76   M=77   N=78   O=79   P=80
Q=81   R=82   S=83   T=84   U=85   V=86   W=87   X=88
Y=89   Z=90
```

## Numbers
```
0=48   1=49   2=50   3=51   4=52
5=53   6=54   7=55   8=56   9=57
```

## Function Keys
```
F1=194   F2=195   F3=196   F4=197
F5=198   F6=199   F7=200   F8=201
F9=202   F10=203  F11=204  F12=205
```

## Special Keys
```
ENTER=176        ESC=177          BACKSPACE=178
TAB=179          SPACE=32         CAPSLOCK=193
DELETE=212       INSERT=209
```

## Navigation
```
UP=218           DOWN=217         LEFT=216         RIGHT=215
HOME=210         END=213          PAGEUP=211       PAGEDOWN=214
```

## Media Keys
```
MUTE=127              VOLUME_DOWN=129       VOLUME_UP=128
PLAY_PAUSE=232        NEXT_TRACK=235        PREV_TRACK=234
STOP=233
```

## Modifiers (Add these values together)
```
CTRL  = 1
SHIFT = 2
ALT   = 4
GUI   = 8  (Windows/Command key)

Examples:
  Ctrl+Shift = 1+2 = 3
  Ctrl+Alt   = 1+4 = 5
  All four   = 1+2+4+8 = 15
```

## Common Shortcuts

### Copy/Paste/Undo
```
Ctrl+C  → REMAP 0 1 67 1
Ctrl+V  → REMAP 0 2 86 1
Ctrl+X  → REMAP 0 3 88 1
Ctrl+Z  → REMAP 0 4 90 1
Ctrl+Y  → REMAP 0 5 89 1
```

### File Operations
```
Ctrl+S     → REMAP 0 1 83 1
Ctrl+O     → REMAP 0 2 79 1
Ctrl+N     → REMAP 0 3 78 1
Ctrl+W     → REMAP 0 4 87 1
Alt+F4     → REMAP 0 5 197 4
```

### Browser
```
Ctrl+T        → REMAP 0 1 84 1    (New tab)
Ctrl+W        → REMAP 0 2 87 1    (Close tab)
Ctrl+Shift+T  → REMAP 0 3 84 3    (Reopen tab)
Ctrl+L        → REMAP 0 4 76 1    (Address bar)
F5            → REMAP 0 5 198 0   (Refresh)
```

### Windows Shortcuts
```
Win+D      → REMAP 0 1 68 8    (Show desktop)
Win+E      → REMAP 0 2 69 8    (File Explorer)
Win+L      → REMAP 0 3 76 8    (Lock)
Alt+Tab    → REMAP 0 4 179 4   (Switch windows)
Win+Left   → REMAP 0 5 216 8   (Snap left)
Win+Right  → REMAP 0 6 215 8   (Snap right)
```

### Screenshot
```
Print Screen        → REMAP 0 1 206 0
Win+Shift+S         → REMAP 0 2 83 10   (Snipping tool)
```

## Command Format Reminder
```
REMAP <layer> <key> <keycode> <modifiers>

Example: REMAP 0 1 65 1
         │     │ │ │  └─ Modifiers (1=Ctrl)
         │     │ │ └──── Keycode (65='A')
         │     │ └────── Key number (1-8)
         └─────┴──────── Layer (0-3)

Result: Layer 0, Key 1 = Ctrl+A
```

## Layer Commands
```
LAYER <key> <target_layer>

Example: LAYER 8 1
         │     │ └─ Target layer
         └─────┴─── Key number

Result: Key 8 becomes a momentary switch to Layer 1
```

## Quick Test Commands
```
DUMP          ← Show current configuration
RESET         ← Reset to defaults
```

---

Print this out and keep it near your macropad! 📋

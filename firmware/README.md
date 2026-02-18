# Noah's 8-Key Macropad - Arduino Firmware

Professional macropad firmware for Seeeduino XIAO with runtime key remapping.

## Features

**Runtime Remapping** - Change key functions without reflashing  
**4 Layers** - Up to 32 different key functions (8 keys × 4 layers)  
**Persistent Storage** - Configuration saved to flash memory  
**Easy Configuration** - Python tool or serial commands  
**Full Keyboard Support** - Letters, numbers, function keys, media keys, modifiers  
**Layer Switching** - Momentary or toggle layer access  

---

## Software Setup

### 1. Install Arduino IDE

Download from: https://www.arduino.cc/en/software

### 2. Install SAMD Board Support

1. Open Arduino IDE
2. Go to **File → Preferences**
3. Add this URL to **Additional Boards Manager URLs**:
   ```
   https://files.seeedstudio.com/arduino/package_seeeduino_boards_index.json
   ```
4. Go to **Tools → Board → Boards Manager**
5. Search for "Seeeduino SAMD" and install it

### 3. Install Required Libraries

Go to **Sketch → Include Library → Manage Libraries** and install:

- **Keyboard** (should be built-in)
- **FlashStorage** by cmaglie

### 4. Upload the Firmware

1. Open `noahs_macropad.ino` in Arduino IDE
2. Select **Tools → Board → Seeeduino XIAO**
3. Select your COM port under **Tools → Port**
4. Click **Upload** (➜)

### 5. Test It!

Open **Serial Monitor** (Ctrl+Shift+M) at **115200 baud**.  
You should see: `Noah's Macropad Ready!`

---

## Default Keymap

### Layer 0 (Default)
```
┌─────┬─────┬─────┬─────┐
│  1  │  2  │  3  │  4  │
├─────┼─────┼─────┼─────┤
│  5  │  6  │  7  │ L1  │  ← Hold Key 8 for Layer 1
└─────┴─────┴─────┴─────┘
```

### Layer 1 (Hold Key 8)
```
┌─────┬─────┬─────┬─────┐
│ F1  │ F2  │ F3  │ F4  │
├─────┼─────┼─────┼─────┤
│ F5  │ F6  │ F7  │ F8  │
└─────┴─────┴─────┴─────┘
```

Layers 2 and 3 are empty (customizable).

---

## Configuration Methods

### Option 1: Python Configuration Tool (Easiest)

```bash
# Install pyserial
pip install pyserial

# Run the config tool
python config_tool.py
```

The tool provides an interactive menu for:
- Remapping individual keys
- Setting layer switches
- Viewing current configuration
- Quick presets (media controls, shortcuts, etc.)

### Option 2: Serial Commands (Manual)

Open Serial Monitor at 115200 baud and use these commands:

#### View Configuration
```
DUMP
```

#### Remap a Key
```
REMAP <layer> <key> <keycode> <modifiers>
```

**Examples:**
```
REMAP 0 1 65 0        ← Layer 0, Key 1 = 'A', no modifiers
REMAP 0 2 67 1        ← Layer 0, Key 2 = Ctrl+C
REMAP 1 1 127 0       ← Layer 1, Key 1 = Mute
REMAP 0 5 83 5        ← Layer 0, Key 5 = Ctrl+Alt+S
```

**Modifier Values:**
- 0 = No modifiers
- 1 = Ctrl
- 2 = Shift
- 4 = Alt
- 8 = GUI (Windows/Cmd)
- Add values to combine (e.g., 3 = Ctrl+Shift)

#### Set Layer Switch Key
```
LAYER <key> <target_layer>
```

**Example:**
```
LAYER 8 1             ← Key 8 switches to Layer 1
```

#### Reset to Defaults
```
RESET
```

---

## Common Keycodes Reference

### Letters
`A=65, B=66, C=67, ... Z=90`

### Numbers
`0=48, 1=49, 2=50, ... 9=57`

### Function Keys
`F1=194, F2=195, F3=196, ... F12=205`

### Special Keys
- `ENTER=176`
- `ESC=177`
- `BACKSPACE=178`
- `TAB=179`
- `SPACE=32`
- `DELETE=212`

### Navigation
- `UP=218`, `DOWN=217`, `LEFT=216`, `RIGHT=215`
- `HOME=210`, `END=213`
- `PAGEUP=211`, `PAGEDOWN=214`

### Media Keys
- `MUTE=127`
- `VOLUME_UP=128`
- `VOLUME_DOWN=129`
- `PLAY_PAUSE=232`
- `NEXT_TRACK=235`
- `PREV_TRACK=234`

Full reference: https://www.arduino.cc/reference/en/language/functions/usb/keyboard/keyboardmodifiers/

---

## Usage Examples

### Example 1: Media Control Pad

Layer 0 = Numbers, Layer 1 = Media controls

```
REMAP 1 1 127 0      # Key 1 = Mute
REMAP 1 2 129 0      # Key 2 = Volume Down
REMAP 1 3 128 0      # Key 3 = Volume Up
REMAP 1 4 232 0      # Key 4 = Play/Pause
REMAP 1 5 234 0      # Key 5 = Previous Track
REMAP 1 6 235 0      # Key 6 = Next Track
LAYER 8 1            # Key 8 = Layer switch
```

### Example 2: Programming Shortcuts

```
REMAP 0 1 67 1       # Key 1 = Ctrl+C (Copy)
REMAP 0 2 86 1       # Key 2 = Ctrl+V (Paste)
REMAP 0 3 90 1       # Key 3 = Ctrl+Z (Undo)
REMAP 0 4 83 1       # Key 4 = Ctrl+S (Save)
REMAP 0 5 70 1       # Key 5 = Ctrl+F (Find)
REMAP 0 6 82 1       # Key 6 = Ctrl+R (Replace)
```

### Example 3: Photoshop Shortcuts

```
REMAP 0 1 66 0       # Key 1 = B (Brush)
REMAP 0 2 69 0       # Key 2 = E (Eraser)
REMAP 0 3 71 0       # Key 3 = G (Paint Bucket)
REMAP 0 4 90 1       # Key 4 = Ctrl+Z (Undo)
REMAP 0 5 90 9       # Key 5 = Ctrl+Alt+Z (Step Backward)
REMAP 0 6 83 3       # Key 6 = Ctrl+Shift+S (Save As)
```

---

## Troubleshooting

### Macropad Not Detected

1. Check USB cable (some are charge-only)
2. Try a different USB port
3. Install CH340 drivers if needed
4. Check Device Manager (Windows) or `ls /dev/tty*` (Linux)

### Keys Not Working

1. Test switch continuity with multimeter
2. Verify pin connections match the firmware
3. Check serial monitor for key press messages
4. Ensure switches are connected to ground

### Configuration Not Saving

1. Configuration saves automatically after each REMAP command
2. If it doesn't persist after power cycle, the flash might be full
3. Try RESET command, then reconfigure

### Serial Port Permission (Linux)

```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

---

## Advanced Customization

### Modify the Firmware

The Arduino sketch is well-commented. You can:

- Add more layers (change `MAX_LAYERS`)
- Add macro support (sequences of keypresses)
- Add RGB LED support
- Add rotary encoder support
- Implement tap vs hold behaviors

### Create Custom Presets

Edit `config_tool.py` to add your own preset configurations in the `quick_presets()` function.

---

## Credits

**Hardware:** noahshdev/Noah Sheppard  
**Firmware:** Arduino + FlashStorage library  
**License:** MIT  

---

## Support

If you need help:
1. Check the serial monitor for error messages
2. Run `DUMP` to see current configuration
3. Try `RESET` to restore defaults
4. Check pin connections with a multimeter


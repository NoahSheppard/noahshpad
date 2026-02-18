#!/usr/bin/env python3
"""
Noah's Macropad Configuration Tool
Easy GUI for remapping keys on your macropad
"""

import serial
import serial.tools.list_ports
import time

# Common keycodes reference
KEYCODES = {
    # Letters
    'A': 65, 'B': 66, 'C': 67, 'D': 68, 'E': 69, 'F': 70, 'G': 71, 'H': 72,
    'I': 73, 'J': 74, 'K': 75, 'L': 76, 'M': 77, 'N': 78, 'O': 79, 'P': 80,
    'Q': 81, 'R': 82, 'S': 83, 'T': 84, 'U': 85, 'V': 86, 'W': 87, 'X': 88,
    'Y': 89, 'Z': 90,
    
    # Numbers
    '0': 48, '1': 49, '2': 50, '3': 51, '4': 52,
    '5': 53, '6': 54, '7': 55, '8': 56, '9': 57,
    
    # Function keys
    'F1': 194, 'F2': 195, 'F3': 196, 'F4': 197, 'F5': 198, 'F6': 199,
    'F7': 200, 'F8': 201, 'F9': 202, 'F10': 203, 'F11': 204, 'F12': 205,
    
    # Special keys
    'ENTER': 176, 'ESC': 177, 'BACKSPACE': 178, 'TAB': 179,
    'SPACE': 32, 'CAPSLOCK': 193,
    
    # Navigation
    'INSERT': 209, 'DELETE': 212, 'HOME': 210, 'END': 213,
    'PAGEUP': 211, 'PAGEDOWN': 214,
    'RIGHT': 215, 'LEFT': 216, 'DOWN': 217, 'UP': 218,
    
    # Media keys
    'MUTE': 127, 'VOLUME_UP': 128, 'VOLUME_DOWN': 129,
    'PLAY_PAUSE': 232, 'NEXT_TRACK': 235, 'PREV_TRACK': 234,
}

# Modifier bits
MODIFIERS = {
    'CTRL': 0x01,
    'SHIFT': 0x02,
    'ALT': 0x04,
    'GUI': 0x08,  # Windows/Command key
}

class MacropadConfigurator:
    def __init__(self):
        self.ser = None
        
    def find_macropad(self):
        """Auto-detect the macropad serial port"""
        ports = serial.tools.list_ports.comports()
        for port in ports:
            # XIAO typically shows as "USB Serial Device" or similar
            if 'USB' in port.description or 'Seeed' in port.description:
                return port.device
        return None
    
    def connect(self, port=None):
        """Connect to the macropad"""
        if port is None:
            port = self.find_macropad()
            if port is None:
                print("Could not find macropad. Please specify the port manually.")
                return False
        
        try:
            self.ser = serial.Serial(port, 115200, timeout=1)
            time.sleep(2)  # Wait for Arduino to reset
            print(f"Connected to macropad on {port}")
            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False
    
    def send_command(self, cmd):
        """Send a command and read response"""
        if not self.ser:
            print("Not connected!")
            return
        
        self.ser.write((cmd + '\n').encode())
        time.sleep(0.1)
        
        # Read response
        while self.ser.in_waiting:
            print(self.ser.readline().decode().strip())
    
    def reset(self):
        """Reset to default configuration"""
        self.send_command("RESET")
    
    def remap_key(self, layer, key_num, keycode, modifiers=0):
        """Remap a specific key"""
        cmd = f"REMAP {layer} {key_num} {keycode} {modifiers}"
        self.send_command(cmd)
    
    def set_layer_key(self, key_num, target_layer):
        """Set a key to switch layers"""
        cmd = f"LAYER {key_num} {target_layer}"
        self.send_command(cmd)
    
    def dump_config(self):
        """Show current configuration"""
        self.send_command("DUMP")
    
    def interactive_mode(self):
        """Interactive configuration mode"""
        print("\n=== Noah's Macropad Configuration Tool ===\n")
        print("Commands:")
        print("  1. Remap a key")
        print("  2. Set layer switch key")
        print("  3. Show current config")
        print("  4. Reset to defaults")
        print("  5. Quick presets")
        print("  q. Quit\n")
        
        while True:
            choice = input("Choose an option: ").strip()
            
            if choice == '1':
                self.interactive_remap()
            elif choice == '2':
                self.interactive_layer()
            elif choice == '3':
                self.dump_config()
            elif choice == '4':
                confirm = input("Reset to defaults? (y/n): ")
                if confirm.lower() == 'y':
                    self.reset()
            elif choice == '5':
                self.quick_presets()
            elif choice.lower() == 'q':
                break
            else:
                print("Invalid choice")
    
    def interactive_remap(self):
        """Interactive key remapping"""
        print("\n--- Remap a Key ---")
        layer = int(input("Layer (0-3): "))
        key = int(input("Key number (1-8): "))
        
        print("\nEnter key to map (examples: A, F1, ENTER, VOLUME_UP)")
        print("Or enter keycode directly (number)")
        key_input = input("Key: ").strip().upper()
        
        if key_input.isdigit():
            keycode = int(key_input)
        else:
            keycode = KEYCODES.get(key_input)
            if keycode is None:
                print(f"Unknown key: {key_input}")
                return
        
        print("\nModifiers (comma-separated): CTRL, SHIFT, ALT, GUI")
        mod_input = input("Modifiers (or leave blank): ").strip().upper()
        
        modifiers = 0
        if mod_input:
            for mod in mod_input.split(','):
                mod = mod.strip()
                if mod in MODIFIERS:
                    modifiers |= MODIFIERS[mod]
        
        self.remap_key(layer, key, keycode, modifiers)
    
    def interactive_layer(self):
        """Interactive layer key setup"""
        print("\n--- Set Layer Switch Key ---")
        key = int(input("Key number (1-8): "))
        layer = int(input("Target layer (1-3): "))
        self.set_layer_key(key, layer)
    
    def quick_presets(self):
        """Quick preset configurations"""
        print("\n--- Quick Presets ---")
        print("1. Media controls (Layer 0: nums, Layer 1: media)")
        print("2. Programming shortcuts (Ctrl+C, Ctrl+V, etc.)")
        print("3. Custom...")
        
        choice = input("Choose preset: ").strip()
        
        if choice == '1':
            # Media controls on layer 1
            self.remap_key(1, 1, KEYCODES['MUTE'], 0)
            self.remap_key(1, 2, KEYCODES['VOLUME_DOWN'], 0)
            self.remap_key(1, 3, KEYCODES['VOLUME_UP'], 0)
            self.remap_key(1, 4, KEYCODES['PLAY_PAUSE'], 0)
            self.remap_key(1, 5, KEYCODES['PREV_TRACK'], 0)
            self.remap_key(1, 6, KEYCODES['NEXT_TRACK'], 0)
            print("Media controls configured on Layer 1!")
        
        elif choice == '2':
            # Programming shortcuts
            self.remap_key(0, 1, KEYCODES['C'], MODIFIERS['CTRL'])  # Ctrl+C
            self.remap_key(0, 2, KEYCODES['V'], MODIFIERS['CTRL'])  # Ctrl+V
            self.remap_key(0, 3, KEYCODES['Z'], MODIFIERS['CTRL'])  # Ctrl+Z
            self.remap_key(0, 4, KEYCODES['S'], MODIFIERS['CTRL'])  # Ctrl+S
            self.remap_key(0, 5, KEYCODES['F'], MODIFIERS['CTRL'])  # Ctrl+F
            print("Programming shortcuts configured!")


def main():
    config = MacropadConfigurator()
    
    # Try to auto-detect port
    port = config.find_macropad()
    if port:
        print(f"Found macropad on: {port}")
    else:
        port = input("Enter serial port (e.g., COM3 or /dev/ttyACM0): ").strip()
    
    if config.connect(port):
        config.interactive_mode()
    else:
        print("Could not connect to macropad")


if __name__ == "__main__":
    main()

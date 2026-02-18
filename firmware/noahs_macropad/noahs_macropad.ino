/*
 * Noah's 8-Key Macropad Firmware
 * For Seeeduino XIAO (SAMD21)
 * 
 * Features:
 * - 8 Cherry MX switches
 * - Runtime key remapping via serial
 * - EEPROM storage for persistent configuration
 * - Support for media keys, shortcuts, and layers
 * 
 * Pin Configuration:
 * SW1 -> Pin 11 (D10)
 * SW2 -> Pin 10 (D9)
 * SW3 -> Pin 9  (D8)
 * SW4 -> Pin 8  (D7)
 * SW5 -> Pin 4  (D3)
 * SW6 -> Pin 5  (D4)
 * SW7 -> Pin 6  (D5)
 * SW8 -> Pin 7  (D6)
 */

#include <Keyboard.h>
#include <FlashStorage.h>

// Pin definitions
const int NUM_KEYS = 8;
const int keyPins[NUM_KEYS] = {10, 9, 8, 7, 3, 4, 5, 6};  // D10, D9, D8, D7, D3, D4, D5, D6

// Key state tracking
bool keyState[NUM_KEYS] = {false};
bool lastKeyState[NUM_KEYS] = {false};
unsigned long lastDebounceTime[NUM_KEYS] = {0};
const unsigned long debounceDelay = 5;

// Layer support
const int MAX_LAYERS = 4;
int currentLayer = 0;
bool layerHoldActive = false;

// Key configuration structure
struct KeyConfig {
  uint8_t keycode;
  uint8_t modifiers;  // Bitmap: Ctrl=1, Shift=2, Alt=4, GUI=8
  uint8_t keyType;    // 0=normal, 1=layer_momentary, 2=layer_toggle, 3=media
};

// Storage for all keys across all layers
struct MacropadConfig {
  KeyConfig keys[MAX_LAYERS][NUM_KEYS];
  bool initialized;
};

// Flash storage
FlashStorage(configStorage, MacropadConfig);
MacropadConfig config;

// Default keymap - Layer 0: Numbers 1-8
void loadDefaultConfig() {
  config.initialized = true;
  
  // Layer 0: Number keys 1-8
  for (int i = 0; i < NUM_KEYS; i++) {
    config.keys[0][i].keycode = '1' + i;
    config.keys[0][i].modifiers = 0;
    config.keys[0][i].keyType = 0;
  }
  
  // Layer 1: Function keys F1-F8
  for (int i = 0; i < NUM_KEYS; i++) {
    config.keys[1][i].keycode = KEY_F1 + i;
    config.keys[1][i].modifiers = 0;
    config.keys[1][i].keyType = 0;
  }
  
  // Layer 2 & 3: Transparent (pass through to layer 0)
  for (int layer = 2; layer < MAX_LAYERS; layer++) {
    for (int i = 0; i < NUM_KEYS; i++) {
      config.keys[layer][i].keycode = 0;  // 0 = transparent
      config.keys[layer][i].modifiers = 0;
      config.keys[layer][i].keyType = 0;
    }
  }
  
  // Key 8 (index 7) on Layer 0 is momentary layer 1 switch
  config.keys[0][7].keycode = 1;  // Target layer 1
  config.keys[0][7].modifiers = 0;
  config.keys[0][7].keyType = 1;  // Momentary layer switch
}

void setup() {
  // Initialize serial for configuration
  Serial.begin(115200);
  
  // Initialize keyboard
  Keyboard.begin();
  
  // Configure pins
  for (int i = 0; i < NUM_KEYS; i++) {
    pinMode(keyPins[i], INPUT_PULLUP);
  }
  
  // Load configuration from flash
  config = configStorage.read();
  
  // If not initialized, load defaults
  if (!config.initialized) {
    loadDefaultConfig();
    configStorage.write(config);
  }
  
  Serial.println("Noah's Macropad Ready!");
  Serial.println("Commands:");
  Serial.println("  RESET - Reset to default keymap");
  Serial.println("  REMAP <layer> <key> <keycode> <modifiers> - Remap a key");
  Serial.println("  LAYER <keycode> <modifiers> - Set current layer switch key");
  Serial.println("  DUMP - Show current configuration");
  Serial.println();
}

void loop() {
  // Check for serial configuration commands
  if (Serial.available() > 0) {
    handleSerialCommand();
  }
  
  // Scan keys
  for (int i = 0; i < NUM_KEYS; i++) {
    bool reading = (digitalRead(keyPins[i]) == LOW);  // Active low (pullup)
    
    // Debounce
    if (reading != lastKeyState[i]) {
      lastDebounceTime[i] = millis();
    }
    
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading != keyState[i]) {
        keyState[i] = reading;
        
        if (keyState[i]) {
          handleKeyPress(i);
        } else {
          handleKeyRelease(i);
        }
      }
    }
    
    lastKeyState[i] = reading;
  }
}

void handleKeyPress(int keyIndex) {
  KeyConfig key = config.keys[currentLayer][keyIndex];
  
  // Handle transparent keys
  if (key.keycode == 0 && currentLayer > 0) {
    // Fall through to lower layers
    for (int layer = currentLayer - 1; layer >= 0; layer--) {
      key = config.keys[layer][keyIndex];
      if (key.keycode != 0) break;
    }
  }
  
  // Handle layer switches
  if (key.keyType == 1) {  // Momentary layer switch
    currentLayer = key.keycode;
    layerHoldActive = true;
    Serial.print("Layer switched to: ");
    Serial.println(currentLayer);
    return;
  } else if (key.keyType == 2) {  // Toggle layer switch
    currentLayer = (currentLayer == key.keycode) ? 0 : key.keycode;
    Serial.print("Layer toggled to: ");
    Serial.println(currentLayer);
    return;
  }
  
  // Press modifiers
  if (key.modifiers & 0x01) Keyboard.press(KEY_LEFT_CTRL);
  if (key.modifiers & 0x02) Keyboard.press(KEY_LEFT_SHIFT);
  if (key.modifiers & 0x04) Keyboard.press(KEY_LEFT_ALT);
  if (key.modifiers & 0x08) Keyboard.press(KEY_LEFT_GUI);
  
  // Press main key
  if (key.keycode != 0) {
    Keyboard.press(key.keycode);
  }
  
  Serial.print("Key ");
  Serial.print(keyIndex + 1);
  Serial.print(" pressed (Layer ");
  Serial.print(currentLayer);
  Serial.println(")");
}

void handleKeyRelease(int keyIndex) {
  KeyConfig key = config.keys[currentLayer][keyIndex];
  
  // Handle transparent keys
  if (key.keycode == 0 && currentLayer > 0) {
    for (int layer = currentLayer - 1; layer >= 0; layer--) {
      key = config.keys[layer][keyIndex];
      if (key.keycode != 0) break;
    }
  }
  
  // Handle layer switches
  if (key.keyType == 1 && layerHoldActive) {  // Momentary layer switch release
    currentLayer = 0;
    layerHoldActive = false;
    Serial.println("Layer returned to 0");
    return;
  }
  
  // Release all keys
  Keyboard.releaseAll();
  
  Serial.print("Key ");
  Serial.print(keyIndex + 1);
  Serial.println(" released");
}

void handleSerialCommand() {
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toUpperCase();
  
  if (cmd == "RESET") {
    loadDefaultConfig();
    configStorage.write(config);
    Serial.println("Configuration reset to defaults");
  }
  else if (cmd.startsWith("REMAP")) {
    // Format: REMAP <layer> <key> <keycode> <modifiers>
    // Example: REMAP 0 1 65 0  (Layer 0, Key 1, 'A', no modifiers)
    int layer, key, keycode, modifiers;
    if (sscanf(cmd.c_str(), "REMAP %d %d %d %d", &layer, &key, &keycode, &modifiers) == 4) {
      if (layer >= 0 && layer < MAX_LAYERS && key >= 1 && key <= NUM_KEYS) {
        config.keys[layer][key - 1].keycode = keycode;
        config.keys[layer][key - 1].modifiers = modifiers;
        config.keys[layer][key - 1].keyType = 0;
        configStorage.write(config);
        Serial.print("Remapped Layer ");
        Serial.print(layer);
        Serial.print(" Key ");
        Serial.print(key);
        Serial.print(" to keycode ");
        Serial.println(keycode);
      } else {
        Serial.println("Invalid parameters");
      }
    } else {
      Serial.println("Usage: REMAP <layer> <key> <keycode> <modifiers>");
    }
  }
  else if (cmd.startsWith("LAYER")) {
    // Format: LAYER <key> <target_layer>
    // Example: LAYER 8 1  (Key 8 switches to layer 1)
    int key, targetLayer;
    if (sscanf(cmd.c_str(), "LAYER %d %d", &key, &targetLayer) == 2) {
      if (key >= 1 && key <= NUM_KEYS && targetLayer >= 0 && targetLayer < MAX_LAYERS) {
        config.keys[0][key - 1].keycode = targetLayer;
        config.keys[0][key - 1].modifiers = 0;
        config.keys[0][key - 1].keyType = 1;  // Momentary
        configStorage.write(config);
        Serial.print("Key ");
        Serial.print(key);
        Serial.print(" set as layer switch to ");
        Serial.println(targetLayer);
      } else {
        Serial.println("Invalid parameters");
      }
    } else {
      Serial.println("Usage: LAYER <key> <target_layer>");
    }
  }
  else if (cmd == "DUMP") {
    Serial.println("\n=== Current Configuration ===");
    for (int layer = 0; layer < MAX_LAYERS; layer++) {
      Serial.print("\nLayer ");
      Serial.print(layer);
      Serial.println(":");
      for (int i = 0; i < NUM_KEYS; i++) {
        Serial.print("  Key ");
        Serial.print(i + 1);
        Serial.print(": ");
        if (config.keys[layer][i].keyType == 1) {
          Serial.print("LAYER_SWITCH(");
          Serial.print(config.keys[layer][i].keycode);
          Serial.println(")");
        } else if (config.keys[layer][i].keycode == 0) {
          Serial.println("TRANSPARENT");
        } else {
          Serial.print("Keycode=");
          Serial.print(config.keys[layer][i].keycode);
          Serial.print(" Mods=");
          Serial.println(config.keys[layer][i].modifiers);
        }
      }
    }
    Serial.println("=========================\n");
  }
  else {
    Serial.println("Unknown command");
  }
}

# ESPHome PN5180 Enhanced Component

**Industrial-grade PN5180 NFC controller with RF power tuning, LPCD, and comprehensive diagnostics.**

Based on John McLear's PN5180 component with major enhancements for stability and RF field strength.

## New Features (vs Standard PN5180)

### 🔋 RF Power Configuration
- **RF Power Level Tuning** (0-255) — Maximize field strength for long-range reading
- **Dynamic Power Control (DPC)** — Auto-adjusts RF power under detuned antenna conditions  
- **Protocol Priority** — Optimize for ISO15693 vicinity cards (20cm+ range) vs ISO14443 proximity

### 📡 Low-Power Card Detection (LPCD)
- **LPCD Mode** — Dramatically reduces power consumption when idle
- **Configurable Intervals** — Check for cards every 100ms without full RF field
- **Perfect for Battery-Powered** access control systems

### 📊 RF Diagnostics & Monitoring
- **AGC (Automatic Gain Control) Sensor** — Real-time RF field quality (10-4000 range)
- **RF Field Strength Sensor** — Monitor field power output
- **Temperature Sensor** — Track module temperature with thermal protection
- **Antenna Tuning Aid** — Use diagnostics to optimize antenna matching

### 🔥 Thermal Protection
- **Automatic Throttling** — Reduces RF power if temperature > 70°C
- **Auto-Recovery** — Restores full power when temperature < 60°C
- **Prevents Overheating** in enclosed installations

### 🏥 Enhanced Health Check
- **RF Config Verification** — Validates RF registers in addition to firmware check
- **AGC Range Validation** — Detects antenna/RF problems (AGC out of 10-4000 range)
- **Auto-Recovery** — Hardware reset via RST pin on failure

---

## Why PN5180 for Industrial Use?

| Feature | PN5180 | PN7160 | PN532 |
|---------|--------|--------|-------|
| **Max RF Power** | **250mA pulses** | Standard (mobile) | Standard |
| **Read Range** | **Best (ISO15693)** | Standard | Short |
| **Power Architecture** | **Industrial (80µF)** | Mobile | Standard |
| **Thermal Stability** | **Excellent** | Standard | Standard |
| **Auto RF Tuning** | **DPC+AWC+ARC** | Basic | None |
| **Card Emulation** | ❌ | ✅ | ❌ |
| **Target Market** | **Industrial/Access** | Mobile/Payment | Hobby |

**Choose PN5180 when:** Stability + RF range > card emulation (Google/Apple Pay)

---

## Installation

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/YOUR_USERNAME/esphome-pn5180-enhanced
    components: [pn5180]
    refresh: 1d
```

---

## Quick Start (Maximum RF Power)

```yaml
spi:
  clk_pin: GPIO18
  miso_pin: GPIO19
  mosi_pin: GPIO23

pn5180:
  id: nfc
  cs_pin: GPIO5
  rst_pin: GPIO17
  busy_pin: GPIO16
  
  # NEW: Maximize RF power for long range
  rf_power_level: 255  # 0-255, default 200
  rf_protocol_priority: ISO15693  # Optimize for vicinity cards
  
  on_tag:
    - homeassistant.tag_scanned: !lambda 'return x;'

binary_sensor:
  - platform: pn5180
    name: "Access Badge"
    uid: "E0-04-01-50-4D-8A-3C-5F"
```

---

## Configuration Variables

### Core (Required)
- **`cs_pin`** (**Required**): SPI chip select
- **`busy_pin`** (**Required**): BUSY signal from PN5180
- **`rst_pin`** (**Required**): Hardware reset (active LOW)
- **`update_interval`** (*Optional*, default `1s`): Tag scan frequency

### NEW: RF Power Configuration

- **`rf_power_level`** (*Optional*, default `200`): RF field strength, 0-255
  - `200` = Safe high power (default)
  - `255` = Maximum power for longest range
  - `150` = Reduced power for close-range/thermal concerns

- **`rf_collision_avoidance`** (*Optional*, default `true`): Enable Dynamic Power Control
  - Automatically adjusts RF power if antenna is detuned
  - Recommended: keep enabled

- **`rf_protocol_priority`** (*Optional*, default `AUTO`): Optimize for card type
  - `ISO15693` = Vicinity cards, longest range (20cm+)
  - `ISO14443A` = Proximity cards (MIFARE, etc.)
  - `ISO14443B` = Proximity cards (alternate)
  - `FELICA` = FeliCa cards
  - `AUTO` = Auto-detect (default)

### NEW: LPCD (Low-Power Card Detection)

- **`lpcd_enabled`** (*Optional*, default `false`): Enable low-power mode when idle
- **`lpcd_interval`** (*Optional*, default `100ms`): Check frequency in LPCD mode
  - Reduces power consumption by ~90% when no card present
  - Perfect for battery-powered applications

### NEW: RF Diagnostics

- **`publish_diagnostics`** (*Optional*, default `false`): Enable diagnostic sensors

- **`agc_sensor`** (*Optional*): AGC (gain control) value sensor
  - Range: 10-4000
  - Higher = better RF field quality
  - Use to tune antenna matching

- **`rf_field_sensor`** (*Optional*): RF field strength indicator

- **`temperature_sensor`** (*Optional*): Module temperature
  - Automatic thermal protection at 70°C
  - Auto-recovery at 60°C

### Health Check (Enhanced)

- **`health_check_enabled`** (*Optional*, default `true`): Periodic health checks
- **`health_check_interval`** (*Optional*, default `60s`): Check frequency
- **`max_failed_checks`** (*Optional*, default `3`): Failures before declaring unhealthy
- **`auto_reset_on_failure`** (*Optional*, default `true`): Auto-reset via RST pin

### Triggers

- **`on_tag`**: Fires when tag detected (variable `x` = UID string)
- **`on_tag_removed`**: Fires when tag removed (variable `x` = UID string)

---

## Full-Featured Example

See [`example-full-features.yaml`](example-full-features.yaml) for a complete configuration showing:
- Maximum RF power for long range
- LPCD for battery savings
- All diagnostic sensors
- Thermal protection monitoring
- Enhanced health checks

---

## Use Cases & Recommendations

### Long-Range Access Control (20cm+)
```yaml
pn5180:
  rf_power_level: 255
  rf_protocol_priority: ISO15693
  lpcd_enabled: true
```

### Battery-Powered Installation
```yaml
pn5180:
  rf_power_level: 200
  lpcd_enabled: true
  lpcd_interval: 100ms
```

### Enclosed/Hot Environment
```yaml
pn5180:
  rf_power_level: 200  # Start moderate
  temperature_sensor:  # Enable thermal protection
    name: "NFC Temp"
```

### Antenna Tuning/Optimization
```yaml
pn5180:
  publish_diagnostics: true
  agc_sensor:
    name: "NFC AGC"  # Optimize antenna for AGC 2000-3000
```

---

## RF Diagnostics Guide

### AGC (Automatic Gain Control)

**Normal Range**: 2000-3000  
**Low (<1000)**: Antenna detuned or card too far  
**High (>3500)**: Excellent coupling, card very close  
**Out of Range (<10 or >4000)**: RF configuration problem

**Use Case**: Tune your antenna impedance matching to achieve AGC 2000-3000 for optimal performance.

### Temperature Monitoring

**Normal**: 25-50°C  
**Warning**: 50-70°C (consider ventilation)  
**Throttle**: >70°C (automatic power reduction)  
**Critical**: >80°C (check for hardware problems)

---

## Thermal Protection

The component automatically monitors temperature (if sensor configured) and:
1. At **70°C**: Reduces `rf_power_level` from 200→150
2. At **60°C**: Restores `rf_power_level` to 200
3. Logs thermal events

**Recommendation**: Mount PN5180 with good airflow if using `rf_power_level: 255` continuously.

---

## Troubleshooting

### Short Read Range

1. **Increase RF power**: `rf_power_level: 255`
2. **Check protocol**: Use `rf_protocol_priority: ISO15693` for vicinity cards
3. **Verify AGC**: Should be 2000-3000. If low, check antenna connections
4. **Check temperature**: Thermal throttling active? Improve cooling

### High Power Consumption

1. **Enable LPCD**: `lpcd_enabled: true` reduces idle consumption by ~90%
2. **Reduce power**: `rf_power_level: 150` if range allows
3. **Increase scan interval**: `update_interval: 2s` or higher

### Cards Not Detected

1. **Check protocol**: ISO15693 cards need `rf_protocol_priority: ISO15693`
2. **Verify AGC**: `agc_sensor` should read 10-4000 when scanning
3. **Check health**: Is `auto_reset_on_failure` recovering the module?

### Module Overheating

1. **Improve ventilation** or add heatsink
2. **Reduce power**: `rf_power_level: 200` or `180`
3. **Monitor temperature**: Add `temperature_sensor` for automatic protection

---

## Hardware Notes

### Wiring (ESP32)

```
PN5180 → ESP32
VCC    → 3.3V
GND    → GND
NSS    → GPIO5  (cs_pin)
MOSI   → GPIO23 (SPI MOSI)
MISO   → GPIO19 (SPI MISO)
SCK    → GPIO18 (SPI CLK)
BUSY   → GPIO16 (busy_pin)
RST    → GPIO17 (rst_pin)
```

**Power**: PN5180 requires **5V supply** for RF antenna (separate from 3.3V logic).

### Antenna Tuning

For maximum range:
1. Enable `agc_sensor`
2. Present card at desired distance
3. Adjust antenna matching components
4. Target AGC value: 2000-3000

---

## Compatibility

- ESP32 (recommended)
- ESP8266 (works, but ESP32 preferred for SPI performance)
- PN5180 NFC Frontend boards
- ISO14443A/B cards (MIFARE, etc.)
- ISO15693 cards (ICODE, vicinity tags)
- FeliCa cards

---

## Contributing

Contributions welcome! This component builds on:
- John McLear's original PN5180 component
- NXP PN5180 datasheet specifications
- Andreas Trappmann's PN5180 Arduino library

---

## License

MIT (same as ESPHome)

---

## Comparison with Other Components

### vs Standard PN5180 Component
✅ All features of standard component  
➕ RF power tuning (up to 255)  
➕ LPCD for power savings  
➕ RF diagnostics sensors  
➕ Thermal protection  
➕ Enhanced health check  

### vs PN7160 Enhanced
✅ **Higher RF power** (industrial vs mobile)  
✅ **Longer range** (ISO15693 vicinity)  
✅ **Better stability** (industrial power architecture)  
✅ **Simpler** (SPI only, no IRQ/VEN complexity)  
❌ No card emulation (Google/Apple Pay)  

### vs PN532 Enhanced
✅ **Much higher RF power** (250mA vs standard)  
✅ **Longer range** (ISO15693 support)  
✅ **Better stability** (modern IC, industrial design)  
✅ **Auto RF tuning** (DPC, AWC, ARC)  
➕ All the same enhancements (health check, diagnostics)  

---

**Choose PN5180 Enhanced when:** You need maximum RF range and stability, and don't need card emulation.

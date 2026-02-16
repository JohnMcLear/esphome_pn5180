# esphome_pn5180

PN5180 NFC/RFID reader component for ESPHome

Exposes scanned tag UIDs to Home Assistant with automation triggers.

## Features

✅ ISO15693 tag reading  
✅ `on_tag` automation triggers  
✅ Configurable scan frequency  
✅ Automatic validation and warnings  
✅ SPI-based communication  

## Usage

### Minimal Configuration

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/johnmclear/esphome_pn5180
      ref: claude

esphome:
  libraries:
    - SPI
    - PN5180_LIBRARY=https://github.com/ATrappmann/PN5180-Library.git#master

esp32:
  board: esp32dev
  framework:
    type: arduino

logger:
api:
ota:
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

spi:
  clk_pin: GPIO18
  miso_pin: GPIO19
  mosi_pin: GPIO23

pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
```

### Full Configuration with All Options

```yaml
spi:
  clk_pin: GPIO18
  miso_pin: GPIO19
  mosi_pin: GPIO23

pn5180:
  cs_pin: GPIO16          # SPI chip select
  busy_pin: GPIO5         # PN5180 busy signal
  rst_pin: GPIO17         # PN5180 reset pin
  update_interval: 500ms  # Scan frequency (200ms-10s, default: 500ms)
  
  on_tag:
    then:
      - logger.log:
          format: "Tag detected: %s"
          args: ['tag_id.c_str()']
      - homeassistant.event:
          event: esphome.nfc_tag_scanned
          data:
            tag_id: !lambda 'return tag_id;'
```

## Configuration Options

| Option | Type | Required | Default | Description |
|--------|------|----------|---------|-------------|
| `cs_pin` | GPIO | Yes | - | SPI chip select pin |
| `busy_pin` | GPIO | Yes | - | PN5180 busy signal pin |
| `rst_pin` | GPIO | Yes | - | PN5180 reset pin |
| `update_interval` | Time | No | `500ms` | How often to scan for tags |
| `on_tag` | Automation | No | - | Actions to perform when tag detected |

## Update Interval Guidelines

The `update_interval` parameter controls how often the PN5180 scans for tags.

### Validation Rules

- **Minimum**: 200ms (hard limit)
- **Recommended minimum**: 250ms (warning if below)
- **Default**: 500ms (recommended)
- **Maximum**: 10s (hard limit)

### Recommended Settings by Use Case

| Use Case | Recommended Interval | Notes |
|----------|---------------------|-------|
| Door Access Control | `200ms` - `300ms` | Fast response needed |
| Attendance Tracking | `500ms` (default) | Good balance |
| Inventory/Asset Tracking | `1s` | Power efficient |
| Passive Monitoring | `5s` | Very low power |

### Performance Considerations

⚠️ **Intervals below 250ms** may cause:
- Increased CPU usage
- Interference with other operations (LED, RTTTL, etc)
- Potential timing jitter

✅ **500ms (default)** provides:
- Responsive tag detection
- Stable performance
- No interference with other operations

### Examples

**Fast scanning (with warning):**
```yaml
pn5180:
  update_interval: 200ms  # Shows warning but works
```

**Recommended default:**
```yaml
pn5180:
  update_interval: 500ms  # Or omit for default
```

**Conservative/power-saving:**
```yaml
pn5180:
  update_interval: 1s
```

## Wiring

### Standard ESP32 Connections

| PN5180 Pin | ESP32 Pin | Notes |
|------------|-----------|-------|
| NSS/CS | GPIO16 | Configurable via `cs_pin` |
| MOSI | GPIO23 | Hardware SPI (fixed) |
| MISO | GPIO19 | Hardware SPI (fixed) |
| SCK | GPIO18 | Hardware SPI (fixed) |
| BUSY | GPIO5 | Configurable via `busy_pin`* |
| RST | GPIO17 | Configurable via `rst_pin` |
| GND | GND | Ground |
| 3.3V | 3.3V | Logic power |
| 5V | 5V | RF antenna power (required!) |

*Note: GPIO5 is a strapping pin on ESP32. You'll get a warning, but it works. Consider using GPIO4 or GPIO21 instead if you prefer.

## Automation Examples

### Basic Logging

```yaml
pn5180:
  on_tag:
    then:
      - logger.log:
          format: "Tag scanned: %s"
          args: ['tag_id.c_str()']
```

### Send to Home Assistant

```yaml
pn5180:
  on_tag:
    then:
      - homeassistant.event:
          event: esphome.nfc_tag_scanned
          data:
            tag_id: !lambda 'return tag_id;'
            location: "front_door"
```

### Visual Feedback

```yaml
pn5180:
  on_tag:
    then:
      - light.turn_on:
          id: status_led
          flash_length: 500ms
      - rtttl.play: "success:d=4,o=5,b=100:16e6"
```

### Conditional Actions (Door Access)

```yaml
pn5180:
  on_tag:
    then:
      - if:
          condition:
            lambda: 'return tag_id == "E0 07 01 23 45 67 89 AB";'
          then:
            - logger.log: "Access granted!"
            - homeassistant.service:
                service: lock.unlock
                data:
                  entity_id: lock.front_door
          else:
            - logger.log: "Access denied!"
            - rtttl.play: "error:d=4,o=5,b=100:8e5"
```

## Troubleshooting

### Tags Not Detected

1. **Check power**: Both 3.3V AND 5V must be connected
2. **Verify wiring**: Especially SPI pins (MOSI, MISO, SCK)
3. **Tag compatibility**: PN5180 works best with ISO15693 tags
4. **Distance**: Tags should be within ~10-20cm of antenna
5. **Update interval**: Try increasing to 1s for testing

### Performance Issues

1. **Slow response**: Decrease `update_interval` (but ≥250ms recommended)
2. **High CPU usage**: Increase `update_interval` to 1s or more
3. **Jittery LED/sound**: Increase `update_interval` to reduce interference

### Compilation Errors

1. **"no CONFIG_SCHEMA"**: Make sure you're using the `claude` branch
2. **SPI errors**: Ensure `spi:` section is configured before `pn5180:`
3. **Library not found**: Check `PN5180_LIBRARY` URL is correct

## Supported Tags

✅ **ISO15693** (Primary support)
- ICODE SLIX
- ICODE SLIX2
- Most ISO15693-compatible tags

❓ **ISO14443A** (May work, depends on library version)
- MIFARE Classic
- NTAG

❌ **Not Supported**
- Low frequency (125kHz) tags
- UHF RFID tags

## TODO

- [ ] Create a test method that periodically (lets say every 60 seconds but should be adjustable through esphome YAML) sends a command to the pn5180 to check its availability and functionality - I imagine the NXP PN5180 spec docs explain what command to send and expect to recieve.  If this fails then allow esphome to know NFC has failed.  Perhaps from this send a RST command to reboot the PN5180.
- [x] Ensure update_interval is a usable and working yaml config change as per pn532 docs
- [ ] Consider different librarys as ATrappmann/PN5180-Library.git seems neglected.  https://github.com/mjmeans/PN5180-Library and https://github.com/playfultechnology/PN5180 for example
- [ ] Check if RF field can be adjusted through ESPHome

## Contributing

Contributions welcome! Please test thoroughly with real hardware before submitting PRs.

## License

Apache 2.0

## Credits

Based on the [PN5180-Library](https://github.com/ATrappmann/PN5180-Library) by Andreas Trappmann.

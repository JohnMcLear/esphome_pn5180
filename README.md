# esphome_pn5180

PN5180 NFC/RFID reader component for ESPHome. Supports ISO15693 tag reading with full Home Assistant integration.

## Features

- `on_tag` automation trigger when a tag is detected
- `on_tag_removed` automation trigger when a tag is removed
- Native `homeassistant.tag_scanned` support
- Binary sensors for tracking specific known tags
- Text sensor for last scanned tag UID
- Periodic hardware health checks with auto-reset
- Configurable scan frequency with validation

## Wiring

| PN5180 Pin | ESP32 Pin | Notes |
|------------|-----------|-------|
| NSS / CS   | GPIO16    | Configurable via `cs_pin` |
| MOSI       | GPIO23    | Hardware SPI |
| MISO       | GPIO19    | Hardware SPI |
| SCK        | GPIO18    | Hardware SPI |
| BUSY       | GPIO5     | Configurable via `busy_pin`* |
| RST        | GPIO17    | Configurable via `rst_pin` |
| GND        | GND       | |
| 3.3V       | 3.3V      | Logic power |
| 5V         | 5V        | RF power — required! |

*GPIO5 is a strapping pin on ESP32. It works fine but ESPHome will warn about it. Use GPIO4 or GPIO21 if you want to avoid the warning.

---

## Minimal Example

```yaml
esphome:
  name: nfc-reader
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

external_components:
  - source:
      type: git
      url: https://github.com/johnmclear/esphome_pn5180
      ref: claude
    refresh: 0s

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

---

## Full Example

```yaml
esphome:
  name: nfc-reader
  libraries:
    - SPI
    - PN5180_LIBRARY=https://github.com/ATrappmann/PN5180-Library.git#master

esp32:
  board: esp32dev
  framework:
    type: arduino

logger:
  level: DEBUG

api:
ota:

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:

external_components:
  - source:
      type: git
      url: https://github.com/johnmclear/esphome_pn5180
      ref: claude
    refresh: 0s

spi:
  clk_pin: GPIO18
  miso_pin: GPIO19
  mosi_pin: GPIO23

# ── Main PN5180 component ──────────────────────────────────────────────────────
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  update_interval: 500ms

  # Health monitoring
  health_check_enabled: true
  health_check_interval: 60s
  max_failed_checks: 3
  auto_reset_on_failure: true

  # Fires when a new tag is placed on the reader
  on_tag:
    then:
      - logger.log:
          format: "Tag detected: %s"
          args: ['tag_id.c_str()']
      - homeassistant.tag_scanned: !lambda 'return tag_id;'

  # Fires when the tag is removed
  on_tag_removed:
    then:
      - logger.log:
          format: "Tag removed: %s"
          args: ['tag_id.c_str()']

# ── Binary sensors for known tags ──────────────────────────────────────────────
binary_sensor:
  - platform: pn5180
    name: "John's Badge"
    uid: "E0 07 01 23 45 67 89 AB"

  - platform: pn5180
    name: "Jane's Badge"
    uid: "A0 B0 C0 D0 E0 F0 00 11"

# ── Text sensor: last scanned UID ──────────────────────────────────────────────
text_sensor:
  - platform: pn5180
    name: "Last NFC Tag"
```

---

## Configuration Reference

### `pn5180:` options

| Option | Type | Required | Default | Description |
|--------|------|----------|---------|-------------|
| `cs_pin` | GPIO | **Yes** | — | SPI chip select |
| `busy_pin` | GPIO | **Yes** | — | PN5180 busy signal |
| `rst_pin` | GPIO | **Yes** | — | PN5180 reset |
| `update_interval` | Time | No | `500ms` | How often to scan (200ms – 10s) |
| `health_check_enabled` | bool | No | `true` | Enable hardware health monitoring |
| `health_check_interval` | Time | No | `60s` | How often to run health check |
| `max_failed_checks` | int | No | `3` | Consecutive failures before declaring unhealthy |
| `auto_reset_on_failure` | bool | No | `true` | Automatically reset PN5180 on failure |
| `on_tag` | Automation | No | — | Actions to run when tag detected |
| `on_tag_removed` | Automation | No | — | Actions to run when tag removed |

### `binary_sensor:` options (platform: pn5180)

| Option | Type | Required | Description |
|--------|------|----------|-------------|
| `name` | string | **Yes** | Sensor name in Home Assistant |
| `uid` | string | **Yes** | Tag UID to track (space-separated hex, e.g. `"E0 07 01 23 45 67 89 AB"`) |

### `text_sensor:` options (platform: pn5180)

| Option | Type | Required | Description |
|--------|------|----------|-------------|
| `name` | string | **Yes** | Sensor name in Home Assistant |

---

## update_interval

Controls how often the PN5180 scans for tags.

| Setting | Value | Notes |
|---------|-------|-------|
| Minimum | `200ms` | Hard limit — error if below |
| Recommended min | `250ms` | Soft limit — warning if below |
| **Default** | **`500ms`** | Recommended for most use cases |
| Maximum | `10s` | Hard limit — error if above |

Scanning below 250ms can interfere with other operations (LED effects, RTTTL, WiFi). 500ms is the right choice for most applications.

### Recommended by use case

| Use Case | Interval |
|----------|----------|
| Door access control | `200ms` – `300ms` |
| Attendance / presence | `500ms` (default) |
| Asset / inventory tracking | `1s` |
| Battery-powered | `5s` |

---

## Health Check

The component periodically verifies the PN5180 hardware is still responsive. If checks fail, it logs warnings, marks the reader as unhealthy, and optionally performs an automatic reset.

```yaml
pn5180:
  health_check_enabled: true    # Default: true
  health_check_interval: 60s    # Default: 60s
  max_failed_checks: 3          # Default: 3 — consecutive failures before declaring unhealthy
  auto_reset_on_failure: true   # Default: true — reset PN5180 on failure threshold
```

### Behaviour

1. Every `health_check_interval` seconds, the component checks the BUSY pin state
2. If the check fails, `consecutive_failures` is incremented
3. Once `max_failed_checks` is reached, the reader is declared unhealthy and tag scanning pauses
4. If `auto_reset_on_failure` is true, a reset is attempted automatically
5. On successful reset, the reader returns to healthy state and scanning resumes

### Log output

```
[I][pn5180]: Initial health check passed
[V][pn5180]: Health check passed
[W][pn5180]: Health check failed (1/3)
[E][pn5180]: PN5180 declared unhealthy
[W][pn5180]: Attempting automatic reset...
[I][pn5180]: Reset successful
[I][pn5180]: PN5180 health restored
```

### Tuning examples

**Aggressive monitoring** — detect failures quickly:
```yaml
health_check_interval: 30s
max_failed_checks: 2
```

**Conservative monitoring** — tolerate temporary glitches:
```yaml
health_check_interval: 120s
max_failed_checks: 5
```

**Disable entirely** — for testing or low-overhead setups:
```yaml
health_check_enabled: false
```

---

## Automation Examples

### Log and scan to Home Assistant

```yaml
pn5180:
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
      - homeassistant.tag_scanned: !lambda 'return tag_id;'
```

### Visual and audio feedback

```yaml
pn5180:
  on_tag:
    then:
      - light.turn_on:
          id: status_led
          flash_length: 500ms
      - rtttl.play: "success:d=4,o=5,b=100:16e6"
  on_tag_removed:
    then:
      - light.turn_off:
          id: status_led
```

### Conditional access control

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
            - rtttl.play: "fail:d=4,o=5,b=100:8e5"
```

### Send event to Home Assistant

```yaml
pn5180:
  on_tag:
    then:
      - homeassistant.event:
          event: esphome.nfc_tag_scanned
          data:
            tag_id: !lambda 'return tag_id;'
            location: "front_door"
  on_tag_removed:
    then:
      - homeassistant.event:
          event: esphome.nfc_tag_removed
          data:
            tag_id: !lambda 'return tag_id;'
            location: "front_door"
```

### Track specific badges with binary sensors

```yaml
binary_sensor:
  - platform: pn5180
    name: "John's Badge"
    uid: "E0 07 01 23 45 67 89 AB"

  - platform: pn5180
    name: "Jane's Badge"
    uid: "A0 B0 C0 D0 E0 F0 00 11"
```

Each sensor turns `ON` when that tag is on the reader and `OFF` when removed. Use these in Home Assistant automations just like any binary sensor.

### Last scanned tag text sensor

```yaml
text_sensor:
  - platform: pn5180
    name: "Last NFC Tag"
```

Publishes the UID of the most recently scanned tag to Home Assistant. Useful for dashboards and logging.

---

## Home Assistant Integration

### Using the Tags system

```yaml
pn5180:
  on_tag:
    then:
      - homeassistant.tag_scanned: !lambda 'return tag_id;'
```

This integrates with Home Assistant's built-in tag system under **Settings → Tags**. You can assign friendly names to tags and create automations directly from there.

### Automation example (Home Assistant side)

```yaml
automation:
  - alias: "Front door NFC - grant access"
    trigger:
      - platform: event
        event_type: esphome.nfc_tag_scanned
        event_data:
          location: "front_door"
          tag_id: "E0 07 01 23 45 67 89 AB"
    action:
      - service: lock.unlock
        target:
          entity_id: lock.front_door

  - alias: "Unknown tag alert"
    trigger:
      - platform: event
        event_type: esphome.nfc_tag_scanned
    condition:
      - condition: template
        value_template: >
          {{ trigger.event.data.tag_id not in [
            'E0 07 01 23 45 67 89 AB',
            'A0 B0 C0 D0 E0 F0 00 11'
          ] }}
    action:
      - service: notify.mobile_app
        data:
          message: "Unknown tag scanned: {{ trigger.event.data.tag_id }}"
```

---

## Supported Tags

| Protocol | Support | Notes |
|----------|---------|-------|
| ISO15693 | ✅ Full | Primary protocol |
| ISO14443A | ❓ Partial | Depends on library version |
| 125kHz (EM4100, HID) | ❌ No | Different hardware required |
| UHF RFID | ❌ No | Different hardware required |

Common ISO15693 tags that work: ICODE SLIX, ICODE SLIX2, TI Tag-it, ST LRI series.

---

## Troubleshooting

### Tags not detected

- Verify **both 3.3V and 5V are connected** — the antenna needs 5V
- Check SPI wiring (MOSI, MISO, SCK)
- Ensure the tag is within 10–20cm of the antenna
- Try increasing `update_interval` to `1s` while debugging
- Enable `DEBUG` logging to see raw output

### Performance issues or LED jitter

- Increase `update_interval` to `500ms` or higher
- Reduce WiFi scan frequency if applicable
- Check for other blocking operations in your config

### "Component pn5180 cannot be loaded via YAML (no CONFIG_SCHEMA)"

- Ensure you have `ref: claude` in your `external_components` source
- Run `esphome clean <config>.yaml` to clear the component cache

### Compilation errors

- Ensure the `spi:` section is present **before** `pn5180:` in your YAML
- Confirm both SPI and PN5180 libraries are listed under `esphome.libraries`

---

## TODO

- [ ] Periodic hardware health check using NXP PN5180 spec commands (BUSY pin check currently used as proxy)
- [x] `update_interval` validated and working as per PN532 docs
- [ ] Evaluate alternative libraries — [tueddy/PN5180-Library](https://github.com/tueddy/PN5180-Library) and [mjmeans/PN5180-Library](https://github.com/mjmeans/PN5180-Library) are candidates

---

## Credits

Based on the [ATrappmann/PN5180-Library](https://github.com/ATrappmann/PN5180-Library) Arduino library.

## License

Apache 2.0

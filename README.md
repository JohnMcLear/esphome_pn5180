# ESPHome PN5180 Component

An ESPHome external component for the PN5180 NFC/RFID reader from NXP Semiconductors.

## Features

- ✅ ESP-IDF framework support (ESP32-C6 and other ESP32 variants)
- ✅ ISO14443A (Type A) card detection
- ✅ Automatic tag presence management with configurable TTL
- ✅ `on_tag` and `on_tag_removed` automation triggers
- ✅ SPI communication
- ✅ Hardware reset and BUSY pin support

## Hardware Requirements

- PN5180 NFC Module
- ESP32 (tested on ESP32-C6)
- SPI connection with CS, BUSY, and RST pins

## Installation

Add the external component to your ESPHome configuration:

```yaml
external_components:
  - source:
      type: local
      path: esphome_pn5180/components
    components: [pn5180]
    refresh: 0s
```

Or from GitHub (once published):

```yaml
external_components:
  - source: github://JohnMcLear/esphome_pn5180
    components: [pn5180]
    refresh: 1d
```

## Configuration

### Basic Configuration

```yaml
spi:
  clk_pin: GPIO20
  miso_pin: GPIO23
  mosi_pin: GPIO15

pn5180:
  cs_pin: GPIO22
  busy_pin: GPIO21
  rst_pin: GPIO18
  id: my_pn5180
  on_tag:
    then:
      - logger.log:
          format: "Tag detected: %s"
          args: ['x.c_str()']
  on_tag_removed:
    then:
      - logger.log:
          format: "Tag removed: %s"
          args: ['x.c_str()']
```

### Configuration Variables

- **cs_pin** (**Required**, Pin): The pin connected to the PN5180's NSS (chip select) pin.
- **busy_pin** (**Required**, Pin): The pin connected to the PN5180's BUSY pin.
- **rst_pin** (**Required**, Pin): The pin connected to the PN5180's RST (reset) pin.
- **update_interval** (*Optional*, Time): How often to scan for tags. Defaults to `1s`.
- **tag_ttl** (*Optional*, Time): Time-to-live for tag presence. If a tag is not seen for this duration, it's considered removed. Defaults to `1000ms`.
- **emulation_message** (*Optional*, String): Message for card emulation mode (future feature).
- **on_tag** (*Optional*, Automation): Automation to run when a tag is detected. The tag UID is provided as a string variable `x`.
- **on_tag_removed** (*Optional*, Automation): Automation to run when a tag is removed. The tag UID is provided as a string variable `x`.

### Integration with Home Assistant

#### Send Tag Events to Home Assistant

```yaml
pn5180:
  cs_pin: GPIO22
  busy_pin: GPIO21
  rst_pin: GPIO18
  on_tag:
    then:
      - homeassistant.tag_scanned: !lambda 'return x;'
```

#### Control a Lock

```yaml
pn5180:
  cs_pin: GPIO22
  busy_pin: GPIO21
  rst_pin: GPIO18
  on_tag:
    then:
      - if:
          condition:
            lambda: 'return x == "04-A1-B2-C3";'  # Your authorized tag UID
          then:
            - lock.unlock: my_lock
            - logger.log: "Authorized tag, unlocking door"
```

## Supported Card Types

Currently, the component supports:
- ISO14443A (Type A) cards
  - MIFARE Classic
  - MIFARE Ultralight
  - NTAG21x series
  - Many other ISO14443A compatible cards

Future support planned for:
- ISO14443B (Type B) cards
- ISO15693 cards (ICODE SLIX, etc.)
- FeliCa cards

## Tag UID Format

Tag UIDs are returned as hyphen-separated uppercase hexadecimal strings:
- Example: `04-A1-B2-C3` for a 4-byte UID
- Example: `04-A1-B2-C3-D4-E5-F6` for a 7-byte UID

## Troubleshooting

### "Failed to reset PN5180"
- Check wiring, especially RST and BUSY pins
- Ensure proper power supply (3.3V)
- Verify SPI connections

### "Failed to read PN5180 version"
- Check SPI wiring (MOSI, MISO, SCK, CS)
- Verify SPI is configured correctly in ESPHome
- Check for proper voltage levels (3.3V)

### Tags not detected
- Verify RF antenna is connected
- Check antenna tuning (may need capacitor adjustment)
- Ensure tag is close enough (within ~5cm typically)
- Check logs for scan activity

### BUSY pin timeout
- Ensure BUSY pin is connected correctly
- Check for proper pull-up/pull-down on BUSY pin if needed
- May indicate communication issues with the PN5180

## Development

This component is based on:
- NXP PN5180 Datasheet
- ATrappmann PN5180 Arduino Library
- ESPHome PN532 component structure

## References

- [PN5180 Datasheet](https://www.nxp.com/docs/en/data-sheet/PN5180A0XX-C1-C2.pdf)
- [PN5180 User Manual](https://www.nxp.com/docs/en/user-guide/UM10954.pdf)
- [ATrappmann PN5180 Library](https://github.com/ATrappmann/PN5180-Library)
- [ESPHome Documentation](https://esphome.io/)

## License

This component is licensed under the Apache License 2.0.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

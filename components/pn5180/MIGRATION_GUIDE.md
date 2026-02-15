# Migration Guide: text_sensor to pn5180 Platform

## Why This Change?

ESPHome NFC/RFID components (like `rc522` and `pn532`) use their own platform, not `text_sensor`. This allows for proper `on_tag` triggers and better integration with ESPHome automations.

## What Changed?

### OLD Structure (text_sensor-based)
```yaml
text_sensor:
  - platform: pn5180
    name: "NFC Scanner"
    cs_pin: 16
    busy_pin: 5
    rst_pin: 17
    # on_tag doesn't work here!
```

### NEW Structure (pn5180 platform)
```yaml
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

## Step-by-Step Migration

### Step 1: Update Component Files

Replace these 3 files in `components/pn5180/`:

1. **__init__.py** → Use `__init___proper.py`
2. **pn5180.h** → Use `pn5180_proper.h`
3. **pn5180.cpp** → Use `pn5180_proper.cpp`

**Delete** (no longer needed):
- `text_sensor.py`

### Step 2: Update Your YAML Configuration

#### Add SPI Configuration
```yaml
# Add this section if not already present
spi:
  clk_pin: GPIO18   # Default ESP32 SPI pins
  miso_pin: GPIO19
  mosi_pin: GPIO23
```

#### Change from text_sensor to pn5180
**Before:**
```yaml
text_sensor:
  - platform: pn5180
    name: "NFC Scanner"
    cs_pin: 16
    busy_pin: 5
    rst_pin: 17
    update_interval: 500ms
```

**After:**
```yaml
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  update_interval: 500ms
  on_tag:
    then:
      - logger.log:
          format: "Tag detected: %s"
          args: ['tag_id.c_str()']
```

### Step 3: Test Configuration

```bash
# Validate
esphome config your_config.yaml

# Compile
esphome compile your_config.yaml

# Upload
esphome upload your_config.yaml
```

## Feature Comparison

| Feature | OLD (text_sensor) | NEW (pn5180 platform) |
|---------|-------------------|------------------------|
| Tag Reading | ✅ Yes | ✅ Yes |
| Text Sensor Entity | ✅ Yes | ❌ No* |
| on_tag Triggers | ❌ No | ✅ Yes |
| Home Assistant Events | Manual | ✅ Built-in |
| Automations | Limited | ✅ Full support |
| ESPHome Convention | ❌ Non-standard | ✅ Standard |

*You can still create a template text_sensor if needed

## Getting Tag State in Home Assistant

### OLD Way (automatic text sensor)
```yaml
# Automatically created entity
sensor.nfc_scanner
```

### NEW Way (use on_tag trigger)
```yaml
pn5180:
  on_tag:
    then:
      - homeassistant.event:
          event: esphome.nfc_tag_scanned
          data:
            tag_id: !lambda 'return tag_id;'
```

### If You Still Want a Text Sensor

You can create one manually:

```yaml
pn5180:
  on_tag:
    then:
      - text_sensor.template.publish:
          id: last_tag
          state: !lambda 'return tag_id;'

text_sensor:
  - platform: template
    name: "Last NFC Tag"
    id: last_tag
```

## Example Migration

### Complete Before (OLD)
```yaml
esphome:
  name: nfc-reader
  libraries:
    - SPI
    - PN5180_LIBRARY=https://github.com/ATrappmann/PN5180-Library.git#master

esp32:
  board: esp32dev

external_components:
  - source:
      type: git
      url: https://github.com/johnmclear/esphome_pn5180

text_sensor:
  - platform: pn5180
    name: "NFC Scanner"
    cs_pin: 16
    busy_pin: 5
    rst_pin: 17
```

### Complete After (NEW)
```yaml
esphome:
  name: nfc-reader
  libraries:
    - SPI
    - PN5180_LIBRARY=https://github.com/ATrappmann/PN5180-Library.git#master

esp32:
  board: esp32dev

external_components:
  - source:
      type: git
      url: https://github.com/johnmclear/esphome_pn5180

# NEW: SPI configuration required
spi:
  clk_pin: GPIO18
  miso_pin: GPIO19
  mosi_pin: GPIO23

# NEW: pn5180 platform with on_tag
pn5180:
  cs_pin: GPIO16
  busy_pin: GPIO5
  rst_pin: GPIO17
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['tag_id.c_str()']
      - homeassistant.event:
          event: esphome.nfc_tag_scanned
          data:
            tag_id: !lambda 'return tag_id;'
```

## Common Issues

### "Component pn5180 not found"
- Ensure `__init__.py` is updated (not `text_sensor.py`)
- Check `external_components` is pointing to correct repo
- Try `esphome clean`

### "SPI not configured"
- Add `spi:` section to your config
- Specify clk_pin, miso_pin, mosi_pin

### "tag_id not defined"
- Make sure you're using `tag_id.c_str()` in logger
- In lambda, use: `return tag_id;`

## Benefits of New Structure

✅ **Standard ESPHome pattern** - Matches rc522, pn532  
✅ **Proper on_tag support** - Native automation triggers  
✅ **Better organization** - Cleaner separation of concerns  
✅ **More flexible** - Can have multiple automations  
✅ **Home Assistant integration** - Works with tag scanning  

## Rollback

If you need to go back to the old structure:

1. Restore backup files
2. Change YAML back to `text_sensor` structure
3. Remove `spi:` section if not used elsewhere
4. Remove `on_tag:` sections

## Need Help?

Check the example configurations:
- `pn5180_proper_example.yaml` - Complete working example
- `pn5180_basic_test.yaml` - Minimal configuration

## Summary

| Task | Action |
|------|--------|
| 1. Update files | Replace __init__.py, .h, .cpp |
| 2. Delete old file | Remove text_sensor.py |
| 3. Add SPI config | Add spi: section to YAML |
| 4. Change platform | text_sensor → pn5180 |
| 5. Add on_tag | Include automation triggers |
| 6. Test | Validate, compile, upload |

Your component will now follow ESPHome standards and have full on_tag support!

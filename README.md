# NOT YET WORKING

# esphome_pn5180
pn5180 component for ESPHome

Exposes a scanned tags UID to Home Assistant - that's all, for now.

## Usage

```
external_components:
  - source:
      type: git
      url: https://github.com/johnmclear/esphome_pn5180
      ref: claude

esphome:
  libraries:
    - SPI
    - PN5180_LIBRARY=https://github.com/ATrappmann/PN5180-Library.git#master

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

# Full ESPHome example

```
esphome:
  name: myboard
  libraries:
    - SPI
    - PN5180_LIBRARY=https://github.com/ATrappmann/PN5180-Library.git#master

esp32:
  board: esp32dev
  framework:
    type: arduino


external_components:
  - source:
      type: git
      url: https://github.com/johnmclear/esphome_pn5180

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


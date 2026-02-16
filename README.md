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


# TODO

 - [ ] Create a test method that periodically (lets say every 60 seconds but should be adjustable through esphome YAML) sends a command to the pn5180 to check its availability and functionality - I imagine the NXP PN5180 spec docs explain what command to send and expect to recieve.  If this fails then allow esphome to know NFC has failed.  Perhaps from this send a RST command to reboot the PN5180.
 - [ ] Ensure update_interval is a usable and working yaml config change as per pn532 docs
 - [ ] Consider different librarys as ATrappmann/PN5180-Library.git seems neglected.  https://github.com/mjmeans/PN5180-Library and https://github.com/playfultechnology/PN5180 for example


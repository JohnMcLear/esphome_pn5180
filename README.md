# esphome_pn5180
pn5180 component for ESPHome

Exposes a scanned tags UID to Home Assistant - that's all, for now.

Referenced from 

https://github.com/esphome/feature-requests/issues/1972#issuecomment-1493112460

## Usage

```
external_components:
  - source:
      type: git
      url: https://github.com/johnmclear/esphome_pn5180

esphome:
  libraries:
    - SPI
    - PN5180_LIBRARY=https://github.com/ATrappmann/PN5180-Library.git#master
```

```
text_sensor:
  - platform: pn5180
    name: "NFC tag scanner"
    cs_pin: 16
    busy_pin: 5
    rst_pin: 17
    update_interval: 500ms
```

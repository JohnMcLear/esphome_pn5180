# esphome_pn5180
pn5180 component for ESPHome

Referenced from 

https://github.com/esphome/feature-requests/issues/1972#issuecomment-1493112460

## Usage

```
external_components:
  - source:
      type: git
      url: https://github.com/johnmclear/pn5180

esphome:
  name: fancyname
  platform: ESP32
  board: esp32dev
  libraries:
    - SPI
    - https://github.com/ATrappmann/PN5180-Library.git#master
```

```
text_sensor:
  - platform: custom
    lambda: |-
      auto pn5180_component = new PN5180Component(16, 5, 17, 500); // PIN ss, busy, rst and poll interval (every 500ms)
      App.register_component(pn5180_component);
      return {pn5180_component};
    text_sensors:
      name: "NFC tag scanner"
```

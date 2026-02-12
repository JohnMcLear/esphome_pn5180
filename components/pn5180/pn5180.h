#pragma once

#include "esphome.h"
#include "PN5180.h"
#include "PN5180ISO15693.h"

using namespace esphome;

class PN5180Component :  public PollingComponent, public text_sensor::TextSensor {
 public:
  // Constructor with pin assignments and update interval
  PN5180Component(uint8_t pin_ss, uint8_t pin_busy, uint8_t pin_rst, uint32_t update_interval)
      :   PollingComponent(update_interval), pn5180_(pin_ss, pin_busy, pin_rst) {}


  // Setup function, called once during initialization
  void setup() override {
     // Initialize the PN5180 instance
     pn5180_.begin();  

     // Reset and set up the RF settings for the PN5180
     pn5180_.reset();
     pn5180_.setupRF();
  }


  // Update function, called periodically according to the update interval
  void update() override {
    static bool default_state = false;
    uint8_t uid[8];
    // Attempt to read the UID of an ISO15693 tag
    ISO15693ErrorCode rc =  pn5180_.getInventory(uid);
    if (ISO15693_EC_OK == rc) {
      // Convert the UID to a string
      String uid_str = "";
      for (uint8_t i = 0; i < sizeof(uid); i++) {
        uid_str += String(uid[i] < 0x10 ? " 0" : " ");
        uid_str += String(uid[i], HEX);
      }
      uid_str.trim();

      // Print the UID to the log
      ESP_LOGI("pn5180", "Read UID: %s", uid_str.c_str());

      // Update the text sensor with the UID value
      publish_state(uid_str.c_str());
      default_state = false;

      // Reset and set up the RF settings for the PN5180 for the next read
      pn5180_.reset();
      pn5180_.setupRF();
    } else {
      // If reading the UID fails, log an error and update the text sensor to show no tag detected
      if (!default_state)
      {
        ESP_LOGI("pn5180", "No NFC detected");
        publish_state("No NFC detected");
      }
      default_state = true;
    }
  }


 protected:
  // The PN5180 instance used for reading ISO15693 tags
  PN5180ISO15693 pn5180_;
};

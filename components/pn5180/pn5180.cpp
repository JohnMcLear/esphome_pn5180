#include "pn5180.h"

namespace esphome {
namespace pn5180 {

PN5180Component::PN5180Component(GPIOPin *cs, GPIOPin *busy, GPIOPin *rst, uint32_t update_interval)
    : PollingComponent(update_interval), cs_(cs), busy_(busy), rst_(rst) {}

void PN5180Component::setup() {
  cs_pin_->setup();
  busy_pin_->setup();
  rst_pin_->setup();

  pn5180_.begin();
  pn5180_.reset();
  pn5180_.setupRF();
}

void PN5180Component::update() {
  static bool default_state = false;
  uint8_t uid[8];
  ISO15693ErrorCode rc = pn5180_.getInventory(uid);
  if (ISO15693_EC_OK == rc) {
    String uid_str = "";
    for (uint8_t i = 0; i < sizeof(uid); i++) {
      uid_str += String(uid[i] < 0x10 ? " 0" : " ");
      uid_str += String(uid[i], HEX);
    }
    uid_str.trim();

    ESP_LOGI("pn5180", "Read UID: %s", uid_str.c_str());
    publish_state(uid_str.c_str());
    default_state = false;

    pn5180_.reset();
    pn5180_.setupRF();
  } else {
    if (!default_state) {
      ESP_LOGI("pn5180", "No NFC detected");
      publish_state("No NFC detected");
    }
    default_state = true;
  }
}

}  // namespace pn5180
}  // namespace esphome


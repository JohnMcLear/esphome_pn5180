#include "pn5180.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pn5180 {

static const char *TAG = "pn5180";

PN5180Component::PN5180Component(
    InternalGPIOPin *cs,
    InternalGPIOPin *busy,
    InternalGPIOPin *rst,
    uint32_t update_interval)
    : PollingComponent(update_interval),
      cs_(cs),
      busy_(busy),
      rst_(rst),
      pn5180_(nullptr) {}

void PN5180Component::setup() {
  ESP_LOGI(TAG, "Initializing PN5180");

  this->pn5180_ = new PN5180ISO15693(
      this->cs_->get_pin(),
      this->busy_->get_pin(),
      this->rst_->get_pin()
  );

  this->pn5180_->begin();
  this->pn5180_->reset();
  this->pn5180_->setupRF();
}

void PN5180Component::update() {
  if (this->pn5180_ == nullptr)
    return;

  static bool no_tag_reported = false;
  uint8_t uid[8];

  ISO15693ErrorCode rc = this->pn5180_->getInventory(uid);

  if (rc == ISO15693_EC_OK) {
    char buffer[32];
    buffer[0] = '\0';

    for (uint8_t i = 0; i < sizeof(uid); i++) {
      char part[4];
      sprintf(part, "%02X", uid[i]);
      strcat(buffer, part);
      if (i < sizeof(uid) - 1)
        strcat(buffer, " ");
    }

    ESP_LOGI(TAG, "Tag detected: %s", buffer);
    this->publish_state(buffer);
    no_tag_reported = false;
  } else {
    if (!no_tag_reported) {
      ESP_LOGD(TAG, "No tag");
      this->publish_state("No NFC detected");
      no_tag_reported = true;
    }
  }
}

}  // namespace pn5180
}  // namespace esphome


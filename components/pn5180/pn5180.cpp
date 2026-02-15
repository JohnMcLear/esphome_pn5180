#include "pn5180.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pn5180 {

static const char *const TAG = "pn5180";

// PN5180Trigger implementation
PN5180Trigger::PN5180Trigger(PN5180Component *parent) {
  parent->add_on_tag_callback([this](const std::string &tag_id) { 
    this->trigger(tag_id); 
  });
}

// PN5180Component implementation
void PN5180Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PN5180...");
  
  // Setup SPI
  this->spi_setup();
  
  // Setup pins
  if (this->busy_pin_ != nullptr) {
    this->busy_pin_->setup();
  }
  if (this->rst_pin_ != nullptr) {
    this->rst_pin_->setup();
  }
  
  // Get pin numbers
  uint8_t cs_pin = ((InternalGPIOPin *)this->cs_)->get_pin();
  uint8_t busy_pin = this->busy_pin_ ? this->busy_pin_->get_pin() : 255;
  uint8_t rst_pin = this->rst_pin_ ? this->rst_pin_->get_pin() : 255;
  
  ESP_LOGCONFIG(TAG, "  CS Pin: GPIO%d", cs_pin);
  ESP_LOGCONFIG(TAG, "  BUSY Pin: GPIO%d", busy_pin);
  ESP_LOGCONFIG(TAG, "  RST Pin: GPIO%d", rst_pin);

  // Create PN5180 object
  this->pn5180_ = new PN5180ISO15693(cs_pin, busy_pin, rst_pin);

  // Initialize hardware
  this->pn5180_->begin();
  this->pn5180_->reset();
  this->pn5180_->setupRF();
  
  ESP_LOGCONFIG(TAG, "PN5180 setup complete");
}

void PN5180Component::dump_config() {
  ESP_LOGCONFIG(TAG, "PN5180:");
  LOG_PIN("  BUSY Pin: ", this->busy_pin_);
  LOG_PIN("  RST Pin: ", this->rst_pin_);
  LOG_UPDATE_INTERVAL(this);
}

void PN5180Component::update() {
  if (this->pn5180_ == nullptr) {
    ESP_LOGW(TAG, "PN5180 not initialized");
    return;
  }

  uint8_t uid[UID_SIZE];
  ISO15693ErrorCode rc = this->pn5180_->getInventory(uid);

  if (rc == ISO15693_EC_OK) {
    // Tag detected
    char buffer[UID_BUFFER_SIZE];
    
    if (!this->format_uid(uid, buffer, sizeof(buffer))) {
      ESP_LOGE(TAG, "Failed to format UID");
      return;
    }

    std::string tag_id(buffer);
    ESP_LOGI(TAG, "Tag detected: %s", tag_id.c_str());
    
    // Fire callbacks only on tag change
    if (tag_id != this->last_tag_id_) {
      this->on_tag_callbacks_.call(tag_id);
      this->last_tag_id_ = tag_id;
    }
    
    this->no_tag_reported_ = false;
  } else {
    // No tag or error
    if (!this->no_tag_reported_) {
      ESP_LOGD(TAG, "No tag detected");
      this->last_tag_id_ = "";
      this->no_tag_reported_ = true;
    }
  }
}

bool PN5180Component::format_uid(const uint8_t *uid, char *buffer, size_t buffer_size) {
  if (buffer_size < UID_BUFFER_SIZE) {
    return false;
  }

  size_t pos = 0;
  for (uint8_t i = 0; i < UID_SIZE; i++) {
    int written = snprintf(buffer + pos, buffer_size - pos, "%02X%s",
                          uid[i], (i < UID_SIZE - 1) ? " " : "");
    
    if (written < 0 || pos + written >= buffer_size) {
      return false;
    }
    pos += written;
  }
  
  buffer[buffer_size - 1] = '\0';
  return true;
}

}  // namespace pn5180
}  // namespace esphome

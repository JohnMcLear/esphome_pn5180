#include "pn5180.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pn5180 {

static const char *const TAG = "pn5180";

// PN5180TagTrigger implementation
PN5180TagTrigger::PN5180TagTrigger(PN5180Component *parent) {
  parent->add_on_tag_callback([this](const std::string &tag_id) { 
    this->trigger(tag_id); 
  });
}

// PN5180Component implementation
PN5180Component::PN5180Component(InternalGPIOPin *cs, InternalGPIOPin *busy,
                                 InternalGPIOPin *rst, uint32_t update_interval)
    : PollingComponent(update_interval), cs_(cs), busy_(busy), rst_(rst) {}

PN5180Component::~PN5180Component() {
  delete this->pn5180_;
}

void PN5180Component::setup() {
  ESP_LOGI(TAG, "Initializing PN5180 on pins CS=%d, BUSY=%d, RST=%d",
           this->cs_->get_pin(), this->busy_->get_pin(), this->rst_->get_pin());

  this->pn5180_ = new PN5180ISO15693(this->cs_->get_pin(), 
                                      this->busy_->get_pin(),
                                      this->rst_->get_pin());

  this->pn5180_->begin();
  this->pn5180_->reset();
  this->pn5180_->setupRF();
  
  ESP_LOGI(TAG, "PN5180 initialized successfully");
}

void PN5180Component::update() {
  if (this->pn5180_ == nullptr) {
    ESP_LOGW(TAG, "PN5180 not initialized, skipping update");
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
    
    // Publish state
    this->publish_state(tag_id);
    
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
      this->publish_state("");
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

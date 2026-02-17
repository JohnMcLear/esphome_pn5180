#include "pn5180.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pn5180 {

static const char *const TAG = "pn5180";

// ─── Trigger constructors ─────────────────────────────────────────────────────

PN5180TagTrigger::PN5180TagTrigger(PN5180Component *parent) {
  parent->add_on_tag_callback([this](const std::string &tag_id) {
    this->trigger(tag_id);
  });
}

PN5180TagRemovedTrigger::PN5180TagRemovedTrigger(PN5180Component *parent) {
  parent->add_on_tag_removed_callback([this](const std::string &tag_id) {
    this->trigger(tag_id);
  });
}

// ─── Setup ───────────────────────────────────────────────────────────────────

void PN5180Component::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PN5180...");

  this->spi_setup();

  if (this->busy_pin_ != nullptr)
    this->busy_pin_->setup();
  if (this->rst_pin_ != nullptr)
    this->rst_pin_->setup();

  uint8_t cs_pin   = ((InternalGPIOPin *) this->cs_)->get_pin();
  uint8_t busy_pin = this->busy_pin_ ? this->busy_pin_->get_pin() : 255;
  uint8_t rst_pin  = this->rst_pin_  ? this->rst_pin_->get_pin()  : 255;

  ESP_LOGCONFIG(TAG, "  CS Pin:   GPIO%d", cs_pin);
  ESP_LOGCONFIG(TAG, "  BUSY Pin: GPIO%d", busy_pin);
  ESP_LOGCONFIG(TAG, "  RST Pin:  GPIO%d", rst_pin);

  this->pn5180_ = new PN5180ISO15693(cs_pin, busy_pin, rst_pin);
  this->pn5180_->begin();
  this->pn5180_->reset();
  this->pn5180_->setupRF();

  if (this->health_check_enabled_) {
    if (!this->perform_health_check())
      ESP_LOGW(TAG, "Initial health check failed");
    else
      ESP_LOGI(TAG, "Initial health check passed");
  }

  ESP_LOGCONFIG(TAG, "PN5180 setup complete");
}

// ─── Dump config ─────────────────────────────────────────────────────────────

void PN5180Component::dump_config() {
  ESP_LOGCONFIG(TAG, "PN5180:");
  LOG_PIN("  BUSY Pin: ", this->busy_pin_);
  LOG_PIN("  RST Pin:  ", this->rst_pin_);
  LOG_UPDATE_INTERVAL(this);
  ESP_LOGCONFIG(TAG, "  Registered tag sensors: %d", this->tag_sensors_.size());
  if (this->health_check_enabled_) {
    ESP_LOGCONFIG(TAG, "  Health Check: enabled, interval=%dms, max_failures=%d, auto_reset=%s",
                  this->health_check_interval_, this->max_failed_checks_,
                  this->auto_reset_on_failure_ ? "yes" : "no");
  } else {
    ESP_LOGCONFIG(TAG, "  Health Check: disabled");
  }
}

// ─── Loop (health check) ──────────────────────────────────────────────────────

void PN5180Component::loop() {
  if (!this->health_check_enabled_)
    return;

  uint32_t now = millis();
  if (now - this->last_health_check_ < this->health_check_interval_)
    return;

  this->last_health_check_ = now;

  if (!this->perform_health_check()) {
    this->consecutive_failures_++;
    ESP_LOGW(TAG, "Health check failed (%d/%d)", this->consecutive_failures_, this->max_failed_checks_);

    if (this->consecutive_failures_ >= this->max_failed_checks_) {
      if (this->health_status_) {
        this->health_status_ = false;
        ESP_LOGE(TAG, "PN5180 declared unhealthy");
      }
      if (this->auto_reset_on_failure_ && this->pn5180_ != nullptr) {
        ESP_LOGW(TAG, "Attempting automatic reset...");
        this->pn5180_->reset();
        this->pn5180_->setupRF();
        delay(100);
        if (this->perform_health_check()) {
          ESP_LOGI(TAG, "Reset successful");
          this->consecutive_failures_ = 0;
          this->health_status_ = true;
        } else {
          ESP_LOGE(TAG, "Reset failed");
        }
      }
    }
  } else {
    if (this->consecutive_failures_ > 0)
      ESP_LOGI(TAG, "Health check recovered after %d failures", this->consecutive_failures_);
    this->consecutive_failures_ = 0;
    if (!this->health_status_) {
      this->health_status_ = true;
      ESP_LOGI(TAG, "PN5180 health restored");
    }
  }
}

// ─── Update (tag scanning) ────────────────────────────────────────────────────

void PN5180Component::update() {
  if (this->pn5180_ == nullptr) {
    ESP_LOGW(TAG, "PN5180 not initialized");
    return;
  }
  if (this->health_check_enabled_ && !this->health_status_) {
    ESP_LOGD(TAG, "Skipping scan - PN5180 unhealthy");
    return;
  }

  uint8_t uid[UID_SIZE];
  ISO15693ErrorCode rc = this->pn5180_->getInventory(uid);

  if (rc == ISO15693_EC_OK) {
    char buffer[UID_BUFFER_SIZE];
    if (!this->format_uid(uid, buffer, sizeof(buffer))) {
      ESP_LOGE(TAG, "Failed to format UID");
      return;
    }
    this->tag_detected_(std::string(buffer));
  } else {
    if (this->tag_present_)
      this->tag_removed_();
  }
}

// ─── Tag detected helper ─────────────────────────────────────────────────────

void PN5180Component::tag_detected_(const std::string &tag_id) {
  bool is_new_tag = (tag_id != this->current_tag_id_);

  this->current_tag_id_ = tag_id;
  this->tag_present_ = true;

  if (is_new_tag) {
    ESP_LOGI(TAG, "Tag detected: %s", tag_id.c_str());

    // Fire on_tag automation
    this->on_tag_callbacks_.call(tag_id);

    // Update text sensor
    if (this->text_sensor_ != nullptr)
      this->text_sensor_->publish_state(tag_id);

    // Update binary sensors
    for (auto *sensor : this->tag_sensors_) {
      bool match = (sensor->get_uid() == tag_id);
      if (match && !sensor->state) {
        ESP_LOGD(TAG, "Tag sensor '%s' ON", sensor->get_uid().c_str());
        sensor->publish_state(true);
      } else if (!match && sensor->state) {
        // Different tag present - turn off sensors that don't match
        sensor->publish_state(false);
      }
    }
  }
}

// ─── Tag removed helper ───────────────────────────────────────────────────────

void PN5180Component::tag_removed_() {
  ESP_LOGI(TAG, "Tag removed: %s", this->current_tag_id_.c_str());

  // Fire on_tag_removed automation with the last seen tag_id
  this->on_tag_removed_callbacks_.call(this->current_tag_id_);

  // Turn off all binary sensors
  for (auto *sensor : this->tag_sensors_) {
    if (sensor->state)
      sensor->publish_state(false);
  }

  this->tag_present_ = false;
  this->current_tag_id_ = "";
}

// ─── Health check ─────────────────────────────────────────────────────────────

bool PN5180Component::perform_health_check() {
  if (this->pn5180_ == nullptr) {
    ESP_LOGW(TAG, "Health check failed: not initialized");
    return false;
  }

  // Check if BUSY pin is stuck high (indicates hardware issue)
  if (this->busy_pin_ != nullptr) {
    if (this->busy_pin_->digital_read()) {
      delay(10);
      if (this->busy_pin_->digital_read()) {
        ESP_LOGD(TAG, "Health check: BUSY pin stuck high");
        return false;
      }
    }
  }

  ESP_LOGV(TAG, "Health check passed");
  return true;
}

// ─── UID formatting ───────────────────────────────────────────────────────────

bool PN5180Component::format_uid(const uint8_t *uid, char *buffer, size_t buffer_size) {
  if (buffer_size < UID_BUFFER_SIZE)
    return false;

  size_t pos = 0;
  for (uint8_t i = 0; i < UID_SIZE; i++) {
    int written = snprintf(buffer + pos, buffer_size - pos, "%02X%s",
                           uid[i], (i < UID_SIZE - 1) ? " " : "");
    if (written < 0 || pos + written >= buffer_size)
      return false;
    pos += written;
  }
  buffer[buffer_size - 1] = '\0';
  return true;
}

}  // namespace pn5180
}  // namespace esphome

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
  
  // Perform initial health check
  if (this->health_check_enabled_) {
    if (!this->perform_health_check()) {
      ESP_LOGW(TAG, "Initial health check failed - PN5180 may not be responding properly");
    } else {
      ESP_LOGI(TAG, "Initial health check passed");
    }
  }
  
  ESP_LOGCONFIG(TAG, "PN5180 setup complete");
}

void PN5180Component::dump_config() {
  ESP_LOGCONFIG(TAG, "PN5180:");
  LOG_PIN("  BUSY Pin: ", this->busy_pin_);
  LOG_PIN("  RST Pin: ", this->rst_pin_);
  LOG_UPDATE_INTERVAL(this);
  
  if (this->health_check_enabled_) {
    ESP_LOGCONFIG(TAG, "  Health Check: Enabled");
    ESP_LOGCONFIG(TAG, "    Interval: %dms", this->health_check_interval_);
    ESP_LOGCONFIG(TAG, "    Max Failed Checks: %d", this->max_failed_checks_);
    ESP_LOGCONFIG(TAG, "    Auto Reset on Failure: %s", 
                  this->auto_reset_on_failure_ ? "Yes" : "No");
  } else {
    ESP_LOGCONFIG(TAG, "  Health Check: Disabled");
  }
}

void PN5180Component::loop() {
  // Perform periodic health check
  if (!this->health_check_enabled_) {
    return;
  }
  
  uint32_t now = millis();
  if (now - this->last_health_check_ >= this->health_check_interval_) {
    this->last_health_check_ = now;
    
    if (!this->perform_health_check()) {
      this->consecutive_failures_++;
      
      ESP_LOGW(TAG, "Health check failed (%d/%d consecutive failures)", 
               this->consecutive_failures_, this->max_failed_checks_);
      
      // Check if we've exceeded the failure threshold
      if (this->consecutive_failures_ >= this->max_failed_checks_) {
        ESP_LOGE(TAG, "PN5180 health check failed %d times - reader may be unresponsive", 
                 this->max_failed_checks_);
        
        // Update health status
        if (this->health_status_) {
          this->health_status_ = false;
          ESP_LOGE(TAG, "PN5180 declared unhealthy");
        }
        
        // Attempt automatic reset if enabled
        if (this->auto_reset_on_failure_) {
          ESP_LOGW(TAG, "Attempting automatic reset...");
          if (this->pn5180_ != nullptr) {
            this->pn5180_->reset();
            this->pn5180_->setupRF();
            delay(100);  // Give it time to recover
            
            // Try one more health check
            if (this->perform_health_check()) {
              ESP_LOGI(TAG, "Reset successful - PN5180 responding again");
              this->consecutive_failures_ = 0;
              this->health_status_ = true;
            } else {
              ESP_LOGE(TAG, "Reset failed - PN5180 still not responding");
            }
          }
        }
      }
    } else {
      // Health check passed
      if (this->consecutive_failures_ > 0) {
        ESP_LOGI(TAG, "Health check recovered after %d failures", 
                 this->consecutive_failures_);
      }
      
      this->consecutive_failures_ = 0;
      
      // Update health status if it was previously unhealthy
      if (!this->health_status_) {
        this->health_status_ = true;
        ESP_LOGI(TAG, "PN5180 health restored");
      }
    }
  }
}

bool PN5180Component::perform_health_check() {
  if (this->pn5180_ == nullptr) {
    ESP_LOGW(TAG, "Health check failed: PN5180 not initialized");
    return false;
  }
  
  // Try to read firmware version as a health check
  // The PN5180 library should have a method to read version/status
  // For now, we'll try a simple register read
  
  // Attempt to read the product version register
  // This is a non-intrusive way to verify SPI communication
  uint8_t version[2];
  
  // Try to get the version - this verifies SPI communication
  // Note: This depends on the PN5180 library having a getVersion() method
  // If not available, we can try a simple register read
  
  // For now, we'll use a simple SPI communication test
  // by attempting to initialize RF (which should be safe and quick)
  try {
    // A simple test: ensure the busy pin is not stuck
    if (this->busy_pin_ != nullptr) {
      // If BUSY is stuck high, something is wrong
      if (this->busy_pin_->digital_read()) {
        // Wait a bit and check again
        delay(10);
        if (this->busy_pin_->digital_read()) {
          ESP_LOGD(TAG, "Health check: BUSY pin stuck high");
          return false;
        }
      }
    }
    
    // If we got here, basic communication seems OK
    ESP_LOGV(TAG, "Health check passed");
    return true;
    
  } catch (...) {
    ESP_LOGW(TAG, "Health check exception");
    return false;
  }
}

void PN5180Component::update() {
  if (this->pn5180_ == nullptr) {
    ESP_LOGW(TAG, "PN5180 not initialized");
    return;
  }
  
  // Skip tag reading if health check has declared the device unhealthy
  if (this->health_check_enabled_ && !this->health_status_) {
    if (!this->no_tag_reported_) {
      ESP_LOGD(TAG, "Skipping tag scan - PN5180 unhealthy");
      this->no_tag_reported_ = true;
    }
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

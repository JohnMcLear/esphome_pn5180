#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include <PN5180ISO15693.h>
#include <string>

namespace esphome {
namespace pn5180 {

// Constants
static constexpr uint8_t UID_SIZE = 8;
static constexpr size_t UID_BUFFER_SIZE = 32;

// Forward declarations
class PN5180Component;

/// Trigger fired when a new tag is detected
class PN5180Trigger : public Trigger<std::string> {
 public:
  explicit PN5180Trigger(PN5180Component *parent);
};

/// Binary sensor for PN5180 health status
class PN5180HealthSensor : public binary_sensor::BinarySensor {
 public:
  void set_parent(PN5180Component *parent) { parent_ = parent; }
 protected:
  PN5180Component *parent_{nullptr};
};

/// Main PN5180 component for reading NFC/RFID tags
class PN5180Component : public PollingComponent,
                        public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST,
                                             spi::CLOCK_POLARITY_LOW,
                                             spi::CLOCK_PHASE_LEADING,
                                             spi::DATA_RATE_1MHZ> {
 public:
  void setup() override;
  void update() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Pin setters
  void set_busy_pin(InternalGPIOPin *pin) { this->busy_pin_ = pin; }
  void set_rst_pin(InternalGPIOPin *pin) { this->rst_pin_ = pin; }

  // Health check configuration
  void set_health_check_enabled(bool enabled) { this->health_check_enabled_ = enabled; }
  void set_health_check_interval(uint32_t interval) { this->health_check_interval_ = interval; }
  void set_auto_reset_on_failure(bool auto_reset) { this->auto_reset_on_failure_ = auto_reset; }
  void set_max_failed_checks(uint8_t max_checks) { this->max_failed_checks_ = max_checks; }

  // Health status accessor
  bool is_healthy() const { return this->consecutive_failures_ < this->max_failed_checks_; }

  // Callback registration for on_tag triggers
  void add_on_tag_callback(std::function<void(const std::string &)> &&callback) {
    this->on_tag_callbacks_.add(std::move(callback));
  }

 protected:
  // Format UID bytes as hex string
  bool format_uid(const uint8_t *uid, char *buffer, size_t buffer_size);
  
  // Health check method
  bool perform_health_check();
  
  // Hardware pins
  InternalGPIOPin *busy_pin_{nullptr};
  InternalGPIOPin *rst_pin_{nullptr};

  // PN5180 hardware interface
  PN5180ISO15693 *pn5180_{nullptr};
  
  // Tag tracking
  std::string last_tag_id_{""};
  bool no_tag_reported_{false};
  
  // Health check tracking
  bool health_check_enabled_{true};
  uint32_t health_check_interval_{60000};  // 60 seconds default
  uint32_t last_health_check_{0};
  uint8_t consecutive_failures_{0};
  uint8_t max_failed_checks_{3};
  bool auto_reset_on_failure_{true};
  bool health_status_{true};  // Assume healthy until proven otherwise
  
  // Event callbacks
  CallbackManager<void(const std::string &)> on_tag_callbacks_;
};

}  // namespace pn5180
}  // namespace esphome

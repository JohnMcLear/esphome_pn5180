#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/components/spi/spi.h"
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

/// Main PN5180 component for reading NFC/RFID tags
class PN5180Component : public PollingComponent, 
                        public spi::SPIDevice<> {
 public:
  void setup() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Pin setters
  void set_busy_pin(InternalGPIOPin *pin) { this->busy_pin_ = pin; }
  void set_rst_pin(InternalGPIOPin *pin) { this->rst_pin_ = pin; }

  // Callback registration for on_tag triggers
  void add_on_tag_callback(std::function<void(const std::string &)> &&callback) {
    this->on_tag_callbacks_.add(std::move(callback));
  }

 protected:
  // Format UID bytes as hex string
  bool format_uid(const uint8_t *uid, char *buffer, size_t buffer_size);
  
  // Hardware pins
  InternalGPIOPin *busy_pin_{nullptr};
  InternalGPIOPin *rst_pin_{nullptr};

  // PN5180 hardware interface
  PN5180ISO15693 *pn5180_{nullptr};
  
  // Tag tracking
  std::string last_tag_id_{""};
  bool no_tag_reported_{false};
  
  // Event callbacks
  CallbackManager<void(const std::string &)> on_tag_callbacks_;
};

}  // namespace pn5180
}  // namespace esphome

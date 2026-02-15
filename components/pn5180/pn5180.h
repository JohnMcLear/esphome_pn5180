#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/components/spi/spi.h"
#include <PN5180ISO15693.h>
#include <string>
#include <vector>

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
class PN5180Component : public PollingComponent, public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW, spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_1MHZ> {
 public:
  void setup() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void dump_config() override;

  // Pin setters
  void set_busy_pin(GPIOPin *busy_pin) { busy_pin_ = busy_pin; }
  void set_rst_pin(GPIOPin *rst_pin) { rst_pin_ = rst_pin; }

  // Callback registration for on_tag triggers
  void add_on_tag_callback(std::function<void(const std::string &)> &&callback) {
    on_tag_callbacks_.add(std::move(callback));
  }

 protected:
  // Format UID bytes as hex string
  bool format_uid(const uint8_t *uid, char *buffer, size_t buffer_size);
  
  // Hardware pins
  GPIOPin *busy_pin_{nullptr};
  GPIOPin *rst_pin_{nullptr};

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

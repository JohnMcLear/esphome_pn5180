#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <PN5180ISO15693.h>
#include <string>

namespace esphome {
namespace pn5180 {

// Constants
static constexpr uint8_t UID_SIZE = 8;
static constexpr size_t UID_BUFFER_SIZE = 32;  // "XX XX XX XX XX XX XX XX\0"

// Forward declarations
class PN5180Component;

/// Trigger fired when a new tag is detected
class PN5180TagTrigger : public Trigger<std::string> {
 public:
  explicit PN5180TagTrigger(PN5180Component *parent);
};

/// Main PN5180 component for reading NFC/RFID tags
class PN5180Component : public PollingComponent,
                        public text_sensor::TextSensor {
 public:
  PN5180Component(InternalGPIOPin *cs, InternalGPIOPin *busy, 
                  InternalGPIOPin *rst, uint32_t update_interval);
  ~PN5180Component();

  // Component lifecycle
  void setup() override;
  void update() override;

  // Callback registration
  void add_on_tag_callback(std::function<void(const std::string &)> &&callback) {
    on_tag_callbacks_.add(std::move(callback));
  }

 protected:
  // Format UID bytes as hex string
  bool format_uid(const uint8_t *uid, char *buffer, size_t buffer_size);
  
  // Hardware pins
  InternalGPIOPin *cs_;
  InternalGPIOPin *busy_;
  InternalGPIOPin *rst_;

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

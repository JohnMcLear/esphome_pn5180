#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <PN5180ISO15693.h>
#include <string>
#include <vector>

namespace esphome {
namespace pn5180 {

static constexpr uint8_t UID_SIZE = 8;
static constexpr size_t UID_BUFFER_SIZE = 32;

class PN5180Component;

// ─── Triggers ────────────────────────────────────────────────────────────────

/// Fires when a new tag is detected (tag_id passed to automation)
class PN5180TagTrigger : public Trigger<std::string> {
 public:
  explicit PN5180TagTrigger(PN5180Component *parent);
};

/// Fires when a tag is removed (last tag_id passed to automation)
class PN5180TagRemovedTrigger : public Trigger<std::string> {
 public:
  explicit PN5180TagRemovedTrigger(PN5180Component *parent);
};

// ─── Binary Sensor ───────────────────────────────────────────────────────────

/// Binary sensor that tracks a specific known tag UID
class PN5180BinarySensor : public binary_sensor::BinarySensor {
 public:
  void set_parent(PN5180Component *parent) { this->parent_ = parent; }
  void set_uid(const std::string &uid) { this->uid_ = uid; }
  const std::string &get_uid() const { return this->uid_; }

 protected:
  PN5180Component *parent_{nullptr};
  std::string uid_;
};

// ─── Text Sensor ─────────────────────────────────────────────────────────────

/// Text sensor that publishes the last scanned tag UID
class PN5180TextSensor : public text_sensor::TextSensor {
 public:
  void set_parent(PN5180Component *parent) { this->parent_ = parent; }

 protected:
  PN5180Component *parent_{nullptr};
};

// ─── Main Component ──────────────────────────────────────────────────────────

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

  // ── Pin configuration ──
  void set_busy_pin(InternalGPIOPin *pin) { this->busy_pin_ = pin; }
  void set_rst_pin(InternalGPIOPin *pin) { this->rst_pin_ = pin; }

  // ── Health check configuration ──
  void set_health_check_enabled(bool enabled) { this->health_check_enabled_ = enabled; }
  void set_health_check_interval(uint32_t ms) { this->health_check_interval_ = ms; }
  void set_auto_reset_on_failure(bool v) { this->auto_reset_on_failure_ = v; }
  void set_max_failed_checks(uint8_t v) { this->max_failed_checks_ = v; }

  // ── Sensor registration ──
  void register_tag_sensor(PN5180BinarySensor *sensor) {
    this->tag_sensors_.push_back(sensor);
  }
  void set_text_sensor(PN5180TextSensor *sensor) {
    this->text_sensor_ = sensor;
  }

  // ── Callbacks ──
  void add_on_tag_callback(std::function<void(const std::string &)> &&cb) {
    this->on_tag_callbacks_.add(std::move(cb));
  }
  void add_on_tag_removed_callback(std::function<void(const std::string &)> &&cb) {
    this->on_tag_removed_callbacks_.add(std::move(cb));
  }

 protected:
  bool format_uid(const uint8_t *uid, char *buffer, size_t buffer_size);
  bool perform_health_check();
  void tag_detected_(const std::string &tag_id);
  void tag_removed_();

  // Hardware
  InternalGPIOPin *busy_pin_{nullptr};
  InternalGPIOPin *rst_pin_{nullptr};
  PN5180ISO15693 *pn5180_{nullptr};

  // Tag state
  std::string current_tag_id_{""};
  bool tag_present_{false};

  // Binary sensors for known tags
  std::vector<PN5180BinarySensor *> tag_sensors_;

  // Text sensor for last scanned tag
  PN5180TextSensor *text_sensor_{nullptr};

  // Health check state
  bool health_check_enabled_{true};
  uint32_t health_check_interval_{60000};
  uint32_t last_health_check_{0};
  uint8_t consecutive_failures_{0};
  uint8_t max_failed_checks_{3};
  bool auto_reset_on_failure_{true};
  bool health_status_{true};

  // Callbacks
  CallbackManager<void(const std::string &)> on_tag_callbacks_;
  CallbackManager<void(const std::string &)> on_tag_removed_callbacks_;
};

}  // namespace pn5180
}  // namespace esphome

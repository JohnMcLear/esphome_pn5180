#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include <PN5180ISO15693.h>
#include <string>
#include <vector>

namespace esphome {
namespace pn5180 {

static constexpr uint8_t UID_SIZE = 8;
static constexpr size_t UID_BUFFER_SIZE = 32;

// PN5180 EEPROM Register Addresses (for RF tuning)
static constexpr uint8_t EEPROM_RF_CONFIG_REG = 0x00;
static constexpr uint8_t EEPROM_RF_DRIVER_ENABLE = 0x38;
static constexpr uint8_t EEPROM_DPC_CONFIG = 0x3A;

class PN5180Component;

// ─── Triggers ────────────────────────────────────────────────────────────────

class PN5180TagTrigger : public Trigger<std::string> {
 public:
  explicit PN5180TagTrigger(PN5180Component *parent);
};

class PN5180TagRemovedTrigger : public Trigger<std::string> {
 public:
  explicit PN5180TagRemovedTrigger(PN5180Component *parent);
};

// ─── Binary Sensor ───────────────────────────────────────────────────────────

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

class PN5180TextSensor : public text_sensor::TextSensor {
 public:
  void set_parent(PN5180Component *parent) { this->parent_ = parent; }

 protected:
  PN5180Component *parent_{nullptr};
};

// ─── RF Protocol Enum ────────────────────────────────────────────────────────

enum RFProtocol {
  RF_PROTOCOL_ISO14443A,
  RF_PROTOCOL_ISO14443B,
  RF_PROTOCOL_ISO15693,
  RF_PROTOCOL_FELICA,
  RF_PROTOCOL_AUTO
};

// ─── Main Component (Enhanced) ───────────────────────────────────────────────

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

  // ── NEW: RF Power Configuration ──
  void set_rf_power_level(uint8_t level) { this->rf_power_level_ = level; }
  void set_rf_collision_avoidance(bool enabled) { this->rf_collision_avoidance_ = enabled; }
  void set_rf_protocol_priority(RFProtocol protocol) { this->rf_protocol_priority_ = protocol; }

  // ── NEW: LPCD Configuration ──
  void set_lpcd_enabled(bool enabled) { this->lpcd_enabled_ = enabled; }
  void set_lpcd_interval(uint32_t ms) { this->lpcd_interval_ = ms; }

  // ── NEW: RF Diagnostics ──
  void set_publish_diagnostics(bool enabled) { this->publish_diagnostics_ = enabled; }
  void set_agc_sensor(sensor::Sensor *sensor) { this->agc_sensor_ = sensor; }
  void set_rf_field_sensor(sensor::Sensor *sensor) { this->rf_field_sensor_ = sensor; }
  void set_temperature_sensor(sensor::Sensor *sensor) { this->temperature_sensor_ = sensor; }

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
  // ── Core functions ──
  bool format_uid(const uint8_t *uid, char *buffer, size_t buffer_size);
  bool perform_health_check();
  void tag_detected_(const std::string &tag_id);
  void tag_removed_();

  // ── NEW: RF Configuration functions ──
  bool configure_rf_power_();
  bool configure_dpc_();
  bool configure_protocol_priority_();

  // ── NEW: LPCD functions ──
  void lpcd_enter_();
  void lpcd_exit_();
  bool lpcd_check_card_presence_();

  // ── NEW: RF Diagnostics functions ──
  void update_diagnostics_();
  uint16_t read_agc_();
  uint16_t read_rf_field_strength_();
  float read_temperature_();
  bool verify_rf_config_();

  // ── Hardware ──
  InternalGPIOPin *busy_pin_{nullptr};
  InternalGPIOPin *rst_pin_{nullptr};
  PN5180ISO15693 *pn5180_{nullptr};

  // ── Tag state ──
  std::string current_tag_id_{""};
  bool tag_present_{false};

  // ── Binary sensors for known tags ──
  std::vector<PN5180BinarySensor *> tag_sensors_;

  // ── Text sensor for last scanned tag ──
  PN5180TextSensor *text_sensor_{nullptr};

  // ── Health check state ──
  bool health_check_enabled_{true};
  uint32_t health_check_interval_{60000};
  uint32_t last_health_check_{0};
  uint8_t consecutive_failures_{0};
  uint8_t max_failed_checks_{3};
  bool auto_reset_on_failure_{true};
  bool health_status_{true};

  // ── NEW: RF Power Configuration ──
  uint8_t rf_power_level_{200};  // 0-255, default 200 (safe high power)
  bool rf_collision_avoidance_{true};  // DPC enabled by default
  RFProtocol rf_protocol_priority_{RF_PROTOCOL_AUTO};

  // ── NEW: LPCD State ──
  bool lpcd_enabled_{false};
  uint32_t lpcd_interval_{100};  // Check every 100ms in LPCD mode
  uint32_t last_lpcd_check_{0};
  bool lpcd_active_{false};

  // ── NEW: RF Diagnostics State ──
  bool publish_diagnostics_{false};
  sensor::Sensor *agc_sensor_{nullptr};
  sensor::Sensor *rf_field_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_{nullptr};
  uint32_t last_diagnostics_update_{0};
  static constexpr uint32_t DIAGNOSTICS_UPDATE_INTERVAL = 5000;  // 5s

  // ── Thermal Protection ──
  float last_temperature_{0.0f};
  bool thermal_throttle_active_{false};
  static constexpr float THERMAL_THROTTLE_TEMP = 70.0f;
  static constexpr float THERMAL_RECOVER_TEMP = 60.0f;

  // ── Callbacks ──
  CallbackManager<void(const std::string &)> on_tag_callbacks_;
  CallbackManager<void(const std::string &)> on_tag_removed_callbacks_;
};

}  // namespace pn5180
}  // namespace esphome

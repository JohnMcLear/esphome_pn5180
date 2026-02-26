#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/automation.h"
#include "esphome/components/spi/spi.h"
#include <vector>
#include <map>

namespace esphome {
namespace pn5180 {

// PN5180 Command Codes
static const uint8_t PN5180_CMD_WRITE_REGISTER = 0x00;
static const uint8_t PN5180_CMD_WRITE_REGISTER_OR_MASK = 0x01;
static const uint8_t PN5180_CMD_WRITE_REGISTER_AND_MASK = 0x02;
static const uint8_t PN5180_CMD_READ_REGISTER = 0x04;
static const uint8_t PN5180_CMD_READ_EEPROM = 0x07;
static const uint8_t PN5180_CMD_SEND_DATA = 0x09;
static const uint8_t PN5180_CMD_READ_DATA = 0x0A;
static const uint8_t PN5180_CMD_LOAD_RF_CONFIG = 0x11;
static const uint8_t PN5180_CMD_RF_ON = 0x16;
static const uint8_t PN5180_CMD_RF_OFF = 0x17;

// PN5180 Registers
static const uint32_t PN5180_REG_SYSTEM_CONFIG = 0x00;
static const uint32_t PN5180_REG_IRQ_ENABLE = 0x01;
static const uint32_t PN5180_REG_IRQ_STATUS = 0x02;
static const uint32_t PN5180_REG_IRQ_CLEAR = 0x03;
static const uint32_t PN5180_REG_TRANSCEIVE_CONTROL = 0x04;
static const uint32_t PN5180_REG_SYSTEM_STATUS = 0x10;
static const uint32_t PN5180_REG_RX_STATUS = 0x13;
static const uint32_t PN5180_REG_RF_STATUS = 0x1D;

// IRQ Status Bits (Corrected per PN5180 Datasheet)
static const uint32_t PN5180_IRQ_RX = (1 << 0);            // RxDone
static const uint32_t PN5180_IRQ_TX = (1 << 1);            // TxDone
static const uint32_t PN5180_IRQ_IDLE = (1 << 2);          // Idle
static const uint32_t PN5180_IRQ_MODE_DET = (1 << 3);      // ModeDet
static const uint32_t PN5180_IRQ_CARD_ACT = (1 << 4);      // CardAct
static const uint32_t PN5180_IRQ_STATE_CHANGE = (1 << 5);  // StateChange
static const uint32_t PN5180_IRQ_RF_OFF_DET = (1 << 6);    // RFOffDet
static const uint32_t PN5180_IRQ_RF_ON_DET = (1 << 7);     // RFOnDet
static const uint32_t PN5180_IRQ_TX_RF_OFF = (1 << 8);     // TxRFOff
static const uint32_t PN5180_IRQ_TX_RF_ON = (1 << 9);      // TxRFOn
static const uint32_t PN5180_IRQ_GENERAL_ERROR = (1 << 17);

class PN5180;

using PN5180Trigger = Trigger<std::string>;
using PN5180TagRemovedTrigger = Trigger<std::string>;

class PN5180 : public PollingComponent,
               public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_LOW,
                                     spi::CLOCK_PHASE_LEADING, spi::DATA_RATE_1MHZ> {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_busy_pin(GPIOPin *pin) { this->busy_pin_ = pin; }
  void set_rst_pin(GPIOPin *pin) { this->rst_pin_ = pin; }
  void set_tag_ttl(uint32_t ttl) { this->tag_ttl_ = ttl; }

  void register_tag(PN5180Trigger *trig) { this->triggers_on_tag_.push_back(trig); }
  void register_tag_removed(PN5180TagRemovedTrigger *trig) { this->triggers_on_tag_removed_.push_back(trig); }

 protected:
  bool reset();
  bool wait_busy(bool level, uint32_t timeout_ms = 500);
  bool transceive(const uint8_t *send_buf, size_t send_len, uint8_t *recv_buf = nullptr, size_t recv_len = 0);
  
  bool write_register(uint32_t reg, uint32_t value);
  bool write_register_or_mask(uint32_t reg, uint32_t mask);
  bool write_register_and_mask(uint32_t reg, uint32_t mask);
  bool read_register(uint32_t reg, uint32_t &value);
  bool read_eeprom(uint8_t addr, uint8_t &value);
  bool load_rf_config(uint8_t tx_config, uint8_t rx_config);
  bool rf_on();
  bool rf_off();
  bool send_data(const uint8_t *data, uint8_t len, uint8_t valid_bits = 0);
  bool read_data(uint8_t *data, uint8_t &len);
  
  bool read_version();
  bool configure_iso14443a();
  bool inventory_iso14443a(std::vector<uint8_t> &uid);
  bool process_tag_scan();
  
  std::string format_uid(const std::vector<uint8_t> &uid);
  
  GPIOPin *busy_pin_{nullptr};
  GPIOPin *rst_pin_{nullptr};
  
  uint32_t tag_ttl_{1000};
  
  std::vector<PN5180Trigger *> triggers_on_tag_;
  std::vector<PN5180TagRemovedTrigger *> triggers_on_tag_removed_;
  
  std::map<std::string, uint32_t> current_tags_;
  std::string last_tag_uid_{};
  
  enum State {
    STATE_NOT_INITIALIZED,
    STATE_READY,
  } state_{STATE_NOT_INITIALIZED};
};

}  // namespace pn5180
}  // namespace esphome

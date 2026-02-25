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
static const uint32_t PN5180_REG_TIMER1_RELOAD = 0x0C;
static const uint32_t PN5180_REG_TIMER1_CONFIG = 0x0F;
static const uint32_t PN5180_REG_RX_STATUS = 0x13;
static const uint32_t PN5180_REG_RF_STATUS = 0x1D;

// RF Protocols
static const uint8_t PN5180_PROTOCOL_ISO15693 = 0x01;
static const uint8_t PN5180_PROTOCOL_ISO14443A = 0x00;

// IRQ Status Bits
static const uint32_t PN5180_IRQ_RX_SOF = (1 << 0);
static const uint32_t PN5180_IRQ_RX = (1 << 1);
static const uint32_t PN5180_IRQ_TX = (1 << 2);
static const uint32_t PN5180_IRQ_IDLE = (1 << 3);
static const uint32_t PN5180_IRQ_MODE_DETECTED = (1 << 4);
static const uint32_t PN5180_IRQ_CARD_ACTIVATED = (1 << 5);
static const uint32_t PN5180_IRQ_STATE_CHANGE = (1 << 6);
static const uint32_t PN5180_IRQ_RFOFF_DET = (1 << 7);
static const uint32_t PN5180_IRQ_RFON_DET = (1 << 8);
static const uint32_t PN5180_IRQ_TX_RFOFF = (1 << 9);
static const uint32_t PN5180_IRQ_TX_RFON = (1 << 10);
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

  void set_cs_pin(GPIOPin *pin) { this->cs_pin_ = pin; }
  void set_busy_pin(GPIOPin *pin) { this->busy_pin_ = pin; }
  void set_rst_pin(GPIOPin *pin) { this->rst_pin_ = pin; }
  void set_tag_ttl(uint32_t ttl) { this->tag_ttl_ = ttl; }
  void set_emulation_message(const std::string &msg) { this->emulation_message_ = msg; }

  void register_tag(PN5180Trigger *trig) { this->triggers_on_tag_.push_back(trig); }
  void register_tag_removed(PN5180TagRemovedTrigger *trig) { this->triggers_on_tag_removed_.push_back(trig); }

 protected:
  // Hardware control
  bool reset();
  bool wait_busy();
  
  // SPI Commands
  bool write_register(uint32_t reg, uint32_t value);
  bool read_register(uint32_t reg, uint32_t &value);
  bool write_eeprom(uint8_t addr, uint8_t value);
  bool read_eeprom(uint8_t addr, uint8_t &value);
  bool load_rf_config(uint8_t tx_config, uint8_t rx_config);
  bool rf_on();
  bool rf_off();
  bool send_data(const uint8_t *data, uint8_t len);
  bool read_data(uint8_t *data, uint8_t &len);
  
  // NFC Operations
  bool read_version();
  bool configure_iso14443a();
  bool inventory_iso14443a(std::vector<uint8_t> &uid);
  bool process_tag_scan();
  
  // Helper functions
  std::string format_uid(const std::vector<uint8_t> &uid);
  
  GPIOPin *cs_pin_{nullptr};
  GPIOPin *busy_pin_{nullptr};
  GPIOPin *rst_pin_{nullptr};
  
  uint32_t tag_ttl_{1000};
  std::string emulation_message_{};
  
  std::vector<PN5180Trigger *> triggers_on_tag_;
  std::vector<PN5180TagRemovedTrigger *> triggers_on_tag_removed_;
  
  std::map<std::string, uint32_t> current_tags_;
  std::string last_tag_uid_{};
  
  enum State {
    STATE_NOT_INITIALIZED,
    STATE_READY,
    STATE_SCANNING,
  } state_{STATE_NOT_INITIALIZED};
};

}  // namespace pn5180
}  // namespace esphome

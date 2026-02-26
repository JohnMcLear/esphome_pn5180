#include "pn5180.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace pn5180 {

static const char *const TAG = "pn5180";

void PN5180::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PN5180 component...");
  this->busy_pin_->setup();
  this->rst_pin_->setup();
  this->rst_pin_->digital_write(true);
  
  this->spi_setup();
  
  if (!this->reset()) {
    ESP_LOGE(TAG, "Failed to reset PN5180");
    this->mark_failed();
    return;
  }
  
  delay(50);
  
  if (!this->read_version()) {
    ESP_LOGE(TAG, "Failed to read PN5180 version");
    this->mark_failed();
    return;
  }
  
  this->rf_off();
  this->state_ = STATE_READY;
}

void PN5180::loop() {}

void PN5180::update() {
  if (this->state_ != STATE_READY) return;
  
  uint32_t now = millis();
  auto it = this->current_tags_.begin();
  while (it != this->current_tags_.end()) {
    if (now - it->second > this->tag_ttl_) {
      ESP_LOGD(TAG, "Tag removed: %s", it->first.c_str());
      for (auto *trigger : this->triggers_on_tag_removed_) trigger->trigger(it->first);
      it = this->current_tags_.erase(it);
    } else {
      ++it;
    }
  }
  
  this->process_tag_scan();
}

void PN5180::dump_config() {
  ESP_LOGCONFIG(TAG, "PN5180:");
  LOG_PIN("  BUSY Pin: ", this->busy_pin_);
  LOG_PIN("  RST Pin: ", this->rst_pin_);
  LOG_UPDATE_INTERVAL(this);
}

bool PN5180::reset() {
  this->rst_pin_->digital_write(false);
  delay(10);
  this->rst_pin_->digital_write(true);
  delay(10);
  this->write_register(PN5180_REG_IRQ_CLEAR, 0xFFFFFFFF);
  return this->wait_busy();
}

bool PN5180::wait_busy() {
  uint32_t start = millis();
  while (this->busy_pin_->digital_read()) {
    if (millis() - start > 100) return false;
    delay(1);
  }
  return true;
}

bool PN5180::write_register(uint32_t reg, uint32_t value) {
  if (!this->wait_busy()) return false;
  this->enable();
  this->transfer_byte(PN5180_CMD_WRITE_REGISTER);
  this->transfer_byte(reg & 0xFF);
  this->transfer_byte(value & 0xFF);
  this->transfer_byte((value >> 8) & 0xFF);
  this->transfer_byte((value >> 16) & 0xFF);
  this->transfer_byte((value >> 24) & 0xFF);
  this->disable();
  delayMicroseconds(10);
  return true;
}

bool PN5180::read_register(uint32_t reg, uint32_t &value) {
  if (!this->wait_busy()) return false;
  this->enable();
  this->transfer_byte(PN5180_CMD_READ_REGISTER);
  this->transfer_byte(reg & 0xFF);
  this->disable();
  if (!this->wait_busy()) return false;
  this->enable();
  uint8_t b0 = this->transfer_byte(0x00);
  uint8_t b1 = this->transfer_byte(0x00);
  uint8_t b2 = this->transfer_byte(0x00);
  uint8_t b3 = this->transfer_byte(0x00);
  this->disable();
  value = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
  delayMicroseconds(10);
  return (value != 0xFFFFFFFF);
}

bool PN5180::read_eeprom(uint8_t addr, uint8_t &value) {
  if (!this->wait_busy()) return false;
  this->enable();
  this->transfer_byte(PN5180_CMD_READ_EEPROM);
  this->transfer_byte(addr);
  this->transfer_byte(0x01);
  this->disable();
  if (!this->wait_busy()) return false;
  this->enable();
  value = this->transfer_byte(0x00);
  this->disable();
  delayMicroseconds(10);
  return true;
}

bool PN5180::load_rf_config(uint8_t tx_config, uint8_t rx_config) {
  if (!this->wait_busy()) return false;
  this->enable();
  this->transfer_byte(PN5180_CMD_LOAD_RF_CONFIG);
  this->transfer_byte(tx_config);
  this->transfer_byte(rx_config);
  this->disable();
  delay(5);
  return this->wait_busy();
}

bool PN5180::rf_on() {
  if (!this->wait_busy()) return false;
  this->enable();
  this->transfer_byte(PN5180_CMD_RF_ON);
  this->disable();
  delay(10);
  return this->wait_busy();
}

bool PN5180::rf_off() {
  if (!this->wait_busy()) return false;
  this->enable();
  this->transfer_byte(PN5180_CMD_RF_OFF);
  this->disable();
  delay(10);
  return this->wait_busy();
}

bool PN5180::send_data(const uint8_t *data, uint8_t len) {
  if (!this->wait_busy()) return false;
  this->enable();
  this->transfer_byte(PN5180_CMD_SEND_DATA);
  this->transfer_byte(0x00);
  for(uint8_t i=0; i<len; i++) this->transfer_byte(data[i]);
  this->disable();
  delayMicroseconds(10);
  return true;
}

bool PN5180::read_data(uint8_t *data, uint8_t &len) {
  uint32_t rx_status;
  if (!this->read_register(PN5180_REG_RX_STATUS, rx_status)) return false;
  uint8_t rx_len = rx_status & 0x1FF;
  if (rx_len == 0) { len = 0; return true; }
  this->enable();
  this->transfer_byte(PN5180_CMD_READ_DATA);
  this->disable();
  if (!this->wait_busy()) return false;
  this->enable();
  for (uint8_t i = 0; i < rx_len && i < len; i++) data[i] = this->transfer_byte(0x00);
  this->disable();
  len = rx_len;
  delayMicroseconds(10);
  return true;
}

bool PN5180::read_version() {
  uint8_t v1, v2, v3;
  if (!this->read_eeprom(0x12, v1) || !this->read_eeprom(0x13, v2) || !this->read_eeprom(0x14, v3)) return false;
  ESP_LOGI(TAG, "PN5180 Version: %02X %02X %02X", v1, v2, v3);
  return (v1 != 0xFF && v1 != 0x00);
}

bool PN5180::configure_iso14443a() {
  this->write_register(PN5180_REG_IRQ_CLEAR, 0xFFFFFFFF);
  if (!this->load_rf_config(0x00, 0x00)) return false;
  if (!this->rf_on()) return false;
  delay(10);
  return true;
}

bool PN5180::inventory_iso14443a(std::vector<uint8_t> &uid) {
  this->write_register(PN5180_REG_IRQ_CLEAR, 0xFFFFFFFF);
  this->write_register(PN5180_REG_TRANSCEIVE_CONTROL, 0x07);
  this->enable();
  this->transfer_byte(PN5180_CMD_SEND_DATA);
  this->transfer_byte(0x00);
  this->transfer_byte(0x26); 
  this->disable();
  uint32_t irq = 0, start = millis();
  while (millis() - start < 20) {
    if (this->read_register(PN5180_REG_IRQ_STATUS, irq)) {
      if (irq & (PN5180_IRQ_RX | PN5180_IRQ_GENERAL_ERROR)) break;
    }
    delay(1);
  }
  if (!(irq & PN5180_IRQ_RX)) return false;
  uint8_t atqa[2], atqa_len = 2;
  if (!this->read_data(atqa, atqa_len)) return false;
  ESP_LOGI(TAG, "Tag found! ATQA: %02X %02X", atqa[0], atqa[1]);
  this->write_register(PN5180_REG_IRQ_CLEAR, 0xFFFFFFFF);
  this->write_register(PN5180_REG_TRANSCEIVE_CONTROL, 0x00); 
  uint8_t anticoll[] = {0x93, 0x20};
  this->send_data(anticoll, 2);
  start = millis();
  while (millis() - start < 20) {
    if (this->read_register(PN5180_REG_IRQ_STATUS, irq)) {
      if (irq & (PN5180_IRQ_RX | PN5180_IRQ_GENERAL_ERROR)) break;
    }
    delay(1);
  }
  if (!(irq & PN5180_IRQ_RX)) return false;
  uint8_t uid_data[10], uid_len = 10;
  if (this->read_data(uid_data, uid_len) && uid_len >= 4) {
    uid.assign(uid_data, uid_data + 4);
    return true;
  }
  return false;
}

bool PN5180::process_tag_scan() {
  this->reset(); 
  if (!this->configure_iso14443a()) return false;
  std::vector<uint8_t> uid;
  if (this->inventory_iso14443a(uid)) {
    std::string uid_str = this->format_uid(uid);
    if (this->current_tags_.find(uid_str) == this->current_tags_.end()) {
      ESP_LOGI(TAG, "New tag: %s", uid_str.c_str());
      for (auto *trig : this->triggers_on_tag_) trig->trigger(uid_str);
    }
    this->current_tags_[uid_str] = millis();
  }
  this->rf_off();
  return true;
}

std::string PN5180::format_uid(const std::vector<uint8_t> &uid) {
  std::string res;
  for (size_t i = 0; i < uid.size(); i++) {
    if (i > 0) res += "-";
    char buf[3]; snprintf(buf, 3, "%02X", uid[i]); res += buf;
  }
  return res;
}

}
}

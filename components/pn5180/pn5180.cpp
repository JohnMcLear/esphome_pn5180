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
  delay(1);
  this->rst_pin_->digital_write(true);
  delay(10);
  uint32_t irq;
  uint32_t start = millis();
  while (millis() - start < 500) {
    if (this->read_register(PN5180_REG_IRQ_STATUS, irq)) {
      if (irq & PN5180_IRQ_IDLE) break;
    }
    delay(1);
  }
  this->write_register(PN5180_REG_IRQ_CLEAR, 0xFFFFFFFF);
  return true;
}

bool PN5180::wait_busy(bool level, uint32_t timeout_ms) {
  uint32_t start = millis();
  while (this->busy_pin_->digital_read() != level) {
    if (millis() - start > timeout_ms) return false;
    yield();
  }
  return true;
}

bool PN5180::transceive(const uint8_t *send_buf, size_t send_len, uint8_t *recv_buf, size_t recv_len) {
  if (!this->wait_busy(false)) return false;
  this->enable();
  delayMicroseconds(10);
  for (size_t i = 0; i < send_len; i++) this->transfer_byte(send_buf[i]);
  this->wait_busy(true);
  this->disable();
  if (!this->wait_busy(false)) return false;
  if (recv_buf == nullptr || recv_len == 0) return true;
  this->enable();
  delayMicroseconds(10);
  for (size_t i = 0; i < recv_len; i++) recv_buf[i] = this->transfer_byte(0x00);
  this->wait_busy(true);
  this->disable();
  this->wait_busy(false);
  return true;
}

bool PN5180::write_register(uint32_t reg, uint32_t value) {
  uint8_t buf[6];
  buf[0] = PN5180_CMD_WRITE_REGISTER;
  buf[1] = reg & 0xFF;
  buf[2] = value & 0xFF;
  buf[3] = (value >> 8) & 0xFF;
  buf[4] = (value >> 16) & 0xFF;
  buf[5] = (value >> 24) & 0xFF;
  return this->transceive(buf, 6);
}

bool PN5180::write_register_or_mask(uint32_t reg, uint32_t mask) {
  uint8_t buf[6];
  buf[0] = PN5180_CMD_WRITE_REGISTER_OR_MASK;
  buf[1] = reg & 0xFF;
  buf[2] = mask & 0xFF;
  buf[3] = (mask >> 8) & 0xFF;
  buf[4] = (mask >> 16) & 0xFF;
  buf[5] = (mask >> 24) & 0xFF;
  return this->transceive(buf, 6);
}

bool PN5180::write_register_and_mask(uint32_t reg, uint32_t mask) {
  uint8_t buf[6];
  buf[0] = PN5180_CMD_WRITE_REGISTER_AND_MASK;
  buf[1] = reg & 0xFF;
  buf[2] = mask & 0xFF;
  buf[3] = (mask >> 8) & 0xFF;
  buf[4] = (mask >> 16) & 0xFF;
  buf[5] = (mask >> 24) & 0xFF;
  return this->transceive(buf, 6);
}

bool PN5180::read_register(uint32_t reg, uint32_t &value) {
  uint8_t cmd[2] = { PN5180_CMD_READ_REGISTER, (uint8_t)(reg & 0xFF) };
  uint8_t resp[4];
  if (!this->transceive(cmd, 2, resp, 4)) return false;
  value = resp[0] | (resp[1] << 8) | (resp[2] << 16) | (resp[3] << 24);
  return (value != 0xFFFFFFFF);
}

bool PN5180::read_eeprom(uint8_t addr, uint8_t &value) {
  uint8_t cmd[3] = { PN5180_CMD_READ_EEPROM, addr, 0x01 };
  return this->transceive(cmd, 3, &value, 1);
}

bool PN5180::load_rf_config(uint8_t tx_config, uint8_t rx_config) {
  uint8_t cmd[3] = { PN5180_CMD_LOAD_RF_CONFIG, tx_config, rx_config };
  return this->transceive(cmd, 3);
}

bool PN5180::rf_on() {
  uint8_t cmd[2] = { PN5180_CMD_RF_ON, 0x00 };
  if (!this->transceive(cmd, 2)) return false;
  uint32_t irq;
  uint32_t start = millis();
  while (millis() - start < 100) {
    if (this->read_register(PN5180_REG_IRQ_STATUS, irq)) {
      if (irq & PN5180_IRQ_RF_ON_DET) break;
    }
    delay(1);
  }
  this->write_register(PN5180_REG_IRQ_CLEAR, PN5180_IRQ_RF_ON_DET);
  return true;
}

bool PN5180::rf_off() {
  uint8_t cmd[2] = { PN5180_CMD_RF_OFF, 0x00 };
  if (!this->transceive(cmd, 2)) return false;
  uint32_t irq;
  uint32_t start = millis();
  while (millis() - start < 100) {
    if (this->read_register(PN5180_REG_IRQ_STATUS, irq)) {
      if (irq & PN5180_IRQ_RF_OFF_DET) break;
    }
    delay(1);
  }
  this->write_register(PN5180_REG_IRQ_CLEAR, PN5180_IRQ_RF_OFF_DET);
  return true;
}

bool PN5180::send_data(const uint8_t *data, uint8_t len, uint8_t valid_bits) {
  this->write_register_and_mask(PN5180_REG_SYSTEM_CONFIG, 0xfffffff8);
  this->write_register_or_mask(PN5180_REG_SYSTEM_CONFIG, 0x00000003);
  uint8_t buf[len + 2];
  buf[0] = PN5180_CMD_SEND_DATA;
  buf[1] = valid_bits;
  memcpy(buf + 2, data, len);
  return this->transceive(buf, len + 2);
}

bool PN5180::read_data(uint8_t *data, uint8_t &len) {
  uint32_t rx_status;
  if (!this->read_register(PN5180_REG_RX_STATUS, rx_status)) return false;
  uint16_t rx_len = rx_status & 0x1FF;
  if (rx_len == 0 || rx_len == 0x1FF) { len = 0; return false; }
  uint8_t cmd[2] = { PN5180_CMD_READ_DATA, 0x00 };
  if (!this->transceive(cmd, 2, data, rx_len)) return false;
  len = rx_len;
  return true;
}

bool PN5180::read_version() {
  uint8_t v1, v2, v3;
  if (!this->read_eeprom(0x12, v1) || !this->read_eeprom(0x13, v2) || !this->read_eeprom(0x14, v3)) return false;
  ESP_LOGI(TAG, "PN5180 Version: %02X %02X %02X", v1, v2, v3);
  return (v1 != 0xFF && v1 != 0x00);
}

bool PN5180::configure_iso14443a() {
  return true;
}

bool PN5180::inventory_iso14443a(std::vector<uint8_t> &uid) {
  uint8_t reqa = 0x26;
  if (!this->send_data(&reqa, 1, 0x07)) return false;
  uint32_t irq = 0, start = millis();
  while (millis() - start < 50) {
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
  uint8_t anticoll[] = {0x93, 0x20};
  this->send_data(anticoll, 2, 0); 
  start = millis();
  while (millis() - start < 50) {
    if (this->read_register(PN5180_REG_IRQ_STATUS, irq)) {
      if (irq & (PN5180_IRQ_RX | PN5180_IRQ_GENERAL_ERROR)) break;
    }
    delay(1);
  }
  if (!(irq & PN5180_IRQ_RX)) return false;
  uint8_t uid_data[10], uid_len = 10;
  if (this->read_data(uid_data, uid_len) && uid_len >= 5) {
    uid.assign(uid_data, uid_data + 4);
    return true;
  }
  return false;
}

bool PN5180::process_tag_scan() {
  for (uint8_t i = 0; i < 0x08; i++) { // Brute-force first 8 protocols
    this->reset();
    if (!this->load_rf_config(i, i | 0x80)) continue;
    if (!this->rf_on()) continue;
    delay(20);
    std::vector<uint8_t> uid;
    if (this->inventory_iso14443a(uid)) {
      std::string uid_str = this->format_uid(uid);
      if (this->current_tags_.find(uid_str) == this->current_tags_.end()) {
        ESP_LOGI(TAG, "New tag (Idx %02X): %s", i, uid_str.c_str());
        for (auto *trig : this->triggers_on_tag_) trig->trigger(uid_str);
      }
      this->current_tags_[uid_str] = millis();
      this->rf_off();
      return true;
    }
    this->rf_off();
  }
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

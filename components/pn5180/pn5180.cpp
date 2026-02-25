#include "pn5180.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace pn5180 {

static const char *const TAG = "pn5180";

void PN5180::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PN5180...");
  
  // Initialize pins
  this->cs_pin_->setup();
  this->cs_pin_->digital_write(true);
  
  this->busy_pin_->setup();
  
  this->rst_pin_->setup();
  this->rst_pin_->digital_write(true);
  
  this->spi_setup();
  
  // Reset the PN5180
  if (!this->reset()) {
    ESP_LOGE(TAG, "Failed to reset PN5180");
    this->mark_failed();
    return;
  }
  
  delay(10);
  
  // Read version to verify communication
  if (!this->read_version()) {
    ESP_LOGE(TAG, "Failed to read PN5180 version");
    this->mark_failed();
    return;
  }
  
  // Turn off RF field initially
  this->rf_off();
  
  this->state_ = STATE_READY;
  ESP_LOGCONFIG(TAG, "PN5180 setup complete");
}

void PN5180::loop() {
  // Nothing to do in loop for now
}

void PN5180::update() {
  if (this->state_ != STATE_READY) {
    return;
  }
  
  // Check for expired tags
  uint32_t now = millis();
  auto it = this->current_tags_.begin();
  while (it != this->current_tags_.end()) {
    if (now - it->second > this->tag_ttl_) {
      ESP_LOGD(TAG, "Tag removed: %s", it->first.c_str());
      for (auto *trigger : this->triggers_on_tag_removed_) {
        trigger->trigger(it->first);
      }
      it = this->current_tags_.erase(it);
    } else {
      ++it;
    }
  }
  
  // Scan for new tags
  this->process_tag_scan();
}

void PN5180::dump_config() {
  ESP_LOGCONFIG(TAG, "PN5180:");
  LOG_PIN("  CS Pin: ", this->cs_pin_);
  LOG_PIN("  BUSY Pin: ", this->busy_pin_);
  LOG_PIN("  RST Pin: ", this->rst_pin_);
  ESP_LOGCONFIG(TAG, "  Tag TTL: %u ms", this->tag_ttl_);
  LOG_UPDATE_INTERVAL(this);
}

bool PN5180::reset() {
  ESP_LOGD(TAG, "Resetting PN5180...");
  
  // Pull reset low
  this->rst_pin_->digital_write(false);
  delay(10);
  
  // Pull reset high
  this->rst_pin_->digital_write(true);
  delay(10);
  
  // Wait for device to be ready
  return this->wait_busy();
}

bool PN5180::wait_busy() {
  // Wait for BUSY line to go low (ready)
  uint32_t start = millis();
  while (this->busy_pin_->digital_read()) {
    if (millis() - start > 100) {
      ESP_LOGW(TAG, "Timeout waiting for BUSY");
      return false;
    }
    delay(1);
  }
  return true;
}

bool PN5180::write_register(uint32_t reg, uint32_t value) {
  if (!this->wait_busy()) {
    return false;
  }
  
  this->enable();
  this->write_byte(PN5180_CMD_WRITE_REGISTER);
  this->write_byte(reg & 0xFF);
  this->write_byte((reg >> 8) & 0xFF);
  this->write_byte((reg >> 16) & 0xFF);
  this->write_byte((reg >> 24) & 0xFF);
  this->write_byte(value & 0xFF);
  this->write_byte((value >> 8) & 0xFF);
  this->write_byte((value >> 16) & 0xFF);
  this->write_byte((value >> 24) & 0xFF);
  this->disable();
  
  return true;
}

bool PN5180::read_register(uint32_t reg, uint32_t &value) {
  if (!this->wait_busy()) {
    return false;
  }
  
  // Send read command
  this->enable();
  this->write_byte(PN5180_CMD_READ_REGISTER);
  this->write_byte(reg & 0xFF);
  this->write_byte((reg >> 8) & 0xFF);
  this->write_byte((reg >> 16) & 0xFF);
  this->write_byte((reg >> 24) & 0xFF);
  this->disable();
  
  if (!this->wait_busy()) {
    return false;
  }
  
  // Read response
  this->enable();
  uint8_t b0 = this->read_byte();
  uint8_t b1 = this->read_byte();
  uint8_t b2 = this->read_byte();
  uint8_t b3 = this->read_byte();
  this->disable();
  
  value = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
  
  return true;
}

bool PN5180::read_eeprom(uint8_t addr, uint8_t &value) {
  if (!this->wait_busy()) {
    return false;
  }
  
  // Send read command
  this->enable();
  this->write_byte(PN5180_CMD_READ_EEPROM);
  this->write_byte(addr);
  this->write_byte(0x01);  // Read 1 byte
  this->disable();
  
  if (!this->wait_busy()) {
    return false;
  }
  
  // Read response
  this->enable();
  value = this->read_byte();
  this->disable();
  
  return true;
}

bool PN5180::load_rf_config(uint8_t tx_config, uint8_t rx_config) {
  if (!this->wait_busy()) {
    return false;
  }
  
  this->enable();
  this->write_byte(PN5180_CMD_LOAD_RF_CONFIG);
  this->write_byte(tx_config);
  this->write_byte(rx_config);
  this->disable();
  
  return this->wait_busy();
}

bool PN5180::rf_on() {
  if (!this->wait_busy()) {
    return false;
  }
  
  this->enable();
  this->write_byte(PN5180_CMD_RF_ON);
  this->disable();
  
  delay(5);
  return true;
}

bool PN5180::rf_off() {
  if (!this->wait_busy()) {
    return false;
  }
  
  this->enable();
  this->write_byte(PN5180_CMD_RF_OFF);
  this->disable();
  
  delay(5);
  return true;
}

bool PN5180::send_data(const uint8_t *data, uint8_t len) {
  if (!this->wait_busy()) {
    return false;
  }
  
  this->enable();
  this->write_byte(PN5180_CMD_SEND_DATA);
  this->write_byte(len);
  this->write_array(data, len);
  this->disable();
  
  return true;
}

bool PN5180::read_data(uint8_t *data, uint8_t &len) {
  if (!this->wait_busy()) {
    return false;
  }
  
  // Check RX status
  uint32_t rx_status;
  if (!this->read_register(PN5180_REG_RX_STATUS, rx_status)) {
    return false;
  }
  
  uint8_t rx_len = rx_status & 0x1FF;  // Lower 9 bits
  if (rx_len == 0) {
    len = 0;
    return true;
  }
  
  // Read data
  this->enable();
  this->write_byte(PN5180_CMD_READ_DATA);
  this->write_byte(0x00);  // Start address
  this->disable();
  
  if (!this->wait_busy()) {
    return false;
  }
  
  this->enable();
  for (uint8_t i = 0; i < rx_len && i < len; i++) {
    data[i] = this->read_byte();
  }
  this->disable();
  
  len = rx_len;
  return true;
}

bool PN5180::read_version() {
  uint8_t product_version = 0;
  uint8_t firmware_version = 0;
  uint8_t eeprom_version = 0;
  
  // Read EEPROM version info
  if (!this->read_eeprom(0x12, product_version)) {
    ESP_LOGE(TAG, "Failed to read product version");
    return false;
  }
  
  if (!this->read_eeprom(0x13, firmware_version)) {
    ESP_LOGE(TAG, "Failed to read firmware version");
    return false;
  }
  
  if (!this->read_eeprom(0x14, eeprom_version)) {
    ESP_LOGE(TAG, "Failed to read EEPROM version");
    return false;
  }
  
  ESP_LOGI(TAG, "PN5180 Version - Product: 0x%02X, Firmware: 0x%02X, EEPROM: 0x%02X",
           product_version, firmware_version, eeprom_version);
  
  return true;
}

bool PN5180::configure_iso14443a() {
  // Load ISO14443A RF configuration
  if (!this->load_rf_config(PN5180_PROTOCOL_ISO14443A, PN5180_PROTOCOL_ISO14443A)) {
    ESP_LOGE(TAG, "Failed to load ISO14443A configuration");
    return false;
  }
  
  // Turn on RF field
  if (!this->rf_on()) {
    ESP_LOGE(TAG, "Failed to turn on RF field");
    return false;
  }
  
  delay(10);
  return true;
}

bool PN5180::inventory_iso14443a(std::vector<uint8_t> &uid) {
  // Clear any previous interrupts
  this->write_register(PN5180_REG_IRQ_CLEAR, 0xFFFFFFFF);
  
  // Send REQA (Request Type A)
  uint8_t reqa[] = {0x26};
  if (!this->send_data(reqa, sizeof(reqa))) {
    return false;
  }
  
  delay(5);
  
  // Check for response
  uint32_t irq_status;
  if (!this->read_register(PN5180_REG_IRQ_STATUS, irq_status)) {
    return false;
  }
  
  if (!(irq_status & PN5180_IRQ_RX)) {
    // No card responded
    return false;
  }
  
  // Read ATQA
  uint8_t atqa[2];
  uint8_t atqa_len = sizeof(atqa);
  if (!this->read_data(atqa, atqa_len)) {
    return false;
  }
  
  if (atqa_len != 2) {
    ESP_LOGD(TAG, "Invalid ATQA length: %d", atqa_len);
    return false;
  }
  
  // Clear interrupts
  this->write_register(PN5180_REG_IRQ_CLEAR, 0xFFFFFFFF);
  
  // Send anticollision command
  uint8_t anticoll[] = {0x93, 0x20};
  if (!this->send_data(anticoll, sizeof(anticoll))) {
    return false;
  }
  
  delay(5);
  
  // Check for response
  if (!this->read_register(PN5180_REG_IRQ_STATUS, irq_status)) {
    return false;
  }
  
  if (!(irq_status & PN5180_IRQ_RX)) {
    return false;
  }
  
  // Read UID
  uint8_t uid_data[5];
  uint8_t uid_len = sizeof(uid_data);
  if (!this->read_data(uid_data, uid_len)) {
    return false;
  }
  
  if (uid_len >= 4) {
    uid.assign(uid_data, uid_data + 4);
    return true;
  }
  
  return false;
}

bool PN5180::process_tag_scan() {
  // Configure for ISO14443A
  if (!this->configure_iso14443a()) {
    return false;
  }
  
  // Try to read a tag
  std::vector<uint8_t> uid;
  if (this->inventory_iso14443a(uid)) {
    std::string uid_str = this->format_uid(uid);
    
    // Check if this is a new tag
    if (this->current_tags_.find(uid_str) == this->current_tags_.end()) {
      ESP_LOGI(TAG, "New tag detected: %s", uid_str.c_str());
      this->last_tag_uid_ = uid_str;
      
      // Trigger callbacks
      for (auto *trigger : this->triggers_on_tag_) {
        trigger->trigger(uid_str);
      }
    }
    
    // Update tag timestamp
    this->current_tags_[uid_str] = millis();
  }
  
  // Turn off RF field
  this->rf_off();
  
  return true;
}

std::string PN5180::format_uid(const std::vector<uint8_t> &uid) {
  std::string result;
  for (size_t i = 0; i < uid.size(); i++) {
    if (i > 0) {
      result += "-";
    }
    char buf[3];
    snprintf(buf, sizeof(buf), "%02X", uid[i]);
    result += buf;
  }
  return result;
}

}  // namespace pn5180
}  // namespace esphome

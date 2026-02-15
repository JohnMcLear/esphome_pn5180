/**
 * Unit tests for PN5180 ESPHome Component
 * 
 * Build with:
 *   g++ -std=c++11 -I/path/to/esphome -lgtest -lgtest_main -pthread test_pn5180.cpp
 * 
 * Run with:
 *   ./test_pn5180
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <cstring>

// Mock the ESPHome components we need
namespace esphome {

class Component {
 public:
  virtual void setup() {}
  virtual void loop() {}
};

class PollingComponent : public Component {
 public:
  PollingComponent(uint32_t update_interval) : update_interval_(update_interval) {}
  virtual void update() = 0;
 protected:
  uint32_t update_interval_;
};

namespace text_sensor {
class TextSensor {
 public:
  void publish_state(const std::string &state) { state_ = state; }
  std::string get_state() const { return state_; }
 private:
  std::string state_;
};
}  // namespace text_sensor

class InternalGPIOPin {
 public:
  InternalGPIOPin(uint8_t pin) : pin_(pin) {}
  uint8_t get_pin() const { return pin_; }
 private:
  uint8_t pin_;
};

template<typename... Ts>
class CallbackManager {
 public:
  void add(std::function<void(Ts...)> &&callback) {
    callbacks_.push_back(std::move(callback));
  }
  void call(Ts... args) {
    for (auto &callback : callbacks_) {
      callback(args...);
    }
  }
 private:
  std::vector<std::function<void(Ts...)>> callbacks_;
};

}  // namespace esphome

// Mock PN5180 library
enum ISO15693ErrorCode {
  ISO15693_EC_OK = 0,
  ISO15693_EC_NO_TAG = 1,
  ISO15693_EC_ERROR = 2,
};

class PN5180ISO15693 {
 public:
  PN5180ISO15693(uint8_t cs, uint8_t busy, uint8_t rst) 
    : cs_(cs), busy_(busy), rst_(rst), initialized_(false) {}
  
  void begin() { initialized_ = true; }
  void reset() {}
  void setupRF() {}
  
  ISO15693ErrorCode getInventory(uint8_t *uid) {
    if (!initialized_) return ISO15693_EC_ERROR;
    if (mock_tag_present_) {
      memcpy(uid, mock_uid_, 8);
      return ISO15693_EC_OK;
    }
    return ISO15693_EC_NO_TAG;
  }
  
  // Test helpers
  void set_mock_tag(const uint8_t *uid) {
    memcpy(mock_uid_, uid, 8);
    mock_tag_present_ = true;
  }
  
  void clear_mock_tag() {
    mock_tag_present_ = false;
  }
  
 private:
  uint8_t cs_, busy_, rst_;
  bool initialized_;
  bool mock_tag_present_ = false;
  uint8_t mock_uid_[8] = {0};
};

// Include the component (would normally be a header include)
namespace esphome {
namespace pn5180 {

static constexpr uint8_t UID_SIZE = 8;
static constexpr size_t UID_BUFFER_SIZE = 32;

class PN5180Component : public PollingComponent,
                        public text_sensor::TextSensor {
 public:
  PN5180Component(InternalGPIOPin *cs, InternalGPIOPin *busy,
                  InternalGPIOPin *rst, uint32_t update_interval)
      : PollingComponent(update_interval), cs_(cs), busy_(busy), rst_(rst) {}

  ~PN5180Component() { delete pn5180_; }

  void setup() override {
    pn5180_ = new PN5180ISO15693(cs_->get_pin(), busy_->get_pin(), rst_->get_pin());
    pn5180_->begin();
    pn5180_->reset();
    pn5180_->setupRF();
  }

  void update() override {
    if (pn5180_ == nullptr) return;

    uint8_t uid[UID_SIZE];
    ISO15693ErrorCode rc = pn5180_->getInventory(uid);

    if (rc == ISO15693_EC_OK) {
      char buffer[UID_BUFFER_SIZE];
      if (!format_uid(uid, buffer, sizeof(buffer))) return;

      std::string tag_id(buffer);
      publish_state(tag_id);

      if (tag_id != last_tag_id_) {
        on_tag_callbacks_.call(tag_id);
        last_tag_id_ = tag_id;
      }
      no_tag_reported_ = false;
    } else {
      if (!no_tag_reported_) {
        publish_state("");
        last_tag_id_ = "";
        no_tag_reported_ = true;
      }
    }
  }

  void add_on_tag_callback(std::function<void(const std::string &)> &&callback) {
    on_tag_callbacks_.add(std::move(callback));
  }

  // Test helpers
  PN5180ISO15693 *get_pn5180() { return pn5180_; }
  std::string get_last_tag_id() const { return last_tag_id_; }

 protected:
  bool format_uid(const uint8_t *uid, char *buffer, size_t buffer_size) {
    if (buffer_size < UID_BUFFER_SIZE) return false;

    size_t pos = 0;
    for (uint8_t i = 0; i < UID_SIZE; i++) {
      int written = snprintf(buffer + pos, buffer_size - pos, "%02X%s",
                            uid[i], (i < UID_SIZE - 1) ? " " : "");
      if (written < 0 || pos + written >= buffer_size) return false;
      pos += written;
    }
    buffer[buffer_size - 1] = '\0';
    return true;
  }

  InternalGPIOPin *cs_;
  InternalGPIOPin *busy_;
  InternalGPIOPin *rst_;
  PN5180ISO15693 *pn5180_{nullptr};
  std::string last_tag_id_{""};
  bool no_tag_reported_{false};
  CallbackManager<void(const std::string &)> on_tag_callbacks_;
};

}  // namespace pn5180
}  // namespace esphome

// ============================================================================
// UNIT TESTS
// ============================================================================

using namespace esphome::pn5180;

class PN5180ComponentTest : public ::testing::Test {
 protected:
  void SetUp() override {
    cs_ = new esphome::InternalGPIOPin(16);
    busy_ = new esphome::InternalGPIOPin(5);
    rst_ = new esphome::InternalGPIOPin(17);
    
    component_ = new PN5180Component(cs_, busy_, rst_, 500);
    component_->setup();
  }

  void TearDown() override {
    delete component_;
    delete cs_;
    delete busy_;
    delete rst_;
  }

  esphome::InternalGPIOPin *cs_;
  esphome::InternalGPIOPin *busy_;
  esphome::InternalGPIOPin *rst_;
  PN5180Component *component_;
};

// Test initialization
TEST_F(PN5180ComponentTest, InitializesSuccessfully) {
  ASSERT_NE(component_->get_pn5180(), nullptr);
}

// Test UID formatting
TEST_F(PN5180ComponentTest, FormatsUIDCorrectly) {
  uint8_t test_uid[] = {0xE0, 0x07, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
  char buffer[32];
  
  bool result = component_->format_uid(test_uid, buffer, sizeof(buffer));
  
  ASSERT_TRUE(result);
  EXPECT_STREQ(buffer, "E0 07 01 23 45 67 89 AB");
}

// Test UID formatting with all zeros
TEST_F(PN5180ComponentTest, FormatsZeroUID) {
  uint8_t test_uid[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  char buffer[32];
  
  bool result = component_->format_uid(test_uid, buffer, sizeof(buffer));
  
  ASSERT_TRUE(result);
  EXPECT_STREQ(buffer, "00 00 00 00 00 00 00 00");
}

// Test UID formatting with all FFs
TEST_F(PN5180ComponentTest, FormatsMaxUID) {
  uint8_t test_uid[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  char buffer[32];
  
  bool result = component_->format_uid(test_uid, buffer, sizeof(buffer));
  
  ASSERT_TRUE(result);
  EXPECT_STREQ(buffer, "FF FF FF FF FF FF FF FF");
}

// Test buffer too small
TEST_F(PN5180ComponentTest, HandlesSmallBuffer) {
  uint8_t test_uid[] = {0xE0, 0x07, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
  char buffer[10];  // Too small
  
  bool result = component_->format_uid(test_uid, buffer, sizeof(buffer));
  
  EXPECT_FALSE(result);
}

// Test tag detection
TEST_F(PN5180ComponentTest, DetectsTag) {
  uint8_t test_uid[] = {0xE0, 0x07, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
  component_->get_pn5180()->set_mock_tag(test_uid);
  
  component_->update();
  
  EXPECT_EQ(component_->get_state(), "E0 07 01 23 45 67 89 AB");
}

// Test no tag detection
TEST_F(PN5180ComponentTest, HandlesNoTag) {
  component_->get_pn5180()->clear_mock_tag();
  
  component_->update();
  
  EXPECT_EQ(component_->get_state(), "");
}

// Test tag change detection
TEST_F(PN5180ComponentTest, DetectsTagChange) {
  uint8_t tag1[] = {0xE0, 0x07, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
  uint8_t tag2[] = {0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x00, 0x11};
  
  // First tag
  component_->get_pn5180()->set_mock_tag(tag1);
  component_->update();
  EXPECT_EQ(component_->get_state(), "E0 07 01 23 45 67 89 AB");
  
  // Second tag
  component_->get_pn5180()->set_mock_tag(tag2);
  component_->update();
  EXPECT_EQ(component_->get_state(), "A0 B0 C0 D0 E0 F0 00 11");
}

// Test callback firing
TEST_F(PN5180ComponentTest, FiresCallback) {
  bool callback_fired = false;
  std::string received_tag_id;
  
  component_->add_on_tag_callback([&](const std::string &tag_id) {
    callback_fired = true;
    received_tag_id = tag_id;
  });
  
  uint8_t test_uid[] = {0xE0, 0x07, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
  component_->get_pn5180()->set_mock_tag(test_uid);
  component_->update();
  
  EXPECT_TRUE(callback_fired);
  EXPECT_EQ(received_tag_id, "E0 07 01 23 45 67 89 AB");
}

// Test callback only fires on tag change
TEST_F(PN5180ComponentTest, CallbackOnlyFiresOnChange) {
  int callback_count = 0;
  
  component_->add_on_tag_callback([&](const std::string &tag_id) {
    callback_count++;
  });
  
  uint8_t test_uid[] = {0xE0, 0x07, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
  component_->get_pn5180()->set_mock_tag(test_uid);
  
  // First detection - should fire
  component_->update();
  EXPECT_EQ(callback_count, 1);
  
  // Same tag - should NOT fire
  component_->update();
  EXPECT_EQ(callback_count, 1);
  
  // Tag removed - should NOT fire
  component_->get_pn5180()->clear_mock_tag();
  component_->update();
  EXPECT_EQ(callback_count, 1);
  
  // New tag - should fire
  uint8_t tag2[] = {0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x00, 0x11};
  component_->get_pn5180()->set_mock_tag(tag2);
  component_->update();
  EXPECT_EQ(callback_count, 2);
}

// Test multiple callbacks
TEST_F(PN5180ComponentTest, SupportsMultipleCallbacks) {
  int callback1_count = 0;
  int callback2_count = 0;
  
  component_->add_on_tag_callback([&](const std::string &) { callback1_count++; });
  component_->add_on_tag_callback([&](const std::string &) { callback2_count++; });
  
  uint8_t test_uid[] = {0xE0, 0x07, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
  component_->get_pn5180()->set_mock_tag(test_uid);
  component_->update();
  
  EXPECT_EQ(callback1_count, 1);
  EXPECT_EQ(callback2_count, 1);
}

// Performance test - formatting speed
TEST_F(PN5180ComponentTest, FormattingPerformance) {
  uint8_t test_uid[] = {0xE0, 0x07, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB};
  char buffer[32];
  
  const int iterations = 10000;
  auto start = std::chrono::high_resolution_clock::now();
  
  for (int i = 0; i < iterations; i++) {
    component_->format_uid(test_uid, buffer, sizeof(buffer));
  }
  
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  
  double avg_time = duration.count() / double(iterations);
  std::cout << "Average UID formatting time: " << avg_time << " microseconds" << std::endl;
  
  // Should be fast - under 10 microseconds average
  EXPECT_LT(avg_time, 10.0);
}

// Main test runner
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

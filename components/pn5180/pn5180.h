#pragma once
#include "esphome.h"

namespace esphome {
namespace pn5180 {

class PN5180Component : public PollingComponent, public text_sensor::TextSensor {
 public:
  PN5180Component(GPIOPin *cs, GPIOPin *busy, GPIOPin *rst, uint32_t update_interval);

  void setup() override;
  void update() override;

 protected:
  GPIOPin *cs_;
  GPIOPin *busy_;
  GPIOPin *rst_;
};

}  // namespace pn5180
}  // namespace esphome

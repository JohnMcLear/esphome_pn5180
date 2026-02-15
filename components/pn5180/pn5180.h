#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include <PN5180ISO15693.h>

namespace esphome {
namespace pn5180 {

class PN5180Component : public PollingComponent,
                        public text_sensor::TextSensor {
 public:
  PN5180Component(
      InternalGPIOPin *cs,
      InternalGPIOPin *busy,
      InternalGPIOPin *rst,
      uint32_t update_interval);

  void setup() override;
  void update() override;

 protected:
  InternalGPIOPin *cs_;
  InternalGPIOPin *busy_;
  InternalGPIOPin *rst_;

  PN5180ISO15693 *pn5180_;
};

}  // namespace pn5180
}  // namespace esphome


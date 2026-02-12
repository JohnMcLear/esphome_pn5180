#pragma once

#include "esphome.h"
#include "PN5180.h"
#include "PN5180ISO15693.h"

using namespace esphome;

class PN5180Component : public PollingComponent, public text_sensor::TextSensor {
 public:
  PN5180Component(InternalGPIOPin *cs_pin, InternalGPIOPin *busy_pin, InternalGPIOPin *rst_pin, uint32_t update_interval)
      : PollingComponent(update_interval),
        cs_pin_(cs_pin),
        busy_pin_(busy_pin),
        rst_pin_(rst_pin),
        pn5180_(cs_pin->get_pin(), busy_pin->get_pin(), rst_pin->get_pin()) {}

  void setup() override;
  void update() override;

 protected:
  InternalGPIOPin *cs_pin_;
  InternalGPIOPin *busy_pin_;
  InternalGPIOPin *rst_pin_;
  PN5180ISO15693 pn5180_;
};


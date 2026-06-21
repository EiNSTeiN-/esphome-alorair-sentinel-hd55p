#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

namespace esphome {
namespace alorair_hd55p {

class AlorairHD55PClimate : public climate::Climate, public Component {
 public:
  void setup() override;
  void dump_config() override;

  climate::ClimateTraits traits() override;

  void update_from_can(float humidity, float setpoint, float temperature, bool dehumidifying);

  Trigger<> *get_turn_on_trigger() { return &this->turn_on_trigger_; }
  Trigger<> *get_turn_off_trigger() { return &this->turn_off_trigger_; }
  Trigger<float> *get_target_humidity_change_trigger() { return &this->target_humidity_change_trigger_; }

 protected:
  void control(const climate::ClimateCall &call) override;

  bool apply_action_();

  bool dehumidifying_{false};
  Trigger<> turn_on_trigger_;
  Trigger<> turn_off_trigger_;
  Trigger<float> target_humidity_change_trigger_;
};

}  // namespace alorair_hd55p
}  // namespace esphome

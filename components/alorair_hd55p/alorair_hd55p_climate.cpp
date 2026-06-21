#include "alorair_hd55p_climate.h"

#include <cmath>

#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace alorair_hd55p {

static const char *const TAG = "alorair_hd55p.climate";

void AlorairHD55PClimate::setup() {
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->mode = climate::CLIMATE_MODE_DRY;
    this->target_humidity = 50;
  }
  this->target_temperature = NAN;
  if (std::isnan(this->target_humidity)) {
    this->target_humidity = 50;
  }
  this->apply_action_();
  this->publish_state();
}

void AlorairHD55PClimate::dump_config() { LOG_CLIMATE("", "AlorAir HD55P Climate", this); }

climate::ClimateTraits AlorairHD55PClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_ACTION);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_HUMIDITY);
  traits.add_feature_flags(climate::CLIMATE_SUPPORTS_TARGET_HUMIDITY);
  traits.set_visual_min_temperature(0);
  traits.set_visual_max_temperature(40);
  traits.set_visual_temperature_step(1);
  traits.set_visual_min_humidity(35);
  traits.set_visual_max_humidity(99);
  return traits;
}

void AlorairHD55PClimate::control(const climate::ClimateCall &call) {
  bool publish = false;

  if (call.get_mode().has_value()) {
    const auto mode = *call.get_mode();
    if (mode == climate::CLIMATE_MODE_OFF) {
      publish |= this->mode != climate::CLIMATE_MODE_OFF;
      this->mode = mode;
      this->turn_off_trigger_.trigger();
    } else if (mode == climate::CLIMATE_MODE_DRY) {
      publish |= this->mode != climate::CLIMATE_MODE_DRY;
      this->mode = mode;
      this->turn_on_trigger_.trigger();
    }
  }

  if (call.get_target_humidity().has_value()) {
    const float requested = clamp(*call.get_target_humidity(), 35.0f, 99.0f);
    if (std::isnan(this->target_humidity) || this->target_humidity != requested) {
      this->target_humidity = requested;
      this->target_humidity_change_trigger_.trigger(requested);
      publish = true;
    }
  }

  publish |= this->apply_action_();
  if (publish) {
    this->publish_state();
  }
}

void AlorairHD55PClimate::update_from_can(float humidity, float setpoint, float temperature, bool dehumidifying) {
  bool publish = false;

  if (std::isnan(this->current_humidity) || this->current_humidity != humidity) {
    this->current_humidity = humidity;
    publish = true;
  }
  if (std::isnan(this->target_humidity) || this->target_humidity != setpoint) {
    this->target_humidity = setpoint;
    publish = true;
  }
  if (std::isnan(this->current_temperature) || this->current_temperature != temperature) {
    this->current_temperature = temperature;
    publish = true;
  }
  if (this->dehumidifying_ != dehumidifying) {
    this->dehumidifying_ = dehumidifying;
    publish = true;
  }

  publish |= this->apply_action_();
  if (publish) {
    this->publish_state();
  }
}

bool AlorairHD55PClimate::apply_action_() {
  const auto next_action = this->mode == climate::CLIMATE_MODE_OFF
                               ? climate::CLIMATE_ACTION_OFF
                               : (this->dehumidifying_ ? climate::CLIMATE_ACTION_DRYING : climate::CLIMATE_ACTION_IDLE);
  if (this->action == next_action) {
    return false;
  }
  this->action = next_action;
  return true;
}

}  // namespace alorair_hd55p
}  // namespace esphome

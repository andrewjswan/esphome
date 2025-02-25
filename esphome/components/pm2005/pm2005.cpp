#include "esphome/core/log.h"
#include "pm2005.h"

namespace esphome {
namespace pm2005 {

static const char *const TAG = "pm2005";

#ifdef PM2005_USE_TYPE_2005
static const uint8_t SITUATION_VALUE_INDEX = 3;
static const uint8_t PM_1_0_VALUE_INDEX = 4;
static const uint8_t PM_2_5_VALUE_INDEX = 6;
static const uint8_t PM_10_0_VALUE_INDEX = 8;
static const uint8_t MEASURING_VALUE_INDEX = 10;
#else
static const uint8_t SITUATION_VALUE_INDEX = 2;
static const uint8_t PM_1_0_VALUE_INDEX = 3;
static const uint8_t PM_2_5_VALUE_INDEX = 5;
static const uint8_t PM_10_0_VALUE_INDEX = 7;
static const uint8_t MEASURING_VALUE_INDEX = 9;
#endif

// Converts a sensor situation to a human readable string
static const LogString *pm2005_get_situation_string(int status) {
  switch (status) {
    case 1:
      return LOG_STR("Close");
    case 2:
      return LOG_STR("Malfunction");
    case 3:
      return LOG_STR("Under detecting");
    case 0x80:
      return LOG_STR("Detecting completed");
    default:
      return LOG_STR("Invalid");
  }
}

// Converts a sensor measuring mode to a human readable string
static const LogString *pm2005_get_measuring_mode_string(int status) {
  switch (status) {
    case 2:
      return LOG_STR("Single measuring mode");
    case 3:
      return LOG_STR("Continuous measuring mode");
    case 5:
      return LOG_STR("Dynamic measuring mode");
    default:
      return LOG_STR("Timing measuring mode");
  }
}

void PM2005Component::setup() {
#ifdef PM2005_USE_TYPE_2005
  ESP_LOGCONFIG(TAG, "Setting up PM2005...");
#else
  ESP_LOGCONFIG(TAG, "Setting up PM2105...");
#endif
  if (this->read(data_buffer_, 12) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Read result failed");
    this->mark_failed();
    return;
  }
}

void PM2005Component::update() {
  if (this->read(data_buffer_, 12) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "Read result failed");
    this->status_set_warning();
    return;
  }

  if (this->sensor_situation_ == data_buffer_[SITUATION_VALUE_INDEX]) {
    return;
  }

  this->sensor_situation_ = data_buffer_[SITUATION_VALUE_INDEX];
  ESP_LOGD(TAG, "Sensor situation: %s.", LOG_STR_ARG(pm2005_get_situation_string(this->sensor_situation_)));
  if (this->sensor_situation_ == 2) {
    this->status_set_warning();
    return;
  }
  if (this->sensor_situation_ != 0x80) {
    return;
  }

  if (this->pm_1_0_sensor_ != nullptr) {
    int16_t pm1 = get_sensor_value_(data_buffer_, PM_1_0_VALUE_INDEX);
    ESP_LOGD(TAG, "PM1.0: %d", pm1);
    this->pm_1_0_sensor_->publish_state(pm1);
  }

  if (this->pm_2_5_sensor_ != nullptr) {
    int16_t pm25 = get_sensor_value_(data_buffer_, PM_2_5_VALUE_INDEX);
    ESP_LOGD(TAG, "PM2.5: %d", pm25);
    this->pm_2_5_sensor_->publish_state(pm25);
  }

  if (this->pm_10_0_sensor_ != nullptr) {
    int16_t pm10 = get_sensor_value_(data_buffer_, PM_10_0_VALUE_INDEX);
    ESP_LOGD(TAG, "PM10: %d", pm10);
    this->pm_10_0_sensor_->publish_state(pm10);
  }

  uint16_t sensor_measuring_mode = get_sensor_value_(data_buffer_, MEASURING_VALUE_INDEX);
  ESP_LOGD(TAG, "The measuring mode of sensor: %s.",
           LOG_STR_ARG(pm2005_get_measuring_mode_string(sensor_measuring_mode)));

  this->status_clear_warning();
}

uint16_t PM2005Component::get_sensor_value_(const uint8_t *data, uint8_t i) {
  return data_buffer_[i] * 0x100 + data_buffer_[i + 1];
}

void PM2005Component::dump_config() {
  ESP_LOGCONFIG(TAG, "PM2005:");

#ifdef PM2005_USE_TYPE_2005
  ESP_LOGCONFIG(TAG, "  Type: PM2005");
#else
  ESP_LOGCONFIG(TAG, "  Type: PM2105");
#endif

  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
#ifdef PM2005_USE_TYPE_2005
    ESP_LOGE(TAG, "Communication with PM2005 failed!");
#else
    ESP_LOGE(TAG, "Communication with PM2105 failed!");
#endif
  }

  LOG_SENSOR("  ", "PM1.0", this->pm_1_0_sensor_);
  LOG_SENSOR("  ", "PM2.5", this->pm_2_5_sensor_);
  LOG_SENSOR("  ", "PM10 ", this->pm_10_0_sensor_);
}

}  // namespace pm2005
}  // namespace esphome

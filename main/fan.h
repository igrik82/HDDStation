#pragma once

#include "driver/ledc.h"
#include "esp_event.h"
#include "esp_log.h" // IWYU pragma: keep
#include "mqtt.h"
#include <cfloat>
#include <cstdint>

namespace Fan_NS {

// Constants
constexpr uint8_t MIN_TEMP_HDD { 25 };
constexpr uint8_t MAX_TEMP_HDD { 40 };
constexpr uint32_t LOW_SPEED_MODE_TIMER = 8000;

constexpr uint32_t _freq_hz { 1000 }; // LOW MODE - 100 Hz - 1000 Hz
static constexpr uint8_t NUM_MEAS = 6; // Number of measurements

class FanPWM {

protected:
    ledc_mode_t _speed_mode { LEDC_LOW_SPEED_MODE };
    ledc_timer_bit_t _duty_resolution { LEDC_TIMER_10_BIT };
    ledc_timer_t _timer_num { LEDC_TIMER_0 };
    int _gpio_num {}; // GPIO_NUM_13:  gpio_num = 13
    ledc_channel_t _channel { LEDC_CHANNEL_0 }; // LEDC channel (0 - 7)
    int _hpoint { 0 };
    uint32_t _duty { 0 }; // range of duty setting is [0, (2**duty_resolution)]
    const uint32_t _max_duty;

    ledc_timer_config_t _timer_conf; // timer configuration variable
    ledc_channel_config_t _led_chanal_conf; // channel configuration variable

    uint32_t _last_duty { 0 }; // last set duty
    bool _fan_is_on { false }; // Was the fan turned on

    QueueHandle_t* _sensor_queue { nullptr }; // sensor queue
    QueueHandle_t* _duty_percent_queue { nullptr }; // current duty queue

    SensorData_t sensor_data {};
    // Buffer for average data
    float buffer_sensor[NUM_MEAS] {};
    size_t size = sizeof(buffer_sensor) / sizeof(float);

public:
    // Constructor
    FanPWM(uint8_t& gpio_num, QueueHandle_t* sensor_queue,
        QueueHandle_t* duty_percent_queue);
    FanPWM(uint8_t& gpio_num, QueueHandle_t* sensor_queue,
        QueueHandle_t* duty_percent_queue, uint32_t freq_hz);
    FanPWM(uint8_t& gpio_num, QueueHandle_t* sensor_queue,
        QueueHandle_t* duty_percent_queue, uint32_t freq_hz, uint32_t duty);

    // Setting duty and frequency
    esp_err_t set_duty(uint32_t duty);
    esp_err_t set_freq(uint32_t freq_hz); // NOTE:ESP8266 does not support
    uint32_t get_max_duty(void) { return _max_duty; }
    void start(void);
    constexpr static const char* TAG = "FanPWM";
};

}; // namespace Fan_NS

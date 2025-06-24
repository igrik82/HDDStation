#include "fan.h"

namespace Fan_NS {
// =================== FanPWM constructor ==================
FanPWM::FanPWM(uint8_t& gpio_num, QueueHandle_t* sensor_queue,
    QueueHandle_t* duty_percent_queue, uint32_t* freq_hz,
    uint32_t* min_hdd_temp, uint32_t* max_hdd_temp)
    : _min_temp_hdd(min_hdd_temp)
    , _max_temp_hdd(max_hdd_temp)
    , _freq_hz(freq_hz)
    , _gpio_num { gpio_num }
    , _max_duty((LOW_SPEED_MODE_TIMER / *_freq_hz) * (2 << (_duty_resolution - 1)))
    , _sensor_queue { sensor_queue }
    , _duty_percent_queue { duty_percent_queue }
{
    // Set timer configuration
    _timer_conf.duty_resolution = _duty_resolution;
    _timer_conf.freq_hz = *_freq_hz;
    _timer_conf.speed_mode = _speed_mode;
    _timer_conf.timer_num = _timer_num;
    // Apply timer configuration
    ledc_timer_config(&_timer_conf);

    // Set channel configuration
    _led_chanal_conf.channel = _channel;
    _led_chanal_conf.duty = _duty;
    _led_chanal_conf.gpio_num = _gpio_num;
    _led_chanal_conf.speed_mode = _speed_mode;
    _led_chanal_conf.hpoint = _hpoint;
    _led_chanal_conf.timer_sel = _timer_num;
    // Apply channel configuration
    ledc_channel_config(&_led_chanal_conf);
    // Initialize service.
    ledc_fade_func_install(0);

    _duty = _max_duty;
    ledc_set_duty(_speed_mode, _channel, _duty);
    ledc_update_duty(_speed_mode, _channel);
}

// =================== FanPWM member functions ==================
esp_err_t FanPWM::set_duty(uint32_t duty)
{
    ledc_set_fade_with_time(_speed_mode, _channel, duty,
        5000); // Slow fade for 5 seconds
    ledc_fade_start(_speed_mode, _channel, LEDC_FADE_NO_WAIT);
    _last_duty = duty;
    return ESP_OK;
};

void FanPWM::start(void)
{
    // initial value
    float min = FLT_MAX;
    float max = -FLT_MAX;
    float sum = 0.0f;
    float result = 0.0f;

    // Push to buffer
    for (uint8_t i = 0; i < size; i++) {
        if (xQueueReceive(*_sensor_queue, &sensor_data, 0) == pdTRUE) {
            buffer_sensor[i] = sensor_data.temperature;
            ESP_LOGI(TAG, "Sensor %d temperature %f", sensor_data.sensor_id,
                sensor_data.temperature);
        } else {
            ESP_LOGE(TAG, "Failed to receive sensor data.");
        }
    }
    // Find min, max and sum
    for (uint8_t i = 0; i < size; i++) {
        sum += buffer_sensor[i];
        if (buffer_sensor[i] < min)
            min = buffer_sensor[i];
        if (buffer_sensor[i] > max)
            max = buffer_sensor[i];
    }
    // Subtract min and max from sum
    sum = sum - min - max;

    // Calculate average
    result = sum / (size - 2);
    // calculate duty
    if (result <= *_min_temp_hdd) {
        _duty = 0;
    } else if (result >= *_max_temp_hdd) {
        _duty = _max_duty;
    } else if (result >= *_min_temp_hdd) {
        _duty = static_cast<uint32_t>(_max_duty / (*_max_temp_hdd - *_min_temp_hdd)) * (result - *_min_temp_hdd);
    }

    // Set duty
    set_duty(_duty);

    // Send % speed
    uint8_t persent = (uint8_t)((_duty * 100) / _max_duty);
    if (xQueueSend(*_duty_percent_queue, &persent, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to send duty percent.");
    }
}

esp_err_t FanPWM::set_freq(
    uint32_t freq) // INFO: Change frequency is not supported for esp8266
{
    // ledc_fade_func_uninstall();
    // _timer_conf.freq_hz = freq;
    // ledc_timer_config(&_timer_conf);
    // ledc_fade_func_install(0);
    return ESP_ERR_NOT_SUPPORTED;
}
}; // namespace Fan_NS

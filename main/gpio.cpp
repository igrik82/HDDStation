#include "gpio.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "projdefs.h"
#include "rom/ets_sys.h"
/*
 * TODO:
 *
 * taskENTER_CRITICAL();
 * // Доступ к защищенной переменной
 * taskEXIT_CRITICAL();
 */
namespace OneWire {

// ========================= Initialization ===============================
// Constructor by default for GPIO
DS18B20::DS18B20(const gpio_num_t pin)
    : _pin { pin }
    , _invert_logic { false }
{
    _init_one_wire_gpio();
};

DS18B20::DS18B20(const gpio_num_t pin, const bool invert_logic)
    : DS18B20(pin)
{
    _invert_logic = invert_logic;
};

// Initialization of the GPIO pin
esp_err_t DS18B20::_init_one_wire_gpio(void)
{
    ESP_LOGI(TAG, "Onewire initialization begin.");
    // Configuration of the GPIO pin
    config.pin_bit_mask = 1ULL << _pin; // Bit mask for the pin
    config.pull_up_en = GPIO_PULLUP_DISABLE; // Because of pull-up resistor is
                                             // physically connected
    config.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable pull-down resistor
    config.intr_type = GPIO_INTR_DISABLE; // Disable interrupt

    esp_err_t status = gpio_config(&config);

    if (status != ESP_OK) {
        ESP_LOGE(TAG, "Onewire initialization fail.");
        return status;
    }

    _is_initialized = true;
    ESP_LOGI(TAG, "Onewire initialization success.");

    return status;
};

// ================ GPIO-mode & level =========================================
// Set GPIO direction
esp_err_t DS18B20::pin_direction(gpio_mode_t direction)
{
    if (direction == GPIO_MODE_OUTPUT) {
        _is_output = true;
    } else {
        _is_output = false;
    }
    return gpio_set_direction(_pin, direction);
}

// Set the output level
esp_err_t DS18B20::set_level(const bool level)
{
    if (_is_output) {
        _level = _invert_logic ? !level : level;
        return gpio_set_level(_pin, _level);
    }
    ESP_LOGE(TAG, "Onewire set_level fail. Onewire in input mode.");
    return ESP_ERR_NOT_SUPPORTED;
}

// Read the input level. If it's output, return ESP_FAIL.
uint8_t DS18B20::get_pin_level(void)
{
    if (_is_output) {
        ESP_LOGE(TAG, "Onewire read fail. Onewire in output mode.");
    }
    return _invert_logic ? !gpio_get_level(_pin) : gpio_get_level(_pin);
};

// ========================= 1-Wire ===========================================
// Reset signal
esp_err_t DS18B20::reset(void)
{
    ESP_LOGI(TAG, "Onewire reset begin.");
    if (_is_initialized == false) {
        ESP_LOGE(TAG, "Onewire reset fail. Onewire not initialized.");
        return ESP_ERR_INVALID_STATE;
    }

    // Check if not set up pull-up resistor
    pin_direction(GPIO_MODE_INPUT);
    if (get_pin_level() != 1) {
        ESP_LOGE(TAG, "Onewire reset fail. Bus is busy.");
        return ESP_ERR_INVALID_RESPONSE;
    }

    // Reset condition
    pin_direction(GPIO_MODE_OUTPUT);
    set_level(0);
    taskENTER_CRITICAL();
    os_delay_us(MASTER_RESET_PULSE_DURATION);
    set_level(1);

    // Check presence
    pin_direction(GPIO_MODE_INPUT);
    os_delay_us(BUS_RECOVERY_DURATION);
    uint8_t response_time = 0;
    while (get_pin_level() == 1) {
        if (response_time > SLAVE_RESPONSE_MAX_DURATION) {
            ESP_LOGE(TAG, "Onewire reset fail. Timeout exceeded.");
            return ESP_ERR_TIMEOUT;
        }
        ++response_time;
        os_delay_us(1);
    }
    taskEXIT_CRITICAL();
    os_delay_us(RECOVERY_AFTER_RESET_PULSE);
    ESP_LOGI(TAG, "Onewire reset success.");
    return ESP_OK;
}

// Write bit
void DS18B20::write_bit(uint8_t bit)
{
    taskENTER_CRITICAL();
    if (bit) {
        // bit is 1
        ets_delay_us(PAUSE_BETWEEN_TIME_SLOTS);
        set_level(0);
        ets_delay_us(MASTER_WRITE_1_PULSE_DURATION);
        set_level(1);
        ets_delay_us(MASTER_WRITE_1_RECOVERY_DURATION);
    } else {
        // bit is 0
        ets_delay_us(PAUSE_BETWEEN_TIME_SLOTS);
        set_level(0);
        ets_delay_us(MASTER_WRITE_0_PULSE_DURATION);
        set_level(1);
        ets_delay_us(MASTER_WRITE_0_RECOVERY_DURATION);
    }
    taskEXIT_CRITICAL();
}

// Write byte
void DS18B20::write_byte(uint8_t byte)
{
    pin_direction(GPIO_MODE_OUTPUT);
    ets_delay_us(BUS_RECOVERY_DURATION);

    for (uint8_t i = 0; i < 8; i++) {
        write_bit(byte & 0x01);
        byte >>= 1;
    }
}

// Read bit
uint8_t DS18B20::read_bit(void)
{
    taskENTER_CRITICAL();
    pin_direction(GPIO_MODE_OUTPUT);
    ets_delay_us(BUS_RECOVERY_DURATION);
    set_level(0);
    os_delay_us(MASTER_READ_PULSE_DURATION);
    set_level(1);
    pin_direction(GPIO_MODE_INPUT);
    os_delay_us(MASTER_READ_SAMPLE);
    uint8_t bit = get_pin_level();
    taskEXIT_CRITICAL();
    os_delay_us(MASTER_READ_RECOVERY_DURATION);
    return bit;
}

// Read byte
uint8_t DS18B20::read_byte(void)
{
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; i++) {
        byte |= (read_bit() << i);
    }

    return byte;
}

// Read ROM
esp_err_t DS18B20::readROM(void)
{
    ESP_LOGI(TAG, "Read ROM command begin.");
    if (ESP_OK != reset()) {
        ESP_LOGE(TAG, "Reset failed in read ROM command.");
        return ESP_FAIL;
    }
    write_byte(READ_ROM); // READ ROM command
    // ets_delay_us(TIME_SLOT_START_DURATION);
    uint8_t buf[8] = { 0 };

    for (uint8_t i = 0; i < 8; i++) {
        buf[i] = read_byte();
    }

    printf("Address ROM: { ");
    for (uint8_t i = 0; i < 8; ++i) {
        if (i == 7) {
            printf("0x%02x", buf[i]);
        } else {
            printf("0x%02x, ", buf[i]);
        }
    }
    printf(" },\n");

    ESP_LOGI(TAG, "Read ROM command success.");
    return ESP_OK;
}

// Match ROM
esp_err_t DS18B20::match_rom(uint8_t (&address)[8])
{
    ESP_LOGI(TAG, "Match ROM command begin.");
    if (ESP_OK != reset()) {
        ESP_LOGE(TAG, "Reset failed in read ROM command.");
        return ESP_FAIL;
    }

    write_byte(MATCH_ROM); // MATCH ROM command

    for (uint8_t i = 0; i < 8; i++) {
        write_byte(address[i]);
    }

    ESP_LOGI(TAG, "Match ROM command success.");
    return ESP_OK;
}

// Get temperature
esp_err_t DS18B20::get_temp(uint8_t (&address)[8], float& temperature)
{

    match_rom(address); // Set address
    write_byte(CONVERT_T); // Convert temperature
    vTaskDelay(pdMS_TO_TICKS(WAIT_FOR_TEMPERATURE_CONVERSION));

    match_rom(address);
    write_byte(READ_SCRATCHPAD); // READ SCRATCHPAD command

    uint8_t data[9];

    for (uint8_t i = 0; i < 9; i++) {
        data[i] = read_byte();
    }

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (0x10 == address[0]) {
        raw = raw << 3; // 9 bit resolution default
        if (data[7] == 0x10) {
            // "count remain" gives full 12 bit resolution
            raw = (raw & 0xFFF0) + 12 - data[6];
        }
    } else {
        uint8_t cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00)
            raw = raw & ~7; // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20)
            raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40)
            raw = raw & ~1; // 11 bit res, 375 ms
                            //// default is 12 bit resolution, 750 ms
                            // conversion time
    }
    temperature = raw / 16.0;
    return ESP_OK;
}
} // namespace OneWire

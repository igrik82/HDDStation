#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_event.h" // IWYU pragma: keep
#include "esp_log.h" // IWYU pragma: keep

// #define esp_delay_us(x) os_delay_us(x) // Delay in microseconds max 65535 us

// ============================= Reset ========================================
#define MASTER_RESET_PULSE_DURATION 490
#define MASTER_READ_PRESENSE 70
#define RECOVERY_AFTER_RESET_PULSE 410
#define SLAVE_RESPONSE_MAX_DURATION 170
// ============================ Write 1 =======================================
#define MASTER_WRITE_1_PULSE_DURATION 6
#define MASTER_WRITE_1_RECOVERY_DURATION 64
// ============================ Write 0 =======================================
#define MASTER_WRITE_0_PULSE_DURATION 60
#define MASTER_WRITE_0_RECOVERY_DURATION 10
// ============================= Read =========================================
#define MASTER_READ_PULSE_DURATION 6
#define MASTER_READ_SAMPLE 9
#define MASTER_READ_RECOVERY_DURATION 55
// ========================== Bus recovery ====================================
#define BUS_RECOVERY_DURATION 2 // Bus recovery time.
#define PAUSE_BETWEEN_TIME_SLOTS 5
// =========================== Commands =======================================
#define WAIT_FOR_TEMPERATURE_CONVERSION 1000
#define MATCH_ROM 0x55 // Match ROM command to address a specific 1-Wire device
#define CONVERT_T 0x44 // Convert temperature command
#define READ_ROM 0x33 // Read ROM command for 1-Wire device
#define READ_SCRATCHPAD 0xBE // Read Scratchpad command to read temperature

namespace OneWire {

class DS18B20 {
protected:
    // ================= Class variables ======================================
    // Initialize queue
    const gpio_num_t _pin; // Pin number
    bool _invert_logic { false }; // Invert logic
    gpio_config_t config; // Pin configuration
    bool _is_initialized { false };
    bool _level; // Output level
    bool _is_output;
    const char* TAG = "DS18B20";

    // Initialization of the GPIO pin in output/input mode
    esp_err_t _init_one_wire_gpio(void);

public:
    // =================== Constructor ========================================
    DS18B20(const gpio_num_t pin);
    DS18B20(const gpio_num_t pin, const bool invert_logic);

    // ================ GPIO-mode & level +====================================
    esp_err_t pin_direction(gpio_mode_t direction);
    esp_err_t set_level(const bool level);
    uint8_t get_pin_level(void);

    // =================== 1-Wire =============================================
    esp_err_t reset(void);
    void write_bit(uint8_t bit);
    void write_byte(uint8_t byte);
    uint8_t read_bit(void);
    uint8_t read_byte(void);
    esp_err_t readROM(void);
    esp_err_t match_rom(uint8_t (&address)[8]);
    esp_err_t get_temp(uint8_t (&address)[8], float& temperature);

}; // class Gpio

} // namespace OneWire

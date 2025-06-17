#pragma once

#include "esp_https_ota.h" // IWYU pragma: keep
#include "esp_log.h"       // IWYU pragma: keep
#include "esp_system.h"    // IWYU pragma: keep
#include <cstring>
#include <string>

namespace Ota_NS {

struct OtaParams {
  std::string firmware_url;
};

class Ota {
protected:
  esp_http_client_config_t config;
  std::string firmware_url;

public:
  explicit Ota(const OtaParams &params);
  ~Ota(void);

  esp_err_t start_update(void);

  constexpr static const char *TAG = "OTA_Update";
};

}; // namespace Ota_NS

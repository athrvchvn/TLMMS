#include "ota_handler.h"
#include "config.h"
#include "nvs_manager.h"
#include "relay_controller.h"
#include "display_primary.h"
#include <Arduino.h>
#include <esp_https_ota.h>
#include <esp_ota_ops.h>

bool OTA::applyUpdate(const char* url, const char* newVersion) {
    // Safety: ensure relay is OFF before OTA
    Relay::off();

    char currentVer[16];
    NVS::getFwVersion(currentVer, sizeof(currentVer));
    DisplayPrimary::showOtaUpdate(currentVer, newVersion, 0);

    esp_http_client_config_t httpCfg = {};
    httpCfg.url              = url;
    httpCfg.cert_pem         = nullptr;  // Set to Firebase root CA cert in production
    httpCfg.skip_cert_common_name_check = true;  // OK for Firebase Storage URLs
    httpCfg.timeout_ms       = 30000;

    esp_https_ota_config_t otaCfg = {};
    otaCfg.http_config = &httpCfg;

    esp_https_ota_handle_t handle = nullptr;
    esp_err_t err = esp_https_ota_begin(&otaCfg, &handle);
    if (err != ESP_OK) {
        DisplayPrimary::showError("OTA_BEGIN_FAILED");
        return false;
    }

    while (true) {
        err = esp_https_ota_perform(handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) break;

        // Update progress display
        int imageLen  = esp_https_ota_get_image_size(handle);
        int readLen   = esp_https_ota_get_image_len_read(handle);
        int pct = (imageLen > 0) ? (readLen * 100 / imageLen) : 0;
        DisplayPrimary::showOtaUpdate(currentVer, newVersion, pct);
        delay(100);
    }

    if (err != ESP_OK || !esp_https_ota_is_complete_data_received(handle)) {
        esp_https_ota_abort(handle);
        DisplayPrimary::showError("OTA_INCOMPLETE");
        return false;
    }

    err = esp_https_ota_finish(handle);
    if (err != ESP_OK) {
        DisplayPrimary::showError("OTA_FINISH_FAILED");
        return false;
    }

    DisplayPrimary::showOtaUpdate(currentVer, newVersion, 100);
    NVS::setFwVersion(newVersion);
    delay(1000);

    esp_restart();
    return true;  // Never reached
}

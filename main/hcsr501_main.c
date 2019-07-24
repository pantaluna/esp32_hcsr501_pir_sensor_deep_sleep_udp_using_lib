/*
 * README.MD
 *
 */
#include "driver/rtc_io.h"

#include "mjd.h"
#include "mjd_lolind32.h"
#include "mjd_net.h"
#include "mjd_wifi.h"

/*
 * Logging
 */
static const char TAG[] = "myapp";

/*
 * FreeRTOS settings
 */
#define MYAPP_RTOS_TASK_STACK_SIZE_8K (8192)
#define MYAPP_RTOS_TASK_PRIORITY_NORMAL (RTOS_TASK_PRIORITY_NORMAL)

/*
 * KConfig: LED, WIFI
 */
static const int MY_LED_ON_DEVBOARD_GPIO_NUM = CONFIG_MY_LED_ON_DEVBOARD_GPIO_NUM;
static const int MY_LED_ON_DEVBOARD_WIRING_TYPE = CONFIG_MY_LED_ON_DEVBOARD_WIRING_TYPE;

static const char *MY_WIFI_SSID = CONFIG_MY_WIFI_SSID;
static const char *MY_WIFI_PASSWORD = CONFIG_MY_WIFI_PASSWORD;

static const char *MY_DEVICE_ID = CONFIG_MY_DEVICE_ID;
static const int MY_DEVICE_SNTP_RESYNC_AFTER_X_BOOTS = CONFIG_MY_DEVICE_SNTP_RESYNC_AFTER_X_BOOTS;

static const int MY_SENSOR_GPIO_INPUT_PIN = CONFIG_MY_SENSOR_GPIO_INPUT_PIN;
static const int MY_SENSOR_BLOCKING_TIME_AFTER_TRIGGER_SECONDS = CONFIG_MY_SENSOR_BLOCKING_TIME_AFTER_TRIGGER_SECONDS;

static const int MY_DO_UPLOAD_TO_UDP_SERVER = CONFIG_MY_DO_UPLOAD_TO_UDP_SERVER;
static const char *MY_UDP_SERVER_HOSTNAME = CONFIG_MY_UDP_SERVER_HOSTNAME;
static const int MY_UDP_SERVER_PORT = CONFIG_MY_UDP_SERVER_PORT;

/*
 * Config (not KConfig)
 */

/*
 * Module: Trigger blocking time logic
 */
static RTC_DATA_ATTR time_t _sensor_last_trigger_time = 0; //@important Allocated in RTC Slow Memory

static esp_err_t _update_sensor_last_trigger_time() {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    time(&_sensor_last_trigger_time);
    ESP_LOGI(TAG, "%s(). Updated _sensor_last_trigger_time: %" PRIi64 "", __FUNCTION__,
            (int64_t )_sensor_last_trigger_time);

    return ESP_OK;
}

static bool _is_valid_trigger() {
    // @dep mjd_get_mcu_boot_count() MY_SENSOR_BLOCKING_TIME_AFTER_TRIGGER_SECONDS
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    bool b_retval = false;

    ESP_LOGI(TAG, "\n\n===SECTION: CHECK TRIGGER===");
    ESP_LOGD(TAG, "  mjd_get_mcu_boot_count():  %u", mjd_get_mcu_boot_count());
    ESP_LOGD(TAG, "  _sensor_last_trigger_time: %" PRIi64 "", (int64_t )_sensor_last_trigger_time);

    if (mjd_get_mcu_boot_count() <= 1) {
        b_retval = false;
        // GOTO
        goto cleanup;
    }

    time_t current_time;
    time(&current_time);
    uint32_t diff_seconds = (uint32_t) difftime(current_time, _sensor_last_trigger_time);
    ESP_LOGD(TAG, "  current_time: %" PRIi64 "", (int64_t )current_time);
    ESP_LOGI(TAG, "  diff_seconds: %u", diff_seconds); // LOG_INFO
    if (diff_seconds > MY_SENSOR_BLOCKING_TIME_AFTER_TRIGGER_SECONDS) {
        b_retval = true;
    }

    // LABEL
    cleanup: ;

    ESP_LOGI(TAG, "%s(). b_retval: %u", __FUNCTION__, b_retval); // LOG_INFO
    return b_retval;
}

/*
 * @brief Module: SNTP sync datetime
 *
 * @important SNTP requires a valid Internet connection.
 *
 */
static esp_err_t _module_sync_datetime() {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    esp_err_t f_retval = ESP_OK;

    const uint32_t SNTP_RESYNC_AFTER_X_BOOTS = 10;

    ESP_LOGI(TAG, "\n\n===SECTION: SNTP sync datetime===");

    if (mjd_get_mcu_boot_count() <= 1 || mjd_get_mcu_boot_count() % SNTP_RESYNC_AFTER_X_BOOTS == 0)
            {

        ESP_LOGI(TAG, "Wifi");
        f_retval = mjd_wifi_sta_init(MY_WIFI_SSID, MY_WIFI_PASSWORD);
        if (f_retval != ESP_OK) {
            ESP_LOGE(TAG, "%s(). mjd_wifi_sta_init() err %i (%s)", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
            // GOTO
            goto cleanup;
        }
        f_retval = mjd_wifi_sta_start();
        if (f_retval != ESP_OK) {
            ESP_LOGE(TAG, "%s(). mjd_wifi_sta_start() err %i (%s)", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
            mjd_led_mark_error(MY_LED_ON_DEVBOARD_GPIO_NUM);
            // GOTO
            goto cleanup;
        }
        mjd_wifi_log_sta_info();
        f_retval = mjd_net_is_internet_reachable();
        if (f_retval != ESP_OK) {
            ESP_LOGE(TAG, "%s(). mjd_net_is_internet_reachable() | err %i (%s)", __FUNCTION__, f_retval,
                    esp_err_to_name(f_retval));
            mjd_led_mark_error(MY_LED_ON_DEVBOARD_GPIO_NUM);
            // GOTO
            goto cleanup;
        }
        ESP_LOGI(TAG, "OK Internet reachable");

        f_retval = mjd_net_sync_current_datetime(true); // @important param_force = true
        if (f_retval != ESP_OK) {
            ESP_LOGE(TAG, "%s(). mjd_net_sync_current_datetime(false) | err %i (%s)", __FUNCTION__, f_retval,
                    esp_err_to_name(f_retval));
            mjd_led_mark_error(MY_LED_ON_DEVBOARD_GPIO_NUM);
            // GOTO
            goto cleanup;
        }
    }
    else {
        ESP_LOGI(TAG, "  Sync NOT required (@dep bootcount %u)", mjd_get_mcu_boot_count());
    }

    // LABEL
    cleanup:

    ESP_LOGI(TAG, "%s(). Cleanup", __FUNCTION__);
    f_retval = mjd_wifi_sta_disconnect_stop();
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). mjd_wifi_sta_disconnect_stop() err %i (%s)", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
        mjd_led_mark_error(MY_LED_ON_DEVBOARD_GPIO_NUM);
    }

    return f_retval;
}

/*
 * @brief Module: SNTP sync datetime
 *
 * @doc Do SNTP Sync if bootcount==1 or bootcount divisible by SNTP_RESYNC_AFTER_X_BOOTS
 *
 */
static esp_err_t _module_udp_upload() {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    esp_err_t f_retval = ESP_OK;

    ESP_LOGI(TAG, "\n\n===SECTION: UDP UPLOAD===");

    if (MY_DO_UPLOAD_TO_UDP_SERVER == 0) {
        ESP_LOGI(TAG, "  UDP not done because MY_DO_UPLOAD_TO_UDP_SERVER == 0");
        // GOTO
        goto cleanup;
    }

    ESP_LOGI(TAG, "Wifi");
    f_retval = mjd_wifi_sta_init(MY_WIFI_SSID, MY_WIFI_PASSWORD);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). mjd_wifi_sta_init() err %i (%s)", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }
    f_retval = mjd_wifi_sta_start();
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). mjd_wifi_sta_start() err %i (%s)", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
        mjd_led_mark_error(MY_LED_ON_DEVBOARD_GPIO_NUM);
        // GOTO
        goto cleanup;
    }

    mjd_wifi_log_sta_info();

    char current_time[14 + 1];
    mjd_get_current_time_yyyymmddhhmmss(current_time);

    float battery_voltage = mjd_lolind32_get_battery_voltage();
    ESP_LOGI(TAG, "LOLIN D32 battery voltage (V): %4.2f", battery_voltage);

    char message[128];
    sprintf(message, "<~>\r\n%s\r\n%s\r\n%4.2f\r\n1", MY_DEVICE_ID, current_time, battery_voltage);
    ESP_LOGI(TAG, "UDP message: %s", message);

    f_retval = mjd_net_udp_send_buffer(MY_UDP_SERVER_HOSTNAME, MY_UDP_SERVER_PORT, (uint8_t *) message,
            strlen(message));
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). mjd_net_udp_send_buffer() err %i (%s)", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
        mjd_led_mark_error(MY_LED_ON_DEVBOARD_GPIO_NUM);
        // GOTO
        goto cleanup;
    }
    ESP_LOGI(TAG, "OK UDP buffer sent");

    // LABEL
    cleanup:

    ESP_LOGI(TAG, "%s(). Cleanup", __FUNCTION__);
    f_retval = mjd_wifi_sta_disconnect_stop();
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). mjd_wifi_sta_disconnect_stop() err %i (%s)", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
        mjd_led_mark_error(MY_LED_ON_DEVBOARD_GPIO_NUM);
    }

    return f_retval;
}

/*
 * TASK
 */
void main_task(void *pvParameter) {
    ESP_LOGD(TAG, "%s()", __FUNCTION__);

    mjd_log_memory_statistics();

    /********************************************************************************
     * Reuseable variables
     *
     */
    esp_err_t f_retval = ESP_OK;

    /*********************************
     * LOGGING
     * Optional for Production: dump less messages
     * @doc It is possible to lower the log level for specific modules.
     *
     */
    esp_log_level_set("memory_layout", ESP_LOG_WARN); // @important Disable INFO messages which are too detailed for me.
    esp_log_level_set("nvs", ESP_LOG_WARN); // @important Disable INFO messages which are too detailed for me.
    esp_log_level_set("phy_init", ESP_LOG_WARN); // @important Disable INFO messages which are too detailed for me.
    esp_log_level_set("RTC_MODULE", ESP_LOG_WARN); // @important Disable INFO messages which are too detailed for me.
    esp_log_level_set("tcpip_adapter", ESP_LOG_WARN); // @important Disable INFO messages which are too detailed for me.
    esp_log_level_set("wifi", ESP_LOG_WARN); // @important Disable INFO messages which are too detailed for me.

    /********************************************************************************
     * SOC init
     *
     * @doc exec nvs_flash_init() - mandatory for Wifi to work later on
     *
     */
// DEVTEMP_BEGIN Fix for ESP-IDF github tag v3.2-dev
#ifndef ESP_ERR_NVS_NEW_VERSION_FOUND
#define ESP_ERR_NVS_NEW_VERSION_FOUND (ESP_ERR_NVS_BASE + 0xff)
#endif
// DEVTEMP-END

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_LOGW(TAG, "  ESP_ERR_NVS_NO_FREE_PAGES - do nvs_flash_erase()");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    } else if (err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "  ESP_ERR_NVS_NEW_VERSION_FOUND - do nvs_flash_erase()");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /********************************************************************************
     * MY STANDARD Init
     *
     */
    mjd_set_timezone_utc();
    mjd_log_time();

    mjd_log_wakeup_details();
    mjd_increment_mcu_boot_count();
    mjd_log_mcu_boot_count();

    /********************************************************************************
     * Dump KConfig
     *
     */
    ESP_LOGI(TAG, "\n\n===SECTION: Dump KConfig===");
    ESP_LOGI(TAG, "MY_LED_ON_DEVBOARD_GPIO_NUM:    %i", MY_LED_ON_DEVBOARD_GPIO_NUM);
    ESP_LOGI(TAG, "MY_LED_ON_DEVBOARD_WIRING_TYPE: %i", MY_LED_ON_DEVBOARD_WIRING_TYPE);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "MY_WIFI_SSID:     %s", MY_WIFI_SSID);
    ESP_LOGI(TAG, "MY_WIFI_PASSWORD: %s", MY_WIFI_PASSWORD);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "MY_DEVICE_ID:               %s", MY_DEVICE_ID);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "MY_SENSOR_GPIO_INPUT_PIN:                %i", MY_SENSOR_GPIO_INPUT_PIN);
    ESP_LOGI(TAG, "MY_SENSOR_TRIGGER_BLOCKING_TIME_SECONDS: %i", MY_SENSOR_BLOCKING_TIME_AFTER_TRIGGER_SECONDS);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "MY_DO_UPLOAD_TO_UDP_SERVER: %i", MY_DO_UPLOAD_TO_UDP_SERVER);
    ESP_LOGI(TAG, "MY_UDP_SERVER_HOSTNAME:     %s", MY_UDP_SERVER_HOSTNAME);
    ESP_LOGI(TAG, "MY_UDP_SERVER_PORT:         %i", MY_UDP_SERVER_PORT);
    ESP_LOGI(TAG, "");

    /********************************************************************************
     * LED Config
     *
     */
    ESP_LOGI(TAG, "\n\n===SECTION: LED===");

    mjd_led_config_t led_config = MJD_LED_CONFIG_DEFAULT();
    led_config.gpio_num = MY_LED_ON_DEVBOARD_GPIO_NUM;
    led_config.wiring_type = MY_LED_ON_DEVBOARD_WIRING_TYPE; // 1 = MCU Huzzah32 | 2 = MCU Lolin32 Lite, LOLIN D32
    mjd_led_config(&led_config);

    mjd_led_blink_times(MY_LED_ON_DEVBOARD_GPIO_NUM, 1);

    /********************************************************************************
     * MAIN
     *
     */

    if (mjd_get_mcu_boot_count() <= 1) {
        ESP_LOGI(TAG, "SNTP Sync required when bootcount==1");
        f_retval = _module_sync_datetime();
        if (f_retval != ESP_OK) {
            ESP_LOGE(TAG, "_module_sync_datetime() | err %i (%s)", f_retval, esp_err_to_name(f_retval));
            // GOTO
            goto cleanup;
        }
    }
    else {
        if (!_is_valid_trigger()) {
            ESP_LOGI(TAG, "This new trigger is ignored. It falls within the blocking time after the last valid trigger.");
        } else {
            _update_sensor_last_trigger_time();

            if (mjd_get_mcu_boot_count() % MY_DEVICE_SNTP_RESYNC_AFTER_X_BOOTS == 0) {
                ESP_LOGI(TAG, "SNTP Sync required when bootcount (%u) divisible by %u", mjd_get_mcu_boot_count(),
                        MY_DEVICE_SNTP_RESYNC_AFTER_X_BOOTS);
                f_retval = _module_sync_datetime();
                if (f_retval != ESP_OK) {
                    ESP_LOGE(TAG, "_module_sync_datetime() | err %i (%s)", f_retval, esp_err_to_name(f_retval));
                    // GOTO
                    goto cleanup;
                }
            }

            f_retval = _module_udp_upload();
            if (f_retval != ESP_OK) {
                ESP_LOGE(TAG, "_module_udp_upload() | err %i (%s)", f_retval, esp_err_to_name(f_retval));
                // GOTO
                goto cleanup;
            }
        }
        // @important
        ESP_LOGI(TAG,
                "Wait 2 seconds extra because the PIR sensor stays *ON for 2,5 seconds (avoid another false trigger...)");
        vTaskDelay(RTOS_DELAY_2SEC);
    }

    /********************************************************************************
     * CLEANUP
     *
     */
    // LABEL
    cleanup: ;

    /********************************************************************************
     * DEEP SLEEP & RESTART TIMER
     * @important Before entering sleep mode, applications must disable WiFi and BT using appropriate calls e.g. esp_bluedroid_disable(), esp_bt_controller_disable(), esp_wifi_stop()
     * @doc https://esp-idf.readthedocs.io/en/latest/api-reference/system/sleep_modes.html
     *
     * @doc esp_sleep_enable_ext0_wakeup @param level input level which will trigger wakeup (0=low, 1=high)
     *
     * @doc After wake up from sleep, IO pad used for wakeup will be configured as RTC IO. Before using this pad as digital GPIO,
     *        reconfigure it using rtc_gpio_deinit(gpio_num) function [not needed in this app].
     *
     * MODE: RTC GPIO ***ENABLED***
     * - Wakeup on level High (1).
     * - RTC pulldown enabled.
     * - The PIR sensor remains HIGH for 2.5 seconds (=blocking time in the data sheet)!
     *
     */
    ESP_LOGI(TAG, "\n\n===SECTION: DEEP SLEEP===");

    mjd_log_memory_statistics();
    mjd_log_time();

    ESP_LOGI(TAG, "Entering deep sleep. The MCU will wake up when RTC GPIO#%u is HIGH, or becomes HIGH...",
            MY_SENSOR_GPIO_INPUT_PIN);
    if (!rtc_gpio_is_valid_gpio(MY_SENSOR_GPIO_INPUT_PIN)) {
        f_retval = ESP_ERR_INVALID_ARG;
        ESP_LOGE(TAG, "%s(). rtc_gpio_is_valid_gpio() [not an RTC pin?] | err %i %s", __FUNCTION__, f_retval,
                esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }
    f_retval = rtc_gpio_pulldown_en(MY_SENSOR_GPIO_INPUT_PIN);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). rtc_gpio_pulldown_en() | err %i %s", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }
    f_retval = esp_sleep_enable_ext0_wakeup(MY_SENSOR_GPIO_INPUT_PIN, 1);
    if (f_retval != ESP_OK) {
        ESP_LOGE(TAG, "%s(). esp_sleep_enable_ext0_wakeup() | err %i %s", __FUNCTION__, f_retval, esp_err_to_name(f_retval));
        // GOTO
        goto cleanup;
    }
    ESP_LOGI(TAG, "LED blink 2x before going into deep sleep");
    mjd_led_blink_times(MY_LED_ON_DEVBOARD_GPIO_NUM, 2);
    esp_deep_sleep_start(); // The code ends here...

    /********************************************************************************
     * Task Delete
     * @doc Code never gets here...
     *
     */
    vTaskDelete(NULL);
}

/*
 * MAIN
 */
void app_main() {
    ESP_LOGI(TAG, "%s()", __FUNCTION__);

    mjd_log_memory_statistics();

    /**********
     * TASK:
     * @important For stability (RMT + Wifi): always use xTaskCreatePinnedToCore(APP_CPU_NUM) [Opposed to xTaskCreate()]
     */
    BaseType_t xReturned;
    xReturned = xTaskCreatePinnedToCore(&main_task, "main_task (name)", MYAPP_RTOS_TASK_STACK_SIZE_8K, NULL,
    MYAPP_RTOS_TASK_PRIORITY_NORMAL, NULL,
    APP_CPU_NUM);
    if (xReturned == pdPASS) {
        ESP_LOGI(TAG, "OK Task has been created, and is running right now");
    }

    /**********
     * END
     */
    ESP_LOGI(TAG, "END %s()", __FUNCTION__);
}

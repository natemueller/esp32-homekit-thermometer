#include <stdio.h>
#include <esp_wifi.h>
#include <FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <nvs_flash.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

#include <dht.h>

#include "config.h"

void on_wifi_ready();

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT && (event_id == WIFI_EVENT_STA_START || event_id == WIFI_EVENT_STA_DISCONNECTED)) {
    printf("STA start\n");
    esp_wifi_connect();
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    printf("WiFI ready\n");
    on_wifi_ready();
  }
}

static void wifi_init() {
  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  wifi_config_t wifi_config = {
    .sta = {
      .ssid = WIFI_SSID,
      .password = WIFI_PASSWORD,
    },
  };

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

const int led_gpio = 2;
bool led_on = false;
void led_write(bool on) {
  gpio_set_level(led_gpio, on ? 1 : 0);
}

void led_init() {
  gpio_set_direction(led_gpio, GPIO_MODE_OUTPUT);
  led_write(led_on);
}

void led_identify_task(void *_args) {
  for (int i=0; i<3; i++) {
    for (int j=0; j<2; j++) {
      led_write(true);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      led_write(false);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

    vTaskDelay(250 / portTICK_PERIOD_MS);
  }

  led_write(led_on);

  vTaskDelete(NULL);
}

void temperature_sensor_identify(homekit_value_t _value) {
  xTaskCreate(led_identify_task, "LED identify", 512, NULL, 2, NULL);
}

homekit_characteristic_t temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
homekit_characteristic_t humidity    = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);


void temperature_sensor_task(void *_args) {
  float humidity_value, temperature_value;
  while (1) {
    bool success = dht_read_float_data(
      DHT_TYPE_DHT22, SENSOR_PIN,
      &humidity_value, &temperature_value
      );
    if (success) {
      temperature.value.float_value = temperature_value;
      humidity.value.float_value = humidity_value;

      homekit_characteristic_notify(&temperature, HOMEKIT_FLOAT(temperature_value));
      homekit_characteristic_notify(&humidity, HOMEKIT_FLOAT(humidity_value));
    } else {
      printf("Couldnt read data from sensor\n");
    }

    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

void temperature_sensor_init() {
  xTaskCreate(temperature_sensor_task, "Temperature Sensor", 8096, NULL, 2, NULL);
}


homekit_accessory_t *accessories[] = {
  HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_thermostat, .services=(homekit_service_t*[]) {
      HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Temperature Sensor"),
          HOMEKIT_CHARACTERISTIC(MANUFACTURER, "NateCo"),
          HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0000001"),
          HOMEKIT_CHARACTERISTIC(MODEL, "MyTemperatureSensor"),
          HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.1"),
          HOMEKIT_CHARACTERISTIC(IDENTIFY, temperature_sensor_identify),
          NULL
        }),
      HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Temperature Sensor"),
          &temperature,
          NULL
        }),
      HOMEKIT_SERVICE(HUMIDITY_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
          HOMEKIT_CHARACTERISTIC(NAME, "Humidity Sensor"),
          &humidity,
          NULL
        }),
      NULL
    }),
  NULL
};

homekit_server_config_t config = {
  .accessories = accessories,
  .password = PAIRING_PASSWORD,
  .setupId = PAIRING_SETUPID
};

void on_wifi_ready() {
  homekit_server_init(&config);
}

void app_main(void) {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK( ret );

  wifi_init();
  led_write(led_on);
  temperature_sensor_init();
}

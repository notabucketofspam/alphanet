#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_log_level.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "AlphaSign.h"

static const char* TAG = "SLOPPO";

// Wi-Fi Configuration
#include "wifi_password.h"
#define UDP_PORT       5005
#define UDP_RX_TIMEOUT_MS 100

// Internal buffer to receive UDP datagrams
static alpha_frame_t rx_buffer;

/*
NOTE:
you gotta have this set up in your kconfig
CONFIG_LWIP_IP4_REASSEMBLY=y
CONFIG_LWIP_IP6_REASSEMBLY=y
*/
// gotta change the setting in menuconfig to increase the max number of wifi static rx buffers

static void udp_server_task(void* pvParameters) {
  char rx_temp[sizeof(alpha_frame_t) + 32]; // Slight overhead buffer

  ESP_LOGI(TAG, "ok we ball");
  while (1) {
    struct sockaddr_in dest_addr;
    // Explicitly zero out the struct first
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(UDP_PORT);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
      ESP_LOGE("UDP", "Unable to create socket: errno %d", errno);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    int err = bind(sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
      ESP_LOGE("UDP", "Socket unable to bind: errno %d", errno);
      close(sock);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }
    ESP_LOGI("UDP", "Socket bound, listening on port %d", UDP_PORT);

    while (1) {
      struct sockaddr_in source_addr;
      socklen_t socklen = sizeof(source_addr);

      // Wait for UDP packet
      int len = recvfrom(sock, rx_temp, sizeof(rx_temp), 0,
        (struct sockaddr*)&source_addr, &socklen);

      //ESP_LOGI(TAG, "got frame");

      if (len == sizeof(alpha_frame_t)) {
        // We received a perfectly sized 1,680-byte frame!
        // Copy it to our typed struct and push to the display driver
        memcpy(&rx_buffer, rx_temp, sizeof(alpha_frame_t));
        alpha_sign_update_frame(&rx_buffer);
      }
    }

    if (sock != -1) {
      close(sock);
    }
  }
}

static void wifi_init_sta(void) {
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  wifi_config_t wifi_config = {
      .sta = {
          .ssid = WIFI_SSID,
          .password = WIFI_PASS,
      },
  };

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_start();
  esp_wifi_connect();
}

void app_main(void) {
  // 1. Initialize NVS (Required by Wi-Fi stack)
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // 2. Start the display driver (spins up permanently on Core 1)
  alpha_sign_init();

  // 3. Connect to Wi-Fi
  wifi_init_sta();

  // 4. Spin up the UDP listener on Core 0
  xTaskCreatePinnedToCore(
    udp_server_task,
    "udp_server",
    4096,
    NULL,
    5,
    NULL,
    0 // Core 0
  );
}


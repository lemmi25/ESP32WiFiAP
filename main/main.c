/*  WiFi connection over AP by Moritz Boesenberg

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include <esp_http_server.h>

#include <sys/param.h>
#include "tcpip_adapter.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define LED_R 0
#define LED_G 2
#define LED_B 4

static EventGroupHandle_t s_wifi_event_group;

static EventGroupHandle_t wifi_event_group;

const int CONNECTED_BIT = BIT0;

static const char *TAG = "ESP32_Server";

static RTC_DATA_ATTR char GOT_IP = false;
static RTC_DATA_ATTR char __SSID[32];
static RTC_DATA_ATTR char __PWD[64];

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }

    ESP_LOGI(TAG, "wifi_event_handler wifi_event_handler wifi_event_handler");
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP ");
        ESP_LOGI(TAG, "Login Success");
        gpio_set_level(LED_R, 1);
        gpio_set_level(LED_G, 1);
        gpio_set_level(LED_B, 1);
        GOT_IP = true;
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:

        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED %d ", esp_wifi_connect());
        esp_wifi_connect();
        if (GOT_IP == false)
        {
            ESP_LOGI(TAG, "Sorry wrong PSW");
            memset(&__PWD[0], 0, sizeof(__PWD));
            memset(&__SSID[0], 0, sizeof(__SSID));
            esp_sleep_enable_timer_wakeup(100000);
            gpio_set_level(LED_R, 0);
            gpio_set_level(LED_G, 0);
            gpio_set_level(LED_B, 0);
            esp_deep_sleep_start();
        }
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static esp_err_t servePage_get_handler(httpd_req_t *req)
{
    httpd_resp_sendstr_chunk(req, "<!DOCTYPE html><html>");

    httpd_resp_sendstr_chunk(req, "<head>");
    httpd_resp_sendstr_chunk(req, "<style>");
    httpd_resp_sendstr_chunk(req, "form {display: grid;padding: 1em; background: #f9f9f9; border: 1px solid #c1c1c1; margin: 2rem auto 0 auto; max-width: 400px; padding: 1em;}}");
    httpd_resp_sendstr_chunk(req, "form input {background: #fff;border: 1px solid #9c9c9c;}");
    httpd_resp_sendstr_chunk(req, "form button {background: lightgrey; padding: 0.7em;width: 100%; border: 0;");
    httpd_resp_sendstr_chunk(req, "label {padding: 0.5em 0.5em 0.5em 0;}");
    httpd_resp_sendstr_chunk(req, "input {padding: 0.7em;margin-bottom: 0.5rem;}");
    httpd_resp_sendstr_chunk(req, "input:focus {outline: 10px solid gold;}");
    httpd_resp_sendstr_chunk(req, "@media (min-width: 300px) {form {grid-template-columns: 200px 1fr; grid-gap: 16px;} label { text-align: right; grid-column: 1 / 2; } input, button { grid-column: 2 / 3; }}");
    httpd_resp_sendstr_chunk(req, "</style>");
    httpd_resp_sendstr_chunk(req, "</head>");

    httpd_resp_sendstr_chunk(req, "<body>");
    httpd_resp_sendstr_chunk(req, "<form class=\"form1\" id=\"loginForm\" action=\"\">");

    httpd_resp_sendstr_chunk(req, "<label for=\"SSID\">WiFi Name</label>");
    httpd_resp_sendstr_chunk(req, "<input id=\"ssid\" type=\"text\" name=\"ssid\" maxlength=\"64\" minlength=\"4\">");

    httpd_resp_sendstr_chunk(req, "<label for=\"Password\">Password</label>");
    httpd_resp_sendstr_chunk(req, "<input id=\"pwd\" type=\"password\" name=\"pwd\" maxlength=\"64\" minlength=\"4\">");

    httpd_resp_sendstr_chunk(req, "<button>Submit</button>");
    httpd_resp_sendstr_chunk(req, "</form>");

    httpd_resp_sendstr_chunk(req, "<script>");
    httpd_resp_sendstr_chunk(req, "document.getElementById(\"loginForm\").addEventListener(\"submit\", (e) => {e.preventDefault(); const formData = new FormData(e.target); const data = Array.from(formData.entries()).reduce((memo, pair) => ({...memo, [pair[0]]: pair[1],  }), {}); var xhr = new XMLHttpRequest(); xhr.open(\"POST\", \"http://192.168.1.1/connection\", true); xhr.setRequestHeader('Content-Type', 'application/json'); xhr.send(JSON.stringify(data)); document.getElementById(\"output\").innerHTML = JSON.stringify(data);});");
    httpd_resp_sendstr_chunk(req, "</script>");

    httpd_resp_sendstr_chunk(req, "</body></html>");

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t servePage = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = servePage_get_handler,
    .user_ctx = NULL};

static esp_err_t psw_ssid_get_handler(httpd_req_t *req)
{
    char buf[128];
    int ret, remaining = req->content_len;

    while (remaining > 0)
    {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                                  MIN(remaining, sizeof(buf)))) <= 0)
        {
            if (ret == 0)
            {
                ESP_LOGI(TAG, "No content received please try again ...");
            }
            else if (ret == HTTPD_SOCK_ERR_TIMEOUT)
            {

                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        /* Log data received */
        /* ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%.*s", ret, buf);
        ESP_LOGI(TAG, "===================================="); */
        cJSON *root = cJSON_Parse(buf);

        sprintf(__SSID, "%s", cJSON_GetObjectItem(root, "ssid")->valuestring);
        sprintf(__PWD, "%s", cJSON_GetObjectItem(root, "pwd")->valuestring);

        ESP_LOGI(TAG, "pwd: %s", __PWD);
        ESP_LOGI(TAG, "ssid: %s", __SSID);

        remaining -= ret;
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    esp_sleep_enable_timer_wakeup(100000);
    esp_deep_sleep_start();
    return ESP_OK;
}

static const httpd_uri_t psw_ssid = {
    .uri = "/connection",
    .method = HTTP_POST,
    .handler = psw_ssid_get_handler,
    .user_ctx = "TEST"};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &servePage);
        httpd_register_uri_handler(server, &psw_ssid);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void wifi_init_softap()
{

    if (strlen(__PWD) && strlen(__SSID) != 0)
    {
        tcpip_adapter_init();
        wifi_event_group = xEventGroupCreate();
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

        wifi_config_t wifi_config = {};
        strcpy((char *)wifi_config.sta.ssid, __SSID);
        strcpy((char *)wifi_config.sta.password, __PWD);

        ESP_LOGI(TAG, "WiFi %s ", wifi_config.sta.ssid);
        ESP_LOGI(TAG, "PSW %s ", wifi_config.sta.password);
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());

        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    }
    else
    {
        s_wifi_event_group = xEventGroupCreate();

        tcpip_adapter_init();

        ESP_ERROR_CHECK(esp_event_loop_create_default());
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL)); //-> just here to get rid of startup error
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();       //= WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

        wifi_config_t wifi_config = {
            .ap = {
                .ssid = "ESP32_Server",
                .ssid_len = strlen("ESP32_Server"),
                .password = "ESP32_Server",
                .max_connection = 1,
                .authmode = WIFI_AUTH_WPA_WPA2_PSK},
        };

        if (strlen("ESP32_Server") == 0) //EXAMPLE_ESP_WIFI_PASS
        {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));

        tcpip_adapter_ip_info_t ip_info;
        IP4_ADDR(&ip_info.ip, 192, 168, 1, 1);
        IP4_ADDR(&ip_info.gw, 192, 168, 1, 1);
        IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

        tcpip_adapter_ap_start((uint8_t *)"9C:B6:D0:E7:30:0F", &ip_info);

        ESP_ERROR_CHECK(esp_wifi_start());
    }

    start_webserver();
}

void app_main()
{
    //set RGB PINs
    gpio_pad_select_gpio(LED_R);
    gpio_pad_select_gpio(LED_G);
    gpio_pad_select_gpio(LED_B);
    
    gpio_set_direction(LED_R, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_G, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_B, GPIO_MODE_OUTPUT);

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");

    wifi_init_softap();
}


#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <esp_log.h>
#include <esp_err.h>
#include <cJSON.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_mac.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"


#include "lwip/err.h"
#include "lwip/sys.h"
#include "mdns.h"
#include "dht11.h"
#include "sdkconfig.h"
#include "esp_sntp.h"
#include "time.h"
#include "esp_event.h"


#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY
#define QUERY_MAX_LEN 256
#define RED "ESP"
#define PASS ""
#define CONFIGURAR 27
#define BUTTON 14



#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif



static EventGroupHandle_t s_wifi_event_group;


#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define LED_GPIO 19 // GPIO donde está conectado el LED (ejemplo GPIO 2)
#define LED_ON 0   // Nivel lógico para encender el LED (0 o 1 dependiendo de la conexión del LED)
#define LED_OFF 1  // Nivel lógico para apagar el LED (0 o 1 dependiendo de la conexión del LED)

#define MQTT_LOGS_SIZE 512
char mqtt_logs[MQTT_LOGS_SIZE];

static const char *TAG = "ESP";
static const char *TAGMQTT = "INFO_MQTT";

static int s_retry_num = 0;
int val;

uint8_t global_ssid[32]={0};
uint8_t global_password[64]={0};

int temperature; 
int humidity;
int humedadsuelo;
bool regar;
bool alarmariego;
static esp_mqtt_client_handle_t client;
struct dht11_reading dht11_data;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAGMQTT, "Last error %s: 0x%x", message, error_code);
    }
}
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
    ESP_LOGD(TAGMQTT, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAGMQTT, "MQTT_EVENT_CONNECTED");

        msg_id = esp_mqtt_client_subscribe(client, "/emqx/recive", 0);
        ESP_LOGI(TAGMQTT, "sent subscribe successful, msg_id=%d", msg_id);
       
        break;
    case MQTT_EVENT_DISCONNECTED:                           
        ESP_LOGI(TAGMQTT, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAGMQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAGMQTT, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAGMQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAGMQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAGMQTT, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAGMQTT, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAGMQTT, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAGMQTT, "Other event id:%d", event->event_id);
        break;
    }}
//////////////////////////////////
static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "ws://test.mosquitto.org:8080/mqtt", // Replace with your local broker's IP address and port
        .credentials.authentication.password = "",         // If your broker requires authentication, provide the credentials here
        .credentials.username = ""
    };

    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
void get_current_time(char *time_buf, size_t buf_size) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(time_buf, buf_size, "%H:%M:%S", &timeinfo); // Format time as HH:MM:SS

  //  printf("Current time: %s\n", time_buf);
}
void publish_sensor_state(esp_mqtt_client_handle_t client) {
    char message[16];
    char log_entry[128];
    int msg_id;

    // Clear previous logs
    memset(mqtt_logs, 0, sizeof(mqtt_logs));

    snprintf(message, sizeof(message), "%d", humidity);
    msg_id = esp_mqtt_client_publish(client, "humidity", message, 0, 0, 0);
    snprintf(log_entry, sizeof(log_entry), "I (%ld) INFO_MQTT: sent publish humidity, msg_id=%d<br>", esp_log_timestamp(), msg_id);
    strncat(mqtt_logs, log_entry, sizeof(mqtt_logs) - strlen(mqtt_logs) - 1);

    snprintf(message, sizeof(message), "%d", temperature);
    msg_id = esp_mqtt_client_publish(client, "temperature", message, 0, 0, 0);
    snprintf(log_entry, sizeof(log_entry), "I (%ld) INFO_MQTT: sent publish temperature, msg_id=%d<br>", esp_log_timestamp(), msg_id);
    strncat(mqtt_logs, log_entry, sizeof(mqtt_logs) - strlen(mqtt_logs) - 1);

    snprintf(message, sizeof(message), "%d", regar);
    msg_id = esp_mqtt_client_publish(client, "regar", message, 0, 0, 0);
    snprintf(log_entry, sizeof(log_entry), "I (%ld) INFO_MQTT: sent publish regar, msg_id=%d<br>", esp_log_timestamp(), msg_id);
    strncat(mqtt_logs, log_entry, sizeof(mqtt_logs) - strlen(mqtt_logs) - 1);

    snprintf(message, sizeof(message), "%d", alarmariego);
    msg_id = esp_mqtt_client_publish(client, "alarmariego", message, 0, 0, 0);
    snprintf(log_entry, sizeof(log_entry), "I (%ld) INFO_MQTT: sent publish alarmariego, msg_id=%d<br>", esp_log_timestamp(), msg_id);
    strncat(mqtt_logs, log_entry, sizeof(mqtt_logs) - strlen(mqtt_logs) - 1);

    snprintf(message, sizeof(message), "%d", humedadsuelo);
    msg_id = esp_mqtt_client_publish(client, "humedadsuelo", message, 0, 0, 0);
    snprintf(log_entry, sizeof(log_entry), "I (%ld) INFO_MQTT: sent publish humedadsuelo, msg_id=%d<br>", esp_log_timestamp(), msg_id);
    strncat(mqtt_logs, log_entry, sizeof(mqtt_logs) - strlen(mqtt_logs) - 1);

    // Get current time
    char current_time[9];
    get_current_time(current_time, sizeof(current_time));

    // Publish current time
    snprintf(message, sizeof(message), "%s", current_time);
    msg_id = esp_mqtt_client_publish(client, "current_tiempo", message, 0, 0, 0);
    snprintf(log_entry, sizeof(log_entry), "I (%ld) INFO_MQTT: sent publish current_time, msg_id=%d<br>", esp_log_timestamp(), msg_id);
    strncat(mqtt_logs, log_entry, sizeof(mqtt_logs) - strlen(mqtt_logs) - 1);
}
void leer_Dht(){
    
    dht11_data = DHT11_read();

    if (dht11_data.status == DHT11_OK) {
                                printf("Humedad: %d%%, Temperatura: %d°C\n", dht11_data.humidity, dht11_data.temperature);
                            
                                humidity = dht11_data.humidity;
                                temperature = dht11_data.temperature;
                            
                            } else {
                                printf("Error al leer datos del sensor DHT11\n");
                }

                }
void initialize_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org"); // Set NTP server
    sntp_init();

    setenv("TZ", "ART3", 1); // ART is Argentina Time, and 3 hours behind UTC
    tzset(); // Apply the time zone setting
}
void update_time_task(void *pvParameters) { 
    time_t now;
    struct tm timeinfo;

    while (1) {
        time(&now);
        localtime_r(&now, &timeinfo);
        if (timeinfo.tm_year >= (2021 - 1900)) {
            break;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    // Main loop to update time and check for hourly update
    while (1) {
        time(&now);
        localtime_r(&now, &timeinfo);

        // Check if it's the top of the hour (minute == 0)
        if ( timeinfo.tm_min == 0 && timeinfo.tm_sec == 0) { //timeinfo.tm_min == 0 &&
         
            // Wait for a minute to avoid multiple triggers within the same minute
            vTaskDelay(60000 / portTICK_PERIOD_MS);
        } else {
            // Wait for one second before checking the time again
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}
esp_err_t send_data(httpd_req_t *req) {
    // Set content type to application/json
    httpd_resp_set_type(req, "application/json");
    // Set cache control to no-cache
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");

    // Get current time
    char current_time[9]; // Buffer to hold time (HH:MM:SS), adjusted for seconds
    get_current_time(current_time, sizeof(current_time));

    // Construct the JSON string
    char json_buf[1024]; // Adjust buffer size as needed
    snprintf(json_buf, sizeof(json_buf), "{\"temperature\": %d, \"humidity\": %d, \"humedadsuelo\": %d, \"regar\": %s, \"alarmariego\": %s, \"time\": \"%s\", \"logs\": \"%s\"}",
             temperature, humidity, humedadsuelo, regar ? "true" : "false", alarmariego ? "true" : "false", current_time, mqtt_logs);

    // Send the JSON
    httpd_resp_send(req, json_buf, strlen(json_buf));

    return ESP_OK;
}



void pin_config()
{
    gpio_set_pull_mode(CONFIGURAR, GPIO_PULLUP_ONLY);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(BUTTON, GPIO_PULLUP_ONLY);
    DHT11_init(4); 

}
esp_err_t init_spiffs(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/storage",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS (%d)", ret);
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS info (%d)", ret);
        return ret;
    }

    ESP_LOGI(TAG, "SPIFFS - Total: %d, Used: %d", total, used);

    esp_err_t ret2 = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret2);  

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    

    return ESP_OK;
}
void print_file_content(const char *filename) {
    ESP_LOGI(TAG, "Opening file %s", filename);
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory");
        fclose(file);
        return;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';

    fclose(file);

    ESP_LOGI(TAG, "File content:");
    ESP_LOGI(TAG, "%s", buffer);

    free(buffer);
}
esp_err_t read_password()
{

    FILE *file = fopen("/storage/config2.json", "rb");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Error al abrir el archivo config2.json");
        return ESP_FAIL;
    }

    fseek(file, 0, SEEK_END);
    size_t json_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *json_content = (char *)malloc(json_size + 1);
    if (json_content == NULL)
    {
        ESP_LOGE(TAG, "Error al reservar memoria para el contenido del JSON");
        fclose(file);
        return ESP_FAIL;
    }

    fread(json_content, 1, json_size, file);
    json_content[json_size] = '\0';

    fclose(file);

    cJSON *json = cJSON_Parse(json_content);
    
    if (json == NULL)
    {
        ESP_LOGE(TAG, "Error al analizar el JSON");
        free(json_content);
        return ESP_FAIL;
    }

    cJSON *red_json = cJSON_GetObjectItem(json, "Red");
    cJSON *pass_json = cJSON_GetObjectItem(json, "Pass");

    if (red_json == NULL || pass_json == NULL || !cJSON_IsString(red_json) || !cJSON_IsString(pass_json))
    {
        ESP_LOGE(TAG, "Error al obtener los valores de Red y Pass del JSON");
        cJSON_Delete(json);
        free(json_content);
        return ESP_FAIL;
    }

    // Convertir las cadenas de caracteres de red y contraseña en matrices de bytes
    memcpy(global_ssid, red_json->valuestring, sizeof(global_ssid));
    memcpy(global_password, pass_json->valuestring, sizeof(global_password));

    cJSON_Delete(json);
    free(json_content);

    return ESP_OK;
}
static void wifi_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data){
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}
void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = RED,
            .ssid_len = strlen(RED),
            .channel = 1,
            .password = PASS,
            .max_connection = 4,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else 
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    if (strlen(PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             RED, PASS, 1);
}
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}
void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

   wifi_config_t wifi_config = {
    .sta = {
        .ssid = {0}, //global_ssid,
        .password = {0}, //global_password,
        .threshold = {
            .authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD
        },
        .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
        .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
     }
    };


    // Copia el contenido de global_ssid y global_password a las matrices en wifi_config
    memcpy(wifi_config.sta.ssid, global_ssid, sizeof(global_ssid));
    memcpy(wifi_config.sta.password, global_password, sizeof(global_password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 global_ssid, global_password);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 global_ssid,global_password);
        ESP_LOGW(TAG, "Fallo modo estacion, pasando a AP");
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

  
}
esp_err_t read_index_html(char **html_content, size_t *html_size){


    FILE *file = fopen("/storage/index.html", "rb");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Error al abrir el archivo index.html");
        return ESP_FAIL;
    }

    fseek(file, 0, SEEK_END);
    *html_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *html_content = (char *)malloc(*html_size + 1);
    if (*html_content == NULL)
    {
        ESP_LOGE(TAG, "Error al reservar memoria para el contenido del archivo");
        fclose(file);
        return ESP_FAIL;
    }

    fread(*html_content, 1, *html_size, file);
    (*html_content)[*html_size] = '\0';

    fclose(file);

    return ESP_OK;
}
esp_err_t read_config_html(char **html_content, size_t *html_size)
{
    FILE *file = fopen("/storage/redconfig.html", "rb");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Error al abrir el archivo config.html");
        return ESP_FAIL;
    }

    fseek(file, 0, SEEK_END);
    *html_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *html_content = (char *)malloc(*html_size + 1);
    if (*html_content == NULL)
    {
        ESP_LOGE(TAG, "Error al reservar memoria para el contenido del archivo");
        fclose(file);
        return ESP_FAIL;
    }

    fread(*html_content, 1, *html_size, file);
    (*html_content)[*html_size] = '\0';

    fclose(file);

    return ESP_OK;
}
esp_err_t read_style(char **html_content, size_t *html_size)
{
    FILE *file = fopen("/storage/style.css", "rb");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Error al abrir el archivo style.css");
        return ESP_FAIL;
    }

    fseek(file, 0, SEEK_END);
    *html_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *html_content = (char *)malloc(*html_size + 1);
    if (*html_content == NULL)
    {
        ESP_LOGE(TAG, "Error al reservar memoria para el contenido del archivo");
        fclose(file);
        return ESP_FAIL;
    }

    fread(*html_content, 1, *html_size, file);
    (*html_content)[*html_size] = '\0';

    fclose(file);

    return ESP_OK;
}
esp_err_t read_codejs(char **html_content, size_t *html_size)
{
    FILE *file = fopen("/storage/app.js", "rb");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "Error al abrir el archivo app.js");
        return ESP_FAIL;
    }

    fseek(file, 0, SEEK_END);
    *html_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *html_content = (char *)malloc(*html_size + 1);
    if (*html_content == NULL)
    {
        ESP_LOGE(TAG, "Error al reservar memoria para el contenido del archivo");
        fclose(file);
        return ESP_FAIL;
    }

    fread(*html_content, 1, *html_size, file);
    (*html_content)[*html_size] = '\0';

    fclose(file);

    return ESP_OK;
}
esp_err_t root_handler(httpd_req_t *req){
    char *html_content;
    size_t html_size;
    esp_err_t ret = read_index_html(&html_content, &html_size);
    if (ret != ESP_OK)
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_content, html_size);

    free(html_content);

    return ESP_OK;}
esp_err_t root_config(httpd_req_t *req){
    char *html_content;
    size_t html_size;
    esp_err_t ret = read_config_html(&html_content, &html_size);
    if (ret != ESP_OK)
    {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_content, html_size);

    free(html_content);

    return ESP_OK;}
esp_err_t root_style_handler(httpd_req_t *req) {
    char *html_content;
    size_t html_size;
    esp_err_t ret = read_style(&html_content, &html_size);
    if (ret != ESP_OK) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, html_content, html_size);

    free(html_content);

    return ESP_OK;
}
esp_err_t root_codejs_handler(httpd_req_t *req) {
    char *html_content;
    size_t html_size;
    esp_err_t ret = read_codejs(&html_content, &html_size);
    if (ret != ESP_OK) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, html_content, html_size);

    free(html_content);

    return ESP_OK;
}
void print_wifi_mode(void)
{
    wifi_mode_t mode;
    if (esp_wifi_get_mode(&mode) == ESP_OK)
    {
        switch (mode)
        {
        case WIFI_MODE_NULL:
            ESP_LOGI(TAG, "Wi-Fi mode: NULL");
            break;
        case WIFI_MODE_STA:
            ESP_LOGI(TAG, "Wi-Fi mode: Station");
            break;
        case WIFI_MODE_AP:
            ESP_LOGI(TAG, "Wi-Fi mode: Access Point");
            break;
        case WIFI_MODE_APSTA:
            ESP_LOGI(TAG, "Wi-Fi mode: Station + Access Point");
            break;
        default:
            ESP_LOGI(TAG, "Wi-Fi mode: Unknown");
            break;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Failed to get Wi-Fi mode");
    }
}
esp_err_t save_config_to_file(const char *ssid, const char *password)
{
    ESP_LOGI(TAG, "Guardando configuración en el archivo");

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Error creando objeto JSON");
        return ESP_FAIL;
    }

    cJSON_AddStringToObject(root, "Red", ssid);
    
    // Limpiar la cadena de la contraseña de caracteres no imprimibles
    char cleaned_password[strlen(password) + 1]; // +1 para el carácter nulo final
    char *cleaned_ptr = cleaned_password;
    for (const char *ptr = password; *ptr; ptr++) {
        if (isprint((unsigned char)*ptr)) {
            *cleaned_ptr++ = *ptr;
        }
    }
    *cleaned_ptr = '\0';

    cJSON_AddStringToObject(root, "Pass", cleaned_password);

    char *json_str = cJSON_Print(root);
    if (json_str == NULL) {
        ESP_LOGE(TAG, "Error convirtiendo JSON a cadena");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    FILE *file = fopen("/storage/config.json", "w");
    if (file == NULL) {
        ESP_LOGE(TAG, "Error abriendo archivo config.json para escritura");
        free(json_str);
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    fprintf(file, "%s", json_str);

    fclose(file);
    free(json_str);
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Configuración guardada correctamente");

    return ESP_OK;
}
esp_err_t config_handler(httpd_req_t *req)
{

    char buf[100];
    int ret, remaining = req->content_len;
    char ssid[32] = "";
    char password[64] = "";
    char *delim = "&=";

    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, sizeof(buf))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                return ESP_OK;
            }
            return ESP_FAIL;
        }

        char *token = strtok(buf, delim);
        while (token != NULL) {
            if (strcmp(token, "red") == 0) {
                token = strtok(NULL, delim); // Saltar "="
                strncpy(ssid, token, sizeof(ssid) - 1);
                ssid[sizeof(ssid) - 1] = '\0'; // Asegurar terminación nula
            } else if (strcmp(token, "contrasena") == 0) {
                token = strtok(NULL, delim); // Saltar "="
                strncpy(password, token, sizeof(password) - 1);
                password[sizeof(password) - 1] = '\0'; // Asegurar terminación nula
            }
            token = strtok(NULL, delim); // Avanzar al siguiente par clave-valor
        }

        remaining -= ret;
    }

    ESP_LOGI(TAG, "Red recibida: %s", ssid);
    ESP_LOGI(TAG, "Contraseña recibida: %s", password);

    save_config_to_file(ssid, password); // Llamar a save_config_to_file con los valores recibidos

    return ESP_OK;
}
esp_err_t toggle_regar_handler(httpd_req_t *req) {
 
    regar = !regar;
    gpio_set_level(LED_GPIO, regar);

    // Send the new state as response
    const char *regar_state = regar ? "ON" : "OFF";
    httpd_resp_send(req, regar_state, strlen(regar_state));

    printf("Regar: %s\n", regar_state);

    return ESP_OK;
}
void iniciar_wifi()
{  
    bool configurar = gpio_get_level(CONFIGURAR) == 1; // Verificar si el pin de configuración está en alto (1)

    if (!configurar)
    {
        wifi_init_softap();
    }
    else
    {
        wifi_init_sta();
    }
    
}
static esp_err_t set_adc(void)
{
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);
    adc1_config_width(ADC_WIDTH_BIT_12);
    return ESP_OK;
}
esp_err_t leerHumedadSuelo(){

   humedadsuelo =  (adc1_get_raw(ADC1_CHANNEL_4)/40);

   
   if(humedadsuelo>50){alarmariego=0;}
   if(humedadsuelo<50){alarmariego=1;}

    return ESP_OK;

}
void timer_callback(TimerHandle_t xTimer) {
    
    leer_Dht();
    leerHumedadSuelo();
    publish_sensor_state(client);
    }
void http_server_task(void *pvParameter)
{
        httpd_handle_t server = NULL;  
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();

         config.max_uri_handlers = 16;

        if (httpd_start(&server, &config) == ESP_OK)
        {
                wifi_mode_t wifi_mode;
                esp_wifi_get_mode(&wifi_mode);

                if (wifi_mode == WIFI_MODE_STA) {
                    // If in station mode, register root_handler
                    httpd_uri_t root = {
                        .uri = "/",
                        .method = HTTP_GET,
                        .handler = root_handler,
                        .user_ctx = NULL
                    };
                    httpd_register_uri_handler(server, &root);
                } 
                if (wifi_mode == WIFI_MODE_AP) {
                    // If in AP mode, register root_config
                    httpd_uri_t config_uri = {
                        .uri = "/",
                        .method = HTTP_GET,
                        .handler = root_config,
                        .user_ctx = NULL
                    };
                    httpd_register_uri_handler(server, &config_uri);
                }
                    httpd_uri_t root_st = {
                        .uri = "/style.css",
                        .method = HTTP_GET,
                        .handler = root_style_handler,
                        .user_ctx = NULL
                    };
                    httpd_register_uri_handler(server, &root_st);
                            
                    httpd_uri_t post_uri = {
                        .uri = "/guardar",
                        .method = HTTP_POST,
                        .handler = config_handler,
                        .user_ctx = NULL
                    };
                    httpd_register_uri_handler(server, &post_uri);
                                                                 
                    httpd_uri_t root_codejs = {
                        .uri = "/app.js",
                        .method = HTTP_GET,
                        .handler = root_codejs_handler,
                        .user_ctx = NULL
                    };
                    httpd_register_uri_handler(server, &root_codejs);

                    httpd_uri_t toggle_regar_uri = {
                        .uri = "/regar",
                        .method = HTTP_GET,
                        .handler = toggle_regar_handler,
                        .user_ctx = NULL
                    };
                    httpd_register_uri_handler(server, &toggle_regar_uri);
            
                    httpd_uri_t send_data_uri_get = {
                        .uri       = "/data",
                        .method    = HTTP_GET,
                        .handler   = send_data,
                        .user_ctx  = NULL
                    };
                    httpd_register_uri_handler(server, &send_data_uri_get);

                      while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

                          }
        }

void app_main(void)
{
    // INICIALIZACION GLOBAL DE LA ESP
    pin_config();    
    init_spiffs();    
    read_password();    
    iniciar_wifi();
    set_adc();
    leerHumedadSuelo(); 
    initialize_sntp();
    // configuracion del DMS//
    mdns_init();
    mdns_hostname_set("esp");
    // INICIALIZACION DEL SISTEMA MQTT
    ESP_LOGI(TAGMQTT, "[APP] Startup..");
    ESP_LOGI(TAGMQTT, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAGMQTT, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_WS", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
       
    mqtt_app_start();
    
    TimerHandle_t xTimer = xTimerCreate(
    "MyTimer",                  // Nombre del temporizador
    pdMS_TO_TICKS(2000),        // Periodo de 500ms convertido a ticks
    pdTRUE,                     // Modo de temporizador repetitivo
    NULL,                       // Parámetro de la función de devolución de llamada
    timer_callback              // Función de devolución de llamada
);
    xTimerStart(xTimer, 0);
  
    xTaskCreate(update_time_task, "update_time_task", 4096, NULL, 5, NULL); 
    // Crear tarea para el núcleo 1
    xTaskCreatePinnedToCore(&http_server_task, "http_server_task", 4096, NULL, 5, NULL, 1);

    vTaskDelay(pdMS_TO_TICKS(100));
}


#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h" // disable brownout problems
#include "soc/rtc_cntl_reg.h" // disable brownout problems
#include "esp_http_server.h"
#include <ESPmDNS.h>

const char* ssid = "kfaryarok3";
const char* password = "edcr66tgvv90";

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

httpd_handle_t stream_httpd = NULL;
bool isStreaming = false;

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char part_buf[64];
    res = httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=frame");
    if (res != ESP_OK) {
        return res;
    }
    isStreaming = true;
    while (isStreaming) {
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            return ESP_FAIL;
        }
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
        size_t hlen = snprintf(part_buf, sizeof(part_buf), "Content-Type:image/jpeg\r\nContent-Length: %u\r\n\r\n", _jpg_buf_len);
        res = httpd_resp_send_chunk(req, part_buf, hlen);
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, "\r\n--frame\r\n", 12);
        }
        esp_camera_fb_return(fb);
        if (res != ESP_OK) {
            break;
        }
    }
    return httpd_resp_send_chunk(req, NULL, 0);
}

static esp_err_t start_stream_handler(httpd_req_t *req) {
    isStreaming = true;
    return httpd_resp_send(req, "Stream started", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t stop_stream_handler(httpd_req_t *req) {
    isStreaming = false;
    return httpd_resp_send(req, "Stream stopped", HTTPD_RESP_USE_STRLEN);
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = [](httpd_req_t *req){
            const char* resp = R"rawliteral(
            <html>
            <head>
            <title>ESP32-CAM Live Stream</title>
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <style>
            body { font-family: Arial; text-align: center; margin: 0 auto; padding-top: 30px;}
            #stream-container { display: inline-block; position: relative; max-width: 100%;overflow: hidden; }
            #stream { max-width: 100%; max-height: 70vh; margin: 0 auto; transform-origin:center; display: none; }
            .button { margin-top: 20px; padding: 10px 20px; font-size: 16px; }
            #status-message { font-size: 20px; color: red; display: none; }
            </style>
            </head>
            <body>
            <h1>ESP32-CAM Live Stream</h1>
            <button class="button" onclick="startStream()">Start Streaming</button>
            <button class="button" onclick="stopStream()">Stop Streaming</button>
            <button class="button" onclick="rotateImage()">Rotate Image</button>
            <br><br>
            <div id="stream-container">
            <img id="stream" src="" alt="">
            <p id="status-message">Stream stopped</p>
            </div>
            <script>
            let rotation = 0;
            function startStream() {
            document.getElementById('stream').style.display = "block";
            document.getElementById('status-message').style.display = "none";
            document.getElementById('stream').src = "stream";
            }
            function stopStream() {
            document.getElementById('stream').src = "";
            document.getElementById('stream').style.display = "none";
            document.getElementById('status-message').style.display = "block";
            fetch('/stop_stream');
            }
            function rotateImage() {
            rotation = (rotation + 90) % 360;
            document.getElementById('stream').style.transform =
            `rotate(${rotation}deg)`;
            }
            </script>
            </body>
            </html>
            )rawliteral";
            return httpd_resp_send(req, resp, strlen(resp));
        },
        .user_ctx = NULL
    };

    httpd_uri_t stream_uri = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL
    };

    httpd_uri_t start_stream_uri = {
        .uri = "/start_stream",
        .method = HTTP_GET,
        .handler = start_stream_handler,
        .user_ctx = NULL
    };

    httpd_uri_t stop_stream_uri = {
        .uri = "/stop_stream",
        .method = HTTP_GET,
        .handler = stop_stream_handler,
        .user_ctx = NULL
    };

    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
        httpd_register_uri_handler(stream_httpd, &stream_uri);
        httpd_register_uri_handler(stream_httpd, &start_stream_uri);
        httpd_register_uri_handler(stream_httpd, &stop_stream_uri);
    } else {
        Serial.println("Failed to start web server!");
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    if (!MDNS.begin("esp32cam")) {
        Serial.println("Error starting mDNS");
        return;
    }
    Serial.println("mDNS responder started");

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_SVGA; // HD resolution FRAMESIZE_HD
    config.jpeg_quality = 12; // Higher quality 10
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    startCameraServer();
    Serial.println("Camera Stream Ready! Go to: http://esp32cam.local/");
}

void loop() {
    // No mDNS service update required
    // Simple loop to keep processor free
}

#include "esp_camera.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <lvgl.h>
#include "ui.h"

// GPIO 引脚定义
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


TFT_eSPI tft = TFT_eSPI();

static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[screenWidth * screenHeight / 13];

void cameraInit() {
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;  // 10 MHz
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);  //初始化失败
    while (1)
      ;  // 停止程序
  }
}

void showingImage() {
  camera_fb_t *fb = esp_camera_fb_get();  // 获取帧缓冲
  if (!fb) {
    Serial.println("Failed to capture image");  // 无法捕获图片
    return;
  }

  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("no JPEG format");  // 非JPEG
  } else {
    Serial.println("Captured JPEG image");

    int offsetX = 0;   //左右偏移 +右 -左
    int offsetY = 80;  //上下偏移 +下 -上

    if (TJpgDec.drawJpg(offsetX, offsetY, (const uint8_t *)fb->buf, fb->len)) {
      Serial.println("JPEG drawn successfully.");
    } else {
      Serial.println("JPEG draw failed.");  // 超出画框
    }
  }

  esp_camera_fb_return(fb);  // 释放缓冲
}

//lvgl flush
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {

  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();

  int16_t x1 = screenWidth - area->x2 - 1;
  int16_t y1 = screenHeight - area->y2 - 1;

  tft.setAddrWindow(x1, y1, w, h);

  for (uint32_t i = 0; i < w * h; i++) {
    uint16_t color = color_p[w * h - 1 - i].full;  // 倒序绘制
    tft.pushColor(color);
  }

  tft.endWrite();

  lv_disp_flush_ready(disp);
}

//touch
uint16_t touchX, touchY;
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  bool touched = tft.getTouch(&touchX, &touchY, 600);
  if (!touched) {
    data->state = LV_INDEV_STATE_REL;
  } else {
    data->state = LV_INDEV_STATE_PR;

    data->point.x = screenWidth - touchX - 1;
    data->point.y = screenHeight - touchY - 1;

    Serial.print("Data x: ");
    Serial.println(data->point.x);
    Serial.print("Data y: ");
    Serial.println(data->point.y);
  }
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

void displayInit() {
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  uint16_t calData[5] = { 353, 3568, 269, 3491, 7 };
  tft.setTouch(calData);

  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
}


void lvglInit() {
  lv_init();

  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, screenWidth * screenHeight / 13);

  /* Initialize the display */
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /* Change the following line to your display resolution */
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  //旋转
  // disp_drv.sw_rotate = 1;  // If you turn on software rotation, Do not update or replace LVGL
  // disp_drv.rotated = LV_DISP_ROT_180;
  // disp_drv.full_refresh = 1;  // full_refresh must be 1

  /* Initialize the (dummy) input device driver */
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  tft.fillScreen(TFT_BLACK);

  // lv_demo_widgets();  // LVGL demo
  ui_init();
}

void cameraTask(void *param) {
  displayInit();
  cameraInit();
  while (1) {
    showingImage();
  }
}


void lvglTask(void *param) {
  lvglInit();
  while (1) {
    lv_timer_handler();
    lv_tick_inc(5);  //放置5ms过去
    vTaskDelay(5);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(4, OUTPUT);

  xTaskCreate(lvglTask, "lvgl", 2048, NULL, 1, NULL);
  xTaskCreate(cameraTask, "camera", 2048, NULL, 0, NULL);
}

void loop() {
}
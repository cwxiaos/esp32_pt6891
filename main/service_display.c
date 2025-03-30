#include "service_display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "lvgl.h"
#include "lv_demos.h"

#include "driver_pt6891.h"

#define OLED_SPI_HOST SPI2_HOST

#define OLED_CLOCK_HZ (27 * 1000 * 1000)
#define OLED_PIN_CS 15
#define OLED_PIN_SCLK 14
#define OLED_PIN_MISO 12
#define OLED_PIN_MOSI 13
#define OLED_PIN_DC 11
#define OLED_PIN_RST 10

#define OLED_WIDTH 95
#define OLED_HEIGHT 28

#define OLED_CMD_BITS 8
#define OLED_PARAM_BITS 8

#define LVGL_TICK_PERIOD_MS    2
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 1
#define LVGL_TASK_STACK_SIZE   (4 * 1024)
#define LVGL_TASK_PRIORITY     2

static const char* TAG = "service_display";

static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_disp_draw_buf_t disp_draw_buf;
static lv_disp_drv_t disp_drv;

static SemaphoreHandle_t lvgl_mux = NULL;


static bool notify_flush_ready(esp_lcd_panel_io_handle_t panel_io_handle, esp_lcd_panel_io_event_data_t* panel_io_event_data, void* user_ctx) {
    lv_disp_drv_t* disp_driver = (lv_disp_drv_t*) user_ctx;
    lv_disp_flush_ready(disp_driver);

    return false;
}

static void disp_flush(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    // ESP_LOGI(TAG, "flush %d %d %d %d", area->x1, area->y1, area->x2, area->y2);
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2, area->y2, color_p);
}

static void lvgl_port_update_callback(lv_disp_drv_t *drv){
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;

    // switch (drv->rotated) {
    // case LV_DISP_ROT_NONE:
    //     // Rotate LCD display
    //     esp_lcd_panel_swap_xy(panel_handle, false);
    //     esp_lcd_panel_mirror(panel_handle, true, false);
    //     break;
    // case LV_DISP_ROT_90:
    //     // Rotate LCD display
    //     esp_lcd_panel_swap_xy(panel_handle, true);
    //     esp_lcd_panel_mirror(panel_handle, true, true);
    //     break;
    // case LV_DISP_ROT_180:
    //     // Rotate LCD display
    //     esp_lcd_panel_swap_xy(panel_handle, false);
    //     esp_lcd_panel_mirror(panel_handle, false, true);
    //     break;
    // case LV_DISP_ROT_270:
    //     // Rotate LCD display
    //     esp_lcd_panel_swap_xy(panel_handle, true);
    //     esp_lcd_panel_mirror(panel_handle, false, false);
    //     break;
    // }
}


static void increase_lvgl_tick(void* arg) {
    // ESP_LOGI(TAG, "Increase LVGL tick");
    lv_tick_inc(2);
}

bool lvgl_lock(int timeout_ms){
    // Convert timeout in milliseconds to FreeRTOS ticks
    // If `timeout_ms` is set to -1, the program will block until the condition is met
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void lvgl_unlock(void){
    xSemaphoreGiveRecursive(lvgl_mux);
}

static void lvgl_port_task(void* arg){
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t task_delay_ms = 1;
    while (1) {
        // Lock the mutex due to the LVGL APIs are not thread-safe
        if (lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            // Release the mutex
            lvgl_unlock();
        }
        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

void panel_init() {
    ESP_LOGI(TAG, "Panel: Init SPI Bus");
    spi_bus_config_t spi_bus_config = {
        .mosi_io_num = OLED_PIN_MOSI,
        .miso_io_num = OLED_PIN_MISO,
        .sclk_io_num = OLED_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = OLED_WIDTH * OLED_HEIGHT * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(OLED_SPI_HOST, &spi_bus_config, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Panel: Init Panel IO");
    esp_lcd_panel_io_handle_t panel_io_handle = NULL;
    esp_lcd_panel_io_spi_config_t panel_io_spi_config = {
        .cs_gpio_num = OLED_PIN_CS,
        .dc_gpio_num = OLED_PIN_DC,
        .spi_mode = 0,
        .pclk_hz = OLED_CLOCK_HZ,
        .trans_queue_depth = 10,
        .on_color_trans_done = notify_flush_ready,
        // .on_color_trans_done = NULL,
        .user_ctx = &disp_drv,
        .lcd_cmd_bits = OLED_CMD_BITS,
        .lcd_param_bits = OLED_PARAM_BITS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)OLED_SPI_HOST, &panel_io_spi_config, &panel_io_handle));

    ESP_LOGI(TAG, "Panel: Install Panel Driver");
    esp_lcd_panel_dev_config_t panel_dev_config = {
        .reset_gpio_num = OLED_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_pt6891(panel_io_handle, &panel_dev_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    // ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
}

void lvgl_init() {
    ESP_LOGI(TAG, "LVGL: Init");
    lv_init();

    lv_color_t* buf1 = heap_caps_malloc(OLED_WIDTH * OLED_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t* buf2 = heap_caps_malloc(OLED_WIDTH * OLED_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);

    assert(buf1 != NULL);
    assert(buf2 != NULL);

    lv_disp_draw_buf_init(&disp_draw_buf, buf1, buf2, OLED_WIDTH * OLED_HEIGHT);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = OLED_WIDTH;
    disp_drv.ver_res = OLED_HEIGHT;
    disp_drv.draw_buf = &disp_draw_buf;
    disp_drv.drv_update_cb = lvgl_port_update_callback;
    disp_drv.flush_cb = disp_flush;
    disp_drv.user_data = panel_handle;
    lv_disp_t* disp = lv_disp_drv_register(&disp_drv);
    
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick"
    };

    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 2 * 1000));

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux != NULL);

    xTaskCreate(lvgl_port_task, "lvgl_port_task", 4096, NULL, 2, NULL);

    if (lvgl_lock(-1)) {
        // lv_disp_set_perf_monitor(disp, true);
        lv_demo_benchmark();
        lvgl_unlock();
    }
}

void service_display_main() {
    panel_init();
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, 0, 1));
    esp_lcd_panel_invert_color(panel_handle, true);
    lvgl_init();

    // uint16_t white[100];
    // for (int i = 0; i < 100; i++) {
    //     white[i] = 0xFFFF;
    // }

    // for (int m = 0; m < 2; m++) {
    //     for (int n = 0; n < 2; n++) {
    //         uint16_t black[100] = {0x0000};
    //         ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, m, n));
    //         for (int y = 0; y < 28; y++) {
    //             esp_lcd_panel_draw_bitmap(panel_handle, 0, y, 95, y, black);
    //         }

    //         for (int i = 0; i < 95; i++) {
    //             ESP_LOGI(TAG, "Panel: Draw %d", i);
    //             for (int j = 0; j < 28; j += 2) {
    //                 esp_lcd_panel_draw_bitmap(panel_handle, i, j, i, j, white);
        
    //                 vTaskDelay(10 / portTICK_PERIOD_MS);
    //             }

    //             vTaskDelay(00 / portTICK_PERIOD_MS);
    //         }
    //     }
    // }

}
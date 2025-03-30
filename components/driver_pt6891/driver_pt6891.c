#include "driver_pt6891.h"

#include <stdio.h>

#include "driver/gpio.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_ops.h"
#include "esp_check.h"

#include "oled_panel_cmds.h"

#define PANEL_WIDTH 95
#define PANEL_HEIGHT 28

#define RAM_START_X 0
#define RAM_START_Y 0
#define RAM_END_X 127
#define RAM_END_Y 32
#define RAM_ROW_LEN (RAM_END_X - RAM_START_X)

static const char* TAG = "driver_pt6891";

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t panel_io_handle;
    int reset_gpio_num;
    bool reset_level;
    int x_gap;
    int y_gap;
    uint8_t fb_bits_per_pixel;
    uint8_t endian_val; // Color Endian
    uint8_t colswap_val;// Scan Direction
    uint8_t mirror_val; // Mirror
    uint8_t colmod_val; // Color Mode
    const pt6891_oled_init_cmd_t* init_cmds;
    uint16_t init_cmds_size;
} pt6891_panel_t;

static const pt6891_oled_init_cmd_t vendor_init_cmds_default[] = {
    // Software Reset
    {OLED_CMD_SWRST, (uint8_t []) {0x00}, 0, 0},
    // Display Off
    {OLED_CMD_DISP_T1_OFF, (uint8_t []) {0x00}, 0, 0},
    // Voltage and Current
    {OLED_CMD_VOL_CUR, (uint8_t []) {OLED_SET_VOL_CUR(1, 0, 7)}, 1, 0},
    // Color Mode (RGB565)
    {OLED_CMD_COLOR_RGB565, (uint8_t []) {0x00}, 0, 0},
    // Red Gamma Table
    {OLED_CMD_GAM_RED, (uint8_t []) {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0D, 0x0F, 0x11, 0x13, 0x16, 0x18, 0x1B, 0x1D, 0x20, 0x23, 0x26, 0x29, 0x2C, 0x30, 0x33, 0x37, 0x3A, 0x3E, 0x42, 0x46, 0x4A, 0x4E, 0x53, 0x57, 0x5C, 0x60, 0x65, 0x6A, 0x6F, 0x74, 0x79, 0x7E, 0x83, 0x89, 0x8E, 0x94, 0x9A, 0x9F, 0xA5, 0xAB, 0xB1, 0xB7, 0xBE, 0xC4, 0xCA, 0xD1, 0xD8, 0xDE, 0xE5, 0xEC, 0xF3, 0xFA}, 64, 0},
    // Gamma Table Update
    {OLED_CMD_GAM_UPDATE, (uint8_t []) {0x00}, 0, 0},
    // Green Gamma Table
    {OLED_CMD_GAM_GREEN, (uint8_t []) {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0D, 0x0F, 0x11, 0x13, 0x16, 0x18, 0x1B, 0x1D, 0x20, 0x23, 0x26, 0x29, 0x2C, 0x30, 0x33, 0x37, 0x3A, 0x3E, 0x42, 0x46, 0x4A, 0x4E, 0x53, 0x57, 0x5C, 0x60, 0x65, 0x6A, 0x6F, 0x74, 0x79, 0x7E, 0x83, 0x89, 0x8E, 0x94, 0x9A, 0x9F, 0xA5, 0xAB, 0xB1, 0xB7, 0xBE, 0xC4, 0xCA, 0xD1, 0xD8, 0xDE, 0xE5, 0xEC, 0xF3, 0xFA}, 64, 0},
    // Gamma Table Update
    {OLED_CMD_GAM_UPDATE, (uint8_t []) {0x00}, 0, 0},
    // Blue Gamma Table
    {OLED_CMD_GAM_BLUE, (uint8_t []) {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0D, 0x0F, 0x11, 0x13, 0x16, 0x18, 0x1B, 0x1D, 0x20, 0x23, 0x26, 0x29, 0x2C, 0x30, 0x33, 0x37, 0x3A, 0x3E, 0x42, 0x46, 0x4A, 0x4E, 0x53, 0x57, 0x5C, 0x60, 0x65, 0x6A, 0x6F, 0x74, 0x79, 0x7E, 0x83, 0x89, 0x8E, 0x94, 0x9A, 0x9F, 0xA5, 0xAB, 0xB1, 0xB7, 0xBE, 0xC4, 0xCA, 0xD1, 0xD8, 0xDE, 0xE5, 0xEC, 0xF3, 0xFA}, 64, 0},
    // Gamma Table Update
    {OLED_CMD_GAM_UPDATE, (uint8_t []) {0x00}, 0, 0},
    // COM Number
    {OLED_CMD_COM_NUM, (uint8_t []) {OLED_SET_COM_NUM(28 - 1)}, 1, 0},
    // Display Row Start
    {OLED_CMD_ROW_START, (uint8_t []) {OLED_SET_ROW_START(0)}, 1, 0},
    // Display Column Start
    {OLED_CMD_COL_START, (uint8_t []) {OLED_SET_COL_START(0)}, 1, 0},
    // Dummy Scan
    {OLED_CMD_DUMMY_SCAN_OFF, (uint8_t []) {0x00}, 0, 0},
    // Clock Divide
    {OLED_CMD_CLK_DIV_1, (uint8_t []) {0x00}, 0, 0},
    // OSC Trimming
    {OLED_CMD_OSC_TRIM, (uint8_t []) {OLED_SET_OSC_TRIM(8)}, 1, 0},
    // COM Pulse Width
    {OLED_CMD_COM_PLS_WIDTH, (uint8_t []) {OLED_SET_COM_PLS_WIDTH_BYTE_1(293), OLED_SET_COM_PLS_WIDTH_BYTE_2(293)}, 2, 0},
    // Blank Period
    {OLED_CMD_BLK_PER, (uint8_t []) {OLED_SET_BLK_PER(0, 10)}, 1, 0},
    // Seg EVEN/ODD Swap
    {OLED_CMD_SEG_SWAP, (uint8_t []) {OLED_SET_SEG_SWAP(0)}, 1, 0},
    // Seg Endian Type
    {OLED_CMD_ENDIAN_BGR, (uint8_t []) {0x00}, 0, 0},
    // Screen Mirror
    {OLED_CMD_MIRROR(0, 0), (uint8_t []) {0x00}, 0, 0},
    // Cathode Scan Direction
    {OLED_CMD_CATH_SCAN_L, (uint8_t []) {0x00}, 0, 0},
    // Anode Trimming
    {OLED_CMD_ANODE_TRIM, (uint8_t []) {OLED_SET_ANODE_TRIM(8)}, 1, 0},
    // Brightness
    {OLED_CMD_BRIGHT, (uint8_t []) {OLED_SET_BRIGHT_R(43), OLED_SET_BRIGHT_G(14), OLED_SET_BRIGHT_B(26)}, 3, 0},
    // Precharge Period
    {OLED_CMD_PRECHG_PRD, (uint8_t []) {OLED_SET_PRECHG_PRD(10)}, 1, 0},
    // Precharge Current
    {OLED_CMD_PRECHG_CUR, (uint8_t []) {OLED_SET_PRECHG_CUR(0)}, 1, 0},
    // INT Enable
    {OLED_CMD_INT_EN, (uint8_t []) {0x00}, 0, 0},
    // Normal Display
    {OLED_CMD_DISP_NORMAL, (uint8_t []) {0x00}, 0, 0},
};

/**
 * @brief Screen Pixel Offsets
 * 
 * [H_MIRROR][V_MIRROR][X_START, Y_START, Y_GATE]
 * 
 */
static const int offsets[2][2][3] = {
    {
        {33, 0, 4},
        {33, 4, 24},
    }, {
        {0, 0, 4},
        {0, 4, 24},
    },
};

static esp_err_t panel_pt6891_reset(esp_lcd_panel_t* panel) {
    pt6891_panel_t* pt6891_panel = __containerof(panel, pt6891_panel_t, base);

    if (pt6891_panel -> reset_gpio_num > -1) {
        // Exit Standby Mode First, in which case RST# will be not able to reset the chip
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_WAKEUP, NULL, 0), TAG, "Failed to send command");
        vTaskDelay(10 / portTICK_PERIOD_MS);
        gpio_set_level(pt6891_panel -> reset_gpio_num, pt6891_panel -> reset_level);
        vTaskDelay(10 / portTICK_PERIOD_MS);
        gpio_set_level(pt6891_panel -> reset_gpio_num, !pt6891_panel -> reset_level);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    } else {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_SWRST, NULL, 0), TAG, "Reset failed");
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    return ESP_OK;
}

static esp_err_t panel_pt6891_init(esp_lcd_panel_t* panel) {
    pt6891_panel_t* pt6891_panel = __containerof(panel, pt6891_panel_t, base);

    const pt6891_oled_init_cmd_t* init_cmds = vendor_init_cmds_default;
    uint16_t init_cmds_size = sizeof(vendor_init_cmds_default) / sizeof(pt6891_oled_init_cmd_t);

    if (pt6891_panel -> init_cmds) {
        init_cmds = pt6891_panel -> init_cmds;
        init_cmds_size = pt6891_panel -> init_cmds_size;
    }

    // Send init commands
    for (int i = 0; i < init_cmds_size; i++) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, init_cmds[i].cmd, init_cmds[i].data, init_cmds[i].data_bytes), TAG, "Failed to send init command");

        if (init_cmds[i].cmd == OLED_CMD_PRECHG_CUR) {
            // Clear frame buffer
            uint16_t blank_frame_buffer[RAM_ROW_LEN + 1] = {0x0000};

            for (int j = RAM_START_Y; j < RAM_END_Y ; j++) {
                ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWADDR, (uint8_t []) {OLED_SET_ROWADDR(j)}, 1), TAG, "Failed to send init command");
                ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_COLADDR, (uint8_t []) {OLED_SET_COLADDR(0)}, 1), TAG, "Failed to send init command");
                ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWLEN, (uint8_t []) {OLED_SET_ROWLEN(RAM_ROW_LEN)}, 1), TAG, "Failed to send init command");
                ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_RAMWR, blank_frame_buffer, RAM_ROW_LEN * pt6891_panel -> fb_bits_per_pixel / 8), TAG, "Failed to send init command");
            }

            vTaskDelay(10 / portTICK_PERIOD_MS);
        }

        vTaskDelay(init_cmds[i].delay_ms / portTICK_PERIOD_MS);
    }

    ESP_LOGD(TAG, "Init PT6891 panel: %p", pt6891_panel);

    return ESP_OK;
}

static esp_err_t panel_pt6891_del(esp_lcd_panel_t* panel) {
    pt6891_panel_t* pt6891_panel = __containerof(panel, pt6891_panel_t, base);

    if (pt6891_panel -> reset_gpio_num > -1) {
        gpio_set_level(pt6891_panel -> reset_gpio_num, pt6891_panel -> reset_level);
    }

    ESP_LOGD(TAG, "Del PT6891 panel: %p", pt6891_panel);
    free(pt6891_panel);
    return ESP_OK;
}

static esp_err_t panel_pt6891_draw_bitmap(esp_lcd_panel_t* panel, int x_start, int y_start, int x_end, int y_end, const void* color_data) {
    pt6891_panel_t* pt6891_panel = __containerof(panel, pt6891_panel_t, base);

    if ((x_end < x_start) || (y_end < y_start)) {
        ESP_RETURN_ON_ERROR(ESP_ERR_INVALID_ARG, TAG, "Invalid Drawing Area: [%d, %d, %d, %d]", x_start, y_start, x_end, y_end);
    }

    int x_mirror = !!(pt6891_panel -> mirror_val & 0x02);
    int y_mirror = pt6891_panel -> mirror_val & 0x01;
    int x_offset = offsets[x_mirror][y_mirror][0];
    int y_offset = offsets[x_mirror][y_mirror][1];
    int y_gate = offsets[x_mirror][y_mirror][2];
    int y_gate_offset = PANEL_HEIGHT - y_gate;

    x_start += pt6891_panel -> x_gap + x_offset;
    x_end += pt6891_panel -> x_gap + x_offset;
    y_start += pt6891_panel -> y_gap;
    y_end += pt6891_panel -> y_gap;
    
    if (y_end < y_gate) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWADDR, (uint8_t []) {OLED_SET_ROWADDR(y_start + y_gate_offset + y_offset)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_COLADDR, (uint8_t []) {OLED_SET_COLADDR(x_start)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWLEN, (uint8_t []) {OLED_SET_ROWLEN(x_end - x_start)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(pt6891_panel -> panel_io_handle, OLED_CMD_RAMWR, color_data, (x_end - x_start + 1) * (y_end - y_start + 1) * pt6891_panel -> fb_bits_per_pixel / 8), TAG, "Failed to send color data");
    } else if (y_start > y_gate - 1) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWADDR, (uint8_t []) {OLED_SET_ROWADDR(y_start - y_gate + y_offset)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_COLADDR, (uint8_t []) {OLED_SET_COLADDR(x_start)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWLEN, (uint8_t []) {OLED_SET_ROWLEN(x_end - x_start + 1 - 1)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(pt6891_panel -> panel_io_handle, OLED_CMD_RAMWR, color_data, (x_end - x_start + 1) * (y_end - y_start + 1) * pt6891_panel -> fb_bits_per_pixel / 8), TAG, "Failed to send color data");
    } else {
        uint16_t stage_size = (x_end - x_start + 1) * (y_gate - 1 - y_start + 1)  * pt6891_panel -> fb_bits_per_pixel / 8;
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWADDR, (uint8_t []) {OLED_SET_ROWADDR(y_start + y_gate_offset + y_offset)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_COLADDR, (uint8_t []) {OLED_SET_COLADDR(x_start)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWLEN, (uint8_t []) {OLED_SET_ROWLEN(x_end - x_start)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(pt6891_panel -> panel_io_handle, OLED_CMD_RAMWR, color_data, stage_size), TAG, "Failed to send color data");

        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWADDR, (uint8_t []) {OLED_SET_ROWADDR(y_offset)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_COLADDR, (uint8_t []) {OLED_SET_COLADDR(x_start)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWLEN, (uint8_t []) {OLED_SET_ROWLEN(x_end - x_start)}, 1), TAG, "Failed to send command");
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(pt6891_panel -> panel_io_handle, OLED_CMD_RAMWR, color_data + stage_size, (x_end - x_start + 1) * (y_end - y_gate + 1) * pt6891_panel -> fb_bits_per_pixel / 8), TAG, "Failed to send color data");
    }

    // For debug
    // ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWADDR, (uint8_t []) {OLED_SET_ROWADDR(y_start)}, 1), TAG, "Failed to send command");
    // ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_COLADDR, (uint8_t []) {OLED_SET_COLADDR(x_start)}, 1), TAG, "Failed to send command");
    // ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_ROWLEN, (uint8_t []) {OLED_SET_ROWLEN(x_end - x_start + 1 - 1)}, 1), TAG, "Failed to send command");
    // ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(pt6891_panel -> panel_io_handle, OLED_CMD_RAMWR, color_data, (x_end - x_start + 1) * (y_end - y_start + 1) * pt6891_panel -> fb_bits_per_pixel / 8), TAG, "Failed to send color data");

    return ESP_OK;
}

static esp_err_t panel_pt6891_mirror(esp_lcd_panel_t* panel, bool mirror_x, bool mirror_y) {
    pt6891_panel_t* pt6891_panel = __containerof(panel, pt6891_panel_t, base);
    
    pt6891_panel -> mirror_val = OLED_CMD_MIRROR(mirror_x, mirror_y);

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, pt6891_panel -> mirror_val, NULL, 0), TAG, "Failed to send command");

    return ESP_OK;
}

static esp_err_t panel_pt6891_swap_xy(esp_lcd_panel_t* panel, bool swap_axes) {
    pt6891_panel_t* pt6891_panel = __containerof(panel, pt6891_panel_t, base);

    //TODO: Add software swap, transpose data
    pt6891_panel -> colswap_val = swap_axes;
    ESP_LOGE(TAG, "TODO: Add software swap");

    return ESP_OK;
}

static esp_err_t panel_pt6891_set_gap(esp_lcd_panel_t* panel, int x_gap, int y_gap) {
    pt6891_panel_t* pt6891_panel = __containerof(panel, pt6891_panel_t, base);

    pt6891_panel -> x_gap = x_gap;
    pt6891_panel -> y_gap = y_gap;

    return ESP_OK;
}

static esp_err_t panel_pt6891_invert_color(esp_lcd_panel_t* panel, bool invert_color_data) {
    pt6891_panel_t* pt6891_panel = __containerof(panel, pt6891_panel_t, base);

    uint8_t invert_val = OLED_CMD_BW_INV_OFF;
    if (invert_color_data) {
        invert_val = OLED_CMD_BW_INV_ON;
    }

    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, invert_val, NULL, 0), TAG, "Failed to send command");

    return ESP_OK;
}

static esp_err_t panel_pt6891_disp_on_off(esp_lcd_panel_t* panel, bool on_off) {
    pt6891_panel_t* pt6891_panel = __containerof(panel, pt6891_panel_t, base);

    if (on_off) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_DISP_NORMAL, NULL, 0), TAG, "Failed to send command");
    } else {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_DISP_T1_OFF, NULL, 0), TAG, "Failed to send command");
    }

    return ESP_OK;
}

static esp_err_t panel_pt6891_disp_sleep(esp_lcd_panel_t* panel, bool sleep) {
    pt6891_panel_t* pt6891_panel = __containerof(panel, pt6891_panel_t, base);

    if (sleep) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_STANDBY, NULL, 0), TAG, "Failed to send command");
    } else {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(pt6891_panel -> panel_io_handle, OLED_CMD_WAKEUP, NULL, 0), TAG, "Failed to send command");
    }

    return ESP_OK;
}

esp_err_t esp_lcd_new_panel_pt6891(const esp_lcd_panel_io_handle_t panel_io_handle, const esp_lcd_panel_dev_config_t* panel_dev_config, esp_lcd_panel_handle_t* panel_handle) {
    esp_err_t ret = ESP_OK;
    pt6891_panel_t* pt6891_panel = NULL;

    ESP_GOTO_ON_FALSE(panel_io_handle && panel_dev_config && panel_handle, ESP_ERR_INVALID_ARG, clear, TAG, "Invalid argument");

    pt6891_panel = (pt6891_panel_t*) calloc(1, sizeof(pt6891_panel_t));
    ESP_GOTO_ON_FALSE(pt6891_panel, ESP_ERR_NO_MEM, clear, TAG, "No MEM for pt6891 panel");

    if (panel_dev_config -> reset_gpio_num > -1) {
        gpio_config_t panel_reset_gpio_config = {
            .pin_bit_mask = 1ULL << panel_dev_config -> reset_gpio_num,
            .mode = GPIO_MODE_OUTPUT,
        };
        ESP_GOTO_ON_ERROR(gpio_config(&panel_reset_gpio_config), clear, TAG, "Configure GPIO for RST failed");
    }

    switch (panel_dev_config -> rgb_ele_order) {
        case COLOR_RGB_ELEMENT_ORDER_RGB: {
            pt6891_panel -> endian_val = OLED_CMD_ENDIAN_RGB;
            break;
        }
        case COLOR_RGB_ELEMENT_ORDER_BGR: {
            pt6891_panel -> endian_val = OLED_CMD_ENDIAN_BGR;
            break;
        }
        default: {
            ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, clear, TAG, "Unsupported RGB endian");
        }
    }

    switch (panel_dev_config -> bits_per_pixel) {
        case 16: {  // RGB565
            pt6891_panel -> colmod_val = OLED_CMD_COLOR_RGB565;
            pt6891_panel -> fb_bits_per_pixel = 16;
            break;
        }
        case 18: {  // RGB666
            pt6891_panel -> colmod_val = OLED_CMD_COLOR_RGB666;
            pt6891_panel -> fb_bits_per_pixel = 24;
            break;
        }
        default: {
            ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, clear, TAG, "Unsupported pixel width");
        }
    }

    pt6891_panel -> base.reset = panel_pt6891_reset;
    pt6891_panel -> base.init = panel_pt6891_init;
    pt6891_panel -> base.del = panel_pt6891_del;
    pt6891_panel -> base.draw_bitmap = panel_pt6891_draw_bitmap;
    pt6891_panel -> base.mirror = panel_pt6891_mirror;
    pt6891_panel -> base.swap_xy = panel_pt6891_swap_xy;
    pt6891_panel -> base.set_gap = panel_pt6891_set_gap;
    pt6891_panel -> base.invert_color = panel_pt6891_invert_color;
    pt6891_panel -> base.disp_on_off = panel_pt6891_disp_on_off;
    pt6891_panel -> base.disp_sleep = panel_pt6891_disp_sleep;
    pt6891_panel -> panel_io_handle = panel_io_handle;
    pt6891_panel -> reset_gpio_num = panel_dev_config -> reset_gpio_num;
    pt6891_panel -> reset_level = panel_dev_config -> flags.reset_active_high;
    if (panel_dev_config -> vendor_config) {
        pt6891_panel -> init_cmds = ((pt6891_oled_vendor_init_t*) (panel_dev_config -> vendor_config)) -> init_cmds;
        pt6891_panel -> init_cmds_size = ((pt6891_oled_vendor_init_t*) (panel_dev_config -> vendor_config)) -> num_init_cmds;
    }

    *panel_handle = &(pt6891_panel -> base);

    ESP_LOGD(TAG, "New PT6891 panel: %p", pt6891_panel);
    ESP_LOGI(TAG, "Panel create success");

    return ESP_OK;
    
clear:
    if (pt6891_panel) {
        if (panel_dev_config -> reset_gpio_num > -1) {
            gpio_set_level(panel_dev_config -> reset_gpio_num, panel_dev_config -> flags.reset_active_high);
        }
    }
    
    return ret;
}
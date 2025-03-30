#ifndef __DRIVER_PT6891_H__
#define __DRIVER_PT6891_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

#include "esp_lcd_types.h"
#include "esp_lcd_panel_dev.h"

/**
 * @brief OLED Panel Init Command
 * 
 */
typedef struct {
    int cmd;                // The Specific OLED Command
    const void* data;       // Buffer that holds the data
    size_t data_bytes;      // The number of bytes in the buffer
    unsigned int delay_ms;  // Delay after the command is sent
} pt6891_oled_init_cmd_t;

/**
 * @brief OLED Panel Vendor Config
 * 
 */
typedef struct {
    const pt6891_oled_init_cmd_t* init_cmds;    // Poniter to init commands array. Set to NULL to use default commands.
    size_t num_init_cmds;                       // Number of commands in above array
} pt6891_oled_vendor_init_t;

/**
 * @brief Create OLED panel for PT6891
 * 
 * @param panel_io_handle OLED panel IO Handle
 * @param panel_dev_config General panel configuration
 * @param panel_handle Returned panel handle
 * @return esp_err_t 
 */
esp_err_t esp_lcd_new_panel_pt6891(const esp_lcd_panel_io_handle_t panel_io_handle, const esp_lcd_panel_dev_config_t* panel_dev_config, esp_lcd_panel_handle_t* panel_handle);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DRIVER_PT6891_H__
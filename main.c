#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// --- Hardware Pins & Settings ---
#define BUZZER_PIN 15
#define JOY_X_PIN 26
#define JOY_Y_PIN 27
#define JOY_BTN_PIN 22
#define JOY_X_ADC 0
#define JOY_Y_ADC 1

// --- LCD Definitions ---
#define I2C_PORT i2c0
#define I2C_ADDR 0x27
#define LCD_CLEARDISPLAY 0x01
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

#define LCD_DISPLAYON 0x04
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_5x8DOTS 0x00

#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE_BIT 0x04
#define LCD_CHR 1
#define LCD_CMD 0

SemaphoreHandle_t xMutex;
int shared_counter = 0;

// --- LCD Helper Functions ---
void i2c_write_byte(uint8_t val) {
    i2c_write_blocking(I2C_PORT, I2C_ADDR, &val, 1, false);
}

void lcd_toggle_enable(uint8_t val) {
    sleep_us(600);
    i2c_write_byte(val | LCD_ENABLE_BIT);
    sleep_us(600);
    i2c_write_byte(val & ~LCD_ENABLE_BIT);
    sleep_us(600);
}

void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;

    i2c_write_byte(high);
    lcd_toggle_enable(high);

    i2c_write_byte(low);
    lcd_toggle_enable(low);
}

void lcd_clear() {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_CMD);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void lcd_move_to(int col, int row) {
    int row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    lcd_send_byte(LCD_SETDDRAMADDR | (col + row_offsets[row]), LCD_CMD);
}

void lcd_putchar(char val) {
    lcd_send_byte(val, LCD_CHR);
}

void lcd_custom_char(uint8_t location, uint8_t charmap[]) {
    location &= 0x7;

    lcd_send_byte(
        LCD_SETCGRAMADDR | (location << 3),
        LCD_CMD
    );

    for (int i = 0; i < 8; i++) {
        lcd_send_byte(charmap[i], LCD_CHR);
    }
}

void lcd_init() {
    vTaskDelay(pdMS_TO_TICKS(50));

    lcd_send_byte(0x03, LCD_CMD);
    lcd_send_byte(0x03, LCD_CMD);
    lcd_send_byte(0x03, LCD_CMD);
    lcd_send_byte(0x02, LCD_CMD);

    lcd_send_byte(
        LCD_FUNCTIONSET |
        LCD_2LINE |
        LCD_5x8DOTS |
        LCD_4BITMODE,
        LCD_CMD
    );

    lcd_send_byte(
        LCD_DISPLAYCONTROL |
        LCD_DISPLAYON,
        LCD_CMD
    );

    lcd_clear();

    lcd_send_byte(
        LCD_ENTRYMODESET | 0x02,
        LCD_CMD
    );

    vTaskDelay(pdMS_TO_TICKS(2));
}

void draw_face(uint8_t char_id) {
    lcd_clear();

    lcd_move_to(5, 1);
    lcd_putchar(char_id);

    lcd_move_to(14, 1);
    lcd_putchar(char_id);

    lcd_move_to(8, 2);
    lcd_putchar(3);

    lcd_move_to(9, 2);
    lcd_putchar(4);

    lcd_move_to(10, 2);
    lcd_putchar(4);

    lcd_move_to(11, 2);
    lcd_putchar(4);

    lcd_move_to(12, 2);
    lcd_putchar(5);
}

// ============================================================
// Interaction Task
// Priority = 2
//
// Sequence:
// 1. Play initial buzzer
// 2. Listen for joystick/button input for 2.5 seconds
// 3. Report result through Serial Monitor
// 4. If successful, play higher-pitched buzzer
// 5. Wait before repeating
// ============================================================

void vInteractionTask(void *pvParameters) {

    adc_init();

    adc_gpio_init(JOY_X_PIN);
    adc_gpio_init(JOY_Y_PIN);

    gpio_init(JOY_BTN_PIN);
    gpio_set_dir(JOY_BTN_PIN, GPIO_IN);
    gpio_pull_up(JOY_BTN_PIN);

    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    printf("\n========================================\n");
    printf("Interaction Task Started\n");
    printf("Task Priority: 2\n");
    printf("========================================\n");

    for (;;) {

        // ----------------------------------------------------
        // STEP 1: BUZZER SOUND FIRST
        // ----------------------------------------------------

        printf("\n[INTERACTION] Starting interaction...\n");
        printf("[BUZZER] Playing initial buzzer sound...\n");

        for (int i = 0; i < 150; i++) {
            gpio_put(BUZZER_PIN, 1);
            sleep_us(1500);

            gpio_put(BUZZER_PIN, 0);
            sleep_us(1500);
        }

        printf("[BUZZER] Initial buzzer finished.\n");


        // ----------------------------------------------------
        // STEP 2: LISTEN FOR USER INPUT
        // ----------------------------------------------------

        printf("[INPUT] Listening for joystick/button input...\n");
        printf("[INPUT] Time window: 2500 ms\n");

        TickType_t start_time = xTaskGetTickCount();

        bool input_detected = false;

        while (
            (xTaskGetTickCount() - start_time)
            < pdMS_TO_TICKS(2500)
        ) {

            // Read joystick X
            adc_select_input(JOY_X_ADC);
            uint16_t x_val = adc_read();

            // Read joystick Y
            adc_select_input(JOY_Y_ADC);
            uint16_t y_val = adc_read();

            // Read joystick button
            bool btn_pressed = !gpio_get(JOY_BTN_PIN);

            // Check for user input
            if (
                btn_pressed ||
                x_val < 1000 ||
                x_val > 3000 ||
                y_val < 1000 ||
                y_val > 3000
            ) {

                input_detected = true;

                printf(
                    "[INPUT] Joystick input DETECTED!\n"
                );

                printf(
                    "[INPUT] X = %u, Y = %u, Button = %s\n",
                    x_val,
                    y_val,
                    btn_pressed ? "PRESSED" : "NOT PRESSED"
                );

                break;
            }

            // Allow other FreeRTOS tasks to run
            vTaskDelay(pdMS_TO_TICKS(50));
        }


        // ----------------------------------------------------
        // STEP 3: CHECK WHETHER TIME WINDOW EXPIRED
        // ----------------------------------------------------

        if (input_detected) {

            printf(
                "[INPUT] SUCCESS: User input was processed "
                "within the 2.5 second time window.\n"
            );

            // ------------------------------------------------
            // STEP 4: SUCCESS BUZZER
            // ------------------------------------------------

            printf(
                "[BUZZER] Playing success buzzer!\n"
            );

            for (int i = 0; i < 300; i++) {

                gpio_put(BUZZER_PIN, 1);
                sleep_us(500);

                gpio_put(BUZZER_PIN, 0);
                sleep_us(500);
            }

            printf(
                "[BUZZER] Success buzzer finished.\n"
            );

        } else {

            printf(
                "[INPUT] TIME WINDOW FINISHED.\n"
            );

            printf(
                "[INPUT] No joystick input was detected "
                "within 2500 ms.\n"
            );

            printf(
                "[BUZZER] No success buzzer played.\n"
            );
        }


        // ----------------------------------------------------
        // STEP 5: WAIT BEFORE NEXT INTERACTION
        // ----------------------------------------------------

        printf(
            "[INTERACTION] Waiting 4 seconds before "
            "next interaction...\n"
        );

        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}


// ============================================================
// LCD Blink Task
// Priority = 1
// ============================================================

void vBlinkTask(void *pvParameters) {

    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);

    gpio_pull_up(4);
    gpio_pull_up(5);

    lcd_init();

    uint8_t eye_open[] = {
        0x0E, 0x1F, 0x1F, 0x1F,
        0x1F, 0x1F, 0x1F, 0x0E
    };

    uint8_t eye_mid[] = {
        0x00, 0x0E, 0x1F, 0x1F,
        0x1F, 0x1F, 0x0E, 0x00
    };

    uint8_t eye_closed[] = {
        0x00, 0x00, 0x00, 0x1F,
        0x1F, 0x00, 0x00, 0x00
    };

    uint8_t smile_left[] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x04, 0x03
    };

    uint8_t smile_center[] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x1F
    };

    uint8_t smile_right[] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x02, 0x04, 0x18
    };

    lcd_custom_char(0, eye_open);
    lcd_custom_char(1, eye_mid);
    lcd_custom_char(2, eye_closed);

    lcd_custom_char(3, smile_left);
    lcd_custom_char(4, smile_center);
    lcd_custom_char(5, smile_right);

    printf("[LCD] Blink Task Started. Priority = 1\n");

    for (;;) {

        draw_face(0);
        vTaskDelay(pdMS_TO_TICKS(2500));

        draw_face(1);
        vTaskDelay(pdMS_TO_TICKS(50));

        draw_face(2);
        vTaskDelay(pdMS_TO_TICKS(150));

        draw_face(1);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


// ============================================================
// MAIN
// ============================================================

int main() {

    stdio_init_all();

    printf("\n========================================\n");
    printf("System Starting...\n");
    printf("========================================\n");

    xMutex = xSemaphoreCreateMutex();

    if (xMutex != NULL) {

        // Interaction Task:
        // Priority 2 = HIGHER
        xTaskCreate(
            vInteractionTask,
            "Interaction Task",
            256,
            NULL,
            2,
            NULL
        );

        // LCD Task:
        // Priority 1 = LOWER
        xTaskCreate(
            vBlinkTask,
            "Blink Task",
            256,
            NULL,
            1,
            NULL
        );

        printf("[SCHEDULER] Interaction Task Priority = 2\n");
        printf("[SCHEDULER] Blink Task Priority = 1\n");
        printf("[SCHEDULER] Starting FreeRTOS scheduler...\n");

        vTaskStartScheduler();
    }

    printf("[ERROR] Failed to create mutex!\n");

    while (1) {
    }
}
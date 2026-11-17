#include "lcd_i2c.h"
#include "stm32f1xx_hal.h"  // or whatever STM32 series you're using

#define LCD_ADDR (0x27 << 1) // change to 0x3F<<1 if needed
#define LCD_BACKLIGHT 0x08
#define ENABLE 0x04
#define RS 0x01

extern I2C_HandleTypeDef hi2c2;  // make sure hi2c2 is declared in main.c

// --- Internal low-level send ---
static void lcd_send_internal(char data, char flags)
{
    char high = (data & 0xF0) | flags | LCD_BACKLIGHT;
    char low  = ((data << 4) & 0xF0) | flags | LCD_BACKLIGHT;

    char data_arr[4];
    data_arr[0] = high | ENABLE;
    data_arr[1] = high;
    data_arr[2] = low | ENABLE;
    data_arr[3] = low;

    HAL_I2C_Master_Transmit(&hi2c2, LCD_ADDR, (uint8_t *)data_arr, 4, 100);
}

// --- Send command ---
void lcd_send_cmd(char cmd)
{
    lcd_send_internal(cmd, 0);
}

// --- Send data (character) ---
void lcd_send_data(char data)
{
    lcd_send_internal(data, RS);
}

// --- Clear LCD ---
void lcd_clear(void)
{
    lcd_send_cmd(0x01);
    HAL_Delay(2);
}

// --- Set cursor position ---
void lcd_set_cursor(uint8_t row, uint8_t col)
{
    uint8_t addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
    lcd_send_cmd(addr);
}

// --- Send string ---
void lcd_send_string(char *str)
{
    while(*str)
        lcd_send_data(*str++);
}

// --- Robust LCD init sequence ---
void lcd_init(void)
{
    HAL_Delay(50);  // power-on wait

    lcd_send_cmd(0x30);
    HAL_Delay(10);

    lcd_send_cmd(0x30);
    HAL_Delay(5);

    lcd_send_cmd(0x30);
    HAL_Delay(5);

    lcd_send_cmd(0x20);  // set 4-bit mode
    HAL_Delay(5);

    lcd_send_cmd(0x28);  // function set: 4-bit, 2 lines, 5x8 font
    HAL_Delay(1);

    lcd_send_cmd(0x08);  // display off
    HAL_Delay(1);

    lcd_send_cmd(0x01);  // clear display
    HAL_Delay(2);

    lcd_send_cmd(0x06);  // entry mode: increment cursor
    HAL_Delay(1);

    lcd_send_cmd(0x0C);  // display on, cursor off, blink off
    HAL_Delay(1);
}
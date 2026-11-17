#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "stm32f1xx_hal.h"

extern I2C_HandleTypeDef hi2c2;

void lcd_init(void);
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
void lcd_send_string(char *str);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_clear(void);

#endif

/** @file lcd.h
 * 
 * @brief  
 *
 * @par       
 * COPYRIGHT NOTICE: (c) 2021  All rights reserved.
 */ 

#include <esp_system.h>

#ifndef LCD_H
#define LCD_H

// LCD module defines
#define LCD_LINEONE             0x00        // start of line 1
#define LCD_LINETWO             0x40        // start of line 2
#define LCD_LINETHREE           0x14        // start of line 3
#define LCD_LINEFOUR            0x54        // start of line 4

#define LCD_BACKLIGHT           0x08
#define LCD_ENABLE              0x04               
#define LCD_COMMAND             0x00
#define LCD_WRITE               0x01

#define LCD_SET_DDRAM_ADDR      0x80
#define LCD_READ_BF             0x40

// LCD instructions
#define LCD_CLEAR               0x01        // replace all characters with ASCII 'space'
#define LCD_HOME                0x02        // return cursor to first position on first line
#define LCD_ENTRY_MODE          0x06        // shift cursor from left to right on read/write
#define LCD_DISPLAY_OFF         0x08        // turn display off
#define LCD_DISPLAY_ON          0x0C        // display on, cursor off, don't blink character
#define LCD_FUNCTION_RESET      0x30        // reset the LCD
#define LCD_FUNCTION_SET_4BIT   0x38        // 4-bit data, 2-line display, 5 x 7 font
#define LCD_SET_CURSOR          0x40        // set cursor position

#define MAXIMA_LONGITUD         100
#define LCD_addr                0x27
#define SDA_pin                 21
#define SCL_pin                 22
#define LCD_cols                16
#define LCD_rows                2
#define TAMU                    6


static esp_err_t I2C_init(void);
static void LCD_writeNibble(uint8_t nibble, uint8_t mode);
static void LCD_writeByte(uint8_t data, uint8_t mode);
static void LCD_pulseEnable(uint8_t nibble);
void LCD_init();
void LCD_setCursor(uint8_t col, uint8_t row);
void LCD_writeChar(char c);
void LCD_writeStr(char* str);
void LCD_home(void);
void LCD_clearScreen(void);
void LCD_Plot(char arr[], int col, int row);



#endif /* LCD_H */

/*** end of file ***/

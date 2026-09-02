#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>

// OLED status display — SSD1306 128x64, I2C, SDA=GPIO41, SCL=GPIO42.
// Chi hien thi trang thai xu ly lenh (READY/RECEIVED/EXECUTING/DONE/ERROR),
// KHONG phai giao dien dieu khien. Chi dung ASCII (khong font tieng Viet).
//
// Neu OLED khong tim thay (0x3C/0x3D deu fail): oled_init() tra false,
// firmware van chay UART binh thuong, moi ham oled_* tro thanh no-op.

bool oled_init();
void oled_show_ready();
void oled_show_received(int cmd);
void oled_show_executing(int cmd);
void oled_show_done(int cmd);
void oled_show_error(int cmd);

// Goi lien tuc trong loop(): tu dong chuyen RECEIVED -> EXECUTING -> DONE
// bang millis() (non-blocking, khong lam anh huong UART).
void oled_update();

#endif

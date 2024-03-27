#include "settings.h"


settings_t g_current_profile;


void settings_init() {
  // заполняем значениями по-умолчанию
  g_current_profile.ferrite_angle = 0;
  g_current_profile.gen_freq = 268435456u; // 140625 * 268435456 / (2^32) = 8789.0625 Гц
  g_current_profile.level_comp = 100;
  g_current_profile.level_sound = 500;
  g_current_profile.level_tx = 900;
  g_current_profile.mask_width = 16;
  g_current_profile.phase_comp_start = 0;
  // тактирование I2C1 включено в main.c
  // настройка I2C1 (remap включен в main.c)
  
}

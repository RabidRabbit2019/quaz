#include "settings.h"
#include "stm32f103x6.h"


settings_t g_current_profile;


static uint32_t crc32( settings_t * a_settings ) {
  CRC->CR = CRC_CR_RESET;
  a_settings->crc32 = 0;
  uint32_t * v_from = (uint32_t *)a_settings;
  for ( int i = 1; i < sizeof(settings_t) / sizeof(uint32_t); ++i ) {
    CRC->DR = *v_from++;
  }
  return CRC->DR;
}


void settings_init() {
  // заполняем значениями по-умолчанию
  g_current_profile.ferrite_angle = 0;
  g_current_profile.gen_freq = 268435456u; // 140625 * 268435456 / (2^32) = 8789.0625 Гц
  g_current_profile.level_comp = 100;
  g_current_profile.level_sound = 500;
  g_current_profile.level_tx = 600;
  g_current_profile.mask_width = 16;
  g_current_profile.phase_comp_start = 0;
  // читаем CRC32
}

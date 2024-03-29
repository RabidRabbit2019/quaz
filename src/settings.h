#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <stdint.h>

typedef struct {
  // уровень выходного сигнала TX [0..2048]
  uint32_t level_tx;
  // уровень выходного сигнала компенсации разбаланса [0..2048]
  uint32_t level_comp;
  // уровень громкости звука [0...2048]
  uint32_t level_sound;
  // начальная фаза генератора компенсации разбаланса
  uint32_t phase_comp_start;
  // частота (сдвиг фазы генератора) TX и сигнала компенсации
  uint32_t gen_freq;
  // угол "феррита", число с фиксированной точкой 16.16, может быть отрицательным
  int ferrite_angle;
  // ширина маски в градусах, маска считается от угла "феррита"
  uint32_t mask_width;
  // зарезервировано
  uint32_t reserved[8];
  // "хэш" для проверки правильности считывания из FLASH
  uint32_t crc32;
} settings_t;


//
void settings_init();
// получить адрес структуры с текущими настройками
settings_t * settings_get_current_profile();


#endif // __SETTINGS_H__

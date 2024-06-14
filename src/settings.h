#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <stdint.h>
#include <stdbool.h>

#define SETTINGS_VERSION    2
#define PROFILES_COUNT      4

typedef struct {
  uint32_t version;
  //
  struct {
    uint32_t id: 8;
    uint32_t serial_num: 24;
  } profile_id;
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
  // угол "вектора грунта", число с фиксированной точкой 16.16, может быть отрицательным
  int ground_angle;
  // ширина маски в градусах, маска считается от угла "феррита"
  uint32_t mask_width;
  // "маска" по уровню, "длине вектора", всё, что короче, не озвучивается
  uint32_t barrier_level;
  // коэффициент для преобразования измерений АЦП в значение напряжения в вольтах
  uint32_t voltmeter;
  // коэффициент для преобразования измерений АЦП в значение тока в миллиамперах
  uint32_t ampermeter;
  // зарезервировано
  uint32_t reserved[2];
  // "хэш" для проверки правильности считывания из FLASH
  uint32_t crc32;
} settings_t;


// сколько записей settings_t помещается на одной транице флэша (1КиБ)
#define SETTINGS_RECORDS (1024/sizeof(settings_t))
// сколько 32-битных слов в одной записи
#define WORDS_IN_RECORD (sizeof(settings_t)/sizeof(uint32_t))
//
#define WORDS_IN_PAGE (1024/sizeof(uint32_t))

//
void settings_init();
// получить адрес структуры с текущими настройками
settings_t * settings_get_current_profile();
// загружает в массив a_dst размером PROFILES_COUNT адреса сохранённых во flash профилей
// если какого-то профиля нет - в массиве будет NULL
// возвращает количество актуальных указателей в a_dst
int load_profiles_pointers( settings_t ** a_dst );
// на входе адрес профиля из флэша, установить его текущим
bool load_profile( settings_t * a_src );
// сохранить текущий профиль с указанным идентификатором
void store_profile( int a_as_id );


#endif // __SETTINGS_H__

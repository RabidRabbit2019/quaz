#include "settings.h"
#include "stm32f103x6.h"

#include <stdbool.h>


extern volatile uint32_t g_milliseconds;


// профили настроек
static settings_t g_profiles[PROFILES_COUNT];
// номер текущего профиля
static int g_current_profile = 0;


//
settings_t * settings_get_current_profile() {
  return &g_profiles[g_current_profile];
}


static uint32_t crc32( settings_t * a_settings ) {
  CRC->CR = CRC_CR_RESET;
  a_settings->crc32 = 0;
  uint32_t * v_from = (uint32_t *)a_settings;
  for ( int i = 1; i < (int)(sizeof(settings_t) / sizeof(uint32_t)); ++i ) {
    CRC->DR = *v_from++;
  }
  return CRC->DR;
}


static uint32_t settings_page_addr() {
  return FLASH_BASE + (((uint32_t)(*((uint16_t *)FLASHSIZE_BASE) - 1)) * 1024u);
}


static bool flash_page_erased() {
  uint32_t * v_ptr = (uint32_t *)settings_page_addr(); 
  for ( int i = 0; i < (int)WORDS_IN_PAGE; ++i ) {
    if ( 0xFFFFFFFFu != *v_ptr++ ) {
      return false;
    }
  }
  return true;
}


static bool record_erased( settings_t * a_record ) {
  uint32_t * v_ptr = (uint32_t *)a_record;
  for ( int i = 0; i < (int)WORDS_IN_RECORD; ++i ) {
    if ( 0xFFFFFFFFu != *v_ptr++ ) {
      return false;
    }
  }
  return true;
}


static bool erase_flash_page() {
  if ( 0 != (FLASH->CR & FLASH_CR_LOCK) ) {
    // флэш-контроллер заблокирован, пробуем разблокировать
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;
    if ( 0 != (FLASH->CR & FLASH_CR_LOCK) ) {
      // всё ещё заблокирован - вот хрень
      return false;
    }
  }
  // стираем страницу
  // сначала ждём освобожения флэш-контроллера
  uint32_t v_from = g_milliseconds;
  while ( 0 != (FLASH->SR & FLASH_SR_BSY) ) {
    if ( ((uint32_t)(g_milliseconds - v_from)) > 500 ) {
      return false;
    }
  }
  // выбираем стирание страницы
  FLASH->CR = (FLASH->CR & ~(FLASH_CR_PG | FLASH_CR_MER | FLASH_CR_OPTPG | FLASH_CR_OPTER))
              | FLASH_CR_PER
              ;
  // адрес страницы
  FLASH->AR = settings_page_addr();
  // пуск!
  FLASH->CR |= FLASH_CR_STRT;
  // ждём завершения стирания
  v_from = g_milliseconds;
  while ( 0 != (FLASH->SR & FLASH_SR_BSY) ) {
    if ( ((uint32_t)(g_milliseconds - v_from)) > 500 ) {
      return false;
    }
  }
  //
  return flash_page_erased();
}


static bool zap_profiles_page() {
  // сначала проверим, может быть страница уже стёрта
  if ( !flash_page_erased() ) {
    return erase_flash_page();
  } else {
    return true;
  }
}



static void write_flash( uint16_t * a_dst, uint16_t * a_src, uint32_t a_count ) {
  if ( 0 != (FLASH->CR & FLASH_CR_LOCK) ) {
    // флэш-контроллер заблокирован, пробуем разблокировать
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;
    if ( 0 != (FLASH->CR & FLASH_CR_LOCK) ) {
      // всё ещё заблокирован - вот хрень
      return;
    }
  }
  // пишем по 16 битов
  // сначала ждём освобожения флэш-контроллера
  uint32_t v_from = g_milliseconds;
  while ( 0 != (FLASH->SR & FLASH_SR_BSY) ) {
    if ( ((uint32_t)(g_milliseconds - v_from)) > 50 ) {
      return;
    }
  }
  while ( 0 != a_count ) {
    // выбираем запись
    FLASH->CR = (FLASH->CR & ~(FLASH_CR_PER | FLASH_CR_MER | FLASH_CR_OPTPG | FLASH_CR_OPTER))
                | FLASH_CR_PG
                ;
    // пишем
    *a_dst++ = *a_src++;
    // ждём завершения записи
    v_from = g_milliseconds;
    while ( 0 != (FLASH->SR & FLASH_SR_BSY) ) {
      if ( ((uint32_t)(g_milliseconds - v_from)) > 10 ) {
        return;
      }
    }
    --a_count;
  }
}


static void write_profile( settings_t * a_profile ) {
  // сначала ищем незанятое место на странице
  settings_t * v_ptr = (settings_t *)settings_page_addr();
  for ( int i = 0; i < (int)SETTINGS_RECORDS; ++i ) {
    if ( record_erased( v_ptr ) ) {
      // запись стёрта, пишем сюда
      a_profile->profile_id.serial_num = g_profiles[g_current_profile].profile_id.serial_num + 1;
      a_profile->crc32 = crc32( a_profile );
      write_flash( (uint16_t *)v_ptr, (uint16_t *)a_profile, sizeof(settings_t)/sizeof(uint16_t) );
      return;
    }
    ++v_ptr;
  }
  // все места заняты, стираем страницу и записываем все профили
  if ( zap_profiles_page() ) {
    write_flash( (uint16_t *)settings_page_addr(), (uint16_t *)g_profiles, PROFILES_COUNT*sizeof(g_profiles[0])/sizeof(uint16_t) );
  }
}


void settings_init() {
  for ( int i = 0; i < PROFILES_COUNT; ++i ) {
    // заполняем значениями по-умолчанию
    g_profiles[i].profile_id.id = i;
    g_profiles[i].profile_id.serial_num = (PROFILES_COUNT - 1) - i;
    g_profiles[i].ferrite_angle_fft = 0;
    g_profiles[i].ferrite_angle_sd = 0;
    g_profiles[i].gen_freq = 268435456u; // 140625 * 268435456 / (2^32) = 8789.0625 Гц
    g_profiles[i].level_comp = 100;
    g_profiles[i].level_sound = 500;
    g_profiles[i].level_tx = 600;
    g_profiles[i].mask_width = 16;
    g_profiles[i].phase_comp_start = 0;
    for ( int r = 0; r < (int)(sizeof(g_profiles[0].reserved)/sizeof(g_profiles[0].reserved[0])); ++r ) {
      g_profiles[i].reserved[r] = 0;
    }
    // считаем CRC32
    g_profiles[i].crc32 = crc32( &g_profiles[i] );
  }
  // последний килобайт флэша отдаём под хранение профилей.
  // пытаемся прочесть профили, активным будет профиль с максимальным serial_num
  bool v_profile_loaded = false;
  // адрес хранения профилей в последнем килобайте флэша
  settings_t * v_profiles = (settings_t *)settings_page_addr();
  for ( int i = 0; i < (int)SETTINGS_RECORDS; ++i ) {
    if ( v_profiles->profile_id.id <= PROFILES_COUNT ) {
      // проверим серийный номер
      if ( v_profiles->profile_id.serial_num >= g_profiles[v_profiles->profile_id.id].profile_id.serial_num ) {
        // проверяем crc32
        if ( crc32( v_profiles ) == v_profiles->crc32 ) {
          // копируем
          g_profiles[v_profiles->profile_id.id] = *v_profiles;
          //
          v_profile_loaded = true;
          // этот профиль возможно надо сделать текущим
          if ( v_profiles->profile_id.serial_num > g_profiles[g_current_profile].profile_id.serial_num ) {
            g_current_profile = v_profiles->profile_id.id;
          }
        }
      }
    }
    //
    ++v_profiles;
  }
  // если ничего не прочитали, значит там либо старьё, либо ничего нет
  if ( !v_profile_loaded ) {
    // если надо, почистим страницу флэша
    if ( zap_profiles_page() ) {
      // запишем первый профиль по-умолчанию
      write_profile( &g_profiles[0] );
      g_current_profile = 0;
    }
  }
}

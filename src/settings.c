#include "settings.h"
#include "adc.h"
#include "gen_dds.h"
#include "stm32f103x6.h"

#include <stdlib.h>


extern volatile uint32_t g_milliseconds;


// профили настроек
static settings_t g_profiles[PROFILES_COUNT];
// номер текущего профиля
static int g_current_profile = 0;


//
settings_t * settings_get_current_profile() {
  return &g_profiles[g_current_profile];
}


static uint32_t crc32( const settings_t * a_settings ) {
  settings_t v_settings = *a_settings;
  v_settings.crc32 = 0;
  CRC->CR = CRC_CR_RESET;
  uint32_t * v_from = (uint32_t *)&v_settings;
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


static bool is_valid_profile( const settings_t * a_profile ) {
  return a_profile->crc32 == crc32( a_profile )
      && a_profile->version == SETTINGS_VERSION
      && a_profile->profile_id.id < PROFILES_COUNT;
}


int load_profiles_pointers( settings_t ** a_dst ) {
  int v_result = 0;
  // сбрасываем все указатели
  for ( int i = 0; i < PROFILES_COUNT; ++i ) {
    a_dst[i] = NULL;
  }
  // читаем профили из flash
  settings_t * v_profiles = (settings_t *)settings_page_addr();
  for ( int i = 0; i < (int)SETTINGS_RECORDS; ++i ) {
    if ( is_valid_profile( &v_profiles[i] ) ) {
      // здесь запись "правильная"
      if ( a_dst[v_profiles[i].profile_id.id] ) {
        // здесь такой профиль уже был, так что проверяем серийный номер
        if ( a_dst[v_profiles[i].profile_id.id]->profile_id.serial_num <= v_profiles[i].profile_id.serial_num ) {
          // очередная запись новее, используем её
          a_dst[v_profiles[i].profile_id.id] = &v_profiles[i];
        }
      } else {
        // записи ещё не было, запомним адрес новой записи
        a_dst[v_profiles[i].profile_id.id] = &v_profiles[i];
        // увеличим счётчик присутствующих профилей
        ++v_result;
      }
    }
  }
  return v_result;
}


void settings_init() {
  for ( int i = 0; i < PROFILES_COUNT; ++i ) {
    // заполняем значениями по-умолчанию
    g_profiles[i].version = SETTINGS_VERSION;
    g_profiles[i].profile_id.id = i;
    g_profiles[i].profile_id.serial_num = (PROFILES_COUNT - 1) - i;
    g_profiles[i].ferrite_angle = 0;
    g_profiles[i].ground_angle = 0;
    // сдвиг фазы для задания частоты генератора TX
    // для получения целого количества периодов в выборке нужно, чтобы
    // 256 * gen_freq / 2^32 было целым числом (256 - количество выборок в буфере)
    // и при этом (2^32) / gen_freq тоже должно быть целым числом
    // частота генератора 140625 * 268435456 / (2^32) = 8789.0625 Гц, 140625 Гц - опорная частота DDS генератора
    // 2^32 - период в 360 градусов
    g_profiles[i].gen_freq = 268435456u;
    g_profiles[i].level_comp = 1;
    g_profiles[i].level_sound = 500;
    g_profiles[i].level_tx = 300;
    g_profiles[i].mask_width = 16;
    g_profiles[i].barrier_level = 99;
    g_profiles[i].phase_comp_start = 0;
    g_profiles[i].voltmeter = 16104u;
    g_profiles[i].ampermeter = 115852u;
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
    if ( !is_valid_profile( &v_profiles[i] ) ) {
      continue;
    }
    // проверим серийный номер
    if ( v_profiles[i].profile_id.serial_num >= g_profiles[v_profiles[i].profile_id.id].profile_id.serial_num ) {
      // копируем
      g_profiles[v_profiles[i].profile_id.id] = *v_profiles;
      //
      v_profile_loaded = true;
      // этот профиль возможно надо сделать текущим
      if ( v_profiles[i].profile_id.serial_num > g_profiles[g_current_profile].profile_id.serial_num ) {
        g_current_profile = v_profiles[i].profile_id.id;
      }
    }
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


bool load_profile( settings_t * a_src ) {
  if ( is_valid_profile( a_src ) ) {
    // профиль норм, запомним серийный номер текущего профиля (он максимальный)
    uint32_t v_serial_num = g_profiles[g_current_profile].profile_id.serial_num;
    // коэффициенты вольтметра и амперметра не загружаем при обычной загрузке профиля
    uint32_t v_voltmeter = g_profiles[g_current_profile].voltmeter;
    uint32_t v_ampermeter = g_profiles[g_current_profile].ampermeter;
    // переключим номер активного профиля
    g_current_profile = a_src->profile_id.id;
    // скопируем профиль из флэша в активный профиль
    g_profiles[g_current_profile] = *a_src;
    // проставим активному профилю максимальный серийный номер
    g_profiles[g_current_profile].profile_id.serial_num = v_serial_num;
    // актуальные значения для вольтметра и амперметра
    g_profiles[g_current_profile].voltmeter = v_voltmeter;
    g_profiles[g_current_profile].ampermeter = v_ampermeter;
    // сохраним активный профиль во флэше, при этом у него увеличится на 1 серийный номер
    write_profile( &g_profiles[g_current_profile] );
    // отключаем ADC и DDS
    gen_dds_shutdown();
    adc_shutdown();
    // заново запускаем
    adc_init();
    gen_dds_init();
    return true;
  } else {
    return false;
  }
}


// сохранить текущий профиль с указанным идентификатором
void store_profile( int a_as_id ) {
  // просто проверка
  if ( a_as_id < 0 || a_as_is >= PROFILES_COUNT ) {
    return;
  }
  // если требуется записать текущий профиль с другим идентификатором
  if ( g_current_profile != a_as_id ) {
    // сначала копируем текущий профиль в профиль с указанным идентификатором
    g_profiles[a_as_id] = g_profiles[g_current_profile];
    // обновляем идентификатор текущего профиля
    g_current_profile = a_as_id;
    // в текущем профиле устанавливаем "правильный" идентификатор
    g_profiles[g_current_profile].profile_id.id = g_current_profile;
  }
  // сохраняем текущий профиль во флэше.
  write_profile( &g_profiles[g_current_profile] );
}

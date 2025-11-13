#ifndef _PERIPHERAL_SETTINGS_KEEPER_H_
#define _PERIPHERAL_SETTINGS_KEEPER_H_

#include <cstdint>

#define DEFAULT_LRF_OFF_TIME_SEC 20					// Время выключения дальномера
#define DEFAULT_LRF_MEASURE_TIMEOUT_SEC 2			// Время накачки дальномера (между замерами)
#define DEFAULT_LRF_BUFFER_SHOW true				// Расчет 3х дистанций и предикция координат

#define DEFAULT_COMPASS_OFFSET			0.0f		// Дефолтный оффсет для калибровки компаса
#define DEFAULT_COMPASS_SCALE			0.0f		// Дефолтный "масштаб" для калибровки компаса
#define DEFAULT_COMPASS_STATE			true		// Показывать компас или нет
#define DEFAULT_COMPASS_MAG_DECL 		10.6f		// Магнитное склонение в центре Москвы

#define DEFAULT_GNSS_STATE				true		// Включить ГПС/ГНСС или нет
#define DEFAULT_GNSS_TIMEZONE			3			// Дефолтный часовой пояс (+3 UTC - Москва)
#define DEFAULT_GNSS_IS_DIST			false		// Флаг перехода на летнее время (нет, работаем по российским правилам)

#define DEFAULT_SHIFT_KEY_STATE			false		// Флаг кнопки SHIFT
#define DEFAULT_FN_KEY_STATE			false		// Флаг кнопки FN
#define DEFAULT_ENGEENIRING_STATE		false		// Флаг кнопки инженерного меню

#define DEFAULT_MENU_OFF_TIME_SEC		20			// Время отключения меню
#define DEFAULT_PERIF_TIPS_SHOW			true		// Флаг показа подсказок
#define DEFAULT_PERIF_ANGLES_SHOW		true		// Флаг показа измерительных линеек углов от MEMS
#define DEFAULT_PERIF_METEO_SHOW		true		// Флаг показа метеоусловий (Т, Р, Н)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*********** Структура для хранения настроек относящихся к функционалу дальномера ***********/

typedef struct {
	uint16_t OFFtime;			//Время отключения дальномера (20, 60, 300 сек)
	uint8_t	measureTime;		//Временной gap между замерами для накачки заряда
	bool bufferShow;			//Флаг необходимости отображения буфера на три замеренных дистанции

} LRF_setting_t;

/*********** Структуры для хранения настроек относящиеся к функционалу компаса ***********/

namespace mag {
	
	using calibration_unit_t = float;
}

typedef struct {
	mag::calibration_unit_t offset[3];
    mag::calibration_unit_t scale[3];
    mag::calibration_unit_t temp_coeff_offset[3]; 	// Температурные коэффициенты смещения
    mag::calibration_unit_t temp_coeff_scale[3];  	// Температурные коэффициенты масштаба
    mag::calibration_unit_t cal_temp;            	// Температура при калибровке

} mag_calibration_t;

typedef struct {					// Структура для хранения температурных данных при калибровке компаса
    float temperature;
    float timestamp;

} mag_temp_data_t;

typedef struct {
	bool active;					// Компас ВКЛ/ВЫКЛ
	float magDeclination;			// Магнитное склонение
	mag_calibration_t calibration;	// Калибровочные коэффициенты

} MAG_setting_t;

/*********** Структура для хранения настроек, относящихся к функционалу GNSS ***********/

typedef struct {
	bool active;				//ГНСС ВКЛ/ВЫКЛ
	int8_t timeZone;			//Часовой пояс по UTC
	bool isDst;					//Флаг летнего времени (false - перехода на летнее время нет)

} GPS_setting_t;

/*********** Структура для хранения настроек, относящихся к функционалу клавиш SHIFT, FN ***********/

typedef struct {
	bool shift;				// Используется SHIFT
	bool FN;				// Используется FN
	bool engeeniring;		// Переход в режим ("инженерное меню", без него не действует юстировка дальномера и полный сброс настроек)

} BTN_setting_t;

/*********** Структура для хранения настроек, относящихся к периферии прибора ***********/

typedef struct {
	uint16_t menuShowtime;			//Время отключения меню (20, 60, 300 сек)
	bool tipsShow;					//Флаг подсказок функций
	bool anglesShow;				//Флаг для отображения углов в интерфейсе (тангаж, крен, рысканье)
	bool meteoShow;					//Флаг для отрисовки метеоусловий

} perif_settings_t;

/*********** Общая структура для хранения настроек, относящихся к функционалу прибора ***********/

typedef struct {

	LRF_setting_t lrf;
	MAG_setting_t compass;
	GPS_setting_t gps;
	BTN_setting_t buttons;
	perif_settings_t perif;
} dev_setting_t;

bool operator!=(const dev_setting_t&, const dev_setting_t&);

#endif /* _PERIPHERAL_SETTINGS_KEEPER_H_ */
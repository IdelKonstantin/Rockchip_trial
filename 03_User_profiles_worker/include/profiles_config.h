#ifndef _USER_PROFILES_KEEPER_H_
#define _USER_PROFILES_KEEPER_H_

#include <cstdint>
#include <string>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define USER_PROFILES_NUMBER_MAX	10
#define USER_FLOAT_EPSILON			std::numeric_limits<float>::epsilon()

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum {					// Типы прицельной марки 
	
	AimType_Circle = 0,			// Круг с крыльями
	AimType_Triangle,			// Треугольник с крыльями
	AimType_TMR4,				// Старый жирный TMR
	AimType_TMR,				// Новый TMR от Думкина
	AimType_MillDot_1,			// МиллДот от Думкина
	AimType_MillDot_2,			// МиллДот от Хаски с Ёлкой
	AimType_MillDot_3,			// МиллДот от Хаски без Ёлки
	AimType_PSO_Triangles,		// ПСО с треугольниками
	AimType_PSO_Dots,			// ПСО с точками
	AimType_Movable,			// Перемещаемая ПМ. горизонталь двигается на верт поправку от пристрелки. Шаг в пикселях OLED с пересчетом в милы
	AimType_Ballistic_Meter,	// Баллистическая c вертикалью в метрах из бал-таблицы
	AimType_Ballistic_Mills,	// Баллистическая c вертикалью в милах
	AimType_LRFspotsize_Box,	// Габаритный квадрат пятна дальномера

} TAimType;

typedef enum {					// Языки локализации
	
	Lang_RUS 				= 0,
	Lang_ENG

} TLanguage;

typedef enum {							// Модель OLED экрана

	DM_SVGA_038				= 0,		//  800 х 600, размер по диагонали 0.38'
	DM_SVGA_050				= 1,		//  800 х 600, размер по диагонали 0.50'
	DM_SVGA_060				= 2,		//  800 х 600, размер по диагонали 0.60'
	DM_SXGA_060				= 3,		//  1280 х 960, размер по диагонали 0.60'
	DM_NONE					= 99		//  Разрешенгие экрана не распознано

} TDisplayModel;

typedef enum {							// Тип и разрешение OLED экрана

	DR_SVGA					= 0,		//  800 х 600
	DR_SXGA					= 1,		// 1280 х 960
	DR_NONE					= 99		//  Разрешение экрана не распознано

} TDisplayRes;

typedef enum {							// Тип тепловизионного модуля

	MT_QVGA					= 0,		//  384 x 288  - 384й модуль на матрице Ulis от Infratest
	MT_VGA					= 1,		//  640 x 480  - 640й модуль на матрице Ulis от Infratest
	MT_VGA_IR				= 2,		//  640 x 512  - 640й модуль на матрице iRay от Infratest
	MT_NONE					= 99		//  Модуль не распознан

} TModuleType;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define USER_PROFILE_DEFAULT_NAME_PREFIX		"Profile_"
#define USER_PROFILE_DEFAULT_CORRECTION_X		0
#define USER_PROFILE_DEFAULT_CORRECTION_Y		0
#define USER_PROFILE_DEFAULT_CORRECTION_H		0
#define USER_PROFILE_DEFAULT_CORRECTION_V		0
#define USER_PROFILE_DEFAULT_BRIGHNESS			120
#define USER_PROFILE_DEFAULT_CONTRAST			120
#define USER_PROFILE_DEFAULT_ZOOM				10
#define USER_PROFILE_DEFAULT_PROFILE_INDEX		0
#define USER_PROFILE_DEFAULT_CONTRAST_PREDUST	0
#define USER_PROFILE_DEFAULT_GAMMA_PREDUST		0
#define USER_PROFILE_DEFAULT_PALETTE			0
#define USER_PROFILE_DEFAULT_PALETTE_WIDTH		1
#define USER_PROFILE_DEFAULT_PALETTE_OFFSET		0
#define USER_PROFILE_DEFAULT_AIM_TYPE			AimType_Circle
#define USER_PROFILE_DEFAULT_AIM_COLOR_INDEX	0
#define USER_PROFILE_DEFAULT_AIM_BRIGHTNESS		0

#define USER_TF_PROFILE_DEFAULT_MENU_NUMBER				1
#define USER_TF_PROFILE_DEFAULT_MENU_TIMEOUT			20
#define USER_TF_PROFILE_DEFAULT_UI_LANGUAGE				Lang_RUS
#define USER_TF_PROFILE_DEFAULT_OLED_SHIFT_X			0
#define USER_TF_PROFILE_DEFAULT_OLED_SHIFT_Y			0
#define USER_TF_PROFILE_DEFAULT_OLED_SHIFT_DX			-1
#define USER_TF_PROFILE_DEFAULT_OLED_SHIFT_DY			-1
#define USER_TF_PROFILE_DEFAULT_RF_TARGET_SIZE			10
#define USER_TF_PROFILE_DEFAULT_RF_TARGET_INDEX			0
#define USER_TF_PROFILE_DEFAULT_RF_TARGET_SIZE_STEP		1	
#define USER_TF_PROFILE_DEFAULT_RF_BOX_SIZE				1
#define USER_TF_PROFILE_DEFAULT_RF_BOX_SIZE_LAST		1
#define USER_TF_PROFILE_DEFAULT_RF_CALCULATED_DISTANCE	600
#define USER_TF_PROFILE_DEFAULT_RF_ON_AIM				false
#define USER_TF_PROFILE_DEFAULT_BAD_PIXS_THRESHOLD		5
#define USER_TF_PROFILE_DEFAULT_AIM_FLASHING			false
#define USER_TF_PROFILE_DEFAULT_ZOOM_IN_NM				false
#define USER_TF_PROFILE_DEFAULT_TP_CHANGE_IN_NM			false
#define USER_TF_PROFILE_DEFAULT_LEFT_ENC_INVERSION		false
#define USER_TF_PROFILE_DEFAULT_TOP_ENC_INVERSION		false
#define USER_TF_PROFILE_DEFAULT_RIGHT_ENC_INVERSION		false
#define USER_TF_PROFILE_DEFAULT_ZOOM_STEP_x10			10

#define USER_TF_HW_PROFILE_DEFAULT_DISPLAY_MODEL		DM_SXGA_060
#define USER_TF_HW_PROFILE_DEFAULT_DISPLAY_RESOLUTION	DR_SXGA
#define USER_TF_HW_PROFILE_DEFAULT_MODULE_TYPE			MT_VGA
#define USER_TF_HW_PROFILE_DEFAULT_MODULE_VERSION		0.0
#define USER_TF_HW_PROFILE_DEFAULT_PIXEL_STEP			1.0
#define USER_TF_HW_PROFILE_DEFAULT_OLED_AUTODETECT		false
#define USER_TF_HW_PROFILE_DEFAULT_RES_AUTODETECT		false
#define USER_TF_HW_PROFILE_DEFAULT_TF_AUTODETECT		false
#define USER_TF_HW_PROFILE_DEFAULT_TF_VER_AUTODETECT	false
#define USER_TF_HW_PROFILE_DEFAULT_FOCAL_LENGHT			35
#define USER_TF_HW_PROFILE_DEFAULT_PIXEL_SIZE			12
#define USER_TF_HW_PROFILE_DEFAULT_SCALE_OLED			2.0f
#define USER_TF_HW_PROFILE_DEFAULT_ADC_BATTERY_CORR		1.0f
#define USER_TF_HW_PROFILE_DEFAULT_ADC_TEMP_CORR		1.0f
#define USER_TF_HW_PROFILE_DEFAULT_MOUNT_ANGLE			0.1f
#define USER_TF_HW_PROFILE_DEFAULT_OLED_RES_X			1280
#define USER_TF_HW_PROFILE_DEFAULT_OLED_RES_Y			960
#define USER_TF_HW_PROFILE_DEFAULT_OLED_RES_X2			(USER_TF_HW_PROFILE_DEFAULT_OLED_RES_X / 2)
#define USER_TF_HW_PROFILE_DEFAULT_OLED_RES_Y2			(USER_TF_HW_PROFILE_DEFAULT_OLED_RES_Y / 2)
#define USER_TF_HW_PROFILE_DEFAULT_TF_RESX				640
#define USER_TF_HW_PROFILE_DEFAULT_TF_RESY				480
#define USER_TF_HW_PROFILE_DEFAULT_TF_RESX2				(USER_TF_HW_PROFILE_DEFAULT_TF_RESX / 2)
#define USER_TF_HW_PROFILE_DEFAULT_TF_RESY2				(USER_TF_HW_PROFILE_DEFAULT_TF_RESY / 2)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace user {

	bool inline floatsNotEqual(float a, float b);

	typedef struct TPprofile {

		std::string name;
		int32_t correctionX;					// Текущая поправка для активного профиля по X в пикселях OLED
		int32_t	correctionY;					// Текущая поправка для активного профиля по Y в пикселях OLED
		int32_t	correctionH;					// Поправка пристрелки по горизонтали
		int32_t	correctionV;					// Поправка пристрелки по вертикали	
		uint8_t	brightnessOLED;					// Яркость OLED   	128 = без изменений, 0 = мин, 255 макс
		uint8_t	contrastOLED;					// Контраст OLED	128 = без изменений, 0 = мин, 255 макс
		uint8_t	zoomTF;							// Zoom ( 10 .. 80) в формате от х1.0 до х8.0 умноженный на 10 чтобы INTa хватило (шаг через 0.5)
		uint8_t	pictureProfileIndex;			// Индекс выбранного профиля тепловизионной картинки
		uint8_t	contrastPredust;				// Пункт меню предустановки контрастера (0.5)
		uint8_t	gammaPredust;					// Пункт меню предустановки гаммы (0..4)
		uint8_t	palette;						// Пункт меню предустановки палитры (0..4)
		uint8_t	paletteWidth;					// Ширина окна для двойной палитры (1..50)
		uint8_t	paletteOffset;					// Смещение окна для двойной палитры (0..99)
		TAimType	pricelType;					// Тип прицельной марки (Пункт меню типа прицельной марки)
		uint8_t		pricelColor;				// Пункт меню цвета прицельной марки (0..4)
		uint8_t		pricelBright;				// Пункт меню яркости прицельной марки (0..2)

		bool operator!=(const TPprofile& other) const;

	} TP_profile_t;

	typedef struct TPprofiles {

		uint8_t indexOfSelected;
		TPprofile profile[USER_PROFILES_NUMBER_MAX];

		bool operator!=(const TPprofiles& other) const;

	} TP_profile_info_t;

	typedef struct TPCommonSettings  {														// Настройки, общие для всего прицела и не зависят от профиля
		
		uint8_t		Menu_Main_Number;														// Выбранный пункт главного меню
		uint32_t	Menu_TimeOut;															// Таймер автоотключения меню (0, 20, 60, 300) секунд (для выбора в настройках)
		TLanguage	Lang;																	// Язык пользовательского интерфейса
		uint8_t 	OLED_shift_x;															// переменная для смещения по x для защиты от выгорания OLED
		uint8_t 	OLED_shift_y;															// переменная для смещения по y для защиты от выгорания OLED
		int8_t  	OLED_shift_dir_x;														// переменная для направления смещения по x для защиты от выгорания OLED  -1 или +1
		int8_t  	OLED_shift_dir_y;														// переменная для направления смещения по y для защиты от выгорания OLED  -1 или +1
		uint16_t	Dalnomer_TargetSize;													// Размер цели для анализа
		uint8_t		Dalnomer_TargetSizeIndex;												// Индекс размера цели для анализа
		uint8_t		Dalnomer_StepSize;														// Шаг изменения размера цели (1 5 10 см)
		uint8_t		Dalnomer_BoxSize;														// Размер квадрата для того чтобы вписать цель и получить расстояние
		uint8_t		Dalnomer_BoxSize_Last;													// Предыдущий размер квадрата для того чтобы вписать цель и получить расстояние (чтобы стереть при перерисовке)
		float		Dalnomer_Distance;														// Вычисленная дистанция пассивного дальномера
		bool	    Dalnomer_OnAim;														 	// Дальномер на ПМ (да/нет)
		uint8_t		BadPixThreshold;														// Порог для алгоритма поиска быитых пикселей (5..30) (см документашию на модуль Thermoframe)
		bool		AimFlashCenter;														 	// мигающий центр ПМ (да/нет)
		bool		ZoomInNM;																// Зум правым энкодером в нормальном режиме (да/нет)
		bool		ChangeTPInNM;															// Смена термопрофилей левым энкодером в нормальном режиме (да/нет)
		bool		INVencL;																// Общая инверсия левого энкодера   (да/нет)
		bool		INVencT;																// Общая инверсия верхнего энкодера (да/нет)
		bool		INVencR;																// Общая инверсия правого энкодера  (да/нет)
		uint8_t		ZoomStep;																// Шаг при плавном зуме 0.1 0.2 0.5 1.0 (умноженный на 10)

		bool operator!=(const TPCommonSettings& other) const;

	} TP_common_settings_t;

	typedef struct TPCommonSettingsHW {														// Настройки, общие для всего прицела и не зависят от профиля
	    
		TDisplayModel  	DisplayModel;														// Модель OLED экрана [0:9]
		TDisplayRes   	DisplayRes;															// Разрешения OLED экрана (0 = SVGA = 800 х 600 | 1 = SXGA = 1280 х 960 )
		TModuleType		ModuleType;															// Тип тепловизионного модуля (0 = QVGA = 384 x 288 |1 = VGA 640 x 480 | 2 = VGA_IR 640 x 512 iRay | 3 = SXGA 1280 x 960)
		float		Module_ver;																// Версия ПО тепловизионного модуля (до 13й версии - переворот экрана и другие координаты)
		float		PixelStep;																// Размер шага поправок (цена пиксела в см на 100м)
		uint8_t		fAutoDetectedOLEDModel;													// Флаг того что смогли автоопределить модель OLED
		uint8_t		fAutoDetectedOLEDRes;													// Флаг того что смогли автоопределить тип/разрешение OLED
		uint8_t		fAutoDetectedModule;													// Флаг того что смогли автоопределить тип/разрешение тепловизионного модуля
		uint8_t		fAutoDetectedModuleVer;													// Флаг того что смогли автоопределить тип/разрешение версии ПО ПЛИС тепловизионного модуля
		uint16_t 	FocalLenght;															// Фокусное расстояние объектива (35/50/60/100мм)
		uint8_t		PixelSize;																// Размер пикселя матрицы (12/17 мкм)
		float		ScaleSensorOLED;														// Коэффициент пересчеты патрицы в OLED (2.00 1.2 1.00)
		float		ADC_BatteryCorr;														// Коэффициент коррекции анализа батареи питания в вольтах (простой сдвиг)
		float		ADC_TemperatureCorr;													// Коэффициент коррекции температуры в градусах (простой сдвиг)
		int16_t		MountAngle;																// Наклон планки крепления в mil, точность 0.1mil, Умноженный на 10 чтобы не использовать float, для смещения ПМ на дальние дистанции и большие объективы (100 и более)
		int16_t		DisplayResX;															// Разрешение OLED экрана по горизонтали в пикселях OLED
		int16_t		DisplayResY;															// Разрешение OLED экрана по вертикали в пикселях OLED
		int16_t		DisplayResX2;															// центр OLED экрана по горизонтали в пикселях OLED
		int16_t		DisplayResY2;															// центр OLED экрана по вертикали в пикселях OLED
		int16_t		ModuleResX;																// Разрешение матрицы болометра по горизонтали в пикселях болометра
		int16_t		ModuleResY;																// Разрешение матрицы болометра по вертикали в пикселях болометра
		int16_t		ModuleResX2;															// центр матрицы болометра по горизонтали в пикселях болометра
		int16_t		ModuleResY2;															// центр матрицы болометра по вертикали в пикселях болометра

		bool operator!=(const TPCommonSettingsHW& other) const;

	} T_common_settings_HW_t;
};

#endif /* _USER_PROFILES_KEEPER_H_ */
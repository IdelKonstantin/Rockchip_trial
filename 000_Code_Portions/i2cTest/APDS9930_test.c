#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <math.h>

// Адрес APDS-9930
#define APDS9930_I2C_ADDR    0x39

// Регистры APDS-9930
#define APDS9930_ENABLE      0x80
#define APDS9930_ATIME       0x81
#define APDS9930_WTIME       0x83
#define APDS9930_AILTL       0x84
#define APDS9930_AILTH       0x85
#define APDS9930_AIHTL       0x86
#define APDS9930_AIHTH       0x87
#define APDS9930_PILTL       0x88
#define APDS9930_PILTH       0x89
#define APDS9930_PIHTL       0x8A
#define APDS9930_PIHTH       0x8B
#define APDS9930_PERS        0x8C
#define APDS9930_CONFIG      0x8D
#define APDS9930_PPULSE      0x8E
#define APDS9930_CONTROL     0x8F
#define APDS9930_ID          0x92
#define APDS9930_STATUS      0x93
#define APDS9930_CDATAL      0x94
#define APDS9930_CDATAH      0x95
#define APDS9930_IRDATAL     0x96
#define APDS9930_IRDATAH     0x97
#define APDS9930_PDATAL      0x98
#define APDS9930_PDATAH      0x99
#define APDS9930_POFFSET     0x9D

// Битовая маска для регистра ENABLE
#define APDS9930_PON         0x01  // Power ON
#define APDS9930_AEN         0x02  // ALS Enable
#define APDS9930_PEN         0x04  // Proximity Enable
#define APDS9930_WEN         0x08  // Wait Enable
#define APDS9930_AIEN        0x10  // ALS Interrupt Enable
#define APDS9930_PIEN        0x20  // Proximity Interrupt Enable

// Битовая маска для регистра STATUS
#define APDS9930_AVALID      0x01  // ALS Data Valid
#define APDS9930_PVALID      0x02  // Proximity Data Valid

// Структура для данных датчика
typedef struct {
    float ambient_light;     // Освещенность в люксах
    float ir_light;          // Инфракрасная составляющая
    uint16_t proximity;      // Данные приближения
    uint16_t clear_light;    // Свет без ИК составляющей
} APDS9930_Data;

// Структура для калибровочных коэффициентов
typedef struct {
    float ch0_coeff;         // Коэффициент для канала 0 (clear)
    float ch1_coeff;         // Коэффициент для канала 1 (IR)
    float df;                // Device factor
    float ga;                // Glass attenuation
} APDS9930_Calib;

// Основная структура датчика
typedef struct {
    int i2c_fd;
    uint8_t i2c_addr;
    APDS9930_Calib calib;
    uint8_t atime;           // ALS integration time
    uint8_t again;           // ALS gain
} APDS9930;

// Прототипы функций (объявляем ДО их использования)
uint8_t apds9930_read_byte(APDS9930* sensor, uint8_t reg);
int apds9930_write_byte(APDS9930* sensor, uint8_t reg, uint8_t value);
uint16_t apds9930_read_word(APDS9930* sensor, uint8_t reg);
int apds9930_configure(APDS9930* sensor);
int apds9930_read_light_data(APDS9930* sensor, APDS9930_Data* data);
int apds9930_read_proximity_data(APDS9930* sensor, APDS9930_Data* data);
int apds9930_read_all_data(APDS9930* sensor, APDS9930_Data* data);
void apds9930_set_calibration(APDS9930* sensor, float ch0_coeff, float ch1_coeff, float df, float ga);
float apds9930_get_integration_time(APDS9930* sensor);
int apds9930_set_gain(APDS9930* sensor, uint8_t gain);
const char* apds9930_get_gain_name(uint8_t gain);
int apds9930_power_down(APDS9930* sensor);
int apds9930_power_on(APDS9930* sensor);
void apds9930_close(APDS9930* sensor);

// Инициализация APDS-9930
APDS9930* apds9930_init(const char* i2c_device, uint8_t address) {
    APDS9930* sensor = malloc(sizeof(APDS9930));
    if (!sensor) {
        perror("Memory allocation failed");
        return NULL;
    }
    
    // Открываем I2C устройство
    sensor->i2c_fd = open(i2c_device, O_RDWR);
    if (sensor->i2c_fd < 0) {
        perror("Failed to open I2C device");
        free(sensor);
        return NULL;
    }
    
    sensor->i2c_addr = address;
    
    // Устанавливаем адрес устройства
    if (ioctl(sensor->i2c_fd, I2C_SLAVE, sensor->i2c_addr) < 0) {
        perror("Failed to set I2C address");
        close(sensor->i2c_fd);
        free(sensor);
        return NULL;
    }
/*    
    // Проверяем ID устройства
    uint8_t id = apds9930_read_byte(sensor, APDS9930_ID);
    if (id != 0x39 && id != 0x38 && id != 0x29) {
        fprintf(stderr, "Invalid APDS-9930 chip ID: 0x%02X\n", id);
        close(sensor->i2c_fd);
        free(sensor);
        return NULL;
    }
    
    printf("APDS-9930 found with ID: 0x%02X\n", id);
*/    
    // Инициализируем калибровочные коэффициенты (значения по умолчанию)
    sensor->calib.ch0_coeff = 1.0f;
    sensor->calib.ch1_coeff = 1.0f;
    sensor->calib.df = 52.0f;
    sensor->calib.ga = 1.0f;
    
    // Настройка датчика
    if (apds9930_configure(sensor) != 0) {
        fprintf(stderr, "Failed to configure APDS-9930 sensor\n");
        close(sensor->i2c_fd);
        free(sensor);
        return NULL;
    }
    
    printf("APDS-9930 initialized successfully\n");
    return sensor;
}

// Чтение байта из регистра
uint8_t apds9930_read_byte(APDS9930* sensor, uint8_t reg) {
    if (write(sensor->i2c_fd, &reg, 1) != 1) {
        perror("I2C write failed");
        return 0;
    }
    
    uint8_t value;
    if (read(sensor->i2c_fd, &value, 1) != 1) {
        perror("I2C read failed");
        return 0;
    }
    return value;
}

// Запись байта в регистр
int apds9930_write_byte(APDS9930* sensor, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    if (write(sensor->i2c_fd, data, 2) != 2) {
        perror("I2C write failed");
        return -1;
    }
    return 0;
}

// Чтение слова (2 байта) из регистра
uint16_t apds9930_read_word(APDS9930* sensor, uint8_t reg) {
    if (write(sensor->i2c_fd, &reg, 1) != 1) {
        perror("I2C write failed");
        return 0;
    }
    
    uint8_t data[2];
    if (read(sensor->i2c_fd, data, 2) != 2) {
        perror("I2C read failed");
        return 0;
    }
    return (data[1] << 8) | data[0];
}

// Конфигурация датчика
int apds9930_configure(APDS9930* sensor) {
    // Выключаем датчик для конфигурации
    if (apds9930_write_byte(sensor, APDS9930_ENABLE, 0x00) != 0) {
        return -1;
    }
    
    // Устанавливаем время интегрирования ALS (2.73ms)
    sensor->atime = 0xFF;  // 2.73ms
    if (apds9930_write_byte(sensor, APDS9930_ATIME, sensor->atime) != 0) {
        return -1;
    }
    
    // Устанавливаем время ожидания (2.73ms)
    if (apds9930_write_byte(sensor, APDS9930_WTIME, 0xFF) != 0) {
        return -1;
    }
    
    // Настройка контроля усиления
    sensor->again = 0x02;  // 16x gain
    if (apds9930_write_byte(sensor, APDS9930_CONTROL, 
                           (sensor->again & 0x03)) != 0) {
        return -1;
    }
    
    // Настройка проксимити
    if (apds9930_write_byte(sensor, APDS9930_PPULSE, 0x08) != 0) {  // 8 pulses
        return -1;
    }
    
    // Включаем датчик: Power ON, ALS, Proximity
    uint8_t enable_reg = APDS9930_PON | APDS9930_AEN | APDS9930_PEN;
    if (apds9930_write_byte(sensor, APDS9930_ENABLE, enable_reg) != 0) {
        return -1;
    }
    
    // Ждем инициализации
    usleep(10000);
    
    return 0;
}

// Чтение данных освещенности
int apds9930_read_light_data(APDS9930* sensor, APDS9930_Data* data) {
    // Проверяем готовность данных ALS
    uint8_t status = apds9930_read_byte(sensor, APDS9930_STATUS);
    if (!(status & APDS9930_AVALID)) {
        return -1;  // Данные не готовы
    }
    
    // Читаем данные каналов
    uint16_t ch0 = apds9930_read_word(sensor, APDS9930_CDATAL);  // Clear channel
    uint16_t ch1 = apds9930_read_word(sensor, APDS9930_IRDATAL); // IR channel
    
    if (ch0 == 0xFFFF || ch1 == 0xFFFF) {
        return -1;  // Ошибка чтения
    }
    
    // Сохраняем сырые данные
    data->clear_light = ch0;
    data->ir_light = ch1;
    
    // Вычисляем освещенность в люксах
    // Формула из datasheet: Lux = (CH0 - B * CH1) / C
    float ratio = (float)ch1 / (float)ch0;
    
    if (ratio < 0.5f) {
        data->ambient_light = (1.0f * ch0 - 1.64f * ch1);
    } else if (ratio < 0.61f) {
        data->ambient_light = (0.6f * ch0 - 0.59f * ch1);
    } else if (ratio < 0.80f) {
        data->ambient_light = (0.4f * ch0 - 0.26f * ch1);
    } else if (ratio < 1.00f) {
        data->ambient_light = (0.2f * ch0 - 0.13f * ch1);
    } else {
        data->ambient_light = 0.0f;
    }
    
    // Применяем калибровочные коэффициенты
    data->ambient_light *= sensor->calib.ch0_coeff;
    
    return 0;
}

// Чтение данных проксимити
int apds9930_read_proximity_data(APDS9930* sensor, APDS9930_Data* data) {
    // Проверяем готовность данных проксимити
    uint8_t status = apds9930_read_byte(sensor, APDS9930_STATUS);
    if (!(status & APDS9930_PVALID)) {
        return -1;  // Данные не готовы
    }
    
    // Читаем данные проксимити
    data->proximity = apds9930_read_word(sensor, APDS9930_PDATAL);
    
    if (data->proximity == 0xFFFF) {
        return -1;  // Ошибка чтения
    }
    
    return 0;
}

// Чтение всех данных
int apds9930_read_all_data(APDS9930* sensor, APDS9930_Data* data) {
    int light_result = apds9930_read_light_data(sensor, data);
    int prox_result = apds9930_read_proximity_data(sensor, data);
    
    return (light_result == 0 || prox_result == 0) ? 0 : -1;
}

// Установка коэффициентов калибровки
void apds9930_set_calibration(APDS9930* sensor, float ch0_coeff, float ch1_coeff, 
                             float df, float ga) {
    sensor->calib.ch0_coeff = ch0_coeff;
    sensor->calib.ch1_coeff = ch1_coeff;
    sensor->calib.df = df;
    sensor->calib.ga = ga;
}

// Получение времени интегрирования в миллисекундах
float apds9930_get_integration_time(APDS9930* sensor) {
    return (256 - sensor->atime) * 2.73f;
}

// Установка усиления
int apds9930_set_gain(APDS9930* sensor, uint8_t gain) {
    if (gain > 3) {
        fprintf(stderr, "Invalid gain value. Use 0-3\n");
        return -1;
    }
    
    sensor->again = gain;
    return apds9930_write_byte(sensor, APDS9930_CONTROL, gain);
}

// Получение значения усиления
const char* apds9930_get_gain_name(uint8_t gain) {
    switch (gain) {
        case 0: return "1x";
        case 1: return "8x";
        case 2: return "16x";
        case 3: return "120x";
        default: return "Unknown";
    }
}

// Выключение датчика
int apds9930_power_down(APDS9930* sensor) {
    return apds9930_write_byte(sensor, APDS9930_ENABLE, 0x00);
}

// Включение датчика
int apds9930_power_on(APDS9930* sensor) {
    uint8_t enable_reg = APDS9930_PON | APDS9930_AEN | APDS9930_PEN;
    return apds9930_write_byte(sensor, APDS9930_ENABLE, enable_reg);
}

// Освобождение ресурсов
void apds9930_close(APDS9930* sensor) {
    if (sensor) {
        // Выключаем датчик перед закрытием
        apds9930_power_down(sensor);
        
        if (sensor->i2c_fd >= 0) {
            close(sensor->i2c_fd);
        }
        free(sensor);
    }
}

// Функция для оценки уровня освещенности
const char* get_ambient_light_level(float lux) {
    if (lux < 1) return "Полная темнота";
    if (lux < 10) return "Очень темно";
    if (lux < 50) return "Темно";
    if (lux < 100) return "Сумерки";
    if (lux < 200) return "Тусклый свет";
    if (lux < 400) return "Нормальное освещение";
    if (lux < 1000) return "Яркое освещение";
    if (lux < 5000) return "Очень ярко";
    return "Ослепительно ярко";
}

// Функция для оценки расстояния проксимити
const char* get_proximity_level(uint16_t proximity) {
    if (proximity < 100) return "Нет объекта";
    if (proximity < 200) return "Далеко";
    if (proximity < 500) return "Средняя дистанция";
    if (proximity < 1000) return "Близко";
    if (proximity < 2000) return "Очень близко";
    return "Контакт";
}

// Переменная для обработки Ctrl+C
static volatile int keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

// Демонстрация работы с разными усилениями
void demo_gain_modes(APDS9930* sensor) {
    uint8_t gains[] = {0, 1, 2, 3};  // 1x, 8x, 16x, 120x
    int num_gains = sizeof(gains) / sizeof(gains[0]);
    
    printf("\n=== Демонстрация разных усилений ===\n");
    
    for (int i = 0; i < num_gains && keep_running; i++) {
        if (apds9930_set_gain(sensor, gains[i]) == 0) {
            printf("Усиление: %s\n", apds9930_get_gain_name(gains[i]));
            
            // Даем время на стабилизацию
            usleep(100000);
            
            // Делаем несколько измерений
            for (int j = 0; j < 3 && keep_running; j++) {
                APDS9930_Data data;
                if (apds9930_read_all_data(sensor, &data) == 0) {
                    printf("  Измерение %d: %.1f lux | Proximity: %d\n", 
                           j + 1, data.ambient_light, data.proximity);
                }
                usleep(50000);
            }
            printf("\n");
        }
        sleep(1);
    }
}

int main(int argc, char *argv[]) {
    printf("Инициализация датчика APDS-9930...\n");
    
    APDS9930* sensor = apds9930_init("/dev/i2c-1", APDS9930_I2C_ADDR);
    if (!sensor) {
        fprintf(stderr, "Не удалось инициализировать датчик APDS-9930\n");
        fprintf(stderr, "Проверьте подключение и адрес датчика (0x39)\n");
        return 1;
    }
    
    // Устанавливаем обработчик сигнала для Ctrl+C
    signal(SIGINT, signal_handler);
    
    int mode = 0;
    if (argc > 1) {
        mode = atoi(argv[1]);
    }
    
    if (mode == 1) {
        // Режим демонстрации усилений
        demo_gain_modes(sensor);
    } else {
        // Основной режим - непрерывное измерение
        printf("Режим непрерывного измерения APDS-9930\n");
        printf("Освещенность и проксимити в реальном времени (Ctrl+C для выхода):\n");
        printf("=================================================================\n");
        
        time_t last_time = time(NULL);
        int measurement_count = 0;
        
        APDS9930_Data data;
        float total_lux = 0;
        uint32_t total_prox = 0;
        float min_lux = 1000000, max_lux = 0;
        uint16_t min_prox = 0xFFFF, max_prox = 0;
        
        while (keep_running) {
            if (apds9930_read_all_data(sensor, &data) == 0) {
                measurement_count++;
                
                // Статистика по освещенности
                total_lux += data.ambient_light;
                if (data.ambient_light < min_lux) min_lux = data.ambient_light;
                if (data.ambient_light > max_lux) max_lux = data.ambient_light;
                
                // Статистика по проксимити
                total_prox += data.proximity;
                if (data.proximity < min_prox) min_prox = data.proximity;
                if (data.proximity > max_prox) max_prox = data.proximity;
                
                // Выводим текущие значения
                printf("\rОсвещенность: %6.1f lux [%s] | Proximity: %4d [%s] | Clear: %4d | IR: %4d", 
                       data.ambient_light, get_ambient_light_level(data.ambient_light),
                       data.proximity, get_proximity_level(data.proximity),
                       data.clear_light, data.ir_light);
                fflush(stdout);
                
                // Каждые 5 секунд выводим статистику
                time_t current_time = time(NULL);
                if (current_time - last_time >= 5) {
                    printf("\n--- Статистика за последние 5 сек ---\n");
                    printf("Освещенность: Среднее: %6.1f lux | Мин: %6.1f | Макс: %6.1f\n", 
                           total_lux / measurement_count, min_lux, max_lux);
                    printf("Proximity:    Среднее: %6.1f | Мин: %4d | Макс: %4d\n", 
                           (float)total_prox / measurement_count, min_prox, max_prox);
                    printf("Время интегрирования: %.1f ms | Усиление: %s\n",
                           apds9930_get_integration_time(sensor),
                           apds9930_get_gain_name(sensor->again));
                    printf("=================================================================\n");
                    
                    // Сбрасываем статистику
                    last_time = current_time;
                    measurement_count = 0;
                    total_lux = 0;
                    total_prox = 0;
                    min_lux = 1000000;
                    max_lux = 0;
                    min_prox = 0xFFFF;
                    max_prox = 0;
                }
            } else {
                printf("\rОжидание данных...                                          ");
            }
            
            usleep(100000);  // Измерение каждые 100ms
        }
    }
    
    printf("\n\nЗавершение работы...\n");
    apds9930_close(sensor);
    
    return 0;
}
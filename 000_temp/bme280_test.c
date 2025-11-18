#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <signal.h>

// Структура для калибровочных данных BME280
typedef struct {
    uint16_t dig_T1;
    int16_t dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    uint8_t dig_H1, dig_H3;
    int16_t dig_H2, dig_H4, dig_H5, dig_H6;
} BME280_CalibData;

// Структура для данных датчика
typedef struct {
    float temperature;
    float pressure;
    float humidity;
} BME280_Data;

// Структура основного объекта BME280
typedef struct {
    int i2c_fd;
    uint8_t i2c_addr;
    BME280_CalibData calib;
    int32_t t_fine;
} BME280;

// Регистры BME280
#define BME280_REGISTER_DIG_T1        0x88
#define BME280_REGISTER_DIG_T2        0x8A
#define BME280_REGISTER_DIG_T3        0x8C
#define BME280_REGISTER_DIG_P1        0x8E
#define BME280_REGISTER_DIG_P2        0x90
#define BME280_REGISTER_DIG_P3        0x92
#define BME280_REGISTER_DIG_P4        0x94
#define BME280_REGISTER_DIG_P5        0x96
#define BME280_REGISTER_DIG_P6        0x98
#define BME280_REGISTER_DIG_P7        0x9A
#define BME280_REGISTER_DIG_P8        0x9C
#define BME280_REGISTER_DIG_P9        0x9E
#define BME280_REGISTER_DIG_H1        0xA1
#define BME280_REGISTER_DIG_H2        0xE1
#define BME280_REGISTER_DIG_H3        0xE3
#define BME280_REGISTER_DIG_H4        0xE4
#define BME280_REGISTER_DIG_H5        0xE5
#define BME280_REGISTER_DIG_H6        0xE7
#define BME280_REGISTER_CHIPID        0xD0
#define BME280_REGISTER_SOFTRESET     0xE0
#define BME280_REGISTER_CONTROLHUMID  0xF2
#define BME280_REGISTER_CONTROL       0xF4
#define BME280_REGISTER_PRESSUREDATA  0xF7
#define BME280_REGISTER_TEMPDATA      0xFA
#define BME280_REGISTER_HUMIDDATA     0xFD

// Функции для работы с I2C
static uint8_t bme280_read_byte(BME280* sensor, uint8_t reg) {
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

static uint16_t bme280_read_word(BME280* sensor, uint8_t reg) {
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

static int16_t bme280_read_word_signed(BME280* sensor, uint8_t reg) {
    return (int16_t)bme280_read_word(sensor, reg);
}

static void bme280_write_byte(BME280* sensor, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    if (write(sensor->i2c_fd, data, 2) != 2) {
        perror("I2C write failed");
    }
}

// Чтение калибровочных данных
static void bme280_read_calibration_data(BME280* sensor) {
    BME280_CalibData* calib = &sensor->calib;
    
    // Температура
    calib->dig_T1 = bme280_read_word(sensor, BME280_REGISTER_DIG_T1);
    calib->dig_T2 = bme280_read_word_signed(sensor, BME280_REGISTER_DIG_T2);
    calib->dig_T3 = bme280_read_word_signed(sensor, BME280_REGISTER_DIG_T3);
    
    // Давление
    calib->dig_P1 = bme280_read_word(sensor, BME280_REGISTER_DIG_P1);
    calib->dig_P2 = bme280_read_word_signed(sensor, BME280_REGISTER_DIG_P2);
    calib->dig_P3 = bme280_read_word_signed(sensor, BME280_REGISTER_DIG_P3);
    calib->dig_P4 = bme280_read_word_signed(sensor, BME280_REGISTER_DIG_P4);
    calib->dig_P5 = bme280_read_word_signed(sensor, BME280_REGISTER_DIG_P5);
    calib->dig_P6 = bme280_read_word_signed(sensor, BME280_REGISTER_DIG_P6);
    calib->dig_P7 = bme280_read_word_signed(sensor, BME280_REGISTER_DIG_P7);
    calib->dig_P8 = bme280_read_word_signed(sensor, BME280_REGISTER_DIG_P8);
    calib->dig_P9 = bme280_read_word_signed(sensor, BME280_REGISTER_DIG_P9);
    
    // Влажность
    calib->dig_H1 = bme280_read_byte(sensor, BME280_REGISTER_DIG_H1);
    calib->dig_H2 = bme280_read_word_signed(sensor, BME280_REGISTER_DIG_H2);
    calib->dig_H3 = bme280_read_byte(sensor, BME280_REGISTER_DIG_H3);
    
    uint8_t h4_msb = bme280_read_byte(sensor, 0xE4);
    uint8_t h4_lsb = bme280_read_byte(sensor, 0xE5);
    uint8_t h5_msb = bme280_read_byte(sensor, 0xE6);
    uint8_t h5_lsb = bme280_read_byte(sensor, 0xE5);
    
    calib->dig_H4 = (h4_msb << 4) | (h4_lsb & 0x0F);
    calib->dig_H5 = (h5_msb << 4) | (h5_lsb >> 4);
    calib->dig_H6 = (int8_t)bme280_read_byte(sensor, BME280_REGISTER_DIG_H6);
}

// Инициализация датчика
static void bme280_initialize(BME280* sensor) {
    // Сброс
    bme280_write_byte(sensor, BME280_REGISTER_SOFTRESET, 0xB6);
    usleep(10000);
    
    // Настройка влажности
    bme280_write_byte(sensor, BME280_REGISTER_CONTROLHUMID, 0x01);
    
    // Настройка температуры и давления
    bme280_write_byte(sensor, BME280_REGISTER_CONTROL, 0x27);
}

// Компенсация температуры
static int32_t bme280_compensate_temperature(BME280* sensor, int32_t adc_T) {
    BME280_CalibData* calib = &sensor->calib;
    int32_t var1, var2;
    
    var1 = ((((adc_T >> 3) - ((int32_t)calib->dig_T1 << 1))) * ((int32_t)calib->dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib->dig_T1)) * 
             ((adc_T >> 4) - ((int32_t)calib->dig_T1))) >> 12) * 
             ((int32_t)calib->dig_T3)) >> 14;
    
    sensor->t_fine = var1 + var2;
    return (sensor->t_fine * 5 + 128) >> 8;
}

// Компенсация давления
static uint32_t bme280_compensate_pressure(BME280* sensor, int32_t adc_P) {
    BME280_CalibData* calib = &sensor->calib;
    int64_t var1, var2, p;
    
    var1 = ((int64_t)sensor->t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib->dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib->dig_P5) << 17);
    var2 = var2 + (((int64_t)calib->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib->dig_P3) >> 8) + 
           ((var1 * (int64_t)calib->dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib->dig_P1) >> 33;
    
    if (var1 == 0) {
        return 0;
    }
    
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib->dig_P8) * p) >> 19;
    
    p = ((p + var1 + var2) >> 8) + (((int64_t)calib->dig_P7) << 4);
    return (uint32_t)p / 256;
}

// Компенсация влажности
static uint32_t bme280_compensate_humidity(BME280* sensor, int32_t adc_H) {
    BME280_CalibData* calib = &sensor->calib;
    int32_t v_x1_u32r;
    
    v_x1_u32r = (sensor->t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib->dig_H4) << 20) - 
                   (((int32_t)calib->dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * 
                 (((((((v_x1_u32r * ((int32_t)calib->dig_H6)) >> 10) * 
                 (((v_x1_u32r * ((int32_t)calib->dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + 
                 ((int32_t)2097152)) * ((int32_t)calib->dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * 
                 ((int32_t)calib->dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    
    return (uint32_t)(v_x1_u32r >> 12);
}

// Инициализация BME280
BME280* bme280_init(const char* i2c_device, uint8_t address) {
    BME280* sensor = malloc(sizeof(BME280));
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
    sensor->t_fine = 0;
    
    // Устанавливаем адрес устройства
    if (ioctl(sensor->i2c_fd, I2C_SLAVE, sensor->i2c_addr) < 0) {
        perror("Failed to set I2C address");
        close(sensor->i2c_fd);
        free(sensor);
        return NULL;
    }
    
    // Проверяем ID устройства
    if (bme280_read_byte(sensor, BME280_REGISTER_CHIPID) != 0x60) {
        fprintf(stderr, "Invalid BME280 chip ID\n");
        close(sensor->i2c_fd);
        free(sensor);
        return NULL;
    }
    
    // Читаем калибровочные данные
    bme280_read_calibration_data(sensor);
    
    // Инициализируем датчик
    bme280_initialize(sensor);
    
    return sensor;
}

// Чтение данных с датчика
int bme280_read_data(BME280* sensor, BME280_Data* data) {
    uint8_t reg = BME280_REGISTER_PRESSUREDATA;
    uint8_t sensor_data[8];
    
    // Читаем данные давления и температуры
    if (write(sensor->i2c_fd, &reg, 1) != 1) {
        perror("I2C write failed");
        return -1;
    }
    
    if (read(sensor->i2c_fd, sensor_data, 8) != 8) {
        perror("I2C read failed");
        return -1;
    }
    
    int32_t adc_P = (sensor_data[0] << 12) | (sensor_data[1] << 4) | (sensor_data[2] >> 4);
    int32_t adc_T = (sensor_data[3] << 12) | (sensor_data[4] << 4) | (sensor_data[5] >> 4);
    
    // Читаем данные влажности
    reg = BME280_REGISTER_HUMIDDATA;
    uint8_t hum_data[2];
    
    if (write(sensor->i2c_fd, &reg, 1) != 1) {
        perror("I2C write failed");
        return -1;
    }
    
    if (read(sensor->i2c_fd, hum_data, 2) != 2) {
        perror("I2C read failed");
        return -1;
    }
    
    int32_t adc_H = (hum_data[0] << 8) | hum_data[1];
    
    // Компенсируем данные
    int32_t t = bme280_compensate_temperature(sensor, adc_T);
    uint32_t p = bme280_compensate_pressure(sensor, adc_P);
    uint32_t h = bme280_compensate_humidity(sensor, adc_H);
    
    data->temperature = t / 100.0f;
    data->pressure = p / 100.0f;
    data->humidity = h / 1024.0f;
    
    return 0;
}

// Освобождение ресурсов
void bme280_close(BME280* sensor) {
    if (sensor) {
        if (sensor->i2c_fd >= 0) {
            close(sensor->i2c_fd);
        }
        free(sensor);
    }
}

// Переменная для обработки Ctrl+C
static volatile int keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

int main() {
    printf("Инициализация датчика BME280...\n");
    
    BME280* sensor = bme280_init("/dev/i2c-1", 0x76);
    if (!sensor) {
        fprintf(stderr, "Не удалось инициализировать датчик BME280\n");
        return 1;
    }
    
    printf("Датчик успешно инициализирован!\n");
    printf("Чтение данных (для остановки нажмите Ctrl+C):\n");
    printf("----------------------------------------\n");
    
    // Устанавливаем обработчик сигнала для Ctrl+C
    signal(SIGINT, signal_handler);
    
    BME280_Data data;
    
    while (keep_running) {
        if (bme280_read_data(sensor, &data) == 0) {
            printf("Температура: %.2f °C\n", data.temperature);
            printf("Давление: %.2f hPa\n", data.pressure);
            printf("Влажность: %.2f %%\n", data.humidity);
            printf("----------------------------------------\n");
        } else {
            fprintf(stderr, "Ошибка чтения данных с датчика\n");
            break;
        }
        
        sleep(2);
    }
    
    printf("\nЗавершение работы...\n");
    bme280_close(sensor);
    
    return 0;
}
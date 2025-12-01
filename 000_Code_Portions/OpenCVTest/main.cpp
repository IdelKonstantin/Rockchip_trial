#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <fstream>  // Добавьте этот заголовок для ofstream
#include <iomanip>  // Добавьте этот заголовок для setprecision
#include <sstream>
#include <omp.h>

using namespace cv;
using namespace std;
using namespace std::chrono;

// Однопоточная версия (для сравнения)
void overlayImageSingleThread(Mat& background, const Mat& overlay, int x, int y, double scale = 1.0) {
    Mat scaledOverlay;
    if (scale != 1.0) {
        resize(overlay, scaledOverlay, Size(), scale, scale, INTER_LINEAR);
    } else {
        scaledOverlay = overlay;
    }
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + scaledOverlay.cols > background.cols) {
        x = background.cols - scaledOverlay.cols;
    }
    if (y + scaledOverlay.rows > background.rows) {
        y = background.rows - scaledOverlay.rows;
    }
    
    for (int i = 0; i < scaledOverlay.rows; i++) {
        for (int j = 0; j < scaledOverlay.cols; j++) {
            Point pos(j + x, i + y);
            
            if (pos.x >= background.cols || pos.y >= background.rows) {
                continue;
            }
            
            Vec4b pixel = scaledOverlay.at<Vec4b>(i, j);
            
            if (pixel[3] > 0) {
                double alpha = pixel[3] / 255.0;
                Vec3b bgPixel = background.at<Vec3b>(pos);
                
                Vec3b result;
                result[0] = saturate_cast<uchar>(pixel[0] * alpha + bgPixel[0] * (1 - alpha));
                result[1] = saturate_cast<uchar>(pixel[1] * alpha + bgPixel[1] * (1 - alpha));
                result[2] = saturate_cast<uchar>(pixel[2] * alpha + bgPixel[2] * (1 - alpha));
                
                background.at<Vec3b>(pos) = result;
            }
        }
    }
}

// Многопоточная версия с OpenMP
void overlayImageParallel(Mat& background, const Mat& overlay, int x, int y, double scale = 1.0) {
    Mat scaledOverlay;
    if (scale != 1.0) {
        resize(overlay, scaledOverlay, Size(), scale, scale, INTER_LINEAR);
    } else {
        scaledOverlay = overlay;
    }
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + scaledOverlay.cols > background.cols) {
        x = background.cols - scaledOverlay.cols;
    }
    if (y + scaledOverlay.rows > background.rows) {
        y = background.rows - scaledOverlay.rows;
    }
    
    const int rows = scaledOverlay.rows;
    const int cols = scaledOverlay.cols;
    
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            Point pos(j + x, i + y);
            
            if (pos.x >= background.cols || pos.y >= background.rows) {
                continue;
            }
            
            Vec4b pixel = scaledOverlay.at<Vec4b>(i, j);
            
            if (pixel[3] > 0) {
                double alpha = pixel[3] / 255.0;
                Vec3b bgPixel = background.at<Vec3b>(pos);
                
                Vec3b result;
                result[0] = saturate_cast<uchar>(pixel[0] * alpha + bgPixel[0] * (1 - alpha));
                result[1] = saturate_cast<uchar>(pixel[1] * alpha + bgPixel[1] * (1 - alpha));
                result[2] = saturate_cast<uchar>(pixel[2] * alpha + bgPixel[2] * (1 - alpha));
                
                background.at<Vec3b>(pos) = result;
            }
        }
    }
}

// Функция для загрузки PNG с прозрачностью
Mat loadIconWithAlpha(const string& filename) {
    Mat image = imread(filename, IMREAD_UNCHANGED);
    if (image.empty()) {
        throw runtime_error("Не удалось загрузить иконку: " + filename);
    }
    
    if (image.channels() == 3) {
        cvtColor(image, image, COLOR_BGR2BGRA);
    }
    
    return image;
}

// Функция для измерения времени выполнения
template<typename Func>
long long measureTime(Func&& func) {
    auto start = high_resolution_clock::now();
    func();
    auto end = high_resolution_clock::now();
    return duration_cast<milliseconds>(end - start).count();
}

// Функция для генерации имени файла с временной меткой
string generateFilename(const string& baseName, const string& extension, const string& suffix = "") {
    auto now = system_clock::now();
    auto time_t_now = system_clock::to_time_t(now);
    tm local_tm = *localtime(&time_t_now);
    
    stringstream ss;
    ss << baseName;
    if (!suffix.empty()) {
        ss << "_" << suffix;
    }
    ss << "_" << put_time(&local_tm, "%Y%m%d_%H%M%S") << "." << extension;
    return ss.str();
}

// Функция для сохранения изображения с проверкой
bool saveImageWithCheck(const Mat& image, const string& filename, int quality = 95) {
    vector<int> compression_params;
    compression_params.push_back(IMWRITE_JPEG_QUALITY);
    compression_params.push_back(quality);
    
    bool success = imwrite(filename, image, compression_params);
    if (success) {
        cout << "Изображение сохранено: " << filename << endl;
        
        // Проверяем, что файл действительно создан и читается
        Mat checkImage = imread(filename);
        if (!checkImage.empty()) {
            cout << "✓ Файл проверен: " << checkImage.cols << "x" << checkImage.rows << " пикселей" << endl;
        } else {
            cerr << "✗ Ошибка: сохраненный файл не читается!" << endl;
            return false;
        }
    } else {
        cerr << "✗ Ошибка сохранения файла: " << filename << endl;
        cerr << "Проверьте права доступа к директории!" << endl;
    }
    return success;
}

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            cout << "Использование: " << argv[0] << " <фон> <иконка> [x y scale threads]" << endl;
            cout << "Пример: " << argv[0] << " background.jpg icon.png 100 100 0.5 8" << endl;
            return -1;
        }

        // Настройка количества потоков
        int numThreads = 8;
        if (argc >= 7) {
            numThreads = atoi(argv[6]);
        }
        omp_set_num_threads(numThreads);
        
        string backgroundFile = argv[1];
        string iconFile = argv[2];
        
        int x = 50, y = 50;
        double scale = 1.0;
        
        if (argc >= 5) {
            x = atoi(argv[3]);
            y = atoi(argv[4]);
        }
        if (argc >= 6) {
            scale = atof(argv[5]);
        }
        
        cout << "=== НАСТРОЙКИ ===" << endl;
        cout << "Потоков: " << omp_get_max_threads() << endl;
        cout << "Позиция иконки: (" << x << ", " << y << ")" << endl;
        cout << "Масштаб иконки: " << scale << endl;
        
        // Загружаем изображения
        Mat background = imread(backgroundFile);
        if (background.empty()) {
            throw runtime_error("Не удалось загрузить основное изображение: " + backgroundFile);
        }
        
        Mat icon = loadIconWithAlpha(iconFile);
        cout << "Фон: " << background.cols << "x" << background.rows << endl;
        cout << "Иконка: " << icon.cols << "x" << icon.rows << " (масштаб: " << scale << ")" << endl;
        
        // Сохраняем оригинальные изображения (для отладки)
        saveImageWithCheck(background, "original_background.jpg", 95);
        
        // Тестируем однопоточную версию
        cout << "\n=== ТЕСТ ОДНОПОТОЧНОЙ ВЕРСИИ ===" << endl;
        Mat resultSingle = background.clone();
        long long timeSingle = measureTime([&]() {
            overlayImageSingleThread(resultSingle, icon, x, y, scale);
        });
        
        // Сохраняем однопоточный результат
        string singleThreadFile = generateFilename("result", "jpg", "single_thread");
        saveImageWithCheck(resultSingle, singleThreadFile, 95);
        
        // Тестируем многопоточную версию
        cout << "\n=== ТЕСТ МНОГОПОТОЧНОЙ ВЕРСИИ ===" << endl;
        Mat resultParallel = background.clone();
        long long timeParallel = measureTime([&]() {
            overlayImageParallel(resultParallel, icon, x, y, scale);
        });
        
        // Сохраняем многопоточный результат
        string parallelFile = generateFilename("result", "jpg", "parallel");
        saveImageWithCheck(resultParallel, parallelFile, 95);
        
        // Сохраняем разницу (для отладки)
        Mat diff;
        string diffFile;
        compare(resultSingle, resultParallel, diff, CMP_NE);
        int differentPixels = countNonZero(diff);
        if (differentPixels > 0) {
            diffFile = generateFilename("difference", "jpg", "debug");
            saveImageWithCheck(diff, diffFile, 95);
        }
        
        // Выводим результаты бенчмарка
        cout << "\n=== РЕЗУЛЬТАТЫ БЕНЧМАРКА ===" << endl;
        cout << "Однопоточная версия: " << timeSingle << " мс" << endl;
        cout << "Многопоточная версия: " << timeParallel << " мс" << endl;
        cout << "Ускорение: " << fixed << setprecision(2) << (double)timeSingle / timeParallel << "x" << endl;
        cout << "Различающихся пикселей: " << differentPixels << endl;
        
        // Сохраняем сводку результатов в текстовый файл
        string summaryFile = generateFilename("benchmark", "txt", "summary");
        ofstream summary(summaryFile);
        if (summary.is_open()) {
            summary << "БЕНЧМАРК НАЛОЖЕНИЯ ИКОНОК" << endl;
            summary << "========================" << endl;
            summary << "Время выполнения: " << endl;
            summary << "  Однопоточная версия: " << timeSingle << " мс" << endl;
            summary << "  Многопоточная версия: " << timeParallel << " мс" << endl;
            summary << "  Ускорение: " << fixed << setprecision(2) << (double)timeSingle / timeParallel << "x" << endl;
            summary << endl;
            summary << "Настройки: " << endl;
            summary << "  Потоков: " << omp_get_max_threads() << endl;
            summary << "  Позиция: (" << x << ", " << y << ")" << endl;
            summary << "  Масштаб: " << scale << endl;
            summary << "  Размер фона: " << background.cols << "x" << background.rows << endl;
            summary << "  Размер иконки: " << icon.cols << "x" << icon.rows << endl;
            summary << endl;
            summary << "Файлы результатов: " << endl;
            summary << "  Однопоточный: " << singleThreadFile << endl;
            summary << "  Многопоточный: " << parallelFile << endl;
            if (differentPixels > 0) {
                summary << "  Разница: " << diffFile << " (" << differentPixels << " пикселей)" << endl;
            }
            summary.close();
            cout << "Сводка сохранена: " << summaryFile << endl;
        }
        
        // Сохраняем финальный результат (многопоточная версия)
        string finalFile = "final_result.jpg";
        if (saveImageWithCheck(resultParallel, finalFile, 95)) {
            cout << "\n✓ Финальный результат: " << finalFile << endl;
        }
        
    } catch (const exception& e) {
        cerr << "❌ Ошибка: " << e.what() << endl;
        return -1;
    }
    
    cout << "\n🎉 Программа завершена успешно!" << endl;
    return 0;
}
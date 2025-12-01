#include "plugin_interface.h"
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>

class PluginHandle {
private:
    void* handle_;
    std::unique_ptr<Plugin> plugin_;
    
public:
    PluginHandle(void* handle, Plugin* plugin) 
        : handle_(handle), plugin_(plugin) {}
    
    ~PluginHandle() {
        // Сначала удаляем плагин, потом закрываем handle
        plugin_.reset(); // Явно освобождаем плагин
        
        if (handle_) {
            // Получаем функцию уничтожения перед закрытием
            auto destroy_func = reinterpret_cast<void(*)(Plugin*)>(
                dlsym(handle_, "destroy_plugin"));
            
            if (!dlerror() && destroy_func) {
                // Функция destroy_plugin сама вызовет delete
                // поэтому сбрасываем unique_ptr без вызова delete
                plugin_.release();
            }
            
            dlclose(handle_);
        }
    }
    
    // Запрещаем копирование
    PluginHandle(const PluginHandle&) = delete;
    PluginHandle& operator=(const PluginHandle&) = delete;
    
    // Разрешаем перемещение
    PluginHandle(PluginHandle&& other) noexcept 
        : handle_(other.handle_), plugin_(std::move(other.plugin_)) {
        other.handle_ = nullptr;
    }
    
    PluginHandle& operator=(PluginHandle&& other) noexcept {
        if (this != &other) {
            // Освобождаем текущие ресурсы
            plugin_.reset();
            if (handle_) {
                dlclose(handle_);
            }
            
            // Перемещаем из other
            handle_ = other.handle_;
            plugin_ = std::move(other.plugin_);
            other.handle_ = nullptr;
        }
        return *this;
    }
    
    Plugin* get() const { return plugin_.get(); }
    Plugin* operator->() const { return plugin_.get(); }
    
    std::string getName() const { 
        return plugin_ ? plugin_->getName() : "Invalid Plugin"; 
    }
};

class PluginLoader {
private:
    std::vector<PluginHandle> plugins_;
    
    // Проверка, является ли файл обычным файлом
    bool isRegularFile(const std::string& path) {
        struct stat statbuf;
        if (stat(path.c_str(), &statbuf) == 0) {
            return S_ISREG(statbuf.st_mode);
        }
        return false;
    }
    
    // Получение расширения файла
    std::string getFileExtension(const std::string& filename) {
        size_t dotPos = filename.find_last_of(".");
        if (dotPos != std::string::npos) {
            return filename.substr(dotPos);
        }
        return "";
    }
    
public:
    // Загрузка всех плагинов из указанной директории
    void loadFromDirectory(const std::string& directory) {
        std::cout << "Поиск плагинов в директории: " << directory << std::endl;
        
        DIR* dir = opendir(directory.c_str());
        if (!dir) {
            std::cerr << "Не удалось открыть директорию: " << directory << std::endl;
            return;
        }
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            
            // Пропускаем текущую и родительскую директории
            if (filename == "." || filename == "..") {
                continue;
            }
            
            std::string fullpath = directory + "/" + filename;
            
            // Проверяем, что это обычный файл с расширением .so
            if (isRegularFile(fullpath) && getFileExtension(filename) == ".so") {
                loadPlugin(fullpath);
            }
        }
        
        closedir(dir);
    }
    
    // Загрузка конкретного плагина
    bool loadPlugin(const std::string& filepath) {
        // Открываем shared library
        void* handle = dlopen(filepath.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Ошибка загрузки плагина " << filepath 
                      << ": " << dlerror() << std::endl;
            return false;
        }
        
        // Получаем указатель на функцию создания
        auto create_func = reinterpret_cast<Plugin*(*)()>(
            dlsym(handle, "create_plugin"));
        
        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            std::cerr << "Ошибка поиска create_plugin в " << filepath 
                      << ": " << dlsym_error << std::endl;
            dlclose(handle);
            return false;
        }
        
        // Проверяем функцию уничтожения
        auto destroy_func = reinterpret_cast<void(*)(Plugin*)>(
            dlsym(handle, "destroy_plugin"));
        
        dlsym_error = dlerror();
        if (dlsym_error) {
            std::cerr << "Ошибка поиска destroy_plugin в " << filepath 
                      << ": " << dlsym_error << std::endl;
            dlclose(handle);
            return false;
        }
        
        // Создаем экземпляр плагина
        Plugin* plugin = create_func();
        if (!plugin) {
            std::cerr << "Ошибка создания плагина из " << filepath << std::endl;
            dlclose(handle);
            return false;
        }
        
        // Создаем PluginHandle который управляет временем жизни
        plugins_.emplace_back(handle, plugin);
        std::cout << "Загружен плагин: " << plugin->getName() 
                  << " из " << filepath << std::endl;
        
        return true;
    }
    
    // Вывод информации о всех загруженных плагинах
    void listPlugins() const {
        std::cout << "\n=== Загруженные плагины (" << plugins_.size() 
                  << ") ===" << std::endl;
        for (size_t i = 0; i < plugins_.size(); ++i) {
            std::cout << i + 1 << ". " << plugins_[i].getName() << std::endl;
        }
        std::cout << "===========================" << std::endl;
    }
    
    // Демонстрация работы с плагинами (должна быть вызвана ДО уничтожения PluginLoader)
    void demonstratePlugins() {
        std::cout << "\nДемонстрация работы с плагинами:" << std::endl;
        for (size_t i = 0; i < plugins_.size(); ++i) {
            // Используем плагины пока они еще загружены
            std::cout << "Плагин " << i + 1 << ": " << plugins_[i].getName() << std::endl;
        }
    }
    
    // Получение плагина по индексу
    Plugin* getPlugin(size_t index) const {
        if (index < plugins_.size()) {
            return plugins_[index].get();
        }
        return nullptr;
    }
    
    // Получение имени плагина по индексу
    std::string getPluginName(size_t index) const {
        if (index < plugins_.size()) {
            return plugins_[index].getName();
        }
        return "";
    }
    
    // Количество загруженных плагинов
    size_t count() const { return plugins_.size(); }
};

int main() {
    {
        // Создаем область видимости чтобы PluginLoader был уничтожен до return
        PluginLoader loader;
        
        // Загружаем плагины из директории plugins
        std::cout << "Загрузка плагинов..." << std::endl;
        loader.loadFromDirectory("./plugins");
        
        // Если в ./plugins нет плагинов, пробуем из текущей директории
        if (loader.count() == 0) {
            std::cout << "Плагины не найдены в ./plugins, поиск в текущей директории..." << std::endl;
            loader.loadFromDirectory(".");
        }
        
        // Показываем список загруженных плагинов
        loader.listPlugins();
        
        // Демонстрация работы с плагинами ДО выхода из области видимости
        loader.demonstratePlugins();
        
        // PluginLoader будет уничтожен здесь, после всех операций с плагинами
    } // Конец области видимости loader
    
    std::cout << "\nПрограмма завершена успешно." << std::endl;
    return 0;
}
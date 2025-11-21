#ifndef INI_PARSER_H
#define INI_PARSER_H

#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <utility>

class INIParser {
public:
    // Загрузить и распарсить INI-файл
    bool load(const std::string& filename);
    
    // Получить строковое значение
    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;
    
    // Получить целочисленное значение
    int getInt(const std::string& section, const std::string& key, int defaultValue = 0) const;
    
    // Получить значение с плавающей точкой
    double getDouble(const std::string& section, const std::string& key, double defaultValue = 0.0) const;
    
    // Получить булево значение
    bool getBool(const std::string& section, const std::string& key, bool defaultValue = false) const;
    
    // Проверить существование секции
    bool hasSection(const std::string& section) const;
    
    // Проверить существование ключа
    bool hasKey(const std::string& section, const std::string& key) const;
    
    // Получить все секции
    std::vector<std::string> getSections() const;
    
    // Получить все ключи в секции
    std::vector<std::string> getKeys(const std::string& section) const;
    
    // Установить значение (для создания/изменения INI в памяти)
    void setValue(const std::string& section, const std::string& key, const std::string& value);
    
    // Сохранить INI-файл
    bool save(const std::string& filename) const;
    
    // Очистить все данные
    void clear();

    // Преобразование строки с HEX с число
    static int hexStringToIntSstream(const std::string& hexStr);

private:
    // Структура для хранения данных: [секция -> [ключ -> значение]]
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data_;
    
    // Вспомогательные методы
    static std::string trim(const std::string& str);
    static std::string toLower(const std::string& str);
    static bool parseLine(const std::string& line, std::string& section, std::string& key, std::string& value);
};

#endif // INI_PARSER_H
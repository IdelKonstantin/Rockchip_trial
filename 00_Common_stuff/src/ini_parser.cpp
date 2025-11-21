#include "ini_parser.h"

#include <sstream>

bool INIParser::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string currentSection = "global";
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(file, line)) {
        lineNumber++;
        
        // Удаляем комментарии (все что после ; или #)
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // Обрезаем пробелы
        line = trim(line);
        
        // Пропускаем пустые строки
        if (line.empty()) {
            continue;
        }
        
        std::string section, key, value;
        if (parseLine(line, section, key, value)) {
            if (!section.empty()) {
                // Новая секция
                currentSection = section;
            } else if (!key.empty()) {
                // Ключ-значение
                data_[currentSection][key] = value;
            }
        } else {
            // Некорректная строка, можно добавить логирование
            continue;
        }
    }
    
    file.close();
    return true;
}

std::string INIParser::getString(const std::string& section, const std::string& key, 
                                const std::string& defaultValue) const {
    auto sectionIt = data_.find(section);
    if (sectionIt == data_.end()) {
        return defaultValue;
    }
    
    auto keyIt = sectionIt->second.find(key);
    if (keyIt == sectionIt->second.end()) {
        return defaultValue;
    }
    
    return keyIt->second;
}

int INIParser::getInt(const std::string& section, const std::string& key, 
                     int defaultValue) const {
    std::string value = getString(section, key);
    if (value.empty()) {
        return defaultValue;
    }
    
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return defaultValue;
    }
}

double INIParser::getDouble(const std::string& section, const std::string& key, 
                           double defaultValue) const {
    std::string value = getString(section, key);
    if (value.empty()) {
        return defaultValue;
    }
    
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        return defaultValue;
    }
}

bool INIParser::getBool(const std::string& section, const std::string& key, 
                       bool defaultValue) const {
    std::string value = toLower(getString(section, key));
    
    if (value.empty()) {
        return defaultValue;
    }
    
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return true;
    } else if (value == "false" || value == "0" || value == "no" || value == "off") {
        return false;
    }
    
    return defaultValue;
}

bool INIParser::hasSection(const std::string& section) const {
    return data_.find(section) != data_.end();
}

bool INIParser::hasKey(const std::string& section, const std::string& key) const {
    auto sectionIt = data_.find(section);
    if (sectionIt == data_.end()) {
        return false;
    }
    return sectionIt->second.find(key) != sectionIt->second.end();
}

std::vector<std::string> INIParser::getSections() const {
    std::vector<std::string> sections;
    sections.reserve(data_.size());
    
    for (const auto& pair : data_) {
        sections.push_back(pair.first);
    }
    
    return sections;
}

std::vector<std::string> INIParser::getKeys(const std::string& section) const {
    std::vector<std::string> keys;
    auto sectionIt = data_.find(section);
    
    if (sectionIt != data_.end()) {
        keys.reserve(sectionIt->second.size());
        for (const auto& pair : sectionIt->second) {
            keys.push_back(pair.first);
        }
    }
    
    return keys;
}

void INIParser::setValue(const std::string& section, const std::string& key, 
                        const std::string& value) {
    data_[section][key] = value;
}

bool INIParser::save(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& sectionPair : data_) {
        // Записываем секцию
        file << "[" << sectionPair.first << "]\n";
        
        // Записываем все ключи-значения в секции
        for (const auto& keyPair : sectionPair.second) {
            file << keyPair.first << " = " << keyPair.second << "\n";
        }
        
        file << "\n"; // Пустая строка между секциями
    }
    
    file.close();
    return true;
}

void INIParser::clear() {
    data_.clear();
}

// Вспомогательные методы

std::string INIParser::trim(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::string INIParser::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool INIParser::parseLine(const std::string& line, std::string& section, 
                         std::string& key, std::string& value) {
    section.clear();
    key.clear();
    value.clear();
    
    // Проверяем секцию [section_name]
    if (line.front() == '[' && line.back() == ']') {
        section = line.substr(1, line.length() - 2);
        section = trim(section);
        return true;
    }
    
    // Парсим ключ=значение
    size_t equalsPos = line.find('=');
    if (equalsPos != std::string::npos) {
        key = trim(line.substr(0, equalsPos));
        value = trim(line.substr(equalsPos + 1));
        
        // Удаляем кавычки если есть
        if (value.length() >= 2 && 
            ((value.front() == '"' && value.back() == '"') ||
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.length() - 2);
        }
        
        return !key.empty();
    }
    
    return false;
}

int hexStringToIntSstream(const std::string& hexStr) {

    std::stringstream ss;
    int value;
    
    ss << std::hex << hexStr;
    ss >> value;
    
    if (ss.fail()) {
        throw std::invalid_argument("Invalid HEX string: " + hexStr);
    }
    
    return value;
}

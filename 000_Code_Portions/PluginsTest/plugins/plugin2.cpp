#include "plugin_interface.h"
#include <string>

class Plugin2 : public Plugin {
public:
    std::string getName() const override {
        return "Плагин номер 2";
    }
};

// Экспортируемые функции
extern "C" {
    Plugin* create_plugin() {
        return new Plugin2();
    }
    
    void destroy_plugin(Plugin* plugin) {
        delete plugin;
    }
}
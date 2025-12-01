#include "plugin_interface.h"
#include <string>

class Plugin1 : public Plugin {
public:
    std::string getName() const override {
        return "Плагин номер 1";
    }
};

// Экспортируемые функции
extern "C" {
    Plugin* create_plugin() {
        return new Plugin1();
    }
    
    void destroy_plugin(Plugin* plugin) {
        delete plugin;
    }
}
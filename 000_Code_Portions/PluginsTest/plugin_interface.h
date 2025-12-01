#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <string>
#include <memory>

class Plugin {
public:
    virtual ~Plugin() = default;
    virtual std::string getName() const = 0;
};

using PluginPtr = std::unique_ptr<Plugin>;

// Функции, которые должны быть в каждом плагине SO
extern "C" {
    Plugin* create_plugin();
    void destroy_plugin(Plugin* plugin);
}

#endif
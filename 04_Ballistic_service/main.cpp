#include "json_working_stuff.h"
#include "ballistic_daemon.h"

#include <string>
#include <iostream>

int main(int argc, char* argv[]) {

    if(argc < 2) {

        std::cerr << "Не задан путь к файлу конфига. Использование:" << std::endl;
        std::cerr << argv[0] << " <путь к конфигу *.ini>" << std::endl;
        return 1;
    }

    ballisticDaemon ballDi{std::string(argv[1]), std::string(argv[0])};

    if(!ballDi.init()) {
        return 2;
    }

    ballDi.run();
    return 0;
}
#include "GPS_daemon.h"

#include <unistd.h>
#include <string>
#include <iostream>

int main(int argc, char* argv[]) {

    if(argc < 2) {

        std::cerr << "Не задан путь к файлу конфига. Использование:" << std::endl;
        std::cerr << argv[0] << " <путь к конфигу *.ini>" << std::endl;
        return 1;
    }

    gpsDaemon gps{std::string(argv[1]), std::string(argv[0])};

    //gpsDaemon gps{"conf.ini", "gps_daemon"};

    if(!gps.init()) {
        return 2;
    }

    gps.run();
    return 0;
}
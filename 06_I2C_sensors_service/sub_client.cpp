#include <iostream>
#include <memory>
#include <string>
#include <thread>

////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ini_parser.h"
#include "zhelpers.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

void getFromDaemon();

std::shared_ptr<zmq::socket_t> suber{nullptr};

////////////////////////////////////////////////////////////////////////////////////////////////////

int main () {

	INIParser iniParser;    
	iniParser.load("conf.ini");

	zmq::context_t context(1);

	/* Сокет для входящих сообщений для демона по расчету баллистики */

	auto subAddr = std::string("tcp://") + iniParser.getString("Zeromq_pub", "host", "localhost")
	+ std::string(":") + iniParser.getString("Zeromq_pub", "port", "5437");

	suber = std::make_shared<zmq::socket_t>(context, ZMQ_SUB);
	suber->setsockopt(ZMQ_SUBSCRIBE, "", 0);
	suber->connect(subAddr);

	std::thread t(getFromDaemon);

	t.join();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void getFromDaemon() {	/* Получение результатов */

	std::vector<zmq::pollitem_t> items = {
		{static_cast<void*>(*suber), 0, ZMQ_POLLIN, 0}
	};

	std::string clientMessage;

	while(true) {

		int events = zmq::poll(items.data(), items.size(), 100);

		if (events > 0) {	

			if (items[0].revents & ZMQ_POLLIN) {
				
				clientMessage = s_recv(*suber);
				std::cerr << "Сообщение получено:\n" << clientMessage << std::endl;
			}			
		}

		sleep(0UL);
	}
}
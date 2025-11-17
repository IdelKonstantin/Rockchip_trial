#include <iostream>
#include <memory>
#include <string>
#include <thread>

////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ini_parser.h"
#include "zhelpers.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

void sendToDaemon();
void getFromDaemon();

std::shared_ptr<zmq::socket_t> pusher{nullptr};
std::shared_ptr<zmq::socket_t> suber{nullptr};

////////////////////////////////////////////////////////////////////////////////////////////////////

int main () {

	INIParser iniParser;    
	iniParser.load("conf.ini");

	zmq::context_t context(1);

	/* Сокет для исходящих сообщений для демона по расчету баллистики с заданием на расчет */

	auto pushAddr = std::string("tcp://") + iniParser.getString("Zeromq_pull", "host", "localhost") 
	+ std::string(":") + iniParser.getString("Zeromq_pull", "port", "5436");

	pusher = std::make_shared<zmq::socket_t>(context, ZMQ_PUSH);
	pusher->connect(pushAddr);

	/* Сокет для входящих сообщений для демона по расчету баллистики */

	auto subAddr = std::string("tcp://") + iniParser.getString("Zeromq_pub", "host", "localhost") 
	+ std::string(":") + iniParser.getString("Zeromq_pub", "port", "5435");

	suber = std::make_shared<zmq::socket_t>(context, ZMQ_SUB);
	suber->setsockopt(ZMQ_SUBSCRIBE, "", 0);
	suber->connect(subAddr);

	std::thread t1(sendToDaemon);
	std::thread t2(getFromDaemon);

	t1.join();
	t2.join();

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void sendToDaemon() { /* Отправка задания */

	std::cout << "Нажмите ENTER для отправки сообщения (или 'q' + ENTER для выхода)...";
	std::string input;
	std::string dataToSend;
	bool status = false;

	while (true) {

		std::getline(std::cin, input);

		if (input == "q") {
			break;
		}

		status = !status;

		std::cout << "Статус GPS: " << std::boolalpha << status << std::endl;

		dataToSend = (status) ? std::string("ON") : std::string("OFF");

		s_send(*pusher, dataToSend, ZMQ_DONTWAIT);
		std::cout << "Сообщение отправлено! Статус: [" << dataToSend << "]" << std::endl;
	}	
}

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
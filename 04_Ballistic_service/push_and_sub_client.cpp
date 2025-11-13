#include <iostream>
#include <memory>
#include <string>
#include <thread>

////////////////////////////////////////////////////////////////////////////////////////////////////

#include "ini_parser.h"
#include "zhelpers.h"
#include "random_UUID_generator.h"

////////////////////////////////////////////////////////////////////////////////////////////////////

std::string inputJsonCase = R"boot({
	"Bullet": {
		"DF": "G7",
		"BC": 0.448,
		"CDM": {},
		"MBC": {},
		"V0": 830,
		"lenght": 47.65,
		"weight": 300,
		"diam.": 8.585,
		"CCF_0.9": 1,
		"CCF_1.0": 1,
		"CCF_1.1": 1,
		"V0temp": 15,
		"therm": 0
	},
	"Rifle": {
		"zero": 100,
		"scope_height": 4.9,
		"twist": 228,
		"twist.dir": "R",
		"zero.atm": "not_here",
		"zero.temp": 22,
		"zero.press": 995,
		"POI_vert": 0,
		"POI_horiz": 0,
		"roll": 0
	},
	"Scope": {
		"units": "MRAD",
		"vert.click": 0.1,
		"horiz.click": 0.1
	},
	"Inputs": {
		"dist.": 200,
		"terrain_angle": 0,
		"target_azimuth": 0,
		"latitude": 0,
		"targ.speed": 0
	},
	"Options": {
		"koriolis": true,
		"rangecard": false,
		"therm.corr": false,
		"aerojump": true
	},
	"Meteo": {
		"temp.": 15,
		"press.": 1013,
		"humid.": 0,
		"wind": "simple",
		"windage": [
			{
			"dist.": 200,
			"speed": 2,
			"dir.": 37,
			"incl.": 0
			}
		]
	},)boot";

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
	+ std::string(":") + iniParser.getString("Zeromq_pull", "port", "5433");

	pusher = std::make_shared<zmq::socket_t>(context, ZMQ_PUSH);
	pusher->connect(pushAddr);

	/* Сокет для входящих сообщений для демона по расчету баллистики */

	auto subAddr = std::string("tcp://") + iniParser.getString("Zeromq_pub", "host", "localhost") 
	+ std::string(":") + iniParser.getString("Zeromq_pub", "port", "5434");

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
	std::string token;
	std::string dataToSend; 

	while (true) {

		std::getline(std::cin, input);

		if (input == "q") {
			break;
		}

		token = RandomUUIDGenerator::generateToken();

		dataToSend = inputJsonCase;
		dataToSend += (std::string("\"Token\": \"") + token + std::string("\"\n}"));

		s_send(*pusher, dataToSend, ZMQ_DONTWAIT);
		std::cout << "Сообщение отправлено! Token: [" << token << "]" << std::endl;
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
				std::cout << "Сообщение получено:\n" << clientMessage << std::endl;
			}			
		}

		sleep(0UL);
	}
}
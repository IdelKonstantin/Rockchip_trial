#include "I2C_daemon.h"
#include "CFastLog.h"

#include "meteo_sensor_fabrique.h"
#include "light_sensor_fabrique.h"
#include "proxy_sensor_fabrique.h"
#include "imu_sensor_fabrique.h"

#include <iostream>

i2cDaemon::i2cDaemon(const std::string inifilePath, const std::string serviceName) : 
baseDaemon(inifilePath, serviceName) {}

bool i2cDaemon::initZMQworkers() {

	try {

		auto sendAddr = std::string("tcp://*:") + m_iniParser->getString("Zeromq_pub", "port", "5437");

		m_zmqPUBer = std::make_shared<zmq::socket_t>(m_context, ZMQ_PUB);
		m_zmqPUBer->bind(sendAddr);

		LOG_INFO(fastlog::LogEventType::System) << "Создан zmq-сокет (публикатор) по адресу: " << sendAddr;
	}
	catch(const zmq::error_t& ex) {

		LOG_CRIT(fastlog::LogEventType::System) << "Ошибка в инициации сокетов zmq: [" << ex.what() << "]";
		return false;
	}	

	return true;
}

void i2cDaemon::stopZMQ() {

	if(m_zmqPUBer) {

		m_zmqPUBer->close();
		m_zmqPUBer.reset();
	}

	m_context.close();
}

bool i2cDaemon::init() {

	if(!initBaseDaemon()) {

		std::cerr << "Ошибка в инициализации базового демона!" << std::endl;
		return false;
	}

	try {
		
		std::string i2cDeviceName = m_iniParser->getString("Meteo", "name", "BME280");
		std::string i2cDeviceFD = m_iniParser->getString("Meteo", "dev", "/dev/i2c-1");
		std::string i2cDeviceAddr = m_iniParser->getString("Meteo", "addr", "0x76");

		m_meteo = meteoSensorFabrique{}.produceSensor(meteo::sensorTypeByName.at(i2cDeviceName), i2cDeviceFD, INIParser::hexStringToIntSstream(i2cDeviceAddr));

		if(!m_meteo) {

			LOG_WARN(fastlog::LogEventType::System) << "Не создан обработчик метеодатчика [" << i2cDeviceName 
			<< ", " << i2cDeviceFD << ", " << i2cDeviceAddr << "]";
		}

		LOG_INFO(fastlog::LogEventType::System) << "Создан обработчик метеодатчика [" << m_meteo->whoAmI() << "]";

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		i2cDeviceName = m_iniParser->getString("Lux", "name", "APDS9930");
		i2cDeviceFD = m_iniParser->getString("Lux", "dev", "/dev/i2c-1");
		i2cDeviceAddr = m_iniParser->getString("Lux", "addr", "0x39");

		m_lux = lightSensorFabrique{}.produceSensor(light::sensorTypeByName.at(i2cDeviceName), i2cDeviceFD, INIParser::hexStringToIntSstream(i2cDeviceAddr));

		if(!m_lux) {

			LOG_WARN(fastlog::LogEventType::System) << "Не создан обработчик люксометра [" << i2cDeviceName 
			<< ", " << i2cDeviceFD << ", " << i2cDeviceAddr << "]";
		}

		LOG_INFO(fastlog::LogEventType::System) << "Создан обработчик люксометра [" << m_lux->whoAmI() << "]";

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		i2cDeviceName = m_iniParser->getString("Proxy", "name", "MAX44009");
		i2cDeviceFD = m_iniParser->getString("Proxy", "dev", "/dev/i2c-1");
		i2cDeviceAddr = m_iniParser->getString("Proxy", "addr", "0x4A");

		m_proxy = proxySensorFabrique{}.produceSensor(proxy::sensorTypeByName.at(i2cDeviceName), i2cDeviceFD, INIParser::hexStringToIntSstream(i2cDeviceAddr));

		if(!m_proxy) {

			LOG_WARN(fastlog::LogEventType::System) << "Не создан обработчик датчика приближения [" << i2cDeviceName 
			<< ", " << i2cDeviceFD << ", " << i2cDeviceAddr << "]";
		}

		LOG_INFO(fastlog::LogEventType::System) << "Создан обработчик датчика приближения [" << m_proxy->whoAmI() << "]";

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		i2cDeviceName = m_iniParser->getString("IMU", "name", "MPU6050");
		i2cDeviceFD = m_iniParser->getString("IMU", "dev", "/dev/i2c-1");
		i2cDeviceAddr = m_iniParser->getString("IMU", "addr", "0x68");

		m_imu = imuSensorFabrique{}.produceSensor(IMU::sensorTypeByName.at(i2cDeviceName), i2cDeviceFD, INIParser::hexStringToIntSstream(i2cDeviceAddr));

		if(!m_imu) {

			LOG_WARN(fastlog::LogEventType::System) << "Не создан обработчик MEMS [" << i2cDeviceName 
			<< ", " << i2cDeviceFD << ", " << i2cDeviceAddr << "]";
		}

		LOG_INFO(fastlog::LogEventType::System) << "Создан обработчик MEMS [" << m_imu->whoAmI() << "]";

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


		if(!m_imu->init()) {

			LOG_WARN(fastlog::LogEventType::System) << "Не инициирован MEMS: " << m_imu->whoAmI();	
		}
		else {

			LOG_INFO(fastlog::LogEventType::System) << "MEMS: " << m_imu->whoAmI() << " успешно инициирован";
		}
		

		if(!m_lux->init()) {

			LOG_WARN(fastlog::LogEventType::System) << "Не инициирован люксометр: " << m_lux->whoAmI();	
		}
		else {

			LOG_INFO(fastlog::LogEventType::System) << "Люксометр: " << m_lux->whoAmI() << " успешно инициирован";
		}

		if(!m_proxy->init()) {

			LOG_WARN(fastlog::LogEventType::System) << "Не инициирован датчик приближения: " << m_proxy->whoAmI();	
		}
		else {

			LOG_INFO(fastlog::LogEventType::System) << "Датчик приближения: " << m_proxy->whoAmI() << " успешно инициирован";
		}

		if(!m_meteo->init()) {

			LOG_WARN(fastlog::LogEventType::System) << "Не инициирован метеодатчик: " << m_meteo->whoAmI();	
		}
		else {

			LOG_INFO(fastlog::LogEventType::System) << "Метеодатчик: " << m_meteo->whoAmI() << " успешно инициирован";
		}

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}
	catch(const std::exception& ex) {

		LOG_CRIT(fastlog::LogEventType::System) << "Ошибка в запуске демона шины i2c1. Причина: " << ex.what();
		return false;
	}

	if(!initZMQworkers()) {

		LOG_CRIT(fastlog::LogEventType::System) << "Не инициирован сокет ZMQ! Аварийное завершение";
		return false;
	}

	LOG_INFO(fastlog::LogEventType::System) << "Демон для работы с датчиками на шине I2C успешно инициирован";
	return true;
}

void i2cDaemon::run() {

	LOG_INFO(fastlog::LogEventType::System) << "Демон для работы с датчиками на шине I2C успешно стартовал";

	while(!canExit()) {

		auto meteo = m_meteo->getMeteoConditions();
		auto light = m_lux->getLightData();
		auto proxy = m_proxy->getProximityStatus();
		auto mems = m_imu->getIMUData();

		sendDataToSubscribers(serializeResult(meteo, light, proxy, mems));
		sleep(0UL);
	}

	stopZMQ();
}

void i2cDaemon::sendDataToSubscribers(const std::string& serializedData) {

	s_send(*m_zmqPUBer, serializedData, ZMQ_DONTWAIT);
}


std::string i2cDaemon::serializeResult(const meteo::data& meteo, const light::data& light, 
	const proxy::data& proxy, const IMU::data& IMU) {

	m_json.clear();

	m_json["meteo"]["temp."] = meteo.temperature;
	m_json["meteo"]["press."] = meteo.pressure;
	m_json["meteo"]["humid."] = meteo.humidity;
	m_json["meteo"]["wind"] = meteo.windSpeed;
	m_json["meteo"]["wind_dir."] = meteo.windDirection;

	m_json["light"]["intens."] = light.lightIntencity;
	m_json["light"]["level"] = light.lightLevel;

	m_json["proxy"]["engaged"] = proxy.proximity;

	m_json["IMU"]["acls."]["x"] = IMU.acceleration.x;
	m_json["IMU"]["acls."]["y"] = IMU.acceleration.y;
	m_json["IMU"]["acls."]["z"] = IMU.acceleration.z;

	m_json["IMU"]["gyros"]["x"] = IMU.gyroscope.x;
	m_json["IMU"]["gyros"]["y"] = IMU.gyroscope.y;
	m_json["IMU"]["gyros"]["z"] = IMU.gyroscope.z;

	m_json["IMU"]["angles"]["roll"] = IMU.angle.roll;
	m_json["IMU"]["angles"]["pitch"] = IMU.angle.pitch;
	m_json["IMU"]["angles"]["yaw"] = IMU.angle.yaw;

	return m_json.dump(4);
}
#ifndef ENGINE_MESSAGE_H
#define ENGINE_MESSAGE_H

#include "VisItExports.h"

#include <sstream>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
namespace in_situ{

class V_VISITXPORT EngineMessage {
public:
	enum Type {
		invalid
		//from Engine
		,shmInit 
		,addObject
		,addPorts //info about all the expected data
		,addCommands //info about the commands wen can use to controll the simulation
		//from module
		,ready //is the module executing and ready to receive data
		,executeCommand //let the simulation executa a command
		,goOn
		,constGrids //tell the Engine if the grids change over time
		,nThTimestep //tell the Engine how often output should be generated 
	};
	static const std::string EoM; 
	EngineMessage(Type t);
	
	Type type();
	template <typename T>
	EngineMessage& operator<<(const T& t) {
		m_msgIn << t;
		return *this;
	}
	EngineMessage& operator<<(const std::string& t) {
		*this << t.size();
		m_msgIn << t;
		return *this;
	}
	template <typename T>
	EngineMessage& operator<<(const std::vector<T>& t) {
		m_msgIn << t.size();
		for (auto e : t) {
			m_msgIn << e;
		}
		return *this;
	}

	template <typename T>
	EngineMessage& operator>>(T& t) {
		*m_msgOut >> t;
		return *this;
	}
	EngineMessage& operator>>(std::string& t) {
		int size;
		*this >> size;
		t.resize(size);
		m_msgOut->read(t.data(), size);
		return *this;
	}

	template <typename T>
	EngineMessage& operator>>(std::vector<T>& t) {
		int size;
		*this >> size;
		t.resize(size);
		for (size_t i = 0; i < size; i++) {
			*this >> t[i];
		}
		return *this;
	}



	bool sendMessage(std::shared_ptr<boost::asio::ip::tcp::socket> s);
	static EngineMessage&& read(std::shared_ptr<boost::asio::ip::tcp::socket> s);
	static void read(std::shared_ptr<boost::asio::ip::tcp::socket> s, std::function<void(EngineMessage&&)> handler);

private:
	Type m_type;
	std::stringstream m_msgIn;
	std::unique_ptr<std::istream> m_msgOut;
	EngineMessage(boost::asio::streambuf& streambuf);
};

}

#endif // !ENGINE_MESSAGE_H

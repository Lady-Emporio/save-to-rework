


#include <winsock2.h>
#include <Ws2tcpip.h>
#include <Winsock2.h> 
#include <ws2tcpip.h> 
#include <wchar.h>
#include <string>
#include <iostream>
#include <string>
#include <thread>
#include "someFunc.h"
#include <mutex>
#include <sstream> 


#include "json.hpp"
using json = nlohmann::json;
inline std::mutex manager_mutex;


void run_daemon_timeout() {
	std::thread thr(timeout_deamon);
	thr.detach();
}

CHECK_SOCKET checkRandomSocket_isOK_notBlockMutex(int index, ManagerOnline * manager, int * fd) {
	CHECK_SOCKET result = CHECK_SOCKET::OK;
	ManagerLiveSocketPtr m = (*manager)[index];
	if (nullptr == m) {
		result = CHECK_SOCKET::OUT_RANGE;
	}
	else if (m->isNeedDel()) {
		result = CHECK_SOCKET::FALL;
		*fd = m->s;
	}
	return result;
}

void timeout_deamon() {
	ManagerOnline * manager = ManagerOnline::getManager();
	int indexCheck = -1;

	for (;;)
	{
		++indexCheck;
		int sleepSec = 60;
		{
			//######################## CLOSE mutex
			std::lock_guard<std::mutex> lock(manager_mutex);
			int fd;
			CHECK_SOCKET status = checkRandomSocket_isOK_notBlockMutex(indexCheck, manager, &fd);
			switch (status) {
			case CHECK_SOCKET::OK:
				log("_____ timeout_deamon() . Check: '" + std::to_string(indexCheck) + "' is good.");
				sleepSec = 20;
				break;
			case CHECK_SOCKET::FALL:
				manager->Remove(fd);
				log("_____ timeout_deamon() . remove: '" + std::to_string(indexCheck) + "'.");
				break;
			case CHECK_SOCKET::OUT_RANGE:
				indexCheck = -1;
				sleepSec = 10;
				log("_____ timeout_deamon() . Check: '" + std::to_string(indexCheck) + "' is end.");
				break;
			};
			//######################## OPEN mutex
		}
		std::this_thread::sleep_for(std::chrono::seconds(sleepSec));
	}
}

std::string rjust(std::string text, size_t width, char fillchar) {
	if (text.size() >= width) {
		return text;
	}
	std::string returnString(width - text.size(), fillchar);
	returnString += text;
	return returnString;
}

void log(std::string text)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	std::stringstream buf;
	buf << st.wYear << "." <<
		rjust(std::to_string(st.wMonth), 2) << "." <<
		rjust(std::to_string(st.wDay), 2) << " " <<
		rjust(std::to_string(st.wHour), 2) << ":" <<
		rjust(std::to_string(st.wMinute), 2) << ":" <<
		rjust(std::to_string(st.wSecond), 2) << "_" <<
		rjust(std::to_string(st.wMilliseconds), 3);
	auto now = buf.str();

	std::cout << now << " | " << text.c_str() << std::endl;
}

int createSocket()
{
	SOCKET _s;
	_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_s == INVALID_SOCKET)
	{
		log("ERROR createSocket. Error create socket.");
	}
	else {
		log("_____ createSocket. create socket.");
	}
	return _s;
}

void bind(int s)
{
	int port = 5555;
	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_port = htons(port);

	inet_pton(AF_INET, "127.0.0.1", &(service.sin_addr));

	int iResult = bind(s, (SOCKADDR *)&service, sizeof(service));
	if (iResult == SOCKET_ERROR) {
		log("ERROR bindS. Error with bind server.");
	}
	else {
		log("_____ bindS. Good bind on: '" + std::to_string(port) + "'.");
		return;
	}
	log("______ bind. bind create socket.");
}

void init()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
		log("ERROR WSAStartup failed.");
	}
}

void listen(int s)
{
	if (listen(s, 60) == SOCKET_ERROR) {
		log("ERROR listen error");
	}
}

void run_daemon_server()
{
	std::thread thr(server_forever);
	thr.detach();
}

void server_forever()
{
	init();
	int server = createSocket();
	bind(server);
	listen(server);

	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 500000;

	std::vector<int>using_sockets;
	ManagerOnline * manager = ManagerOnline::getManager();
	while (true) {
		fd_set readfds;
		{
			std::lock_guard<std::mutex> lock(manager_mutex);
			manager->fillFDset(&readfds);

		}
		FD_SET(server, &readfds);

		int result = select(
			NULL,
			&readfds,
			NULL,
			NULL,
			&tv
		);
		if (0 == result) {
			log("____ timeout.");
			continue;
		}
		else if (SOCKET_ERROR == result) {
			log("ERROR select error." + std::to_string(WSAGetLastError()));
			continue;
		}
		// NEW CONNECTION ######################################
		if (FD_ISSET(server, &readfds)) {
			struct sockaddr_storage their_addr;
			socklen_t addr_size;
			addr_size = sizeof their_addr;
			int new_fd = accept(server, (struct sockaddr *)&their_addr, &addr_size);
			log("____ server_forever(). new connection on:" + std::to_string(new_fd));
			{
				std::lock_guard<std::mutex> lock(manager_mutex);
				ManagerLiveSocket t(new_fd, "unknow guest: " + std::to_string(new_fd));
				manager->Add(t);
			}
		}
		//COME MESSAGE ######################################
		std::vector<int> needDelete;
		{//++++++++++++++++++++++ close mutex
			std::lock_guard<std::mutex> lock(manager_mutex);
			for (auto it = manager->socketsFD.begin(); it != manager->socketsFD.end(); ++it) {
				int i = (*it).second->s;

				if (FD_ISSET(i, &readfds)) {
					std::string message;
					int res = getMessage(i, &message);
					if (res != 1) {
						needDelete.push_back(i);
					}

					if (res == -1 && message == "") {
						log("server_forever(). Message about close socket on: " + std::to_string(i) + ". Name: '" + it->second->name + "'.");
					}
					else {
						work_with_message(message, &(it->second));
					}
				}
			}
			for (int i : needDelete) {
				manager->Remove(i);
			}
		}//-------------------------------- open mutex
		needDelete.clear();
	}
}

int getMessage(int socket, std::string *message)
{
	if (!isCanRead(socket)) {
		log("ERROR getMessage. call isCanRead when not messages.");
		return 0;
	}
	*message = "";
	while (isCanRead(socket)) {
		char buf[400 * 8 * 2];
		int byte_count;
		byte_count = recv(socket, buf, sizeof(buf), 0);
		if (0 == byte_count) {
			log("getMessage(). get: recv close connection.");
			closesocket(socket);
			return -1;
		}
		else if (SOCKET_ERROR == byte_count) {
			log("getMessage(). get: recv error." + std::to_string(WSAGetLastError()));
			return 0;
		}

		buf[byte_count] = '\0';
		*message += buf;
	}
	std::string log_message = "getMessage().  get:";
	log_message += *message;
	log(log_message);
	return 1;

}

bool isCanRead(int s) {
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(s, &readfds);
	int result = select(
		NULL,
		&readfds,
		NULL,
		NULL,
		&tv
	);
	if (0 == result) {
		return false;
	}
	return true;
}

bool sendall(int s, std::string text)
{
	int len;
	char *buf = text.data();
	len = strlen(buf);

	int total = 0;
	int bytesleft = len;
	int n;
	while (total < len) {
		n = send(s, buf + total, bytesleft, 0);

		if (SOCKET_ERROR == n) {
			int result_error = WSAGetLastError();
			switch (result_error) {
			case WSAENOTCONN:
				log("ERROR sendAll. Socket is not connected." + std::to_string(result_error));
				break;
			case WSAECONNABORTED:
				log("ERROR sendAll. Software caused connection abort." + std::to_string(result_error));
				break;
			case WSAENOTSOCK:
				log("ERROR sendAll. Socket operation on nonsocket.." + std::to_string(result_error));
				break;
			default:
				log("ERROR sendAll. send error" + std::to_string(result_error));
			}
			return false;
		}

		total += n;
		bytesleft -= n;
	}
	return true;
}

std::map<int, ManagerLiveSocket> getManagerSocket()
{
	static std::map<int, ManagerLiveSocket> m;
	return m;
}

void work_with_message(std::string message, ManagerLiveSocketPtr* now_manager)
{
	//if error!=""  - server return error.
	(*now_manager)->updateTime();
	int fd = (*now_manager)->s;
	std::string nameSocket = (*now_manager)->name;
	log("____ work_with_message(). Message: '" + message + "'. Fd: '" + std::to_string(fd) + "'. Name:'" + nameSocket + "'.");
	std::string e = "";


	json json_object = json::parse(message, nullptr, false);
	if (json_object.is_discarded()) {
		e = "Can not parse json.";
		answerError(fd, e);
		return;
	}

	std::string nameJson = json_object.value("name", "-1");
	if ("-1" == nameJson) {
		e = "Can not get name.";
		answerError(fd, e);
		return;
	}
	std::string valueJson = json_object.value("value", "-1");
	if ("-1" == valueJson) {
		e = "Can not get value.";
		answerError(fd, e);
		return;
	}
	if ("auth" == nameJson) {
		ManagerOnline* t_m = ManagerOnline::getManager();
		t_m->auth(now_manager, valueJson);
		return;
	}
	if (!(*now_manager)->isAuth) {
		e = "Not auth";
		answerError(fd, e);
		return;
	}
	//##########################################
	if ("ping" == nameJson) {
		answer(fd,"pong");
		return;
	}
	if ("message" == nameJson) {
		ManagerOnline* t_m = ManagerOnline::getManager();
		ManagerLiveSocketPtr sendTo = (*t_m)[valueJson];
		if (nullptr != sendTo) {
			answer(fd, "Error send. Not found: '"+ valueJson +"'.");
			return;
		}
		if (sendall(sendTo->s, "NewMessage")) {
			answer(fd, "Error send. Not can send: '" + valueJson + "'.");
			return;
		}
		else {
			answer(fd, "Good send: '" + valueJson + "'.");
		}
		return;
	}
	//##########################################

}

void answerError(int fd, std::string textError)
{
	log("server_forever(). answer on Error: '" + textError + "'.");
	sendall(fd, textError);
}

void answer(int fd, std::string value) {
	log("server_forever(). answer on message: '" + value + "'.");
	sendall(fd, value);
}
#pragma once
#include <string>
#include <map>
#include "Managers.h"

enum CHECK_SOCKET{
	OUT_RANGE,
	OK,
	FALL,
};
std::string rjust(std::string text, size_t width, char fillchar = '0');
void log(std::string text);
int createSocket();
void bind(int s);
void listen(int s);
void init();
void run_daemon_server();
void server_forever();
int getMessage(int socket, std::string *message);
bool isCanRead(int s);
bool sendall(int s, std::string text);
void timeout_deamon();
void run_daemon_timeout();

void answerError(int fd, std::string textError);
void answer(int fd, std::string value);
void work_with_message(std::string message,ManagerLiveSocketPtr* now_manager);
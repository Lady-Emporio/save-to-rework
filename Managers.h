#pragma once
#include <windows.h>
#include <chrono>
#include <memory>
#include <string>
#include <algorithm>
#include <vector>
#include <map>

const int sec30=30; 

class ManagerOnline;
class ManagerLiveSocket;
using ManagerLiveSocketPtr = std::shared_ptr<ManagerLiveSocket>;

class ManagerLiveSocket
{
public:
	int s;
	std::string name;
	bool isAuth;
	std::chrono::time_point<std::chrono::system_clock> create;

	ManagerLiveSocket(int s, std::string name);
	~ManagerLiveSocket();
	void updateTime();
	bool isNeedDel();
};

class ManagerOnline
{
	ManagerOnline() {}
	std::map<std::string, ManagerLiveSocketPtr> socketsName;
public:
	static ManagerOnline * getManager();
	ManagerLiveSocketPtr* getFD(int fd);
	
	void Add(const ManagerLiveSocket& ms);
	void Remove(std::string name);
	void Remove(int socketfd);
	int size();
	ManagerLiveSocketPtr operator[](int index);
	ManagerLiveSocketPtr operator[](std::string name);
	
	void fillFDset(fd_set *x);
	std::map<int, ManagerLiveSocketPtr> socketsFD;
	void auth(ManagerLiveSocketPtr* now_manager,std::string name);
	std::vector<ManagerLiveSocketPtr> getVector();

	std::map<std::string, ManagerLiveSocketPtr> needAuth;
};


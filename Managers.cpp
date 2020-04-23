#include "Managers.h"
#include "someFunc.h"

ManagerLiveSocket::ManagerLiveSocket(int s, std::string name) :s(s), name(name)
{
	updateTime();
	isAuth = false;
}


ManagerLiveSocket::~ManagerLiveSocket()
{}

void ManagerLiveSocket::updateTime()
{
	create = std::chrono::system_clock::now();
}

bool ManagerLiveSocket::isNeedDel()
{
	std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
	int elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(now - create).count();

	if (elapsed_seconds > sec30) {
		return true;
	}
	return false;
}

ManagerOnline * ManagerOnline::getManager() {
	static ManagerOnline * w = new ManagerOnline();
	return w;
}

ManagerLiveSocketPtr* ManagerOnline::getFD(int socketfd)
{
	auto tmp = socketsFD.find(socketfd);
	return &(*tmp).second;
}

void ManagerOnline::Add(const ManagerLiveSocket& ms) {
	ManagerLiveSocketPtr d = std::make_shared<ManagerLiveSocket>(ms);
	socketsName.insert({ d->name, d });
	socketsFD.insert({ d->s, d });
}

void ManagerOnline::Remove(std::string name) {
	auto tmp = socketsName.find( name );
	if (tmp == socketsName.end()) {
		return;
	}
	int socketfd = tmp->second->s;
	//ManagerLiveSocketPtr t = (*tmp).second;
	socketsFD.erase(tmp->second->s);
	socketsName.erase(tmp);

	closesocket(socketfd);
}
void ManagerOnline::Remove(int socketfd) {
	auto tmp = socketsFD.find( socketfd );
	if (tmp == socketsFD.end()) {
		return;
	}
	//ManagerLiveSocketPtr t = (*tmp).second;
	socketsName.erase(tmp->second->name);
	socketsFD.erase(tmp);

	closesocket(socketfd);
}
int ManagerOnline::size()
{
	return socketsFD.size();
}
ManagerLiveSocketPtr ManagerOnline::operator[](int index){
	if (index >= socketsFD.size()){
		return nullptr;
	}
	auto nowCheckSocket = socketsFD.begin();
	for (int i = 0; i != index; ++i) {
		++nowCheckSocket;
	}
	
	ManagerLiveSocketPtr m = (*nowCheckSocket).second;
	return m;
}
ManagerLiveSocketPtr ManagerOnline::operator[](std::string name){
	auto tmp = socketsName.find( name ); 
	ManagerLiveSocketPtr res = nullptr;

	if ( tmp != socketsName.end() ) {
		res = tmp->second;
	} 

	return res;
}

void ManagerOnline::fillFDset(fd_set * fds)
{
	FD_ZERO(fds);
	for (auto it = socketsName.begin(); it != socketsName.end(); ++it) {
		
		FD_SET((*it).second->s, fds);
	};
}

void ManagerOnline::auth(ManagerLiveSocketPtr* now_manager, std::string name)
{
	int fd = (*now_manager)->s;
	ManagerLiveSocketPtr isExistAlready = (*this)[name];
	if (nullptr != isExistAlready) {
		answer(fd, "AlreadyExistAuth.");
		return;
	}
	this->Remove(fd);
	
	ManagerLiveSocket t(fd, name);
	t.isAuth = true;
	this->Add(t);

	answer(fd, "Good auth: '" + name + "'.");
}

std::vector<ManagerLiveSocketPtr> ManagerOnline::getVector()
{
	std::vector<ManagerLiveSocketPtr> toReturn;
	for (auto it = socketsName.begin(); it != socketsName.end(); ++it) {
		toReturn.push_back((*it).second);
	};
	return toReturn;
}

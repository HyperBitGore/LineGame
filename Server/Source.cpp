#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

std::vector<SOCKET> socks;
std::mutex mt1;

enum { CHANGE, WIN, LOSE, REST, DISCONNECT };

//used to be easily serilized and sent to user
struct Point {
	int x;
	int y;
	uint32_t col;
};
std::vector<std::vector<Point>> map;
void generateMap(int w, int h) {
	map.clear();
	for (int i = 0; i < h; i++) {
		std::vector<Point> level;
		for (int j = 0; j < w; j++) {
			Point p = { j, i, 0};
			level.push_back(p);
		}
		map.push_back(level);
	}
}
Point* findPoint(int x, int y) {
	return &map[y][x];
}

void listeningThread() {
	bool exitf = false;
	SOCKET listener = socket(AF_INET, SOCK_STREAM, 0);
	if (listener == INVALID_SOCKET) {
		std::cerr << "Socket creation failed " << std::endl;
		exitf = true;
	}
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(6890);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(listener, (sockaddr*)&hint, sizeof(hint));
	//tells winsock that the socket is for listening
	listen(listener, SOMAXCONN);
	while (!exitf) {
		//wait for connection
		sockaddr_in client;
		int clientsize = sizeof(client);
		
		SOCKET clientSocket = accept(listener, (sockaddr*)&client, &clientsize);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "Client connect failed" << std::endl;
			exitf = true;
		}
		char host[NI_MAXHOST]; //clients remote name
		char service[NI_MAXHOST]; //service (port) client is connection on

		ZeroMemory(host, NI_MAXHOST);
		ZeroMemory(host, NI_MAXHOST);

		if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
			std::cout << host << " connected on port " << service << std::endl;
		}
		else {
			inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
			std::cout << host << " connected on port " << ntohs(client.sin_port) << std::endl;
		}
		//put server connect send changes event here
		mt1.lock();
		socks.push_back(clientSocket);
		mt1.unlock();
	}
}
//https://stackoverflow.com/questions/69328638/error-invalid-padding-length-on-received-packet-when-connecting-to-winsock-ser


//send changes to users
//Send all current changes to user who connects
//implement win and lose state


int main() {
	bool exitf = false;
	WSADATA wsdat;
	WORD ver = MAKEWORD(2, 2);
	//start winsock
	int wsOK = WSAStartup(ver, &wsdat);
	if (wsOK != 0) {
		std::cerr << "Can't init winsock!" << std::endl;
		return -1;
	}
	generateMap(800, 800);
	std::thread list(listeningThread);
	char buf[1024];
	while(!exitf){
		mt1.lock();
		for (int i = 0; i < socks.size();) {
			bool er = false;
			ZeroMemory(buf, 1024);
			int bytes = recv(socks[i], buf, 1024, 0);
			if (bytes == SOCKET_ERROR) {
				std::cerr << "Error from recv()" << std::endl;
				exitf = true;
				break;
			}else if (bytes == 0) {
				std::cerr << "Disconnect" << std::endl;
				er = true;
			}
			else {
				Point* p;
				char* tt;
				int* pt;
				UINT32* ppo;
				int ptx, pty;
				switch (buf[0]) {
				case DISCONNECT:
					std::cout << "Disconnect" << std::endl;
					er = true;
					break;
				case CHANGE:
					tt = buf;
					tt++;
					pt = (int*)tt;
					ptx = *pt;
					pt++;
					pty = *pt;
					pt++;
					ppo = (UINT32*)pt;
					p = findPoint(ptx, pty);
					p->col = *ppo;
					char buff[13];
					buff[0] = CHANGE;
					tt = buff;
					tt++;
					pt = (int*)tt;
					*pt = ptx;
					pt++;
					*pt = pty;
					pt++;
					ppo = (UINT32*)pt;
					*ppo = p->col;
					for (auto& j : socks) {
						send(j, buff, 13, 0);
					}
					break;
				default:
					//send(socks[i], buf, bytes + 1, 0);
					//std::cout << "wrote buffer \n";
					break;
				}
			}
			if (er) {
				closesocket(socks[i]);
				socks.erase(socks.begin() + i);
			}
			else {
				i++;
			}
		}
		mt1.unlock();

	}
	//cleanup sockets
	list.join();
	for (auto& i : socks) {
		closesocket(i);
	}
	socks.clear();
	WSACleanup();
	return 0;
}
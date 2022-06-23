#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

//std::vector<SOCKET> socks;
fd_set master;
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

//https://stackoverflow.com/questions/69328638/error-invalid-padding-length-on-received-packet-when-connecting-to-winsock-ser


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
	FD_ZERO(&master);
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
	FD_SET(listener, &master);


	char buf[1024];
	TIMEVAL timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	while(!exitf){
		mt1.lock();
		fd_set copy = master;
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);
		for (int i = 0; i < socketCount; i++) {
			ZeroMemory(buf, 1024);
			SOCKET sock = copy.fd_array[i];
			if (sock == listener) {
				sockaddr_in client;
				int clientsize = sizeof(client);

				SOCKET clientSocket = accept(sock, (sockaddr*)&client, &clientsize);
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
				//socks.push_back(clientSocket);
				send(clientSocket, "Welcome to server\n", 19, 0);
				FD_SET(clientSocket, &master);
			}
			else {
				int bytesIn = recv(sock, buf, 1024, 0);
				if (bytesIn <= 0) {
					std::cerr << "Disconnect" << std::endl;
					closesocket(sock);
					FD_CLR(sock, &master);
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
						closesocket(sock);
						FD_CLR(sock, &master);
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
						for (int j = 0; j < master.fd_count; j++) {
							send(master.fd_array[j], buff, 13, 0);
						}
						std::cout << "Sent change \n";
						break;
					default:
						/*for (int j = 0; j < master.fd_count; j++) {
							if (master.fd_array[j] != sock) {
								send(master.fd_array[j], buf, bytesIn, 0);
							}
						}
						std::cout << "wrote buffer \n";*/
						break;
					}
				}
			}
		}
		mt1.unlock();

	}
	//cleanup sockets
	fd_set cpyo = master;
	int socCount = select(0, &cpyo, nullptr, nullptr, nullptr);
	for (int j = 0; j < socCount; j++) {
		closesocket(cpyo.fd_array[j]);
	}
	//socks.clear();
	WSACleanup();
	return 0;
}
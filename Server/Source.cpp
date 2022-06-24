#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")

fd_set master;

enum { CHANGE = 25, WIN, LOSE, REST, DISCONNECT, DEATH };

//used to be easily serilized and sent to user
struct Point {
	int x;
	int y;
	UINT32 col;
};

struct Player {
	SOCKET sock;
	UINT32 col;
	std::string name;
	bool dead;
};
struct RGBCol {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

std::vector<std::vector<Point>> map;
std::vector<Player> players;
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
	if (x >= 0 && y >= 0) {
		return &map[y][x];
	}
	return nullptr;
}

void sendChanges(SOCKET soc) {
	char buf[4096];
	size_t cb = 4095;
	/*for (int i = 0; i < changes.size();) {
		ZeroMemory(buf, 4096);
		buf[0] = CHANGE;
		char* tp = buf;
		tp++;
		int* tt = (int*)tp;
		while (cb >= 12) {
			*tt = changes[i]->x;
			tt++;
			*tt = changes[i]->y;
			tt++;
			UINT32* ttp = (UINT32*)tt;
			*ttp = changes[i]->col;
			ttp++;
			tt = (int*)ttp;
			i++;
			cb -= 12;
			tp += 12;
		}
		send(soc, buf, 4096, 0);
	}*/
	ZeroMemory(buf, 4096);
	buf[0] = REST;
	send(soc, buf, 4096, 0);
}

RGBCol getRandomColor() {
	return { uint8_t(rand() % 255 + 10), uint8_t(rand() % 255 + 50), uint8_t(rand() % 255 + 50) };
}


//https://stackoverflow.com/questions/69328638/error-invalid-padding-length-on-received-packet-when-connecting-to-winsock-ser


//implement win and lose state


int main() {
	bool exitf = false;
	WSADATA wsdat;
	WORD ver = MAKEWORD(2, 2);
	srand(time(NULL));
	//start winsock
	int wsOK = WSAStartup(ver, &wsdat);
	if (wsOK != 0) {
		std::cerr << "Can't init winsock!" << std::endl;
		return -1;
	}
	players.reserve(100);
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
	bool playing = true;

	char buf[16];
	TIMEVAL timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	while(!exitf){
		fd_set copy = master;
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);
		for (int i = 0; i < socketCount; i++) {
			ZeroMemory(buf, 16);
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
				//sendChanges(clientSocket);
				//put server connect send state of game here, so if game going on or not
				Player p;
				p.sock = clientSocket;
				buf[0] = playing;
				char* tp = buf;
				tp++;
				RGBCol col = getRandomColor();
				uint8_t* tto = (uint8_t*)tp;
				*tto = col.r;
				tto++;
				*tto = col.g;
				tto++;
				*tto = col.b;
				//need random spawn position to be sent here
				send(clientSocket, buf, 16, 0);
				//recieve col
				ZeroMemory(buf, 16);
				recv(clientSocket, buf, 16, 0);
				UINT32* pp = (UINT32*)buf;
				p.col = *pp;
				//need getting of name here
				ZeroMemory(buf, 16);
				recv(clientSocket, buf, 16, 0);
				p.name = buf;
				p.dead = false;
				players.push_back(p);
				std::cout << p.name << " : " << p.col << std::endl;
				FD_SET(clientSocket, &master);
			}
			else {
				int bytesIn = recv(sock, buf, 16, 0);
				if (bytesIn <= 0) {
					std::cerr << "Disconnect" << std::endl;
					for (int j = 0; j < players.size(); j++) {
						if (players[j].sock == sock) {
							players.erase(players.begin() + j);
							j = players.size();
						}
					}
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
						for (int j = 0; j < players.size(); j++) {
							if (players[j].sock == sock) {
								players.erase(players.begin() + j);
								j = players.size();
							}
						}
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
						if (p != nullptr) {
							for (auto& j : players) {
								if (j.sock == sock) {
									if (j.col != p->col && p->col != 0) {
										j.dead = true;
									}
								}
							}
							p->col = *ppo;
							char buff[16];
							ZeroMemory(buff, 16);
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
								if (master.fd_array[j] != sock) {
									send(master.fd_array[j], buff, 16, 0);
								}
							}
							std::cout << "Sent change \n";
						}
						break;
					}
				}
			}
		}
		for (auto& i : players) {
			if (i.dead) {
				ZeroMemory(buf, 16);
				char* ttp = buf;
				buf[0] = DEATH;
				ttp++;
				bool* tt = (bool*)ttp;
				*tt = i.dead;
				send(i.sock, buf, 16, 0);
			}
		}
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
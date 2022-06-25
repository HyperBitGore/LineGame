#pragma once
#define NOMINMAX
#include "GoreEngine.h"
#include <WS2tcpip.h>
#include <thread>
#include <mutex>
#include <algorithm>

#pragma comment (lib, "ws2_32.lib")


enum { CHANGE = 25, WIN, LOSE, REST, DISCONNECT, DEATH};

struct Change {
	int x;
	int y;
	UINT32 col;
	bool lose;
};

struct lineMaker {
	float x;
	float y;
	float trajx;
	float trajy;
	float mvtime;
	float sx;
	float sy;
};

std::mutex mt1;
std::vector<Change> changes;
//135.148.45.140
void connectSocket(SOCKET* sock, bool* mode, UINT32* col, std::string& outname, SDL_Surface* surf, lineMaker* player) {
	std::string ip;
	std::cout << "Input server IP: \n";
	std::cin >> ip;
	std::cout << std::endl;
	std::string name;
	std::cout << "Input username: ";
	std::cin >> name;
	sockaddr_in in;
	in.sin_family = AF_INET;
	in.sin_port = htons(6890);
	inet_pton(AF_INET, ip.c_str(), &in.sin_addr);
	int rsc = connect(*sock, (sockaddr*)&in, sizeof(in));
	if (rsc == SOCKET_ERROR) {
		std::cerr << "Socket connection failed ERR: " << WSAGetLastError() << std::endl;
		return;
	}

	std::cout << std::endl;
	//recieve the state of game, so if one going on put into wait state and if not put into playing state
	char buf[16];
	ZeroMemory(buf, 16);
	recv(*sock, buf, 16, 0);
	bool* state = (bool*)buf;
	*mode = *state;
	state++;
	uint8_t* unp = (uint8_t*)state;
	SDL_Color rg = {0, 0, 0, 255};
	rg.r = *unp;
	unp++;
	rg.g = *unp;
	unp++;
	rg.b = *unp;
	unp++;
	int* toop = (int*)unp;
	player->x = *toop;
	toop++;
	player->y = *toop;
	UINT32 otc = Gore::Engine::ConvertColorToUint32RGBA(rg, surf->format);
	//sending converted color back
	ZeroMemory(buf, 16);
	UINT32* op = (UINT32*)buf;
	*op = otc;
	send(*sock, buf, 16, 0);
	*col = otc;
	//sending name
	send(*sock, name.c_str(), 16, 0);
	outname = name;
	player->sx = player->x;
	player->sy = player->y;
}


void recvThread(SOCKET soc1) {
	char buf[16];
	fd_set master;
	FD_ZERO(&master);
	FD_SET(soc1, &master);
	while (true) {
		ZeroMemory(buf, 16);
		fd_set copy = master;
		int slt = select(0, &copy, nullptr, nullptr, nullptr);
		if (slt >= 1) {
			int bytesIn = recv(copy.fd_array[0], buf, 16, 0);
			char* tf;
			int* tpf;
			bool* ttp;
			UINT32* tpu;
			int px, py;
			switch (buf[0]) {
			case CHANGE:
				tf = buf;
				tf++;
				tpf = (int*)tf;
				px = *tpf;
				tpf++;
				py = *tpf;
				tpf++;
				tpu = (UINT32*)tpf;
				//std::cout << "recv change\n";
				changes.push_back({ px, py, *tpu, false });
				//Gore::Engine::SetPixelSurface(surf, px, py, *tpu);
				break;
			case WIN:
				//display the name winning
				ZeroMemory(buf, 16);
				recv(copy.fd_array[0], buf, 16, 0);
				changes.push_back({ 0, 0, 0, true });
				std::cout << buf << " won!" << std::endl;
				break;
			case DEATH:
				tf = buf;
				tf++;
				ttp = (bool*)tf;
				changes.push_back({ 0, 0, 0, false });
				break;
			}
		}
		else if (slt == -1) {
			return;
		}
	}
}

void calcSlope(int x1, int y1, int x2, int y2, float* dx, float* dy) {
	int steps = std::max(abs(x1 - x2), abs(y1 - y2));
	if (steps == 0) {
		*dx = *dy = 0;
		return;
	}
	*dx = (x1 - x2);
	*dx /= steps;

	*dy = (y1 - y2);
	*dy /= steps;
}
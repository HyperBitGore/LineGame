#pragma once
#include "GoreEngine.h"
#include <WS2tcpip.h>
#include <thread>
#include <mutex>

#pragma comment (lib, "ws2_32.lib")


enum { CHANGE = 25, WIN, LOSE, REST, DISCONNECT };

struct Change {
	int x;
	int y;
	UINT32 col;
};

std::mutex mt1;
std::vector<Change> changes;
//135.148.45.140
void connect(SOCKET* sock) {
	std::string ip;
	std::cout << "Input server IP: \n";
	std::cin >> ip;
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
	//setup recieving of chunks of data for already written parts of screen


}


void recvThread(SOCKET soc1) {
	char buf[13];
	fd_set master;
	FD_ZERO(&master);
	FD_SET(soc1, &master);
	while (true) {
		ZeroMemory(buf, 13);
		fd_set copy = master;
		int slt = select(0, &copy, nullptr, nullptr, nullptr);
		if (slt >= 1) {
			int bytesIn = recv(copy.fd_array[0], buf, 13, 0);
			char* tf;
			int* tpf;
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
				std::cout << "recv change\n";
				changes.push_back({ px, py, *tpu });
				//Gore::Engine::SetPixelSurface(surf, px, py, *tpu);
				break;
			case REST:

				break;
			}
		}
		else if (slt == -1) {
			return;
		}
	}
}
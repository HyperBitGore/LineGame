#pragma once
#include "GoreEngine.h"
#include <WS2tcpip.h>

#pragma comment (lib, "ws2_32.lib")


enum { CHANGE, WIN, LOSE, REST, DISCONNECT };

void connect(SOCKET* soc) {
	std::string ip;
	std::cout << "Input server IP: \n";
	std::cin >> ip;
	sockaddr_in in;
	in.sin_family = AF_INET;
	in.sin_port = htons(6890);
	inet_pton(AF_INET, ip.c_str(), &in.sin_addr);
	int rsc = connect(*soc, (sockaddr*)&in, sizeof(in));
	if (rsc == SOCKET_ERROR) {
		std::cerr << "Socket connection failed ERR: " << WSAGetLastError() << std::endl;
		return;
	}


	std::cout << std::endl;
	//setup recieving of chunks of data for already written parts of screen


}

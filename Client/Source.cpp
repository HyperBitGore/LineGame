#include "Header.h"
#undef main

//all connections use TCP

//change movement to be a line that tracks towards mouse
//game win and lose state



void updatePlayer(lineMaker* player, double* delta, SOCKET sock, SDL_Surface* surf, UINT32 dc) {
	player->mvtime += *delta;
	if (player->mvtime > 0.01) {
		player->x += player->trajx;
		player->y += player->trajy;
		player->mvtime = 0;
		if (player->x >= 0 && player->y >= 0 && player->x <= 799 && player->y <= 799) {
			Gore::Engine::SetPixelSurface(surf, (int)player->y, (int)player->x, dc);
			//sending change spot to server
			char mt[16];
			ZeroMemory(mt, 16);
			mt[0] = CHANGE;
			char* mtp = mt;
			mtp++;
			char* t = mtp;
			int* tp = (int*)t;
			*tp = (int)player->x;
			tp++;
			*tp = (int)player->y;
			tp++;
			UINT32* tpt = (UINT32*)tp;
			*tpt = dc;
			//send packet over tcp socket
			send(sock, mt, 16, 0);
		}
	}
}


int main() {
	bool exitf = false;
	srand(time(NULL));
	if (SDL_Init(SDL_INIT_EVERYTHING) > 0) {
		std::cerr << SDL_GetErrorMsg << std::endl;
	}
	SDL_Window* wind = SDL_CreateWindow("Line Draw", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 800, 0);
	SDL_Renderer* rend = SDL_CreateRenderer(wind, -1, SDL_RENDERER_ACCELERATED);
	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, 800, 800, 32, SDL_PIXELFORMAT_RGBA8888);
	SDL_Event e;
	Gore::Engine::clearSurface(surf);

	SDL_Color curcolor = { 20, 50, 255, 255 };
	SDL_Color enemycol = { 255, 0, 50, 255 };
	Uint32 cur32col = Gore::Engine::ConvertColorToUint32RGBA(curcolor, surf->format);
	Uint32 enemy32col = Gore::Engine::ConvertColorToUint32RGBA(enemycol, surf->format);

	SDL_Rect screct = { 0, 0, surf->w, surf->h };
	const Uint8* keys;
	keys = SDL_GetKeyboardState(NULL);
	Gore::DeltaTimer dt;
	double qtimer = 0;
	Uint32 dc = cur32col;

	//init winsock
	WSADATA wsdat;
	WORD ver = MAKEWORD(2, 2);
	//start winsock
	int wsOK = WSAStartup(ver, &wsdat);
	if (wsOK != 0) {
		std::cerr << "Can't init winsock!" << std::endl;
		return -1;
	}
	//creates our connection with the server
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Failed to create socket ERR: " << WSAGetLastError() << std::endl;
		return -1;
	}
	bool mode = true;
	std::string name;
	lineMaker player = { 300, 300, 0, 0, 0 };
	connectSocket(&sock, &mode, &dc, name, surf, &player);
	TIMEVAL timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 150;
	std::thread recvT(recvThread, sock);
	while (!exitf) {
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				exitf = true;
				break;
			}
		}
		double delta = dt.getDelta();
		int fps = 1 / delta;
		std::string fp = "Line Game - FPS: " + std::to_string(fps);
		SDL_SetWindowTitle(wind, fp.c_str());
		qtimer += delta;
		SDL_SetRenderDrawColor(rend, 0, 0, 0, 0);
		SDL_RenderClear(rend);
		SDL_PumpEvents();
		if (qtimer > 0.1) {
			if (keys[SDL_SCANCODE_Q]) {
				if (dc == cur32col) {
					dc = enemy32col;
				}
				else {
					dc = cur32col;
				}
				qtimer = 0;
			}
		}

		int mx, my;
		SDL_GetMouseState(&mx, &my);
		if (mode) {
			calcSlope((int)player.x, (int)player.y, mx, my, &player.trajx, &player.trajy);
			player.trajx = -player.trajx;
			player.trajy = -player.trajy;
			updatePlayer(&player, &delta, sock, surf, dc);
		}
		SDL_Texture* sctex = SDL_CreateTextureFromSurface(rend, surf);
		SDL_RenderCopy(rend, sctex, NULL, &screct);
		SDL_RenderPresent(rend);
		SDL_DestroyTexture(sctex);
		for (int i = 0; i < changes.size();) {
			if (changes[i].col == 0) {
				if (changes[i].lose) {
					Gore::Engine::clearSurface(surf);
					mode = true;
					player.x = player.sx;
					player.y = player.sy;
				}
				else {
					mode = false;
				}
			}
			else {
				Gore::Engine::SetPixelSurface(surf, changes[i].y, changes[i].x, changes[i].col);
			}
			changes.erase(changes.begin() + i);
		}
	}
	//shutsdown winsock
	char mto[1];
	mto[0] = DISCONNECT;
	send(sock, mto, 1, 0);
	closesocket(sock);
	recvT.join();
	recvT.~thread();
	WSACleanup();
	return 0;
}
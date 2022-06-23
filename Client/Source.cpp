#include "Header.h"
#undef main

//all connections use TCP

//recieve changes from server(need seperate thread??)
//game win and lose state



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

	for (int i = 0; i < 100; i++) {
		for (int j = 0; j < 100; j++) {
			Gore::Engine::SetPixelSurface(surf, &i, &j, &cur32col);
		}
	}
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
	connect(&sock);
	char rec_buf[1024];
	while (!exitf) {
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
			case SDL_QUIT:
				exitf = true;
				break;
			}
		}
		double delta = dt.getDelta();
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
		if (SDL_GetMouseState(&mx, &my) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			Gore::Engine::SetPixelSurface(surf, my, mx, dc);
			//sending change spot to server
			char mt[13];
			mt[0] = CHANGE;
			char* mtp = mt;
			mtp++;
			char* t = mtp;
			int* tp = (int*)t;
			*tp = mx;
			tp++;
			*tp = my;
			tp++;
			UINT32* tpt = (UINT32*)tp;
			*tpt = dc;
			//send packet over tcp socket
			send(sock, mt, 13, 0);
			//recv(sock, mt, )
		}
		SDL_GetMouseState(&mx, &my);
		if (Gore::Engine::GetPixelSurface(surf, &my, &mx) != 0 && Gore::Engine::GetPixelSurface(surf, &my, &mx) != cur32col) {
			//Gore::Engine::clearSurface(surf);
		}
		std::cout << Gore::Engine::GetPixelSurface(surf, &mx, &my) << "\n";
		SDL_Texture* sctex = SDL_CreateTextureFromSurface(rend, surf);
		SDL_RenderCopy(rend, sctex, NULL, &screct);
		SDL_RenderPresent(rend);
		SDL_DestroyTexture(sctex);
	}
	//shutsdown winsock
	char mto[1];
	mto[0] = DISCONNECT;
	send(sock, mto, 1, 0);
	closesocket(sock);
	WSACleanup();
	return 0;
}
#include "GoreEngine.h"

void Gore::Engine::insertTex(TexListMem*& tex, SDL_Texture* current, std::string name) {
	texp t;
	t = new TexListMem;
	t->current = current;
	t->next = tex;
	t->name = name;
	tex = t;
}

SDL_Texture* Gore::Engine::findTex(texp head, std::string name) {
	texp temp = head;
	while (temp != NULL) {
		if (std::strcmp(temp->name.c_str(), name.c_str()) == 0) {
			return temp->current;
		}
		temp = temp->next;
	}
	return NULL;
}

void Gore::Engine::insertSprite(spxp& sp, SDL_Surface* surf, std::string name) {
	spxp t;
	t = new SpriteListMem;
	t->current = surf;
	t->next = sp;
	t->name = name;
	sp = t;
}
SDL_Surface* Gore::Engine::findSprite(spxp sp, std::string name) {
	spxp temp = sp;
	while (temp != NULL) {
		if (temp->name.compare(name) == 0) {
			return temp->current;
		}
		temp = temp->next;
	}
	return NULL;
}


//Pixel manipulation
void Gore::Engine::SetPixelSurface(SDL_Surface* surf, int* y, int* x, Uint32* pixel) {
	SDL_LockSurface(surf);
	Uint32* pixels = (Uint32*)surf->pixels;
	pixels[(*y * surf->w) + *x] = *pixel;
	SDL_UnlockSurface(surf);
}
void Gore::Engine::SetPixelSurface(SDL_Surface* surf, int y, int x, Uint32 pixel) {
	SDL_LockSurface(surf);
	Uint32* pixels = (Uint32*)surf->pixels;
	pixels[(y * surf->w) + x] = pixel;
	SDL_UnlockSurface(surf);
}

Uint32 Gore::Engine::GetPixelSurface(SDL_Surface* surf, int* y, int* x) {
	SDL_LockSurface(surf);
	Uint32* pixels = (Uint32*)surf->pixels;
	SDL_UnlockSurface(surf);
	return pixels[*y * surf->w + *x];
}
void Gore::Engine::SetPixelSurfaceColorRGBA(SDL_Surface* surf, int* y, int* x, SDL_Color* color) {
	SDL_LockSurface(surf);
	Uint32* pixels = (Uint32*)surf->pixels;
	Uint32* pixel = pixels + (*y * surf->w) + *x;
	*pixel = SDL_MapRGBA(surf->format, color->r, color->g, color->b, color->a);
	SDL_UnlockSurface(surf);
}
void Gore::Engine::SetPixelSurfaceColorRGB(SDL_Surface* surf, int* y, int* x, SDL_Color* color) {
	SDL_LockSurface(surf);
	Uint32* pixels = (Uint32*)surf->pixels;
	Uint32* pixel = pixels + (*y * surf->w) + *x;
	*pixel = SDL_MapRGB(surf->format, color->r, color->g, color->b);
	SDL_UnlockSurface(surf);
}

Uint32 Gore::Engine::ConvertColorToUint32RGB(SDL_Color color, SDL_PixelFormat* format) {
	return SDL_MapRGB(format, color.r, color.g, color.b);
}
Uint32 Gore::Engine::ConvertColorToUint32RGBA(SDL_Color color, SDL_PixelFormat* format) {
	return SDL_MapRGBA(format, color.r, color.g, color.b, color.a);
}
void Gore::Engine::clearSurface(SDL_Surface* surf) {
	SDL_LockSurface(surf);
	memset(surf->pixels, 0, (surf->w * surf->h) * surf->format->BytesPerPixel);
	SDL_UnlockSurface(surf);
}
void Gore::Engine::clearTexture(SDL_Texture* tex, int* pitch, int w, int h) {
	Uint32* pixels;
	SDL_LockTexture(tex, NULL, (void**)&pixels, pitch);
	memset(pixels, 0, (w * h) * (sizeof(pixels)));
	SDL_UnlockTexture(tex);
}
//Texture has to be made with SDL_TEXTUREACCESS_STREAMING flag
//Texture editing is extremely slow because we have to get data from gpu, so try to do a large amount of them at once
//Have to divide the pitch by sizeof(unsigned int) to get proper x and y 
void Gore::Engine::SetPixelTexture(SDL_Texture* tex, int* y, int* x, Uint32* pixel, int* pitch) {
	Uint32* pixels;
	SDL_LockTexture(tex, NULL, (void**)&pixels, pitch);
	pixels[*y * ((*pitch) / sizeof(unsigned int)) + *x] = *pixel;
	SDL_UnlockTexture(tex);
}
//Faster if you leave this locked, 
Uint32 Gore::Engine::GetPixelTexture(SDL_Texture* tex, int* y, int* x, int* pitch) {
	Uint32* pixels;
	SDL_LockTexture(tex, NULL, (void**)&pixels, pitch);
	return pixels[*y * ((*pitch) / sizeof(unsigned int)) + *x];
	SDL_UnlockTexture(tex);
}
//Decodes everything to a 32bit pixel format, but can convert your format to any type
SDL_Surface* Gore::Engine::loadPNG(std::string name, SDL_PixelFormatEnum format, int w, int h) {
	unsigned int wr = w;
	unsigned int hr = h;
	unsigned char* data;
	unsigned error = lodepng_decode32_file(&data, &wr, &hr, name.c_str());
	if (error) {
		std::cout << lodepng_error_text(error) << std::endl;
	}
	//Uses x86 little endians for masks
	SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(data, w, h, 32, 4 * w, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	SDL_Surface* ret = SDL_ConvertSurfaceFormat(surf, format, 0);
	if (surf == NULL) {
		std::cout << SDL_GetError() << std::endl;
	}
	SDL_FreeSurface(surf);
	free(data);
	return ret;
}
//Setpixel won't work without formats without 32bits of color data to use, you can just edit the setpixel yourself if you want to workout outside of RGBA8888
SDL_Surface* Gore::Engine::LoadBMP(const char* file, SDL_PixelFormatEnum format) {
	SDL_Surface* temp = SDL_LoadBMP(file);
	SDL_Surface* remove = SDL_ConvertSurfaceFormat(temp, format, 0);
	SDL_FreeSurface(temp);
	return remove;
}

//Loads textures into memory as a linked list. Keep width/heights even with names or you'll get an error
Gore::texp& Gore::Engine::loadTextureList(std::vector<std::string> names, std::vector<unsigned int> widths, std::vector<unsigned int> heights, SDL_PixelFormatEnum format, SDL_Renderer* rend, std::string filepath) {
	texp head = NULL;
	int j = 0;
	for (auto& i : names) {
		std::string t = i;
		if (filepath != "NULL") {
			t = filepath + i;
		}
		SDL_Surface* surf = loadPNG(t, format, widths[j], heights[j]);
		SDL_Texture* tex = SDL_CreateTextureFromSurface(rend, surf);
		insertTex(head, tex, i);
		SDL_FreeSurface(surf);
		j++;
	}
	return head;
}
Gore::spxp& Gore::Engine::loadSpriteList(std::vector<std::string> names, std::vector<unsigned int> widths, std::vector<unsigned int> heights, SDL_PixelFormatEnum format, std::string filepath) {
	spxp head = NULL;
	int j = 0;
	for (auto& i : names) {
		std::string t = i;
		if (filepath != "NULL") {
			t = filepath + i;
		}
		SDL_Surface* surf = loadPNG(t, format, widths[j], heights[j]);
		insertSprite(head, surf, i);
		j++;
	}
	return head;
}


//Text functions
//Input starting integer number for character then will loop through till it hits end of input whole time adding to out
void Gore::Engine::mapTextTextures(int start, texp& out, texp& input) {
	texp t = input;
	while (t != NULL) {
		std::string temp;
		temp.push_back(start);
		insertTex(out, t->current, temp);
		start++;
		t = t->next;
	}
}

//Width and height for individual letters
void Gore::Engine::drawText(SDL_Renderer* rend, texp& texthead, std::string text, int x, int y, int w, int h) {
	int sx = x;
	int sy = y;
	for (auto& i : text) {
		if (i == ' ') {
			sx += w + 1;
		}
		else {
			SDL_Rect rect = { sx, sy, w, h };
			std::string t;
			t.push_back(i);
			SDL_Texture* temp = findTex(texthead, t);
			if (temp != NULL) {
				SDL_RenderCopy(rend, temp, NULL, &rect);
				sx += w + 1;
			}
		}
	}
}


//Misc
double Gore::DeltaTimer::getDelta() {
	double delta = 0;
	LAST = NOW;
	NOW = SDL_GetPerformanceCounter();
	delta = (double)((NOW - LAST) * 1000 / (double)SDL_GetPerformanceFrequency());
	delta = delta * 0.001;
	return delta;
}
Gore::Point Gore::Engine::raycast2DPixel(SDL_Surface* surf, int sx, int sy, float angle, int step) {
	//get the trajectory to step for each axis
	float stepx = cosf(angle * M_PI / 180);
	float stepy = sinf(angle * M_PI / 180);
	//init needed variables
	Uint32 col = GetPixelSurface(surf, &sy, &sx);
	SDL_Color set = { 100, 250, 80, 0 };
	float cx = sx;
	float cy = sy;
	//instead of a bunch of getpixel calls we just get pixel data once
	Uint32* pixels = (Uint32*)surf->pixels;
	while (col == 0 && cx > 0 && cx < surf->w && cy > 0 && cy < surf->h) {
		cx += stepx;
		cy += stepy;
		int bx = (int)cx;
		int by = (int)cy;
		col = pixels[by * surf->w + bx];
	}
	return { (int)cx , (int)cy };
}


SDL_Surface* Gore::Engine::createCircle(int w, int h, SDL_Color startcolor) {
	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA8888);

	int x = ((w >> 1) - 1);
	int y = 0;
	int tx = 1;
	int ty = 1;
	int error = (tx - w);

	int sx = (w>>1);
	int sy = (h>>1);
	Uint32 col = ConvertColorToUint32RGB(startcolor, surf->format);
	while (x >= y)
	{
		//  Each of the following renders an octant of the circle
		SetPixelSurface(surf, sy - y, sx + x, col);
		SetPixelSurface(surf,sy + y, sx + x,  col);
		SetPixelSurface(surf, sy - y, sx - x, col);
		SetPixelSurface(surf, sy + y, sx - x, col);
		SetPixelSurface(surf, sy - x, sx + y, col);
		SetPixelSurface(surf, sy + x, sx + y, col);
		SetPixelSurface(surf, sy - x, sx - y, col);
		SetPixelSurface(surf, sy + x, sx - y, col);
		if (error <= 0)
		{
			++y;
			error += ty;
			ty += 2;
		}

		if (error > 0)
		{
			--x;
			tx += 2;
			error += (tx - w);
		}
	}
	return surf;
}
SDL_Surface* Gore::Engine::fillCircle(int w, int h, SDL_Color startcolor) {
	int radius = (w >> 1);
	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA8888);
	Uint32 curcol = ConvertColorToUint32RGB(startcolor, surf->format);
	double area = M_PI * (std::pow((double)radius, 2));
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			float dx = radius - x;
			float dy = radius - y;
			float distance = sqrtf(std::powf(dx, 2) + std::powf(dy, 2));
			if (distance - radius < 1) {
				SetPixelSurface(surf, &y, &x, &curcol);
			}
		}
	}
	return surf;
}
SDL_Surface* Gore::Engine::createBloom(int w, int h, SDL_Color startcolor, float magnitude) {
	int radius = (w >> 1);
	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA8888);
	SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_ADD);
	Uint32 curcol = ConvertColorToUint32RGBA(startcolor, surf->format);
	double area = M_PI * (std::pow((double)radius, 2));
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			float dx = radius - x;
			float dy = radius - y;
			float distance = sqrtf(std::powf(dx, 2) + std::powf(dy, 2));
			float xp, yp;
			if (x < radius) {
				xp = std::powf(std::abs(0 - x), 2);
			}
			else {
				xp = std::powf(w - x, 2);
			}
			if (y < (h>>1)) {
				yp = std::powf(std::abs(0 - y), 2);
			}
			else {
				yp = std::powf(h - y, 2);
			}
			float distfromside = sqrtf(xp + yp);
			if (distance - radius < 1) {
				curcol = ConvertColorToUint32RGBA({ startcolor.r, startcolor.g, startcolor.b, (Uint8)(distfromside * magnitude) }, surf->format);
				SetPixelSurface(surf, &y, &x, &curcol);
			}
		}
	}
	return surf;
}


//Memory related
//This should work fine even if your computer pads structs, but also might not I have no way to test
char* Gore::Engine::serilizeStruct(char* ptr, int size) {
	char* mt = (char*)std::malloc(size);
	for (int i = 0; i < size; i++) {
		mt[i] = *ptr;
		ptr++;
	}
	return mt;
}
void Gore::Engine::deserilizeStruct(char* dest, char* data, int size) {
	for (int i = 0; i < size; i++) {
		*dest = data[i];
		dest++;
	}
}
//point system
//Memory footprint high, if I pack data into bits instead of bytes will reduce greatly
//Really useful for an animation system
bool* Gore::Engine::createPoints(SDL_Surface* surf) {
	bool* pt = (bool*)std::malloc((surf->w * surf->h));
	for (int i = 0; i < surf->h; i++) {
		for (int j = 0; j < surf->w; j++) {
			Uint32 col = GetPixelSurface(surf, &i, &j);
			if (col > 0) {
				*pt = false;
			}
			else {
				*pt = true;
			}
			pt++;
		}
	}
	return pt;
}
//Need to improve finding of differences between frames; ignores some differences for some reason
Gore::TrList Gore::Engine::generatePixelTransforms(spxp& spritelist) {
	TrList list = NULL;
	spxp ptr = spritelist;
	spxp bef = NULL;
	//getting last frame so we can reset frame with first frame data
	while (ptr != NULL) {
		bef = ptr;
		ptr = ptr->next;
	}
	ptr = spritelist;
	SDL_Surface* prev = bef->current;
	//first transform needs to be differences between last frame and first
	//second frame is basically being skipped in the data; color data seems to be wrong for 
	while (ptr != NULL) {
		PixelTransform* p = new PixelTransform;
		size_t size = 0;
		std::vector<int>xs;
		std::vector<int>ys;
		std::vector<Uint32>cols;
		//parse difference from previous frame
		for (int i = 0; i < ptr->current->h; i++) {
			for (int j = 0; j < ptr->current->w; j++) {
				Uint32 curcol = GetPixelSurface(ptr->current, &i, &j);
				Uint32 prevcol = GetPixelSurface(prev, &i, &j);
				if (curcol != prevcol) {
					xs.push_back(j);
					ys.push_back(i);
					cols.push_back(prevcol);
					size += 12;
				}
			}
		}
		p->data = (char*)std::malloc(size);
		for (int i = 0, ind = 0; i < size; ind++) {
			char dat[12];
			//write position data
			Uint32* c = (Uint32*)dat;
			*c = (Uint32)xs[ind];
			c++;
			*c = (Uint32)ys[ind];
			c++;
			//writing color data now
			*c = cols[ind];
			//writing to data block
			for (int j = 0; j < 12; j++, i++) {
				p->data[i] = dat[j];
			}
		}
		p->size = size;
		p->next = list;
		list = p;
		prev = ptr->current;
		if (ptr->next == NULL) {
			bef = ptr;
		}
		ptr = ptr->next;
	}
	return list;
}
void Gore::Engine::switchTranformFrames(SDL_Surface* surf, TrList& frames, TrList& begin) {
	//go through each frames data and change the surface
		for (int i = 0; i < frames->size; i += 12) {
			Uint32* p = (Uint32*)&frames->data[i];
			Uint32 x, y, col;
			x = *p;
			p++;
			y = *p;
			p++;
			col = *p;
			SetPixelSurface(surf, y, x, col);
		}
		frames = frames->next;
		if (frames == NULL) {
			frames = begin;
		}
}
SDL_Surface* Gore::Engine::Engine::initTransformSurf(spxp& head) {
	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, head->current->w, head->current->h, 32, SDL_PIXELFORMAT_RGBA8888);
	clearSurface(surf);
	for (int i = 0; i < surf->h; i++) {
		for (int j = 0; j < surf->w; j++) {
			Uint32 col = GetPixelSurface(head->current, &i, &j);
			SetPixelSurface(surf, &i, &j, &col);
		}
	}
	return surf;
}

//misc
//Takes in degrees returns radians
float Gore::Engine::trajX(float deg) {
	deg = cos(deg * M_PI / 180);
	return deg;
}
//Takes in degrees return radians
float Gore::Engine::trajY(float deg) {
	deg = sin(deg * M_PI / 180);
	return deg;
}
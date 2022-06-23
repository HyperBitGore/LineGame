#pragma once
#include <iostream>
#include <fstream>
#include <random>
#include <SDL.h>
#include <array>
#include <list>
#include "lodepng.h"

//Didn't want to include windows header to do resource exe inclusion so just follow along with this page if you want to do this. 
//https://stackoverflow.com/questions/22151099/including-data-file-into-c-project
//or this
//https://stackoverflow.com/questions/48326287/is-there-a-cross-platform-way-to-embed-resources-in-a-binary-application-written



namespace Gore {
	struct TexListMem {
		SDL_Texture* current;
		TexListMem* next;
		std::string name;
	};
	typedef TexListMem* texp;
	struct SpriteListMem {
		SDL_Surface* current;
		SpriteListMem* next;
		std::string name;
	};
	typedef SpriteListMem* spxp;
	struct Point {
		int x;
		int y;
	};
	//4 byte: x, 4 byte: y, 4 byte: color data; repeat through data;
	struct PixelTransform {
		char* data;
		size_t size;
		PixelTransform* next;
	};
	typedef PixelTransform* TrList;

	class DeltaTimer {
	private:
		Uint64 LAST = 0;
		Uint64 NOW = SDL_GetPerformanceCounter();
	public:
		double getDelta();
	};
	class Engine {
	public:
		//Texture lists
		static void insertTex(TexListMem*& tex, SDL_Texture* current, std::string name);
		static SDL_Texture* findTex(texp head, std::string name);
		//Surface lists
		static void insertSprite(SpriteListMem*& sp, SDL_Surface* surf, std::string name);
		static SDL_Surface* findSprite(spxp sp, std::string name);
		//pixel manipulation
		static void SetPixelSurface(SDL_Surface* surf, int* y, int* x, Uint32* pixel);
		static void SetPixelSurface(SDL_Surface* surf, int y, int x, Uint32 pixel);

		static Uint32 GetPixelSurface(SDL_Surface* surf, int* y, int* x);
		static void SetPixelSurfaceColorRGB(SDL_Surface* surf, int* y, int* x, SDL_Color* color);
		static void SetPixelSurfaceColorRGBA(SDL_Surface* surf, int* y, int* x, SDL_Color* color);
		static Uint32 ConvertColorToUint32RGB(SDL_Color color, SDL_PixelFormat* format);
		static Uint32 ConvertColorToUint32RGBA(SDL_Color color, SDL_PixelFormat* format);
		static void clearSurface(SDL_Surface* surf);
		//texture manipulation
		static void clearTexture(SDL_Texture* tex, int* pitch, int w, int h);
		static void SetPixelTexture(SDL_Texture* tex, int* y, int* x, Uint32* pixel, int* pitch);
		static Uint32 GetPixelTexture(SDL_Texture* tex, int* y, int* x, int* pitch);
		//image loading
		static SDL_Surface* loadPNG(std::string name, SDL_PixelFormatEnum format, int w, int h);
		static SDL_Surface* LoadBMP(const char* file, SDL_PixelFormatEnum format);
		static texp& loadTextureList(std::vector<std::string> names, std::vector<unsigned int> widths, std::vector<unsigned int> heights, SDL_PixelFormatEnum format, SDL_Renderer* rend, std::string filepath = "NULL");
		static spxp& loadSpriteList(std::vector<std::string> names, std::vector<unsigned int> widths, std::vector<unsigned int> heights, SDL_PixelFormatEnum format, std::string filepath = "NULL");
		//Text functions
		static void mapTextTextures(int start, texp& out, texp& input);
		static void drawText(SDL_Renderer* rend, texp& texthead, std::string text, int x, int y, int w, int h);
		//misc utility
		static Point raycast2DPixel(SDL_Surface* surf, int sx, int sy, float angle, int step);
		static SDL_Surface* createCircle(int w, int h, SDL_Color startcolor);
		static SDL_Surface* fillCircle(int w, int h, SDL_Color startcolor);
		static SDL_Surface* createBloom(int w, int h, SDL_Color startcolor, float magnitude);
		//memory related
		static char* serilizeStruct(char* ptr, int size);
		static void deserilizeStruct(char* dest, char* data, int size);
		//point system
		static bool* createPoints(SDL_Surface* surf);
		static TrList generatePixelTransforms(spxp& spritelist);
		static void switchTranformFrames(SDL_Surface* surf, TrList& frames, TrList& begin);
		static SDL_Surface* initTransformSurf(spxp& head);
		//misc
		static float trajX(float deg);
		//Takes in degrees return radians
		static float trajY(float deg);

	};

	//procedural animation system
	namespace Animate {
		//Forward Kinematics bone
		class FKBone {
		private:
			float length;
		public:
			bool forw = true;
			float angle;
			FKBone* forward;
			FKBone* backward;
			float x;
			float y;
			FKBone(float ia, float il, float ix, float iy) { angle = ia; length = il; x = ix; y = iy; forward = NULL; backward = NULL; }
			//need to calculate angle for each based on sum of parents angles
			float getEndX() {
				float tangle = angle;
				FKBone* t = backward;
				while (t != NULL) {
					tangle += t->angle;
					t = t->backward;
				}
				return x + std::cosf(tangle) * length;
			}
			float getEndY() {
				float tangle = angle;
				FKBone* t = backward;
				while (t != NULL) {
					tangle += t->angle;
					t = t->backward;
				}
				return y + std::sinf(tangle) * length;
			}

		};
		//class to easily initilize list of bones
		class FKLimb {
		public:
			std::vector<FKBone> bones;
			//put bones in order of where you want them to align in limb structure
			FKLimb(std::vector<FKBone> bs) {
				bones = bs;
				for (int i = 0; i < bones.size(); i++) {
					if (i - 1 >= 0) {
						bones[i].backward = &bones[i - 1];
					}
					else {
						bones[i].backward = NULL;
					}
					if (i + 1 < bones.size()) {
						bones[i].forward = &bones[i + 1];
					}
					else {
						bones[i].forward = NULL;
					}

				}
			}
			~FKLimb() {
				bones.clear();
			}
			//
			void update() {
				for (int i = 0; i < bones.size(); i++) {
					if (bones[i].backward != NULL) {
						bones[i].x = bones[i - 1].getEndX();
						bones[i].y = bones[i - 1].getEndY();
					}
				}
			}
			//handle timing yourself
			int animate(int index, float minang, float maxang, float ainc) {
				if (index > bones.size() - 1) {
					return -1;
				}
				//edit targeted angles in a set angle range	
				if (bones[index].forw) {
					bones[index].angle += ainc;
				}
				else {
					bones[index].angle -= ainc;
				}
				if (bones[index].angle <= minang) {
					bones[index].forw = true;
				}
				else if (bones[index].angle >= maxang) {
					bones[index].forw = false;
				}
				return 1;
			}
			void debugDraw(SDL_Renderer* rend) {
				for (int i = 0; i < bones.size(); i++) {
					SDL_SetRenderDrawColor(rend, 255, 100, 150, 0);
					SDL_RenderDrawLineF(rend, bones[i].x, bones[i].y, bones[i].getEndX(), bones[i].getEndY());
					SDL_SetRenderDrawColor(rend, 255, 0, 0, 0);
					SDL_Rect rect = { bones[i].x, bones[i].y, 5, 5 };
					SDL_RenderFillRect(rend, &rect);
				}
			}
		};
		//Inverse Kinematics bone
		class IKBone {
		private:
			float angle;
		public:
			float length;
			IKBone* backward = NULL;
			IKBone* forward = NULL;
			float x, y;
			IKBone(float ix, float iy, float il, float ia) { x = ix; y = iy; length = il; angle = ia; }
			float getEndX() {
				return x + std::cosf(angle) * length;
			}
			float getEndY() {
				return y + std::sinf(angle) * length;
			}
			void pointAt(float ix, float iy) {
				float dx = ix - x;
				float dy = iy - y;
				angle = std::atan2f(dy, dx);
			}
			void drag(float ix, float iy) {
				pointAt(ix, iy);
				x = ix - std::cosf(angle) * length;
				y = iy - std::sinf(angle) * length;
				if (backward != NULL) {
					backward->drag(x, y);
				}
			}
		};

		class IKLimb {
		private:
			float x;
			float y;
		public:
			std::vector<IKBone> bones;
			IKLimb(IKBone b, int num) {
				bones.push_back(b);
				x = b.x;
				y = b.y;
				for (int i = 0; i < num; i++) {
					b.x += b.length;
					bones.push_back(b);
				}
				for (int i = 0; i < bones.size(); i++) {
					if (i - 1 >= 0) {
						bones[i].backward = &bones[i - 1];
					}
					else {
						bones[i].backward = NULL;
					}
					if (i + 1 < bones.size()) {
						bones[i].forward = &bones[i + 1];
					}
					else {
						bones[i].forward = NULL;
					}

				}

			}
			IKLimb(std::vector<IKBone> bs) {
				if (bs.size() > 0) {
					x = bs[0].x;
					y = bs[0].y;
					bones = bs;
					for (int i = 0; i < bones.size(); i++) {
						if (i - 1 >= 0) {
							bones[i].backward = &bones[i - 1];
						}
						else {
							bones[i].backward = NULL;
						}
						if (i + 1 < bones.size()) {
							bones[i].forward = &bones[i + 1];
						}
						else {
							bones[i].forward = NULL;
						}

					}
				}
				else {
					x = 0;
					y = 0;
				}

			}
			~IKLimb() {
				bones.clear();
			}
			void drag(float ix, float iy) {
				if (bones.size() > 0) {
					bones[bones.size() - 1].drag(ix, iy);
				}
			}
			void update() {
				for (int i = 0; i < bones.size(); i++) {
					if (bones[i].backward != NULL) {
						bones[i].x = bones[i - 1].getEndX();
						bones[i].y = bones[i - 1].getEndY();
					}
					else {
						bones[i].x = x;
						bones[i].y = y;
					}
				}
			}
			void reach(float ix, float iy) {
				drag(ix, iy);
				update();
			}
			void debugDraw(SDL_Renderer* rend) {
				for (int i = 0; i < bones.size(); i++) {
					SDL_SetRenderDrawColor(rend, 255, 100, 150, 0);
					SDL_RenderDrawLineF(rend, bones[i].x, bones[i].y, bones[i].getEndX(), bones[i].getEndY());
					SDL_SetRenderDrawColor(rend, 255, 0, 0, 0);
					SDL_Rect rect = { bones[i].x, bones[i].y, 5, 5 };
					SDL_RenderFillRect(rend, &rect);
				}
			}
		};
	}
	class Particle {
	public:
		float x;
		float y;
		float trajx;
		float trajy;
		int rangehigh;
		int rangelow;
		double animtime = 0;
		double movetime = 0;
		bool erase;
		SDL_Rect rect;
		Gore::texp bhead;
		Gore::texp head;
		Uint8 alpha = 255;
	public:
		Particle() { rangehigh = 0; rangelow = 0; x = 0; y = 0; trajx = 0; trajy = 0; rect = { 0, 0, 0, 0 }; head = NULL; bhead = head; erase = false; }
		Particle(float cx, float cy, int rangel, int rangeh, SDL_Rect crect, Gore::texp list) { rangehigh = rangeh; rangelow = rangel; x = cx; y = cy; trajx = 0; trajy = 0; rect = crect; head = list; bhead = head; erase = false; };
		virtual void draw(SDL_Renderer* rend) {
			SDL_SetTextureAlphaMod(head->current, alpha);
			rect.x = x;
			rect.y = y;
			SDL_RenderCopy(rend, head->current, NULL, &rect);
			SDL_SetTextureAlphaMod(head->current, 0);
		}
		virtual void update(double* delta) {
			animtime += *delta;
			movetime += *delta;
			if (movetime > 0.05) {
				x += trajx;
				y += trajy;
				movetime = 0;
			}
			if (animtime > 0.1) {
				head = head->next;
				alpha -= 5;
				if (alpha <= 0) {
					erase = true;
					alpha = 0;
				}
				if (head == NULL) {
					head = bhead;
				}
				animtime = 0;
			}
		}
	};

	class Emitter {
	private:
		std::vector<Particle> particles;
		Particle* p;
	public:
		Emitter() { p = NULL; timetospawn = 0.1; }
		Emitter(Particle* par, double spawntime) { p = par; timetospawn = spawntime; }
		double ctime = 0;
		double timetospawn;
		virtual void spawnParticle() {
			p->trajx = cos(double(p->rangelow + (std::rand() % (p->rangehigh - p->rangelow + 1))) * M_PI / 180.0);
			p->trajy = sin(double(p->rangelow + (std::rand() % (p->rangehigh - p->rangelow + 1))) * M_PI / 180.0);
			particles.push_back(*p);
		}
		virtual void update(double* delta, SDL_Renderer* rend) {
			ctime += *delta;
			if (ctime > timetospawn) {
				spawnParticle();
				ctime = 0;
			}
			for (int i = 0; i < particles.size();) {
				particles[i].update(delta);
				particles[i].draw(rend);
				if (particles[i].erase) {
					particles.erase(particles.begin() + i);
				}
				else {
					i++;
				}
			}
		}
	};

	class Bounder {
	private:
	public:
		float x, y;
		int w, h;
		Bounder() { x = 0; y = 0; w = 50; h = 50; }
		Bounder(float ix, float iy, int iw, int ih) { x = ix; y = iy; w = iw; h = ih; }

		bool contains(float ix, float iy) {
			return!(ix < x || iy < y || ix >= (x + w) || iy >= (y + h));
		}
		bool contains(Bounder b) {
			return (b.x >= x && b.x + b.w < x + w && b.y >= y && b.y + b.h < y + h);
		}
		bool overlaps(Bounder b) {
			bool bt = (x < b.x + b.w && x + w >= b.x && y < b.y + b.h && y + h >= b.y);
			return bt;
		}
	};

	namespace QTree {
		template<class TI>
		class QuadTree;

		template<typename TO>
		struct QuadStore;


		template<class T>
		class QuadItem {
		public:
			T p;
			QuadTree<T>* qt;
			typename std::list<QuadStore<T>>::iterator pos;
		};
		template<typename TK>
		struct QuadStore {
			QuadItem<TK> item;
			Bounder b;
		};

		template<typename TE>
		struct ReturnItem {
			QuadItem<TE>& item;
			Bounder* b;
		};

		constexpr int M_DEPTH = 8;
		template<class TP>
		class QuadTree {
		protected:
			Bounder area_bounds[4];
			QuadTree<TP>* children[4] = { NULL, NULL, NULL, NULL };
			//items on this node
			std::list<QuadStore<TP>> items;
			Bounder area;
			size_t depth;
		public:
			QuadTree(Bounder b, size_t d) {
				resize(b);
				depth = d;
			}
			std::list<ReturnItem<TP>> search(Bounder b) {
				std::list<ReturnItem<TP>> li;
				search(b, li);
				return li;
			}
			void search(Bounder b, std::list<ReturnItem<TP>>& li) {
				for (auto& i : items) {
					if (b.overlaps(i.b)) {
						ReturnItem<TP> p = { i.item, &i.b };
						li.push_back(p);
					}
				}
				for (int i = 0; i < 4; i++) {
					if (children[i] != NULL) {
						if (b.contains(area_bounds[i])) {
							children[i]->itemsAdd(li);
						}
						else if (area_bounds[i].overlaps(b)) {
							children[i]->search(b, li);
						}
					}
				}
			}
			void itemsAdd(std::list<ReturnItem<TP>>& li) {
				for (auto& i : items) {
					ReturnItem<TP> p = { i.item, &i.b };
					li.push_back(p);
				}
				for (int i = 0; i < 4; i++) {
					if (children[i] != NULL) {
						children[i]->itemsAdd(li);
					}
				}
			}
			//clears and changes tree area
			void resize(Bounder b) {
				clear();
				area = b;
				area_bounds[0] = Bounder(b.x, b.y, b.w / 2.0f, b.h / 2.0f);
				area_bounds[1] = Bounder(b.x + (b.w / 2.0f), b.y, b.w / 2.0f, b.h / 2.0f);
				area_bounds[2] = Bounder(b.x, b.y + (b.h / 2.0f), b.w / 2.0f, b.h / 2.0f);
				area_bounds[3] = Bounder(b.x + (b.w / 2.0f), b.y + (b.h / 2.0f), b.w / 2.0f, b.h / 2.0f);
			}
			//gets number of objects in tree
			size_t retsize() {
				size_t tet = 0;
				retsize(&tet);
				return tet;
			}
			void retsize(size_t* tet) {
				*tet += items.size();
				for (int i = 0; i < 4; i++) {
					if (children[i] != NULL) {
						children[i]->retsize(tet);
					}
				}
			}
			//clears entire tree of data
			void clear() {
				items.clear();
				for (int i = 0; i < 4; i++) {
					if (children[i] != NULL) {
						children[i]->clear();
						delete children[i];
						children[i] = NULL;
					}
				}
			}
			void insert(TP p, Bounder b) {
				for (int i = 0; i < 4; i++) {
					if (area_bounds[i].contains(b)) {
						if (depth + 1 <= M_DEPTH) {
							if (children[i] == NULL) {
								children[i] = new QuadTree(area_bounds[i], depth + 1);
							}
							children[i]->insert(p, b);
							return;
						}
					}
				}
				typename std::list<QuadStore<TP>>::iterator it = items.end();
				QuadItem<TP> ip = { p, this, it };
				QuadStore<TP> out = { ip, b };
				items.push_back(out);
				//have to set the actual pos value after because it doesn't exist otherwise
				items.back().item.pos = --items.end();
			}
			void remove(typename std::list<ReturnItem<TP>>::iterator& it) {
				it->item.qt->items.erase(it->item.pos);
			}
			bool move(typename std::list<ReturnItem<TP>>::iterator it) {
				if (!it->item.qt->area.contains(it->item.pos->b)) {
					TP tp = it->item.p;
					Bounder tb = it->item.pos->b;
					remove(it);
					insert(tp, tb);
					return true;
				}
				return false;
			}
		};
	}
}

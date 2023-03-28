#include <cmath>
#include <cstdio>

#include <iostream>
#include <limits>
#include <mutex>
#include <thread>

#include <map>
#include <set>
#include <string>
#include <vector>

#ifdef WIN32
#include <Windows.h>
#endif



#define UTF8_3(BYTE0,BYTE1,BYTE2) BYTE0 | (BYTE1<<8) | (BYTE2<<16)

//https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit
#define COLOR_RADAR_BG       22 //58
#define COLOR_RADAR_GRID    118 //195
#define COLOR_AIRCRAFT      196
#define COLOR_AIRCRAFT_ADDR  51

using Degrees = float;

template<class T> constexpr T qNaN = std::numeric_limits<T>::quiet_NaN();

template<class T> struct Vec2 final {
	T x, y;
	Vec2() = default;
	Vec2(T x,T y) : x(x),y(y) {}
	template<class T2> explicit Vec2(Vec2<T2> const& other) : x((T)other.x),y((T)other.y) {}
};
template<class T> constexpr static Vec2<T> operator*(Vec2<T> const& a,T sc) { return { a.x*sc, a.y*sc }; }
template<class T> constexpr static Vec2<T> operator-(Vec2<T> const& a,Vec2<T> const& b) { return { a.x-b.x, a.y-b.y }; }
template<class T> constexpr static Vec2<T> operator+(Vec2<T> const& a,Vec2<T> const& b) { return { a.x+b.x, a.y+b.y }; }
template<class T> constexpr static Vec2<T> operator/(Vec2<T> const& a,Vec2<T> const& b) { return { a.x/b.x, a.y/b.y }; }
template<class T> constexpr static Vec2<T> operator*(Vec2<T> const& a,Vec2<T> const& b) { return { a.x*b.x, a.y*b.y }; }
template<class T> constexpr static bool operator==(Vec2<T> const& a,Vec2<T> const& b) { return a.x==b.x && a.y==b.y; }
using Vec2f = Vec2<float>;
using Vec2u = Vec2<unsigned>;
using CellInd = Vec2<int32_t>;
using Position = Vec2<Degrees>;

//static Position camera_cen = { -97.088857f, 32.831865f }; //long/lat degrees



struct Aircraft final {
	std::string addr;

	Position pos;      //long/lat degrees
	Degrees  heading;  //degrees
	float    altitude; //meters
	float    speed;    //m/s

	int squawk_code;

	explicit Aircraft(std::string const& addr) :
		addr(addr),
		pos{qNaN<Degrees>,qNaN<Degrees>}, heading(qNaN<Degrees>), speed(qNaN<float>),
		squawk_code(-1)
	{}
};



struct Pixel final {
	//https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit
	uint8_t color_bg;
	uint8_t color_fg;
	uint16_t : 16;
	uint32_t glyph_fg;

	void draw_clear() {
		color_bg = COLOR_RADAR_BG;
		color_fg = 255;
		glyph_fg = ' ';
	}
};

struct Map final {
	Vec2u res = { 0u, 0u };
	Position coord_lo, coord_hi;
	std::vector<Pixel> pixels;

	std::mutex _mutex;

	void resize(Vec2u const& res) {
		if (this->res!=res) {
			this->res = res;

			pixels.resize( res.y * res.x );
		}
	}

	CellInd pos_to_cellind(Vec2f const& pos) {
		Vec2f part = ( pos - coord_lo ) / ( coord_hi - coord_lo );
		Vec2f pixelf = Vec2f(res) * part;
		return CellInd( (int)pixelf.x, (int)pixelf.y );
	}
	Pixel*  pos_to_pixel  (Vec2f const& pos) {
		Vec2f part = ( pos - coord_lo ) / ( coord_hi - coord_lo );
		Vec2f pixelf = Vec2f(res) * part;
		if ( pixelf.x<0.0f || pixelf.y<0.0f || pixelf.x>=res.x || pixelf.y>=res.y ) return nullptr;
		return pixels.data() + (int)res.x*(int)pixelf.y + (int)pixelf.x;
	}

	bool is_valid(CellInd const& ind) const {
		return ind.x>=0 && ind.y>=0 && (unsigned)ind.x<res.x && (unsigned)ind.y<res.y;
	}
	Pixel& pixel_at(CellInd const& ind) { return pixels[ ind.y*res.x + ind.x ]; }

	void acquire() { _mutex.lock  (); }
	void release() { _mutex.unlock(); }

	void draw_clear() {
		for (Pixel& pixel : pixels) pixel.draw_clear();
	}
	void draw_grid() {
		std::set<unsigned> gridlines_lat  = { 0u, res.y/4u, res.y/2u, 3u*res.y/4u, res.y-1u };
		std::set<unsigned> gridlines_long = { 0u, res.x/4u, res.x/2u, 3u*res.x/4u, res.x-1u };

		for     ( unsigned j=0; j<res.y; ++j ) {
			for ( unsigned i=0; i<res.x; ++i ) {
				bool is_lat  = gridlines_lat .find(j)!=gridlines_lat .cend();
				bool is_long = gridlines_long.find(i)!=gridlines_long.cend();
				uint32_t ch;
				//strs = [ "╔","╗","╤", "╚","╝","╧", "╟","╢","┼", "═","─", "║","│" ]
				if (is_lat&&is_long) {
					if (j==0) {
						if      (i==      0u) ch=UTF8_3( 0xe2, 0x95, 0x94 ); //╔
						else if (i==res.x-1u) ch=UTF8_3( 0xe2, 0x95, 0x97 ); //╗
						else                  ch=UTF8_3( 0xe2, 0x95, 0xa4 ); //╤
					} else if (j==res.y-1u) {
						if      (i==      0u) ch=UTF8_3( 0xe2, 0x95, 0x9a ); //╚
						else if (i==res.x-1u) ch=UTF8_3( 0xe2, 0x95, 0x9d ); //╝
						else                  ch=UTF8_3( 0xe2, 0x95, 0xa7 ); //╧
					} else {
						if      (i==      0u) ch=UTF8_3( 0xe2, 0x95, 0x9f ); //╟
						else if (i==res.x-1u) ch=UTF8_3( 0xe2, 0x95, 0xa2 ); //╢
						else                  ch=UTF8_3( 0xe2, 0x94, 0xbc ); //┼
					}
				} else if (is_lat ) {
					ch = ( j==0 || j==res.y-1u ) ? UTF8_3( 0xe2, 0x95, 0x90 ) : UTF8_3( 0xe2, 0x94, 0x80 ); //═, ─
				} else if (is_long) {
					ch = ( i==0 || i==res.x-1u ) ? UTF8_3( 0xe2, 0x95, 0x91 ) : UTF8_3( 0xe2, 0x94, 0x82 ); //║, │
				} else {
					continue;
				}

				Pixel& pixel = pixels[ j*res.x + i ];
				pixel.color_fg = COLOR_RADAR_GRID;
				pixel.glyph_fg = ch;
		}}
	}
	void draw_aircrafts(std::map< std::string, std::unique_ptr<Aircraft> > const& aircrafts) {
		for (auto const& iter : aircrafts) {
			Aircraft const* aircraft = iter.second.get();
			if ( std::isnan(aircraft->pos.x) || std::isnan(aircraft->pos.y) ) continue;

			CellInd ind = pos_to_cellind(aircraft->pos);
			if (is_valid(ind)) {
				Pixel& pixel = pixel_at(ind);
				pixel.color_fg = COLOR_AIRCRAFT;
				pixel.glyph_fg = UTF8_3( 0xe2, 0x9c, 0x88 ); //✈
			}

			for ( size_t i=0; i<aircraft->addr.length(); ++i ) {
				CellInd ind2 = ind + CellInd( 1+(int)i, 0 );
				if (is_valid(ind2)) {
					pixel_at(ind2).color_fg = COLOR_AIRCRAFT_ADDR;
					pixel_at(ind2).glyph_fg = aircraft->addr[i];
				}
			}

			//if (cell) cell->draw_aircraft(aircraft);
			//← → ↓ ↑ ↖ ↗ ↘ ↙
		}
	}

	void draw_present() const {
		printf("\x1b[0m");
		for     ( unsigned j=0; j<res.y; ++j ) {
			for ( unsigned i=0; i<res.x; ++i ) {
				Pixel const& pixel = pixels[ j*res.x + i ];
				printf("\x1b[%u;%uH",j+1u,i+1u);
				printf(
					"\x1b[38;5;%d;48;5;%dm",
					(int)pixel.color_fg, (int)pixel.color_bg
				);

				uint32_t glyph = pixel.glyph_fg;
				for (int ic=0;ic<4;++ic) {
					if (!(glyph&0xFF)) break;
					putchar(glyph&0xFF); glyph>>=8;
				}
			}
			putchar('\n');
		}
	}
};



static Map* map;
static std::map< std::string, std::unique_ptr<Aircraft> >* aircrafts; //Address onto aircraft

inline static void update_aircraft() {
	bool last = false;
	do {
		std::vector<std::string> lines;

		while (true) {
			std::string line;
			std::getline(std::cin,line);

			if ( std::cin.bad() || std::cin.eof() ) { last=true; break; }
			if (line.empty()) break;

			lines.push_back(line);
		}
		if (lines.empty()) continue;

		map->acquire();

		Aircraft* aircraft = nullptr;
		for (std::string const& line : lines) {
			if (line.starts_with("  ICAO Address:  ")) { //e.g. "  ICAO Address:  A1DCF2 (Mode S / ADS-B)"
				std::string address;
				for (size_t i=17;;++i) if (line[i]!=' ') address+=line[i]; else break;
				if (address=="000000") break;

				auto iter = aircrafts->find(address);
				if (iter==aircrafts->cend()) {
					auto iter2 = aircrafts->emplace( address, new Aircraft(address) );
					aircraft = iter2.first->second.get();
				} else {
					aircraft = iter->second.get();
				}

				break;
			}
		}
		if (!aircraft) {
			map->release();
			continue;
		}

		for (std::string const& line : lines) {
			if (line.starts_with("  Speed:         ")) { //e.g. "  Speed:         269 kt groundspeed"
				std::string speed;
				for (size_t i=17;;++i) if (line[i]!=' ') speed+=line[i]; else break;

				//Convert from knots to meters per second.  What are you do doing standing in
				//	hurricane-speed winds throwing a board out the fuselage?  Aircraft speed has
				//	*never* been literally measured this way   :V
				aircraft->speed = (float)( atof(speed.c_str()) * (4.63/9.0) );
			} else if (line.starts_with("  CPR longitude: ")) { //e.g. "  CPR longitude: -97.16112 (101619)"
				if (line[17]=='(') continue;

				std::string long_deg;
				for (size_t i=17;;++i) if (line[i]!=' ') long_deg+=line[i]; else break;

				aircraft->pos.x = (float)atof(long_deg.c_str());
			} else if (line.starts_with("  CPR latitude:  ")) { //e.g. "  CPR latitude:  32.81234 (49490)"
				if (line[17]=='(') continue;

				std::string lat_deg;
				for (size_t i=17;;++i) if (line[i]!=' ') lat_deg+=line[i]; else break;

				aircraft->pos.y = (float)atof(lat_deg.c_str());
			} else if (line.starts_with("  Altitude:      ")) { //e.g. "  Altitude:      9775 ft barometric"
				std::string alt;
				for (size_t i=17;;++i) if (line[i]!=' ') alt+=line[i]; else break;

				//Convert from ft to meters.
				aircraft->altitude = (float)( atof(alt.c_str()) * 0.3048 );
			} else if (line.starts_with("  Heading:       245")) { //e.g. "  Heading:       245"
				aircraft->heading = (Degrees)atof(line.c_str()+17);
			} else if (line.starts_with("  Squawk:        ")) { //e.g. "  Squawk:        7374"
				aircraft->squawk_code = atoi(line.c_str()+17);
			}
		}

		map->release();
	} while (!last);
}

static bool running = true;
void main_update_aircraft() {
	while (running) {
		update_aircraft();
	}
}
void main_display() {
	#ifdef _WIN32
	{
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

		SetConsoleCP      (CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);

		DWORD mode;
		GetConsoleMode(handle,&mode);
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(handle,mode);
	}
	#endif

	while (running) {
		map->draw_clear();

		map->draw_grid();

		map->acquire();
		map->draw_aircrafts(*aircrafts);
		map->release();

		map->draw_present();
	}
}

int main( int /*argc*/, char* /*argv*/[] ) {
	Map _map; map=&_map;

	map->resize({ 81u, 41u }); //Multiple of 4 plus 1 for nice lines

	//{ -97.088857f, 32.831865f };
	map->coord_lo = { -98, 32 }; //long/lat degrees
	map->coord_hi = { -96, 34 }; //long/lat degrees



	std::map< std::string, std::unique_ptr<Aircraft> > _aircrafts; aircrafts=&_aircrafts;



	std::thread thread1( main_update_aircraft );
	std::thread thread2( main_display );
	thread1.join();
	thread2.join();

	//while (true) std::this_thread::yield();
	//getchar();



	return 0;
}

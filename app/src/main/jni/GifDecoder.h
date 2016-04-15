#ifndef _GIFDECODER_H_
#define _GIFDECODER_H_
#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>
#include "common.h"
#include <string.h>


using std::cout;
using std::endl;
using std::ifstream;

template <typename T> void log(const char *name, T value);
bool skipBlock(ifstream& is);
class ByteEater {
public:
	virtual bool eat(std::ifstream& is);
	virtual ~ByteEater();
	bool skip(std::ifstream& is);
	bool readu1(ifstream& is, u1 *dest);
	short convertu2(char* bytes);
};

class LSD : public ByteEater {
public:
	void resolvePacketFields(char& fileds);
	bool eat(std::ifstream& is);
	u2 sWidth;
	u2 sHeight;
	u1 bgIndex;
	bool hasGTable;
	u1 sizeGTable;
};

class ColorTable : ByteEater {
public:
	int tableSize;
	ColorTable(int size);
	bool eat(ifstream& is);
	char *colors;
	u1 red(int index);
	u1 green(int index) ;
	u1 blue(int index) ;
	~ColorTable();
};

class AppExtBlock : ByteEater {
public:
	bool eat(ifstream& is);
};

class GraphicCtrlExt : ByteEater {
public:
	bool eat(ifstream& is) ;
	u1 delay;
	bool transparency;
	u1 transparencyIndex;
};

class ImageDes : ByteEater {
public:
	bool eat(ifstream& is);
	u2 leftPos;
	u2 topPos;
	u2 iWidth;
	u2 iHeight;
	bool hasLTable;
	bool interlace;
	u1 sizeLTable;
};
struct Dict {
	char value;
	int len;
	int preIndex;
};

class LZWDecoder : ByteEater {
public:
	LZWDecoder(LSD *lsd_p, ImageDes* imgDes_p);
	u1 *pixels;
	bool eat(ifstream& is);
private:
	LSD *lsd;
	ImageDes *imgDes;
};

class GifDecoder {
public:
	GifDecoder();
	~GifDecoder();
	void loadGif(const char *file);
	void processStream(ifstream& is);

	u1* pixels;
	int width;
	int height;

	u4* getPixels() ;
	ColorTable* getColorTable();
	ColorTable *gct;
	ColorTable *fuck;

private:
};

template<typename _Tp>
class array_ptr {
private:
	_Tp *m_pointer;
public:
	typedef _Tp element_type;
	explicit array_ptr(element_type* p = 0): m_pointer(p) {}
	~array_ptr() {
		delete m_pointer;
	}
	element_type& operator*() const {
		assert(m_pointer != 0);
		return *m_pointer;
	}
	element_type* operator->() const {
		assert(m_pointer != 0);
		return m_pointer;
	}
	element_type* get() const {
		return m_pointer;
	}
	element_type* release() {
		element_type* tmp = m_pointer;
		m_pointer = 0;
		return tmp;
	}

};
void exportGifToTxt(GifDecoder* result);
#endif
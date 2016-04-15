#ifndef _GIFDECODER_H_
#define _GIFDECODER_H_
#include <vector>
#include <fstream>
#include <iostream>
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
	bool readUnsignedChar(ifstream& is, unsigned char *dest);
	short readShort(char* bytes);
};

class LSD : public ByteEater {
public:
	void resolvePacketFields(char& fileds);
	bool eat(std::ifstream& is);
	unsigned short sWidth;
	unsigned short sHeight;
	unsigned char bgIndex;
	bool hasGTable;
	unsigned char sizeGTable;
};

class ColorTable : ByteEater {
public:
	int tableSize;
	ColorTable(int size);
	bool eat(ifstream& is);
	char *colors;
	unsigned char red(int index);
	unsigned char green(int index) ;
	unsigned char blue(int index) ;
	~ColorTable();
};

class AppExtBlock : ByteEater {
public:
	bool eat(ifstream& is);
};

class GraphicCtrlExt : ByteEater {
public:
	bool eat(ifstream& is) ;
	uint8_t delay;
	bool transparency;
	unsigned char transparencyIndex;
};

class ImageDes : ByteEater {
public:
	bool eat(ifstream& is);
	unsigned short leftPos;
	unsigned short topPos;
	unsigned short iWidth;
	unsigned short iHeight;
	bool hasLTable;
	bool interlace;
	unsigned char sizeLTable;
};
struct Dict {
	char value;
	int len;
	int preIndex;
};

class LZWDecoder : ByteEater {
public:
	LZWDecoder(LSD *lsd_p, ImageDes* imgDes_p);
	uint8_t *pixels;
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
	void processStream(std::ifstream& is);

	uint8_t* pixels;
	int width;
	int height;

	uint32_t* getPixels() ;
	ColorTable* getColorTable();
	ColorTable *gct;
	ColorTable *fuck;

private:
};

void exportGifToTxt(GifDecoder* result);
#endif
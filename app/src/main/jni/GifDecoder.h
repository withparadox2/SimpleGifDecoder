#ifndef _GIFDECODER_H_
#define _GIFDECODER_H_
void processStream(std::ifstream& is);
struct LSD {
	unsigned short sWidth;
	unsigned short sHeight;
	char packetFields;
	char bgIndex;
	char aspectRatio;
};

class GifDecoder
{
public:
	GifDecoder();
	~GifDecoder();
	
};
#endif
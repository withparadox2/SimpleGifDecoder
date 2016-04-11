#ifndef _GIFDECODER_H_
#define _GIFDECODER_H_
#include <vector>
using std::cout;
using std::endl;
using std::ifstream;
template <typename T> void log(const char *name, T value);
void processStream(std::ifstream& is);
class ByteEater {
public:
    virtual bool eat(std::ifstream& is){
    };
    bool skip(std::ifstream& is) {
        char dataSize;
        for (;;) {
            if(!is.read(&dataSize, 1)) {
                return false;
            } 
            if(dataSize > 0) {
                is.seekg(dataSize, is.cur);
            } else {
                return true;
            }
        }
    }
    bool readUnsignedChar(ifstream& is, unsigned char *dest) {
        char temp;
        is.read(&temp, 1);
        *dest = temp;
        return is;
    }
    short readShort(char* bytes) {
        return bytes[1] << 8 | (unsigned char)bytes[0];
    } 
};
class LSD : public ByteEater {
public:
    void resolvePacketFields(char& fileds);
    bool eat(std::ifstream& is) {
        char* dst = new char[7];    
        is.read(dst, 7);
        if(!is) {
            return false;
        }
        sWidth = readShort(dst);
        sHeight = readShort(dst + 2);
        log("sWidth", sWidth);
        log("sHeight", sHeight);
        char packetFields = *(dst+4);
        bgIndex = *(dst+5);
        log("bgIndex", (int)bgIndex);
        resolvePacketFields(packetFields);
        delete[] dst;
        return true;
    }
    unsigned short sWidth;
    unsigned short sHeight;
    unsigned char bgIndex;
    bool hasGTable;
    unsigned char sizeGTable;
};
void LSD::resolvePacketFields(char& fileds) {
    hasGTable = (fileds & (1 << 7)) != 0;
    log("hasGTable", hasGTable);
    if(hasGTable) {
        sizeGTable = fileds & 0x7;
        log("sizeGTable", (int)sizeGTable);
    }
}
class ColorTable : ByteEater {
public:
    int tableSize;
    ColorTable(int size) : tableSize(size) {
        colors = new char[size*3];
        log("table color size", size*3);
    }
    bool eat(ifstream& is) {
        is.read(colors, tableSize*3);
        return is;
    }
    char *colors; 
    unsigned char red(int index) {
        return colors[3*index];
    }
    unsigned char green(int index) {
        return colors[3*index + 1];
    }
    unsigned char blue(int index) {
        return colors[3*index + 2];
    }
    ~ColorTable() {
        delete[] colors;
    }
};
class AppExtBlock : ByteEater {
public:
    bool eat(ifstream& is) {
        char blockSize;//11
        is.read(&blockSize, 1);
        if(!is) return false;
        // log("blockSize", (int)blockSize);
        char* block = new char[blockSize + 1];
        block[blockSize] = 0;
        is.read(block, blockSize);
        if(!is) {
            delete[] block;
            return false;
        }
        if(!strncmp("NETSCAPE2.0", block, blockSize)) {
            //todo : read netscape ext
            skip(is);
        } else {
            skip(is);
        }
        delete[] block;
        return true;
    }
};
class GraphicCtrlExt : ByteEater {
public:
    bool eat(ifstream& is) {
        char blockSize;
        is.read(&blockSize, 1);
        if(!is) return false;
        char* block = new char[blockSize];
        is.read(block, blockSize);
        if(!is) {
            delete[] block;
            return false;
        }
        char packetFields = block[0];
        transparency = (packetFields & 1) != 0;
        delay = readShort(block+1);
        transparencyIndex = block[3];
        //todo : handle dispose
        delete[] block;
        skip(is);
    }
    uint8_t delay;
    bool transparency;
    unsigned char transparencyIndex;
};
class ImageDes : ByteEater {
public:
    bool eat(ifstream& is) {
        char *data = new char[9];
        is.read(data, 9);
        if(!is) {
            delete[] data;
            return false;
        }
        leftPos = readShort(data);
        topPos = readShort(data + 2);
        iWidth = readShort(data + 4);
        iHeight = readShort(data + 6);
        char packetFields = data[8];
        hasLTable = (packetFields & 1<<7) != 0;
        interlace = (packetFields & 1<<6) != 0;
        sizeLTable = packetFields & 0x7;
        log("iWidth", iWidth);
        log("iHeight", iHeight);
        log("hasLTable", hasLTable);
        log("sizeLTable", (int)sizeLTable);
        return true;
    }
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
    LZWDecoder(LSD *lsd_p, ImageDes* imgDes_p) : lsd(lsd_p), imgDes(imgDes_p){}
    uint8_t *pixels;
    bool eat(ifstream& is) {
        int width = imgDes->iWidth;
        int height = imgDes->iHeight;
        int nPixels = width * height;

        pixels = new uint8_t[nPixels];

        char dataSize;
        if(!is.read(&dataSize, 1)) return false;

        log("dataSize", (int)dataSize);
        
        char codeSize = dataSize + 1;
        int codeMask = (1 << codeSize) - 1;
        int clearFlag = 1 << dataSize;
        int endFlag = clearFlag + 1;
        int available;
        unsigned char bits = 0;
        unsigned char blockAvailable = 0;
        uint32_t datum = 0;
        uint32_t code;
        int32_t preCode = -1;

        int pixelsLen = 0;

        std::vector<Dict> dicts(1 << codeSize);
        for (available = 0; available < (1 << dataSize); available++) {
        	Dict &dict = dicts[available];
        	dict.value = available;
        	dict.preIndex = -1;
        	dict.len = 1;

        }

        available++;//clear flag, we need plus one
        available++;//stop flag, we need plus one

        for (;;) {
        	cout << "for" << endl;
            while(bits < codeSize) {
                if(blockAvailable == 0) {
                    if (!readUnsignedChar(is, &blockAvailable)) return false;
                    if (blockAvailable == 0) {
                    	return true;
                    } 
                }
                char byte;
                if (!is.read(&byte, 1)) return false;
                // log("read byte", (uint32_t)(unsigned char)byte);
                datum |= ((uint32_t)(unsigned char) byte) << bits;
                bits += 8;
                blockAvailable--;
            }
            // log("before datum", datum);
            code = datum & codeMask;
            datum >>= codeSize;
            bits -= codeSize;
            // log("datum", datum);
            // log("code", code);

            if (code > available) {
                //dict doesn't hold this code, something wrong
                return false;
            } else if (code == endFlag) {
            	if(blockAvailable > 0) {
            		return false;
            	}
              break;
            } else if (code == clearFlag) {
                codeSize = dataSize + 1;
                codeMask = (1 << codeSize) - 1;
                preCode = -1;
                dicts.resize(1 << codeSize);
				 				for (available = 0; available < (1 << dataSize); available++) {
				        	Dict &dict = dicts[available];
				        	dict.value = available;
				        	dict.preIndex = -1;
				        	dict.len = 1;
				        }
				        available++;//clear flag, we need plus one
				        available++;//stop flag, we need plus one
				        continue;
            }

            if (preCode > -1 && codeSize < 13) {
            //	log("insert dict =======", 0);
            	int ptr = code;
            	if(code == available) {
            		ptr = preCode;
            	}
            	while (dicts[ptr].preIndex != -1) {
            		ptr = dicts[ptr].preIndex;
            	}
            	dicts[available].value = dicts[ptr].value;
            	dicts[available].preIndex = preCode;
            	dicts[available].len = dicts[preCode].len + 1;
            	available++;

            	if (available == (1 << codeSize) && codeSize < 12) {
            		codeSize++;
            		dicts.resize(1 << codeSize);
            	}
            }

            preCode = code;
            int matchLen = dicts[code].len;
            log("pixelsLen", pixelsLen);
            if(pixelsLen + matchLen > nPixels) return false;
            while(code != -1) {
            	pixels[dicts[code].len - 1] = (uint8_t)dicts[code].value;
            	code = dicts[code].preIndex;
            	
            }
            pixelsLen += matchLen;
            pixels += matchLen;
        }
        return true;
    }
private:
    LSD *lsd;
    ImageDes *imgDes;
};
class GifDecoder {
public:
    GifDecoder();
    ~GifDecoder();
};
#endif
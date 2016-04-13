#include <iostream>
#include <fstream>
#include <string.h>
#include "GifDecoder.h"
int main() {
    const char *file = "flower.gif";
    ifstream is(file, ifstream::binary);
    if (is.is_open()) {
        is.seekg(0, is.beg);
        processStream(is);
        is.close();
    }
}
GifDecoder* gifResult;
ColorTable *gct;

GifDecoder* loadGif(const char *file) {
    gifResult = new GifDecoder;
    ifstream is(file, ifstream::binary);
    if (is.is_open()) {
        is.seekg(0, is.beg);
        processStream(is);
        is.close();
    }
    return gifResult;
}
template <typename T> void log(const char *name, T value) {
    //cout << name << " = " << value << endl;
}

void processStream(ifstream& is) {
    char *header = new char[6];
    is.read(header, 6);
    if (!is) return;
    if (strncmp("GIF89a", header, 6)) {
        std::cerr << "input file is unsupported!";
        return;
    }
    delete []header;
    LSD lsd;
    if(!lsd.eat(is)) return;

    gct = new ColorTable(1 << (lsd.sizeGTable + 1));
    if(lsd.hasGTable) {
        if(!gct->eat(is)) return;
    }

    for(;;) {
        char blockType;
        is.read(&blockType, 1);
        if(!is) return;
        log("blockType", (int)blockType);
        switch((unsigned char)blockType) {
            case 0x21:
                char label;
                is.read(&label, 1);
                log("label", (int)label);
                if(!is) return;
                switch((unsigned char)label) {
                    case 0xff: {
                        AppExtBlock appExt;
                        appExt.eat(is);
                        break;
                    }
                    case 0xf9: {
                        GraphicCtrlExt gExt;
                        gExt.eat(is);
                        break;
                    }
                }
                break;
            case 0x2c: {
                ImageDes imgDes;
                if(!imgDes.eat(is)) return;
                if(imgDes.hasLTable) {
                    ColorTable lct(1 << (imgDes.sizeLTable + 1));
                    if(!lct.eat(is)) return;
                }
                LZWDecoder lzw(&lsd, &imgDes);
                lzw.eat(is);

                uint32_t size = lsd.sWidth * lsd.sHeight;
                log("size size" , size);
                gifResult->pixels = lzw.pixels;
                gifResult->width = lsd.sWidth;
                gifResult->height = lsd.sHeight;
                gifResult->gct = gct;

                return;
                //std::ofstream os("color.txt");
                //
                //for (int i=0; i< size; i++) {
                //	int index = pixels[i];
                //	int red = (int)gct.red(index);
                //	int green = (int)gct.green(index);
									//int blue = (int)gct.blue(index);
									//cout << "i = " << i << endl;
                //	os << red <<" "<<green<<" "<<blue << " ";
                //}
                //os.close();

                //break;
            }
            default:
                return;
        }
    }
}

void LSD::resolvePacketFields(char& fileds) {
    hasGTable = (fileds & (1 << 7)) != 0;
    log("hasGTable", hasGTable);
    if(hasGTable) {
        sizeGTable = fileds & 0x7;
        log("sizeGTable", (int)sizeGTable);
    }
}
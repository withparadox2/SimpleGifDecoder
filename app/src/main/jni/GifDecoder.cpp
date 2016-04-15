#include "GifDecoder.h"

int main() {
  const char *file = "bingbang.gif";
  ifstream is(file, ifstream::binary);
  if (is.is_open()) {
    is.seekg(0, is.beg);
    GifDecoder decoder;
    decoder.processStream(is);
    exportGifToTxt(&decoder);
    is.close();
    log("finish", "decode");
  }
}

bool skipBlock(ifstream& is) {
  char dataSize;
  for (;;) {
    if (!is.read(&dataSize, 1)) {
      return false;
    }
    if (dataSize > 0) {
      is.seekg(dataSize, is.cur);
    } else {
      return true;
    }
  }
}

//def class ByteEater
bool ByteEater::eat(ifstream& is) {}

ByteEater::~ByteEater() {};

bool ByteEater::skip(ifstream& is) {
  return skipBlock(is);
}

bool ByteEater::readUnsignedChar(ifstream& is, unsigned char *dest) {
  char temp;
  is.read(&temp, 1);
  *dest = temp;
  return is;
}
short ByteEater::readShort(char* bytes) {
  return bytes[1] << 8 | (unsigned char)bytes[0];
}

//def class LSD
void LSD::resolvePacketFields(char& fileds) {
  hasGTable = (fileds & (1 << 7)) != 0;
  log("hasGTable", hasGTable);
  if (hasGTable) {
    sizeGTable = fileds & 0x7;
    log("sizeGTable", (int)sizeGTable);
  }
}

bool LSD::eat(std::ifstream& is) {
  char* dst = new char[7];
  is.read(dst, 7);
  if (!is) {
    return false;
  }
  sWidth = readShort(dst);
  sHeight = readShort(dst + 2);
  log("sWidth", sWidth);
  log("sHeight", sHeight);
  char packetFields = *(dst + 4);
  bgIndex = *(dst + 5);
  log("bgIndex", (int)bgIndex);
  resolvePacketFields(packetFields);
  delete[] dst;
  return true;
}

//def class ColorTable
ColorTable::ColorTable(int size) : tableSize(size) {
  colors = new char[size * 3];
  log("table color size", size * 3);
}

bool ColorTable::eat(ifstream& is) {
  is.read(colors, tableSize * 3);
  return is;
}

unsigned char ColorTable::red(int index) {
  return colors[3 * index];
}
unsigned char ColorTable::green(int index) {
  return colors[3 * index + 1];
}
unsigned char ColorTable::blue(int index) {
  return colors[3 * index + 2];
}
ColorTable::~ColorTable() {
  delete[] colors;
}

//def class AppExtBlock
bool AppExtBlock::eat(ifstream& is) {
  char blockSize;//11
  is.read(&blockSize, 1);
  if (!is) return false;
  // log("blockSize", (int)blockSize);
  char* block = new char[blockSize + 1];
  block[blockSize] = 0;
  is.read(block, blockSize);
  if (!is) {
    delete[] block;
    return false;
  }
  if (!strncmp("NETSCAPE2.0", block, blockSize)) {
    //todo : read netscape ext
    skip(is);
  } else {
    skip(is);
  }
  delete[] block;
  return true;
}

//def class GraphicCtrlExt
bool GraphicCtrlExt::eat(ifstream& is) {
  char blockSize;
  is.read(&blockSize, 1);
  if (!is) return false;
  char* block = new char[blockSize];
  is.read(block, blockSize);
  if (!is) {
    delete[] block;
    return false;
  }
  char packetFields = block[0];
  transparency = (packetFields & 1) != 0;
  delay = readShort(block + 1);
  transparencyIndex = block[3];
  //todo : handle dispose
  delete[] block;
  skip(is);
}

//def class ImageDes

bool ImageDes::eat(ifstream& is) {
  char *data = new char[9];
  is.read(data, 9);
  if (!is) {
    delete[] data;
    return false;
  }
  leftPos = readShort(data);
  topPos = readShort(data + 2);
  iWidth = readShort(data + 4);
  iHeight = readShort(data + 6);
  char packetFields = data[8];
  hasLTable = (packetFields & 1 << 7) != 0;
  interlace = (packetFields & 1 << 6) != 0;
  sizeLTable = packetFields & 0x7;
  log("leftPos", leftPos);
  log("topPos", topPos);
  log("iWidth", iWidth);
  log("iHeight", iHeight);
  log("hasLTable", hasLTable);
  log("interlace", interlace);
  log("sizeLTable", (int)sizeLTable);
  return true;
}

//def class LZWDecoder

LZWDecoder::LZWDecoder(LSD *lsd_p, ImageDes* imgDes_p) : lsd(lsd_p), imgDes(imgDes_p) {}
bool LZWDecoder::eat(ifstream& is) {
  int width = imgDes->iWidth;
  int height = imgDes->iHeight;
  int nPixels = width * height;

  pixels = new uint8_t[nPixels];

  char dataSize;
  if (!is.read(&dataSize, 1)) return false;

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
    //cout << "for" << endl;
    while (bits < codeSize) {
      if (blockAvailable == 0) {
        if (!readUnsignedChar(is, &blockAvailable)) return false;
        if (blockAvailable == 0) {
          return true;
        }
      }
      char byte;
      if (!is.read(&byte, 1)) return false;
      datum |= ((uint32_t)(unsigned char) byte) << bits;
      bits += 8;
      blockAvailable--;
    }

    code = datum & codeMask;


    datum >>= codeSize;
    bits -= codeSize;
    //log("code", code);



    if (code > available) {
      //dict doesn't hold this code, something wrong
      return false;
    } else if (code == endFlag) {
      if (blockAvailable > 0) {
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
      //  log("insert dict =======", 0);
      int ptr = code;
      if (code == available) {
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
        codeMask = (1 << codeSize) - 1;

        dicts.resize(1 << codeSize);
      }
    }


    preCode = code;
    int matchLen = dicts[code].len;
    // log("pixelsLen", pixelsLen);
    if (pixelsLen + matchLen > nPixels) return false;
    while (code != -1) {
      // log("insert index",  )
      pixels[pixelsLen + dicts[code].len - 1] = (uint8_t)dicts[code].value;
      code = dicts[code].preIndex;

    }
    pixelsLen += matchLen;
    //pixels += matchLen;
  }
  return true;
}

//def class GifDecoder
GifDecoder::GifDecoder(): gct(NULL), pixels(NULL), width(-1), height(-1) {
}

GifDecoder::~GifDecoder() {
  if (gct) {
    delete gct;
  }
  if (pixels) {
    delete[] pixels;
  }
}

void GifDecoder::loadGif(const char *file) {
  ifstream is(file, ifstream::binary);
  if (is.is_open()) {
    is.seekg(0, is.beg);
    processStream(is);
    is.close();
  }
}

uint32_t* GifDecoder::getPixels() {
  int count = width * height;
  //LOGD("ccwidth = %i, height = %i, cout = %i", width, height, count);
  uint32_t *bitmap = new uint32_t[count];
  // LOGD("create bitmap");
  for (int i = 0; i < count; i++) {
    int index = pixels[i];

    unsigned char red = gct->red(index);
    unsigned char green =  gct->green(index);
    unsigned char blue =  gct->blue(index);
    bitmap[i] = 0xff << 24 | blue << 16 | green << 8 | red;
  }
  return bitmap;
}

ColorTable* GifDecoder::getColorTable() {
  return this->gct;
}


void GifDecoder::processStream(ifstream& is) {
  char *header = new char[6];
  is.read(header, 6);
  if (!is) return;
  if (strncmp("GIF89a", header, 6)) {
    std::cerr << "input file is unsupported!";
    return;
  }
  delete []header;
  LSD lsd;
  if (!lsd.eat(is)) return;

  ColorTable *gct = new ColorTable(1 << (lsd.sizeGTable + 1));
  if (lsd.hasGTable) {
    if (!gct->eat(is)) return;
  }

  for (;;) {
    char blockType;
    is.read(&blockType, 1);
    if (!is) return;
    log("blockType", (u4)(u1)blockType);
    switch ((unsigned char)blockType) {
    case 0x21:
      char label;
      is.read(&label, 1);
      log("label", (u4)(u1)label);
      if (!is) return;
      switch ((unsigned char)label) {
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
      case 0xfe:
      case 0x01:
      default:
        skipBlock(is);
        break;
      }
      break;
    case 0x2c: {
      ImageDes imgDes;
      if (!imgDes.eat(is)) return;
      if (imgDes.hasLTable) {
        ColorTable lct(1 << (imgDes.sizeLTable + 1));
        if (!lct.eat(is)) return;
      }
      LZWDecoder lzw(&lsd, &imgDes);
      lzw.eat(is);

      uint32_t size = lsd.sWidth * lsd.sHeight;
      this->pixels = lzw.pixels;
      this->width = lsd.sWidth;
      this->height = lsd.sHeight;
      this->gct = gct;

      return;
      //break;
    }
    case 0x3b: // terminator
      return;
    default:
      return;
    }
  }
}


void exportGifToTxt(GifDecoder* result) {
#ifndef GIF_ANDROID
  if (result == NULL) {
    return;
  }
  ColorTable *gct = result->getColorTable();
  std::ofstream os("color.txt");
  u4 size = result->width * result -> height;
  for (u4 i = 0; i < size; i++) {
    int index = (result->pixels)[i];
    int red = (int)gct->red(index);
    int green = (int)gct->green(index);
    int blue = (int)gct->blue(index);
    os << red << " " << green << " " << blue << " ";

  }
  os.close();
#endif
}

template <typename T> void log(const char *name, T value) {
#ifndef GIF_ANDROID
  cout << name << " = " << value << endl;
#endif
}

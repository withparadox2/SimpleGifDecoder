#include "GifDecoder.h"
using std::string;

int main() {
  string file("earth.gif");
  ifstream is(file.c_str(), ifstream::binary);
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
    u4 pos = (u1)dataSize;
    if (pos > 0) {
      is.seekg(pos, is.cur);
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

bool ByteEater::readu1(ifstream& is, u1 *dest) {
  char temp;
  is.read(&temp, 1);
  *dest = temp;
  return is;
}
short ByteEater::convertu2(char* bytes) {
  return bytes[1] << 8 | (u1)bytes[0];
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
  char *dst = new char[7];
  array_ptr<char> dstMgr(dst);
  is.read(dst, 7);
  if (!is) return false;
  sWidth = convertu2(dst);
  sHeight = convertu2(dst + 2);
  log("sWidth", sWidth);
  log("sHeight", sHeight);
  char packetFields = *(dst + 4);
  bgIndex = *(dst + 5);
  log("bgIndex", (int)bgIndex);
  resolvePacketFields(packetFields);
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

u1 ColorTable::red(int index) {
  return colors[3 * index];
}
u1 ColorTable::green(int index) {
  return colors[3 * index + 1];
}
u1 ColorTable::blue(int index) {
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
  array_ptr<char> arrayMgr(block);
  block[blockSize] = 0;
  is.read(block, blockSize);
  if (!is) {
    return false;
  }
  if (!strncmp("NETSCAPE2.0", block, blockSize)) {
    //todo : read netscape ext
    skip(is);
  } else {
    skip(is);
  }
  return true;
}

//def class GraphicCtrlExt
bool GraphicCtrlExt::eat(ifstream& is) {
  char blockSize;
  is.read(&blockSize, 1);
  if (!is) return false;
  char* block = new char[blockSize];
  array_ptr<char> arrayMgr(block);

  is.read(block, blockSize);
  if (!is) return false;
  char packetFields = block[0];
  transparency = (packetFields & 1) != 0;
  delay = convertu2(block + 1) * 10;
  transparencyIndex = block[3];
  //todo : handle dispose
  skip(is);
}

//def class ImageDes
bool ImageDes::eat(ifstream& is) {
  char *data = new char[9];
  array_ptr<char> arrayMgr(data);
  is.read(data, 9);
  if (!is)  return false;
  leftPos = convertu2(data);
  topPos = convertu2(data + 2);
  iWidth = convertu2(data + 4);
  iHeight = convertu2(data + 6);
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

  u1 codeSize = dataSize + 1;
  int codeMask = (1 << codeSize) - 1;
  int clearFlag = 1 << dataSize;
  int endFlag = clearFlag + 1;
  int available;
  u1 bits = 0;
  u1 blockAvailable = 0;
  u4 datum = 0;
  u4 code;
  int preCode = -1;

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
        if (!readu1(is, &blockAvailable)) return false;
        if (blockAvailable == 0) {
          return true;
        }
      }
      char byte;
      if (!is.read(&byte, 1)) return false;
      datum |= ((u4)(u1) byte) << bits;
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
      pixels[pixelsLen + dicts[code].len - 1] = (u1)dicts[code].value;
      code = dicts[code].preIndex;

    }
    pixelsLen += matchLen;
    //pixels += matchLen;
  }
  return skipBlock(is);
}

//def class GifDecoder
GifDecoder::GifDecoder(): gct(NULL), width(-1), height(-1) , frameCount(0) {
}

GifDecoder::~GifDecoder() {
  if (gct) {
    delete gct;
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

u4* GifDecoder::getPixels(u2 frameIndex) {
  int count = width * height;
  u1 *pixels = frames[frameIndex % frameCount].pixels;

  u4 *bitmap = new u4[count];
  for (int i = 0; i < count; i++) {
    int index = pixels[i];
    u1 red = gct->red(index);
    u1 green =  gct->green(index);
    u1 blue =  gct->blue(index);
    bitmap[i] = 0xff << 24 | blue << 16 | green << 8 | red;
  }
  return bitmap;
}

int GifDecoder::getFrameDelay(u2 index) {
  return frames[index % frameCount].graphExt->delay;
}

ColorTable* GifDecoder::getColorTable() {
  return this->gct;
}


void GifDecoder::processStream(ifstream& is) {
  char *header = new char[6];
  array_ptr<char> mgr(header);
  is.read(header, 6);
  if (!is) return;
  if (strncmp("GIF89a", header, 6)) {
    std::cerr << "input file is unsupported!";
    return;
  }
  LSD lsd;
  if (!lsd.eat(is)) return;

  this->width = lsd.sWidth;
  this->height = lsd.sHeight;

  ColorTable *gct = new ColorTable(1 << (lsd.sizeGTable + 1));
  if (lsd.hasGTable) {
    if (!gct->eat(is)) return;
  }

  this->gct = gct;

  GraphicCtrlExt *gExt = NULL;

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
        gExt = new GraphicCtrlExt;
        gExt->eat(is);
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

      Frame *frame = new Frame;
      frame->id = frameCount;

      if (gExt) {
        frame->graphExt = gExt;
        gExt = NULL;
      }
      frame->pixels = lzw.pixels;

      frames.push_back(std::move(*frame));

      this->frameCount++;
      log("frameCount", frames.size());
      break;
    }
    case 0x3b: // terminator
      return;
    default:
      return;
    }
  }
}

Frame::Frame() : graphExt(NULL), pixels(NULL) {}
Frame::Frame(const Frame& copy) {
  graphExt = copy.graphExt;
  pixels = copy.pixels;
  id = copy.id;
}

Frame::Frame(Frame && frame) noexcept {
  graphExt = frame.graphExt;
  pixels = frame.pixels;
  id = frame.id;
  frame.graphExt = NULL;
  frame.pixels = NULL;
}

Frame::~Frame() {
  if (graphExt) {
    delete graphExt;
  }
  if (pixels) {
    delete[] pixels;
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
  log("size", size);
  u1 *pixels = (result->frames).at(20).pixels;
  for (u4 i = 0; i < size; i++) {
    int index = pixels[i];

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

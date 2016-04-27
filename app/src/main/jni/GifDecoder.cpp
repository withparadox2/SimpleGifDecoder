#include "GifDecoder.h"
#include <time.h>
using std::string;

int main() {
  string file("earth.gif");
  ifstream is(file.c_str(), ifstream::binary);
  if (is.is_open()) {

    GifDecoder decoder;
    DataWrapper *data = new DataWrapper(is);

    decoder.dataWrapper = data;
    decoder.processStream(*data);
    exportGifToTxt(&decoder);
    is.close();
  }
}



//def class ByteEater
bool ByteEater::eat(DataWrapper& is) {}

ByteEater::~ByteEater() {};

bool ByteEater::skip(DataWrapper& is) {
  return is.skipBlock();
}

bool ByteEater::readu1(DataWrapper& is, u1 *dest) {
  char temp;
  bool result = is.read(&temp, 1);
  *dest = temp;
  return result;
}

u2 ByteEater::convertu2(char* bytes) {
  return bytes[1] << 8 | (u1)bytes[0];
}

//def DataWrapper
DataWrapper::DataWrapper(ifstream& is) : curPos(0) {
  is.seekg(0, is.end);
  int length = is.tellg();
  is.seekg(0, is.beg);
  this->data = new char[length];
  this->dataLen = length;
  is.read(this->data, length);
}

DataWrapper::DataWrapper(char *dataByte, int length) : curPos(0), data(dataByte), dataLen(length) {}

bool DataWrapper::read(char* data, int length) {
  if (!checkRange(this->curPos + length - 1)) {
    return false;
  }

  for (int i = 0; i < length; i++) {
    data[i] = (this->data)[this->curPos];
    this->curPos ++;
  }

  return true;
}

bool DataWrapper::seekg (int off, ifstream::seekdir way) {
  if (way == ifstream::cur) {
    if (!checkRange(off + this->curPos)) {
      return false;
    }
    this->curPos = off + this->curPos;
  } else if (way == ifstream::beg) {
    if (!checkRange(off)) {
      return false;
    }
    this->curPos = off;
  } else if (way == ifstream::end) {
    if (!checkRange(this->dataLen - 1 + off)) {
      return false;
    }
    this->curPos = this->dataLen - 1 + off;
  } else {
    //todo throw exception
    return false;
  }
  return true;
}

bool DataWrapper::skipBlock() {
  char dataSize;
  for (;;) {
    if (!this->read(&dataSize, 1)) {
      return false;
    }
    u4 pos = (u1)dataSize;
    if (pos > 0) {
      this->seekg(pos, ifstream::cur);
    } else {
      break;
    }
  }
  return true;
}

//TODO check range
void DataWrapper::setCurPos(int pos) {
  this->curPos = pos;
}

int DataWrapper::getCurPos() {
  return curPos;
}

DataWrapper::~DataWrapper() {
  if (!data) {
    delete[] data;
  }
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

bool LSD::eat(DataWrapper& is) {
  char *dst = new char[7];
  array_ptr<char> dstMgr(dst);
  if (!is.read(dst, 7)) return false;
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

bool ColorTable::eat(DataWrapper& is) {
  return is.read(colors, tableSize * 3);
}

u1 ColorTable::red(int index) const {
  return colors[3 * index];
}
u1 ColorTable::green(int index) const {
  return colors[3 * index + 1];
}
u1 ColorTable::blue(int index) const {
  return colors[3 * index + 2];
}
ColorTable::~ColorTable() {
  delete[] colors;
}

//def class AppExtBlock
bool AppExtBlock::eat(DataWrapper& is) {
  char blockSize;//11
  if (!is.read(&blockSize, 1)) return false;
  // log("blockSize", (int)blockSize);
  char* block = new char[blockSize + 1];
  array_ptr<char> arrayMgr(block);
  block[blockSize] = 0;
  if (!is.read(block, blockSize)) {
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
bool GraphicCtrlExt::eat(DataWrapper& is) {
  char blockSize;
  if (!is.read(&blockSize, 1)) return false;
  char* block = new char[blockSize];
  array_ptr<char> arrayMgr(block);

  if (!is.read(block, blockSize)) return false;
  char packetFields = block[0];
  transparency = (packetFields & 1) != 0;
  delay = convertu2(block + 1) * 10;
  transparencyIndex = block[3];
  //todo : handle dispose
  skip(is);
}

//def class ImageDes
bool ImageDes::eat(DataWrapper& is) {
  char *data = new char[9];
  array_ptr<char> arrayMgr(data);
  if (!is.read(data, 9))  return false;
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
LZWDecoder::~LZWDecoder() {
  if (!pixels) {
    delete pixels;
  }
}

u1* LZWDecoder::stolenPixels() {
  u1* temp = pixels;
  pixels = NULL;
  return temp;
}

bool LZWDecoder::skipFrame(DataWrapper& is, Frame& frame) {
  is.seekg(1, ifstream::cur);
  is.skipBlock();
  frame.dataIndex = is.getCurPos();
}

bool LZWDecoder::eat(DataWrapper& is, GifDecoder& decoder) {
  int width = decoder.width;
  int height = decoder.height;
  int nPixels = width * height;

  log("size pixels", nPixels);
  pixels = new u1[nPixels];
  log("pixels addrress", *pixels);


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
      pixels[pixelsLen + dicts[code].len - 1] = (u1)dicts[code].value;
      code = dicts[code].preIndex;
    }
    pixelsLen += matchLen;
    //pixels += matchLen;
  }
  return false;
}

//def class GifDecoder
GifDecoder::GifDecoder(): gct(NULL), width(-1), height(-1) , frameCount(0), dataWrapper(NULL) {
}

GifDecoder::~GifDecoder() {
  if (gct) {
    delete gct;
  }
  if(dataWrapper) {
    delete dataWrapper;
  }
}

void GifDecoder::loadGif(const char *file) {
  ifstream is(file, ifstream::binary);
  if (is.is_open()) {
    DataWrapper *data = new DataWrapper(is);
    this->dataWrapper = data;
    processStream(*data);
    is.close();
  }
}

void GifDecoder::loadGif(char *data, int length) {
  DataWrapper *dataWrap = new DataWrapper(data, length);
  this->dataWrapper = dataWrap;
  processStream(*dataWrap);
}

void GifDecoder::decodeFrame(u2 frameIndex) {
  Frame &frame = frames.at(frameIndex);
  this->dataWrapper->setCurPos(frame.dataIndex);
  LZWDecoder lzw;
  lzw.eat(*(this->dataWrapper), *this);
  frame.pixels = lzw.stolenPixels();
  if (frameIndex == this->frameCount - 1) {
    delete this->dataWrapper;
    dataWrapper = NULL;
  }
}

u4* GifDecoder::getPixels(u2 frameIndex) {
  int count = width * height;
  int newIndex = frameIndex % frameCount;
  if (frames[newIndex].pixels == NULL) {
    this->decodeFrame(newIndex);
  }
  u1 *pixels = frames[newIndex].pixels;

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

void GifDecoder::processStream(DataWrapper& is) {
  char *header = new char[6];
  array_ptr<char> mgr(header);
  if (!is.read(header, 6)) return;
  if (strncmp("GIF89a", header, 6)) {
    std::cerr << "input file is unsupported!";
    return;
  }
  LSD lsd;
  if (!lsd.eat(is)) return;

  this->width = lsd.sWidth;
  this->height = lsd.sHeight;

  std::unique_ptr<ColorTable> ctPtr(new ColorTable(1 << (lsd.sizeGTable + 1)));
  if (lsd.hasGTable) {
    if (!ctPtr->eat(is)) return;
  }
  this->gct = ctPtr.release();

  std::unique_ptr<GraphicCtrlExt> gExtPtr;

  for (;;) {
    char blockType;
    if (!is.read(&blockType, 1)) return;
    log("blockType", (u4)(u1)blockType);
    switch ((u1)blockType) {
    case 0x21:
      char label;
      if (!is.read(&label, 1)) return;
      log("label", (u4)(u1)label);

      switch ((u1)label) {
      case 0xff: {
        AppExtBlock appExt;
        appExt.eat(is);
        break;
      }
      case 0xf9: {
        gExtPtr.reset(new GraphicCtrlExt);
        gExtPtr->eat(is);
        break;
      }
      case 0xfe:
      case 0x01:
      default:
        is.skipBlock();
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

      Frame tempFrame;
      tempFrame.graphExt = gExtPtr.release();
      tempFrame.dataIndex = is.getCurPos();
      frames.push_back(std::move(tempFrame));

      this->frameCount++;

      LZWDecoder lzw;
      lzw.skipFrame(is, tempFrame);
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

Frame::Frame() : graphExt(NULL), pixels(NULL), dataIndex(0) {}
Frame::Frame(const Frame& copy) {
  graphExt = copy.graphExt;
  pixels = copy.pixels;
  dataIndex = copy.dataIndex;
}

Frame::Frame(Frame && frame) noexcept {
  graphExt = frame.graphExt;
  pixels = frame.pixels;
  dataIndex = frame.dataIndex;
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
  const ColorTable& gct = *(result->gct);
  std::ofstream os("color.txt");
  u4 size = result->width * result -> height;

  result->decodeFrame(30);
  u1 *pixels = (result->frames).at(30).pixels;
  for (u4 i = 0; i < size; i++) {
    u4 index = pixels[i];
    u4 red = (u4)gct.red(index);
    u4 green = (u4)gct.green(index);
    u4 blue = (u4)gct.blue(index);
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

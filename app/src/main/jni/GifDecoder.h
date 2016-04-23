#ifndef _GIFDECODER_H_
#define _GIFDECODER_H_
#include <vector>
#include <fstream>
#include <iostream>
#include <cassert>
#include "common.h"
#include <string.h>
#include <memory>


using std::cout;
using std::endl;
using std::ifstream;

class DataWrapper;
template <typename T> void log(const char *name, T value);

class DataWrapper {
public:
  DataWrapper(ifstream& is);
  DataWrapper(char *dataArray, int length);
  ~DataWrapper();
  bool read(char* data, int length);
  bool seekg (int off, ifstream::seekdir way);
  bool skipBlock();

private:
  bool checkRange(int pos) {
    return 0 <= pos && pos < dataLen;
  }
  char* data;
  int dataLen;
  int curPos;
};


class ByteEater {
public:
  virtual bool eat(DataWrapper& is);
  virtual ~ByteEater();
  bool skip(DataWrapper& is);
  bool readu1(DataWrapper& is, u1 *dest);
  u2 convertu2(char* bytes);
};


class LSD : public ByteEater {
public:
  void resolvePacketFields(char& fileds);
  bool eat(DataWrapper& is);
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
  bool eat(DataWrapper& is);
  char *colors;
  u1 red(int index) const;
  u1 green(int index) const;
  u1 blue(int index) const;
  ~ColorTable();
};

class AppExtBlock : ByteEater {
public:
  bool eat(DataWrapper& is);
};

class GraphicCtrlExt : ByteEater {
public:
  bool eat(DataWrapper& is) ;
  u1 delay;
  bool transparency;
  u1 transparencyIndex;
};

class ImageDes : ByteEater {
public:
  bool eat(DataWrapper& is);
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
  LZWDecoder(LSD& lsd_p, ImageDes& imgDes_p);
  ~LZWDecoder();
  bool eat(DataWrapper& is);
  u1* stolenPixels();
private:
  LSD& lsd;
  ImageDes& imgDes;
  u1 *pixels;
};

class Frame {
public:
  Frame();
  Frame(const Frame&);
  Frame(Frame&&) noexcept;
  ~Frame();
  GraphicCtrlExt* graphExt;
  u1* pixels;
};

class GifDecoder {
public:
  GifDecoder();
  ~GifDecoder();
  void loadGif(const char *file);
  void loadGif(char *data, int length);
  void processStream(DataWrapper& is);

  std::vector<Frame> frames;
  u2 frameCount;

  int width;
  int height;

  u4* getPixels(u2 index) ;
  ColorTable *gct;

  int getFrameDelay(u2 index);
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
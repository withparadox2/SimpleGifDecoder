#include <jni.h>
#include <android/bitmap.h>
#include "GifDecoder.h"

#define JNIREG_CLASS "com/withparadox2/simplegifdecoder/GifDrawable"

void getFrame(JNIEnv* env, jclass clazz, jlong handle, jint index, jobject jBmpObj) {
  GifDecoder *decoder =  ((GifDecoder*)handle);

  void* bitmapPixels;

  if (AndroidBitmap_lockPixels(env, jBmpObj, &bitmapPixels) < 0) {
    return;
  }

  if (decoder->width > 0 && decoder->height > 0) {
    int pixelsCount = decoder->width * decoder->height;
    array_ptr<u4> pixels(decoder->getPixels(index));
    memcpy(bitmapPixels, pixels.get(), pixelsCount * 4);
  }

  AndroidBitmap_unlockPixels(env, jBmpObj);
}

jlong decodeGif(JNIEnv* env, jclass clazz, jbyteArray array) {
  char *data = env->GetByteArrayElements(array, NULL);
  int length = (int) env->GetArrayLength(array);
  GifDecoder *decoder = new GifDecoder();
  decoder->loadGif(data, length);
  return decoder;
}

jlong loadGif(JNIEnv* env, jclass clazz, jstring fileName) {
  const char* fileNameChars = env->GetStringUTFChars(fileName, 0);
  GifDecoder *decoder = new GifDecoder();
  decoder->loadGif(fileNameChars);
  env->ReleaseStringUTFChars(fileName, fileNameChars);
  return decoder;
}

jint getFrameCount(JNIEnv* env, jclass clazz, jlong handle) {
  return ((GifDecoder*)handle)->frameCount;
}

jint getFrameDelay(JNIEnv* env, jclass clazz, jlong handle, jint index) {
  return ((GifDecoder*)handle)->getFrameDelay(index);
}

void onFinalize(JNIEnv* env, jclass clazz, jlong handle) {
  delete (void*)handle;
}

jint getWidth(JNIEnv* env, jclass clazz, jlong handle) {
  return ((GifDecoder*)handle)->width;
}

jint getHeight(JNIEnv* env, jclass clazz, jlong handle) {
  return ((GifDecoder*)handle)->height;
}


static JNINativeMethod gMethods[] = {
  { "getFrame",
    "(JILandroid/graphics/Bitmap;)V", (void*) getFrame
  },
  { "loadGif",
    "(Ljava/lang/String;)J", (void*)  loadGif
  },
  { "getFrameCount",
    "(J)I", (void*)  getFrameCount
  },
  { "getFrameDelay",
    "(JI)I", (void*)  getFrameDelay
  },
  { "onFinalize",
    "(J)V", (void*)  onFinalize
  },
  { "decodeGif",
    "([B)J", (void*)  decodeGif
  },
  { "getWidth",
    "(J)I", (void*)  getWidth
  },
  { "getHeight",
    "(J)I", (void*)  getHeight
  }
};
int registerNativeMethods(JNIEnv* env, const char* className, JNINativeMethod* gMethods, int numOfMethods)
{
  jclass clazz;
  clazz = env->FindClass(className);
  if (clazz == NULL) {
    return JNI_FALSE;
  }
  if (env->RegisterNatives(clazz, gMethods, numOfMethods) < 0) {
    return JNI_FALSE;
  }
  return JNI_TRUE;
}

int registerNatives(JNIEnv* env) {
  if (!registerNativeMethods(env, JNIREG_CLASS, gMethods,
                             sizeof(gMethods) / sizeof(gMethods[0])))
    return JNI_FALSE;

  return JNI_TRUE;
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
  JNIEnv* env = NULL;
  jint result = -1;
  if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
    return -1;
  }
  if (!registerNatives(env)) {
    return -1;
  }
  return JNI_VERSION_1_4;
}
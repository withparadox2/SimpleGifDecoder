#include <jni.h>
#include <android/bitmap.h>
#include "GifDecoder.h"

#define JNIREG_CLASS "com/withparadox2/simplegifdecoder/MainActivity"

jobject getGifBitmap(JNIEnv* env, jclass clazz, jstring fileName) {
  const char* fileNameChars = env->GetStringUTFChars(fileName, 0);
  GifDecoder *decoder = new GifDecoder();
  decoder->loadGif(fileNameChars);
  env->ReleaseStringUTFChars(fileName, fileNameChars);
  LOGD("width = %i, height = %i", decoder->width, decoder->height);
  // Creaing Bitmap Config Class
  jclass bmpCfgCls = env->FindClass("android/graphics/Bitmap$Config");
  jmethodID bmpClsValueOfMid = env->GetStaticMethodID(bmpCfgCls, "valueOf", "(Ljava/lang/String;)Landroid/graphics/Bitmap$Config;");
  jobject jBmpCfg = env->CallStaticObjectMethod(bmpCfgCls, bmpClsValueOfMid, env->NewStringUTF("ARGB_8888"));

  // Creating a Bitmap Class
  int imgWidth = decoder->width;
  int imgHeight = decoder->height;

  jclass bmpCls = env->FindClass("android/graphics/Bitmap");
  jmethodID createBitmapMid = env->GetStaticMethodID(bmpCls, "createBitmap", "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
  jobject jBmpObj = env->CallStaticObjectMethod(bmpCls, createBitmapMid, imgWidth, imgHeight, jBmpCfg);

  void* bitmapPixels;

  if (AndroidBitmap_lockPixels(env, jBmpObj, &bitmapPixels) < 0) {
    return 0;
  }

  //uint32_t *re = new uint32_t[1];
  //re[0] = 0xff000088;
  //memcpy(bitmapPixels, re, 4);

  if (decoder->width > 0 && decoder->height > 0) {
    int pixelsCount = decoder->width * decoder->height;
    //LOGD("before get result");
    // decoder->getPixels();
    // LOGD("pixels count = %i", pixelsCount);
    memcpy(bitmapPixels, decoder->getPixels(), pixelsCount * 4);
  }
  return jBmpObj;
}


static JNINativeMethod gMethods[] = {
  { "getGifBitmap",
    "(Ljava/lang/String;)Ljava/lang/Object;", (void*) getGifBitmap
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
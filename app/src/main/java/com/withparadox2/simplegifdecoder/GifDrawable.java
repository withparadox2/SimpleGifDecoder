package com.withparadox2.simplegifdecoder;

import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import java.io.File;
import java.io.IOException;
import java.lang.ref.WeakReference;

public class GifDrawable extends Drawable {

  private InvalidateHandler handler;
  private Bitmap bitmap;
  private long handle;
  private int count;
  private int index;

  public GifDrawable(File file) {
    if (file.exists()) {
      handle = loadGif(file.getAbsolutePath());
      startShowGif();
    }
  }

  public GifDrawable(AssetManager manager, String name) {
    try {
      loadByFd(manager.openFd(name));
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  public GifDrawable(Resources res, int id) {
    try {
      loadByFd(res.openRawResourceFd(id));
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  private void loadByFd(AssetFileDescriptor descriptor) throws IOException {
    byte[] bytes =
        Utils.readByteArray(descriptor.createInputStream(), (int) descriptor.getLength());
    handle = decodeGif(bytes);
    startShowGif();
  }

  private void startShowGif() {
    handler = new InvalidateHandler(this);
    count = getFrameCount(handle);
    handler.post(action);
  }

  Runnable action = new Runnable() {
    @Override public void run() {
      bitmap = (Bitmap) getFrame(handle, index);
      index++;
      if (index >= count) {
        index = 0;
      }
      invalidateSelf();
    }
  };

  static {
    System.loadLibrary("simplegif");
  }

  @Override public void draw(Canvas canvas) {
    if (bitmap != null && !bitmap.isRecycled()) {
      canvas.drawBitmap(bitmap, 0, 0, null);
      if (count > 1) {
        handler.removeCallbacks(action);
        handler.postDelayed(action, getFrameDelay(handle, index));
      }
    }
  }

  @Override public void setAlpha(int alpha) {

  }

  @Override public void setColorFilter(ColorFilter colorFilter) {

  }

  @Override public int getOpacity() {
    return 0;
  }

  static class InvalidateHandler extends Handler {
    WeakReference<GifDrawable> drawableRef;

    public InvalidateHandler(GifDrawable drawable) {
      super(Looper.getMainLooper());
      drawableRef = new WeakReference<GifDrawable>(drawable);
    }

    @Override public void handleMessage(Message msg) {
      GifDrawable drawable = drawableRef.get();
      if (drawable != null) {
        drawable.invalidateSelf();
      }
    }
  }

  private void destroyNativeObj() {
    if (handle != 0) {
      onFinalize(handle);
    }
  }

  public native static long decodeGif(byte[] data);

  public native static long loadGif(String path);

  public native static Object getFrame(long handle, int index);

  public native static int getFrameCount(long handle);

  public native static int getFrameDelay(long handle, int index);

  public native static void onFinalize(long handle);

  @Override protected void finalize() throws Throwable {
    super.finalize();
    destroyNativeObj();
  }
}

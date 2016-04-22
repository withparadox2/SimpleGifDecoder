package com.withparadox2.simplegifdecoder;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import java.io.File;
import java.lang.ref.WeakReference;

public class GifDrawable extends Drawable {

  private InvalidateHandler handler;
  private boolean isAnimate = true;
  private Bitmap bitmap;
  private long handle;
  private int count;
  private boolean isReady = false;

  public GifDrawable(File file) {
    handler = new InvalidateHandler(this);
    if (file.exists()) {
      handle = loadGif(file.getAbsolutePath());
      count = getFrameCount(handle);
      isReady = true;
      handler.post(action);
    }
  }

  int index;
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

  public native static long loadGif(String path);

  public native static Object getFrame(long handle, int index);

  public native static int getFrameCount(long handle);

  public native static int getFrameDelay(long handle, int index);
}

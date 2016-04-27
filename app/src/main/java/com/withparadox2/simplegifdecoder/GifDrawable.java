package com.withparadox2.simplegifdecoder;

import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapShader;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import java.io.File;
import java.io.IOException;
import java.lang.ref.WeakReference;

public class GifDrawable extends Drawable {

  private InvalidateHandler handler;
  private Bitmap bitmap;
  private long handle;
  private int count;
  private int index;
  private BitmapState bitmapState;
  private int bitmapWidth = -1;
  private int bitmapHeight = -1;

  public GifDrawable(File file) {
    if (file.exists()) {
      long start = System.currentTimeMillis();
      handle = loadGif(file.getAbsolutePath());
      long end = System.currentTimeMillis();
      Log.d("GifDrawable", "===============" + (end - start));

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
      if (bitmap != null) {
        bitmap.recycle();
      }
      bitmap = (Bitmap) getFrame(handle, index);
      bitmapHeight = bitmap.getHeight();
      bitmapWidth = bitmap.getWidth();
      confirmBitmapState();
      bitmapState.mRebuildShader = true;

      index++;
      if (index >= count) {
        index = 0;
      }
      invalidateSelf();
    }
  };

  @Override public int getIntrinsicWidth() {
    return bitmapWidth;
  }

  @Override public int getIntrinsicHeight() {
    return bitmapHeight;
  }

  public void setTileModeX(Shader.TileMode mode) {
    confirmBitmapState();
    setTileModeXY(mode, bitmapState.mTileModeY);
  }

  public void setTileModeY(Shader.TileMode mode) {
    confirmBitmapState();
    setTileModeXY(bitmapState.mTileModeX, mode);
  }

  public void setTileModeXY(Shader.TileMode xmode, Shader.TileMode ymode) {
    confirmBitmapState();
    final BitmapState state = bitmapState;
    if (state.mTileModeX != xmode || state.mTileModeY != ymode) {
      state.mTileModeX = xmode;
      state.mTileModeY = ymode;
      state.mRebuildShader = true;
      invalidateSelf();
    }
  }

  private void confirmBitmapState() {
    if (bitmapState == null) {
      bitmapState = new BitmapState();
    }
  }

  @Override public void draw(Canvas canvas) {
    if (bitmap != null && !bitmap.isRecycled()) {
      confirmBitmapState();
      BitmapState state = bitmapState;
      if (state.mTileModeY == null && state.mTileModeX == null) {
        canvas.drawBitmap(bitmap, 0, 0, null);
      } else {
        Paint paint = state.mPaint;
        if (state.mRebuildShader) {
          paint.setShader(new BitmapShader(bitmap, state.mTileModeX, state.mTileModeY));
          state.mRebuildShader = false;
        }
        canvas.drawRect(getBounds(), paint);
      }

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

  final static class BitmapState extends ConstantState {
    final Paint mPaint;

    Shader.TileMode mTileModeX = null;
    Shader.TileMode mTileModeY = null;
    boolean mRebuildShader;

    BitmapState() {
      mPaint = new Paint(Paint.FILTER_BITMAP_FLAG | Paint.DITHER_FLAG);
    }

    @Override public Drawable newDrawable() {
      return new GifDrawable(null);
    }

    @Override public int getChangingConfigurations() {
      return 0;//WTF?
    }
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

  static {
    System.loadLibrary("simplegif");
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

package com.withparadox2.simplegifdecoder;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.ImageView;
import java.io.File;

public class GifImageView extends ImageView {
  public GifImageView(Context context) {
    super(context);
  }

  public GifImageView(Context context, AttributeSet attrs) {
    super(context, attrs);
  }

  public GifImageView(Context context, AttributeSet attrs, int defStyleAttr) {
    super(context, attrs, defStyleAttr);
  }

  public void setGifDrawable(Drawable drawable) {
    drawable.setCallback(new Drawable.Callback() {
      @Override public void invalidateDrawable(Drawable who) {
        invalidate();
      }

      @Override public void scheduleDrawable(Drawable who, Runnable what, long when) {

      }

      @Override public void unscheduleDrawable(Drawable who, Runnable what) {

      }
    });
    setImageDrawable(drawable);
  }
}

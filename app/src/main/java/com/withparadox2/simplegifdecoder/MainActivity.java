package com.withparadox2.simplegifdecoder;

import android.app.Activity;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Environment;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ImageView;
import android.widget.Toast;
import java.io.File;

public class MainActivity extends Activity {

  int index = 0;

  @Override protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    final GifImageView imageView = (GifImageView) findViewById(R.id.iv_flower);
    File extStore = Environment.getExternalStorageDirectory();

    imageView.setImage(new File(extStore.getPath(), "earth.gif"));
    //File flowerFile = new File(extStore.getPath(), "earth.gif");
    //if (flowerFile.exists()) {
    //  Toast.makeText(this, "lll", Toast.LENGTH_SHORT).show();
    //  final long handle = loadGif(flowerFile.getAbsolutePath());
    //  final int count = getFrameCount(handle);
    //  imageView.postDelayed(new Runnable() {
    //    @Override public void run() {
    //      imageView.setImageBitmap((Bitmap) getFrame(handle, index));
    //      index++;
    //      if (index >= count) {
    //        index = 0;
    //      }
    //      imageView.postDelayed(this, getFrameDelay(handle, index));
    //    }
    //  }, getFrameDelay(handle, index));
    //}
  }

  static {
    System.loadLibrary("simplegif");
  }

  @Override public boolean onCreateOptionsMenu(Menu menu) {
    // Inflate the menu; this adds items to the action bar if it is present.
    getMenuInflater().inflate(R.menu.menu_main, menu);
    return true;
  }

  @Override public boolean onOptionsItemSelected(MenuItem item) {
    // Handle action bar item clicks here. The action bar will
    // automatically handle clicks on the Home/Up button, so long
    // as you specify a parent activity in AndroidManifest.xml.
    int id = item.getItemId();

    //noinspection SimplifiableIfStatement
    if (id == R.id.action_settings) {
      return true;
    }

    return super.onOptionsItemSelected(item);
  }

  public native static long loadGif(String path);

  public native static Object getFrame(long handle, int index);

  public native static int getFrameCount(long handle);

  public native static int getFrameDelay(long handle, int index);
}

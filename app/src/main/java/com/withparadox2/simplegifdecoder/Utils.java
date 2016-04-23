package com.withparadox2.simplegifdecoder;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;

public class Utils {
  public static byte[] readByteArray(File file) {
    if (!file.exists() || !file.isFile()) {
      return null;
    }
    byte[] data = null;
    FileInputStream stream = null;
    try {
      stream = new FileInputStream(file);
      data = new byte[(int) file.length()];
      stream.read(data);
    } catch (IOException e) {
      e.printStackTrace();
    } finally {
      if (stream != null) {
        try {
          stream.close();
        } catch (IOException e) {
          e.printStackTrace();
        }
      }
    }
    return data;
  }

  public static byte[] readByteArray(InputStream stream, int length) {
    byte[] data = null;
    try {
      data = new byte[(int) length];
      stream.read(data);
    } catch (IOException e) {
      e.printStackTrace();
    } finally {
      if (stream != null) {
        try {
          stream.close();
        } catch (IOException e) {
          e.printStackTrace();
        }
      }
    }
    return data;
  }
}

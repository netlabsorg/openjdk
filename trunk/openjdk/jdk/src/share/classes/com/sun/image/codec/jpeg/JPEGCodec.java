/* JPEGCodec.java --
   Copyright (C) 2007 Free Software Foundation, Inc.
   Copyright (C) 2007 Matthew Flaschen

   This file is part of GNU Classpath.

   GNU Classpath is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GNU Classpath is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Classpath; see the file COPYING.  If not, write to the
   Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA.

   Linking this library statically or dynamically with other modules is
   making a combined work based on this library.  Thus, the terms and
   conditions of the GNU General Public License cover the whole
   combination.

   As a special exception, the copyright holders of this library give you
   permission to link this library with independent modules to produce an
   executable, regardless of the license terms of these independent
   modules, and to copy and distribute the resulting executable under
   terms of your choice, provided that you also meet, for each linked
   independent module, the terms and conditions of the license of that
   module.  An independent module is a module which is not derived from
   or based on this library.  If you modify this library, you may extend
   this exception to your version of the library, but you are not
   obligated to do so.  If you do not wish to do so, delete this
   exception statement from your version. */

package com.sun.image.codec.jpeg;

import java.lang.RuntimeException;
import java.lang.UnsupportedOperationException;

import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;

import java.awt.Graphics2D;
import java.awt.image.BufferedImage;
import java.awt.image.Raster;
import java.awt.image.ColorModel;

import javax.imageio.*;
import javax.imageio.stream.*;
import javax.imageio.plugins.jpeg.*;

import java.util.Iterator;

public class JPEGCodec
{

	public static JPEGImageDecoder createJPEGDecoder(InputStream is)
	{
		return new ImageIOJPEGImageDecoder(is);
	}

	public static JPEGImageEncoder createJPEGEncoder(OutputStream os)
	{
		return new ImageIOJPEGImageEncoder(os);
	}

	public static JPEGImageDecoder createJPEGDecoder(InputStream src, JPEGDecodeParam jdp)
	{
    return new ImageIOJPEGImageDecoder(src);
	}

	public static JPEGImageEncoder createJPEGEncoder(OutputStream dest, JPEGEncodeParam jep)
	{
    return new ImageIOJPEGImageEncoder(dest);
	}

	public static JPEGEncodeParam getDefaultJPEGEncodeParam(BufferedImage bi)
  {
    throw new UnsupportedOperationException("FIX ME!");
  }

	public static JPEGEncodeParam getDefaultJPEGEncodeParam(int numBands, int colorID)
  {
    throw new UnsupportedOperationException("FIX ME!");
  }

	public static JPEGEncodeParam getDefaultJPEGEncodeParam(JPEGDecodeParam jdp)
  {
    throw new UnsupportedOperationException("FIX ME!");
  }

	public static JPEGEncodeParam getDefaultJPEGEncodeParam(Raster ras, int colorID)
  {
    throw new UnsupportedOperationException("FIX ME!");
  }


	private static class ImageIOJPEGImageDecoder implements JPEGImageDecoder
	{

		private static final String JPGMime = "image/jpeg";

		private ImageReader JPGReader;

		private InputStream in;

		private ImageIOJPEGImageDecoder (InputStream newIs)
		{
			in = newIs;

			Iterator<ImageReader> JPGReaderIter = ImageIO.getImageReadersByMIMEType(JPGMime);
			if(JPGReaderIter.hasNext())
			{
				JPGReader  = JPGReaderIter.next();
			}

			JPGReader.setInput(new MemoryCacheImageInputStream(in));
		}

		public BufferedImage decodeAsBufferedImage() throws IOException, ImageFormatException
		{
			return JPGReader.read(0);
		}

		public Raster decodeAsRaster() throws IOException, ImageFormatException
		{
			return JPGReader.readRaster(0, null);
		}

		public InputStream getInputStream()
		{
			return in;
		}

		public JPEGDecodeParam getJPEGDecodeParam()
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

		public void setJPEGDecodeParam(JPEGDecodeParam jdp)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

	}

  private static class ImageIOJPEGEncodeParam implements JPEGEncodeParam
  {
    private ImageWriter writer;
    private ImageWriteParam p;
    private int width;
    private int height;

    private ImageIOJPEGEncodeParam (ImageWriter _writer, int _width, int _height)
    {
      writer = _writer;
      p = _writer.getDefaultWriteParam();
      width = _width;
      height = _height;
    }

    private ImageWriteParam getImageWriteParam()
    {
      return p;
    }

    public void setQuality(float i, boolean b)
    {
      p.setCompressionMode(ImageWriteParam.MODE_EXPLICIT);
      p.setCompressionQuality(i);
    }

    public JPEGEncodeParam clone()
    {
      return new ImageIOJPEGEncodeParam(writer, width, height);
    }

    public void setTableInfoValid(boolean b)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public void setImageInfoValid(boolean b)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public int getHorizontalSubsampling(int i)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public int getVerticalSubsampling(int i)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public int getWidth()
    {
      return width;
    }

    public int getHeight()
    {
      return height;
    }

    public int getDensityUnit()
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public int getXDensity()
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public int getYDensity()
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public int getRestartInterval()
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public JPEGQTable getQTable(int i)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public void setDensityUnit(int i)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public void setXDensity(int i)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public void setYDensity(int i)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public void setRestartInterval(int i)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public void setQTable(int i, JPEGQTable jqt)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }
  }

  private static class ImageIOJPEGImageEncoder implements JPEGImageEncoder
  {

    private static final String JPGMime = "image/jpeg";

    private ImageWriter JPGWriter;
    private OutputStream out;
    private ImageIOJPEGEncodeParam param;

    private ImageIOJPEGImageEncoder (OutputStream newOs)
    {
      out = newOs;
      param = null;

      Iterator<ImageWriter> JPGWriterIter = ImageIO.getImageWritersByMIMEType(JPGMime);
      if(JPGWriterIter.hasNext())
      {
        JPGWriter = JPGWriterIter.next();
      }

      JPGWriter.setOutput(new MemoryCacheImageOutputStream(out));
    }

    private BufferedImage checkImage(BufferedImage bi)
    {
      boolean needsConversion = false;

      int[] bandSizes = bi.getSampleModel().getSampleSize();
      for (int i = 0; i < bandSizes.length; i++)
      {
        if (bandSizes[i] > 8)
        {
          needsConversion = true;
          break;
        }
      }

      if (needsConversion)
      {
        BufferedImage newBi = new BufferedImage(bi.getWidth(), bi.getHeight(), BufferedImage.TYPE_INT_RGB);
        Graphics2D g = newBi.createGraphics();
        g.drawImage(bi, 0, 0, null);
        bi = newBi;
      }

      return bi;
    }

    public JPEGEncodeParam getJPEGEncodeParam()
    {
      return param;
    }

    public void setJPEGEncodeParam(JPEGEncodeParam jep)
    {
      if (!(jep instanceof ImageIOJPEGEncodeParam))
        throw new RuntimeException("Must specify object returned by getDefaultJPEGEncodeParam()");
      param = (ImageIOJPEGEncodeParam)jep;
    }

    public JPEGEncodeParam getDefaultJPEGEncodeParam(BufferedImage bi)
    {
      return new ImageIOJPEGEncodeParam(JPGWriter, bi.getWidth(), bi.getHeight());
    }

    public JPEGEncodeParam getDefaultJPEGEncodeParam(Raster ras, int colorID) throws ImageFormatException
    {
      return new ImageIOJPEGEncodeParam(JPGWriter, ras.getWidth(), ras.getHeight());
    }

    public JPEGEncodeParam getDefaultJPEGEncodeParam(int numBands, int colorID) throws ImageFormatException
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public JPEGEncodeParam getDefaultJPEGEncodeParam(JPEGDecodeParam jdp) throws ImageFormatException
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public int getDefaultColorId(ColorModel cm)
    {
      throw new UnsupportedOperationException("FIX ME!");
    }

    public OutputStream getOutputStream()
    {
      return out;
    }

    public void encode(BufferedImage bi, JPEGEncodeParam p) throws IOException, ImageFormatException
    {
      if (!(p instanceof ImageIOJPEGEncodeParam))
        throw new RuntimeException("Must specify object returned by getDefaultJPEGEncodeParam()");
      bi = checkImage(bi);
      JPGWriter.write(null, new IIOImage(bi, null, null), ((ImageIOJPEGEncodeParam)p).getImageWriteParam());
    }

    public void encode(Raster ras) throws IOException, ImageFormatException
    {
      JPGWriter.write(null, new IIOImage(ras, null, null), param.getImageWriteParam());
    }

    public void encode(BufferedImage bi) throws IOException, ImageFormatException
    {
      bi = checkImage(bi);
      JPGWriter.write(null, new IIOImage(bi, null, null), param.getImageWriteParam());
    }

    public void encode(Raster ras, JPEGEncodeParam p) throws IOException, ImageFormatException
    {
      if (!(p instanceof ImageIOJPEGEncodeParam))
        throw new RuntimeException("Must specify object returned by getDefaultJPEGEncodeParam()");
      JPGWriter.write(null, new IIOImage(ras, null, null), ((ImageIOJPEGEncodeParam)p).getImageWriteParam());
    }
  }
}

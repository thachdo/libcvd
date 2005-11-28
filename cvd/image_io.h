/*                       
	This file is part of the CVD Library.

	Copyright (C) 2005 The Authors

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef CVD_IMAGE_IO_H
#define CVD_IMAGE_IO_H

#include <cvd/config.h>

#include <cvd/exceptions.h>
#include <cvd/image_convert.h>
#include <cvd/internal/load_and_save.h>
#include <cvd/internal/name_builtin_types.h>
#include <cvd/internal/name_CVD_rgb_types.h>
#include <errno.h>
#include <memory>
#include <string>
#include <fstream>

#include <pnm_src/pnm_grok.h>
#include <pnm_src/save_postscript.h>
#include <pnm_src/bmp.h>


#ifdef CVD_IMAGE_HAVE_JPEG
	#include <pnm_src/jpeg.h>
#endif

#ifdef CVD_IMAGE_HAVE_TIFF
	#include <pnm_src/tiff.h>
#endif

namespace CVD
{
	


	////////////////////////////////////////////////////////////////////////////////
	//
	// Image loading
	//

	#ifndef DOXYGEN_IGNORE_INTERNAL
	namespace ImageType
	{
		enum ImageType
		{
			PNM,
			PS,
			EPS,
			BMP,
			CVD_IMAGE_HAVE_JPEG  //This is a macro, ending in ,
		};
	}
	#endif

	#ifdef DOXYGEN_INCLUDE_ONLY_FOR_DOCS
	// This is not the real definition, but this is what it would look like if all
	// the macros were expanded. The real definition is in internal/disk_image.h
	/// Contains the enumeration of possible image types
	namespace ImageType
	{
		/// Possible image types
		enum ImageType
		{
			/// PNM image format (PBM, PGM or PPM). This is a raw image format.
			PNM, 
			/// JPEG image format. This is a compressed (lossy) image format, but defaults to 95% quality, which has very few compression artefacts. This image type is only present if libjpeg is available.
			JPEG,
			/// Postscript  format. This outputs a bare PostScript image with the coordinate system set up 
			/// to that (x,y) corresponds to pixel (x,y), with (0,0) being at the top left of the pixel (0,0).
			/// The Y axis is therefore inverted compared to normal postscript drawing, but is image aligned.
			/// To get the drawing axes aligned with the centre of the pixels, write the postscript command
			/// ".5 .5 translate" after the image.
			/// The image data in encoded in ASCII-85 for portability. Helper functions are provided for
			/// generating EPS figures. See CVD::output_eps_header and CVD::output_eps_footer
			PS,
			/// EPS format. This outputs a complete EPS (Encapsulated PostScript) figure.
			EPS,
		};
	}
	#endif
	
	/// Load an image from a stream. This function resizes the Image as necessary.
	/// It will also perform image type conversion (e.g. colour to greyscale)
	/// according the Pixel:::CIE conversion.
	/// @param I The pixel type of the image
	/// @param im The image to receive the loaded image data
	/// @param i The stream
	/// @ingroup gImageIO
	template<class I> void img_load(Image<I>& im, std::istream& i)
	{
	  if(!i.good())
	  {
	    //Check for one of the commonest errors and put in
	    //a special case
	    std::ifstream* fs;
	    if((fs = dynamic_cast<std::ifstream*>(&i)) && !fs->is_open())
	      throw Exceptions::Image_IO::IfstreamNotOpen();
	    else
	      throw Exceptions::Image_IO::EofBeforeImage();
	  }
	  unsigned char c = i.peek();
	  
	  if(!i.good())
	    throw Exceptions::Image_IO::EofBeforeImage();
	  	  
	  if(c == 'P')
	    PNM::readPNM(im, i);
#ifdef CVD_IMAGE_HAVE_JPEG
	  else if(c == 0xff)
	    JPEG::readJPEG(im, i);
#endif
#ifdef CVD_IMAGE_HAVE_TIFF
	  else if(c == 'I')
	    TIFF::readTIFF(im, i);
#endif
	  else if(c == 'B')
	    BMP::readBMP(im, i);
	  else
	    throw Exceptions::Image_IO::UnsupportedImageType();
	}
	
	//  syg21
	template<class I> void img_load(Image<I> &im, const std::string &s)
	{
	  std::ifstream i(s.c_str(), std::ios::in|std::ios::binary);

	  if(!i.good())
	    throw Exceptions::Image_IO::OpenError(s, "for reading", errno);
	  img_load(im, i);
	}	


	////////////////////////////////////////////////////////////////////////////////
	//
	// Image saving
	//
	#ifndef DOXYGEN_IGNORE_INTERNAL
	namespace Internal
	{
		template<class C, int i> struct save_default_
		{
			static const bool use_16bit=1;
		};

		template<class C> struct save_default_<C,1>
		{
			static const bool use_16bit=Pixel::traits<typename Pixel::Component<C>::type>::bits_used > 8;
		};

		template<class C> struct save_default
		{
			static const bool use_16bit = save_default_<C, Pixel::traits<typename Pixel::Component<C>::type>::integral>::use_16bit;
		};
	}
	#endif	



	/// Save an image to a stream. This function will convert types if necessary.
	/// @param PixelType The pixel type of the image
	/// @param Conversion The conversion class to use
	/// @param im The image to save
	/// @param o The stream 
	/// @param t The image file format to use (see ImageType::ImageType for a list of supported formats)
	/// @param cv The image instance conversion to use, if necessary (see Pixel for a list of common conversions)
	/// @param channels dunno
	/// @param use_16bit dunno
	/// @ingroup gImageIO
	template<class PixelType> 
	void img_save(const BasicImage<PixelType>& im, std::ostream& o, ImageType::ImageType t)
	{
	  switch (t) {
	  case ImageType::PNM:  PNM::writePNM(im, o); break;
	  case ImageType::JPEG: JPEG::writeJPEG(im,o); break;
	  case ImageType::PS:   PS::writePS(im, o);  break;
	  case ImageType::EPS:  PS::writeEPS(im,o);  break;
	  case ImageType::BMP:  BMP::writeBMP(im,o);  break;
	  }
	}

	template<class PixelType> void img_save(const BasicImage<PixelType>& im, const std::string& name, ImageType::ImageType t)
	{
	  std::ofstream out(name.c_str(), std::ios::out|std::ios::binary);
	  if(!out.good())
	    throw Exceptions::Image_IO::OpenError(name, "for writing", errno);
	  img_save(im, out, t);
	}

	template<class PixelType> void img_save(const BasicImage<PixelType>& im, const std::string& name)
	{
	  size_t dot = name.rfind('.');
	  if (dot == std::string::npos) {
	    img_save(im, name, ImageType::PNM);
	    return;
	  }
	  
	  std::string suffix = name.substr(dot+1,name.length()-dot-1);
	  for (size_t i=0; i<suffix.length(); i++)
	    suffix[i] = tolower(suffix[i]);

	  if (suffix == "jpg") {
	    img_save(im, name, ImageType::JPEG);
	  } else if (suffix == "ps") {
	    img_save(im, name, ImageType::PS);
	  } else if (suffix == "eps") {
	    img_save(im, name, ImageType::EPS);
	  } else if (suffix == "bmp") {
	    img_save(im, name, ImageType::BMP);
	  } else {
	    img_save(im, name, ImageType::PNM);
	  }
	}

	////////////////////////////////////////////////////////////////////////////////
	//
	// Legacy pnm_* functions
	//

	/// Save an image to a stream as a PNM. 
	/// @b Deprecated Use img_save() instead, i.e. <code>img_save(im, o, ImageType::PNM);</code>
	/// @param PixelType The pixel type of the image
	/// @param im The image
	/// @param o The stream
	/// @ingroup gImageIO
	template<class PixelType> void pnm_save(const BasicImage<PixelType>& im, std::ostream& o)
	{
		img_save(im, o, ImageType::PNM);
	}

	/// Load a PNM image from a stream.
	/// @b Deprecated Use img_load(Image<I>& im, std::istream& i) instead. This can handle 
	/// and automatically detect other file formats as well.
	/// @param PixelType The pixel type of the image
	/// @param im The image
	/// @param i The stream
	/// @ingroup gImageIO
	template<class PixelType> void pnm_load(Image<PixelType>& im, std::istream& i)
	{
		img_load(im, i);
	}

	////////////////////////////////////////////////////////////////////////////////
	//
	// Postscript helper functions
	//

	/// Outputs an EPS footer to an ostream
	/// @param o the ostream
	/// @ingroup gImageIO
	void output_eps_footer(std::ostream& o);

	/// Outputs an EPS header to an ostream. Typical use is to output the header,
	/// save a raw PS image, output some other PS (eg annotations) and the output
	/// the EPS footer.
	/// @param o the ostream
	/// @param xs the width of the image
	/// @param ys the height of the image
	/// @ingroup gImageIO
	void output_eps_header(std::ostream& o, int xs, int ys);


	/// Outputs an EPS header to an ostream. Typical use is to output the header,
	/// save a raw PS image, output some other PS (eg annotations) and the output
	/// the EPS footer.
	/// @param o the ostream
	/// @param s size  of the image
	/// @ingroup gImageIO
	void output_eps_header(std::ostream & o, const ImageRef& s);

	/// Outputs an EPS header to an ostream. Typical use is to output the header,
	/// save a raw PS image, output some other PS (eg annotations) and the output
	/// the EPS footer.
	/// @param o the ostream
	/// @param im the image
	/// @ingroup gImageIO
	template<class PixelType> void output_eps_header(std::ostream& o, const BasicImage<PixelType>& im)
	{
		output_eps_header(o, im.size());
	}

}

#endif

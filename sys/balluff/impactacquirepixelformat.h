/* GStreamer
 * @copyright: Copyright [2023] Balluff GmbH, all rights reserved
 * @description: Header file of impactacquiresrc pixel formats.
 * @authors: Danxuan Zhu <danxuan.zhu@balluff.de>
 * @initial data: 2023-11-15
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _IMPACTACQUIRE_PIXELFORMAT_H_
#define _IMPACTACQUIRE_PIXELFORMAT_H_

#include "common/genicampixelformat.h"

typedef struct
{
  const char *pixel_format;
  const char *image_destination_pixel_format;
  int endianness;
  const char *gst_pixel_format;
  const char *gst_caps_string;
  int bpp;
  int depth;
  int row_multiple;
} GstImpactAcquirePixelFormatInfo;

GstImpactAcquirePixelFormatInfo gst_impactacquire_pixel_format_infos[] = {
  {"Mono8", "Mono8", 0, "GRAY8", GST_VIDEO_CAPS_MAKE ("GRAY8"), 8, 8, 4}
  ,
/*
  {"Mono10", "Mono10", G_LITTLE_ENDIAN, "GRAY16_LE", GST_VIDEO_CAPS_MAKE( "GRAY16_LE" ), 10, 16, 4}
  ,
  {"Mono10", "Mono10", G_BIG_ENDIAN, "GRAY16_BE", GST_VIDEO_CAPS_MAKE( "GRAY16_BE" ), 10, 16, 4}
  ,
  {"Mono12", "Mono12", G_LITTLE_ENDIAN, "GRAY16_LE", GST_VIDEO_CAPS_MAKE( "GRAY16_LE" ), 12, 16, 4}
  ,
  {"Mono12", "Mono12", G_BIG_ENDIAN, "GRAY16_BE", GST_VIDEO_CAPS_MAKE( "GRAY16_BE" ), 12, 16, 4}
  ,
  {"Mono14", "Mono14", G_LITTLE_ENDIAN, "GRAY16_LE", GST_VIDEO_CAPS_MAKE( "GRAY16_LE" ), 14, 16, 4}
  ,
  {"Mono14", "Mono14", G_BIG_ENDIAN, "GRAY16_BE", GST_VIDEO_CAPS_MAKE( "GRAY16_BE" ), 14, 16, 4}
  ,
*/
  {"Mono16", "Mono16", G_LITTLE_ENDIAN, "GRAY16_LE", GST_VIDEO_CAPS_MAKE ("GRAY16_LE"), 16, 16, 4}
  ,
  {"Mono16", "Mono16", G_BIG_ENDIAN, "GRAY16_BE", GST_VIDEO_CAPS_MAKE ("GRAY16_BE"), 16, 16, 4}
  ,
  {"BayerRG8", "Raw", 0, "rggb", GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER8 ("rggb"), 8, 8, 1}
  ,
/*
  {"BayerRG10", "Raw", 0, "rggb16", GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER16 ("rggb16", "1234"), 10, 16, 1}
  ,
  {"BayerRG12", "Raw", 0, "rggb16", GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER16 ("rggb16", "1234"), 12, 16, 1}
  ,
  {"BayerRG14", "Raw", 0, "rggb16", GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER16 ("rggb16", "1234"), 14, 16, 1}
  ,
*/
  {"BayerRG16", "Raw", 0, "rggb16", GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER16 ("rggb16", "1234"), 16, 16, 1}
  ,
  {"BayerGR8", "Raw", 0, "grbg", GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER8 ("grbg"), 8, 8, 1}
  ,
/*
  {"BayerGR10", "Raw", 0, "grbg16", GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER16 ("grbg16", "1234"), 10, 16, 1}
  ,
  {"BayerGR12", "Raw", 0, "grbg16", GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER16 ("grbg16", "1234"), 12, 16, 1}
  ,
  {"BayerGR14", "Raw", 0, "grbg16", GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER16 ("grbg16", "1234"), 14, 16, 1}
  ,
*/
  {"BayerGR16", "Raw", 0, "grbg16", GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER16 ("grbg16", "1234"), 16, 16, 1}
  ,
  {"RGB8", "BGR888Packed", 0, "RGB", GST_VIDEO_CAPS_MAKE ("RGB"), 24, 24, 4}
  ,
  {"RGB8Packed", "BGR888Packed", 0, "RGB", GST_VIDEO_CAPS_MAKE ("RGB"), 24, 24, 4}
  ,
  {"BGR8", "RGB888Packed", 0, "BGR", GST_VIDEO_CAPS_MAKE ("BGR"), 24, 24, 4}
  ,
  {"BGR8Packed", "RGB888Packed", 0, "BGR", GST_VIDEO_CAPS_MAKE ("BGR"), 24, 24, 4}
  ,
  {"BGRa8", "RGBx888Packed", 0, "BGRA", GST_VIDEO_CAPS_MAKE ("BGRA"), 32, 32, 4}
  ,
  {"BGRA8Packed", "RGBx888Packed", 0, "BGRA", GST_VIDEO_CAPS_MAKE ("BGRA"), 32, 32, 4}
  ,
  {"BGRa8", "RGBx888Packed", 0, "BGRx", GST_VIDEO_CAPS_MAKE ("BGRx"), 32, 32, 4}
  ,
  {"BGRA8Packed", "RGBx888Packed", 0, "BGRx", GST_VIDEO_CAPS_MAKE ("BGRx"), 32, 32, 4}
  ,
  {"YUV422Packed", "YUV422_UYVYPacked", 0, "UYVY", GST_VIDEO_CAPS_MAKE ("UYVY"), 16, 16, 4}
  ,
  {"YUV422_YUYVPacked", "YUV422_UYVYPacked", 0, "UYVY", GST_VIDEO_CAPS_MAKE ("UYVY"), 16, 16, 4}
  ,
  {"YUV8_UYV", "YUV444_UYVPacked", 0, "IYU2", GST_VIDEO_CAPS_MAKE ("IYU2"), 24, 24, 4}
  ,
  {"YUV444Packed", "YUV444_UYVPacked", 0, "IYU2", GST_VIDEO_CAPS_MAKE ("IYU2"), 24, 24, 4}
  ,
  {"YUV422_8", "YUV422_UYVYPacked", 0, "UYVY", GST_VIDEO_CAPS_MAKE ("UYVY"), 16, 16, 4}
  ,
  {"YUV422_8_UYVY", "YUV422_UYVYPacked", 0, "UYVY", GST_VIDEO_CAPS_MAKE ("UYVY"), 16, 16, 4}
};

static GstCaps *
gst_impactacquire_pixel_format_caps_from_pixel_format_var (int index, int endianness, int width, int height)
{
  const char *caps_string;
  GstCaps *caps;
  GstStructure *structure;

  const GstImpactAcquirePixelFormatInfo *info = &gst_impactacquire_pixel_format_infos[index];

  GST_DEBUG ("Trying to create caps from: %s, endianness=%d, %dx%d", info->gst_pixel_format, endianness, width, height);

  caps_string = info->gst_caps_string;
  if (caps_string == NULL) {
    return NULL;
  }

  GST_DEBUG ("Got caps string: %s", caps_string);

  structure = gst_structure_from_string (caps_string, NULL);
  if (structure == NULL) {
    return NULL;
  }

  gst_structure_set (structure,
      "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height, "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);

  if (g_str_has_prefix (info->pixel_format, "Bayer")) {
    g_assert (info);
    gst_structure_set (structure, "bpp", G_TYPE_INT, (gint) info->bpp, NULL);
  }

  caps = gst_caps_new_empty ();
  gst_caps_append_structure (caps, structure);

  return caps;
}

#endif

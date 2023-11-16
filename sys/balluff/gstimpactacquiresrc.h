/* GStreamer
 * @copyright: Copyright [2023] Balluff GmbH, all rights reserved
 * @description: Header file of the impactacquiresrc element.
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

#ifndef _GST_IMPACTACQUIRESRC_H_
#define _GST_IMPACTACQUIRESRC_H_

#include <gst/base/gstpushsrc.h>
#include <mvDeviceManager/Include/mvDeviceManager.h>

enum
{
  GST_IMPACTACQUIRESRC_INFO_STRING_SIZE = 38
};

G_BEGIN_DECLS
#define GST_TYPE_IMPACTACQUIRESRC   (gst_impactacquiresrc_get_type())
#define GST_IMPACTACQUIRESRC(obj)   (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_IMPACTACQUIRESRC,GstImpactAcquireSrc))
#define GST_IMPACTACQUIRESRC_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_IMPACTACQUIRESRC,GstImpactAcquireSrcClass))
#define GST_IS_IMPACTACQUIRESRC(obj)   (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_IMPACTACQUIRESRC))
#define GST_IS_IMPACTACQUIRESRC_CLASS(obj)   (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_IMPACTACQUIRESRC))
typedef struct _GstImpactAcquireSrc GstImpactAcquireSrc;
typedef struct _GstImpactAcquireSrcClass GstImpactAcquireSrcClass;

struct _GstImpactAcquireSrc
{
  GstPushSrc base_impactacquiresrc;

  GstCaps *caps;

  gint deviceId;
  gchar *serialNumber;
  HDEV deviceHandle;
  HDRV driverHandle;
  gboolean deviceConnected;
  gboolean boAcquisitionRunning;
  guint64 frameNumber;
  gint failedFrames;
  gint width;
  gint height;
  gint offsetx;
  gint offsety;
  gchar *dest_pixel_format;
  gchar *src_pixel_format;
  gchar *gst_pixel_format;
  gboolean boDebayerOnHost;
  double exposureTime;
  double mvExposureAutoUpperLimit;
  double mvExposureAutoLowerLimit;
  gchar *exposureAuto;
  double gain;
  gchar *gainAuto;
  gchar *configurationFilePath;
  double fps;
};

struct _GstImpactAcquireSrcClass
{
  GstPushSrcClass base_impactacquiresrc_class;
};

GType gst_impactacquiresrc_get_type (void);

G_END_DECLS
#endif

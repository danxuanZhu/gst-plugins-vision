/* GStreamer
 * @copyright: Copyright [2023] Balluff GmbH, all rights reserved
 * @description: Balluff Impact Acquire GenICam GStreamer Plugin
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstimpactacquiresrc
 *
 * The impactacquiresrc element uses Balluff's Impact Acquire API to get video from GenTL compliant devices.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 impactacquiresrc ! 'video/x-raw, format=GRAY8' ! queue ! autovideosink
 * ]|
 * Outputs device output to screen.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstimpactacquiresrc.h"
#include <gst/gst.h>
#include <glib.h>
#include "impactacquirepixelformat.h"
#include <apps/Common/exampleHelper_C.h>
#include <mvPropHandling/Include/mvPropHandlingDatatypes.h>

/* debug category */
GST_DEBUG_CATEGORY_STATIC (gst_impactacquiresrc_debug_category);
#define GST_CAT_DEFAULT gst_impactacquiresrc_debug_category

/* prototypes */
static void gst_impactacquiresrc_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_impactacquiresrc_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void gst_impactacquiresrc_dispose (GObject * object);
static void gst_impactacquiresrc_finalize (GObject * object);
static gboolean gst_impactacquiresrc_start (GstBaseSrc * bsrc);
static gboolean gst_impactacquiresrc_stop (GstBaseSrc * bsrc);
static GstCaps *gst_impactacquiresrc_get_caps (GstBaseSrc * bsrc, GstCaps * filter);
static gboolean gst_impactacquiresrc_set_caps (GstBaseSrc * bsrc, GstCaps * caps);
static GstFlowReturn gst_impactacquiresrc_create (GstPushSrc * bsrc, GstBuffer ** buf);
static void gst_impactacquiresrc_update_caps (GstImpactAcquireSrc * src);

static HDMR hDMR = INVALID_ID;

// todo: should be replaced by device's default values. See ACQ-4735
/* parameters */
typedef enum _GST_IMPACTACQUIRESRC_PROP
{
  PROP_0,
  PROP_CAMERA,
  PROP_SERIAL,
  PROP_HEIGHT,
  PROP_WIDTH,
  PROP_OFFSETX,
  PROP_OFFSETY,
  PROP_FPS,
  PROP_DESTINATION_PIXEL_FORMAT,
  PROP_SOURCE_PIXEL_FORMAT,
  PROP_DEBAYER_ON_HOST,
  PROP_EXPOSURETIME,
  PROP_EXPOSUREAUTO,
  PROP_MVEXPOSUREAUTOUPPERLIMIT,
  PROP_MVEXPOSUREAUTOLOWERLIMIT,
  PROP_CONFIGURATION_FILE_PATH,
  PROP_GAIN,
  PROP_GAINAUTO
} GST_IMPACTACQUIRESRC_PROP;

#define DEFAULT_PROP_CAMERA                           0
#define DEFAULT_PROP_SERIAL                           ""
#define DEFAULT_PROP_SIZE                             0
#define DEFAULT_PROP_OFFSET                           0
#define DEFAULT_PROP_FPS                              0.0
#define DEFAULT_PROP_DESTINATION_PIXEL_FORMAT         "Raw"
#define DEFAULT_PROP_SOURCE_PIXEL_FORMAT              "Auto"
#define DEFAULT_PROP_GST_PIXEL_FORMAT                 "GRAY8"
#define DEFAULT_PROP_DEBAYER_ON_HOST                  "Auto"
#define DEFAULT_PROP_EXPOSURETIME                     20000.0
#define DEFAULT_PROP_EXPOSUREAUTO                     "Off"
#define DEFAULT_PROP_MVEXPOSUREAUTOUPPERLIMIT         20000.0
#define DEFAULT_PROP_MVEXPOSUREAUTOLOWERLIMIT         10.0
#define DEFAULT_PROP_GAIN                             0.0
#define DEFAULT_PROP_GAINAUTO                         "Off"
#define DEFAULT_PROP_CONFIGURATION_FILE_PATH          ""

/* pad templates */
static GstStaticPadTemplate gst_impactacquiresrc_src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE
        ("{ GRAY8, GRAY16_LE, RGB, BGR, BGRA, BGRx, UYVY, IYU2 }") ";"
        GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER8 ("{ rggb }") ";"
        GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER8 ("{ grbg }") ";"
        GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER16 ("{ rggb16 }", "1234") ";"
        GST_GENICAM_PIXEL_FORMAT_MAKE_BAYER16 ("{ grbg16 }", "1234")
    )
    );

/* class initialisation */
G_DEFINE_TYPE_WITH_CODE (GstImpactAcquireSrc, gst_impactacquiresrc, GST_TYPE_PUSH_SRC,
    GST_DEBUG_CATEGORY_INIT (gst_impactacquiresrc_debug_category, "impactacquiresrc", 0,
        "debug category for impactacquiresrc element"));

/* class init */
static void
gst_impactacquiresrc_class_init (GstImpactAcquireSrcClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstPushSrcClass *push_src_class = GST_PUSH_SRC_CLASS (klass);
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS (klass);

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass), &gst_impactacquiresrc_src_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "Balluff Impact Acquire Video Source", "Source/Video/Device",
      "Balluff Impact Acquire video source", "Danxuan Zhu <danxuan.zhu@balluff.de>");

  gobject_class->set_property = gst_impactacquiresrc_set_property;
  gobject_class->get_property = gst_impactacquiresrc_get_property;
  gobject_class->dispose = gst_impactacquiresrc_dispose;
  gobject_class->finalize = gst_impactacquiresrc_finalize;

  base_src_class->start = GST_DEBUG_FUNCPTR (gst_impactacquiresrc_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR (gst_impactacquiresrc_stop);
  base_src_class->get_caps = GST_DEBUG_FUNCPTR (gst_impactacquiresrc_get_caps);
  base_src_class->set_caps = GST_DEBUG_FUNCPTR (gst_impactacquiresrc_set_caps);

  push_src_class->create = GST_DEBUG_FUNCPTR (gst_impactacquiresrc_create);

  // register gstreamer properties.
  g_object_class_install_property (gobject_class, PROP_CAMERA,
      g_param_spec_int ("camera", "camera",
          "(Number) Camera ID as defined by Balluff's Impact Acquire API. If only one camera is connected this parameter will be ignored and the lone camera will be used. If there are multiple cameras and this parameter isn't defined, the plugin will output a list of available cameras and their IDs. Note that if there are multiple cameras available to the API and the camera parameter isn't defined then this plugin will not run.",
          0, 100, DEFAULT_PROP_CAMERA, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_SERIAL,
      g_param_spec_string ("serial", "Serial number",
          "Serial number, overrides all other interface/device properties",
          DEFAULT_PROP_SERIAL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
      g_param_spec_int ("height", "height",
          "(Pixels) The height of the picture.",
          0, 10000, DEFAULT_PROP_SIZE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
      g_param_spec_int ("width", "width",
          "(Pixels) The width of the picture.",
          0, 10000, DEFAULT_PROP_SIZE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_OFFSETX,
      g_param_spec_int ("offsetx", "Horizontal offset",
          "(Pixels) The horizontal offset of the area of interest (AOI).",
          0, 10000, DEFAULT_PROP_OFFSET, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_OFFSETY,
      g_param_spec_int ("offsety", "Vertical offset",
          "(Pixels) The vertical offset of the area of interest (AOI).",
          0, 10000, DEFAULT_PROP_OFFSET, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_FPS,
      g_param_spec_double ("fps", "Framerate",
          "(Frames per second) Sets the framerate of the video coming from the camera. Setting the value too high might cause the plugin to crash. Note that if your pipeline proves to be too much for your computer then the resulting video won't be in the resolution you set. Setting this parameter will set acquisitionframerateenable to true. The value of this parameter will be saved to the camera, but it will have no effect unless either this or the acquisitionframerateenable parameters are set. Reconnect the camera or use the reset parameter to reset.",
          0.0, 1024.0, DEFAULT_PROP_FPS, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_DESTINATION_PIXEL_FORMAT,
      g_param_spec_string ("dest-pixel-format", "Destination Pixel format",
          "Force the destination pixel format (e.g., Mono8). Default to 'Auto', which will use GStreamer negotiation.",
          DEFAULT_PROP_DESTINATION_PIXEL_FORMAT, (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_SOURCE_PIXEL_FORMAT,
      g_param_spec_string ("src-pixel-format", "Source Pixel format",
          "Force the source pixel format (e.g., Mono8). Default to 'Auto', which will use the default pixel format from the camera.",
          DEFAULT_PROP_SOURCE_PIXEL_FORMAT, (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_DEBAYER_ON_HOST,
      g_param_spec_string ("debayer-on-host", "Debayering On Host",
          "(Yes, Auto) Specify where to debayer the color image. Select 'Yes', the driver on the host will handle the debayering; select 'Auto', the debayering on the camera will be preferred if possible.",
          DEFAULT_PROP_DEBAYER_ON_HOST, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_EXPOSURETIME,
      g_param_spec_double ("exposuretime", "Exposure Time",
          "Sets the exposure time in microseconds when 'autoexposure' is 'Off'. This controls how long the photosensitive cells are exposed to light.",
          10.0, 20000000.0, DEFAULT_PROP_EXPOSURETIME, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_EXPOSUREAUTO,
      g_param_spec_string ("autoexposure", "Automatic exposure setting",
          "(Off,  Continuous) Sets the automatic exposure mode. The exact algorithm used to implement this control is device specific. 'Off': Exposure duration is user controlled using 'exposuretime'. \n'Continuous': Exposure duration is constantly adapted by the device to maximize the dynamic range.",
          DEFAULT_PROP_EXPOSUREAUTO, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_MVEXPOSUREAUTOUPPERLIMIT,
      g_param_spec_double ("autoExposureUpperLimit", "mvAutoExposureUpperLimit",
          "The upper limit of the exposure time in auto exposure mode [us].",
          10.0, 20000000.0, DEFAULT_PROP_MVEXPOSUREAUTOUPPERLIMIT,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_MVEXPOSUREAUTOLOWERLIMIT,
      g_param_spec_double ("autoExposureLowerLimit", "mvAutoExposureLowerLimit",
          "The lower limit of the exposure time in auto exposure mode [us].",
          10.0, 20000000.0, DEFAULT_PROP_MVEXPOSUREAUTOLOWERLIMIT,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_GAIN,
      g_param_spec_double ("gain", "Analog gain",
          "Sets the analog gain as an absolute phsical value in dB when 'autogain' is 'Off'. This is an amplification factor applied to the video signal.",
          0.0, 48.0, DEFAULT_PROP_GAIN, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_GAINAUTO,
      g_param_spec_string ("autogain", "Automatic analog gain setting",
          "(Off,  Continuous) Sets the automatic gain mode (AGC) for analog gain. The exact algorithm used to implement this control is device specific. 'Off': Analog gain is user controlled using 'gain'. \n'Continuous': Analog gain is constantly adapted by the device.",
          DEFAULT_PROP_GAINAUTO, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_CONFIGURATION_FILE_PATH,
      g_param_spec_string ("configurationFile", "Configuration file to configure the device",
          "Can be used to configure the device and the image processing pipeline instead of configuring all properties manually.",
          DEFAULT_PROP_CONFIGURATION_FILE_PATH, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}

/* plugin init */
static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_impactacquiresrc_debug_category, "impactacquiresrc", 0,
      "debug category for impactacquiresrc element");
  return gst_element_register (plugin, "impactacquiresrc", GST_RANK_NONE, GST_TYPE_IMPACTACQUIRESRC);
}

/* src init */
static void
gst_impactacquiresrc_init (GstImpactAcquireSrc * src)
{
  GST_DEBUG_OBJECT (src, "Initialising defaults");
  src->caps = NULL;
  src->deviceId = 0;
  src->serialNumber = g_strdup (DEFAULT_PROP_SERIAL);
  src->deviceHandle = INVALID_ID;
  src->driverHandle = INVALID_ID;
  src->deviceConnected = FALSE;
  src->boAcquisitionRunning = FALSE;
  src->frameNumber = 0;
  src->failedFrames = 0;
  src->width = DEFAULT_PROP_SIZE;
  src->height = DEFAULT_PROP_SIZE;
  src->offsetx = DEFAULT_PROP_OFFSET;
  src->offsety = DEFAULT_PROP_OFFSET;
  src->dest_pixel_format = g_strdup (DEFAULT_PROP_DESTINATION_PIXEL_FORMAT);
  src->src_pixel_format = g_strdup (DEFAULT_PROP_SOURCE_PIXEL_FORMAT);
  src->gst_pixel_format = g_strdup (DEFAULT_PROP_GST_PIXEL_FORMAT);
  src->boDebayerOnHost = FALSE;
  src->fps = DEFAULT_PROP_FPS;
  src->exposureTime = DEFAULT_PROP_EXPOSURETIME;
  src->exposureAuto = DEFAULT_PROP_EXPOSUREAUTO;
  src->mvExposureAutoUpperLimit = DEFAULT_PROP_MVEXPOSUREAUTOUPPERLIMIT;
  src->mvExposureAutoLowerLimit = DEFAULT_PROP_MVEXPOSUREAUTOLOWERLIMIT;
  src->gain = DEFAULT_PROP_GAIN;
  src->gainAuto = DEFAULT_PROP_GAINAUTO;
  src->configurationFilePath = DEFAULT_PROP_CONFIGURATION_FILE_PATH;

  // Mark this element as a live source (disable preroll)
  gst_base_src_set_live (GST_BASE_SRC (src), TRUE);
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_do_timestamp (GST_BASE_SRC (src), TRUE);
}

gboolean
boDeviceFeatureAvailableInList (HOBJ hProp, const char *strFeature)
{
  TPROPHANDLING_ERROR objResult;
  unsigned int dictSize;
  gboolean boFeatureFound = FALSE;
  if ((objResult = OBJ_GetDictSize (hProp, &dictSize)) == PROPHANDLING_NO_ERROR) {
    char propStr[DEFAULT_STRING_SIZE_LIMIT];
    size_t bufSize = DEFAULT_STRING_SIZE_LIMIT;
    for (unsigned int i = 0; i < dictSize; i++) {
      if ((objResult = OBJ_GetIDictEntry (hProp, propStr, bufSize, 0, i)) == PROPHANDLING_NO_ERROR) {
        boFeatureFound = (g_ascii_strncasecmp (strFeature, propStr, -1) == 0);
        if (boFeatureFound) {
          break;
        }
      }
    }
  }
  return boFeatureFound && (objResult == PROPHANDLING_NO_ERROR);
}


void
gst_impactacquiresrc_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
  GstImpactAcquireSrc *src = GST_IMPACTACQUIRESRC (object);

  switch (property_id) {
  case PROP_CAMERA:
    src->deviceId = g_value_get_int (value);
    break;
  case PROP_SERIAL:
    src->serialNumber = g_value_dup_string (value);
    break;
  case PROP_HEIGHT:
    src->height = g_value_get_int (value);
    break;
  case PROP_WIDTH:
    src->width = g_value_get_int (value);
    break;
  case PROP_OFFSETX:
    src->offsetx = g_value_get_int (value);
    break;
  case PROP_OFFSETY:
    src->offsety = g_value_get_int (value);
    break;
  case PROP_FPS:
    src->fps = g_value_get_double (value);
    break;
  case PROP_DEBAYER_ON_HOST:
    src->boDebayerOnHost = g_ascii_strncasecmp (g_value_dup_string (value), "Yes", -1) == 0 ? TRUE : FALSE;
    break;
  case PROP_EXPOSURETIME:
    src->exposureTime = g_value_get_double (value);
    break;
  case PROP_MVEXPOSUREAUTOUPPERLIMIT:
    src->mvExposureAutoUpperLimit = g_value_get_double (value);
    break;
  case PROP_MVEXPOSUREAUTOLOWERLIMIT:
    src->mvExposureAutoLowerLimit = g_value_get_double (value);
    break;
  case PROP_EXPOSUREAUTO:
    src->exposureAuto = g_value_dup_string (value);
    break;
  case PROP_GAIN:
    src->gain = g_value_get_double (value);
    break;
  case PROP_GAINAUTO:
    src->gainAuto = g_value_dup_string (value);
    break;
  case PROP_CONFIGURATION_FILE_PATH:
    src->configurationFilePath = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

void
gst_impactacquiresrc_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
  GstImpactAcquireSrc *src = GST_IMPACTACQUIRESRC (object);
  GST_DEBUG_OBJECT (src, "Getting a property.");
  HOBJ hProp = INVALID_ID;
  HLIST ImageDestinationList = INVALID_ID;
  TPROPHANDLING_ERROR objResult;
  size_t bufSize = DEFAULT_STRING_SIZE_LIMIT;
  char buf[DEFAULT_STRING_SIZE_LIMIT];

  if (src->deviceConnected) {
    switch (property_id) {
    case PROP_CAMERA:
      g_value_set_int (value, src->deviceId);
      break;
    case PROP_SERIAL:
      g_value_set_string (value, src->serialNumber);
      break;
    case PROP_HEIGHT:
      hProp = getSettingProp (src->driverHandle, "Base", "Height");
      g_value_set_int (value, getPropI64 (hProp, 0));
      break;
    case PROP_WIDTH:
      hProp = getSettingProp (src->driverHandle, "Base", "Width");
      g_value_set_int (value, getPropI64 (hProp, 0));
      break;
    case PROP_OFFSETX:
      hProp = getSettingProp (src->driverHandle, "Base", "OffsetX");
      g_value_set_int (value, getPropI64 (hProp, 0));
      break;
    case PROP_OFFSETY:
      hProp = getSettingProp (src->driverHandle, "Base", "OffsetY");
      g_value_set_int (value, getPropI64 (hProp, 0));
      break;
    case PROP_DESTINATION_PIXEL_FORMAT:
      ImageDestinationList = getDriverList (src->driverHandle, "ImageDestination", "Base", dmltSetting);
      if ((objResult =
              OBJ_GetHandleEx (ImageDestinationList, "PixelFormat", &hProp, smIgnoreLists | smIgnoreMethods,
                  INT_MAX)) == PROPHANDLING_NO_ERROR) {
        if (OBJ_GetSFormattedEx (hProp, buf, &bufSize, 0, 0) == PROPHANDLING_NO_ERROR) {
          g_value_set_string (value, buf);
        }
      } else {
        GST_ERROR_OBJECT (src, "Failed to inquire property PixelFormat: Unexpected error(code: %d(%s))", objResult,
            DMR_ErrorCodeToString (objResult));
      }
      break;
    case PROP_SOURCE_PIXEL_FORMAT:
      hProp = getSettingProp (src->driverHandle, "Base", "PixelFormat");
      if (OBJ_GetSFormattedEx (hProp, buf, &bufSize, 0, 0) == PROPHANDLING_NO_ERROR) {
        g_value_set_string (value, buf);
      }
      break;
    case PROP_FPS:
      hProp = getSettingProp (src->driverHandle, "Base", "AcquisitionFrameRate");
      g_value_set_int (value, getPropI64 (hProp, 0));
      break;
    case PROP_DEBAYER_ON_HOST:
      g_value_set_string (value, src->boDebayerOnHost ? "Yes" : DEFAULT_PROP_DEBAYER_ON_HOST);
      break;
    case PROP_EXPOSURETIME:
      hProp = getSettingProp (src->driverHandle, "Base", "ExposureTime");
      g_value_set_int (value, getPropI64 (hProp, 0));
      break;
    case PROP_MVEXPOSUREAUTOUPPERLIMIT:
      hProp = getSettingProp (src->driverHandle, "Base", "mvExposureAutoUpperLimit");
      g_value_set_int (value, getPropI64 (hProp, 0));
      break;
    case PROP_MVEXPOSUREAUTOLOWERLIMIT:
      hProp = getSettingProp (src->driverHandle, "Base", "mvExposureAutoLowerLimit");
      g_value_set_int (value, getPropI64 (hProp, 0));
      break;
    case PROP_EXPOSUREAUTO:
      hProp = getSettingProp (src->driverHandle, "Base", "ExposureAuto");
      if (OBJ_GetSFormattedEx (hProp, buf, &bufSize, 0, 0) == PROPHANDLING_NO_ERROR) {
        g_value_set_string (value, buf);
      }
      break;
    case PROP_GAIN:
      hProp = getSettingProp (src->driverHandle, "Base", "Gain");
      g_value_set_int (value, getPropI64 (hProp, 0));
      break;
    case PROP_GAINAUTO:
      hProp = getSettingProp (src->driverHandle, "Base", "GainAuto");
      if (OBJ_GetSFormattedEx (hProp, buf, &bufSize, 0, 0) == PROPHANDLING_NO_ERROR) {
        g_value_set_string (value, buf);
      }
      break;
    case PROP_CONFIGURATION_FILE_PATH:
      g_value_set_string (value, src->configurationFilePath);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
  } else {
    switch (property_id) {
    case PROP_CAMERA:
      g_value_set_int (value, src->deviceId);
      break;
    case PROP_SERIAL:
      g_value_set_string (value, src->serialNumber);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, src->height);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, src->width);
      break;
    case PROP_OFFSETX:
      g_value_set_int (value, src->offsetx);
      break;
    case PROP_OFFSETY:
      g_value_set_int (value, src->offsety);
      break;
    case PROP_DESTINATION_PIXEL_FORMAT:
      g_value_set_string (value, src->dest_pixel_format);
      break;
    case PROP_SOURCE_PIXEL_FORMAT:
      g_value_set_string (value, src->src_pixel_format);
      break;
    case PROP_FPS:
      g_value_set_double (value, src->fps);
      break;
    case PROP_DEBAYER_ON_HOST:
      g_value_set_string (value, src->boDebayerOnHost ? "Yes" : DEFAULT_PROP_DEBAYER_ON_HOST);
      break;
    case PROP_EXPOSURETIME:
      g_value_set_double (value, src->exposureTime);
      break;
    case PROP_MVEXPOSUREAUTOUPPERLIMIT:
      g_value_set_double (value, src->mvExposureAutoUpperLimit);
      break;
    case PROP_MVEXPOSUREAUTOLOWERLIMIT:
      g_value_set_double (value, src->mvExposureAutoLowerLimit);
      break;
    case PROP_EXPOSUREAUTO:
      g_value_set_string (value, src->exposureAuto);
      break;
    case PROP_GAIN:
      g_value_set_double (value, src->gain);
      break;
    case PROP_GAINAUTO:
      g_value_set_string (value, src->gainAuto);
      break;
    case PROP_CONFIGURATION_FILE_PATH:
      g_value_set_string (value, src->configurationFilePath);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
  }
}

static GstCaps *
gst_impactacquiresrc_get_supported_caps (GstImpactAcquireSrc * src)
{
  GstCaps *caps;
  int i;
  HOBJ hPropImageDestinationPixelFormat = INVALID_ID;
  TPROPHANDLING_ERROR objResult;

  HLIST ImageDestinationList = getDriverList (src->driverHandle, "ImageDestination", "Base", dmltSetting);
  if ((objResult =
          OBJ_GetHandleEx (ImageDestinationList, "PixelFormat", &hPropImageDestinationPixelFormat,
              smIgnoreLists | smIgnoreMethods, INT_MAX)) != PROPHANDLING_NO_ERROR) {
    GST_ERROR_OBJECT (src,
        "Failed to inquire property PixelFormat from ImageDestination: Unexpected error(code: %d(%s))", objResult,
        DMR_ErrorCodeToString (objResult));
  }

  HOBJ hPropImageFormatControlPixelFormat = getSettingProp (src->driverHandle, "Base", "PixelFormat");

  HOBJ hWidth = getSettingProp (src->driverHandle, "Base", "Width");
  HOBJ hHeight = getSettingProp (src->driverHandle, "Base", "Height");

  caps = gst_caps_new_empty ();

  /* check every pixel format GStreamer supports */
  for (i = 0; i < G_N_ELEMENTS (gst_impactacquire_pixel_format_infos); i++) {
    const GstImpactAcquirePixelFormatInfo *info = &gst_impactacquire_pixel_format_infos[i];

    if (boDeviceFeatureAvailableInList (hPropImageFormatControlPixelFormat, info->pixel_format)) {
      GstCaps *format_caps;

      format_caps =
          gst_impactacquire_pixel_format_caps_from_pixel_format_var
          (i, G_BYTE_ORDER, getPropI64 (hWidth, 0), getPropI64 (hHeight, 0));

      if (format_caps) {
        GST_DEBUG_OBJECT (src, "PixelFormat %s is supported, adding to caps", info->pixel_format);
        gst_caps_append (caps, format_caps);
      }
    } else {
      if (!g_str_has_prefix (info->pixel_format, "Bayer")
          && boDeviceFeatureAvailableInList (hPropImageDestinationPixelFormat, info->image_destination_pixel_format)) {
        GstCaps *format_caps;

        format_caps =
            gst_impactacquire_pixel_format_caps_from_pixel_format_var
            (i, G_BYTE_ORDER, getPropI64 (hWidth, 0), getPropI64 (hHeight, 0));

        if (format_caps) {
          GST_DEBUG_OBJECT (src,
              "PixelFormat %s is not supported by the camera but supported by the impact acquire driver, adding to caps",
              info->pixel_format);
          gst_caps_append (caps, format_caps);
        }
      } else {
        GST_DEBUG_OBJECT (src, "PixelFormat %s NOT supported at all, NOT adding to caps", info->pixel_format);
      }
    }
  }
  GST_DEBUG_OBJECT (src, "Supported caps are %" GST_PTR_FORMAT, caps);
  return caps;
}

static void
gst_impactacquiresrc_update_caps (GstImpactAcquireSrc * src)
{
  if (src->caps != NULL) {
    gst_caps_unref (src->caps);
  }
  src->caps = gst_impactacquiresrc_get_supported_caps (src);
}

/* capabilities negotiation */
static GstCaps *
gst_impactacquiresrc_get_caps (GstBaseSrc * bsrc, GstCaps * filter)
{
  GstImpactAcquireSrc *src = GST_IMPACTACQUIRESRC (bsrc);
  GstCaps *result = gst_caps_new_empty ();

  GST_DEBUG_OBJECT (src, "Received a request for caps. Filter:\n%" GST_PTR_FORMAT, filter);
  if (!src->deviceConnected) {
    GST_DEBUG_OBJECT (src, "Could not send caps - no camera connected.");
    return gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD (bsrc));
  } else {
    gst_impactacquiresrc_update_caps (src);
    result = gst_caps_copy (src->caps);
    GST_DEBUG_OBJECT (src, "Return caps:\n%" GST_PTR_FORMAT, result);
  }
  return result;
}

/* capabilities set */
static gboolean
gst_impactacquiresrc_set_caps (GstBaseSrc * bsrc, GstCaps * caps)
{
  GstImpactAcquireSrc *src = GST_IMPACTACQUIRESRC (bsrc);
  gint i;
  gboolean boMatchPixelFormat = FALSE;
  HOBJ hPropImageDestinationPixelFormat = INVALID_ID;
  TPROPHANDLING_ERROR objResult;

  GST_DEBUG_OBJECT (src, "Setting caps to %" GST_PTR_FORMAT, caps);

  HLIST ImageDestinationList = getDriverList (src->driverHandle, "ImageDestination", "Base", dmltSetting);
  if ((objResult =
          OBJ_GetHandleEx (ImageDestinationList, "PixelFormat", &hPropImageDestinationPixelFormat,
              smIgnoreLists | smIgnoreMethods, INT_MAX)) != PROPHANDLING_NO_ERROR) {
    GST_ERROR_OBJECT (src,
        "Failed to inquire property PixelFormat from ImageDestination: Unexpected error(code: %d(%s))", objResult,
        DMR_ErrorCodeToString (objResult));
    return FALSE;
  }

  HOBJ hPropImageFormatControlPixelFormat = getSettingProp (src->driverHandle, "Base", "PixelFormat");

  for (i = 0; i < G_N_ELEMENTS (gst_impactacquire_pixel_format_infos); i++) {
    GstCaps *super_caps;
    GstImpactAcquirePixelFormatInfo *info = &gst_impactacquire_pixel_format_infos[i];
    super_caps = gst_caps_from_string (info->gst_caps_string);
    if (gst_caps_is_subset (caps, super_caps)) {
      if (src->boDebayerOnHost && !g_str_has_prefix (info->pixel_format, "Mono")
          && !g_str_has_prefix (info->pixel_format, "Bayer")
          && boDeviceFeatureAvailableInList (hPropImageDestinationPixelFormat, info->image_destination_pixel_format)) {
        // the image color will be debayered by the impact acquire driver
        size_t bufSize = DEFAULT_STRING_SIZE_LIMIT;
        char buf[DEFAULT_STRING_SIZE_LIMIT];
        if (OBJ_GetSFormattedEx (hPropImageFormatControlPixelFormat, buf, &bufSize, 0, 0) == PROPHANDLING_NO_ERROR) {
          src->src_pixel_format = g_strdup (buf);
        }
        src->dest_pixel_format = g_strdup (info->image_destination_pixel_format);
        setPropS (hPropImageDestinationPixelFormat, src->dest_pixel_format, 0);
        boMatchPixelFormat = TRUE;
      } else {
        if (boDeviceFeatureAvailableInList (hPropImageFormatControlPixelFormat, info->pixel_format)) {
          // no pixel format conversion by the impact acquire driver is needed
          src->src_pixel_format = g_strdup (info->pixel_format);
          src->dest_pixel_format = g_strdup (DEFAULT_PROP_DESTINATION_PIXEL_FORMAT);
          setPropS (hPropImageFormatControlPixelFormat, src->src_pixel_format, 0);
          setPropS (hPropImageDestinationPixelFormat, src->dest_pixel_format, 0);
          boMatchPixelFormat = TRUE;
        } else if (!g_str_has_prefix (info->pixel_format, "Bayer")
            && boDeviceFeatureAvailableInList (hPropImageDestinationPixelFormat,
                info->image_destination_pixel_format)) {
          // pixel format conversion by the impact acquire driver is needed
          size_t bufSize = DEFAULT_STRING_SIZE_LIMIT;
          char buf[DEFAULT_STRING_SIZE_LIMIT];
          if (OBJ_GetSFormattedEx (hPropImageFormatControlPixelFormat, buf, &bufSize, 0, 0) == PROPHANDLING_NO_ERROR) {
            src->src_pixel_format = g_strdup (buf);
          }
          src->dest_pixel_format = g_strdup (info->image_destination_pixel_format);
          setPropS (hPropImageDestinationPixelFormat, src->dest_pixel_format, 0);
          boMatchPixelFormat = TRUE;
        }
      }
      if (boMatchPixelFormat) {
        src->gst_pixel_format = g_strdup (info->gst_pixel_format);
        GST_DEBUG_OBJECT (src,
            "%s Set caps match PixelFormat (ImageFormatControl) '%s' and PixelFormat (ImageDestination) '%s'.",
            __FUNCTION__, src->src_pixel_format, src->dest_pixel_format);
        break;
      }
    }
  }
  if (!boMatchPixelFormat) {
    GST_ERROR_OBJECT (bsrc, "Unsupported caps: %" GST_PTR_FORMAT, caps);
  }
  return boMatchPixelFormat;
}

/* device detection */
static gboolean
gst_impactacquiresrc_select_device (GstImpactAcquireSrc * src)
{
  TDMR_ERROR result;
  TDMR_DeviceInfo deviceInfo;
  size_t bufSize = sizeof (TDMR_DeviceInfo);
  unsigned int numDevices = 0;
  DMR_GetDeviceCount (&numDevices);
  GST_DEBUG_OBJECT (src, "device count: %d", numDevices);
  if (numDevices > 0) {
    if (g_ascii_strncasecmp (src->serialNumber, DEFAULT_PROP_SERIAL, -1) == 0) {
      if ((result = DMR_GetDevice (&src->deviceHandle, dmdsmFamily, "mv*", 0, '*')) != DMR_NO_ERROR) {
        GST_ERROR_OBJECT (src,
            "Cannot get the handle of the device with device ID 0 due to error %s, canceling initialisation.",
            DMR_ErrorCodeToString (result));
        goto error;
      }
    } else {
      if ((result = DMR_GetDevice (&src->deviceHandle, dmdsmSerial, src->serialNumber, 0, '*')) != DMR_NO_ERROR) {
        GST_ERROR_OBJECT (src,
            "Cannot get the handle of the device with serial number %s due to error %s, canceling initialisation.",
            src->serialNumber, DMR_ErrorCodeToString (result));
        goto error;
      }
    }

    if ((result =
            DMR_GetDeviceInfoEx (src->deviceHandle, dmditDeviceInfoStructure, &deviceInfo, &bufSize)) == DMR_NO_ERROR) {
      if (g_ascii_strncasecmp (src->serialNumber, DEFAULT_PROP_SERIAL, -1) == 0) {
        strcpy (src->serialNumber, deviceInfo.serial);
      }
      src->deviceId = deviceInfo.deviceId;
      unsigned int inUse = 0;
      size_t bufSize = sizeof (inUse);
      if ((result = DMR_GetDeviceInfoEx (src->deviceHandle, dmditDeviceIsInUse, &inUse, &bufSize)) != DMR_NO_ERROR) {
        if (result != DMR_NO_ERROR) {
          GST_ERROR_OBJECT (src, "Device %s is currently in use, canceling initialisation.", src->serialNumber);
          goto error;
        }
      }
    }
  } else {
    GST_ERROR_OBJECT (src, "No devices detected, canceling initialisation.");
    goto error;
  }
  GST_DEBUG_OBJECT (src, "Got device: ID: %i, Serial No: %s", src->deviceId, src->serialNumber);
  return TRUE;

error:
  return FALSE;
}

static void
setPropF (HOBJ hProp, double value, int index)
{
  TPROPHANDLING_ERROR result = PROPHANDLING_NO_ERROR;

  if ((result = OBJ_SetF (hProp, value, index)) != PROPHANDLING_NO_ERROR) {
    //printf( "setPropI: Failed to write property value(%s).\n", DMR_ErrorCodeToString( result ) );
    exit (42);
  }
}

/* device init */
static gboolean
gst_impactacquiresrc_connect_device (GstImpactAcquireSrc * src)
{
  TDMR_ERROR result;

  HOBJ hPropInterfaceLayout = getDeviceProp (src->deviceHandle, "InterfaceLayout");
  conditionalSetPropI (hPropInterfaceLayout, dilGenICam, 1);

  HOBJ hPropAcquisitionStartStopBehaviour = getDeviceProp (src->deviceHandle, "AcquisitionStartStopBehaviour");
  conditionalSetPropI (hPropAcquisitionStartStopBehaviour, assbUser, 1);

  if (g_strcmp0 (src->configurationFilePath, "") != 0) {
    HOBJ hLoadSettings = getDeviceProp (src->deviceHandle, "LoadSettings");
    conditionalSetPropI (hLoadSettings, dlsNoLoad, 1);
  }

  if ((result = DMR_OpenDevice (src->deviceHandle, &src->driverHandle)) != DMR_NO_ERROR) {
    GST_ERROR_OBJECT (src, "Failed to initialize device %s, canceling initialisation: Unexpected error(code: %d(%s))",
        src->serialNumber, result, DMR_ErrorCodeToString (result));
  } else {
    GST_DEBUG_OBJECT (src, "Device %s is initialized", src->serialNumber);
  }

  if (g_strcmp0 (src->configurationFilePath, "") != 0) {
    if ((result = DMR_LoadSetting (src->driverHandle, src->configurationFilePath, sfDefault, sUser)) != DMR_NO_ERROR) {
      GST_ERROR_OBJECT (src,
          "Failed to load device configuration file <%s> for device %s, canceling initialisation: Unexpected error(code: %d(%s))",
          src->configurationFilePath, src->serialNumber, result, DMR_ErrorCodeToString (result));
    } else {
      GST_DEBUG_OBJECT (src, "Configuration <%s> file for device %s loaded", src->configurationFilePath,
          src->serialNumber);
    }
  }
  // overwrite any properties that appear on the command line
  if ((src->width != DEFAULT_PROP_SIZE) || (src->offsetx != DEFAULT_PROP_OFFSET)) {
    HOBJ hProp = getSettingProp (src->driverHandle, "Base", "Width");
    // if there is an offset make sure that the width is not too large - this is probably not a comprehensive test!
    gint max = getPropI64 (hProp, PROP_MAX_VAL);
    if (src->width == DEFAULT_PROP_SIZE) {
      src->width = max - src->offsetx;
    } else if ((src->width + src->offsetx) > max) {
      src->width -= src->offsetx;
    }
    if (isFeatureWriteable (hProp)) {
      GST_DEBUG_OBJECT (src, "Setting Width to %d", src->width);
      setPropI64 (hProp, src->width, 0);
    }
  }

  if ((src->height != DEFAULT_PROP_SIZE) || (src->offsety != DEFAULT_PROP_OFFSET)) {
    HOBJ hProp = getSettingProp (src->driverHandle, "Base", "Height");
    // if there is an offset make sure that the height is not too large - this is probably not a comprehensive test!
    gint max = getPropI64 (hProp, PROP_MAX_VAL);
    if (src->height == DEFAULT_PROP_SIZE) {
      src->height = max - src->offsety;
    } else if ((src->height + src->offsety) > max) {
      src->height -= src->offsety;
    }
    if (isFeatureWriteable (hProp)) {
      GST_DEBUG_OBJECT (src, "Setting Height to %d", src->height);
      setPropI64 (hProp, src->height, 0);
    }
  }

  if (src->offsetx != DEFAULT_PROP_OFFSET) {
    HOBJ hProp = getSettingProp (src->driverHandle, "Base", "OffsetX");
    if (isFeatureWriteable (hProp)) {
      GST_DEBUG_OBJECT (src, "Setting XOffset to %d", src->offsetx);
      setPropI64 (hProp, src->offsetx, 0);
    }
  }

  if (src->offsety != DEFAULT_PROP_OFFSET) {
    HOBJ hProp = getSettingProp (src->driverHandle, "Base", "OffsetY");
    if (isFeatureWriteable (hProp)) {
      GST_DEBUG_OBJECT (src, "Setting YOffset to %d", src->offsety);
      setPropI64 (hProp, src->offsety, 0);
    }
  }

  if (src->fps != DEFAULT_PROP_FPS) {
    HOBJ hPropEnable = getSettingProp (src->driverHandle, "Base", "AcquisitionFrameRateEnable");
    if (isFeatureWriteable (hPropEnable)) {
      setPropI (hPropEnable, 1, 0);
      GST_DEBUG_OBJECT (src, "Setting AcquisitionFrameRateEnable to On");
      HOBJ hProp = getSettingProp (src->driverHandle, "Base", "AcquisitionFrameRate");
      if (isFeatureWriteable (hProp)) {
        GST_DEBUG_OBJECT (src, "Setting AcquisitionFrameRate to %lf", src->fps);
        setPropF (hProp, src->fps, 0);
      }
    }
  }

  if (src->exposureTime != DEFAULT_PROP_EXPOSURETIME) {
    HOBJ hProp = getSettingProp (src->driverHandle, "Base", "ExposureTime");
    if (isFeatureWriteable (hProp)) {
      GST_DEBUG_OBJECT (src, "Setting ExposureTime to %lf", src->exposureTime);
      setPropF (hProp, src->exposureTime, 0);
    }
  }

  if (src->mvExposureAutoUpperLimit != DEFAULT_PROP_MVEXPOSUREAUTOUPPERLIMIT) {
    HOBJ hProp = getSettingProp (src->driverHandle, "Base", "mvExposureAutoUpperLimit");
    if (isFeatureWriteable (hProp)) {
      GST_DEBUG_OBJECT (src, "Setting mvExposureAutoUpperLimit to %lf", src->mvExposureAutoUpperLimit);
      setPropI64 (hProp, src->mvExposureAutoUpperLimit, 0);
    }
  }

  if (src->mvExposureAutoLowerLimit != DEFAULT_PROP_MVEXPOSUREAUTOLOWERLIMIT) {
    HOBJ hProp = getSettingProp (src->driverHandle, "Base", "mvExposureAutoLowerLimit");
    if (isFeatureWriteable (hProp)) {
      GST_DEBUG_OBJECT (src, "Setting mvExposureAutoLowerLimit to %lf", src->mvExposureAutoLowerLimit);
      setPropI64 (hProp, src->mvExposureAutoLowerLimit, 0);
    }
  }

  if (g_strcmp0 (src->exposureAuto, DEFAULT_PROP_EXPOSUREAUTO) != 0) {
    HOBJ hProp = getSettingProp (src->driverHandle, "Base", "ExposureAuto");
    if (isFeatureWriteable (hProp)) {
      GST_DEBUG_OBJECT (src, "Setting ExposureAuto to %s", src->exposureAuto);
      setPropS (hProp, src->exposureAuto, 0);
    }
  }

  if (src->gain != DEFAULT_PROP_GAIN) {
    HOBJ hProp = getSettingProp (src->driverHandle, "Base", "Gain");
    if (isFeatureWriteable (hProp)) {
      GST_DEBUG_OBJECT (src, "Setting Gain to %lf", src->gain);
      setPropF (hProp, src->gain, 0);
    }
  }

  if (g_strcmp0 (src->gainAuto, DEFAULT_PROP_GAINAUTO) != 0) {
    HOBJ hPropSelector = getSettingProp (src->driverHandle, "Base", "GainSelector");
    if (isFeatureWriteable (hPropSelector)) {
      GST_DEBUG_OBJECT (src, "Setting GainSelector to 'AnalogAll'");
      setPropS (hPropSelector, "AnalogAll", 0);
    }
    HOBJ hProp = getSettingProp (src->driverHandle, "Base", "GainAuto");
    if (isFeatureWriteable (hProp)) {
      GST_DEBUG_OBJECT (src, "Setting GainAuto to %s", src->gainAuto);
      setPropS (hProp, src->gainAuto, 0);
    }
  }

  src->deviceConnected = (result == DMR_NO_ERROR);
  return src->deviceConnected;
}

/* device close */
static gboolean
gst_impactacquiresrc_disconnect_device (GstImpactAcquireSrc * src)
{
  TDMR_ERROR result;
  if (src->deviceHandle) {
    if ((result = DMR_CloseDevice (src->driverHandle, src->deviceHandle)) != DMR_NO_ERROR) {
      GST_ERROR_OBJECT (src, "Failed to close Device %s: Unexpected error(code: %d(%s)).", src->serialNumber, result,
          DMR_ErrorCodeToString (result));
    } else {
      GST_DEBUG_OBJECT (src, "Device %s is closed", src->serialNumber);
    }
    src->deviceConnected = !(result == DMR_NO_ERROR);
  }
  return !src->deviceConnected;
}

/* acquisition start */
static gboolean
gst_impactacquiresrc_start_acquisition (GstImpactAcquireSrc * src)
{
  TDMR_ERROR result = DMR_NO_ERROR;

  if ((g_ascii_strncasecmp (src->gst_pixel_format, "BGRA", -1) == 0)) {
    HOBJ hSystemSettingProp = getSystemSettingProp (src->driverHandle, "MemoryInitEnable");
    setPropI (hSystemSettingProp, 1, 0);
    hSystemSettingProp = getSystemSettingProp (src->driverHandle, "MemoryInitValue");
    setPropI (hSystemSettingProp, 255, 0);
    GST_DEBUG_OBJECT (src, "Image request buffer with alpha channel has been initialized with 0xFF.");
  }

  while ((result = DMR_ImageRequestSingle (src->driverHandle, 0, 0)) == DMR_NO_ERROR);
  if (result != DEV_NO_FREE_REQUEST_AVAILABLE) {
    GST_ERROR_OBJECT (src, "Failed to queue buffers to the request queue: Unexpected error(code: %d(%s))", result,
        DMR_ErrorCodeToString (result));
  }

  result = DMR_AcquisitionStart (src->driverHandle);
  if (result != DMR_NO_ERROR) {
    GST_ERROR_OBJECT (src, "Failed to start acquisition: Unexpected error(code: %d(%s))", result,
        DMR_ErrorCodeToString (result));
  } else {
    GST_DEBUG_OBJECT (src, "Acquisition has started.");
    src->frameNumber = 0;
    src->failedFrames = 0;
    src->boAcquisitionRunning = TRUE;
  }
  return src->boAcquisitionRunning;
}

/* acquisition stop */
static gboolean
gst_impactacquiresrc_stop_acquisition (GstImpactAcquireSrc * src)
{
  TDMR_ERROR result = DMR_AcquisitionStop (src->driverHandle);
  if (result != DMR_NO_ERROR) {
    GST_ERROR_OBJECT (src, "Failed to stop acquisition: Unexpected error(code: %d(%s))", result,
        DMR_ErrorCodeToString (result));
  } else {
    GST_DEBUG_OBJECT (src, "Acquisition has stopped.");
    src->boAcquisitionRunning = FALSE;
    // clear all queues
    if ((result = DMR_ImageRequestReset (src->driverHandle, 0, 0)) != DMR_NO_ERROR) {
      GST_ERROR_OBJECT (src, "Failed to reset all queues: Unexpected error(code: %d(%s))", result,
          DMR_ErrorCodeToString (result));
    }
  }
  return !src->boAcquisitionRunning;
}

static gboolean
gst_impactacquiresrc_start (GstBaseSrc * bsrc)
{
  DMR_Init (&hDMR);

  GstImpactAcquireSrc *src = GST_IMPACTACQUIRESRC (bsrc);
  gboolean boGstSrcStarted = FALSE;
  if (!gst_impactacquiresrc_select_device (src) || !gst_impactacquiresrc_connect_device (src)) {
    gst_impactacquiresrc_disconnect_device (src);
    GST_ERROR_OBJECT (src, "%s failed", __FUNCTION__);
  } else {
    boGstSrcStarted = TRUE;
  }
  return boGstSrcStarted;
}

static gboolean
gst_impactacquiresrc_stop (GstBaseSrc * bsrc)
{
  GstImpactAcquireSrc *src = GST_IMPACTACQUIRESRC (bsrc);
  if (src->boAcquisitionRunning) {
    gst_impactacquiresrc_stop_acquisition (src);
  }
  GST_DEBUG_OBJECT (src, "stop");
  return gst_impactacquiresrc_disconnect_device (src);
}

typedef struct
{
  GstImpactAcquireSrc *src;
  int requestNr;
} CurrentRequestInfo;

/* current frame buffer release */
static void
release_frame_buffer (void *data)
{
  CurrentRequestInfo *currentRequestInfo = (CurrentRequestInfo *) data;
  DMR_ImageRequestUnlock (currentRequestInfo->src->driverHandle, currentRequestInfo->requestNr);
  DMR_ImageRequestSingle (currentRequestInfo->src->driverHandle, 0, 0);
  g_free (currentRequestInfo);
}

/* stream buffers handling*/
static GstFlowReturn
gst_impactacquiresrc_create (GstPushSrc * psrc, GstBuffer ** buf)
{
  GstImpactAcquireSrc *src = GST_IMPACTACQUIRESRC (psrc);
  if (!src->boAcquisitionRunning) {
    if (!gst_impactacquiresrc_start_acquisition (src)) {
      return GST_FLOW_ERROR;
    }
  }

  const gint timeout_ms = 500;
  gint requestNr;
  RequestResult requestResult;
  ImageBuffer *pIB = 0;
  TDMR_ERROR result = DMR_ImageRequestWaitFor (src->driverHandle, timeout_ms, 0, &requestNr);
  if (result == DMR_NO_ERROR) {
    CurrentRequestInfo *currentRequestInfo = (CurrentRequestInfo *) g_malloc0 (sizeof (CurrentRequestInfo));
    currentRequestInfo->src = src;
    currentRequestInfo->requestNr = requestNr;
    // check if the request contains a valid image
    result = DMR_GetImageRequestResultEx (src->driverHandle, requestNr, &requestResult, sizeof (requestResult), 0, 0);
    if ((result == DMR_NO_ERROR) && (requestResult.result == rrOK)) {
      if ((result = DMR_GetImageRequestBuffer (src->driverHandle, requestNr, &pIB)) == DMR_NO_ERROR) {
        GST_DEBUG_OBJECT (src, "%s: get request buffer %d with size %d and pixel format %d", __FUNCTION__, requestNr,
            pIB->iSize, pIB->pixelFormat);
        *buf =
            gst_buffer_new_wrapped_full ((GstMemoryFlags) GST_MEMORY_FLAG_READONLY, (gpointer) pIB->vpData, pIB->iSize,
            0, pIB->iSize, currentRequestInfo, (GDestroyNotify) release_frame_buffer);
      } else {
        src->failedFrames += 1;
        release_frame_buffer (currentRequestInfo);
        GST_ERROR_OBJECT (src, "DMR_GetImageRequestBuffer failed(code: %d(%s))", result,
            DMR_ErrorCodeToString (result));
      }
    } else {
      src->failedFrames += 1;
      release_frame_buffer (currentRequestInfo);
      GST_ERROR_OBJECT (src, "DMR_GetImageRequestResult: ERROR! Return value: %d(%s), request result: %d.", result,
          DMR_ErrorCodeToString (result), requestResult.result);
    }
    GST_BUFFER_OFFSET (*buf) = src->frameNumber;
    src->frameNumber += 1;
    GST_BUFFER_OFFSET_END (*buf) = src->frameNumber;
  } else {
    GST_ERROR_OBJECT (src, "DMR_ImageRequestWaitFor failed(code: %d(%s))", result, DMR_ErrorCodeToString (result));
  }
  return GST_FLOW_OK;
}

void
gst_impactacquiresrc_dispose (GObject * object)
{
  GstImpactAcquireSrc *src = GST_IMPACTACQUIRESRC (object);
  GST_DEBUG_OBJECT (src, "dispose");
  G_OBJECT_CLASS (gst_impactacquiresrc_parent_class)->dispose (object);
}

void
gst_impactacquiresrc_finalize (GObject * object)
{
  GstImpactAcquireSrc *src = GST_IMPACTACQUIRESRC (object);
  GST_DEBUG_OBJECT (src, "finalize");
  if (src->caps) {
    gst_caps_unref (src->caps);
    src->caps = NULL;
  }
  g_free (src->dest_pixel_format);
  g_free (src->src_pixel_format);
  g_free (src->serialNumber);
  G_OBJECT_CLASS (gst_impactacquiresrc_parent_class)->finalize (object);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    impactacquire,
    "Balluff Impact Acquire video elements",
    plugin_init, GST_PACKAGE_VERSION, GST_PACKAGE_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)

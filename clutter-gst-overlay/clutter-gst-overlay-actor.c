/*
 * clutter-gst-overlay.
 *
 * Clutter actor controlling GStreamer window.
 *
 * clutter-gst-overlay-actor.c - ClutterActor using native window to display a
 *                               video stream.
 *
 * Authored By Viatcheslav Gachkaylo  <vgachkaylo@crystalnix.com>
 *             Vadim Zakondyrin       <thekondr@crystalnix.com>
 *
 * Copyright (C) 2011 Crystalnix
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "clutter-gst-overlay-actor.h"
#include <gst/interfaces/xoverlay.h>
#include <gst/video/video.h>
#include <X11/Xlib.h>

#define CLUTTER_GST_OVERLAY_ACTOR_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        CLUTTER_TYPE_GST_OVERLAY_ACTOR, ClutterGstOverlayActorPrivate))

struct _ClutterGstOverlayActorPrivate
{
  GstElement *pipeline;
  GstElement *video_sink;

  Display    *display;
  Window      window;

  gchar      *font_name;
  gdouble     buffer_fill;

  ClutterGstOverlayStates states;
};

typedef enum {
  GST_PLAY_FLAG_VIDEO         = (1 << 0),
  GST_PLAY_FLAG_AUDIO         = (1 << 1),
  GST_PLAY_FLAG_TEXT          = (1 << 2),
  GST_PLAY_FLAG_VIS           = (1 << 3),
  GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
  GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
  GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
  GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
  GST_PLAY_FLAG_BUFFERING     = (1 << 8),
  GST_PLAY_FLAG_DEINTERLACE   = (1 << 9)
} GstPlayFlags;

enum {
  PROP_0,

  /* For ClutterMedia interface */
  PROP_AUDIO_VOLUME,
  PROP_BUFFER_FILL,
  PROP_CAN_SEEK,
  PROP_DURATION,
  PROP_PLAYING,
  PROP_PROGRESS,
  PROP_SUBTITLE_FONT_NAME,
  PROP_SUBTITLE_URI,
  PROP_URI,

  PROP_N_TEXT,
  PROP_N_AUDIO,
  PROP_N_VIDEO,
  PROP_CURRENT_TEXT,
  PROP_CURRENT_AUDIO,
  PROP_CURRENT_VIDEO,
  PROP_MUTE
};

static void clutter_media_interface_init (ClutterMediaIface *iface);

G_DEFINE_TYPE_WITH_CODE (ClutterGstOverlayActor,
                         clutter_gst_overlay_actor,
                         CLUTTER_TYPE_RECTANGLE,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_MEDIA,
                                                clutter_media_interface_init));

static void
clutter_gst_overlay_actor_dispose (GObject *gobject)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (gobject)->priv;

  if (priv->pipeline)
    {
      gst_element_set_state (priv->pipeline, GST_STATE_NULL);

      gst_object_unref (GST_OBJECT (priv->pipeline));

      priv->pipeline = NULL;
    }

  G_OBJECT_CLASS (clutter_gst_overlay_actor_parent_class)->dispose (gobject);
}

static void
clutter_gst_overlay_actor_finalize (GObject *gobject)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (gobject)->priv;

  g_free (priv->font_name);

  XDestroyWindow (priv->display, priv->window);

  G_OBJECT_CLASS (clutter_gst_overlay_actor_parent_class)->finalize (gobject);
}

static void
clutter_gst_overlay_actor_show (ClutterActor *self,
                                gpointer      user_data)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  XMapWindow (priv->display, priv->window);
}

static void
clutter_gst_overlay_actor_hide (ClutterActor *self,
                                gpointer      user_data)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  XUnmapWindow (priv->display, priv->window);
}

static void
clutter_gst_overlay_actor_allocate (ClutterActor *self,
                                    ClutterActorBox *box,
                                    ClutterAllocationFlags flags,
                                    gpointer user_data)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;
  gfloat x, y, w, h;

  clutter_actor_get_transformed_position (self, &x, &y);

  clutter_actor_get_transformed_size (self, &w, &h);

  /* In XResizeWindow
   * if either width or height is zero,
   * a BadValue error results.
   */
  if (w <= 0)
    w = 1;

  if (h <= 0)
    h = 1;

  XMoveResizeWindow (priv->display, priv->window,
                     x, y, w, h);

  gst_x_overlay_expose (GST_X_OVERLAY (priv->video_sink));
}

static void
clutter_gst_overlay_actor_parent_set (ClutterActor *self,
                                      ClutterActor *old_parent,
                                      gpointer      user_data)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;
  ClutterStage *stage_new_parent = CLUTTER_STAGE (clutter_actor_get_stage (self));

  if (!CLUTTER_IS_STAGE (stage_new_parent))
    return;

  ClutterActor *parent = clutter_actor_get_parent (self);
  Window window_new_parent = clutter_x11_get_stage_window (stage_new_parent);

  /* We should track all parents for getting 'allocate' signal
   * for correct allocating X window in stage coordinates
   * when any parent moved and 'parent_set' signal
   * for get 'allocate' signal from new parent of any parent
   */

  g_signal_handlers_disconnect_by_func (self,
                                        clutter_gst_overlay_actor_allocate,
                                        self);
  g_signal_handlers_disconnect_by_func (self,
                                        clutter_gst_overlay_actor_parent_set,
                                        self);

  while (parent)
    {
      g_signal_connect_swapped (parent, "parent-set",
                                G_CALLBACK (clutter_gst_overlay_actor_parent_set),
                                self);

      g_signal_connect_swapped (parent, "allocation-changed",
                                G_CALLBACK (clutter_gst_overlay_actor_allocate),
                                self);

      parent = clutter_actor_get_parent (parent);
    }

  XReparentWindow (priv->display, priv->window,
                   window_new_parent, 0, 0);

  clutter_gst_overlay_actor_allocate (self, NULL, 0, NULL);
}

static gint
get_pipeline_int_prop (ClutterGstOverlayActor *self,
                       const gchar            *prop)
{
  gint value = -1;

  g_object_get (G_OBJECT (self->priv->pipeline), prop, &value, NULL);

  return value;
}

static void
set_pipeline_int_prop (ClutterGstOverlayActor *self,
                       const gchar            *prop,
                       gint                    value)
{
  g_object_set (G_OBJECT (self->priv->pipeline), prop, value, NULL);
}

static gint
get_n_text (ClutterGstOverlayActor *self)
{
  return get_pipeline_int_prop (self, "n-text");
}

static gint
get_n_audio (ClutterGstOverlayActor *self)
{
  return get_pipeline_int_prop (self, "n-audio");
}

static gint
get_n_video (ClutterGstOverlayActor *self)
{
  return get_pipeline_int_prop (self, "n-video");
}

static gint
get_current_text (ClutterGstOverlayActor *self)
{
  return get_pipeline_int_prop (self, "current-text");
}

static gint
get_current_audio (ClutterGstOverlayActor *self)
{
  return get_pipeline_int_prop (self, "current-audio");
}

static gint
get_current_video (ClutterGstOverlayActor *self)
{
  return get_pipeline_int_prop (self, "current-video");
}

static void
set_current_text (ClutterGstOverlayActor *self,
                  gint                    stream)
{
  set_pipeline_int_prop (self, "current-text", stream);
}

static void
set_current_audio (ClutterGstOverlayActor *self,
                   gint                    stream)
{
  set_pipeline_int_prop (self, "current-audio", stream);
}

static void
set_current_video (ClutterGstOverlayActor *self,
                   gint                    stream)
{
  set_pipeline_int_prop (self, "current-video", stream);
}

static void
set_audio_volume (ClutterGstOverlayActor *self,
                  gdouble                 volume)
{
  g_object_set (G_OBJECT (self->priv->pipeline), "volume", volume, NULL);
}

static gdouble
get_audio_volume (ClutterGstOverlayActor *self)
{
  gdouble volume = -1;

  g_object_get (G_OBJECT (self->priv->pipeline), "volume", &volume, NULL);

  return volume;
}

static void
set_uri (ClutterGstOverlayActor *self,
         const gchar            *uri)
{
  g_object_set (G_OBJECT (self->priv->pipeline), "uri", uri, NULL);
}

static gchar *
get_uri (ClutterGstOverlayActor *self)
{
  gchar *uri = NULL;

  g_object_get (G_OBJECT (self->priv->pipeline), "uri", &uri, NULL);

  return uri;
}

static gboolean
test_uri (ClutterGstOverlayActor *self)
{
  gboolean result = FALSE;

  gchar *uri = get_uri (self);

  if (uri)
    result = TRUE;

  g_free (uri);

  return result;
}

static void
set_playing (ClutterGstOverlayActor *self,
             gboolean playing)
{
    GstStateChangeReturn state_change = 
      gst_element_set_state (self->priv->pipeline,
                             playing ? GST_STATE_PLAYING : GST_STATE_PAUSED);

    if (state_change == GST_STATE_CHANGE_FAILURE)
      g_warning ("Unable to set playing\n");
}

static gboolean
get_playing (ClutterGstOverlayActor *self)
{
  gboolean playing = FALSE;
  GstState state, pending;

  gst_element_get_state (self->priv->pipeline, &state, &pending, 0);

  playing = pending ? (pending == GST_STATE_PLAYING) :
                      (state   == GST_STATE_PLAYING);

  return playing;
}

/* We can't get duration, set/get progress before main loop started */

static gdouble
get_duration (ClutterGstOverlayActor *self)
{
  gint64 duration = -1;
  gboolean result;
  GstFormat format = GST_FORMAT_TIME;

  result = gst_element_query_duration (self->priv->pipeline, &format, &duration);

  if (!result)
    {
      g_warning ("Unable to get duration\n");
      return -1;
    }

  if (format != GST_FORMAT_TIME)
    {
      g_warning ("It is not format of the duration\n");
      return -1;
    }

  return (gdouble)duration;
}

static void
set_progress (ClutterGstOverlayActor *self,
              gdouble                 progress)
{
  if (test_uri (self))
    {
      gboolean result;

      result = gst_element_seek_simple (self->priv->pipeline, GST_FORMAT_TIME,
                                        GST_SEEK_FLAG_FLUSH | 
                                        GST_SEEK_FLAG_KEY_UNIT,
                                        progress * get_duration (self));

      if (!result)
        g_warning ("Unable to set progress\n");
    }
  else
    g_warning ("Unable to set progress: no URI is set\n");
}

static gdouble
get_progress (ClutterGstOverlayActor *self)
{
  gint64 progress = -1;
  gboolean result;
  GstFormat format = GST_FORMAT_TIME;

  result = gst_element_query_position (self->priv->pipeline, &format, &progress);

  if (!result)
    {
      g_warning ("Unable to get progress\n");
      return -1;
    }

  if (format != GST_FORMAT_TIME)
    {
      g_warning ("It is not format of the progress\n");
      return -1;
    }

  return (gdouble)progress / get_duration (self);
}

static void
set_subtitle_uri (ClutterGstOverlayActor *self,
                  const gchar            *uri)
{
  g_object_set (G_OBJECT (self->priv->pipeline), "suburi", uri, NULL);
}

static gchar *
get_subtitle_uri (ClutterGstOverlayActor *self)
{
  gchar *uri = NULL;

  g_object_get (G_OBJECT (self->priv->pipeline), "suburi", &uri, NULL);

  return uri;
}

static void
set_subtitle_font_name (ClutterGstOverlayActor *self,
                        const gchar            *font_name)
{
  ClutterGstOverlayActorPrivate *priv = self->priv;

  g_free (priv->font_name);

  priv->font_name = g_strdup (font_name);

  g_object_set (G_OBJECT (priv->pipeline),
                "subtitle-font-desc", font_name,
                NULL);
}

static gchar *
get_subtitle_font_name (ClutterGstOverlayActor *self)
{
  return g_strdup (self->priv->font_name);
}

static gdouble
get_buffer_fill (ClutterGstOverlayActor *self)
{
  return self->priv->buffer_fill;
}

static gboolean
get_can_seek (ClutterGstOverlayActor *self)
{
  gboolean can_seek = FALSE;
  GstQuery *seeking = gst_query_new_seeking (GST_FORMAT_TIME);

  if (gst_element_query (self->priv->pipeline, seeking))
    gst_query_parse_seeking (seeking, NULL, &can_seek, NULL, NULL);

  gst_query_unref (seeking);

  return can_seek;
}

static GstPad*
get_pad (ClutterGstOverlayActor *self,
         const gchar            *type_of_pad,
         gint                    stream)
{
  GstPad *pad = NULL;

  g_signal_emit_by_name (self->priv->pipeline, type_of_pad, stream, &pad);

  return pad;
}

static GstPad*
get_text_pad (ClutterGstOverlayActor *self,
              gint                    stream)
{
  return get_pad (self, "get-text-pad", stream);
}

static GstPad*
get_audio_pad (ClutterGstOverlayActor *self,
               gint                    stream)
{
  return get_pad (self, "get-audio-pad", stream);
}

static GstPad*
get_video_pad (ClutterGstOverlayActor *self,
               gint                    stream)
{
  return get_pad (self, "get-video-pad", stream);
}

static void
clutter_gst_overlay_actor_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  ClutterGstOverlayActor *self = CLUTTER_GST_OVERLAY_ACTOR (object);

  switch (property_id)
    {
    case PROP_AUDIO_VOLUME:
      set_audio_volume (self, g_value_get_double (value));
      break;

    case PROP_PLAYING:
      set_playing (self, g_value_get_boolean (value));
      break;

    case PROP_PROGRESS:
      set_progress (self, g_value_get_double (value));
      break;

    case PROP_SUBTITLE_FONT_NAME:
      set_subtitle_font_name (self, g_value_get_string (value));
      break;

    case PROP_SUBTITLE_URI:
      set_subtitle_uri (self, g_value_get_string (value));
      break;

    case PROP_URI:
      set_uri (self, g_value_get_string (value));
      break;

    case PROP_CURRENT_TEXT:
      set_current_text (self, g_value_get_int (value));
      break;

    case PROP_CURRENT_AUDIO:
      set_current_audio (self, g_value_get_int (value));
      break;

    case PROP_CURRENT_VIDEO:
      set_current_video (self, g_value_get_int (value));
      break;

    case PROP_MUTE:
      clutter_gst_overlay_actor_set_mute (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clutter_gst_overlay_actor_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  ClutterGstOverlayActor *self = CLUTTER_GST_OVERLAY_ACTOR (object);

  switch (property_id)
    {
    case PROP_AUDIO_VOLUME:
      g_value_set_double (value, get_audio_volume (self));
      break;

    case PROP_BUFFER_FILL:
      g_value_set_double (value, get_buffer_fill (self));
      break;

    case PROP_CAN_SEEK:
      g_value_set_boolean (value, get_can_seek (self));
      break;

    case PROP_DURATION:
      g_value_set_double (value, get_duration (self));
      break;

    case PROP_PLAYING:
      g_value_set_boolean (value, get_playing (self));
      break;

    case PROP_PROGRESS:
      g_value_set_double (value, get_progress (self));
      break;

    case PROP_SUBTITLE_FONT_NAME:
      g_value_take_string (value, get_subtitle_font_name (self));
      break;

    case PROP_SUBTITLE_URI:
      g_value_take_string (value, get_subtitle_uri (self));
      break;

    case PROP_URI:
      g_value_take_string (value, get_uri (self));
      break;

    case PROP_N_TEXT:
      g_value_set_int (value, get_n_text (self));
      break;

    case PROP_N_AUDIO:
      g_value_set_int (value, get_n_audio (self));
      break;

    case PROP_N_VIDEO:
      g_value_set_int (value, get_n_video (self));
      break;

    case PROP_CURRENT_TEXT:
      g_value_set_int (value, get_current_text (self));
      break;

    case PROP_CURRENT_AUDIO:
      g_value_set_int (value, get_current_audio (self));
      break;

    case PROP_CURRENT_VIDEO:
      g_value_set_int (value, get_current_video (self));
      break;

    case PROP_MUTE:
      g_value_set_boolean (value, clutter_gst_overlay_actor_get_mute (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  ClutterGstOverlayActor *actor = CLUTTER_GST_OVERLAY_ACTOR (data);

  switch (GST_MESSAGE_TYPE (msg)) {

  case GST_MESSAGE_EOS: {
    actor->priv->states |= CLUTTER_GST_OVERLAY_STATE_ENDED;

    gst_element_set_state (actor->priv->pipeline, GST_STATE_READY);

    g_signal_emit_by_name (actor, "eos");
    break;
  }

  case GST_MESSAGE_ERROR: {
    gchar *debug;
    GError *error;

    gst_message_parse_error (msg, &error, &debug);
    g_free (debug);

    gst_element_set_state (actor->priv->pipeline, GST_STATE_NULL);
    g_signal_emit_by_name (actor, "error", error);

    g_error_free (error);
    break;
  }

  case GST_MESSAGE_BUFFERING: {
    gint percent;
    ClutterGstOverlayActorPrivate *priv = actor->priv;

    gst_message_parse_buffering (msg, &percent);
    priv->buffer_fill = (double)percent / 100.0;
    
    if (priv->buffer_fill < 1)
      priv->states |= CLUTTER_GST_OVERLAY_STATE_LOADING;
    else
      priv->states &= ~CLUTTER_GST_OVERLAY_STATE_LOADING;

    break;
  }

  case GST_MESSAGE_ELEMENT : {
    if (gst_structure_has_name (msg->structure, "prepare-xwindow-id"))
      gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (actor->priv->video_sink),
                                    actor->priv->window);
    break;
  }

  case GST_MESSAGE_STATE_CHANGED: {
    GstState old_state, new_state;
    GstElement *src = GST_ELEMENT (GST_MESSAGE_SRC (msg));

    if (actor->priv->pipeline != src)
      return;

    gst_message_parse_state_changed (msg, &old_state, &new_state, NULL);

    if (old_state == GST_STATE_PAUSED &&
        new_state == GST_STATE_PLAYING)
      {
        actor->priv->states |= CLUTTER_GST_OVERLAY_STATE_PLAYING;
        actor->priv->states &= ~CLUTTER_GST_OVERLAY_STATE_ENDED;
      }

    if (old_state == GST_STATE_PLAYING &&
        new_state == GST_STATE_PAUSED)
      {
        actor->priv->states &= ~CLUTTER_GST_OVERLAY_STATE_PLAYING;
      }

    break;
  }

  default:
    break;
  }

  return TRUE;
}

static void
clutter_media_interface_init (ClutterMediaIface *iface)
{
}

static void
clutter_gst_overlay_actor_init (ClutterGstOverlayActor *self)
{
  ClutterGstOverlayActorPrivate *priv;

  GstElement *pipeline;
  GstElement *video_sink;
  GstBus *bus;

  Display *display = GINT_TO_POINTER (clutter_x11_get_default_display ());
  Window rootwindow = clutter_x11_get_root_window ();
  int screen = clutter_x11_get_default_screen ();
  Window window;

  self->priv = priv = CLUTTER_GST_OVERLAY_ACTOR_GET_PRIVATE (self);
  priv->display = display;

  /* Used for creating X-window
     independent of the window-manager */
  XSetWindowAttributes attributes;
  attributes.override_redirect = True;
  attributes.background_pixel = BlackPixel(display, screen);
  priv->window = window = XCreateWindow (display, rootwindow,
                                         0, 0, 1, 1, 0, 0, 0, 0,
                                         CWBackPixel | CWOverrideRedirect,
                                         &attributes);

  XSync (display, FALSE);

  priv->pipeline   = pipeline   = gst_element_factory_make ("playbin2",
                                                            "pipeline");
  priv->video_sink = video_sink = gst_element_factory_make ("ximagesink",
                                                            "window");
  priv->font_name  = NULL;
  priv->buffer_fill = 1.0;

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, bus_call, self);
  gst_object_unref (bus);

  g_signal_connect (self, "show",
                    G_CALLBACK (clutter_gst_overlay_actor_show), NULL);
  g_signal_connect (self, "hide",
                    G_CALLBACK (clutter_gst_overlay_actor_hide), NULL);
  g_signal_connect (self, "allocation-changed",
                    G_CALLBACK (clutter_gst_overlay_actor_allocate), NULL);
  g_signal_connect (self, "parent-set",
                    G_CALLBACK (clutter_gst_overlay_actor_parent_set), NULL);

  g_object_set (G_OBJECT (pipeline), "video-sink", video_sink, NULL);

  clutter_gst_overlay_actor_allocate (CLUTTER_ACTOR (self), NULL, 0, NULL);
}

static void
clutter_gst_overlay_actor_class_init (ClutterGstOverlayActorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  //  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (ClutterGstOverlayActorPrivate));

  gobject_class->dispose = clutter_gst_overlay_actor_dispose;
  gobject_class->finalize = clutter_gst_overlay_actor_finalize;
  gobject_class->set_property = clutter_gst_overlay_actor_set_property;
  gobject_class->get_property = clutter_gst_overlay_actor_get_property;

  g_object_class_override_property (gobject_class,
                                    PROP_AUDIO_VOLUME,
                                    "audio-volume");

  g_object_class_override_property (gobject_class,
                                    PROP_BUFFER_FILL,
                                    "buffer-fill");

  g_object_class_override_property (gobject_class,
                                    PROP_CAN_SEEK,
                                    "can-seek");

  g_object_class_override_property (gobject_class,
                                    PROP_DURATION,
                                    "duration");

  g_object_class_override_property (gobject_class,
                                    PROP_PLAYING,
                                    "playing");

  g_object_class_override_property (gobject_class,
                                    PROP_PROGRESS,
                                    "progress");

  g_object_class_override_property (gobject_class,
                                    PROP_SUBTITLE_FONT_NAME,
                                    "subtitle-font-name");

  g_object_class_override_property (gobject_class,
                                    PROP_SUBTITLE_URI,
                                    "subtitle-uri");

  g_object_class_override_property (gobject_class,
                                    PROP_URI,
                                    "uri");

  pspec = g_param_spec_int ("n-text",
                            "N text",
                            "Count of subtitle streams",
                            0,
                            G_MAXINT,
                            0,
                            G_PARAM_READABLE);
  g_object_class_install_property (gobject_class,
                                   PROP_N_TEXT, pspec);

  pspec = g_param_spec_int ("n-audio",
                            "N audio",
                            "Count of audio streams",
                            0,
                            G_MAXINT,
                            0,
                            G_PARAM_READABLE);
  g_object_class_install_property (gobject_class,
                                   PROP_N_AUDIO, pspec);

  pspec = g_param_spec_int ("n-video",
                            "N video",
                            "Count of video streams",
                            0,
                            G_MAXINT,
                            0,
                            G_PARAM_READABLE);
  g_object_class_install_property (gobject_class,
                                   PROP_N_VIDEO, pspec);

  pspec = g_param_spec_int ("current-text",
                            "Current text",
                            "Number of current subtitle stream",
                            -1,
                            G_MAXINT,
                            -1,
                            G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class,
                                   PROP_CURRENT_TEXT, pspec);

  pspec = g_param_spec_int ("current-audio",
                            "Current audio",
                            "Number of current audio stream",
                            -1,
                            G_MAXINT,
                            -1,
                            G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class,
                                   PROP_CURRENT_AUDIO, pspec);

  pspec = g_param_spec_int ("current-video",
                            "Current video",
                            "Number of current video stream",
                            -1,
                            G_MAXINT,
                            -1,
                            G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class,
                                   PROP_CURRENT_VIDEO, pspec);

  pspec = g_param_spec_boolean ("mute",
                                "Muted",
                                "Is audio channel muted",
                                FALSE,
                                G_PARAM_READWRITE);
  g_object_class_install_property (gobject_class,
                                   PROP_MUTE, pspec);
}

ClutterActor *
clutter_gst_overlay_actor_new (void)
{
  return g_object_new (CLUTTER_TYPE_GST_OVERLAY_ACTOR, NULL);
}

ClutterActor *
clutter_gst_overlay_actor_new_with_uri (const gchar *uri)
{
  return g_object_new (CLUTTER_TYPE_GST_OVERLAY_ACTOR,
                       "uri", uri, NULL);
}

void
clutter_gst_overlay_actor_play (ClutterGstOverlayActor *self)
{
  g_return_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (self));

  set_playing (self, TRUE);
}

void
clutter_gst_overlay_actor_pause (ClutterGstOverlayActor *self)
{
  g_return_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (self));

  set_playing (self, FALSE);
}

void
clutter_gst_overlay_actor_stop (ClutterGstOverlayActor *self)
{
  g_return_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (self));

  GstStateChangeReturn state_change;
  set_playing (self, FALSE);
  state_change = gst_element_set_state (self->priv->pipeline,
                                        GST_STATE_READY);

  g_return_if_fail (state_change != GST_STATE_CHANGE_FAILURE);
}

void
clutter_gst_overlay_actor_set_mute (ClutterGstOverlayActor *self,
                                    gboolean                mute)
{
  g_return_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (self));

  g_object_set (G_OBJECT (self->priv->pipeline), "mute", mute, NULL);
}

gboolean
clutter_gst_overlay_actor_get_mute (ClutterGstOverlayActor *self)
{
  gboolean is_muted;

  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (self), FALSE);

  g_object_get (G_OBJECT (self->priv->pipeline), "mute", &is_muted, NULL);

  return is_muted;
}

void
clutter_gst_overlay_actor_set_subtitle_flag (ClutterGstOverlayActor *self,
                                             gboolean                flag)
{
  GstPlayFlags flags;

  g_return_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (self));

  g_object_get (G_OBJECT (self->priv->pipeline), "flags", &flags, NULL);

  if (flag)
    flags |= GST_PLAY_FLAG_TEXT;
  else
    flags &= ~GST_PLAY_FLAG_TEXT;

  g_object_set (G_OBJECT (self->priv->pipeline), "flags", flags, NULL);
}

gboolean
clutter_gst_overlay_actor_get_subtitle_flag (ClutterGstOverlayActor *self)
{
  GstPlayFlags flags;

  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (self), FALSE);

  g_object_get (G_OBJECT (self->priv->pipeline), "flags", &flags, NULL);

  return !(!(flags & GST_PLAY_FLAG_TEXT));
}

gboolean
clutter_gst_overlay_actor_get_video_size (ClutterGstOverlayActor *self,
                                          gint                   *width,
                                          gint                   *height)
{
  gint video_stream = get_current_video (self);
  GstPad *video_pad = get_video_pad (self, video_stream);
  gboolean result;

  g_return_val_if_fail (video_pad != NULL, FALSE);

  result = gst_video_get_size (video_pad, width, height);

  gst_object_unref (video_pad);

  return result;
}

ClutterGstOverlayStates
clutter_gst_overlay_actor_get_states (ClutterGstOverlayActor *self)
{
  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (self),
                        CLUTTER_GST_OVERLAY_STATE_NULL);

  return self->priv->states;
}

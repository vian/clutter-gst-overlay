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
#include <X11/Xlib.h>

#define CLUTTER_GST_OVERLAY_ACTOR_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        CLUTTER_TYPE_GST_OVERLAY_ACTOR, ClutterGstOverlayActorPrivate))

struct _ClutterGstOverlayActorPrivate
{
  GstElement *pipeline;
  GstElement *video_sink;

  Display    *display;
  Window     window;
};

enum {
  PROP_0,

  PROP_AUDIO_VOLUME,
  PROP_BUFFER_FILL,
  PROP_CAN_SEEK,
  PROP_DURATION,
  PROP_PLAYING,
  PROP_PROGRESS,
  PROP_SUBTITLE_FONT_NAME,
  PROP_SUBTITLE_URI,
  PROP_URI
};

static void clutter_media_interface_init (ClutterMediaIface *iface);

G_DEFINE_TYPE_WITH_CODE (ClutterGstOverlayActor,
                         clutter_gst_overlay_actor,
                         CLUTTER_TYPE_RECTANGLE,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_MEDIA,
                                                clutter_media_interface_init));

static void
clutter_gst_overlay_actor_show (ClutterActor *self,
                                gpointer user_data)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  XMapWindow (priv->display, priv->window);
}

static void
clutter_gst_overlay_actor_hide (ClutterActor *self,
                                gpointer user_data)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  XUnmapWindow (priv->display, priv->window);
}

static void
clutter_gst_overlay_actor_allocate (ClutterActor *self,
                                    const ClutterActorBox *box,
                                    ClutterAllocationFlags flags,
                                    gpointer user_data)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  XMoveResizeWindow (priv->display, priv->window,
                     box->x1, box->y1,
                     box->x2 - box->x1,
                     box->y2 - box->y1);

  gst_x_overlay_expose (GST_X_OVERLAY (priv->video_sink));
}

static void
clutter_gst_overlay_actor_parent_set (ClutterActor *self,
                                      ClutterActor *old_parent,
                                      gpointer user_data)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;
  ClutterStage *stage_new_parent = CLUTTER_STAGE (clutter_actor_get_stage (self));

  Window window_new_parent = clutter_x11_get_stage_window (stage_new_parent);

  XReparentWindow (priv->display, priv->window,
                   window_new_parent, 0, 0);
}

static void
set_audio_volume (ClutterGstOverlayActor *self,
                  gdouble volume)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  g_object_set (G_OBJECT (priv->pipeline), "volume", volume, NULL);
}

static gdouble
get_audio_volume (ClutterGstOverlayActor *self)
{
  gdouble volume;
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  g_object_get (G_OBJECT (priv->pipeline), "volume", &volume, NULL);

  return volume;
}

static void
set_uri (ClutterGstOverlayActor *self,
         const gchar *uri)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  g_object_set (G_OBJECT (priv->pipeline), "uri", uri, NULL);
}

static gchar *
get_uri (ClutterGstOverlayActor *self)
{
  gchar *uri;
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  g_object_get (G_OBJECT (priv->pipeline), "uri", &uri, NULL);

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
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  if (test_uri (self))
    {
      GstStateChangeReturn state_change = 
        gst_element_set_state (priv->pipeline,
                               playing ? GST_STATE_PLAYING : GST_STATE_PAUSED);

      g_return_if_fail (state_change == GST_STATE_CHANGE_SUCCESS);
    }
  else
    {
      if (playing)
        g_warning ("Unable to start playing: no URI is set\n");
    }
}

static gboolean
get_playing (ClutterGstOverlayActor *self)
{
  gboolean playing;
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;
  GstState state, pending;

  gst_element_get_state (priv->pipeline, &state, &pending, 0);

  playing = pending ? (pending == GST_STATE_PLAYING) :
                      (state   == GST_STATE_PLAYING);

  return playing;
}

static void
set_progress (ClutterGstOverlayActor *self,
              gdouble progress)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  if (test_uri (self))
    {
      gboolean result;

      result = gst_element_seek_simple (priv->pipeline, GST_FORMAT_PERCENT,
                                        GST_SEEK_FLAG_FLUSH | 
                                        GST_SEEK_FLAG_KEY_UNIT,
                                        progress * GST_FORMAT_PERCENT_SCALE);

      if (!result)
        g_warning ("Unable to set progress\n");
    }
  else
    g_warning ("Unable to set progress: no URI is set\n");
}

static gdouble
get_progress (ClutterGstOverlayActor *self)
{
  gint64 progress;
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;
  gboolean result;
  GstFormat format = GST_FORMAT_PERCENT;

  result = gst_element_query_position (priv->pipeline, &format, &progress);

  if (!result)
    {
      g_warning ("Unable to get progress\n");
      return -1;
    }

  if (format != GST_FORMAT_PERCENT)
    {
      g_warning ("It is not format progress\n");
      return -1;
    }

  return (gdouble)progress/(gdouble)GST_FORMAT_PERCENT_SCALE;
}

static void
set_subtitle_uri (ClutterGstOverlayActor *self,
                  const gchar *uri)
{
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  g_object_set (G_OBJECT (priv->pipeline), "suburi", uri, NULL);
}

static gchar *
get_subtitle_uri (ClutterGstOverlayActor *self)
{
  gchar *uri;
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  g_object_get (G_OBJECT (self), "suburi", &uri, NULL);

  return uri;
}

static void
clutter_media_interface_init (ClutterMediaIface *iface)
{
}

static void
clutter_gst_overlay_actor_init (ClutterGstOverlayActor *self)
{
  self->priv = CLUTTER_GST_OVERLAY_ACTOR_GET_PRIVATE (self);
}

static void
clutter_gst_overlay_actor_class_init (ClutterGstOverlayActorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (ClutterGstOverlayActorPrivate));
}

static gboolean
bus_call (GstBus *bus,
          GstMessage *msg,
          gpointer data)
{
  switch (GST_MESSAGE_TYPE (msg)) {

  case GST_MESSAGE_EOS: {
    g_print ("End of stream\n");
    break;
  }

  case GST_MESSAGE_ERROR: {
    gchar *debug;
    GError *error;

    gst_message_parse_error (msg, &error, &debug);
    g_free (debug);

    g_printerr ("Error: %s\n", error->message);
    g_error_free (error);
    break;
  }

  default:
    break;
  }

  return TRUE;
}

ClutterActor *
clutter_gst_overlay_actor_new (void)
{
  ClutterActor *actor = g_object_new (CLUTTER_TYPE_GST_OVERLAY_ACTOR, NULL);
  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (actor)->priv;

  GstElement *pipeline;
  GstElement *video_sink;
  GstBus *bus;

  Display *display = priv->display = (Display*)clutter_x11_get_default_display ();
  Window rootwindow = clutter_x11_get_root_window ();
  int screen = clutter_x11_get_default_screen ();
  Window window;

  /* Used for creating X-window
     independent of the window-manager */
  XSetWindowAttributes attributes;
  attributes.override_redirect = True;
  attributes.background_pixel = BlackPixel(display, screen);
  priv->window = window = XCreateWindow (display, rootwindow, 0, 0, 1, 1, 0, 0, 0, 0,
                                         CWBackPixel | CWOverrideRedirect, &attributes);

  XSync (display, FALSE);

  priv->pipeline   = pipeline   = gst_element_factory_make ("playbin2",
                                                            "pipeline");
  priv->video_sink = video_sink = gst_element_factory_make ("ximagesink",
                                                            "window");

  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (bus, bus_call, NULL);
  gst_object_unref (bus);

  g_signal_connect (actor, "show", G_CALLBACK (clutter_gst_overlay_actor_show), NULL);
  g_signal_connect (actor, "hide", G_CALLBACK (clutter_gst_overlay_actor_hide), NULL);
  g_signal_connect (actor, "allocation-changed", G_CALLBACK (clutter_gst_overlay_actor_allocate), NULL);
  g_signal_connect (actor, "parent-set", G_CALLBACK (clutter_gst_overlay_actor_parent_set), NULL);

  gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (video_sink), window);

  g_object_set (G_OBJECT (pipeline), "video-sink", video_sink, NULL);

  return actor;
}

ClutterActor *
clutter_gst_overlay_actor_new_with_uri (const gchar *uri)
{
  ClutterActor *actor = clutter_gst_overlay_actor_new ();

  set_uri (CLUTTER_GST_OVERLAY_ACTOR (actor), uri);

  return actor;
}

void
clutter_gst_overlay_actor_play (ClutterGstOverlayActor *self)
{
  g_return_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (self));

  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);
}

void
clutter_gst_overlay_actor_pause (ClutterGstOverlayActor *self)
{
  g_return_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (self));

  ClutterGstOverlayActorPrivate *priv = CLUTTER_GST_OVERLAY_ACTOR (self)->priv;

  gst_element_set_state (priv->pipeline, GST_STATE_PAUSED);
}

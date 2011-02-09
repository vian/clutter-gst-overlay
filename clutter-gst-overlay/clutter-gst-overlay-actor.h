/*
 * clutter-gst-overlay.
 *
 * Clutter actor controlling GStreamer window.
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CLUTTER_GST_OVERLAY_ACTOR_H__
#define __CLUTTER_GST_OVERLAY_ACTOR_H__

/* clutter-gst-overlay-actor.h */

#include <glib-object.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11-texture-pixmap.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define CLUTTER_TYPE_GST_OVERLAY_ACTOR (clutter_gst_overlay_actor_get_type ())

#define CLUTTER_GST_OVERLAY_ACTOR(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	CLUTTER_TYPE_GST_OVERLAY_ACTOR, ClutterGstOverlayActor))

#define CLUTTER_GST_OVERLAY_ACTOR_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	CLUTTER_TYPE_GST_OVERLAY_ACTOR, ClutterGstOverlayActorClass))

#define CLUTTER_IS_GST_OVERLAY_ACTOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	CLUTTER_TYPE_GST_OVERLAY_ACTOR))

#define CLUTTER_IS_GST_OVERLAY_ACTOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
	CLUTTER_TYPE_GST_OVERLAY_ACTOR))

#define CLUTTER_GST_OVERLAY_ACTOR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
	CLUTTER_TYPE_GST_OVERLAY_ACTOR, ClutterGstOverlayActorClass))

typedef struct _ClutterGstOverlayActor         ClutterGstOverlayActor;
typedef struct _ClutterGstOverlayActorClass    ClutterGstOverlayActorClass;
typedef struct _ClutterGstOverlayActorPrivate  ClutterGstOverlayActorPrivate;

struct _ClutterGstOverlayActor
{
  ClutterX11TexturePixmap         parent;
  ClutterGstOverlayActorPrivate  *priv;
};

struct _ClutterGstOverlayActorClass
{
  ClutterX11TexturePixmapClass parent_class;

  /* Future padding */
  void (* _clutter_reserved1) (void);
  void (* _clutter_reserved2) (void);
  void (* _clutter_reserved3) (void);
  void (* _clutter_reserved4) (void);
  void (* _clutter_reserved5) (void);
  void (* _clutter_reserved6) (void);
};

typedef enum {
  CLUTTER_GST_OVERLAY_STATE_NULL    = (1 << 0),
  CLUTTER_GST_OVERLAY_STATE_PLAYING = (1 << 1),
  CLUTTER_GST_OVERLAY_STATE_LOADING = (1 << 2),
  CLUTTER_GST_OVERLAY_STATE_ENDED   = (1 << 3)
} ClutterGstOverlayStates;

GType                      clutter_gst_overlay_actor_get_type                      (void) G_GNUC_CONST;
ClutterActor *             clutter_gst_overlay_actor_new                           (void);
ClutterActor *             clutter_gst_overlay_actor_new_with_uri                  (const gchar *uri);
void                       clutter_gst_overlay_actor_play                          (ClutterGstOverlayActor *self);
void                       clutter_gst_overlay_actor_pause                         (ClutterGstOverlayActor *self);
void                       clutter_gst_overlay_actor_stop                          (ClutterGstOverlayActor *self);
void                       clutter_gst_overlay_actor_set_mute                      (ClutterGstOverlayActor *self, gboolean mute);
gboolean                   clutter_gst_overlay_actor_get_mute                      (ClutterGstOverlayActor *self);
void                       clutter_gst_overlay_actor_set_subtitle_flag             (ClutterGstOverlayActor *self, gboolean flag);
gboolean                   clutter_gst_overlay_actor_get_subtitle_flag             (ClutterGstOverlayActor *self);
gboolean                   clutter_gst_overlay_actor_get_video_size                (ClutterGstOverlayActor *self, gint *width, gint *height);
ClutterGstOverlayStates    clutter_gst_overlay_actor_get_states                    (ClutterGstOverlayActor *self);

G_END_DECLS

#endif /* __CLUTTER_GST_OVERLAY_ACTOR_H__ */

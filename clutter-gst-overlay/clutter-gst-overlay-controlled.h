/*
 * clutter-gst-overlay.
 *
 * Clutter container actor with embedded video view (or nothing, if playing audio)
 * and controls view.
 *
 * Authored By Viatcheslav Gachkaylo  <vgachkaylo@crystalnix.com>
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

#ifndef __CLUTTER_GST_OVERLAY_CONTROLLED_H__
#define __CLUTTER_GST_OVERLAY_CONTROLLED_H__

/* clutter-gst-overlay-actor.h */

#include <glib-object.h>
#include <clutter/clutter.h>
#include "clutter-gst-overlay-actor.h"

G_BEGIN_DECLS

#define CLUTTER_TYPE_GST_OVERLAY_CONTROLLED (clutter_gst_overlay_controlled_get_type ())

#define CLUTTER_GST_OVERLAY_CONTROLLED(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
	CLUTTER_TYPE_GST_OVERLAY_CONTROLLED, ClutterGstOverlayControlled))

#define CLUTTER_GST_OVERLAY_CONTROLLED_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST ((klass), \
	CLUTTER_TYPE_GST_OVERLAY_CONTROLLED, ClutterGstOverlayControlledClass))

#define CLUTTER_IS_GST_OVERLAY_CONTROLLED(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
	CLUTTER_TYPE_GST_OVERLAY_CONTROLLED))

#define CLUTTER_IS_GST_OVERLAY_CONTROLLED_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
	CLUTTER_TYPE_GST_OVERLAY_CONTROLLED))

#define CLUTTER_GST_OVERLAY_CONTROLLED_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
	CLUTTER_TYPE_GST_OVERLAY_CONTROLLED, ClutterGstOverlayControlledClass))

typedef struct _ClutterGstOverlayControlled         ClutterGstOverlayControlled;
typedef struct _ClutterGstOverlayControlledClass    ClutterGstOverlayControlledClass;
typedef struct _ClutterGstOverlayControlledPrivate  ClutterGstOverlayControlledPrivate;

struct _ClutterGstOverlayControlled
{
  ClutterBox                parent;
  ClutterGstOverlayControlledPrivate  *priv;
};

struct _ClutterGstOverlayControlledClass
{
  ClutterBoxClass parent_class;

  /* Future padding */
  void (* _clutter_reserved1) (void);
  void (* _clutter_reserved2) (void);
  void (* _clutter_reserved3) (void);
  void (* _clutter_reserved4) (void);
  void (* _clutter_reserved5) (void);
  void (* _clutter_reserved6) (void);
};

GType                     clutter_gst_overlay_controlled_get_type               (void) G_GNUC_CONST;
ClutterActor *            clutter_gst_overlay_controlled_new                    (ClutterGstOverlayActor *video_actor,
                                                                                 ClutterTexture *controls_texture);
ClutterGstOverlayActor *  clutter_gst_overlay_controlled_get_video_actor        (ClutterGstOverlayControlled *self);
void                      clutter_gst_overlay_controlled_set_video_actor        (ClutterGstOverlayControlled *self,
                                                                                 ClutterGstOverlayActor *video_actor);
void                      clutter_gst_overlay_controlled_set_controls_texture   (ClutterGstOverlayControlled *self,
                                                                                 ClutterTexture *controls_texture);

G_END_DECLS

#endif /* __CLUTTER_GST_OVERLAY_CONTROLLED_H__ */

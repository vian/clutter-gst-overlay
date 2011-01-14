/*
 * clutter-gst-overlay.
 *
 * Clutter actor controlling GStreamer window.
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

#ifndef __CLUTTER_GST_OVERLAY_ACTOR_H__
#define __CLUTTER_GST_OVERLAY_ACTOR_H__

/* clutter-gst-overlay-actor.h */

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define CLUTTER_TYPE_GST_OVERLAY_ACTOR  			(clutter_gst_overlay_actor_get_type ())
#define CLUTTER_GST_OVERLAY_ACTOR(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLUTTER_TYPE_GST_OVERLAY_ACTOR, ClutterGstOverlayActor))
#define CLUTTER_GST_OVERLAY_ACTOR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), CLUTTER_TYPE_GST_OVERLAY_ACTOR, ClutterGstOverlayActorClass))
#define CLUTTER_IS_GST_OVERLAY_ACTOR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLUTTER_TYPE_GST_OVERLAY_ACTOR))
#define CLUTTER_IS_GST_OVERLAY_ACTOR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), CLUTTER_TYPE_GST_OVERLAY_ACTOR))
#define CLUTTER_GST_OVERLAY_ACTOR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), CLUTTER_TYPE_GST_OVERLAY_ACTOR, ClutterGstOverlayActorClass))

G_END_DECLS

#endif /* __CLUTTER_GST_OVERLAY_ACTOR_H__ */
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
 
#include "clutter-gst-overlay-controlled.h"

#define CLUTTER_GST_OVERLAY_CONTROLLED_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        CLUTTER_TYPE_GST_OVERLAY_CONTROLLED, ClutterGstOverlayControlledPrivate))

struct _ClutterGstOverlayControlledPrivate
{
  ClutterGstOverlayActor *video_actor;
  ClutterBox             *controls_actor;
  ClutterTexture         *controls_texture;
};

enum {
  PROP_0,

  /* For ClutterMedia interface */
  PROP_VIDEO_ACTOR,
  PROP_CONTROLS_TEXTURE
};

G_DEFINE_TYPE (ClutterGstOverlayControlled,
               clutter_gst_overlay_controlled,
               CLUTTER_TYPE_BOX);

static void
clutter_gst_overlay_controlled_dispose (GObject *gobject)
{
  G_OBJECT_CLASS (clutter_gst_overlay_controlled_parent_class)->dispose (gobject);
}

static void
clutter_gst_overlay_controlled_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (clutter_gst_overlay_controlled_parent_class)->finalize (gobject);
}

ClutterActor *
clutter_gst_overlay_controlled_new (ClutterGstOverlayActor *video_actor,
                                    ClutterTexture *controls_texture)
{
  ClutterLayoutManager *main_layout = clutter_box_layout_new();
  clutter_box_layout_set_vertical(CLUTTER_BOX_LAYOUT(main_layout), TRUE);
  
  return g_object_new (CLUTTER_TYPE_GST_OVERLAY_CONTROLLED,
                       "layout-manager", main_layout,  
                       "video-actor", video_actor, 
                       "controls-texture", controls_texture,
                       NULL);
}

static void
clutter_gst_overlay_controlled_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  ClutterGstOverlayControlled *self = CLUTTER_GST_OVERLAY_CONTROLLED (object);

  switch (property_id)
    {
    case PROP_VIDEO_ACTOR:
      g_value_set_object (value, clutter_gst_overlay_controlled_get_video_actor (self));
      break;
      
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clutter_gst_overlay_controlled_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  ClutterGstOverlayControlled *self = CLUTTER_GST_OVERLAY_CONTROLLED (object);

  switch (property_id)
    {
    case PROP_VIDEO_ACTOR:
      clutter_gst_overlay_controlled_set_video_actor
        (self, 
         CLUTTER_GST_OVERLAY_ACTOR (g_value_get_object (value)));
      break;

    case PROP_CONTROLS_TEXTURE:
      clutter_gst_overlay_controlled_set_controls_texture
        (self,
         CLUTTER_TEXTURE (g_value_get_object (value)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clutter_gst_overlay_controlled_init (ClutterGstOverlayControlled *self)
{
  ClutterGstOverlayControlledPrivate *priv;
  
  self->priv = priv = CLUTTER_GST_OVERLAY_CONTROLLED_GET_PRIVATE (self);
}

static void
clutter_gst_overlay_controlled_class_init (ClutterGstOverlayControlledClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (ClutterGstOverlayControlledPrivate));

  gobject_class->dispose = clutter_gst_overlay_controlled_dispose;
  gobject_class->finalize = clutter_gst_overlay_controlled_finalize;
  gobject_class->set_property = clutter_gst_overlay_controlled_set_property;
  gobject_class->get_property = clutter_gst_overlay_controlled_get_property;

  pspec = g_param_spec_object ("video-actor",
                               "Video actor",
                               "An instance of ClutterGstOverlayActor used for playing video",
                               CLUTTER_TYPE_GST_OVERLAY_ACTOR,
                               G_PARAM_READWRITE);
                            
  g_object_class_install_property (gobject_class,
                                   PROP_VIDEO_ACTOR, pspec);

  pspec = g_param_spec_object ("controls-texture",
                               "Controls texture",
                               "A player controls UI ClutterTexture",
                               CLUTTER_TYPE_TEXTURE,
                               G_PARAM_WRITABLE);
                               
  g_object_class_install_property (gobject_class,
                                   PROP_CONTROLS_TEXTURE, pspec);
}

ClutterGstOverlayActor *
clutter_gst_overlay_controlled_get_video_actor (ClutterGstOverlayControlled *self)
{
  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_CONTROLLED (self), NULL);
  
  ClutterGstOverlayControlledPrivate *priv = CLUTTER_GST_OVERLAY_CONTROLLED_GET_PRIVATE (self);
  
  return priv->video_actor;
}

void 
clutter_gst_overlay_controlled_set_video_actor (ClutterGstOverlayControlled *self,
                                                ClutterGstOverlayActor *video_actor)
{
  g_return_if_fail (CLUTTER_IS_GST_OVERLAY_CONTROLLED (self));
  
  ClutterGstOverlayControlledPrivate *priv = CLUTTER_GST_OVERLAY_CONTROLLED_GET_PRIVATE (self);
  
  if (CLUTTER_IS_GST_OVERLAY_ACTOR (priv->video_actor))
    clutter_container_remove_actor (CLUTTER_CONTAINER (self), CLUTTER_ACTOR (priv->video_actor));
  
  if (CLUTTER_IS_GST_OVERLAY_ACTOR (video_actor))
    clutter_box_pack_at (CLUTER_BOX (self), CLUTTER_ACTOR (video_actor), 0,
                         "expand", TRUE,
                         "x-fill", TRUE,
                         "y-fill", TRUE,
                         NULL);
  
  priv->video_actor = video_actor;
}

void
clutter_gst_overlay_controlled_set_controls_texture (ClutterGstOverlayControlled *self,
                                                     ClutterTexture *controls_texture)
{
  g_return_if_fail (CLUTTER_IS_GST_OVERLAY_CONTROLLED (self));
  g_return_if_fail (CLUTTER_IS_TEXTURE (controls_texture));
  
  ClutterGstOverlayControlledPrivate *priv = CLUTTER_GST_OVERLAY_CONTROLLED_GET_PRIVATE (self);
  
  ClutterActor *play_button;
  ClutterActor *pause_button;
  ClutterActor *sound_on_button;
  ClutterActor *sound_off_button;
  ClutterActor *seek_handle;
  ClutterActor *buffered_progress;
  ClutterActor *seek_bar;
  GList        *subtitle_radios;
  ClutterActor *volume_bgr;
  ClutterActor *volume_active;
  
}

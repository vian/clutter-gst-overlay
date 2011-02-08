/*
 * clutter-gst-overlay.
 *
 * Clutter container actor with embedded video view (or nothing, if playing audio)
 * and controls view.
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

#include "clutter-gst-overlay-controlled.h"

#define CLUTTER_GST_OVERLAY_CONTROLLED_GET_PRIVATE(obj) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj), \
        CLUTTER_TYPE_GST_OVERLAY_CONTROLLED, ClutterGstOverlayControlledPrivate))

#define BUTTON_TEXTURE_WIDTH   48
#define BUTTON_TEXTURE_HEIGHT  48
#define HANDLER_TEXTURE_WIDTH  (BUTTON_TEXTURE_WIDTH * 2)
#define HANDLER_TEXTURE_HEIGHT (BUTTON_TEXTURE_HEIGHT * 1)

#define HANDLER_TEXTURE_LEFT_OFFSET  10
#define HANDLER_TEXTURE_RIGHT_OFFSET 14

#define PLAY_BUTTON_X          0
#define PAUSE_BUTTON_X         (BUTTON_TEXTURE_WIDTH * 1)
#define SOUND_OFF_BUTTON_X     (BUTTON_TEXTURE_WIDTH * 2)
#define SOUND_ON_BUTTON_X      (BUTTON_TEXTURE_WIDTH * 3)
#define EMPTY_HANDLER_X        (BUTTON_TEXTURE_WIDTH * 4)
#define FULL_HANDLER_X         (BUTTON_TEXTURE_WIDTH * 4 + HANDLER_TEXTURE_WIDTH)

#define PLAY_PAUSE_BUTTON_POS     0
#define SOUND_ON_OFF_BUTTON_POS   1

struct _ClutterGstOverlayControlledPrivate
{
  ClutterGstOverlayActor *video_actor;
  ClutterBox             *controls_actor;
  ClutterTexture         *controls_texture;

  ClutterActor *play_button;
  ClutterActor *pause_button;
  ClutterActor *sound_on_button;
  ClutterActor *sound_off_button;
  ClutterBox   *seek_bar_container;
  ClutterActor *seek_handle;
  ClutterActor *buffered_progress;
  ClutterActor *seek_bar;
  GList        *subtitle_radios;
  ClutterActor *volume_bgr;
  ClutterActor *volume_active;
  ClutterBox   *volume_box;
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
update_subtitles_control (ClutterGstOverlayControlled *self);

static void
create_subtitles_control (ClutterGstOverlayControlled *self);

static void
clutter_gst_overlay_controlled_dispose (GObject *gobject)
{
  ClutterGstOverlayControlled *self = CLUTTER_GST_OVERLAY_CONTROLLED (gobject);
  ClutterGstOverlayControlledPrivate *priv = self->priv;

  if (priv->play_button != NULL)
    {
      g_object_unref (priv->play_button);
      priv->play_button = NULL;
    }

  if (priv->pause_button != NULL)
    {
      g_object_unref (priv->pause_button);
      priv->pause_button = NULL;
    }

  if (priv->sound_on_button != NULL)
    {
      g_object_unref (priv->sound_on_button);
      priv->sound_on_button = NULL;
    }

  if (priv->sound_off_button != NULL)
    {
      g_object_unref (priv->sound_off_button);
      priv->sound_off_button = NULL;
    }

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
  ClutterLayoutManager *layout = clutter_box_layout_new();
  ClutterColor color = { 0xFF, 0xFF, 0xFF, 0xFF };

  self->priv = priv = CLUTTER_GST_OVERLAY_CONTROLLED_GET_PRIVATE (self);

  priv->play_button = NULL;
  priv->pause_button = NULL;
  priv->sound_on_button = NULL;
  priv->sound_off_button = NULL;
  priv->subtitle_radios = NULL;

  priv->controls_actor = CLUTTER_BOX (clutter_box_new (layout));
  clutter_box_set_color (priv->controls_actor, &color);
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

  return self->priv->video_actor;
}

void
clutter_gst_overlay_controlled_set_video_actor (ClutterGstOverlayControlled *self,
                                                ClutterGstOverlayActor *video_actor)
{
  g_return_if_fail (CLUTTER_IS_GST_OVERLAY_CONTROLLED (self));

  ClutterGstOverlayControlledPrivate *priv = self->priv;

  if (CLUTTER_IS_GST_OVERLAY_ACTOR (priv->video_actor))
    clutter_container_remove_actor (CLUTTER_CONTAINER (self), CLUTTER_ACTOR (priv->video_actor));

  if (CLUTTER_IS_GST_OVERLAY_ACTOR (video_actor))
    clutter_box_pack_at (CLUTTER_BOX (self), CLUTTER_ACTOR (video_actor), 0,
                         "expand", TRUE,
                         "x-fill", TRUE,
                         "y-fill", TRUE,
                         NULL);

  priv->video_actor = video_actor;
}

static ClutterActor *
get_sub_texture (ClutterTexture *texture,
                 int sub_x,
                 int sub_y,
                 int sub_w,
                 int sub_h)
{
  CoglHandle *full_texture = clutter_texture_get_cogl_texture (texture);
  ClutterActor *new_texture = clutter_texture_new ();
  CoglHandle *sub_texture = cogl_texture_new_from_sub_texture (full_texture,
                                                               sub_x, sub_y,
                                                               sub_w, sub_h);
  clutter_texture_set_cogl_texture (CLUTTER_TEXTURE (new_texture), sub_texture);
  return new_texture;
}

static void
create_button_actor (ClutterActor *button,
                     GCallback     c_handle,
                     gpointer      user_data)
{
  clutter_actor_set_reactive (button, TRUE);

  g_signal_connect (button, "button-press-event", c_handle, user_data);
}

static ClutterActor *
create_button_from_texture (ClutterTexture *full_texture,
                            int             sub_x,
                            int             sub_y,
                            int             sub_w,
                            int             sub_h,
                            GCallback       c_handle,
                            gpointer        user_data)
{
  ClutterActor * new_button = get_sub_texture (full_texture,
                                               sub_x, sub_y,
                                               sub_w, sub_h);

  create_button_actor (new_button, c_handle, user_data);

  return new_button;
}

static gboolean
play_media (ClutterActor *actor,
            ClutterEvent *event,
            gpointer      user_data)
{
  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_CONTROLLED (user_data), FALSE);

  ClutterGstOverlayControlled *self = CLUTTER_GST_OVERLAY_CONTROLLED(user_data);
  ClutterGstOverlayControlledPrivate *priv = self->priv;

  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (priv->video_actor),
                        FALSE);
  g_return_val_if_fail (CLUTTER_IS_BOX (priv->controls_actor), FALSE);

  clutter_container_remove_actor (CLUTTER_CONTAINER (priv->controls_actor),
                                  priv->play_button);

  clutter_box_pack_at (priv->controls_actor, priv->pause_button,
                       PLAY_PAUSE_BUTTON_POS, NULL, NULL);

  clutter_media_set_playing (CLUTTER_MEDIA (priv->video_actor), TRUE);

  create_subtitles_control (self);

  return TRUE;
}

static gboolean
pause_media (ClutterActor *actor,
             ClutterEvent *event,
             gpointer      user_data)
{
  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_CONTROLLED (user_data), FALSE);

  ClutterGstOverlayControlled *self = CLUTTER_GST_OVERLAY_CONTROLLED(user_data);
  ClutterGstOverlayControlledPrivate *priv = self->priv;

  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (priv->video_actor),
                        FALSE);
  g_return_val_if_fail (CLUTTER_IS_BOX (priv->controls_actor), FALSE);

  clutter_container_remove_actor (CLUTTER_CONTAINER (priv->controls_actor),
                                  priv->pause_button);

  clutter_box_pack_at (priv->controls_actor, priv->play_button,
                       PLAY_PAUSE_BUTTON_POS, NULL, NULL);

  clutter_media_set_playing (CLUTTER_MEDIA (priv->video_actor), FALSE);

  return TRUE;
}

static gboolean
sound_on_media (ClutterActor *actor,
                ClutterEvent *event,
                gpointer      user_data)
{
  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_CONTROLLED (user_data), FALSE);

  ClutterGstOverlayControlled *self = CLUTTER_GST_OVERLAY_CONTROLLED(user_data);
  ClutterGstOverlayControlledPrivate *priv = self->priv;

  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (priv->video_actor),
                        FALSE);
  g_return_val_if_fail (CLUTTER_IS_BOX (priv->controls_actor), FALSE);

  clutter_container_remove_actor (CLUTTER_CONTAINER (priv->controls_actor),
                                  priv->sound_on_button);

  clutter_box_pack_at (priv->controls_actor, priv->sound_off_button,
                       SOUND_ON_OFF_BUTTON_POS, NULL, NULL);

  clutter_gst_overlay_actor_set_mute (priv->video_actor, FALSE);

  return TRUE;
}

static gboolean
sound_off_media (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      user_data)
{
  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_CONTROLLED (user_data), FALSE);

  ClutterGstOverlayControlled *self = CLUTTER_GST_OVERLAY_CONTROLLED(user_data);
  ClutterGstOverlayControlledPrivate *priv = self->priv;

  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (priv->video_actor),
                        FALSE);
  g_return_val_if_fail (CLUTTER_IS_BOX (priv->controls_actor), FALSE);

  clutter_container_remove_actor (CLUTTER_CONTAINER (priv->controls_actor),
                                  priv->sound_off_button);

  clutter_box_pack_at (priv->controls_actor, priv->sound_on_button,
                       SOUND_ON_OFF_BUTTON_POS, NULL, NULL);

  clutter_gst_overlay_actor_set_mute (priv->video_actor, TRUE);

  return TRUE;
}

static gboolean
set_volume_media (ClutterActor *actor,
                  ClutterEvent *event,
                  gpointer      user_data)
{
  gfloat x, y, width, height;
  gfloat left_border, right_border;
  gdouble volume_value;
  ClutterGstOverlayControlled *slf = CLUTTER_GST_OVERLAY_CONTROLLED (user_data);
  ClutterMedia *media = CLUTTER_MEDIA (slf->priv->video_actor);

  clutter_actor_get_size (actor, &width, &height);
  clutter_event_get_coords (event, &x, &y);
  clutter_actor_transform_stage_point (actor, x, y, &x, &y);
  left_border = HANDLER_TEXTURE_LEFT_OFFSET;

  right_border = width - HANDLER_TEXTURE_RIGHT_OFFSET;

  if (x <= left_border)
    volume_value = 0.0;
  else
    if (x >= right_border)
      volume_value = 1.0;
    else
      volume_value = (x - left_border) / (right_border - left_border);

  clutter_media_set_audio_volume (media, volume_value);

  clutter_actor_set_clip (slf->priv->volume_active, 0, 0, x, height);

  return TRUE;
}

static gboolean
seek_clicked_cb (ClutterActor *actor,
                 ClutterEvent *event,
                 gpointer      user_data)
{
  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_CONTROLLED (user_data), FALSE);

  ClutterGstOverlayControlled *self = CLUTTER_GST_OVERLAY_CONTROLLED(user_data);
  ClutterGstOverlayControlledPrivate *priv = self->priv;

  g_return_val_if_fail (CLUTTER_IS_GST_OVERLAY_ACTOR (priv->video_actor), FALSE);

  // TODO: move the slider, set play progress
  return TRUE;
}

static void
remove_control_cb(ClutterActor *actor, gpointer data)
{
  clutter_container_remove_actor(CLUTTER_CONTAINER(data), actor);
}

static ClutterActor *
create_subtitle_radio (const gchar *text)
{
  ClutterLayoutManager *layout;
  ClutterBox *box;
  ClutterActor *rect;
  ClutterActor *text_actor;

  layout = clutter_bin_layout_new (CLUTTER_BIN_ALIGNMENT_CENTER,
                                   CLUTTER_BIN_ALIGNMENT_CENTER);

  box = CLUTTER_BOX (clutter_box_new (layout));

  rect = clutter_rectangle_new ();
  clutter_actor_set_size (rect, BUTTON_TEXTURE_WIDTH, BUTTON_TEXTURE_HEIGHT);

  text_actor = clutter_text_new_with_text ("Sans 16", text);

  clutter_bin_layout_add (CLUTTER_BIN_LAYOUT (layout), rect,
                          CLUTTER_BIN_ALIGNMENT_CENTER,
                          CLUTTER_BIN_ALIGNMENT_CENTER);

  clutter_bin_layout_add (CLUTTER_BIN_LAYOUT (layout), text_actor,
                          CLUTTER_BIN_ALIGNMENT_CENTER,
                          CLUTTER_BIN_ALIGNMENT_CENTER);

  return CLUTTER_ACTOR (box);
}

static void
subtitle_button_set_color (ClutterActor       *actor,
                           const ClutterColor *color)
{
  ClutterBox *subtitle_box = CLUTTER_BOX (actor);
  GList *children = clutter_container_get_children (CLUTTER_CONTAINER (actor));
  GList *rect = children;

  while (rect != NULL && !CLUTTER_IS_RECTANGLE (rect->data))
    rect = g_list_next (rect);

  if (children != NULL)
    clutter_rectangle_set_color (CLUTTER_RECTANGLE (rect->data), color);

  g_list_free (children);
}

static gboolean
change_current_subtitle (ClutterActor *actor,
                         ClutterEvent *event,
                         gpointer      user_data)
{
  ClutterGstOverlayControlled *slf = CLUTTER_GST_OVERLAY_CONTROLLED (user_data);
  ClutterGstOverlayControlledPrivate *priv = slf->priv;
  gint index = g_list_index (priv->subtitle_radios, actor);

  if (index == 0)
    {
      clutter_gst_overlay_actor_set_subtitle_flag (priv->video_actor, FALSE);
    }
  else
    {
      g_object_set (G_OBJECT (priv->video_actor),
                    "current-text", index - 1, NULL);
      clutter_gst_overlay_actor_set_subtitle_flag (priv->video_actor, TRUE);
    }

  update_subtitles_control (slf);

  return TRUE;
}

static void
subtitle_button_destroy (gpointer data,
                         gpointer user_data)
{
  if (CLUTTER_IS_BOX (data))
    clutter_actor_destroy (CLUTTER_ACTOR (data));
}

static void
update_subtitles_control (ClutterGstOverlayControlled *self)
{
  ClutterGstOverlayControlledPrivate *priv = self->priv;
  gint n_text, current_text, i;
  ClutterActor *subtitle_radio;
  gboolean flag = clutter_gst_overlay_actor_get_subtitle_flag (priv->video_actor);
  static ClutterColor active_subtitle_radio = { 0xaa, 0xaa, 0xaa, 0xff };
  static ClutterColor passive_subtitle_radio = { 0xcc, 0xcc, 0xcc, 0xff };

  g_object_get (G_OBJECT (priv->video_actor),
                "n-text", &n_text,
                "current-text", &current_text,
                NULL);

  for (i = 0; i <= n_text; ++i)
    {
      subtitle_radio = CLUTTER_ACTOR (g_list_nth (priv->subtitle_radios, 
                                                  i)->data);
      if ((!flag && i == 0) ||
          (flag && i == current_text + 1))

        {
          subtitle_button_set_color (subtitle_radio,
                                     &active_subtitle_radio);
        }
      else
        subtitle_button_set_color (subtitle_radio,
                                   &passive_subtitle_radio);
    }
}

static void
create_subtitles_control (ClutterGstOverlayControlled *self)
{
  ClutterGstOverlayControlledPrivate *priv = self->priv;
  gint n_text, current_text, i;
  ClutterActor *subtitle_radio;
  gchar *text;

  g_object_get (G_OBJECT (priv->video_actor),
                "n-text", &n_text,
                "current-text", &current_text,
                NULL);

  g_list_foreach (priv->subtitle_radios, subtitle_button_destroy, NULL);
  //  g_list_free (priv->subtitle_radios);

  for (i = 0; i <= n_text; ++i)
    {
      text = (i == 0) ? g_strdup_printf ("DS") : g_strdup_printf ("S%d", i);
      subtitle_radio = create_subtitle_radio (text);
      g_free (text);

      priv->subtitle_radios = g_list_append (priv->subtitle_radios,
                                             subtitle_radio);

      create_button_actor (subtitle_radio,
                           G_CALLBACK (change_current_subtitle), self);

      clutter_box_pack (CLUTTER_BOX (priv->controls_actor),
                        subtitle_radio,
                        NULL, NULL);
    }

  update_subtitles_control (self);
}

void
clutter_gst_overlay_controlled_set_controls_texture (ClutterGstOverlayControlled *self,
                                                     ClutterTexture *controls_texture)
{
  g_return_if_fail (CLUTTER_IS_GST_OVERLAY_CONTROLLED (self));
  g_return_if_fail (CLUTTER_IS_TEXTURE (controls_texture));

  ClutterGstOverlayControlledPrivate *priv = self->priv;
  ClutterGstOverlayActor *video_actor = self->priv->video_actor;
  ClutterLayoutManager *seek_bar_layout;
  ClutterLayoutManager *volume_layout;

  static ClutterColor seek_bar_color = { 0xcc, 0xcc, 0xcc, 0xff };
  static ClutterColor buffered_progress_color = { 0xff, 0xf0, 0xf0, 0xff };

  if (CLUTTER_IS_TEXTURE (priv->controls_texture))
    {
      if (priv->controls_texture == controls_texture)
        return;

      clutter_container_foreach (CLUTTER_CONTAINER (priv->controls_actor),
                                 remove_control_cb,
                                 priv->controls_actor);

      g_object_unref (priv->pause_button);
      g_object_unref (priv->play_button);
      g_object_unref (priv->sound_on_button);
      g_object_unref (priv->sound_off_button);
    }

  /* Creating control buttons */
  priv->play_button = create_button_from_texture (controls_texture,
                                                  PLAY_BUTTON_X, 0,
                                                  BUTTON_TEXTURE_WIDTH,
                                                  BUTTON_TEXTURE_HEIGHT,
                                                  G_CALLBACK (play_media),
                                                  self);

  priv->pause_button = create_button_from_texture (controls_texture,
                                                   PAUSE_BUTTON_X, 0,
                                                   BUTTON_TEXTURE_WIDTH,
                                                   BUTTON_TEXTURE_HEIGHT,
                                                   G_CALLBACK (pause_media),
                                                   self);

  priv->sound_on_button = create_button_from_texture (controls_texture,
                                                      SOUND_ON_BUTTON_X, 0,
                                                      BUTTON_TEXTURE_WIDTH,
                                                      BUTTON_TEXTURE_HEIGHT,
                                                      G_CALLBACK (sound_on_media),
                                                      self);

  priv->sound_off_button = create_button_from_texture (controls_texture,
                                                       SOUND_OFF_BUTTON_X, 0,
                                                       BUTTON_TEXTURE_WIDTH,
                                                       BUTTON_TEXTURE_HEIGHT,
                                                       G_CALLBACK (sound_off_media),
                                                       self);

  g_object_ref (priv->pause_button);
  g_object_ref (priv->play_button);
  g_object_ref (priv->sound_on_button);
  g_object_ref (priv->sound_off_button);

  // TODO: remove memory leaks (when setting texture again)
  seek_bar_layout = clutter_bin_layout_new (CLUTTER_BIN_ALIGNMENT_FIXED, CLUTTER_BIN_ALIGNMENT_FIXED);
  priv->seek_bar_container = CLUTTER_BOX (clutter_box_new (seek_bar_layout));

  priv->seek_bar = clutter_rectangle_new_with_color (&seek_bar_color);
  clutter_actor_set_height (priv->seek_bar, 8);
  clutter_box_pack (priv->seek_bar_container, priv->seek_bar,
                    "x-align", CLUTTER_BIN_ALIGNMENT_FILL,
                    "y-align", CLUTTER_BIN_ALIGNMENT_CENTER,
                    NULL);
  create_button_actor(priv->seek_bar, G_CALLBACK(seek_clicked_cb), self);


  priv->buffered_progress = clutter_rectangle_new_with_color (&buffered_progress_color);
  // TODO: add appropriate width (how much of the stream is buffered now)
  clutter_actor_set_y(priv->buffered_progress, 17);
  clutter_actor_set_height(priv->buffered_progress, 3);

  clutter_box_pack (priv->seek_bar_container, priv->buffered_progress,
                    "x-align", CLUTTER_BIN_ALIGNMENT_FIXED,
                    "y-align", CLUTTER_BIN_ALIGNMENT_FIXED,
                    NULL);

  /* Creating volume handle */
  volume_layout = clutter_bin_layout_new (CLUTTER_BIN_ALIGNMENT_CENTER,
                                          CLUTTER_BIN_ALIGNMENT_CENTER);

  priv->volume_box = CLUTTER_BOX (clutter_box_new (volume_layout));

  priv->volume_bgr = get_sub_texture (controls_texture,
                                      EMPTY_HANDLER_X, 0,
                                      HANDLER_TEXTURE_WIDTH,
                                      HANDLER_TEXTURE_HEIGHT);

  priv->volume_active = get_sub_texture (controls_texture,
                                         FULL_HANDLER_X, 0,
                                         HANDLER_TEXTURE_WIDTH,
                                         HANDLER_TEXTURE_HEIGHT);

  clutter_bin_layout_add (CLUTTER_BIN_LAYOUT (volume_layout),
                          priv->volume_bgr,
                          CLUTTER_BIN_ALIGNMENT_CENTER,
                          CLUTTER_BIN_ALIGNMENT_CENTER);

  clutter_bin_layout_add (CLUTTER_BIN_LAYOUT (volume_layout),
                          priv->volume_active,
                          CLUTTER_BIN_ALIGNMENT_CENTER,
                          CLUTTER_BIN_ALIGNMENT_CENTER);

  create_button_actor (CLUTTER_ACTOR (priv->volume_box),
                       G_CALLBACK (set_volume_media),
                       self);

  /* Installing all controls */
  clutter_box_pack (CLUTTER_BOX (self->priv->controls_actor),
                    priv->play_button,
                    NULL, NULL);

  clutter_box_pack (CLUTTER_BOX (self->priv->controls_actor),
                    priv->sound_off_button,
                    NULL, NULL);

  clutter_box_pack (CLUTTER_BOX (self->priv->controls_actor),
                    CLUTTER_ACTOR (priv->seek_bar_container),
                    "expand", FALSE,
                    "x-fill", TRUE,
                    "y-fill", FALSE,
                    NULL);

  clutter_box_pack (CLUTTER_BOX (priv->controls_actor),
                    CLUTTER_ACTOR (priv->volume_box),
                    NULL, NULL);

  //  create_subtitles_control (self);

  clutter_box_pack (CLUTTER_BOX (self),
                    CLUTTER_ACTOR (self->priv->controls_actor),
                    "x-fill", TRUE,
                    NULL);
}

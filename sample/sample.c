/* 

gcc -o sample/sample sample/sample.c clutter-gst-overlay/clutter-gst-overlay-actor.c `pkg-config --libs --cflags clutter-1.0 gstreamer-0.10 gstreamer-interfaces-0.10`

 */


#include <clutter/clutter.h>
#include "../clutter-gst-overlay/clutter-gst-overlay-actor.h"

void close_actor (ClutterMedia *media,
                  gpointer user_data)
{
  clutter_main_quit ();
}
      
gboolean test_uri (gpointer user_data)
{
  ClutterMedia *media = CLUTTER_MEDIA (user_data);
  gchar *uri = clutter_media_get_uri (media);
  gchar *suburi = clutter_media_get_subtitle_uri (media);

  g_print ("My uri: %s\n", uri);
  g_print ("My subtitle uri: %s\n", suburi);

  g_free (uri);
  g_free (suburi);

  return FALSE;
}

gboolean test_func (gpointer user_data)
{
  ClutterMedia *media = CLUTTER_MEDIA (user_data);
  gdouble volume = clutter_media_get_audio_volume (media);

  g_print ("My volume: %g\n", volume);

  //  clutter_media_set_audio_volume (media, (volume <= 0.5) ? 1.0 : 0.0);
  gboolean is_muted;
  g_object_get (G_OBJECT (user_data), "mute", &is_muted, NULL);
  g_object_set (G_OBJECT (user_data), "mute", !is_muted, NULL);
  g_print ("Can seek: ");
  g_print ((clutter_media_get_can_seek (media)) ? "TRUE" : "FALSE");
  g_print ("\n");

  gchar *font = clutter_media_get_subtitle_font_name (media);
  g_print ("My font name: %s\n", font);
  g_free (font);

  gdouble buffer_fill = clutter_media_get_buffer_fill (media);
  g_print ("Buffer fill: %.2f\n", buffer_fill);

  return TRUE;
}

int main (int argc, char *argv[])
{
  ClutterColor stage_color = { 0x00, 0xFF, 0x00, 0xFF };

  if (argc != 2 && argc != 3) {
    g_printerr ("Usage: %s <uri to video-file> <uri to subtitle-file>\n",
                argv[0]);
    return -1;
  }

  clutter_init (&argc, &argv);
  gst_init (&argc, &argv);

  ClutterActor *stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 640, 480);
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

  ClutterActor *rect = clutter_gst_overlay_actor_new ();
  clutter_actor_set_size (rect, 440, 280);
  clutter_actor_set_position (rect, 100, 100);

  //  clutter_media_set_filename (CLUTTER_MEDIA (rect), argv[1]);
  clutter_media_set_uri (CLUTTER_MEDIA (rect), argv[1]);
  clutter_media_set_subtitle_uri (CLUTTER_MEDIA (rect), argv[2]);

  //  ClutterActor *cont = clutter_group_new ();
  //  clutter_actor_set_position (cont, 200, 200);
  //  clutter_container_add_actor (CLUTTER_CONTAINER(cont), rect);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), rect);

  clutter_actor_show (stage);

  clutter_media_set_playing (CLUTTER_MEDIA (rect), TRUE);
  clutter_media_set_subtitle_font_name (CLUTTER_MEDIA (rect), "Sans bold italic 32");
  clutter_media_set_audio_volume (CLUTTER_MEDIA (rect), 0.75);
  //  clutter_media_set_progress (CLUTTER_MEDIA (rect), 0.5);

  g_signal_connect (CLUTTER_MEDIA (rect), "eos",
                    G_CALLBACK (close_actor), NULL);

  g_timeout_add_seconds (0, test_uri, rect);
  g_timeout_add_seconds (5, test_func, rect);

  clutter_main ();

  clutter_actor_destroy (rect);

  return 0;
}

/* 

gcc -o sample/sample sample/sample.c clutter-gst-overlay/clutter-gst-overlay-actor.c `pkg-config --libs --cflags clutter-1.0 gstreamer-0.10 gstreamer-interfaces-0.10`

 */


#include <clutter/clutter.h>
#include "../clutter-gst-overlay/clutter-gst-overlay-actor.h"

/*
static void
restart_video (GstElement *playbin,
               gpointer    user_data)
{
  ClutterGstOverlayActor *gst_actor = CLUTTER_GST_OVERLAY_ACTOR (user_data);
  gchar *uri = get_uri (gst_actor);
  gdouble duration = get_duration (gst_actor);
  //  set_uri (gst_actor, uri);
  g_print ("Restarting... %s %g\n", uri, GST_TIME_AS_SECONDS (duration));

  g_free (uri);
  gst_element_set_state (playbin, GST_STATE_PAUSED);
  set_progress (gst_actor, 0.5);
  gst_element_set_state (playbin, GST_STATE_PLAYING);
}
*/

void my_close (ClutterMedia *media,
            gpointer      user_data)
{
  clutter_media_set_progress (CLUTTER_MEDIA (user_data), 0.0);
}

int main (int argc, char *argv[])
{
  ClutterColor stage_color = { 0x00, 0xFF, 0x00, 0xFF };

  /*  if (argc != 2) {
    g_printerr ("Usage: %s <uri to video-file>\n", argv[0]);
    return -1;
  }
  */
  clutter_init (&argc, &argv);
  gst_init (&argc, &argv);

  ClutterActor *stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 640, 480);
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

  ClutterActor *rect = clutter_gst_overlay_actor_new ();
  clutter_actor_set_size (rect, 440, 280);
  clutter_actor_set_position (rect, 100, 100);

  clutter_media_set_uri (CLUTTER_MEDIA (rect), argv[1]);
  clutter_media_set_subtitle_uri (CLUTTER_MEDIA (rect), argv[2]);

  //  ClutterActor *cont = clutter_group_new ();
  //  clutter_actor_set_position (cont, 200, 200);
  //  clutter_container_add_actor (CLUTTER_CONTAINER(cont), rect);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), rect);

  clutter_actor_show (stage);

  clutter_media_set_playing (CLUTTER_MEDIA (rect), TRUE);
  clutter_media_set_subtitle_font_name (CLUTTER_MEDIA (rect), "Sans bold italic 32");

  g_signal_connect (CLUTTER_MEDIA (rect), "eos",
                    G_CALLBACK (my_close), rect);

  clutter_main ();

  return 0;
}

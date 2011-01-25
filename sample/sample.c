/* 

gcc -o sample/sample sample/sample.c clutter-gst-overlay/clutter-gst-overlay-actor.c `pkg-config --libs --cflags clutter-1.0 gstreamer-0.10 gstreamer-interfaces-0.10`

 */


#include <clutter/clutter.h>
#include "../clutter-gst-overlay/clutter-gst-overlay-actor.h"

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
  g_object_set (G_OBJECT (rect), "uri", argv[1],
		"subtitle-uri", argv[2],
		"subtitle-font-name", "Monospace bold italic 24",
		NULL);

  //  ClutterActor *cont = clutter_group_new ();
  //  clutter_actor_set_position (cont, 200, 200);
  //  clutter_container_add_actor (CLUTTER_CONTAINER(cont), rect);

  clutter_container_add_actor (CLUTTER_CONTAINER (stage), rect);

  clutter_actor_show (stage);

  //clutter_gst_overlay_actor_play (CLUTTER_GST_OVERLAY_ACTOR (rect));
  g_object_set (G_OBJECT (rect), "playing", TRUE, "audio-volume", 1.0, NULL);

  clutter_main ();

  return 0;
}

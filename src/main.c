#include "app.h"

void
populate_canvas (App * app)
{
    GOStyleLine *line = NULL;
    GocGroup *top_level_group;
    GOStyle *style;
    GocItem *item;
    GOArrow *arr;

    GtkWidget *button;

    top_level_group = goc_canvas_get_root (app->canvas);

    button = gtk_button_new_with_label ("System 1");

    goc_item_new (top_level_group, GOC_TYPE_WIDGET, "widget", button, "x",
                  100.0, "y", 200.0, "width", 100.0, "height", 50.0, NULL);

    g_signal_connect (button, "button-press-event",
                      G_CALLBACK (propagate_button_press_event_to_canvas_cb),
                      app);
    g_signal_connect (button, "button-release-event",
                      G_CALLBACK (propagate_button_release_event_to_canvas_cb),
                      app);
    g_signal_connect (button, "motion-notify-event",
                      G_CALLBACK (propagate_motion_notify_event_to_canvas_cb),
                      app);

    button = gtk_button_new_with_label ("System 2");

    goc_item_new (top_level_group, GOC_TYPE_WIDGET, "widget", button, "x",
                  300.0, "y", 250.0, "width", 100.0, "height", 50.0, NULL);

    g_signal_connect (button, "button-press-event",
                      G_CALLBACK (propagate_button_press_event_to_canvas_cb),
                      app);
    g_signal_connect (button, "button-release-event",
                      G_CALLBACK (propagate_button_release_event_to_canvas_cb),
                      app);
    g_signal_connect (button, "motion-notify-event",
                      G_CALLBACK (propagate_motion_notify_event_to_canvas_cb),
                      app);

    /* Test of new GocItem */

    item = goc_item_new (top_level_group, MY_TYPE_SYSTEM, NULL);

    g_object_get(item, "widget", &button, NULL);

    g_signal_connect (button, "button-press-event",
                      G_CALLBACK (propagate_button_press_event_to_canvas_cb),
                      app);
    g_signal_connect (button, "button-release-event",
                      G_CALLBACK (propagate_button_release_event_to_canvas_cb),
                      app);
    g_signal_connect (button, "motion-notify-event",
                      G_CALLBACK (propagate_motion_notify_event_to_canvas_cb),
                      app);
}

int
main (int argc, char *argv[])
{
    App *app;

    app = (App *) g_new (App, 1);

    libgoffice_init ();

    gtk_init (&argc, &argv);

    app_init (app);

    GET_UI_ELEMENT (GtkWidget, window1);
    GET_UI_ELEMENT (GtkBox, scrolledwindow1);

    gtk_window_set_role (GTK_WINDOW (window1), "MyGtkTraining");

    app->canvas = g_object_new (GOC_TYPE_CANVAS, "expand", TRUE, NULL);

    gtk_container_add (GTK_CONTAINER (scrolledwindow1),
                       GTK_WIDGET (app->canvas));

    populate_canvas (app);

    gtk_widget_show_all (window1);

    g_signal_connect (app->canvas, "button-press-event",
                      G_CALLBACK (button_press_cb), app);
    g_signal_connect (app->canvas, "button-release-event",
                      G_CALLBACK (button_release_cb), app);
    g_signal_connect (app->canvas, "motion-notify-event",
                      G_CALLBACK (motion_notify_cb), app);

    gtk_main ();

    libgoffice_shutdown ();

    return 0;
}
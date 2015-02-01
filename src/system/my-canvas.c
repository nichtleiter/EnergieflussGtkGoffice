#include "my-canvas.h"

#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

enum
{
    PROP_0,
    /* property entries */
    PROP_TIMELINE,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* 'private'/'protected' functions */
static void my_canvas_class_init (MyCanvasClass * klass);
static void my_canvas_init (MyCanvas * self);
static void my_canvas_finalize (GObject *);
static void my_canvas_dispose (GObject *);

struct _MyCanvasPrivate
{
    /* private members go here */
    GocItem *active_item;
    gdouble offsetx, offsety;
    guint add_arrow_mode;
    guint add_system_mode;
    MyTimelineModel *timeline;
};

/* prototypes of private methods */

void
my_canvas_group_add_item (MyCanvas * self, GocGroup * group, GocItem * item);


G_DEFINE_TYPE_WITH_PRIVATE (MyCanvas, my_canvas, GOC_TYPE_CANVAS);


static void
my_canvas_set_property (GObject * object,
                        guint property_id,
                        const GValue * value, GParamSpec * pspec)
{
    MyCanvas *self = MY_CANVAS (object);

    MyCanvasPrivate *priv = my_canvas_get_instance_private (MY_CANVAS (self));

    switch (property_id) {

        case PROP_TIMELINE:
            priv->timeline = g_value_get_object (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
my_canvas_get_property (GObject * object,
                        guint property_id, GValue * value, GParamSpec * pspec)
{

    MyCanvas *self = MY_CANVAS (object);

    MyCanvasPrivate *priv = my_canvas_get_instance_private (MY_CANVAS (self));

    switch (property_id) {

        case PROP_TIMELINE:
            g_value_set_object (value, priv->timeline);
            break;

        default:
            /* We don't have any other property... */
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

GQuark
my_canvas_error_quark (void)
{
    return g_quark_from_static_string ("my-canvas-error-quark");
}

static gboolean
my_canvas_drag_drag_point (MyCanvas * self, gdouble x, gdouble y)
{
    goc_item_set (self->_priv->active_item, "x", x, "y", y, NULL);
}


static gboolean
my_canvas_button_press_1_cb (GocCanvas * canvas, GdkEventButton * event,
                             gpointer data)
{
    MyCanvas *self = MY_CANVAS (canvas);

    gdouble offsetx, offsety;
    gdouble x_cv, y_cv, dx, dy;

    /* coordinates on canvas */
    x_cv = event->x;
    y_cv = event->y;

    if (event->window != gtk_layout_get_bin_window (&canvas->base)) {
        gint x, y;

        gtk_widget_translate_coordinates (GTK_WIDGET (data),
                                          GTK_WIDGET (canvas),
                                          event->x, event->y, &x, &y);

        /* coordinates on canvas */
        x_cv = x;
        y_cv = y;
    }

    dx = event->x;
    dy = event->y;

    self->_priv->active_item = goc_canvas_get_item_at (canvas, x_cv, y_cv);

    /* if in ADD SYSTEM MODE */
    if (self->_priv->add_system_mode && !GOC_IS_ITEM (self->_priv->active_item)) {

        MySystem *system;

        system = g_object_new (MY_TYPE_SYSTEM, "x", x_cv, "y", y_cv, NULL);

        my_timeline_model_add_object (self->_priv->timeline, system);

        self->_priv->add_system_mode = FALSE;
    }
    /* if in ADD ARROW MODE */
    else if (self->_priv->add_arrow_mode
             && MY_IS_SYSTEM (self->_priv->active_item)) {

        MyFlowArrow *arrow;
        MyDragPoint *point;

        arrow =
            g_object_new (MY_TYPE_FLOW_ARROW, "primary-system",
                          self->_priv->active_item, "x1", x_cv, "y1", y_cv,
                          "x0", x_cv, "y0", y_cv, NULL);

        if (!my_timeline_model_add_object (self->_priv->timeline, arrow)) {

            self->_priv->add_arrow_mode = FALSE;

            g_object_unref (arrow);

        }
        else {

            my_flow_arrow_show_drag_points (arrow);

            point = my_flow_arrow_get_drag_point (arrow);

            goc_item_set (GOC_ITEM (point), "x", x_cv, "y", y_cv, NULL);

            self->_priv->active_item = GOC_ITEM (point);
        }
    }
    else {
        self->_priv->add_arrow_mode = FALSE;
    }

    if (GOC_IS_CIRCLE (self->_priv->active_item)
        || MY_IS_DRAG_POINT (self->_priv->active_item)) {

        gdouble x, y;

        g_object_get (self->_priv->active_item, "x", &x, "y", &y, NULL);

        dx = event->x - x;
        dy = event->y - y;
    }

    self->_priv->offsetx = (canvas->direction == GOC_DIRECTION_RTL) ?
        canvas->scroll_x1 + (canvas->width -
                             dx) /
        canvas->pixels_per_unit : canvas->scroll_x1 +
        dx / canvas->pixels_per_unit;

    self->_priv->offsety = canvas->scroll_y1 + dy / canvas->pixels_per_unit;

    if (MY_IS_DRAG_POINT (self->_priv->active_item)) {
        my_drag_point_begin_dragging (MY_DRAG_POINT (self->_priv->active_item));
    }

    return FALSE;
}

static gboolean
my_canvas_button_press_3_cb (GocCanvas * canvas, GdkEventButton * event,
                             gpointer data)
{
    GdkEventButton *event_button;

    return FALSE;
}

static void
my_canvas_class_init (MyCanvasClass * klass)
{
    GObjectClass *gobject_class;

    gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = my_canvas_finalize;
    gobject_class->dispose = my_canvas_dispose;
    gobject_class->set_property = my_canvas_set_property;
    gobject_class->get_property = my_canvas_get_property;

    obj_properties[PROP_TIMELINE] =
        g_param_spec_object ("timeline",
                             "timeline",
                             "Timeline",
                             MY_TYPE_TIMELINE_MODEL, G_PARAM_READWRITE);

    g_object_class_install_properties (gobject_class,
                                       N_PROPERTIES, obj_properties);
}

static void
my_canvas_init (MyCanvas * self)
{
    GocGroup *root;

    root = goc_canvas_get_root (GOC_CANVAS (self));

    self->_priv = my_canvas_get_instance_private (self);

    self->_priv->active_item = NULL;
    self->_priv->add_arrow_mode = FALSE;
    self->_priv->add_system_mode = FALSE;

    self->group_arrows = goc_group_new (root);
    self->group_systems = goc_group_new (root);

    g_signal_connect (G_OBJECT (self), "button-press-event",
                      G_CALLBACK (my_canvas_button_press_cb), NULL);
    g_signal_connect (self, "button-release-event",
                      G_CALLBACK (my_canvas_button_release_cb), NULL);
    g_signal_connect (self, "motion-notify-event",
                      G_CALLBACK (my_canvas_motion_notify_cb), NULL);
}

static void
my_canvas_dispose (GObject * object)
{
    G_OBJECT_CLASS (my_canvas_parent_class)->dispose (object);
}

static void
my_canvas_finalize (GObject * object)
{
    G_OBJECT_CLASS (my_canvas_parent_class)->finalize (object);
}

/* begin public functions */

void
my_canvas_show_drag_points_of_all_arrows (MyCanvas * self)
{
    GList *l;
    GocGroup *group;

    group = self->group_arrows;

    for (l = group->children; l != NULL; l = l->next) {
        if (MY_IS_FLOW_ARROW (l->data)) {
            my_flow_arrow_show_drag_points (MY_FLOW_ARROW (l->data));
        }
    }
}

gboolean
my_canvas_generate_json_data_stream (MyCanvas * self, gchar ** str, gsize * len)
{
    JsonGenerator *gen = json_generator_new ();
    JsonArray *array_arrows, *array_systems, *array_root;
    JsonNode *node, *root, *arrows, *systems;
    GocGroup *group_arrows, *group_systems;
    GList *l;

    group_arrows = self->group_arrows;
    group_systems = self->group_systems;

    root = json_node_new (JSON_NODE_ARRAY);
    arrows = json_node_new (JSON_NODE_ARRAY);
    systems = json_node_new (JSON_NODE_ARRAY);

    array_root = json_array_new ();
    array_arrows = json_array_new ();
    array_systems = json_array_new ();

    json_node_set_array (root, array_root);
    json_node_set_array (arrows, array_arrows);
    json_node_set_array (systems, array_systems);

    json_array_add_element (array_root, arrows);
    json_array_add_element (array_root, systems);

    for (l = group_arrows->children; l != NULL; l = l->next) {
        if (MY_IS_FLOW_ARROW (l->data)) {
            node = json_gobject_serialize (G_OBJECT (l->data));
            json_array_add_element (array_arrows, node);
        }
    }
    for (l = group_systems->children; l != NULL; l = l->next) {
        if (MY_IS_SYSTEM (l->data)) {
            node = json_gobject_serialize (G_OBJECT (l->data));
            json_array_add_element (array_systems, node);
        }
    }

    json_generator_set_root (gen, root);
    json_generator_set_pretty (gen, TRUE);

    *str = json_generator_to_data (gen, len);

    json_node_free (root);

    json_array_unref (array_systems);
    json_array_unref (array_arrows);
    json_array_unref (array_root);

    return TRUE;
}

gboolean
my_canvas_button_press_cb (GocCanvas * canvas, GdkEventButton * event,
                           gpointer data)
{
    if (event->button == 1) {
        my_canvas_button_press_1_cb (canvas, event, data);
        return TRUE;
    }
    else if (event->button == 3) {
        return my_canvas_button_press_3_cb (canvas, event, data);
    }

    return FALSE;
}

gboolean
my_canvas_button_release_cb (GocCanvas * canvas, GdkEvent * event,
                             gpointer data)
{
    MyCanvas *self = MY_CANVAS (canvas);
    MySystem *primary_system;
    MyFlowArrow *arrow;
    GocItem *item;

    if (self->_priv->add_arrow_mode) {
        self->_priv->add_arrow_mode = FALSE;
    }

    if (MY_IS_DRAG_POINT (self->_priv->active_item)) {

        gdouble d =
            goc_item_distance (GOC_ITEM (self->group_systems), event->button.x,
                               event->button.y, &item);

        g_object_get (self->_priv->active_item, "linked-item", &arrow, NULL);

        if (MY_IS_FLOW_ARROW (arrow)) {

            g_object_get (arrow, "primary-system", &primary_system, NULL);

            /* only do it if drag point is over a system but not over the system the corresponding arrow is linked with */

            if (d == 0. && MY_IS_SYSTEM (item)
                && MY_SYSTEM (primary_system) != MY_SYSTEM (item)) {

                g_object_set (arrow, "secondary-system", item, NULL);

            }
            else {
                g_object_set (arrow, "secondary-system", NULL, NULL);
            }
        }

        my_drag_point_end_dragging (MY_DRAG_POINT (self->_priv->active_item));
    }

    self->_priv->active_item = NULL;

    return TRUE;
}

gboolean
my_canvas_motion_notify_cb (GocCanvas * canvas, GdkEventMotion * event,
                            gpointer data)
{
    MyCanvas *self = MY_CANVAS (canvas);
    GocItem *active_item = self->_priv->active_item;

    cairo_matrix_t matrix;
    gdouble x_cv, y_cv;

    /* coordinates on canvas */
    x_cv = event->x;
    y_cv = event->y;

    if (active_item != NULL) {

        gdouble x_item_old, y_item_old;
        gdouble x_item_new, y_item_new;

        if (GOC_IS_WIDGET (active_item)) {

            gint x, y;

            g_return_val_if_fail (GTK_IS_WIDGET (data), FALSE);

            gtk_widget_translate_coordinates (GTK_WIDGET (data),
                                              GTK_WIDGET (canvas),
                                              event->x, event->y, &x, &y);

            /* coordinates on canvas */
            x_cv = x;
            y_cv = y;
        }

        g_object_get (active_item, "x", &x_item_old, "y", &y_item_old, NULL);

        x_item_new = x_cv - self->_priv->offsetx;
        y_item_new = y_cv - self->_priv->offsety;

        if (GOC_IS_WIDGET (active_item)) {
            goc_item_set (active_item, "x", x_item_new, "y", y_item_new, NULL);
        }
        else if (MY_IS_DRAG_POINT (active_item)) {
            my_canvas_drag_drag_point (self, x_item_new, y_item_new);
        }
        else if (GOC_IS_CIRCLE (active_item)) {
            goc_item_set (active_item, "x", x_item_new, "y", y_item_new, NULL);
        }

        goc_item_invalidate (active_item);
        gtk_widget_queue_draw (GTK_WIDGET (canvas));
    }
    return TRUE;
}

void
my_canvas_foreach_populate_group_from_array (GocItem * item, GocGroup * group)
{
    g_return_if_fail (GOC_IS_GROUP (group));
    g_return_if_fail (GOC_IS_ITEM (item));

    my_canvas_group_add_item (NULL, group, item);
}

/* transition */
void
my_canvas_model_current_index_changed (MyCanvas * self, MyTimelineModel * model)
{
    GPtrArray *array;
    GocGroup *root;
    GList *l;

    g_return_if_fail (MY_IS_CANVAS (self));
    g_return_if_fail (MY_IS_TIMELINE_MODEL (model));

    root = goc_canvas_get_root (GOC_CANVAS (self));

    if (GOC_IS_GROUP (self->group_arrows)) {

        l = self->group_arrows->children;

        while (l != NULL) {

            /*if (MY_IS_FLOW_ARROW (l->data)) { */
            /*g_print ("Arrow -> RefCount: %u\n", */
            /*G_OBJECT (l->data)->ref_count); */
            /*} */
            /*else { */
            /*g_print ("Other -> RefCount: %u\n", */
            /*G_OBJECT (l->data)->ref_count); */
            /*} */

            goc_group_remove_child (self->group_arrows, GOC_ITEM (l->data));

            /* since list changes upon goc_group_remove_child l must be refreshed */
            l = self->group_arrows->children;
        }

        goc_item_destroy (GOC_ITEM (self->group_arrows));
        self->group_arrows = goc_group_new (root);
        goc_item_lower_to_bottom (GOC_ITEM (self->group_arrows));
    }

    array = my_timeline_model_get_arrows_of_current_pos (self->_priv->timeline);

    if (array != NULL) {

        g_print ("array->len: %u\n", array->len);

        g_ptr_array_foreach (array, (GFunc)
                             my_canvas_foreach_populate_group_from_array,
                             self->group_arrows);
    }
}

void
my_canvas_group_add_item (MyCanvas * self, GocGroup * group, GocItem * item)
{
    g_return_if_fail (GOC_IS_GROUP (group));
    g_return_if_fail (GOC_IS_ITEM (item));

    goc_group_add_child (group, item);
    goc_item_invalidate (item);
}

void
my_canvas_set_add_system_mode (MyCanvas * self)
{
    g_return_if_fail (MY_IS_CANVAS (self));

    self->_priv->add_system_mode = TRUE;
}

void
my_canvas_set_add_arrow_mode (MyCanvas * self)
{
    g_return_if_fail (MY_IS_CANVAS (self));

    self->_priv->add_arrow_mode = TRUE;
}

void
my_canvas_model_arrow_added_at_current_index (MyCanvas * self,
                                              MyFlowArrow * arrow,
                                              MyTimelineModel * model)
{
    g_return_if_fail (MY_IS_CANVAS (self));
    g_return_if_fail (MY_IS_TIMELINE_MODEL (model));
    g_return_if_fail (MY_IS_FLOW_ARROW (arrow));

    my_canvas_group_add_item (self, self->group_arrows, GOC_ITEM (arrow));
}

void
my_canvas_model_system_added (MyCanvas * self, MySystem * system,
                              MyTimelineModel * model)
{
    g_return_if_fail (MY_IS_CANVAS (self));
    g_return_if_fail (MY_IS_TIMELINE_MODEL (model));
    g_return_if_fail (MY_IS_SYSTEM (system));

    my_canvas_group_add_item (self, self->group_systems, GOC_ITEM (system));
}

void
my_canvas_model_systems_changed (MyCanvas * self, MyTimelineModel * model)
{
    g_return_if_fail (MY_IS_CANVAS (self));
    g_return_if_fail (MY_IS_TIMELINE_MODEL (model));
}

void
my_canvas_set_timeline (MyCanvas * self, MyTimelineModel * model)
{

    g_return_if_fail (MY_IS_CANVAS (self));
    g_return_if_fail (MY_IS_TIMELINE_MODEL (model));

    g_object_set (self, "timeline", model, NULL);

    g_signal_connect_swapped (model, "current-pos-changed",
                              G_CALLBACK
                              (my_canvas_model_current_index_changed), self);

    g_signal_connect_swapped (model, "systems-changed",
                              G_CALLBACK
                              (my_canvas_model_systems_changed), self);

    g_signal_connect_swapped (model, "system-added",
                              G_CALLBACK (my_canvas_model_system_added), self);

    g_signal_connect_swapped (model, "arrow-added-at-current-pos",
                              G_CALLBACK
                              (my_canvas_model_arrow_added_at_current_index),
                              self);
}

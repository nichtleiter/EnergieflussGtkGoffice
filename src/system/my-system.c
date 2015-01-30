#include "my-system.h"

#include <json-glib/json-glib.h>

/******************************************************************************
 * GocOffscreenBox: code mostly copied from gtk+/tests/gtkoffscreenbox.[c,h]
 ******************************************************************************/

#define GOC_TYPE_OFFSCREEN_BOX              (goc_offscreen_box_get_type ())
#define GOC_OFFSCREEN_BOX(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOC_TYPE_OFFSCREEN_BOX, GocOffscreenBox))
#define GOC_OFFSCREEN_BOX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOC_TYPE_OFFSCREEN_BOX, GocOffscreenBoxClass))
#define GOC_IS_OFFSCREEN_BOX(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOC_TYPE_OFFSCREEN_BOX))
#define GOC_IS_OFFSCREEN_BOX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOC_TYPE_OFFSCREEN_BOX))
#define GOC_OFFSCREEN_BOX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GOC_TYPE_OFFSCREEN_BOX, GocOffscreenBoxClass))

typedef struct _GocOffscreenBox GocOffscreenBox;
typedef GtkBinClass GocOffscreenBoxClass;

static GObjectClass *goc_offscreen_box_parent_class;

struct _GocOffscreenBox
{
    GtkBin base;
    GtkWidget *child;
    GdkWindow *offscreen_window;
    gdouble angle, scale;
};

struct _GocOffscreenBoxClass
{
    GtkBinClass parent_class;
};

/******************************************************************************
 * GocOffscreenBox: End
 ******************************************************************************/

enum
{
    PROP_0,
    PROP_ID,
    PROP_LABEL,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* 'private'/'protected' functions */
static void my_system_class_init (MySystemClass * klass);
static void my_system_init (MySystem * self);
static void my_system_finalize (GObject *);
static void my_system_dispose (GObject *);

typedef struct
{
    /* private members go here */
    guint id;
    gchar *label;
} MySystemPrivate;


static void my_system_json_serializable (JsonSerializableIface * iface);

G_DEFINE_TYPE_WITH_CODE (MySystem, my_system, GOC_TYPE_WIDGET,
                         G_ADD_PRIVATE (MySystem)
                         G_IMPLEMENT_INTERFACE (JSON_TYPE_SERIALIZABLE,
                                                my_system_json_serializable))

     static JsonNode *_serialize_property (JsonSerializable * serializable,
                                           const gchar * name,
                                           const GValue * value,
                                           GParamSpec * pspec)
{
    JsonNode *json_node = NULL;

    g_return_val_if_fail (MY_IS_SYSTEM (serializable), FALSE);

    if (g_str_equal (name, "widget")
        || g_str_equal (name, "canvas")
        || g_str_equal (name, "parent")
        ) {
        return json_node;
    }
    else {
        /* Default serialization here (pixbufs excluded) */
        JsonSerializableIface *iface = NULL;

        iface = g_type_default_interface_peek (JSON_TYPE_SERIALIZABLE);
        json_node = iface->serialize_property (serializable,
                                               name, value, pspec);
    }

    return json_node;           /* NULL indicates default deserializer */
}

static gboolean
_deserialize_property (JsonSerializable * serializable,
                       const gchar * name,
                       GValue * value, GParamSpec * pspec, JsonNode * node)
{
    gboolean result = FALSE;

    g_return_val_if_fail (node != NULL, FALSE);

    JsonSerializableIface *iface = NULL;

    iface = g_type_default_interface_peek (JSON_TYPE_SERIALIZABLE);
    result = iface->deserialize_property (serializable,
                                          name, value, pspec, node);

    return result;
}

static void
my_system_json_serializable (JsonSerializableIface * iface)
{
    iface->serialize_property = _serialize_property;
    iface->deserialize_property = _deserialize_property;
}

static void
my_system_set_property (GObject * object,
                        guint property_id,
                        const GValue * value, GParamSpec * pspec)
{
    MySystem *self = MY_SYSTEM (object);
    MySystemPrivate *priv = my_system_get_instance_private (self);

    switch (property_id) {

        case PROP_ID:
            priv->id = g_value_get_uint (value);
            break;

        case PROP_LABEL:
            priv->label = g_value_dup_string (value);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
my_system_get_property (GObject * object,
                        guint property_id, GValue * value, GParamSpec * pspec)
{
    MySystem *self = MY_SYSTEM (object);

    MySystemPrivate *priv = my_system_get_instance_private (self);


    switch (property_id) {

        case PROP_ID:
            g_value_set_uint (value, priv->id);
            break;

        case PROP_LABEL:
            g_value_set_string (value, priv->label);
            break;

        default:
            /* We don't have any other property... */
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

GQuark
my_system_error_quark (void)
{
    return g_quark_from_static_string ("my-system-error-quark");
}

MyAnchorType
calculate_anchor (GtkAllocation from, GtkAllocation to)
{
    gdouble dx, dy, alpha;
    MyAnchorType anchor;

    dx = to.x - from.x;
    dy = to.y - from.y;

    alpha = atan2 (dy, dx);

    anchor = MY_ANCHOR_SOUTH;

    if (-M_PI / 4 < alpha && alpha <= M_PI / 4) {
        anchor = MY_ANCHOR_WEST;
    }
    else if (M_PI / 4 < alpha && alpha <= 3 * M_PI / 4) {
        anchor = MY_ANCHOR_NORTH;
    }
    else if (3 * M_PI / 4 < alpha || alpha <= -3 * M_PI / 4) {
        anchor = MY_ANCHOR_EAST;
    }
    return anchor;
}

void
alloc_get_coordinate_of_anchor (GtkAllocation alloc, MyAnchorType anchor,
                                gdouble * x, gdouble * y)
{

    if (anchor == MY_ANCHOR_WEST) {
        *x = alloc.x;
        *y = alloc.y + alloc.height / 2;
    }
    else if (anchor == MY_ANCHOR_SOUTH) {
        *x = alloc.x + alloc.width / 2;
        *y = alloc.y + alloc.height;
    }
    else if (anchor == MY_ANCHOR_NORTH) {
        *x = alloc.x + alloc.width / 2;
        *y = alloc.y;
    }
    else if (anchor == MY_ANCHOR_EAST) {
        *x = alloc.x + alloc.width;
        *y = alloc.y + alloc.height / 2;
    }
}

void
my_system_get_coordinate_of_anchor (MySystem * system, MyAnchorType anchor,
                                    gdouble * x, gdouble * y)
{
    GtkAllocation alloc;

    g_return_if_fail (MY_IS_SYSTEM (system));

    gtk_widget_get_allocation (GOC_WIDGET (system)->ofbox, &alloc);

    alloc_get_coordinate_of_anchor (alloc, anchor, x, y);
}

void
my_system_connection_set_coordinate_of_arrow_tip (GtkAllocation from,
                                                  GtkAllocation to, gdouble * x,
                                                  gdouble * y)
{
    MyAnchorType anchor_to;

    anchor_to = calculate_anchor (from, to);

    alloc_get_coordinate_of_anchor (to, anchor_to, x, y);
}

void
my_system_draw_energy_flow (GocItem const *item, cairo_t * cr)
{
    MySystem *self;
    GocGroup *group_arrows = NULL;
    MyCanvas *canvas;
    GtkAllocation alloc_primary;
    GList *l;

    self = MY_SYSTEM (item);

    MySystemClass *class = MY_SYSTEM_GET_CLASS (self);
    GocItemClass *parent_class = g_type_class_peek_parent (class);

    /* chaining up */
    parent_class->draw (item, cr);

    g_object_get (self, "canvas", &canvas, NULL);

    group_arrows = canvas->group_arrows;

    if (!GOC_IS_GROUP (group_arrows)) {
        g_print ("can't get arrow group...\n");
        return;
    }

    gtk_widget_get_allocation (GOC_WIDGET (self)->ofbox, &alloc_primary);

    for (l = group_arrows->children; l != NULL; l = l->next) {

        MySystem *secondary_system, *primary_system;
        MyAnchorType anchor_source;

        gdouble x0, x1, y0, y1, arrow_len;
        gdouble energy_quantity;

        if (!MY_IS_FLOW_ARROW (l->data)) {
            continue;
        }

        g_object_get (l->data, "linked-system", &primary_system, NULL);

        /* fetch next arrow if current arrow doesn't belong to system */
        if (primary_system != self) {
            continue;
        }

        g_object_get (l->data,
                      "energy-quantity", &energy_quantity,
                      "anchor", &anchor_source,
                      "secondary-system", &secondary_system, NULL);

        /* draw arrow */

        arrow_len = alloc_primary.width * 0.3;

        /* if arrow depicts transfer to other system */
        if (MY_IS_SYSTEM (secondary_system)) {

            GtkAllocation alloc_secondary;

            gtk_widget_get_allocation (GOC_WIDGET (secondary_system)->ofbox,
                                       &alloc_secondary);

            if (energy_quantity < 0.0) {
                x0 = alloc_primary.x + alloc_primary.width / 2;
                y0 = alloc_primary.y + alloc_primary.height / 2;

                my_system_connection_set_coordinate_of_arrow_tip (alloc_primary,
                                                                  alloc_secondary,
                                                                  &x1, &y1);

            }
            else {
                x0 = alloc_secondary.x + alloc_secondary.width / 2;
                y0 = alloc_secondary.y + alloc_secondary.height / 2;

                my_system_connection_set_coordinate_of_arrow_tip
                    (alloc_secondary, alloc_primary, &x1, &y1);
            }
        }
        /* if arrow depicts transfer to environment */
        else if (energy_quantity < 0.0) {

            alloc_get_coordinate_of_anchor (alloc_primary, anchor_source, &x0,
                                            &y0);

            if (anchor_source == MY_ANCHOR_WEST) {
                x1 = x0 - arrow_len;
                y1 = y0;
            }
            else if (anchor_source == MY_ANCHOR_EAST) {
                x1 = x0 + arrow_len;
                y1 = y0;
            }
            else if (anchor_source == MY_ANCHOR_SOUTH) {
                x1 = x0;
                y1 = y0 + arrow_len;
            }
            else if (anchor_source == MY_ANCHOR_NORTH) {
                x1 = x0;
                y1 = y0 - arrow_len;
            }
        }
        /* if arrow depicts transfer from environment */
        else if (energy_quantity >= 0.0) {

            alloc_get_coordinate_of_anchor (alloc_primary, anchor_source, &x1,
                                            &y1);

            if (anchor_source == MY_ANCHOR_WEST) {
                x0 = x1 - arrow_len;
                y0 = y1;
            }
            else if (anchor_source == MY_ANCHOR_EAST) {
                x0 = x1 + arrow_len;
                y0 = y1;
            }
            else if (anchor_source == MY_ANCHOR_SOUTH) {
                y0 = y1 + arrow_len;
                x0 = x1;
            }
            else if (anchor_source == MY_ANCHOR_NORTH) {
                y0 = y1 - arrow_len;
                x0 = x1;
            }
        }

        if (!my_flow_arrow_is_dragged (MY_FLOW_ARROW (l->data))) {
            goc_item_set (GOC_ITEM (l->data), "x0", x0, "y0", y0, "x1", x1,
                          "y1", y1, NULL);
        }
    }
}

static void
my_system_class_init (MySystemClass * klass)
{
    GObjectClass *gobject_class;
    GocItemClass *gi_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gi_class = (GocItemClass *) klass;

    gobject_class->set_property = my_system_set_property;
    gobject_class->get_property = my_system_get_property;

    gobject_class->finalize = my_system_finalize;
    gobject_class->dispose = my_system_dispose;

    obj_properties[PROP_ID] =
        g_param_spec_uint ("id",
                           "id",
                           "unique identifier of system",
                           0, G_MAXUINT, 0,
                           G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    obj_properties[PROP_LABEL] =
        g_param_spec_string ("label",
                             "label",
                             "label text",
                             NULL, G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

    g_object_class_install_properties (gobject_class,
                                       N_PROPERTIES, obj_properties);

    gi_class->draw = my_system_draw_energy_flow;
}

static gboolean
my_system_begin_drag (GtkWidget * button, GdkEventButton * event,
                      MySystem * self)
{
    GocCanvas *canvas;

    g_object_get (self, "canvas", &canvas, NULL);

    my_canvas_button_press_cb (canvas, event, button);

    return FALSE;
}

static gboolean
my_system_is_dragged (GtkWidget * button,
                      GdkEventMotion * event, MySystem * self)
{
    GocCanvas *canvas;

    g_object_get (self, "canvas", &canvas, NULL);

    my_canvas_motion_notify_cb (canvas, event, button);

    return FALSE;
}

static gboolean
my_system_end_drag (GtkWidget * button, GdkEvent * event, MySystem * self)
{
    GocCanvas *canvas;

    g_object_get (self, "canvas", &canvas, NULL);

    my_canvas_button_release_cb (canvas, event, button);

    return FALSE;
}


static void
my_system_init (MySystem * self)
{
    GtkWidget *button;

    MySystemPrivate *priv = my_system_get_instance_private (self);

    button = (GtkWidget *) my_system_widget_new ();

    g_object_bind_property (self, "id", button, "id",
                            G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE);

    priv->label = g_strdup_printf("System %u", priv->id);

    goc_item_set (GOC_ITEM (self), "widget", button, "width", 300.0, "height",
                  250.0, NULL);

    g_signal_connect (button, "button-press-event",
                      G_CALLBACK (my_system_begin_drag), self);

    g_signal_connect (button, "button-release-event",
                      G_CALLBACK (my_system_end_drag), self);

    g_signal_connect (button, "motion-notify-event",
                      G_CALLBACK (my_system_is_dragged), self);
}

static void
my_system_dispose (GObject * object)
{
    G_OBJECT_CLASS (my_system_parent_class)->dispose (object);
}

static void
my_system_finalize (GObject * object)
{
    /* free/unref instance resources here */
    G_OBJECT_CLASS (my_system_parent_class)->finalize (object);
}

/* begin public methods */

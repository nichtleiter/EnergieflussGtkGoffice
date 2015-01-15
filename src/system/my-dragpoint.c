#include "my-dragpoint.h"

enum
{
    PROP_0,
    /* property entries */
    PROP_LINKED_ITEM,
    N_PROPERTIES
};


static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

/* 'private'/'protected' functions */
static void my_drag_point_class_init (MyDragPointClass * klass);
static void my_drag_point_init (MyDragPoint * self);
static void my_drag_point_finalize (GObject * );
static void my_drag_point_dispose (GObject * );

struct _MyDragPointPrivate
{
    GocItem *linked_item;
    gboolean is_dragged;
};

#define MY_DRAG_POINT_GET_PRIVATE(o)      (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                       MY_TYPE_DRAG_POINT, \
                                       MyDragPointPrivate))

G_DEFINE_TYPE (MyDragPoint, my_drag_point, GOC_TYPE_CIRCLE);


GQuark
my_drag_point_error_quark (void)
{
  return g_quark_from_static_string ("my-drag-point-error-quark");
}

static void
my_drag_point_set_property (GObject * object,
                            guint property_id,
                            const GValue * value, GParamSpec * pspec)
{
    MyDragPoint *self = MY_DRAG_POINT (object);

    switch (property_id) {

        case PROP_LINKED_ITEM: {
                self->_priv->linked_item =
                    GOC_ITEM (g_value_get_object (value));

                return;
            }
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

static void
my_drag_point_get_property (GObject * object,
                            guint property_id,
                            GValue * value, GParamSpec * pspec)
{
    MyDragPoint *self = MY_DRAG_POINT (object);

    switch (property_id) {

        case PROP_LINKED_ITEM:
            g_value_set_object (value, self->_priv->linked_item);
            break;

        default:
            /* We don't have any other property... */
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
            break;
    }
}

gboolean
my_drag_point_button_pressed (GocItem * item, int button, double x, double y)
{
    GOStyle *style;
    cairo_surface_t *surf;
    cairo_t *cr;
    GOStyledObject *gso = GO_STYLED_OBJECT(item);
    MyDragPoint *self = MY_DRAG_POINT (item);
    MyDragPointClass *class = MY_DRAG_POINT_GET_CLASS (self);
    GocItemClass *parent_class = g_type_class_peek_parent (class);

    parent_class->button_pressed (item, button, x, y);

    g_print ("button pressed...\n");

    style = go_style_dup (go_styled_object_get_style (gso));
    style->line.width = 2;
    style->fill.type = GO_STYLE_FILL_PATTERN;
    style->fill.pattern.pattern = GO_PATTERN_SOLID;
    style->fill.pattern.back = GO_COLOR_RED;
    style->fill.pattern.fore = GO_COLOR_RED;
    go_styled_object_set_style (gso, style);

    g_object_unref (style);

    return FALSE;
}

static void
my_drag_point_class_init (MyDragPointClass * klass)
{
    GObjectClass *gobject_class;
    GocItemClass *gi_class;

    gobject_class = G_OBJECT_CLASS (klass);
    gi_class = (GocItemClass *) klass;

    gobject_class = G_OBJECT_CLASS (klass);

    gobject_class->finalize = my_drag_point_finalize;
    gobject_class->dispose = my_drag_point_dispose;
    gobject_class->set_property = my_drag_point_set_property;
    gobject_class->get_property = my_drag_point_get_property;

    obj_properties[PROP_LINKED_ITEM] =
        g_param_spec_object ("linked-item",
                             "linked item",
                             "A pointer to the linked item",
                             MY_TYPE_FLOW_ARROW, G_PARAM_READWRITE);

    g_object_class_install_properties (gobject_class,
                                       N_PROPERTIES, obj_properties);

    g_type_class_add_private (gobject_class,
                              sizeof (MyDragPointPrivate));

    gi_class->button_pressed = my_drag_point_button_pressed;
}

static void
my_drag_point_init (MyDragPoint * self)
{
    self->_priv = MY_DRAG_POINT_GET_PRIVATE (self);

    /* to init any of the private data, do e.g: */

    self->_priv->linked_item = NULL;
    self->_priv->is_dragged = FALSE;
}

static void
my_drag_point_dispose (GObject *object)
{
    G_OBJECT_CLASS (my_drag_point_parent_class)->dispose (object);
}

static void
my_drag_point_finalize (GObject * object)
{
    /* free/unref instance resources here */
    G_OBJECT_CLASS (my_drag_point_parent_class)->finalize (object);
}

MyDragPoint *
my_drag_point_new (void)
{
    MyDragPoint *self;

    self = g_object_new (MY_TYPE_DRAG_POINT, NULL);

    return self;
}


void
my_drag_point_begin_dragging (MyDragPoint * self)
{
    g_return_if_fail(MY_IS_DRAG_POINT(self));

    self->_priv->is_dragged = TRUE;

    if(MY_IS_FLOW_ARROW(self->_priv->linked_item)) {
        my_flow_arrow_begin_dragging(MY_FLOW_ARROW(self->_priv->linked_item));
    }
}

gboolean
my_drag_point_is_dragged (MyDragPoint * self)
{
    g_return_if_fail(MY_IS_DRAG_POINT(self));

    return self->_priv->is_dragged;
}

void
my_drag_point_end_dragging (MyDragPoint * self)
{
    g_return_if_fail(MY_IS_DRAG_POINT(self));
    
    if(MY_IS_FLOW_ARROW(self->_priv->linked_item)) {
        my_flow_arrow_end_dragging(MY_FLOW_ARROW(self->_priv->linked_item));
    }

    self->_priv->is_dragged = FALSE;
}
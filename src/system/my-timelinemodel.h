#ifndef __MY_TIMELINE_MODEL_H__
#define __MY_TIMELINE_MODEL_H__

#include <glib-object.h>


G_BEGIN_DECLS

#define MY_TYPE_TIMELINE_MODEL             (my_timeline_model_get_type())
#define MY_TIMELINE_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),MY_TYPE_TIMELINE_MODEL,MyTimelineModel))
#define MY_TIMELINE_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),MY_TYPE_TIMELINE_MODEL,MyTimelineModelClass))
#define MY_IS_TIMELINE_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj),MY_TYPE_TIMELINE_MODEL))
#define MY_IS_TIMELINE_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),MY_TYPE_TIMELINE_MODEL))
#define MY_TIMELINE_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),MY_TYPE_TIMELINE_MODEL,MyTimelineModelClass))

#define MY_TIMELINE_MODEL_ERROR                (my_timeline_model_error_quark ())

typedef struct _MyTimelineModel MyTimelineModel;
typedef struct _MyTimelineModelClass MyTimelineModelClass;
typedef struct _MyTimelineModelPrivate MyTimelineModelPrivate;

struct _MyTimelineModel
{
    GObject parent;
    /* insert public members here */

    /* private */
    MyTimelineModelPrivate *_priv;
};

struct _MyTimelineModelClass
{
    GObjectClass parent_class;

};

#include "my-system.h"
#include "my-systemmodel.h"
#include "my-systemwidget.h"

GType my_timeline_model_get_type (void);

/* fill in public functions */
MyTimelineModel *my_timeline_model_new (void);

gboolean
my_timeline_model_add_object (MyTimelineModel * self, gpointer object);

GPtrArray *
my_timeline_model_get_arrows_of_current_pos (MyTimelineModel * self);

void
my_timeline_model_append_to_timeline (MyTimelineModel * self);

gboolean
my_timeline_model_remove_object (MyTimelineModel * self, gpointer object);

void
my_timeline_model_add_at_current_pos (MyTimelineModel * self);

GPtrArray *
my_timeline_model_get_systems_data_of_current_pos  (MyTimelineModel * self);

guint
my_timeline_model_get_current_pos (MyTimelineModel * self);

gboolean
my_timeline_model_current_pos_is_state (MyTimelineModel * self);

MySystem *
my_timeline_get_system_with_id (MyTimelineModel * self, guint id);

GPtrArray *
my_timeline_get_systems (MyTimelineModel * self);

G_END_DECLS

#endif /* __MY_TIMELINE_MODEL_H__ */

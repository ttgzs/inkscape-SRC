#define __SP_OBJECTGROUP_C__

/*
 * Abstract base class for non-item groups
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * Copyright (C) 1999-2003 Authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "sp-object-group.h"
#include "sp-object-repr.h"
#include "xml/repr.h"

static void sp_objectgroup_class_init (SPObjectGroupClass *klass);
static void sp_objectgroup_init (SPObjectGroup *objectgroup);

static void sp_objectgroup_child_added (SPObject * object, SPRepr * child, SPRepr * ref);
static void sp_objectgroup_remove_child (SPObject * object, SPRepr * child);
static void sp_objectgroup_order_changed (SPObject * object, SPRepr * child, SPRepr * old_ref, SPRepr * new_ref);
static SPRepr *sp_objectgroup_write (SPObject *object, SPRepr *repr, guint flags);

static SPObjectClass *parent_class;

GType
sp_objectgroup_get_type (void)
{
	static GType objectgroup_type = 0;
	if (!objectgroup_type) {
		GTypeInfo objectgroup_info = {
			sizeof (SPObjectGroupClass),
			NULL,	/* base_init */
			NULL,	/* base_finalize */
			(GClassInitFunc) sp_objectgroup_class_init,
			NULL,	/* class_finalize */
			NULL,	/* class_data */
			sizeof (SPObjectGroup),
			16,	/* n_preallocs */
			(GInstanceInitFunc) sp_objectgroup_init,
			NULL,	/* value_table */
		};
		objectgroup_type = g_type_register_static (SP_TYPE_OBJECT, "SPObjectGroup", &objectgroup_info, (GTypeFlags)0);
	}
	return objectgroup_type;
}

static void
sp_objectgroup_class_init (SPObjectGroupClass *klass)
{
	GObjectClass * object_class;
	SPObjectClass * sp_object_class;

	object_class = (GObjectClass *) klass;
	sp_object_class = (SPObjectClass *) klass;

	parent_class = (SPObjectClass *)g_type_class_ref (SP_TYPE_OBJECT);

	sp_object_class->child_added = sp_objectgroup_child_added;
	sp_object_class->remove_child = sp_objectgroup_remove_child;
	sp_object_class->order_changed = sp_objectgroup_order_changed;
	sp_object_class->write = sp_objectgroup_write;
}

static void
sp_objectgroup_init (SPObjectGroup *objectgroup)
{
}

static void
sp_objectgroup_child_added (SPObject *object, SPRepr *child, SPRepr *ref)
{
	if (((SPObjectClass *) (parent_class))->child_added)
		(* ((SPObjectClass *) (parent_class))->child_added) (object, child, ref);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

static void
sp_objectgroup_remove_child (SPObject *object, SPRepr *child)
{
	if (((SPObjectClass *) (parent_class))->remove_child)
		(* ((SPObjectClass *) (parent_class))->remove_child) (object, child);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

static void
sp_objectgroup_order_changed (SPObject *object, SPRepr *child, SPRepr *old_ref, SPRepr *new_ref)
{
	if (((SPObjectClass *) (parent_class))->order_changed)
		(* ((SPObjectClass *) (parent_class))->order_changed) (object, child, old_ref, new_ref);

	object->requestModified(SP_OBJECT_MODIFIED_FLAG);
}

static SPRepr *
sp_objectgroup_write (SPObject *object, SPRepr *repr, guint flags)
{
	SPObjectGroup *group;
	SPObject *child;
	SPRepr *crepr;

	group = SP_OBJECTGROUP (object);

	if (flags & SP_OBJECT_WRITE_BUILD) {
		GSList *l;
		if (!repr) repr = sp_repr_new ("svg:g");
		l = NULL;
		for ( child = sp_object_first_child(object) ; child != NULL ; child = SP_OBJECT_NEXT(child) ) {
			crepr = child->updateRepr(NULL, flags);
			if (crepr) l = g_slist_prepend (l, crepr);
		}
		while (l) {
			sp_repr_add_child (repr, (SPRepr *) l->data, NULL);
			sp_repr_unref ((SPRepr *) l->data);
			l = g_slist_remove (l, l->data);
		}
	} else {
		for ( child = sp_object_first_child(object) ; child != NULL ; child = SP_OBJECT_NEXT(child) ) {
			child->updateRepr(flags);
		}
	}

	if (((SPObjectClass *) (parent_class))->write)
		((SPObjectClass *) (parent_class))->write (object, repr, flags);

	return repr;
}


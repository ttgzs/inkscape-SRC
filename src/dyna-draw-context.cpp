#define __SP_DYNA_DRAW_CONTEXT_C__

/*
 * Handwriting-like drawing mode
 *
 * Authors:
 *   Mitsuru Oka <oka326@parkcity.ne.jp>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *
 * The original dynadraw code:
 *   Paul Haeberli <paul@sgi.com>
 *
 * Copyright (C) 1998 The Free Software Foundation
 * Copyright (C) 1999-2002 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

/*
 * TODO: Tue Oct  2 22:57:15 2001
 *  - Decide control point behavior when use_calligraphic==1.
 *  - Decide to use NORMALIZED_COORDINATE or not.
 *  - Bug fix.
 */

#define noDYNA_DRAW_VERBOSE

#include "config.h"

#include <math.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "svg/svg.h"
#include <glibmm/i18n.h>
#include "display/curve.h"
#include "display/canvas-bpath.h"
#include "display/sodipodi-ctrl.h"
#include "display/bezier-utils.h"

#include "macros.h"
#include "enums.h"
#include "mod360.h"
#include "inkscape.h"
#include "document.h"
#include "selection.h"
#include "desktop-events.h"
#include "desktop-handles.h"
#include "desktop-affine.h"
#include "desktop-style.h"
#include "snap.h"
#include "sp-desktop-widget.h"
#include "dyna-draw-context.h"
#include "widgets/spw-utilities.h"
#include "libnr/n-art-bpath.h"
#include "libnr/nr-point-fns.h"
#include "xml/repr.h"

#define DDC_RED_RGBA 0xff0000ff
#define DDC_GREEN_RGBA 0x000000ff

#define SAMPLE_TIMEOUT 10
#define TOLERANCE_LINE 1.0
#define TOLERANCE_CALLIGRAPHIC 3.0
#define DYNA_EPSILON 1.0e-6

#define DYNA_MIN_WIDTH 1.0e-6

#define DRAG_MIN 0.0
#define DRAG_DEFAULT 1.0
#define DRAG_MAX 1.0

static void sp_dyna_draw_context_class_init(SPDynaDrawContextClass *klass);
static void sp_dyna_draw_context_init(SPDynaDrawContext *ddc);
static void sp_dyna_draw_context_dispose(GObject *object);

static void sp_dyna_draw_context_setup(SPEventContext *ec);
static void sp_dyna_draw_context_set(SPEventContext *ec, gchar const *key, gchar const *val);
static gint sp_dyna_draw_context_root_handler(SPEventContext *ec, GdkEvent *event);

static void clear_current(SPDynaDrawContext *dc);
static void set_to_accumulated(SPDynaDrawContext *dc);
static void concat_current_line(SPDynaDrawContext *dc);
static void accumulate_calligraphic(SPDynaDrawContext *dc);

static void fit_and_split(SPDynaDrawContext *ddc, gboolean release);
static void fit_and_split_line(SPDynaDrawContext *ddc, gboolean release);
static void fit_and_split_calligraphics(SPDynaDrawContext *ddc, gboolean release);

static void sp_dyna_draw_reset(SPDynaDrawContext *ddc, NR::Point p);
static NR::Point sp_dyna_draw_get_npoint(SPDynaDrawContext const *ddc, NR::Point v);
static NR::Point sp_dyna_draw_get_vpoint(SPDynaDrawContext const *ddc, NR::Point n);
static NR::Point sp_dyna_draw_get_curr_vpoint(SPDynaDrawContext const *ddc);
static void draw_temporary_box(SPDynaDrawContext *dc);


static SPEventContextClass *parent_class;

GtkType
sp_dyna_draw_context_get_type(void)
{
    static GType type = 0;
    if (!type) {
        GTypeInfo info = {
            sizeof(SPDynaDrawContextClass),
            NULL, NULL,
            (GClassInitFunc) sp_dyna_draw_context_class_init,
            NULL, NULL,
            sizeof(SPDynaDrawContext),
            4,
            (GInstanceInitFunc) sp_dyna_draw_context_init,
            NULL,   /* value_table */
        };
        type = g_type_register_static(SP_TYPE_EVENT_CONTEXT, "SPDynaDrawContext", &info, (GTypeFlags)0);
    }
    return type;
}

static void
sp_dyna_draw_context_class_init(SPDynaDrawContextClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    SPEventContextClass *event_context_class = (SPEventContextClass *) klass;

    parent_class = (SPEventContextClass*)g_type_class_peek_parent(klass);

    object_class->dispose = sp_dyna_draw_context_dispose;

    event_context_class->setup = sp_dyna_draw_context_setup;
    event_context_class->set = sp_dyna_draw_context_set;
    event_context_class->root_handler = sp_dyna_draw_context_root_handler;
}

static void
sp_dyna_draw_context_init(SPDynaDrawContext *ddc)
{
    ddc->accumulated = NULL;
    ddc->segments = NULL;
    ddc->currentcurve = NULL;
    ddc->currentshape = NULL;
    ddc->npoints = 0;
    ddc->cal1 = NULL;
    ddc->cal2 = NULL;
    ddc->repr = NULL;

    /* DynaDraw values */
    ddc->cur = NR::Point(0,0);
    ddc->last = NR::Point(0,0);
    ddc->vel = NR::Point(0,0);
    ddc->acc = NR::Point(0,0);
    ddc->ang = NR::Point(0,0);
    ddc->del = NR::Point(0,0);

    /* attributes */
    ddc->use_timeout = FALSE;
    ddc->use_calligraphic = TRUE;
    ddc->timer_id = 0;
    ddc->dragging = FALSE;
    ddc->dynahand = FALSE;

    ddc->mass = 0.3;
    ddc->drag = DRAG_DEFAULT;
    ddc->angle = 30.0;
    ddc->width = 0.2;

    ddc->vel_thin = 0.1;
    ddc->flatness = 0.9;
}

static void
sp_dyna_draw_context_dispose(GObject *object)
{
    SPDynaDrawContext *ddc = SP_DYNA_DRAW_CONTEXT(object);

    if (ddc->accumulated) {
        ddc->accumulated = sp_curve_unref(ddc->accumulated);
    }

    while (ddc->segments) {
        gtk_object_destroy(GTK_OBJECT(ddc->segments->data));
        ddc->segments = g_slist_remove(ddc->segments, ddc->segments->data);
    }

    if (ddc->currentcurve) ddc->currentcurve = sp_curve_unref(ddc->currentcurve);
    if (ddc->cal1) ddc->cal1 = sp_curve_unref(ddc->cal1);
    if (ddc->cal2) ddc->cal2 = sp_curve_unref(ddc->cal2);

    if (ddc->currentshape) {
        gtk_object_destroy(GTK_OBJECT(ddc->currentshape));
        ddc->currentshape = NULL;
    }

    if (ddc->_message_context) {
        delete ddc->_message_context;
    }

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
sp_dyna_draw_context_setup(SPEventContext *ec)
{
    SPDynaDrawContext *ddc = SP_DYNA_DRAW_CONTEXT(ec);

    if (((SPEventContextClass *) parent_class)->setup)
        ((SPEventContextClass *) parent_class)->setup(ec);

    ddc->accumulated = sp_curve_new_sized(32);
    ddc->currentcurve = sp_curve_new_sized(4);

    ddc->cal1 = sp_curve_new_sized(32);
    ddc->cal2 = sp_curve_new_sized(32);

    /* style should be changed when dc->use_calligraphc is touched */
    ddc->currentshape = sp_canvas_item_new(SP_DT_SKETCH(ec->desktop), SP_TYPE_CANVAS_BPATH, NULL);
    sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(ddc->currentshape), DDC_RED_RGBA, SP_WIND_RULE_EVENODD);
    sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(ddc->currentshape), 0x00000000, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
    /* fixme: Cannot we cascade it to root more clearly? */
    g_signal_connect(G_OBJECT(ddc->currentshape), "event", G_CALLBACK(sp_desktop_root_handler), ec->desktop);

    sp_event_context_read(ec, "mass");
    sp_event_context_read(ec, "drag");
    sp_event_context_read(ec, "angle");
    sp_event_context_read(ec, "width");
    sp_event_context_read(ec, "thinning");
    sp_event_context_read(ec, "flatness");

    ddc->is_drawing = false;

    ddc->_message_context = new Inkscape::MessageContext(SP_VIEW(ec->desktop)->messageStack());
}

static void
sp_dyna_draw_context_set(SPEventContext *ec, gchar const *key, gchar const *val)
{
    SPDynaDrawContext *ddc = SP_DYNA_DRAW_CONTEXT(ec);

    if (!strcmp(key, "mass")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : 0.2 );
        ddc->mass = CLAMP(dval, -1000.0, 1000.0);
    } else if (!strcmp(key, "drag")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : DRAG_DEFAULT );
        ddc->drag = CLAMP(dval, DRAG_MIN, DRAG_MAX);
    } else if (!strcmp(key, "angle")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : 0.0);
        ddc->angle = CLAMP (dval, -90, 90);
    } else if (!strcmp(key, "width")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : 0.1 );
        ddc->width = CLAMP(dval, -1000.0, 1000.0);
    } else if (!strcmp(key, "thinning")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : 0.1 );
        ddc->vel_thin = CLAMP(dval, -1.0, 1.0);
    } else if (!strcmp(key, "flatness")) {
        double const dval = ( val ? g_ascii_strtod (val, NULL) : 1.0 );
        ddc->flatness = CLAMP(dval, 0, 1.0);
    }

    //g_print("DDC: %g %g %g %g\n", ddc->mass, ddc->drag, ddc->angle, ddc->width);
}

static double
flerp(double f0, double f1, double p)
{
    return f0 + ( f1 - f0 ) * p;
}

/* Get normalized point */
static NR::Point
sp_dyna_draw_get_npoint(SPDynaDrawContext const *dc, NR::Point v)
{
    NRRect drect;
    sp_desktop_get_display_area(SP_EVENT_CONTEXT(dc)->desktop, &drect);
    double max = MAX (( drect.x1 - drect.x0 ), ( drect.y1 - drect.y0 ));
    return NR::Point(( v[NR::X] - drect.x0 ) / max,  ( v[NR::Y] - drect.y0 ) / max);
}

/* Get view point */
static NR::Point
sp_dyna_draw_get_vpoint(SPDynaDrawContext const *dc, NR::Point n)
{
    NRRect drect;
    sp_desktop_get_display_area(SP_EVENT_CONTEXT(dc)->desktop, &drect);
    double max = MAX (( drect.x1 - drect.x0 ), ( drect.y1 - drect.y0 ));
    return NR::Point(n[NR::X] * max + drect.x0, n[NR::Y] * max + drect.y0);
}

/* Get current view point */
static NR::Point sp_dyna_draw_get_curr_vpoint(SPDynaDrawContext const *dc)
{
    NRRect drect;
    sp_desktop_get_display_area(SP_EVENT_CONTEXT(dc)->desktop, &drect);
    double max = MAX (( drect.x1 - drect.x0 ), ( drect.y1 - drect.y0 ));
    return NR::Point(dc->cur[NR::X] * max + drect.x0, dc->cur[NR::Y] * max + drect.y0);
}

static void
sp_dyna_draw_reset(SPDynaDrawContext *dc, NR::Point p)
{
    dc->last = dc->cur = sp_dyna_draw_get_npoint(dc, p);
    dc->vel = NR::Point(0,0);
    dc->acc = NR::Point(0,0);
    dc->ang = NR::Point(0,0);
    dc->del = NR::Point(0,0);
}

static gboolean
sp_dyna_draw_apply(SPDynaDrawContext *dc, NR::Point p)
{
    NR::Point n = sp_dyna_draw_get_npoint(dc, p);

    /* Calculate mass and drag */
    double const mass = flerp(1.0, 160.0, dc->mass);
    double const drag = flerp(0.0, 0.5, dc->drag * dc->drag);

    /* Calculate force and acceleration */
    NR::Point force = n - dc->cur;
    if ( NR::L2(force) < DYNA_EPSILON ) {
        return FALSE;
    }

    dc->acc = force / mass;

    /* Calculate new velocity */
    dc->vel += dc->acc;

    /* Calculate angle of drawing tool */

    // 1. fixed dc->angle (absolutely flat nib):
    double const radians = ( (dc->angle - 90) / 180.0 ) * M_PI;
    NR::Point ang1 = NR::Point(-sin(radians),  cos(radians));

    // 2. perpendicular to dc->vel (absolutely non-flat nib):
    gdouble const mag_vel = NR::L2(dc->vel);
    if ( mag_vel < DYNA_EPSILON ) {
        return FALSE;
    }
    NR::Point ang2 = NR::rot90(dc->vel) / mag_vel;

    // 3. Average them using flatness parameter:
    // calculate angles
    double a1 = atan2(ang1);
    double a2 = atan2(ang2);
    // flip a2 to force it to be in the same half-circle as a1
    bool flipped = false;
    if (fabs (a2-a1) > 0.5*M_PI) {
        a2 += M_PI;
        flipped = true;
    }
    // normalize a2
    if (a2 > M_PI)
        a2 -= 2*M_PI;
    if (a2 < -M_PI)
        a2 += 2*M_PI;
    // find the flatness-weighted bisector angle, unflip if a2 was flipped
    // FIXME: when dc->vel is oscillating around the fixed angle, the new_ang flips back and forth. How to avoid this?
    double new_ang = a1 + (1 - dc->flatness) * (a2 - a1) - (flipped? M_PI : 0);
    // convert to point
    dc->ang = NR::Point (cos (new_ang), sin (new_ang));

    /* Apply drag */
    dc->vel *= 1.0 - drag;

    /* Update position */
    dc->last = dc->cur;
    dc->cur += dc->vel;

    return TRUE;
}

static void
sp_dyna_draw_brush(SPDynaDrawContext *dc)
{
    g_assert( dc->npoints >= 0 && dc->npoints < SAMPLING_SIZE );

    if (dc->use_calligraphic) {
        /* calligraphics */

        // How much velocity thins strokestyle
        double vel_thin = flerp (0, 160, dc->vel_thin);

        double width = ( 1 - vel_thin * NR::L2(dc->vel) ) * dc->width;
        if ( width < 0.02 * dc->width ) {
            width = 0.02 * dc->width;
        }

        NR::Point del = 0.05 * width * dc->ang;

        dc->point1[dc->npoints] = sp_dyna_draw_get_vpoint(dc, dc->cur + del);
        dc->point2[dc->npoints] = sp_dyna_draw_get_vpoint(dc, dc->cur - del);

        dc->del = del;
    } else {
        dc->point1[dc->npoints] = sp_dyna_draw_get_curr_vpoint(dc);
    }

    dc->npoints++;
}

static gint
sp_dyna_draw_timeout_handler(gpointer data)
{
    SPDynaDrawContext *dc = SP_DYNA_DRAW_CONTEXT(data);
    SPDesktop *desktop = SP_EVENT_CONTEXT(dc)->desktop;
    SPCanvas *canvas = SP_CANVAS(SP_DT_CANVAS(desktop));

    dc->dragging = TRUE;
    dc->dynahand = TRUE;

    int x, y;
    gtk_widget_get_pointer(GTK_WIDGET(canvas), &x, &y);
    NR::Point p = sp_canvas_window_to_world(canvas, NR::Point(x, y));
    p = sp_desktop_w2d_xy_point(desktop, p);
    if (! sp_dyna_draw_apply(dc, p)) {
        return TRUE;
    }
    p = sp_dyna_draw_get_curr_vpoint(dc);
    namedview_free_snap(desktop->namedview, Snapper::SNAP_POINT, p);
    // something's not right here
    if ( dc->cur != dc->last ) {
        sp_dyna_draw_brush(dc);
        g_assert( dc->npoints > 0 );
        fit_and_split(dc, FALSE);
    }

    return TRUE;
}

void
sp_ddc_update_toolbox (SPDesktop *desktop, const gchar *id, double value)
{
    gpointer hb = sp_search_by_data_recursive (desktop->owner->aux_toolbox, (gpointer) id);
    if (hb && GTK_IS_WIDGET(hb) && GTK_IS_SPIN_BUTTON(hb)) {
        GtkAdjustment *a = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON(hb));
        gtk_adjustment_set_value (a, value);
    }
 }

gint
sp_dyna_draw_context_root_handler(SPEventContext *event_context,
                                  GdkEvent *event)
{
    SPDynaDrawContext *dc = SP_DYNA_DRAW_CONTEXT(event_context);
    SPDesktop *desktop = event_context->desktop;

    gint ret = FALSE;

    switch (event->type) {
    case GDK_BUTTON_PRESS:
        if ( event->button.button == 1 ) {

            SPDesktop *desktop = SP_EVENT_CONTEXT_DESKTOP(dc);
            SPItem *layer=SP_ITEM(desktop->currentLayer());
            if ( !layer || desktop->itemIsHidden(layer)) {
                dc->_message_context->flash(Inkscape::WARNING_MESSAGE, _("<b>Current layer is hidden</b>. Unhide it to be able to draw on it."));
                return TRUE;
            }
            if ( !layer || layer->isLocked()) {
                dc->_message_context->flash(Inkscape::WARNING_MESSAGE, _("<b>Current layer is locked</b>. Unlock it to be able to draw on it."));
                return TRUE;
            }

            NR::Point const button_w(event->button.x,
                                     event->button.y);
            NR::Point const button_dt(sp_desktop_w2d_xy_point(desktop, button_w));
            sp_dyna_draw_reset(dc, button_dt);
            sp_dyna_draw_apply(dc, button_dt);
            NR::Point p = sp_dyna_draw_get_curr_vpoint(dc);
            namedview_free_snap(desktop->namedview, Snapper::SNAP_POINT, p);
            /* TODO: p isn't subsequently used; we should probably get rid of the last
               1-2 statements (or use p).  Same for MOTION_NOTIFY below. */
            sp_curve_reset(dc->accumulated);
            if (dc->repr) {
                dc->repr = NULL;
            }

            /* initialize first point */
            dc->npoints = 0;

            sp_canvas_item_grab(SP_CANVAS_ITEM(desktop->acetate),
                                ( dc->use_timeout
                                  ? ( GDK_KEY_PRESS_MASK |
                                      GDK_BUTTON_RELEASE_MASK |
                                      GDK_BUTTON_PRESS_MASK    )
                                  : ( GDK_KEY_PRESS_MASK | 
                                      GDK_BUTTON_RELEASE_MASK |
                                      GDK_POINTER_MOTION_MASK |
                                      GDK_BUTTON_PRESS_MASK    ) ),
                                NULL,
                                event->button.time);

            if ( dc->use_timeout && !dc->timer_id ) {
                dc->timer_id = gtk_timeout_add(SAMPLE_TIMEOUT, sp_dyna_draw_timeout_handler, dc);
            }
            ret = TRUE;

            dc->is_drawing = true;
        }
        break;
    case GDK_MOTION_NOTIFY:
        if ( dc->is_drawing && !dc->use_timeout && ( event->motion.state & GDK_BUTTON1_MASK ) ) {
            dc->dragging = TRUE;
            dc->dynahand = TRUE;

            NR::Point const motion_w(event->motion.x,
                                     event->motion.y);
            NR::Point const motion_dt(sp_desktop_w2d_xy_point(desktop, motion_w));

            if (!sp_dyna_draw_apply(dc, motion_dt)) {
                ret = TRUE;
                break;
            }
            NR::Point p = sp_dyna_draw_get_curr_vpoint(dc);
            namedview_free_snap(desktop->namedview, Snapper::SNAP_POINT, p);
            /* p unused; see comments above in BUTTON_PRESS. */

            if ( dc->cur != dc->last ) {
                sp_dyna_draw_brush(dc);
                g_assert( dc->npoints > 0 );
                fit_and_split(dc, FALSE);
            }
            ret = TRUE;
        }
        break;

    case GDK_BUTTON_RELEASE:
        sp_canvas_item_ungrab(SP_CANVAS_ITEM(desktop->acetate), event->button.time);
        dc->is_drawing = false;
        if ( event->button.button == 1
             && dc->use_timeout
             && dc->timer_id != 0 )
        {
            gtk_timeout_remove(dc->timer_id);
            dc->timer_id = 0;
        }
        if ( dc->dragging && event->button.button == 1 ) {
            dc->dragging = FALSE;

            /* release */
            if (dc->dynahand) {
                dc->dynahand = FALSE;
                /* Remove all temporary line segments */
                while (dc->segments) {
                    gtk_object_destroy(GTK_OBJECT(dc->segments->data));
                    dc->segments = g_slist_remove(dc->segments, dc->segments->data);
                }
                /* Create object */
                fit_and_split(dc, TRUE);
                if (dc->use_calligraphic) {
                    accumulate_calligraphic(dc);
                } else {
                    concat_current_line(dc);
                }
                set_to_accumulated(dc); /* temporal implementation */
                if (dc->use_calligraphic /* || dc->cinside*/) {
                    /* reset accumulated curve */
                    sp_curve_reset(dc->accumulated);
                    clear_current(dc);
                    if (dc->repr) {
                        dc->repr = NULL;
                    }
                }
            }
            ret = TRUE;
        }
        break;
    case GDK_KEY_PRESS:
        switch (event->key.keyval) {
        case GDK_Up:
        case GDK_KP_Up:
            if (!MOD__CTRL_ONLY) {
                dc->angle += 5.0;
                if (dc->angle > 90.0) 
                    dc->angle = 90.0;
                sp_ddc_update_toolbox (desktop, "calligraphy-angle", dc->angle);
                ret = TRUE;
            }
            break;
        case GDK_Down:
        case GDK_KP_Down:
            if (!MOD__CTRL_ONLY) {
                dc->angle -= 5.0;
                if (dc->angle < -90.0) 
                    dc->angle = -90.0;
                sp_ddc_update_toolbox (desktop, "calligraphy-angle", dc->angle);
                ret = TRUE;
            }
            break;
        case GDK_Right:
        case GDK_KP_Right:
            if (!MOD__CTRL_ONLY) {
                dc->width += 0.01;
                if (dc->width > 1.0) 
                    dc->width = 1.0;
                sp_ddc_update_toolbox (desktop, "altx-calligraphy", dc->width); // the same spinbutton is for alt+x
                ret = TRUE;
            }
            break;
        case GDK_Left:
        case GDK_KP_Left:
            if (!MOD__CTRL_ONLY) {
                dc->width -= 0.01;
                if (dc->width < 0.0) 
                    dc->width = 0.0;
                sp_ddc_update_toolbox (desktop, "altx-calligraphy", dc->width); 
                ret = TRUE;
            }
            break;
        case GDK_x:
        case GDK_X:
            if (MOD__ALT_ONLY) {
                gpointer hb = sp_search_by_data_recursive (desktop->owner->aux_toolbox, (gpointer) "altx-calligraphy");
                if (hb && GTK_IS_WIDGET(hb)) {
                    gtk_widget_grab_focus (GTK_WIDGET (hb));
                }
                ret = TRUE;
            }
            break;

        default:
            break;
        }
    default:
        break;
    }

    if (!ret) {
        if (((SPEventContextClass *) parent_class)->root_handler) {
            ret = ((SPEventContextClass *) parent_class)->root_handler(event_context, event);
        }
    }

    return ret;
}


static void
clear_current(SPDynaDrawContext *dc)
{
    /* reset bpath */
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(dc->currentshape), NULL);
    /* reset curve */
    sp_curve_reset(dc->currentcurve);
    sp_curve_reset(dc->cal1);
    sp_curve_reset(dc->cal2);
    /* reset points */
    dc->npoints = 0;
}

static void
set_to_accumulated(SPDynaDrawContext *dc)
{
    SPDesktop *desktop = SP_EVENT_CONTEXT(dc)->desktop;

    if (!sp_curve_empty(dc->accumulated)) {
        NArtBpath *abp;
        gchar *str;

        if (!dc->repr) {
            /* Create object */
            SPRepr *repr = sp_repr_new("svg:path");

            /* Set style */
            sp_desktop_apply_style_tool (desktop, repr, "tools.calligraphic", false);

            dc->repr = repr;

            SPItem *item=SP_ITEM(desktop->currentLayer()->appendChildRepr(dc->repr));
            sp_repr_unref(dc->repr);
            item->transform = SP_ITEM(desktop->currentRoot())->getRelativeTransform(desktop->currentLayer());
            item->updateRepr();
            SP_DT_SELECTION(desktop)->setRepr(dc->repr);
        }
        abp = nr_artpath_affine(sp_curve_first_bpath(dc->accumulated), sp_desktop_dt2root_affine(desktop));
        str = sp_svg_write_path(abp);
        g_assert( str != NULL );
        nr_free(abp);
        sp_repr_set_attr(dc->repr, "d", str);
        g_free(str);
    } else {
        if (dc->repr) {
            sp_repr_unparent(dc->repr);
        }
        dc->repr = NULL;
    }

    sp_document_done(SP_DT_DOCUMENT(desktop));
}

static void
concat_current_line(SPDynaDrawContext *dc)
{
    if (!sp_curve_empty(dc->currentcurve)) {
        NArtBpath *bpath;
        if (sp_curve_empty(dc->accumulated)) {
            bpath = sp_curve_first_bpath(dc->currentcurve);
            g_assert( bpath->code == NR_MOVETO_OPEN );
            sp_curve_moveto(dc->accumulated, bpath->x3, bpath->y3);
        }
        bpath = sp_curve_last_bpath(dc->currentcurve);
        if ( bpath->code == NR_CURVETO ) {
            sp_curve_curveto(dc->accumulated,
                             bpath->x1, bpath->y1,
                             bpath->x2, bpath->y2,
                             bpath->x3, bpath->y3);
        } else if ( bpath->code == NR_LINETO ) {
            sp_curve_lineto(dc->accumulated, bpath->x3, bpath->y3);
        } else {
            g_assert_not_reached();
        }
    }
}

static void
accumulate_calligraphic(SPDynaDrawContext *dc)
{
    if ( !sp_curve_empty(dc->cal1) && !sp_curve_empty(dc->cal2) ) {
        sp_curve_reset(dc->accumulated); /*  Is this required ?? */
        SPCurve *rev_cal2 = sp_curve_reverse(dc->cal2);
        sp_curve_append(dc->accumulated, dc->cal1, FALSE);
        sp_curve_append(dc->accumulated, rev_cal2, TRUE);
        sp_curve_closepath(dc->accumulated);

        sp_curve_unref(rev_cal2);

        sp_curve_reset(dc->cal1);
        sp_curve_reset(dc->cal2);
    }
}

static void
fit_and_split(SPDynaDrawContext *dc,
              gboolean release)
{
    if (dc->use_calligraphic) {
        fit_and_split_calligraphics(dc, release);
    } else {
        fit_and_split_line(dc, release);
    }
}

static double square(double const x)
{
    return x * x;
}

static void
fit_and_split_line(SPDynaDrawContext *dc,
                   gboolean release)
{
    double const tolerance_sq = square( NR::expansion(SP_EVENT_CONTEXT(dc)->desktop->w2d) * TOLERANCE_LINE );

    NR::Point b[4];
    double const n_segs = sp_bezier_fit_cubic(b, dc->point1, dc->npoints, tolerance_sq);
    if ( n_segs > 0
         && dc->npoints < SAMPLING_SIZE )
    {
        /* Fit and draw and reset state */
#ifdef DYNA_DRAW_VERBOSE
        g_print("%d", dc->npoints);
#endif
        sp_curve_reset(dc->currentcurve);
        sp_curve_moveto(dc->currentcurve, b[0]);
        sp_curve_curveto(dc->currentcurve, b[1], b[2], b[3]);
        sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(dc->currentshape), dc->currentcurve);
    } else {
        /* Fit and draw and copy last point */
#ifdef DYNA_DRAW_VERBOSE
        g_print("[%d]Yup\n", dc->npoints);
#endif
        g_assert(!sp_curve_empty(dc->currentcurve));
        concat_current_line(dc);

        SPCanvasItem *cbp = sp_canvas_item_new(SP_DT_SKETCH(SP_EVENT_CONTEXT(dc)->desktop),
                                               SP_TYPE_CANVAS_BPATH,
                                               NULL);
        SPCurve *curve = sp_curve_copy(dc->currentcurve);
        sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(cbp), curve);
        sp_curve_unref(curve);
        /* fixme: We have to parse style color somehow */
        sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(cbp), DDC_GREEN_RGBA, SP_WIND_RULE_EVENODD);
        sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(cbp), 0x000000ff, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
        /* fixme: Cannot we cascade it to root more clearly? */
        g_signal_connect(G_OBJECT(cbp), "event", G_CALLBACK(sp_desktop_root_handler), SP_EVENT_CONTEXT(dc)->desktop);

        dc->segments = g_slist_prepend(dc->segments, cbp);
        dc->point1[0] = dc->point1[dc->npoints - 2];
        dc->npoints = 1;
    }
}

static void
fit_and_split_calligraphics(SPDynaDrawContext *dc, gboolean release)
{
    double const tolerance_sq = square( NR::expansion(SP_EVENT_CONTEXT(dc)->desktop->w2d) * TOLERANCE_CALLIGRAPHIC );

#ifdef DYNA_DRAW_VERBOSE
    g_print("[F&S:R=%c]", release?'T':'F');
#endif

    if (!( dc->npoints > 0 && dc->npoints < SAMPLING_SIZE ))
        return; // just clicked

    if ( dc->npoints == SAMPLING_SIZE - 1 || release ) {
#define BEZIER_SIZE       4
#define BEZIER_MAX_BEZIERS  8
#define BEZIER_MAX_LENGTH ( BEZIER_SIZE * BEZIER_MAX_BEZIERS )

#ifdef DYNA_DRAW_VERBOSE
        g_print("[F&S:#] dc->npoints:%d, release:%s\n",
                dc->npoints, release ? "TRUE" : "FALSE");
#endif

        /* Current calligraphic */
        if ( dc->cal1->end == 0 || dc->cal2->end == 0 ) {
            /* dc->npoints > 0 */
            /* g_print("calligraphics(1|2) reset\n"); */
            sp_curve_reset(dc->cal1);
            sp_curve_reset(dc->cal2);

            sp_curve_moveto(dc->cal1, dc->point1[0]);
            sp_curve_moveto(dc->cal2, dc->point2[0]);
        }

        NR::Point b1[BEZIER_MAX_LENGTH];
        gint const nb1 = sp_bezier_fit_cubic_r(b1, dc->point1, dc->npoints,
                                               tolerance_sq, BEZIER_MAX_BEZIERS);
        g_assert( nb1 * BEZIER_SIZE <= gint(G_N_ELEMENTS(b1)) );

        NR::Point b2[BEZIER_MAX_LENGTH];
        gint const nb2 = sp_bezier_fit_cubic_r(b2, dc->point2, dc->npoints,
                                               tolerance_sq, BEZIER_MAX_BEZIERS);
        g_assert( nb2 * BEZIER_SIZE <= gint(G_N_ELEMENTS(b2)) );

        if ( nb1 != -1 && nb2 != -1 ) {
            /* Fit and draw and reset state */
#ifdef DYNA_DRAW_VERBOSE
            g_print("nb1:%d nb2:%d\n", nb1, nb2);
#endif
            /* CanvasShape */
            if (! release) {
                sp_curve_reset(dc->currentcurve);
                sp_curve_moveto(dc->currentcurve, b1[0]);
                for (NR::Point *bp1 = b1; bp1 < b1 + BEZIER_SIZE * nb1; bp1 += BEZIER_SIZE) {
                    sp_curve_curveto(dc->currentcurve, bp1[1],
                                     bp1[2], bp1[3]);
                }
                sp_curve_lineto(dc->currentcurve,
                                b2[BEZIER_SIZE*(nb2-1) + 3]);
                for (NR::Point *bp2 = b2 + BEZIER_SIZE * ( nb2 - 1 ); bp2 >= b2; bp2 -= BEZIER_SIZE) {
                    sp_curve_curveto(dc->currentcurve, bp2[2], bp2[1], bp2[0]);
                }
                sp_curve_closepath(dc->currentcurve);
                sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(dc->currentshape), dc->currentcurve);
            }

            /* Current calligraphic */
            for (NR::Point *bp1 = b1; bp1 < b1 + BEZIER_SIZE * nb1; bp1 += BEZIER_SIZE) {
                sp_curve_curveto(dc->cal1, bp1[1], bp1[2], bp1[3]);
            }
            for (NR::Point *bp2 = b2; bp2 < b2 + BEZIER_SIZE * nb2; bp2 += BEZIER_SIZE) {
                sp_curve_curveto(dc->cal2, bp2[1], bp2[2], bp2[3]);
            }
        } else {
            /* fixme: ??? */
#ifdef DYNA_DRAW_VERBOSE
            g_print("[fit_and_split_calligraphics] failed to fit-cubic.\n");
#endif
            draw_temporary_box(dc);

            for (gint i = 1; i < dc->npoints; i++) {
                sp_curve_lineto(dc->cal1, dc->point1[i]);
            }
            for (gint i = 1; i < dc->npoints; i++) {
                sp_curve_lineto(dc->cal2, dc->point2[i]);
            }
        }

        /* Fit and draw and copy last point */
#ifdef DYNA_DRAW_VERBOSE
        g_print("[%d]Yup\n", dc->npoints);
#endif
        if (!release) {
            g_assert(!sp_curve_empty(dc->currentcurve));

            SPCanvasItem *cbp = sp_canvas_item_new(SP_DT_SKETCH(SP_EVENT_CONTEXT(dc)->desktop),
                                                   SP_TYPE_CANVAS_BPATH,
                                                   NULL);
            SPCurve *curve = sp_curve_copy(dc->currentcurve);
            sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH (cbp), curve);
            sp_curve_unref(curve);
            sp_canvas_bpath_set_fill(SP_CANVAS_BPATH(cbp), 0x000000ff, SP_WIND_RULE_EVENODD);
            sp_canvas_bpath_set_stroke(SP_CANVAS_BPATH(cbp), 0x00000000, 1.0, SP_STROKE_LINEJOIN_MITER, SP_STROKE_LINECAP_BUTT);
            /* fixme: Cannot we cascade it to root more clearly? */
            g_signal_connect(G_OBJECT(cbp), "event", G_CALLBACK(sp_desktop_root_handler), SP_EVENT_CONTEXT(dc)->desktop);

            dc->segments = g_slist_prepend(dc->segments, cbp);
        }

        dc->point1[0] = dc->point1[dc->npoints - 1];
        dc->point2[0] = dc->point2[dc->npoints - 1];
        dc->npoints = 1;
    } else {
        draw_temporary_box(dc);
    }
}

static void
draw_temporary_box(SPDynaDrawContext *dc)
{
    sp_curve_reset(dc->currentcurve);
    sp_curve_moveto(dc->currentcurve, dc->point1[0]);
    for (gint i = 1; i < dc->npoints; i++) {
        sp_curve_lineto(dc->currentcurve, dc->point1[i]);
    }
    for (gint i = dc->npoints-1; i >= 0; i--) {
        sp_curve_lineto(dc->currentcurve, dc->point2[i]);
    }
    sp_curve_closepath(dc->currentcurve);
    sp_canvas_bpath_set_bpath(SP_CANVAS_BPATH(dc->currentshape), dc->currentcurve);
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=c++:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :

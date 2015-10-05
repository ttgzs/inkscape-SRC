/**
 * @file
 * Measure aux toolbar
 */
/* Authors:
 *   MenTaLguY <mental@rydia.net>
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Frank Felfe <innerspace@iname.com>
 *   John Cliff <simarilius@yahoo.com>
 *   David Turner <novalis@gnu.org>
 *   Josh Andler <scislac@scislac.com>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Maximilian Albert <maximilian.albert@gmail.com>
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Abhishek Sharma
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2004 David Turner
 * Copyright (C) 2003 MenTaLguY
 * Copyright (C) 1999-2011 authors
 * Copyright (C) 2001-2002 Ximian, Inc.
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glibmm/i18n.h>

#include "measure-toolbar.h"

#include "desktop.h"
#include "inkscape.h"
#include "message-stack.h"
#include "document-undo.h"
#include "widgets/ege-adjustment-action.h"
#include "widgets/ege-output-action.h"
#include "preferences.h"
#include "toolbox.h"
#include "widgets/ink-action.h"
#include "ui/icon-names.h"
#include "ui/tools/measure-tool.h"
#include "ui/widget/unit-tracker.h"

using Inkscape::UI::Widget::UnitTracker;
using Inkscape::Util::Unit;
using Inkscape::DocumentUndo;
using Inkscape::UI::ToolboxFactory;
using Inkscape::UI::PrefPusher;
using Inkscape::UI::Tools::MeasureTool;

//########################
//##  Measure Toolbox   ##
//########################

/** Temporary hack: Returns the node tool in the active desktop.
 * Will go away during tool refactoring. */
static MeasureTool *get_measure_tool()
{
    MeasureTool *tool = 0;
    if (SP_ACTIVE_DESKTOP ) {
        Inkscape::UI::Tools::ToolBase *ec = SP_ACTIVE_DESKTOP->event_context;
        if (SP_IS_MEASURE_CONTEXT(ec)) {
            tool = static_cast<MeasureTool*>(ec);
        }
    }
    return tool;
}

static void
sp_measure_fontsize_value_changed(GtkAdjustment *adj, GObject *tbl)
{
    SPDesktop *desktop = static_cast<SPDesktop *>(g_object_get_data( tbl, "desktop" ));

    if (DocumentUndo::getUndoSensitive(desktop->getDocument())) {
        Inkscape::Preferences *prefs = Inkscape::Preferences::get();
        prefs->setInt(Glib::ustring("/tools/measure/fontsize"),
            gtk_adjustment_get_value(adj));
        MeasureTool *mt = get_measure_tool();
        if (mt) {
            mt->showCanvasItems();
        }
    }
}

static void measure_unit_changed(GtkAction* /*act*/, GObject* tbl)
{
    UnitTracker* tracker = reinterpret_cast<UnitTracker*>(g_object_get_data(tbl, "tracker"));
    Glib::ustring const unit = tracker->getActiveUnit()->abbr;
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    prefs->setString("/tools/measure/unit", unit);
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->showCanvasItems();
    }
}

static void toggle_ignore_1st_and_last( GtkToggleAction* act, gpointer data )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(act);
    prefs->setInt("/tools/measure/ignore_1st_and_last", active);
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    if ( active ) {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Start and end measures inactive."));
    } else {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Start and end measures active."));
    }
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->showCanvasItems();
    }
}

static void toggle_all_layers( GtkToggleAction* act, gpointer data )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(act);
    prefs->setInt("/tools/measure/all_layers", active);
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    if ( active ) {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Use all layers in the measure."));
    } else {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Use current layer in the measure."));
    }
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->showCanvasItems();
    }
}

static void toggle_show_in_between( GtkToggleAction* act, gpointer data )
{
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    gboolean active = gtk_toggle_action_get_active(act);
    prefs->setInt("/tools/measure/show_in_between", active);
    SPDesktop *desktop = static_cast<SPDesktop *>(data);
    if ( active ) {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Compute all elements."));
    } else {
        desktop->messageStack()->flash(Inkscape::INFORMATION_MESSAGE, _("Compute max lenght."));
    }
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->showCanvasItems();
    }
}
static void sp_reverse_knots(void){
    MeasureTool *mt = get_measure_tool();
    if (mt) {
        mt->reverseKnots();
    }
}
void sp_measure_toolbox_prep(SPDesktop * desktop, GtkActionGroup* mainActions, GObject* holder)
{
    UnitTracker* tracker = new UnitTracker(Inkscape::Util::UNIT_TYPE_LINEAR);
    Inkscape::Preferences *prefs = Inkscape::Preferences::get();
    tracker->setActiveUnitByAbbr(prefs->getString("/tools/measure/unit").c_str());
    
    g_object_set_data( holder, "tracker", tracker );

    EgeAdjustmentAction *eact = 0;
    Inkscape::IconSize secondarySize = ToolboxFactory::prefToSize("/toolbox/secondary", 1);

    /* Font Size */
    {
        eact = create_adjustment_action( "MeasureFontSizeAction",
                                         _("Font Size"), _("Font Size:"),
                                         _("The font size to be used in the measurement labels"),
                                         "/tools/measure/fontsize", 0.0,
                                         GTK_WIDGET(desktop->canvas), holder, FALSE, NULL,
                                         10, 36, 1.0, 4.0,
                                         0, 0, 0,
                                         sp_measure_fontsize_value_changed);
        gtk_action_group_add_action( mainActions, GTK_ACTION(eact) );
    }


    // units label
    {
        EgeOutputAction* act = ege_output_action_new( "measure_units_label", _("Units:"), _("The units to be used for the measurements"), 0 );
        ege_output_action_set_use_markup( act, TRUE );
        g_object_set( act, "visible-overflown", FALSE, NULL );
        gtk_action_group_add_action( mainActions, GTK_ACTION( act ) );
    }

    // units menu
    {
        GtkAction* act = tracker->createAction( "MeasureUnitsAction", _("Units:"), _("The units to be used for the measurements") );
        g_signal_connect_after( G_OBJECT(act), "changed", G_CALLBACK(measure_unit_changed), holder );
        gtk_action_group_add_action( mainActions, act );
    }
    // ignore_1st_and_last
    {
        InkToggleAction* act = ink_toggle_action_new( "MeasureIgnore1stAndLast",
                                                      _("Ignore first and last"),
                                                      _("Ignore first and last"),
                                                      INKSCAPE_ICON("draw-geometry-line-segment"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), prefs->getBool("/tools/measure/ignore_1st_and_last", true) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_ignore_1st_and_last), desktop) ;
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
    }
    // measure imbetweens
    {
        InkToggleAction* act = ink_toggle_action_new( "MeasureInBettween",
                                                      _("Show measures between items"),
                                                      _("Show measures between items"),
                                                      INKSCAPE_ICON("distribute-randomize"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), prefs->getBool("/tools/measure/show_in_between", true) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_show_in_between), desktop) ;
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
    }
    // measure only current layer
    {
        InkToggleAction* act = ink_toggle_action_new( "MeasureAllLayers",
                                                      _("Measure all layers"),
                                                      _("Measure all layers"),
                                                      INKSCAPE_ICON("dialog-layers"),
                                                      secondarySize );
        gtk_toggle_action_set_active( GTK_TOGGLE_ACTION(act), prefs->getBool("/tools/measure/all_layers", true) );
        g_signal_connect_after( G_OBJECT(act), "toggled", G_CALLBACK(toggle_all_layers), desktop) ;
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
    }
    //toogle start end
    {
        InkAction* act = ink_action_new( "MeasureReverse",
                                          _("Reverse measure"),
                                          _("Reverse measure"),
                                          INKSCAPE_ICON("draw-geometry-mirror"),
                                          secondarySize );
        g_signal_connect_after( G_OBJECT(act), "activate", G_CALLBACK(sp_reverse_knots), 0 );
        gtk_action_group_add_action( mainActions, GTK_ACTION(act) );
    }
} // end of sp_measure_toolbox_prep()


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

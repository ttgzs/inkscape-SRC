/**
 *
 * \brief  Dialog for renaming layers
 *
 * Author:
 *   Bryce W. Harrington <bryce@bryceharrington.com>
 *
 * Copyright (C) 2004 Bryce Harrington
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtkmm/dialog.h>
#include <gtkmm/notebook.h>
#include <gtkmm/separator.h>
#include <gtkmm/frame.h>
#include <gtkmm/table.h>
#include <gtkmm/stock.h>

#include "dialogs/dialog-events.h"
#include "helper/sp-intl.h"
#include "inkscape.h"
#include "desktop.h"
#include "document.h"
#include "desktop-handles.h"
#include "layer-fns.h"

#include "layer-properties.h"

namespace Inkscape {
namespace UI {
namespace Dialogs {

LayerPropertiesDialog::LayerPropertiesDialog()
: _strategy(NULL), _desktop(NULL), _layer(NULL)
{
    GtkWidget *dlg = GTK_WIDGET(gobj());
    g_assert(dlg);

    Gtk::VBox *mainVBox = get_vbox();

    // Layer name widgets
    _layer_name_entry.set_activates_default(true);
    _layer_name_hbox.pack_end(_layer_name_entry, false, false, 4);
    _layer_name_label.set_label(_("Layer Name:"));
    _layer_name_hbox.pack_end(_layer_name_label, false, false, 4);
    mainVBox->pack_start(_layer_name_hbox, false, false, 4);

    // Buttons
    _close_button.set_use_stock(true);
    _close_button.set_label(Gtk::Stock::CANCEL.id);
    _close_button.set_flags(Gtk::CAN_DEFAULT);

    _apply_button.set_use_underline(true);
    _apply_button.set_flags(Gtk::CAN_DEFAULT);

    _close_button.signal_clicked()
        .connect(sigc::mem_fun(*this, &LayerPropertiesDialog::_close));
    _apply_button.signal_clicked()
        .connect(sigc::mem_fun(*this, &LayerPropertiesDialog::_apply));

    signal_delete_event().connect(
        sigc::bind_return(
            sigc::hide(sigc::mem_fun(*this, &LayerPropertiesDialog::_close)),
            true
        )
    );

    add_action_widget(_close_button, Gtk::RESPONSE_CLOSE);
    add_action_widget(_apply_button, Gtk::RESPONSE_APPLY);

    show_all_children();
}

LayerPropertiesDialog::~LayerPropertiesDialog() {
    _setDesktop(NULL);
    _setLayer(NULL);
}

void LayerPropertiesDialog::_showDialog(LayerPropertiesDialog::Strategy &strategy,
                                        SPDesktop *desktop, SPObject *layer)
{
    LayerPropertiesDialog *dialog = new LayerPropertiesDialog();

    dialog->_strategy = &strategy;
    dialog->_setDesktop(desktop);
    dialog->_setLayer(layer);

    dialog->_strategy->setup(*dialog);

    dialog->set_modal(true);
    gtk_window_set_transient_for(GTK_WINDOW(dialog->gobj()), GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(desktop->owner))));
    dialog->property_destroy_with_parent() = true;

    dialog->show();
    dialog->present();
}

void
LayerPropertiesDialog::_apply()
{
    g_assert(_strategy != NULL);

    _strategy->perform(*this);
    sp_document_done(SP_DT_DOCUMENT(SP_ACTIVE_DESKTOP));

    _close();
}

void
LayerPropertiesDialog::_close()
{
    _setLayer(NULL);
    _setDesktop(NULL);
    destroy_();
    Glib::signal_idle().connect(
        sigc::bind_return(
            sigc::bind(sigc::ptr_fun(&::operator delete), this),
            false 
        )
    );
}

void LayerPropertiesDialog::Rename::setup(LayerPropertiesDialog &dialog) {
    SPDesktop *desktop=dialog._desktop;
    dialog.set_title(_("Rename Layer"));
    gchar const *name = desktop->currentLayer()->label();
    dialog._layer_name_entry.set_text(( name ? name : "" ));
    dialog._apply_button.set_label(_("_Rename"));
}

void LayerPropertiesDialog::Rename::perform(LayerPropertiesDialog &dialog) {
    SPDesktop *desktop=dialog._desktop;
    Glib::ustring name(dialog._layer_name_entry.get_text());
    desktop->currentLayer()->setLabel(
        ( name.empty() ? NULL : (gchar *)name.c_str() )
    );
    sp_document_done(SP_DT_DOCUMENT(desktop));
    // TRANSLATORS: This means "The layer has been renamed"
    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("Renamed layer"));
}

void LayerPropertiesDialog::Create::setup(LayerPropertiesDialog &dialog) {
    dialog.set_title(_("Create Layer"));
    dialog._layer_name_entry.set_text("");
    dialog._apply_button.set_label(_("C_reate"));
}

void LayerPropertiesDialog::Create::perform(LayerPropertiesDialog &dialog) {
    SPDesktop *desktop=dialog._desktop;
    SPObject *new_layer=Inkscape::create_layer(
        desktop->currentRoot(), dialog._layer
    );
    Glib::ustring name(dialog._layer_name_entry.get_text());
    if (!name.empty()) {
        new_layer->setLabel((gchar *)name.c_str());
    }
    SP_DT_SELECTION(desktop)->clear();
    desktop->setCurrentLayer(new_layer);
    desktop->messageStack()->flash(Inkscape::NORMAL_MESSAGE, _("New layer created."));
}

void LayerPropertiesDialog::_setDesktop(SPDesktop *desktop) {
    if (desktop) {
        g_object_ref(desktop);
    }
    if (_desktop) {
        g_object_unref(_desktop);
    }
    _desktop = desktop;
}

void LayerPropertiesDialog::_setLayer(SPObject *layer) {
    if (layer) {
        sp_object_ref(layer, NULL);
    }
    if (_layer) {
        sp_object_unref(_layer, NULL);
    }
    _layer = layer;
}

} // namespace
} // namespace
} // namespace


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

// SPDX-License-Identifier: GPL-2.0-or-later
/**
 * @file
 * Inkview - An SVG file viewer.
 */
/*
 * Authors:
 *   Tavmjong Bah
 *
 * Copyright (C) 2018 Authors
 *
 * The contents of this file may be used under the GNU General Public License Version 2 or later.
 * Read the file 'COPYING' for more information.
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"      // Defines ENABLE_NLS
#endif

#include <iostream>

#include <glibmm/i18n.h>  // Internationalization

#include "inkview-application.h"

#include "inkscape.h"             // Inkscape::Application
#include "inkscape-version.h"     // Inkscape version
#include "inkgc/gc-core.h"        // Garbage Collecting init
#include "inkview-window.h"

#ifdef ENABLE_NLS
// Native Language Support - shouldn't this always be used?
#include "helper/gettext.h"   // gettext init
#endif // ENABLE_NLS

// This is a bit confusing as there are two ways to handle command line arguments and files
// depending on if the Gio::APPLICATION_HANDLES_OPEN and/or Gio::APPLICATION_HANDLES_COMMAND_LINE
// flags are set. If the open flag is set and the command line not, the all the remainng arguments
// after calling on_handle_local_options() are assumed to be filenames.

InkviewApplication::InkviewApplication()
    : Gtk::Application("org.inkscape.application.inkview",
                       Gio::APPLICATION_HANDLES_OPEN | // Use default file opening.
                       Gio::APPLICATION_NON_UNIQUE   ) // Allows different instances of Inkview to run at same time.
    , fullscreen(false)
    , recursive(false)
    , timer(0)
    , scale(1.0)
    , preload(false)
{
    // ==================== Initializations =====================
    // Garbage Collector
    Inkscape::GC::init();

#ifdef ENABLE_NLS
    // Native Language Support (shouldn't this always be used?).
    Inkscape::initialize_gettext();
#endif

    Glib::set_application_name(N_("Inkview - An SVG File Viewer"));  // After gettext() init.

    // Will automatically handle character conversions.
    // Note: OPTION_TYPE_FILENAME => std::string, OPTION_TYPE_STRING => Glib::ustring.

    add_main_option_entry(OPTION_TYPE_BOOL,     "version",    'V', N_("Print: Inkview version."),                                                          "");
    add_main_option_entry(OPTION_TYPE_BOOL,     "fullscreen", 'f', N_("Launch in fullscreen mode"),                   "");
    add_main_option_entry(OPTION_TYPE_BOOL,     "recursive",  'r', N_("Search folders recursively"),                  "");
    add_main_option_entry(OPTION_TYPE_INT,      "timer",      't', N_("Change image every NUMBER seconds"), N_("NUMBER"));
    add_main_option_entry(OPTION_TYPE_DOUBLE,   "scale",      's', N_("Scale image by factor NUMBER"),      N_("NUMBER"));
    add_main_option_entry(OPTION_TYPE_BOOL,     "preload",    'p', N_("Preload files"),                               "");
   
    signal_handle_local_options().connect(sigc::mem_fun(*this, &InkviewApplication::on_handle_local_options));

    // This is normally called for us... but after the "handle_local_options" signal is emitted. If
    // we want to rely on actions for handling options, we need to call it here. This appears to
    // have no unwanted side-effect. It will also trigger the call to on_startup().
    register_application();
}

Glib::RefPtr<InkviewApplication> InkviewApplication::create()
{
    return Glib::RefPtr<InkviewApplication>(new InkviewApplication());
}

void
InkviewApplication::on_startup()
{
    Gtk::Application::on_startup();

    // Inkscape::Application should disappear!
    Inkscape::Application::create(true);
}


// Open document window with default document. Either this or on_open() is called.
void
InkviewApplication::on_activate()
{
    std::cerr << "InkviewApplication: No documents to open!" << std::endl;
}

// Open document window for each file. Either this or on_activate() is called.
void
InkviewApplication::on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& hint)
{
    window = new InkviewWindow(files, fullscreen, recursive, timer, scale, preload);
    window->show_all();
    add_window(*window);
}


// ========================= Callbacks ==========================

/*
 * Handle command line options callback.
 */
int
InkviewApplication::on_handle_local_options(const Glib::RefPtr<Glib::VariantDict>& options)
{
    if (!options) {
        std::cerr << "InkviewApplication::on_handle_local_options: options is null!" << std::endl;
        return -1; // Keep going
    }

    if (options->contains("version")) {
        std::cout << "Inkscape " << Inkscape::version_string << std::endl;
        return EXIT_SUCCESS;
    }

    if (options->contains("fullscreen")) {
        fullscreen = true;
    }

    if (options->contains("recursive")) {
        recursive = true;
    }

    if (options->contains("timer")) {
        options->lookup_value("timer", timer);
    }

    if (options->contains("scale")) {
        options->lookup_value("scale", scale);
    }

    if (options->contains("preload")) {
        options->lookup_value("preload", preload);
    }

    return -1; // Keep going
}

/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
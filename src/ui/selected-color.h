/** @file
 * Color selected in color selector widget.
 * This file was created during the refactoring of SPColorSelector
 *//*
 * Authors:
 *	 bulia byak <buliabyak@users.sf.net>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Tomasz Boczkowski <penginsbacon@gmail.com>
 *
 * Copyright (C) 2014 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */
#ifndef SEEN_SELECTED_COLOR
#define SEEN_SELECTED_COLOR

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if GLIBMM_DISABLE_DEPRECATED && HAVE_GLIBMM_THREADS_H
#include <glibmm/threads.h>
#endif

#include <sigc++/signal.h>
#include <gtkmm/widget.h>
#include "color.h"

namespace Inkscape {
namespace UI {

class SelectedColor {
public:
    SelectedColor();
    virtual ~SelectedColor();

    void setColor(SPColor const &color);
    SPColor color() const;

    void setAlpha(gfloat alpha);
    gfloat alpha() const;

    void setColorAlpha(SPColor const &color, gfloat alpha, bool emit = false);
    void colorAlpha(SPColor &color, gfloat &alpha) const;

    sigc::signal<void> signal_changed;
private:
    // By default, disallow copy constructor and assignment operator
    SelectedColor(SelectedColor const &obj);
    SelectedColor& operator=(SelectedColor const &obj);

    SPColor _color;
    /**
     * Color alpha value guaranteed to be in [0, 1].
     */
    gfloat _alpha;

    /**
     * This flag is true if no color is set yet
     */
    bool _virgin;

    static double const _EPSILON;
};

class ColorSelectorFactory {
public:
    virtual ~ColorSelectorFactory() {
    }

    virtual Gtk::Widget* createWidget(SelectedColor &color) const = 0;
    virtual Glib::ustring modeName() const = 0;
};

}
}

#endif
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

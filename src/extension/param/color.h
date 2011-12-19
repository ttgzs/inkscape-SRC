#ifndef SEEN_INK_EXTENSION_PARAMCOLOR_H__
#define SEEN_INK_EXTENSION_PARAMCOLOR_H__
/*
 * Copyright (C) 2005-2007 Authors:
 *   Ted Gould <ted@gould.cx>
 *   Johan Engelen <johan@shouraizou.nl> *
 *   Jon A. Cruz <jon@joncruz.org>
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <gtkmm/widget.h>
#include "xml/node.h"
#include "document.h"
#include <color.h>
#include "extension/param/parameter.h"

namespace Inkscape {
namespace Extension {

class ParamColor : public Parameter {
private:
    guint32 _value;
public:
    ParamColor(const gchar * name, const gchar * guitext, const gchar * desc, const Parameter::_scope_t scope, bool gui_hidden, const gchar * gui_tip, Inkscape::Extension::Extension * ext, Inkscape::XML::Node * xml);

    virtual ~ParamColor(void);

    /** Returns \c _value, with a \i const to protect it. */
    guint32 get( SPDocument const * /*doc*/, Inkscape::XML::Node const * /*node*/ ) const { return _value; }

    guint32 set (guint32 in, SPDocument * doc, Inkscape::XML::Node * node);

    Gtk::Widget * get_widget(SPDocument * doc, Inkscape::XML::Node * node, sigc::signal<void> * changeSignal);

    // Explicitly call superclass version to avoid method being hidden.
    virtual void string(std::list <std::string> &list) const { return Parameter::string(list); }

    virtual void string (std::string &string) const;

    sigc::signal<void> * _changeSignal;
}; // class ParamColor

}  // namespace Extension
}  // namespace Inkscape

#endif // SEEN_INK_EXTENSION_PARAMCOLOR_H__

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

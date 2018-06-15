/*
 * The reference corresponding to the inkscape:live-effect attribute
 *
 * Copyright (C) 2007 Johan Engelen
 *
 * Released under GNU GPL, read the file 'COPYING' for more information.
 */

#include "live_effects/lpeobject-reference.h"

#include <string.h>

#include "bad-uri-exception.h"
#include "live_effects/lpeobject.h"
#include "object/uri.h"

namespace Inkscape {

namespace LivePathEffect {

static void lpeobjectreference_href_changed(SPObject *old_ref, SPObject *ref, LPEObjectReference *lpeobjref);
static void lpeobjectreference_delete_self(SPObject *deleted, LPEObjectReference *lpeobjref);
static void lpeobjectreference_source_modified(SPObject *iSource, guint flags, LPEObjectReference *lpeobjref);

LPEObjectReference::LPEObjectReference(SPObject* i_owner) : URIReference(i_owner)
{
    owner=i_owner;
    lpeobject_href = nullptr;
    lpeobject_repr = nullptr;
    lpeobject = nullptr;
    _changed_connection = changedSignal().connect(sigc::bind(sigc::ptr_fun(lpeobjectreference_href_changed), this)); // listening to myself, this should be virtual instead

    user_unlink = nullptr;
}

LPEObjectReference::~LPEObjectReference(void)
{
    _changed_connection.disconnect(); // to do before unlinking

    quit_listening();
    unlink();
}

bool LPEObjectReference::_acceptObject(SPObject * const obj) const
{
    if (IS_LIVEPATHEFFECT(obj)) {
        return URIReference::_acceptObject(obj);
    } else {
        return false;
    }
}

void
LPEObjectReference::link(const char *to)
{
    if ( to && !to[0] ) {
        quit_listening();
        unlink();
    } else {
        if ( !lpeobject_href || ( strcmp(to, lpeobject_href) != 0 ) ) {
            if (lpeobject_href) {
                g_free(lpeobject_href);
            }
            lpeobject_href = g_strdup(to);
            try {
                attach(Inkscape::URI(to));
            } catch (Inkscape::BadURIException &e) {
                /* TODO: Proper error handling as per
                 * http://www.w3.org/TR/SVG11/implnote.html#ErrorProcessing.
                 */
                g_warning("%s", e.what());
                detach();
            }
        }
    }
}

void
LPEObjectReference::unlink(void)
{
    if (lpeobject_href) {
        g_free(lpeobject_href);
        lpeobject_href = nullptr;
    }
    detach();
}

void
LPEObjectReference::start_listening(LivePathEffectObject* to)
{
    if ( to == nullptr ) {
        return;
    }
    lpeobject = to;
    lpeobject_repr = to->getRepr();
    _delete_connection = to->connectDelete(sigc::bind(sigc::ptr_fun(&lpeobjectreference_delete_self), this));
    _modified_connection = to->connectModified(sigc::bind<2>(sigc::ptr_fun(&lpeobjectreference_source_modified), this));
}

void
LPEObjectReference::quit_listening(void)
{
    _modified_connection.disconnect();
    _delete_connection.disconnect();
    lpeobject_repr = nullptr;
    lpeobject = nullptr;
}

static void
lpeobjectreference_href_changed(SPObject */*old_ref*/, SPObject */*ref*/, LPEObjectReference *lpeobjref)
{
    lpeobjref->quit_listening();
    LivePathEffectObject *refobj = LIVEPATHEFFECT( lpeobjref->getObject() );
    if ( refobj ) {
        lpeobjref->start_listening(refobj);
    }

    lpeobjref->owner->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
}

static void
lpeobjectreference_delete_self(SPObject */*deleted*/, LPEObjectReference *lpeobjref)
{
    lpeobjref->quit_listening();
    lpeobjref->unlink();
    if (lpeobjref->user_unlink) {
        lpeobjref->user_unlink(lpeobjref, lpeobjref->owner);
    }
}

static void
lpeobjectreference_source_modified(SPObject */*iSource*/, guint /*flags*/, LPEObjectReference *lpeobjref)
{
//    We dont need to request update when LPE XML is updated
//    Retain it temporary, drop if no regression
//    SPObject *owner_obj = lpeobjref->owner;
//    if (owner_obj) {
//        lpeobjref->owner->requestDisplayUpdate(SP_OBJECT_MODIFIED_FLAG);
//    }
}

} //namespace LivePathEffect

} // namespace inkscape

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

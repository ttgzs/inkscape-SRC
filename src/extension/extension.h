#ifndef __INK_EXTENSION_H__
#define __INK_EXTENSION_H__

/** \file
 * Frontend to certain, possibly pluggable, actions.
 */

/*
 * Authors:
 *   Ted Gould <ted@gould.cx>
 *
 * Copyright (C) 2002-2004 Authors
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include <vector>
#include "xml/repr.h"
#include <extension/extension-forward.h>

/** The key that is used to identify that the I/O should be autodetected */
#define SP_MODULE_KEY_AUTODETECT "autodetect"
/** This is the key for the SVG input module */
#define SP_MODULE_KEY_INPUT_SVG "org.inkscape.input.svg"
/** Specifies the input module that should be used if none are selected */
#define SP_MODULE_KEY_INPUT_DEFAULT SP_MODULE_KEY_AUTODETECT
/** The key for outputing standard W3C SVG */
#define SP_MODULE_KEY_OUTPUT_SVG "org.inkscape.output.svg.plain"
/** This is an output file that has SVG data with the Sodipodi namespace extensions */
#define SP_MODULE_KEY_OUTPUT_SVG_INKSCAPE "org.inkscape.output.svg.inkscape"
/** Which output module should be used? */
#define SP_MODULE_KEY_OUTPUT_DEFAULT SP_MODULE_KEY_AUTODETECT

/** Defines the key for Postscript printing */
#define SP_MODULE_KEY_PRINT_PS    "org.inkscape.print.ps"
/** Defines the key for printing with GNOME Print */
#define SP_MODULE_KEY_PRINT_GNOME "org.inkscape.print.gnome"
/** Defines the key for printing under Win32 */
#define SP_MODULE_KEY_PRINT_WIN32 "org.inkscape.print.win32"
#ifdef WIN32
/** Defines the default printing to use */
#define SP_MODULE_KEY_PRINT_DEFAULT  SP_MODULE_KEY_PRINT_WIN32
#else
#ifdef WITH_GNOME_PRINT
/** Defines the default printing to use */
#define SP_MODULE_KEY_PRINT_DEFAULT  SP_MODULE_KEY_PRINT_GNOME
#else
/** Defines the default printing to use */
#define SP_MODULE_KEY_PRINT_DEFAULT  SP_MODULE_KEY_PRINT_PS
#endif
#endif

/** Mime type for SVG */
#define MIME_SVG "image/svg+xml"

namespace Inkscape {
namespace Extension {

/** The object that is the basis for the Extension system.  This object
    contains all of the information that all Extension have.  The
    individual items are detailed within. This is the interface that
    those who want to _use_ the extensions system should use.  This
    is most likely to be those who are inside the Inkscape program. */
class Extension {
public:
    /** An enumeration to identify if the Extension has been loaded or not. */
    typedef enum {
        STATE_LOADED,      /**< The extension has been loaded successfully */
        STATE_UNLOADED,    /**< The extension has not been loaded */
        STATE_DEACTIVATED, /**< The extension is missing something which makes it unusable */
    } state_t;

private:
    gchar     *id;                        /**< The unique identifier for the Extension */
    gchar     *name;                      /**< A user friendly name for the Extension */
    state_t    _state;                    /**< Which state the Extension is currently in */
    std::vector<Dependency *>  _deps;     /**< Dependencies for this extension */

protected:
    SPRepr *repr;                         /**< The XML description of the Extension */
    Implementation::Implementation * imp; /**< An object that holds all the functions for making this work */
    ExpirationTimer * timer;              /**< Timeout to unload after a given time */

public:
                  Extension    (SPRepr * in_repr,
                                Implementation::Implementation * in_imp);
    virtual      ~Extension    (void);

    void          set_state    (state_t in_state);
    state_t       get_state    (void);
    bool          loaded       (void);
    virtual bool  check        (void);
    SPRepr *      get_repr     (void);
    gchar *       get_id       (void);
    gchar *       get_name     (void);
    void          deactivate   (void);
    bool          deactivated  (void);


/* Parameter Stuff */
    /* This is what allows modules to save values that are persistent
       through a reset and while running.  These values should be created
       when the extension is initialized, but will pull from previously
       set values if they are available.
     */
private:
    GSList * parameters; /**< A table to store the parameters for this extension.
                              This only gets created if there are parameters in this
                              extension */
    /** This is an enum to define the parameter type */
    enum param_type_t {
        PARAM_BOOL,   /**< Boolean parameter */
        PARAM_INT,    /**< Integer parameter */
        PARAM_FLOAT,  /**< Float parameter */
        PARAM_STRING, /**< String parameter */
        PARAM_CNT     /**< How many types are there? */
    };
    /** A union to select between the types of parameters.  They will
        probbably all fit within a pointer of various systems, but making
        them a union ensures this */
    union param_switch_t {
        bool t_bool;      /**< To get a boolean use this */
        int  t_int;       /**< If you want an integer this is your variable */
        float t_float;    /**< This guy stores the floating point numbers */
        gchar * t_string; /**< Strings are here */
    };
    /** The class that actually stores the value and type of the
        variable.  It should really take up a very small space.  It's
        important to note that the name is stored as the key to the
        hash table. */
    class param_t {
    public:
        gchar * name;        /**< The name of this parameter */
        param_type_t type;   /**< What type of variable */
        param_switch_t val;  /**< Here is the actual value */
    };
public:
    class param_wrong_type {}; /**< An error class for when a parameter is
                                    called on a type it is not */
    class param_not_exist {};  /**< An error class for when a parameter is
                                    looked for that just simply doesn't exist */
    class no_overwrite {};     /**< An error class for when a filename
                                    already exists, but the user doesn't
                                    want to overwrite it */
private:
    void             make_param       (SPRepr * paramrepr);
    inline param_t * param_shared     (const gchar * name,
                                       GSList * list);
public:
    bool             get_param_bool   (const gchar * name,
                                       const SPReprDoc *   doc = NULL);
    int              get_param_int    (const gchar * name,
                                       const SPReprDoc *   doc = NULL);
    float            get_param_float  (const gchar * name,
                                       const SPReprDoc *   doc = NULL);
    const gchar *    get_param_string (const gchar * name,
                                       const SPReprDoc *   doc = NULL);
    bool             set_param_bool   (const gchar * name,
                                       bool          value,
                                       SPReprDoc *   doc = NULL);
    int              set_param_int    (const gchar * name,
                                       int           value,
                                       SPReprDoc *   doc = NULL);
    float            set_param_float  (const gchar * name,
                                       float         value,
                                       SPReprDoc *   doc = NULL);
    const gchar *    set_param_string (const gchar * name,
                                       const gchar * value,
                                       SPReprDoc *   doc = NULL);
};



/*

This is a prototype for how collections should work.  Whoever gets
around to implementing this gets to decide what a 'folder' and an
'item' really is.  That is the joy of implementing it, eh?

class Collection : public Extension {

public:
    folder  get_root (void);
    int     get_count (folder);
    thumbnail get_thumbnail(item);
    item[]  get_items(folder);
    folder[]  get_folders(folder);
    metadata get_metadata(item);
    image   get_image(item);

};
*/

}  /* namespace Extension */
}  /* namespace Inkscape */

#endif /* __INK_EXTENSION_H__ */

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

/**
 * @file
 * SVG stylesheets implementation.
 */
/* Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Peter Moulder <pmoulder@mail.csse.monash.edu.au>
 *   bulia byak <buliabyak@users.sf.net>
 *   Abhishek Sharma
 *   Tavmjong Bah <tavmjong@free.fr>
 *   Kris De Gussem <Kris.DeGussem@gmail.com>
 *
 * Copyright (C) 2001-2002 Lauris Kaplinski
 * Copyright (C) 2001 Ximian, Inc.
 * Copyright (C) 2005 Monash University
 * Copyright (C) 2012 Kris De Gussem
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cstring>
#include <string>
#include <algorithm>

#include "libcroco/cr-sel-eng.h"
#include "xml/croco-node-iface.h"

#include "svg/svg.h"
#include "svg/svg-color.h"
#include "svg/svg-icc-color.h"

#include "display/canvas-bpath.h"
#include "attributes.h"
#include "document.h"
#include "extract-uri.h"
#include "uri-references.h"
#include "uri.h"
#include "sp-paint-server.h"
#include "streq.h"
#include "strneq.h"
#include "style.h"
#include "svg/css-ostringstream.h"
#include "xml/repr.h"
#include "xml/simple-document.h"
#include "util/units.h"
#include "macros.h"
#include "preferences.h"

#include "sp-filter-reference.h"

#include <sigc++/functors/ptr_fun.h>
#include <sigc++/adaptors/bind.h>

#include <2geom/math-utils.h>

using Inkscape::CSSOStringStream;
using std::vector;

#define BMAX 8192

#define SP_CSS_FONT_SIZE_DEFAULT 12.0;

struct SPStyleEnum;

int SPStyle::count = 0;

/*#########################
## FORWARD DECLARATIONS
#########################*/
void sp_style_filter_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style);
void sp_style_fill_paint_server_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style);
void sp_style_stroke_paint_server_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style);

static void sp_style_object_release(SPObject *object, SPStyle *style);
static CRSelEng *sp_repr_sel_eng();

SPStyle::SPStyle(SPDocument *document_in) :

    // Unimplemented SVG 1.1: alignment-baseline, clip, clip-path, color-profile, cursor,
    // dominant-baseline, flood-color, flood-opacity, font-size-adjust,
    // glyph-orientation-horizontal, glyph-orientation-vertical, kerning, lighting-color,
    // pointer-events, stop-color, stop-opacity, unicode-bidi

    // For enums:   property( name, enumeration, default value , inherits = true );
    // For scale24: property( name, default value = 0, inherits = true );

    // 'font-size' and 'font-family' must come first as other properties depend on them
    // for calculated values (through 'em' and 'ex'). ('ex' is currently not read.)
    // The following properties can depend on 'em' and 'ex':
    //   baseline-shift, kerning, letter-spacing, stroke-dash-offset, stroke-width, word-spacing,
    //   Non-SVG 1.1: text-indent, line-spacing
    // Question: Is SPStyle responsible for keeping calculated values up-to-data? If so,
    // we may need an update method.

    // Hidden in SPIFontStyle:
    //   font
    //   font-family
    //   font-specification
    font_size(),
    font_style(       "font-style",      enum_font_style,      SP_CSS_FONT_STYLE_NORMAL   ),
    font_variant(     "font-variant",    enum_font_variant,    SP_CSS_FONT_VARIANT_NORMAL ),
    font_weight(      "font-weight",     enum_font_weight,     SP_CSS_FONT_WEIGHT_NORMAL, SP_CSS_FONT_WEIGHT_400  ),
    font_stretch(     "font-stretch",    enum_font_stretch,    SP_CSS_FONT_STRETCH_NORMAL ),

    text_indent(      "text-indent",                     0.0 ),  // SPILength
    text_align(       "text-align",      enum_text_align,      SP_CSS_TEXT_ALIGN_START    ),
    text_decoration(),
    text_decoration_line(),
    text_decoration_style(),
    text_decoration_color( "text-decoration-color" ), // SPIPaint

    line_height(      "line-height",                     1.0 ),  // SPILengthOrNormal
    letter_spacing(   "letter-spacing",                  0.0 ),  // SPILengthOrNormal
    word_spacing(     "word-spacing",                    0.0 ),  // SPILengthOrNormal
    text_transform(   "text-transform",  enum_text_transform,  SP_CSS_TEXT_TRANSFORM_NONE ),

    direction(        "direction",       enum_direction,       SP_CSS_DIRECTION_LTR       ),
    block_progression("block-progression", enum_block_progression, SP_CSS_BLOCK_PROGRESSION_TB),
    writing_mode(     "writing-mode",    enum_writing_mode,    SP_CSS_WRITING_MODE_LR_TB  ),
    baseline_shift(),
    text_anchor(      "text-anchor",     enum_text_anchor,     SP_CSS_TEXT_ANCHOR_START   ),


    clip_rule(        "clip-rule",       enum_clip_rule,       SP_WIND_RULE_NONZERO       ),
    display(          "display",         enum_display,         SP_CSS_DISPLAY_INLINE,   false ),
    overflow(         "overflow",        enum_overflow,        SP_CSS_OVERFLOW_VISIBLE, false ),
    visibility(       "visibility",      enum_visibility,      SP_CSS_VISIBILITY_VISIBLE  ),
    opacity(          "opacity",                               SP_SCALE24_MAX,          false ),

    isolation(        "isolation",       enum_isolation,       SP_CSS_ISOLATION_AUTO      ),
    blend_mode(       "blend_mode",      enum_blend_mode,      SP_CSS_BLEND_NORMAL        ),

    color(            "color"                                ),  // SPIPaint
    color_interpolation(        "color-interpolation",         enum_color_interpolation, SP_CSS_COLOR_INTERPOLATION_SRGB),
    color_interpolation_filters("color-interpolation-filters", enum_color_interpolation, SP_CSS_COLOR_INTERPOLATION_LINEARRGB),

    fill(             "fill"                                 ),  // SPIPaint
    fill_opacity(     "fill-opacity",                          SP_SCALE24_MAX             ),
    fill_rule(        "fill-rule",       enum_fill_rule,       SP_WIND_RULE_NONZERO       ),

    stroke(           "stroke"                               ),  // SPIPaint
    stroke_width(     "stroke-width",      1.0               ),  // SPILength
    stroke_linecap(   "stroke-linecap",  enum_stroke_linecap,  SP_STROKE_LINECAP_BUTT     ),
    stroke_linejoin(  "stroke-linejoin", enum_stroke_linejoin, SP_STROKE_LINEJOIN_MITER   ),
    stroke_miterlimit("stroke-miterlimit",   4               ),  // SPIFloat (only use of float!)
    stroke_dasharray(),  // SPIDashArray
    stroke_dashoffset("stroke-dashoffset", 0.0               ),  // SPILength for now

    stroke_opacity(   "stroke-opacity",    SP_SCALE24_MAX),

//  marker({ "marker", "marker-start", "marker-mid", "marker-end" }), C++11 
    paint_order(), // SPIPaintOrder

    filter(),
    filter_blend_mode("filter-blend-mode", enum_blend_mode,    SP_CSS_BLEND_NORMAL),
    filter_gaussianBlur_deviation( "filter-gaussianBlur-deviation", 0.0 ), // SPILength

    color_rendering(  "color-rendering", enum_color_rendering, SP_CSS_COLOR_RENDERING_AUTO),
    image_rendering(  "image-rendering", enum_image_rendering, SP_CSS_IMAGE_RENDERING_AUTO),
    shape_rendering(  "shape-rendering", enum_shape_rendering, SP_CSS_SHAPE_RENDERING_AUTO),
    text_rendering(    "text-rendering", enum_text_rendering,  SP_CSS_TEXT_RENDERING_AUTO ),

    enable_background("enable-background", enum_enable_background, SP_CSS_BACKGROUND_ACCUMULATE, false)
{
    // std::cout << "\nSPStyle::SPStyle( SPDocument ): Entrance: (" << count << ")" << std::endl;
    document = document_in;
    // if( document ) std::cout << "SPStyle::SPStyle: have document" << std::endl;
    // else           std::cout << "SPStyle::SPStyle: don't have document" << std::endl;

    static bool first = true;
    if( first ) {
        std::cout << "Size of SPStyle: " << sizeof(SPStyle) << std::endl;
        std::cout << "        SPIBase: " << sizeof(SPIBase) << std::endl;
        std::cout << "       SPIFloat: " << sizeof(SPIFloat) << std::endl;
        std::cout << "     SPIScale24: " << sizeof(SPIScale24) << std::endl;
        std::cout << "      SPILength: " << sizeof(SPILength) << std::endl;
        std::cout << "  SPILengthOrNormal: " << sizeof(SPILengthOrNormal) << std::endl;
        std::cout << "       SPIPaint: " << sizeof(SPIPaint) << std::endl;
        std::cout << "   Glib::ustring:" << sizeof(Glib::ustring) << std::endl;
        first = false;
    }

    ++count; // Poor man's memory leak detector

    refcount = 1;
    object = NULL;

    cloned = false;

    new (&release_connection) sigc::connection();
    new (&filter_modified_connection) sigc::connection();
    new (&fill_ps_modified_connection) sigc::connection();
    new (&stroke_ps_modified_connection) sigc::connection();


    text = new SPFontStyle();
    text->font                  = SPIString( "font"         );
    text->font_family           = SPIString( "font-family"  );
    text->font_specification    = SPIString( "-inkscape-font-specification" );

    // Properties that depend on 'font-size' for calculating lengths.
    baseline_shift.setStylePointer(    this );
    text_indent.setStylePointer(       this );
    line_height.setStylePointer(       this );
    letter_spacing.setStylePointer(    this );
    word_spacing.setStylePointer(      this );
    stroke_width.setStylePointer(      this );
    stroke_dashoffset.setStylePointer( this );

    // Properties that depend on 'color'
    text_decoration_color.setStylePointer( this );
    fill.setStylePointer(                  this );
    stroke.setStylePointer(                this );
    // color.setStylePointer( this ); // Doen't need reference to self

    // 'text_decoration' shorthand requires access to dependent properties
    text_decoration.setStylePointer( this );

    // SPIPaint, SPIFilter needs access to 'this' (SPStyle)
    // for setting up signals...  'fill', 'stroke' already done
    filter.setStylePointer( this );

    marker[SP_MARKER_LOC]       = SPIString( "marker"       );
    marker[SP_MARKER_LOC_START] = SPIString( "marker-start" );
    marker[SP_MARKER_LOC_MID]   = SPIString( "marker-mid"   );
    marker[SP_MARKER_LOC_END]   = SPIString( "marker-end"   );

    // This might be too resource hungary... but for now it possible to loop over properties

    // Must be before 'fill', 'stroke', 'text-decoration-color', ...
    properties.push_back( &color );

    // Must be before properties that need to know em, ex size (SPILength, SPILenghtOrNormal)
    properties.push_back( &font_size ); 

    properties.push_back( &font_style );
    properties.push_back( &font_variant );
    properties.push_back( &font_weight );
    properties.push_back( &font_stretch );
    properties.push_back( &text_indent );
    properties.push_back( &text_align );

    properties.push_back( &text_decoration );
    properties.push_back( &text_decoration_line );
    properties.push_back( &text_decoration_style );
    properties.push_back( &text_decoration_color );

    properties.push_back( &line_height );
    properties.push_back( &letter_spacing );
    properties.push_back( &word_spacing );
    properties.push_back( &text_transform );

    properties.push_back( &direction );
    properties.push_back( &block_progression );
    properties.push_back( &writing_mode );
    properties.push_back( &baseline_shift );
    properties.push_back( &text_anchor );

    properties.push_back( &clip_rule );
    properties.push_back( &display );
    properties.push_back( &overflow );
    properties.push_back( &visibility );
    properties.push_back( &opacity );

    properties.push_back( &isolation );
    properties.push_back( &blend_mode );

    properties.push_back( &color_interpolation );
    properties.push_back( &color_interpolation_filters );

    properties.push_back( &fill );
    properties.push_back( &fill_opacity );
    properties.push_back( &fill_rule );

    properties.push_back( &stroke );
    properties.push_back( &stroke_width );
    properties.push_back( &stroke_linecap );
    properties.push_back( &stroke_linejoin );
    properties.push_back( &stroke_miterlimit );
    properties.push_back( &stroke_dasharray );
    properties.push_back( &stroke_dashoffset );
    properties.push_back( &stroke_opacity );

    properties.push_back( &marker[SP_MARKER_LOC] );      
    properties.push_back( &marker[SP_MARKER_LOC_START] );
    properties.push_back( &marker[SP_MARKER_LOC_MID] );  
    properties.push_back( &marker[SP_MARKER_LOC_END] );  

    properties.push_back( &paint_order );

    properties.push_back( &filter );
    properties.push_back( &filter_blend_mode );
    properties.push_back( &filter_gaussianBlur_deviation );

    properties.push_back( &color_rendering );
    properties.push_back( &image_rendering );
    properties.push_back( &shape_rendering );
    properties.push_back( &text_rendering );

    properties.push_back( &enable_background );

    // properties.push_back( &text->font );  We could have a preference to select 'font' shorthand
    properties.push_back( &text->font_family ); // Must be first as other values depend on it ('ex')
    properties.push_back( &text->font_specification );
}

SPStyle::SPStyle( SPObject *object_in) {

    std::cout << "\nSPStyle::SPstyle( SPObject ): Entrance"
              << (object_in?(object_in->getId()?object_in->getId():"id null"):"object null") << std::endl;
    // g_return_val_if_fail(object_in != NULL, NULL);
    // g_return_val_if_fail(SP_IS_OBJECT(object_in), NULL);
    if( !object_in ) return;

    SPStyle( object_in->document );

    object = object_in;

    release_connection = object->connectRelease(sigc::bind<1>(sigc::ptr_fun(&sp_style_object_release), this));

    if (object->cloned) {
        cloned = true;
    }
}

SPStyle::~SPStyle() {

    --count; // Poor man's memory leak detector.

    // Remove connections
    release_connection.disconnect();
    release_connection.~connection();

    // The following shoud be moved into SPIPaint and SPIFilter
    if (fill.value.href) {
        fill_ps_modified_connection.disconnect();
    }

    if (stroke.value.href) {
        stroke_ps_modified_connection.disconnect();
    }

    if (filter.href) {
        filter_modified_connection.disconnect();
    }

    filter_modified_connection.~connection();
    fill_ps_modified_connection.~connection();
    stroke_ps_modified_connection.~connection();

    properties.clear();
    delete text;

    // std::cout << "SPStyle::~SPstyle(): Exit\n" << std::endl;
}

// Used in SPStyle::clear()
void clear_property( SPIBase* p ) {
    p->clear();
}


// Matches void sp_style_clear();
void
SPStyle::clear() {

    for_each( properties.begin(), properties.end(), clear_property );

    // Release connection to object, created in sp_style_new_from_object()
    release_connection.disconnect();

    // href->detach() called in fill->clear()...
    fill_ps_modified_connection.disconnect();
    if (fill.value.href) {
        delete fill.value.href;
        fill.value.href = NULL;
    }
    stroke_ps_modified_connection.disconnect();
    if (stroke.value.href) {
        delete stroke.value.href;
        stroke.value.href = NULL;
    }
    filter_modified_connection.disconnect();
    if (filter.href) {
        delete filter.href;
        filter.href = NULL;
    }

    if (document) {
        filter.href = new SPFilterReference(document);
        filter.href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_filter_ref_changed), this));

        fill.value.href = new SPPaintServerReference(document);
        fill.value.href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_fill_paint_server_ref_changed), this));

        stroke.value.href = new SPPaintServerReference(document);
        stroke.value.href->changedSignal().connect(sigc::bind(sigc::ptr_fun(sp_style_stroke_paint_server_ref_changed), this));
    }

    cloned = false;

}

// Matches void sp_style_read(SPStyle *style, SPObject *object, Inkscape::XML::Node *repr)
void
SPStyle::read( SPObject *object, Inkscape::XML::Node *repr ) {

    // std::cout << "SPstyle::read( SPObject, Inkscape::XML::Node ): Entrance: "
    //           << (object?(object->getId()?object->getId():"id null"):"object null") << " "
    //           << (repr?(repr->name()?repr->name():"no name"):"repr null")
    //           << std::endl;
    g_assert(repr != NULL);
    g_assert(!object || (object->getRepr() == repr));

    // // Uncomment to verify that we don't need to call clear.
    // std::cout << " Creating temp style for testing" << std::endl;
    // SPStyle *temp = new SPStyle();
    // if( !(*temp == *this ) ) std::cout << "SPStyle::read: Need to clear" << std::endl;
    // delete temp;

    clear(); // FIXME, If this isn't here, gradient editing stops working. Why?

    if (object && object->cloned) {
        cloned = true;
    }

    /* 1. Style attribute */
    // std::cout << " MERGING STYLE ATTRIBUTE" << std::endl;
    gchar const *val = repr->attribute("style");
    if( val != NULL && *val ) {
        merge_string( val );
    }

    /* 2 Style sheet */
    // std::cout << " MERGING OBJECT STYLESHEET" << std::endl;
    if (object) {
        merge_object_stylesheet( object );
    } else {
        // std::cerr << "SPStyle::read: No object! Can not read style sheet" << std::endl;
    }

    /* 3 Presentation attributes */
    // std::cout << " MERGING PRESENTATION ATTRIBUTES" << std::endl;
    for(std::vector<SPIBase*>::size_type i = 0; i != properties.size(); ++i) {
        properties[i]->read_attribute( repr );
    }

    /* 4 Cascade from parent */
    // std::cout << " CASCADING FROM PARENT" << std::endl;
    if( object ) {
        if( object->parent ) {
            cascade( object->parent->style );
        }
    } else {
        // When does this happen?
        // std::cout << "SPStyle::read(): reading via repr->parent()" << std::endl;
        if( repr->parent() ) {
            SPStyle *parent = new SPStyle();
            parent->read( NULL, repr->parent() );
            cascade( parent );
            delete parent;
        }
    }
}

// Matches void sp_style_read_from_object(SPStyle *style, SPObject *object);
void
SPStyle::read_from_object( SPObject *object ) {

    // std::cout << "SPStyle::read_from_object: "<< (object->getId()?object->getId():"null")<< std::endl;

    g_return_if_fail(object != NULL);
    g_return_if_fail(SP_IS_OBJECT(object));

    Inkscape::XML::Node *repr = object->getRepr();
    g_return_if_fail(repr != NULL);

    read( object, repr );
}

// Matches sp_style_merge_property(SPStyle *style, gint id, gchar const *val)
void
SPStyle::read_if_unset( gint id, gchar const *val ) {

    // std::cout << "SPStyle::read_if_unset: Entrance: " << (val?val:"null") << std::endl;
    // To Do: If it is not too slow, use std::map instead of std::vector inorder to remove switch()
    // (looking up SP_PROP_xxxx already uses a hash).
    g_return_if_fail(val != NULL);

    switch (id) {
        case SP_PROP_INKSCAPE_FONT_SPEC:
            text->font_specification.read_if_unset( val );
            break;
        case SP_PROP_FONT_FAMILY:
            text->font_family.read_if_unset( val );
            break;
        case SP_PROP_FONT_SIZE:
            font_size.read_if_unset( val );
            break;
        case SP_PROP_FONT_SIZE_ADJUST:
            if (strcmp(val, "none") != 0) {
                g_warning("Unimplemented style property id SP_PROP_FONT_SIZE_ADJUST: value: %s", val);
            }
            break;
        case SP_PROP_FONT_STYLE:
            font_style.read_if_unset( val );
            break;
        case SP_PROP_FONT_VARIANT:
            font_variant.read_if_unset( val );
            break;
        case SP_PROP_FONT_WEIGHT:
            font_weight.read_if_unset( val );
            break;
        case SP_PROP_FONT_STRETCH:
            font_stretch.read_if_unset( val );
            break;
        case SP_PROP_FONT:
            // TO DO: Create SPIFont class as wrapper for individual font properties
            // (see text-decoration).
            if (!text->font.set) {
                g_free(text->font.value);
                text->font.value = g_strdup(val);
                text->font.set = TRUE;
                text->font.inherit = (val && !strcmp(val, "inherit"));

                // Break string into white space separated tokens
                std::stringstream os( val );
                Glib::ustring param;

                while (os >> param) {

                    // CSS is case insensitive but we're comparing against lowercase strings
                    Glib::ustring lparam = param.lowercase();

                    if (lparam == "/") {

                        os >> param;
                        lparam = param.lowercase();
                        line_height.read_if_unset( lparam.c_str() );

                    } else {

                        // Skip if "normal" as that is the default (and we don't know which attribute it applies to).
                        if (lparam == "normal") continue;

                        // Check each property in turn

                        // font-style
                        SPIEnum test_style("font-style", enum_font_style);
                        test_style.read( lparam.c_str() );
                        if( test_style.set ) {
                            font_style = test_style;
                            continue;
                        }

                        // font-variant (small-caps)
                        SPIEnum test_variant("font-variant", enum_font_variant);
                        test_variant.read( lparam.c_str() );
                        if( test_variant.set ) {
                            font_variant = test_variant;
                            continue;
                        }

                        // font-weight
                        SPIEnum test_weight("font-weight", enum_font_weight);
                        test_weight.read( lparam.c_str() );
                        if( test_weight.set ) {
                            font_weight = test_weight;
                            continue; // Next parameter
                        }

                        // Font-size
                        SPIFontSize test_size;
                        test_size.read( lparam.c_str() );
                        if( test_size.set ) {
                            font_size = test_size;
                            continue;
                        }

                        // No valid property value found.
                        break;
                    }
                } // params

                // The rest must be font-family...
                std::string val_s = val;
                std::string family = val_s.substr( val_s.find( param ) );

                text->font_family.read_if_unset( family.c_str() );

                // Set all properties to their default values per CSS 2.1 spec if not already set
                font_size.read_if_unset( "medium" );
                font_style.read_if_unset( "normal" );
                font_variant.read_if_unset( "normal" );
                font_weight.read_if_unset( "normal" );
                line_height.read_if_unset( "normal" );
            }

            break;

            /* Text */
        case SP_PROP_TEXT_INDENT:
            text_indent.read_if_unset( val );
            break;
        case SP_PROP_TEXT_ALIGN:
            text_align.read_if_unset( val );
            break;
        case SP_PROP_TEXT_DECORATION:
            text_decoration.read_if_unset( val );
            break;
        case SP_PROP_TEXT_DECORATION_LINE:
            text_decoration_line.read_if_unset( val );
            break;
        case SP_PROP_TEXT_DECORATION_STYLE:
            text_decoration_style.read_if_unset( val );
            break;
        case SP_PROP_TEXT_DECORATION_COLOR:
            text_decoration_color.read_if_unset( val );
            break;
        case SP_PROP_LINE_HEIGHT:
            line_height.read_if_unset( val );
            break;
        case SP_PROP_LETTER_SPACING:
            letter_spacing.read_if_unset( val );
            break;
        case SP_PROP_WORD_SPACING:
            word_spacing.read_if_unset( val );
            break;
        case SP_PROP_TEXT_TRANSFORM:
            text_transform.read_if_unset( val );
            break;
            /* Text (css3) */
        case SP_PROP_DIRECTION:
            direction.read_if_unset( val );
            break;
        case SP_PROP_BLOCK_PROGRESSION:
            block_progression.read_if_unset( val );
            break;
        case SP_PROP_WRITING_MODE:
            writing_mode.read_if_unset( val );
            break;
        case SP_PROP_TEXT_ANCHOR:
            text_anchor.read_if_unset( val );
            break;
        case SP_PROP_BASELINE_SHIFT:
            baseline_shift.read_if_unset( val );
            break;
        case SP_PROP_TEXT_RENDERING:
            text_rendering.read_if_unset( val );
            break;
        case SP_PROP_ALIGNMENT_BASELINE:
            g_warning("Unimplemented style property SP_PROP_ALIGNMENT_BASELINE: value: %s", val);
            break;
        case SP_PROP_DOMINANT_BASELINE:
            g_warning("Unimplemented style property SP_PROP_DOMINANT_BASELINE: value: %s", val);
            break;
        case SP_PROP_GLYPH_ORIENTATION_HORIZONTAL:
            g_warning("Unimplemented style property SP_PROP_ORIENTATION_HORIZONTAL: value: %s", val);
            break;
        case SP_PROP_GLYPH_ORIENTATION_VERTICAL:
            g_warning("Unimplemented style property SP_PROP_ORIENTATION_VERTICAL: value: %s", val);
            break;
        case SP_PROP_KERNING:
            g_warning("Unimplemented style property SP_PROP_KERNING: value: %s", val);
            break;
            /* Misc */
        case SP_PROP_CLIP:
            g_warning("Unimplemented style property SP_PROP_CLIP: value: %s", val);
            break;
        case SP_PROP_COLOR:
            color.read_if_unset( val );
            break;
        case SP_PROP_CURSOR:
            g_warning("Unimplemented style property SP_PROP_CURSOR: value: %s", val);
            break;
        case SP_PROP_DISPLAY:
            display.read_if_unset( val );
            break;
        case SP_PROP_OVERFLOW:
            overflow.read_if_unset( val );
            break;
        case SP_PROP_VISIBILITY:
            visibility.read_if_unset( val );
            break;
        case SP_PROP_ISOLATION:
            isolation.read_if_unset( val );
            break;
        case SP_PROP_BLEND_MODE:
            blend_mode.read_if_unset( val );
            break;

            /* SVG */
            /* Clip/Mask */
        case SP_PROP_CLIP_PATH:
            /** \todo
             * This is a workaround. Inkscape only supports 'clip-path' as SVG attribute, not as
             * style property. By having both CSS and SVG attribute set, editing of clip-path
             * will fail, since CSS always overwrites SVG attributes.
             * Fixes Bug #324849
             */
            g_warning("attribute 'clip-path' given as CSS");

            //XML Tree being directly used here.
            this->object->getRepr()->setAttribute("clip-path", val);
            break;
        case SP_PROP_CLIP_RULE:
            clip_rule.read_if_unset( val );
            break;
        case SP_PROP_MASK:
            /** \todo
             * See comment for SP_PROP_CLIP_PATH
             */
            g_warning("attribute 'mask' given as CSS");
            
            //XML Tree being directly used here.
            this->object->getRepr()->setAttribute("mask", val);
            break;
        case SP_PROP_OPACITY:
            opacity.read_if_unset( val );
            break;
        case SP_PROP_ENABLE_BACKGROUND:
            enable_background.read_if_unset( val );
            break;
            /* Filter */
        case SP_PROP_FILTER:
            if( !filter.inherit ) filter.read_if_unset( val );
            break;
        case SP_PROP_FLOOD_COLOR:
            g_warning("Unimplemented style property SP_PROP_FLOOD_COLOR: value: %s", val);
            break;
        case SP_PROP_FLOOD_OPACITY:
            g_warning("Unimplemented style property SP_PROP_FLOOD_OPACITY: value: %s", val);
            break;
        case SP_PROP_LIGHTING_COLOR:
            g_warning("Unimplemented style property SP_PROP_LIGHTING_COLOR: value: %s", val);
            break;
            /* Gradient */
        case SP_PROP_STOP_COLOR:
            g_warning("Unimplemented style property SP_PROP_STOP_COLOR: value: %s", val);
            break;
        case SP_PROP_STOP_OPACITY:
            g_warning("Unimplemented style property SP_PROP_STOP_OPACITY: value: %s", val);
            break;
            /* Interactivity */
        case SP_PROP_POINTER_EVENTS:
            g_warning("Unimplemented style property SP_PROP_POINTER_EVENTS: value: %s", val);
            break;
            /* Paint */
        case SP_PROP_COLOR_INTERPOLATION:
            // We read it but issue warning
            color_interpolation.read_if_unset( val );
            if( color_interpolation.value != SP_CSS_COLOR_INTERPOLATION_SRGB ) {
                g_warning("Inkscape currently only supports color-interpolation = sRGB");
            }
            break;
        case SP_PROP_COLOR_INTERPOLATION_FILTERS:
            color_interpolation_filters.read_if_unset( val );
            break;
        case SP_PROP_COLOR_PROFILE:
            g_warning("Unimplemented style property SP_PROP_COLOR_PROFILE: value: %s", val);
            break;
        case SP_PROP_COLOR_RENDERING:
            color_rendering.read_if_unset( val );
            break;
        case SP_PROP_FILL:
            fill.read_if_unset( val );
            break;
        case SP_PROP_FILL_OPACITY:
            fill_opacity.read_if_unset( val );
            break;
        case SP_PROP_FILL_RULE:
            fill_rule.read_if_unset( val );
            break;
        case SP_PROP_IMAGE_RENDERING:
            image_rendering.read_if_unset( val );
            break;
        case SP_PROP_MARKER:
            /* TODO:  Call sp_uri_reference_resolve(SPDocument *document, guchar const *uri) */ 
            marker[SP_MARKER_LOC].read_if_unset( val );
            break;
        case SP_PROP_MARKER_START:
            /* TODO:  Call sp_uri_reference_resolve(SPDocument *document, guchar const *uri) */
            marker[SP_MARKER_LOC_START].read_if_unset( val );
            break;
        case SP_PROP_MARKER_MID:
            /* TODO:  Call sp_uri_reference_resolve(SPDocument *document, guchar const *uri) */
            marker[SP_MARKER_LOC_MID].read_if_unset( val );
            break;
        case SP_PROP_MARKER_END:
            /* TODO:  Call sp_uri_reference_resolve(SPDocument *document, guchar const *uri) */
            marker[SP_MARKER_LOC_END].read_if_unset( val );
            break;
        case SP_PROP_SHAPE_RENDERING:
            shape_rendering.read_if_unset( val );
            break;
        case SP_PROP_STROKE:
            stroke.read_if_unset( val );
            break;
        case SP_PROP_STROKE_WIDTH:
            stroke_width.read_if_unset( val );
            break;
        case SP_PROP_STROKE_DASHARRAY:
            stroke_dasharray.read_if_unset( val );
            break;
        case SP_PROP_STROKE_DASHOFFSET:
            stroke_dashoffset.read_if_unset( val );
            break;
        case SP_PROP_STROKE_LINECAP:
            stroke_linecap.read_if_unset( val );
            break;
        case SP_PROP_STROKE_LINEJOIN:
            stroke_linejoin.read_if_unset( val );
            break;
        case SP_PROP_STROKE_MITERLIMIT:
            stroke_miterlimit.read_if_unset( val );
            break;
        case SP_PROP_STROKE_OPACITY:
            stroke_opacity.read_if_unset( val );
            break;
        case SP_PROP_PAINT_ORDER:
            paint_order.read_if_unset( val );
            break;
        default:
            g_warning("SPIStyle::read_if_unset(): Invalid style property id: %d value: %s", id, val);
            break;
    }
}

Glib::ustring
SPStyle::write( guint const flags, SPStyle const *const base ) const {

    // std::cout << "SPStyle::write" << std::endl;

    Glib::ustring style_string;
    for(std::vector<SPIBase*>::size_type i = 0; i != properties.size(); ++i) {
        if( base != NULL ) {
            style_string += properties[i]->write( flags, base->properties[i] );
        } else {
            style_string += properties[i]->write( flags, NULL );
        }
    }

    // Remove trailing ';'
    if( style_string.size() > 0 ) {
        style_string.erase( style_string.size() - 1 );
    }
    return style_string;
}

// Corresponds to sp_style_merge_from_parent()
void
SPStyle::cascade( SPStyle const *const parent ) {
    // std::cout << "SPStyle::cascade" << std::endl;
    for(std::vector<SPIBase*>::size_type i = 0; i != properties.size(); ++i) {
        properties[i]->cascade( parent->properties[i] );
    }
}

// Corresponds to sp_style_merge_from_dying_parent()
void
SPStyle::merge( SPStyle const *const parent ) {
    // std::cout << "SPStyle::merge" << std::endl;
    for(std::vector<SPIBase*>::size_type i = 0; i != properties.size(); ++i) {
        properties[i]->merge( parent->properties[i] );
    }
}

// Mostly for unit testing
bool
SPStyle::operator==(const SPStyle& rhs) {

    // Uncomment for testing
    // for(std::vector<SPIBase*>::size_type i = 0; i != properties.size(); ++i) {
    //     if( *properties[i] != *rhs.properties[i])
    //     std::cout << properties[i]->name << ": "
    //               << properties[i]->write(SP_STYLE_FLAG_ALWAYS,NULL) << " " 
    //               << rhs.properties[i]->write(SP_STYLE_FLAG_ALWAYS,NULL)
    //               << (*properties[i]  == *rhs.properties[i]) << std::endl;
    // }

    for(std::vector<SPIBase*>::size_type i = 0; i != properties.size(); ++i) {
        if( *properties[i] != *rhs.properties[i]) return false;
    }
    return true;
}

void
SPStyle::merge_string( gchar const *const p ) {

    // std::cout << "SPStyle::merge_string: " << (p?p:"null") << std::endl;
    CRDeclaration *const decl_list
        = cr_declaration_parse_list_from_buf(reinterpret_cast<guchar const *>(p), CR_UTF_8);
    if (decl_list) {
        merge_decl_list( decl_list );
        cr_declaration_destroy(decl_list);
    }
}

void
SPStyle::merge_decl_list( CRDeclaration const *const decl_list ) {

    // std::cout << "SPStyle::merge_decl_list" << std::endl;

    // In reverse order, as later declarations to take precedence over earlier ones.
    // (Properties are only set if not previously set. See:
    // Ref: http://www.w3.org/TR/REC-CSS2/cascade.html#cascading-order point 4.)
    if (decl_list->next) {
        merge_decl_list( decl_list->next );
    }
    merge_decl( decl_list );
}

void
SPStyle::merge_decl(  CRDeclaration const *const decl ) {

    // std::cout << "SPStyle::merge_decl" << std::endl;

    unsigned const prop_idx = sp_attribute_lookup(decl->property->stryng->str);
    if (prop_idx != SP_ATTR_INVALID) {
        /** \todo
         * effic: Test whether the property is already set before trying to
         * convert to string. Alternatively, set from CRTerm directly rather
         * than converting to string.
         */
        guchar *const str_value_unsigned = cr_term_to_string(decl->value);
        gchar *const str_value = reinterpret_cast<gchar *>(str_value_unsigned);
        read_if_unset( prop_idx, str_value );
        g_free(str_value);
    }
}

void
SPStyle::merge_props( CRPropList *const props ) {

    // std::cout << "SPStyle::merge_props" << std::endl;

    // In reverse order, as later declarations to take precedence over earlier ones.
    if (props) {
        merge_props( cr_prop_list_get_next( props ) );
        CRDeclaration *decl = NULL;
        cr_prop_list_get_decl(props, &decl);
        merge_decl( decl );
    }
}

void
SPStyle::merge_object_stylesheet( SPObject const *const object ) {

    // std::cout << "SPStyle::merge_object_stylesheet: " << (object->getId()?object->getId():"null") << std::endl;

    static CRSelEng *sel_eng = NULL;
    if (!sel_eng) {
        sel_eng = sp_repr_sel_eng();
    }

    CRPropList *props = NULL;

    //XML Tree being directly used here while it shouldn't be.
    CRStatus status = cr_sel_eng_get_matched_properties_from_cascade(sel_eng,
                                                                     object->document->style_cascade,
                                                                     object->getRepr(),
                                                                     &props);
    g_return_if_fail(status == CR_OK);
    /// \todo Check what errors can occur, and handle them properly.
    if (props) {
        merge_props(props);
        cr_prop_list_destroy(props);
    }
}

// Internal
/**
 * Release callback.
 */
static void
sp_style_object_release(SPObject *object, SPStyle *style)
{
    (void)object; // TODO
    style->object = NULL;
}

// Internal
/**
 * Emit style modified signal on style's object if the filter changed.
 */
static void
sp_style_filter_ref_modified(SPObject *obj, guint flags, SPStyle *style)
{
    (void)flags; // TODO
    SPFilter *filter=static_cast<SPFilter *>(obj);
    if (style->getFilter() == filter)
    {
        if (style->object) {
            style->object->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        }
    }
}

// Internal
/**
 * Gets called when the filter is (re)attached to the style
 */
void
sp_style_filter_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style)
{
    if (old_ref) {
        style->filter_modified_connection.disconnect();
    }
    if ( SP_IS_FILTER(ref))
    {
        style->filter_modified_connection =
           ref->connectModified(sigc::bind(sigc::ptr_fun(&sp_style_filter_ref_modified), style));
    }

    sp_style_filter_ref_modified(ref, 0, style);
}

/**
 * Emit style modified signal on style's object if server is style's fill
 * or stroke paint server.
 */
static void
sp_style_paint_server_ref_modified(SPObject *obj, guint flags, SPStyle *style)
{
    (void)flags; // TODO
    SPPaintServer *server = static_cast<SPPaintServer *>(obj);

    if ((style->fill.isPaintserver())
        && style->getFillPaintServer() == server)
    {
        if (style->object) {
            /** \todo
             * fixme: I do not know, whether it is optimal - we are
             * forcing reread of everything (Lauris)
             */
            /** \todo
             * fixme: We have to use object_modified flag, because parent
             * flag is only available downstreams.
             */
            style->object->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        }
    } else if ((style->stroke.isPaintserver())
        && style->getStrokePaintServer() == server)
    {
        if (style->object) {
            /// \todo fixme:
            style->object->requestModified(SP_OBJECT_MODIFIED_FLAG | SP_OBJECT_STYLE_MODIFIED_FLAG);
        }
    } else if (server) {
        g_assert_not_reached();
    }
}

/**
 * Gets called when the paintserver is (re)attached to the style
 */
void
sp_style_fill_paint_server_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style)
{
    if (old_ref) {
        style->fill_ps_modified_connection.disconnect();
    }
    if (SP_IS_PAINT_SERVER(ref)) {
        style->fill_ps_modified_connection =
           ref->connectModified(sigc::bind(sigc::ptr_fun(&sp_style_paint_server_ref_modified), style));
    }

    sp_style_paint_server_ref_modified(ref, 0, style);
}

/**
 * Gets called when the paintserver is (re)attached to the style
 */
void
sp_style_stroke_paint_server_ref_changed(SPObject *old_ref, SPObject *ref, SPStyle *style)
{
    if (old_ref) {
        style->stroke_ps_modified_connection.disconnect();
    }
    if (SP_IS_PAINT_SERVER(ref)) {
        style->stroke_ps_modified_connection =
          ref->connectModified(sigc::bind(sigc::ptr_fun(&sp_style_paint_server_ref_modified), style));
    }

    sp_style_paint_server_ref_modified(ref, 0, style);
}

// Called in: desktop-style.cpp, gradient-chemistry.cpp, sp-object.cpp, sp-stop.cpp, style.cpp
// text-editing.cpp, libnrtype/font-lister.cpp, widgets/dash-selector.cpp, widgets/fill-style.cpp,
// widgets/stroke-style.cpp, widgets/text-toolbar.cpp, ui/dialog/glyphs.cpp, ui/dialog/swatches.cpp,
// ui/dialog/swatches.cpp, ui/dialog/text-edit.cpp. ui/tools/freehand-base.cpp,
// ui/widget/object-composite-settings.cpp, ui/widget/selected-style.cpp, ui/widget/style-swatch.cpp

/**
 * Returns a new SPStyle object with default settings.
 */
SPStyle *
sp_style_new(SPDocument *document)
{
    SPStyle *const style = new SPStyle( document );
    return style;
}

// Called in: sp-object.cpp
/**
 * Creates a new SPStyle object, and attaches it to the specified SPObject.
 */
SPStyle *
sp_style_new_from_object(SPObject *object)
{
    g_return_val_if_fail(object != NULL, NULL);
    g_return_val_if_fail(SP_IS_OBJECT(object), NULL);

    SPStyle *style = sp_style_new( object->document );
    style->object = object;
    style->release_connection = object->connectRelease(sigc::bind<1>(sigc::ptr_fun(&sp_style_object_release), style));

    if (object->cloned) {
        style->cloned = true;
    }

    return style;
}

// Called in display/drawing-item.cpp, display/nr-filter-primitive.cpp, libnrtype/Layout-TNG-Input.cpp
/**
 * Increase refcount of style.
 */
SPStyle *
sp_style_ref(SPStyle *style)
{
    g_return_val_if_fail(style != NULL, NULL);
    g_return_val_if_fail(style->refcount > 0, NULL);

    style->refcount += 1;

    return style;
}

// Called in style.cpp, desktop-style.cpp, sp-object.cpp, sp-stop.cpp, text-editing.cpp
// display/drawing-group.cpp, ...
/**
 * Decrease refcount of style with possible destruction.
 */
SPStyle *
sp_style_unref(SPStyle *style)
{
    g_return_val_if_fail(style != NULL, NULL);
    g_return_val_if_fail(style->refcount > 0, NULL);

    style->refcount -= 1;

    if (style->refcount < 1) {
        delete style;
        return NULL;
    }
    return style;
}



// Called in: sp-clippath.cpp, sp-item.cpp (suspicious), sp-object.cpp, sp-style-elem.cpp
/**
 * Read style properties from object's repr.
 *
 * 1. Reset existing object style
 * 2. Load current effective object style
 * 3. Load i attributes from immediate parent (which has to be up-to-date)
 */
void
sp_style_read_from_object(SPStyle *style, SPObject *object)
{
    // std::cout << "sp_style_read_from_object: " << (object->getId()?object->getId():"null") << std::endl;
    g_return_if_fail(style != NULL);
    g_return_if_fail(object != NULL);
    g_return_if_fail(SP_IS_OBJECT(object));

    Inkscape::XML::Node *repr = object->getRepr();
    g_return_if_fail(repr != NULL);

    style->read( object, repr );
}

// Called in: libnrtype/font-lister.cpp, widgets/dash-selector.cpp, widgets/text-toolbar.cpp,
// ui/dialog/text-edit.cpp
// Why is this called when draging a gradient handle?
/**
 * Read style properties from preferences.
 * @param style The style to write to
 * @param path Preferences directory from which the style should be read
 */
void
sp_style_read_from_prefs(SPStyle *style, Glib::ustring const &path)
{
    g_return_if_fail(style != NULL);
    g_return_if_fail(path != "");

    Inkscape::Preferences *prefs = Inkscape::Preferences::get();

    // not optimal: we reconstruct the node based on the prefs, then pass it to
    // sp_style_read for actual processing.
    Inkscape::XML::SimpleDocument *tempdoc = new Inkscape::XML::SimpleDocument;
    Inkscape::XML::Node *tempnode = tempdoc->createElement("prefs");

    std::vector<Inkscape::Preferences::Entry> attrs = prefs->getAllEntries(path);
    for (std::vector<Inkscape::Preferences::Entry>::iterator i = attrs.begin(); i != attrs.end(); ++i) {
        tempnode->setAttribute(i->getEntryName().data(), i->getString().data());
    }

    style->read( NULL, tempnode );

    Inkscape::GC::release(tempnode);
    Inkscape::GC::release(tempdoc);
    delete tempdoc;
}


static CRSelEng *
sp_repr_sel_eng()
{
    CRSelEng *const ret = cr_sel_eng_new();
    cr_sel_eng_set_node_iface(ret, &Inkscape::XML::croco_node_iface);

    /** \todo
     * Check whether we need to register any pseudo-class handlers.
     * libcroco has its own default handlers for first-child and lang.
     *
     * We probably want handlers for link and arguably visited (though
     * inkscape can't visit links at the time of writing).  hover etc.
     * more useful in inkview than the editor inkscape.
     *
     * http://www.w3.org/TR/SVG11/styling.html#StylingWithCSS says that
     * the following should be honoured, at least by inkview:
     * :hover, :active, :focus, :visited, :link.
     */

    g_assert(ret);
    return ret;
}


// Called in text-editting.cpp, ui/tools/frehand-base.cpp, ui/widget/style-swatch.cpp
/**
 * Parses a style="..." string and merges it with an existing SPStyle.
 */
void
sp_style_merge_from_style_string(SPStyle *const style, gchar const *const p)
{
    // std::cout << "sp_style_merge_from_style_string: " << (p?p:"null") <<std::endl;
    /*
     * Reference: http://www.w3.org/TR/SVG11/styling.html#StyleAttribute:
     * ``When CSS styling is used, CSS inline style is specified by including
     * semicolon-separated property declarations of the form "name : value"
     * within the style attribute''.
     *
     * That's fairly ambiguous.  Is a `value' allowed to contain semicolons?
     * Why does it say "including", what else is allowed in the style
     * attribute value?
     */

    CRDeclaration *const decl_list
        = cr_declaration_parse_list_from_buf(reinterpret_cast<guchar const *>(p), CR_UTF_8);
    if (decl_list) {
        style->merge_decl_list( decl_list );
        cr_declaration_destroy(decl_list);
    }
}

/** Indexed by SP_CSS_FONT_SIZE_blah.   These seem a bit small */
static float const font_size_table[] = {6.0, 8.0, 10.0, 12.0, 14.0, 18.0, 24.0};

// Called in sp-object.cpp, sp-tref.cpp, sp-use.cpp
/**
 * Sets computed values in \a style, which may involve inheriting from (or in some other way
 * calculating from) corresponding computed values of \a parent.
 *
 * References: http://www.w3.org/TR/SVG11/propidx.html shows what properties inherit by default.
 * http://www.w3.org/TR/SVG11/styling.html#Inheritance gives general rules as to what it means to
 * inherit a value.  http://www.w3.org/TR/REC-CSS2/cascade.html#computed-value is more precise
 * about what the computed value is (not obvious for lengths).
 *
 * \pre \a parent's computed values are already up-to-date.
 */
void
sp_style_merge_from_parent(SPStyle *const style, SPStyle const *const parent)
{
    // std::cout << "sp_style_merge_from_parent" << std::endl;
    g_return_if_fail(style != NULL);

    if (!parent)
        return;

    style->cascade( parent );
    return;
}

// Called in: sp-use.cpp, sp-tref.cpp, sp-item.cpp
/**
 * Combine \a style and \a parent style specifications into a single style specification that
 * preserves (as much as possible) the effect of the existing \a style being a child of \a parent.
 *
 * Called when the parent repr is to be removed (e.g. the parent is a \<use\> element that is being
 * unlinked), in which case we copy/adapt property values that are explicitly set in \a parent,
 * trying to retain the same visual appearance once the parent is removed.  Interesting cases are
 * when there is unusual interaction with the parent's value (opacity, display) or when the value
 * can be specified as relative to the parent computed value (font-size, font-weight etc.).
 *
 * Doesn't update computed values of \a style.  For correctness, you should subsequently call
 * sp_style_merge_from_parent against the new parent (presumably \a parent's parent) even if \a
 * style was previously up-to-date wrt \a parent.
 *
 * \pre \a parent's computed values are already up-to-date.
 *   (\a style's computed values needn't be up-to-date.)
 */
void
sp_style_merge_from_dying_parent(SPStyle *const style, SPStyle const *const parent)
{
    // std::cout << "sp_style_merge_from_dying_parent" << std::endl;
    style->merge( parent );
}

// The following functions should be incorporated into SPIPaint. FIXME
// Called in: style.cpp, style-internal.cpp
void
sp_style_set_ipaint_to_uri(SPStyle *style, SPIPaint *paint, const Inkscape::URI *uri, SPDocument *document)
{
    // std::cout << "sp_style_set_ipaint_to_uri: Entrance: " << uri << "  " << (void*)document << std::endl;
    // it may be that this style's SPIPaint has not yet created its URIReference;
    // now that we have a document, we can create it here
    if (!paint->value.href && document) {
        paint->value.href = new SPPaintServerReference(document);
        paint->value.href->changedSignal().connect(sigc::bind(sigc::ptr_fun((paint == &style->fill)? sp_style_fill_paint_server_ref_changed : sp_style_stroke_paint_server_ref_changed), style));
    }

    if (paint->value.href){
        if (paint->value.href->getObject()){
            paint->value.href->detach();
        }

        try {
            paint->value.href->attach(*uri);
        } catch (Inkscape::BadURIException &e) {
            g_warning("%s", e.what());
            paint->value.href->detach();
        }
    }
}

// Called in: style.cpp, style-internal.cpp
void
sp_style_set_ipaint_to_uri_string (SPStyle *style, SPIPaint *paint, const gchar *uri)
{
    try {
        const Inkscape::URI IURI(uri);
        sp_style_set_ipaint_to_uri(style, paint, &IURI, style->document);
    } catch (...) {
        g_warning("URI failed to parse: %s", uri);
    }
}

// Called in: desktop-style.cpp
void
sp_style_set_to_uri_string (SPStyle *style, bool isfill, const gchar *uri)
{
    sp_style_set_ipaint_to_uri_string (style, isfill? &style->fill : &style->stroke, uri);
}

// Called in: widgets/font-selector.cpp, widgets/text-toolbar.cpp, ui/dialog/text-edit.cpp
gchar const *
sp_style_get_css_unit_string(int unit)
{
    // specify px by default, see inkscape bug 1221626, mozilla bug 234789

    switch (unit) {

        case SP_CSS_UNIT_NONE: return "px";
        case SP_CSS_UNIT_PX: return "px";
        case SP_CSS_UNIT_PT: return "pt";
        case SP_CSS_UNIT_PC: return "pc";
        case SP_CSS_UNIT_MM: return "mm";
        case SP_CSS_UNIT_CM: return "cm";
        case SP_CSS_UNIT_IN: return "in";
        case SP_CSS_UNIT_EM: return "em";
        case SP_CSS_UNIT_EX: return "ex";
        case SP_CSS_UNIT_PERCENT: return "%";
        default: return "px";
    }
    return "px";
}

// Called in: style-internal.cpp, widgets/text-toolbar.cpp, ui/dialog/text-edit.cpp
/*
 * Convert a size in pixels into another CSS unit size
 */
double
sp_style_css_size_px_to_units(double size, int unit)
{
    double unit_size = size;
    switch (unit) {

        case SP_CSS_UNIT_NONE: unit_size = size; break;
        case SP_CSS_UNIT_PX: unit_size = size; break;
        case SP_CSS_UNIT_PT: unit_size = Inkscape::Util::Quantity::convert(size, "px", "pt");  break;
        case SP_CSS_UNIT_PC: unit_size = Inkscape::Util::Quantity::convert(size, "px", "pc");  break;
        case SP_CSS_UNIT_MM: unit_size = Inkscape::Util::Quantity::convert(size, "px", "mm");  break;
        case SP_CSS_UNIT_CM: unit_size = Inkscape::Util::Quantity::convert(size, "px", "cm");  break;
        case SP_CSS_UNIT_IN: unit_size = Inkscape::Util::Quantity::convert(size, "px", "in");  break;
        case SP_CSS_UNIT_EM: unit_size = size / SP_CSS_FONT_SIZE_DEFAULT; break;
        case SP_CSS_UNIT_EX: unit_size = size * 2.0 / SP_CSS_FONT_SIZE_DEFAULT ; break;
        case SP_CSS_UNIT_PERCENT: unit_size = size * 100.0 / SP_CSS_FONT_SIZE_DEFAULT; break;

        default:
            g_warning("sp_style_get_css_font_size_units conversion to %d not implemented.", unit);
            break;
    }

    return unit_size;
}

// Called in: widgets/text-toolbar.cpp, ui/dialog/text-edit.cpp
/*
 * Convert a size in a CSS unit size to pixels
 */
double
sp_style_css_size_units_to_px(double size, int unit)
{
    if (unit == SP_CSS_UNIT_PX) {
        return size;
    }
    //g_message("sp_style_css_size_units_to_px %f %d = %f px", size, unit, out);
    return size * (size / sp_style_css_size_px_to_units(size, unit));;
}

// Called in style.cpp, text-editing.cpp
/**
 * Dumps the style to a CSS string, with either SP_STYLE_FLAG_IFSET or
 * SP_STYLE_FLAG_ALWAYS flags. Used with Always for copying an object's
 * complete cascaded style to style_clipboard. When you need a CSS string
 * for an object in the document tree, you normally call
 * sp_style_write_difference instead to take into account the object's parent.
 *
 * \pre style != NULL.
 * \pre flags in {IFSET, ALWAYS}.
 * \post ret != NULL.
 */
gchar *
sp_style_write_string(SPStyle const *const style, guint const flags)
{
    /** \todo
     * Merge with write_difference, much duplicate code!
     */
    g_return_val_if_fail(style != NULL, NULL);
    g_return_val_if_fail(((flags == SP_STYLE_FLAG_IFSET) ||
                          (flags == SP_STYLE_FLAG_ALWAYS)  ),
                         NULL);

    return g_strdup( style->write( flags ).c_str() );
}


// Called in style.cpp, path-chemistry, NOT in text-editting.cpp (because of bug)
/**
 * Dumps style to CSS string, see sp_style_write_string()
 *
 * \pre from != NULL.
 * \pre to != NULL.
 * \post ret != NULL.
 */
gchar *
sp_style_write_difference(SPStyle const *const from, SPStyle const *const to)
{
    g_return_val_if_fail(from != NULL, NULL);
    g_return_val_if_fail(to != NULL, NULL);

    return g_strdup( from->write( SP_STYLE_FLAG_IFDIFF, to ).c_str() );
}


// FIXME: Everything below this line belongs in a different file - css-chemistry?

void
sp_style_set_property_url (SPObject *item, gchar const *property, SPObject *linked, bool recursive)
{
    Inkscape::XML::Node *repr = item->getRepr();

    if (repr == NULL) return;

    SPCSSAttr *css = sp_repr_css_attr_new();
    if (linked) {
        gchar *val = g_strdup_printf("url(#%s)", linked->getId());
        sp_repr_css_set_property(css, property, val);
        g_free(val);
    } else {
        sp_repr_css_unset_property(css, "filter");
    }

    if (recursive) {
        sp_repr_css_change_recursive(repr, css, "style");
    } else {
        sp_repr_css_change(repr, css, "style");
    }
    sp_repr_css_attr_unref(css);
}

// Called in sp-object.cpp
/**
 * Clear all style property attributes in object.
 */
void
sp_style_unset_property_attrs(SPObject *o)
{
    if (!o) {
        return;
    }

    SPStyle *style = o->style;
    if (!style) {
        return;
    }

    Inkscape::XML::Node *repr = o->getRepr();
    if (!repr) {
        return;
    }

    if (style->opacity.set) {
        repr->setAttribute("opacity", NULL);
    }
    if (style->color.set) {
        repr->setAttribute("color", NULL);
    }
    if (style->color_interpolation.set) {
        repr->setAttribute("color-interpolation", NULL);
    }
    if (style->color_interpolation_filters.set) {
        repr->setAttribute("color-interpolation-filters", NULL);
    }
    if (style->fill.set) {
        repr->setAttribute("fill", NULL);
    }
    if (style->fill_opacity.set) {
        repr->setAttribute("fill-opacity", NULL);
    }
    if (style->fill_rule.set) {
        repr->setAttribute("fill-rule", NULL);
    }
    if (style->stroke.set) {
        repr->setAttribute("stroke", NULL);
    }
    if (style->stroke_width.set) {
        repr->setAttribute("stroke-width", NULL);
    }
    if (style->stroke_linecap.set) {
        repr->setAttribute("stroke-linecap", NULL);
    }
    if (style->stroke_linejoin.set) {
        repr->setAttribute("stroke-linejoin", NULL);
    }
    if (style->marker[SP_MARKER_LOC].set) {
        repr->setAttribute("marker", NULL);
    }
    if (style->marker[SP_MARKER_LOC_START].set) {
        repr->setAttribute("marker-start", NULL);
    }
    if (style->marker[SP_MARKER_LOC_MID].set) {
        repr->setAttribute("marker-mid", NULL);
    }
    if (style->marker[SP_MARKER_LOC_END].set) {
        repr->setAttribute("marker-end", NULL);
    }
    if (style->stroke_opacity.set) {
        repr->setAttribute("stroke-opacity", NULL);
    }
    if (style->stroke_dasharray.set) {
        repr->setAttribute("stroke-dasharray", NULL);
    }
    if (style->stroke_dashoffset.set) {
        repr->setAttribute("stroke-dashoffset", NULL);
    }
    if (style->paint_order.set) {
        repr->setAttribute("paint-order", NULL);
    }
    if (style->text->font_specification.set) {
        repr->setAttribute("-inkscape-font-specification", NULL);
    }
    if (style->text->font_family.set) {
        repr->setAttribute("font-family", NULL);
    }
    if (style->text_anchor.set) {
        repr->setAttribute("text-anchor", NULL);
    }
    if (style->writing_mode.set) {
        repr->setAttribute("writing_mode", NULL);
    }
    if (style->filter.set) {
        repr->setAttribute("filter", NULL);
    }
    if (style->enable_background.set) {
        repr->setAttribute("enable-background", NULL);
    }
    if (style->clip_rule.set) {
        repr->setAttribute("clip-rule", NULL);
    }
    if (style->color_rendering.set) {
        repr->setAttribute("color-rendering", NULL);
    }
    if (style->image_rendering.set) {
        repr->setAttribute("image-rendering", NULL);
    }
    if (style->shape_rendering.set) {
        repr->setAttribute("shape-rendering", NULL);
    }
    if (style->text_rendering.set) {
        repr->setAttribute("text-rendering", NULL);
    }
}

/**
 * \pre style != NULL.
 * \pre flags in {IFSET, ALWAYS}.
 */
SPCSSAttr *
sp_css_attr_from_style(SPStyle const *const style, guint const flags)
{
    g_return_val_if_fail(style != NULL, NULL);
    g_return_val_if_fail(((flags == SP_STYLE_FLAG_IFSET) ||
                          (flags == SP_STYLE_FLAG_ALWAYS)  ),
                         NULL);
    gchar *style_str = sp_style_write_string(style, flags);
    SPCSSAttr *css = sp_repr_css_attr_new();
    sp_repr_css_attr_add_from_string(css, style_str);
    g_free(style_str);
    return css;
}

// Called in: selection-chemistry.cpp, widgets/stroke-marker-selector.cpp, widgets/stroke-style.cpp,
// ui/tools/freehand-base.cpp
/**
 * \pre object != NULL
 * \pre flags in {IFSET, ALWAYS}.
 */
SPCSSAttr *sp_css_attr_from_object(SPObject *object, guint const flags)
{
    g_return_val_if_fail(((flags == SP_STYLE_FLAG_IFSET) ||
                          (flags == SP_STYLE_FLAG_ALWAYS)  ),
                         NULL);
    SPCSSAttr * result = 0;    
    if (object->style) {
        result = sp_css_attr_from_style(object->style, flags);
    }
    return result;
}

// Called in: selection-chemistry.cpp, ui/dialog/inkscape-preferences.cpp
/**
 * Unset any text-related properties
 */
SPCSSAttr *
sp_css_attr_unset_text(SPCSSAttr *css)
{
    sp_repr_css_set_property(css, "font", NULL); // not implemented yet
    sp_repr_css_set_property(css, "-inkscape-font-specification", NULL);
    sp_repr_css_set_property(css, "font-size", NULL);
    sp_repr_css_set_property(css, "font-size-adjust", NULL); // not implemented yet
    sp_repr_css_set_property(css, "font-style", NULL);
    sp_repr_css_set_property(css, "font-variant", NULL);
    sp_repr_css_set_property(css, "font-weight", NULL);
    sp_repr_css_set_property(css, "font-stretch", NULL);
    sp_repr_css_set_property(css, "font-family", NULL);
    sp_repr_css_set_property(css, "text-indent", NULL);
    sp_repr_css_set_property(css, "text-align", NULL);
    sp_repr_css_set_property(css, "text-decoration", NULL);
    sp_repr_css_set_property(css, "line-height", NULL);
    sp_repr_css_set_property(css, "letter-spacing", NULL);
    sp_repr_css_set_property(css, "word-spacing", NULL);
    sp_repr_css_set_property(css, "text-transform", NULL);
    sp_repr_css_set_property(css, "direction", NULL);
    sp_repr_css_set_property(css, "block-progression", NULL);
    sp_repr_css_set_property(css, "writing-mode", NULL);
    sp_repr_css_set_property(css, "text-anchor", NULL);
    sp_repr_css_set_property(css, "kerning", NULL); // not implemented yet
    sp_repr_css_set_property(css, "dominant-baseline", NULL); // not implemented yet
    sp_repr_css_set_property(css, "alignment-baseline", NULL); // not implemented yet
    sp_repr_css_set_property(css, "baseline-shift", NULL);

    return css;
}

// Called in style.cpp
static bool
is_url(char const *p)
{
    if (p == NULL)
        return false;
/** \todo
 * FIXME: I'm not sure if this applies to SVG as well, but CSS2 says any URIs
 * in property values must start with 'url('.
 */
    return (g_ascii_strncasecmp(p, "url(", 4) == 0);
}

// Called in: ui/dialog/inkscape-preferences.cpp, ui/tools/tweek-tool.cpp
/**
 * Unset any properties that contain URI values.
 *
 * Used for storing style that will be reused across documents when carrying
 * the referenced defs is impractical.
 */
SPCSSAttr *
sp_css_attr_unset_uris(SPCSSAttr *css)
{
// All properties that may hold <uri> or <paint> according to SVG 1.1
    if (is_url(sp_repr_css_property(css, "clip-path", NULL))) sp_repr_css_set_property(css, "clip-path", NULL);
    if (is_url(sp_repr_css_property(css, "color-profile", NULL))) sp_repr_css_set_property(css, "color-profile", NULL);
    if (is_url(sp_repr_css_property(css, "cursor", NULL))) sp_repr_css_set_property(css, "cursor", NULL);
    if (is_url(sp_repr_css_property(css, "filter", NULL))) sp_repr_css_set_property(css, "filter", NULL);
    if (is_url(sp_repr_css_property(css, "marker", NULL))) sp_repr_css_set_property(css, "marker", NULL);
    if (is_url(sp_repr_css_property(css, "marker-start", NULL))) sp_repr_css_set_property(css, "marker-start", NULL);
    if (is_url(sp_repr_css_property(css, "marker-mid", NULL))) sp_repr_css_set_property(css, "marker-mid", NULL);
    if (is_url(sp_repr_css_property(css, "marker-end", NULL))) sp_repr_css_set_property(css, "marker-end", NULL);
    if (is_url(sp_repr_css_property(css, "mask", NULL))) sp_repr_css_set_property(css, "mask", NULL);
    if (is_url(sp_repr_css_property(css, "fill", NULL))) sp_repr_css_set_property(css, "fill", NULL);
    if (is_url(sp_repr_css_property(css, "stroke", NULL))) sp_repr_css_set_property(css, "stroke", NULL);

    return css;
}

// Called in style.cpp
/**
 * Scale a single-value property.
 */
static void
sp_css_attr_scale_property_single(SPCSSAttr *css, gchar const *property,
                                  double ex, bool only_with_units = false)
{
    gchar const *w = sp_repr_css_property(css, property, NULL);
    if (w) {
        gchar *units = NULL;
        double wd = g_ascii_strtod(w, &units) * ex;
        if (w == units) {// nothing converted, non-numeric value
            return;
        }
        if (only_with_units && (units == NULL || *units == '\0' || *units == '%')) {
            // only_with_units, but no units found, so do nothing.
            return;
        }
        Inkscape::CSSOStringStream os;
        os << wd << units; // reattach units
        sp_repr_css_set_property(css, property, os.str().c_str());
    }
}

// Called in style.cpp for stroke-dasharray
/**
 * Scale a list-of-values property.
 */
static void
sp_css_attr_scale_property_list(SPCSSAttr *css, gchar const *property, double ex)
{
    gchar const *string = sp_repr_css_property(css, property, NULL);
    if (string) {
        Inkscape::CSSOStringStream os;
        gchar **a = g_strsplit(string, ",", 10000);
        bool first = true;
        for (gchar **i = a; i != NULL; i++) {
            gchar *w = *i;
            if (w == NULL)
                break;
            gchar *units = NULL;
            double wd = g_ascii_strtod(w, &units) * ex;
            if (w == units) {// nothing converted, non-numeric value ("none" or "inherit"); do nothing
                g_strfreev(a);
                return;
            }
            if (!first) {
                os << ",";
            }
            os << wd << units; // reattach units
            first = false;
        }
        sp_repr_css_set_property(css, property, os.str().c_str());
        g_strfreev(a);
    }
}

// Called in: text-editing.cpp, 
/**
 * Scale any properties that may hold <length> by ex.
 */
SPCSSAttr *
sp_css_attr_scale(SPCSSAttr *css, double ex)
{
    sp_css_attr_scale_property_single(css, "baseline-shift", ex);
    sp_css_attr_scale_property_single(css, "stroke-width", ex);
    sp_css_attr_scale_property_list  (css, "stroke-dasharray", ex);
    sp_css_attr_scale_property_single(css, "stroke-dashoffset", ex);
    sp_css_attr_scale_property_single(css, "font-size", ex);
    sp_css_attr_scale_property_single(css, "kerning", ex);
    sp_css_attr_scale_property_single(css, "letter-spacing", ex);
    sp_css_attr_scale_property_single(css, "word-spacing", ex);
    sp_css_attr_scale_property_single(css, "line-height", ex, true);

    return css;
}


// Called in style.cpp, xml/repr-css.cpp
/**
 * Remove quotes and escapes from a string. Returned value must be g_free'd.
 * Note: in CSS (in style= and in stylesheets), unquoting and unescaping is done
 * by libcroco, our CSS parser, though it adds a new pair of "" quotes for the strings
 * it parsed for us. So this function is only used to remove those quotes and for
 * presentation attributes, without any unescaping. (XML unescaping
 * (&amp; etc) is done by XML parser.)
 */
gchar *
attribute_unquote(gchar const *val)
{
    if (val) {
        if (*val == '\'' || *val == '"') {
            int l = strlen(val);
            if (l >= 2) {
                if ( ( val[0] == '"' && val[l - 1] == '"' )  ||
                     ( val[0] == '\'' && val[l - 1] == '\'' )  ) {
                    return (g_strndup (val+1, l-2));
                }
            }
        }
    }

    return (val? g_strdup (val) : NULL);
}

// Called in style.cpp, xml/repr-css.cpp
/**
 * Quote and/or escape string for writing to CSS (style=). Returned value must be g_free'd.
 */
Glib::ustring css2_escape_quote(gchar const *val) {

    Glib::ustring t;
    bool quote = false;
    bool last_was_space = false;

    for (gchar const *i = val; *i; i++) {
        bool is_space = ( *i == ' ' );
        if (g_ascii_isalnum(*i) || *i=='-' || *i=='_') {
            // ASCII alphanumeric, - and _ don't require quotes
            t.push_back(*i);
        } else if ( is_space && !last_was_space ) {
            // non-consecutive spaces don't require quotes
            t.push_back(*i);
        } else if (*i=='\'') {
            // single quotes require escaping and quotes
            t.push_back('\\');
            t.push_back(*i);
            quote = true;
        } else {
            // everything else requires quotes
            t.push_back(*i);
            quote = true;
        }
        if (i == val && !g_ascii_isalpha(*i)) {
            // a non-ASCII/non-alpha initial character requires quotes
            quote = true;
        }
        last_was_space = is_space;
    }

    if (last_was_space) {
        // a trailing space requires quotes
        quote = true;
    }

    if (quote) {
        // we use single quotes so the result can be stored in an XML
        // attribute without incurring un-aesthetic additional quoting
        // (our XML emitter always uses double quotes)
        t.insert(t.begin(), '\'');
        t.push_back('\'');
    }

    return t;
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
// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:fileencoding=utf-8:textwidth=99 :

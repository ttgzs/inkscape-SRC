/**
 * @file
 * @brief SVG drawing for display
 *//*
 * Authors:
 *   Krzysztof Kosiński <tweenk.pl@gmail.com>
 *
 * Copyright (C) 2011 Authors
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#ifndef SEEN_INKSCAPE_DISPLAY_DRAWING_H
#define SEEN_INKSCAPE_DISPLAY_DRAWING_H

#include <set>
#include <boost/utility.hpp>
#include <sigc++/sigc++.h>
#include <2geom/rect.h>
#include "display/display-forward.h"
#include "display/drawing-item.h"
#include "display/rendermode.h"

namespace Inkscape {

struct OutlineColors {
    guint32 paths;
    guint32 clippaths;
    guint32 masks;
    guint32 images;
};

class Drawing
    : boost::noncopyable
{
public:
    Drawing(SPCanvasArena *arena = NULL);
    ~Drawing();

    DrawingItem *root() { return _root; }
    SPCanvasArena *arena() { return _canvasarena; }
    void setRoot(DrawingItem *item);

    RenderMode renderMode() const;
    ColorMode colorMode() const;
    bool outline() const;
    bool renderFilters() const;
    int blurQuality() const;
    int filterQuality() const;
    void setRenderMode(RenderMode mode);
    void setColorMode(ColorMode mode);
    void setBlurQuality(int q);
    void setFilterQuality(int q);
    void setExact(bool e);

    Geom::OptIntRect const &cacheLimit() const;
    void setCacheLimit(Geom::OptIntRect const &r);

    OutlineColors const &colors() const { return _colors; }

    void update(Geom::IntRect const &area = Geom::IntRect::infinite(), UpdateContext const &ctx = UpdateContext(), unsigned flags = DrawingItem::STATE_ALL, unsigned reset = 0);
    void render(DrawingContext &ct, Geom::IntRect const &area, unsigned flags = 0);
    DrawingItem *pick(Geom::Point const &p, double delta, bool sticky);

    sigc::signal<void, DrawingItem *> signal_request_update;
    sigc::signal<void, Geom::IntRect const &> signal_request_render;
    sigc::signal<void, DrawingItem *> signal_item_deleted;

private:
    DrawingItem *_root;
    std::set<DrawingItem *> _cached_items;
public:
    // TODO: remove these temporarily public members
    guint32 outlinecolor;
    double delta;
private:
    bool _exact;  // if true then rendering must be exact
    RenderMode _rendermode;
    ColorMode _colormode;
    int _blur_quality;
    int _filter_quality;
    Geom::OptIntRect _cache_limit;

    OutlineColors _colors;

    SPCanvasArena *_canvasarena; // may be NULL is this arena is not the screen but used for export etc.

    friend class DrawingItem;
};

} // end namespace Inkscape

#endif // !SEEN_INKSCAPE_DRAWING_H

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

#define INKSCAPE_LPE_MIRROR_REFLECT_CPP
/** \file
 * LPE <mirror_reflection> implementation: mirrors a path with respect to a given line.
 */
/*
 * Authors:
 *   Maximilian Albert
 *   Johan Engelen
 *
 * Copyright (C) Johan Engelen 2007 <j.b.c.engelen@utwente.nl>
 * Copyright (C) Maximilin Albert 2008 <maximilian.albert@gmail.com>
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/lpe-mirror_reflect.h"
#include <sp-path.h>
#include <display/curve.h>
#include <svg/path-string.h>

#include <2geom/path.h>
#include <2geom/transforms.h>
#include <2geom/matrix.h>

namespace Inkscape {
namespace LivePathEffect {

LPEMirrorReflect::LPEMirrorReflect(LivePathEffectObject *lpeobject) :
    Effect(lpeobject),
    reflection_line(_("Reflection line"), _("Line which serves as 'mirror' for the reflection"), "reflection_line", &wr, this, "M0,0 L100,100")
{
    registerParameter( dynamic_cast<Parameter *>(&reflection_line) );
}

LPEMirrorReflect::~LPEMirrorReflect()
{

}

void
LPEMirrorReflect::doOnApply (SPLPEItem *lpeitem)
{
    using namespace Geom;

    SPCurve *curve = sp_path_get_curve_for_edit (SP_PATH(lpeitem));
    Point A(curve->first_point().to_2geom());
    Point B(curve->last_point().to_2geom());

    Point M = (2*A + B)/3; // some point between A and B (a bit closer to A)
    Point perp_dir = unit_vector((B - A).ccw());
    
    Point C(M[X], M[Y] + 150);
    Point D(M[X], M[Y] - 150);

    Piecewise<D2<SBasis> > rline = Piecewise<D2<SBasis> >(D2<SBasis>(Linear(C[X], D[X]), Linear(C[Y], D[Y])));
    reflection_line.param_set_and_write_new_value(rline);
}

std::vector<Geom::Path>
LPEMirrorReflect::doEffect_path (std::vector<Geom::Path> const & path_in)
{
    std::vector<Geom::Path> path_out;

    std::vector<Geom::Path> mline(reflection_line.get_pathvector());
    Geom::Point A(mline.front().initialPoint());
    Geom::Point B(mline.back().finalPoint());

    Geom::Matrix m1(1.0, 0.0, 0.0, 1.0, A[0], A[1]);
    double hyp = Geom::distance(A, B);
    double c = (B[0] - A[0]) / hyp; // cos(alpha)
    double s = (B[1] - A[1]) / hyp; // sin(alpha)

    Geom::Matrix m2(c, -s, s, c, 0.0, 0.0);
    Geom::Matrix sca(1.0, 0.0, 0.0, -1.0, 0.0, 0.0);

    Geom::Matrix m = m1.inverse() * m2;
    m = m * sca;
    m = m * m2.inverse();
    m = m * m1;

    for (int i = 0; i < path_in.size(); ++i) {
        path_out.push_back(path_in[i] * m);
    }

    return path_out;
}

/* ######################## */

} //namespace LivePathEffect
} /* namespace Inkscape */

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

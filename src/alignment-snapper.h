// SPDX-License-Identifier: GPL-2.0-or-later
/** \file
 * Authors:
 *   Parth Pant <parthpant4@gmail.com>
 *
 * Copyright (C) 2021 Authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#ifndef SEEN_ALIGNMENT_SNAPPER_H
#define SEEN_ALIGNMENT_SNAPPER_H

#include <2geom/affine.h>

#include "snap-enums.h"
#include "snapper.h"
#include "snap-candidate.h"

class SPDesktop;
class SPNamedView;
class SPItem;
class SPObject;
class SPPath;
class SPDesktop;

namespace Inkscape
{

/**
 * Snapping things to on-canvas alignment guides
 */
class AlignmentSnapper : public Snapper
{

public:
    AlignmentSnapper(SnapManager *sm, Geom::Coord const d);
    ~AlignmentSnapper() override;

    /**
     * @return true if this Snapper will snap at least one kind of point.
     */
    bool ThisSnapperMightSnap() const override;

    /**
     * @return Snap tolerance (desktop coordinates); depends on current zoom so that it's always the same in screen pixels.
     */
    Geom::Coord getSnapperTolerance() const override; //returns the tolerance of the snapper in screen pixels (i.e. independent of zoom)

    bool getSnapperAlwaysSnap() const override; //if true, then the snapper will always snap, regardless of its tolerance

    void freeSnap(IntermSnapResults &isr,
                  Inkscape::SnapCandidatePoint const &p,
                  Geom::OptRect const &bbox_to_snap,
                  std::vector<SPItem const *> const *it,
                  std::vector<SnapCandidatePoint> *unselected_nodes) const override;

    void constrainedSnap(IntermSnapResults &isr,
                  Inkscape::SnapCandidatePoint const &p,
                  Geom::OptRect const &bbox_to_snap,
                  SnapConstraint const &c,
                  std::vector<SPItem const *> const *it,
                  std::vector<SnapCandidatePoint> *unselected_nodes) const override;

private:
    std::vector<SnapCandidatePoint> *_points_to_snap_to;

    /** Collects and caches points on bounding boxes of the candidates
     * @param is the point first point in the selection?
     */
    void _collectBBoxPoints(bool const &first_point) const;

    void _snapBBoxPoints(IntermSnapResults &isr,
                         SnapCandidatePoint const &p,
                         std::vector<SnapCandidatePoint> *unselected_nodes,
                         SnapConstraint const &c =  SnapConstraint(),
                         Geom::Point const &p_proj_on_constraint = Geom::Point()) const;

    SnapSourceType source2alignment(SnapSourceType s) const;
}; // end of AlignmentSnapper class

} // end of namespace Inkscape

#endif
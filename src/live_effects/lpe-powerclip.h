#ifndef INKSCAPE_LPE_POWERCLIP_H
#define INKSCAPE_LPE_POWERCLIP_H

/*
 * Inkscape::LPEPowerClip
 *
 * Released under GNU GPL, read the file 'COPYING' for more information
 */

#include "live_effects/effect.h"
#include "live_effects/parameter/hidden.h"
#include "live_effects/parameter/path.h"
#include "live_effects/lpegroupbbox.h"

namespace Inkscape {
namespace LivePathEffect {

class LPEPowerClip : public Effect, GroupBBoxEffect {
public:
    LPEPowerClip(LivePathEffectObject *lpeobject);
    virtual ~LPEPowerClip();
    virtual void doBeforeEffect (SPLPEItem const* lpeitem);
    virtual Geom::PathVector doEffect_path (Geom::PathVector const & path_in);
    //virtual void doOnVisibilityToggled(SPLPEItem const* lpeitem);
    virtual void doOnRemove (SPLPEItem const* /*lpeitem*/);
    virtual Gtk::Widget * newWidget();
    void toggleClip();
    void addInverse (SPItem * clip_data);
    void removeInverse (SPItem * clip_data);
    void flattenClip(SPItem * clip_data, Geom::PathVector &path_in);
    void setFillRule(SPItem * clip_data);
protected:
    //virtual void addCanvasIndicators(SPLPEItem const *lpeitem, std::vector<Geom::PathVector> &hp_vec);

private:
    BoolParam inverse;
    BoolParam flatten;
    BoolParam fillrule;
    BoolParam convert_shapes;
    HiddenParam is_inverse;
    Geom::Path clip_box;
    bool is_clip;
    bool hide_clip;
    bool previous_fillrule;
    bool previous_hide_clip;
};

} //namespace LivePathEffect
} //namespace Inkscape
#endif

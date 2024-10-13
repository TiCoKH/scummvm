#ifndef GOLDBOX_POOLRAD_VIEWS_PARTY_H
#define GOLDBOX_POOLRAD_VIEWS_PARTY_H

#include "goldbox/poolrad/views/view.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

class Party : public View {
public:
    Party(UIElement *parent);
    void draw() override;
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_PARTYVIEW_H

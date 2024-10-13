#ifndef GOLDBOX_POOLRAD_VIEWS_PROMPTVIEW_H
#define GOLDBOX_POOLRAD_VIEWS_PROMPTVIEW_H

#include "goldbox/poolrad/views/view.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

class PromptView : public View {
public:
    PromptView(UIElement *parent);
    void draw() override;
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_PROMPTVIEW_H

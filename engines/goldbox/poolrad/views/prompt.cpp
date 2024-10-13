#include "goldbox/poolrad/views/prompt.h"
#include "graphics/managed_surface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

PromptView::PromptView(UIElement *parent) : View("PromptView", parent) {}

void PromptView::draw() {
    Surface s = getSurface();
    s.writeStringC("CHOOSE A FUNCTION", 24, 2, 0);
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

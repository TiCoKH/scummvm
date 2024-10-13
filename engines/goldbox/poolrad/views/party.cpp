#include "goldbox/poolrad/views/party.h"
#include "graphics/managed_surface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

Party::Party(UIElement *parent) : View("Party", parent) {}

void Party::draw() {
    Surface s = getSurface();
    s.writeStringC("NAME    AC  HP", 1, 2, 0);
    // Example of displaying one character; expand this as needed.
    s.writeStringC("DORNIC     9   4", 3, 2, 0);
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

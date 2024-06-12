#ifndef ALIS_SCREEN_H
#define ALIS_SCREEN_H

#include "common/scummsys.h"
#include "common/rect.h"
#include "graphics/cursorman.h"
#include "graphics/surface.h"

namespace Alis {

class AlisScreen {
public:
    AlisScreen();
    ~AlisScreen();

    void init(uint16 width, uint16 height);
    void draw();

private:
    Graphics::Surface _surface;
};

} // End of namespace Alis

#endif // ALIS_SCREEN_H

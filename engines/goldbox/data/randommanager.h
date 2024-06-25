// RandomManager.h
#ifndef GOLD_BOX_RANDOM_MANAGER_H
#define GOLD_BOX_RANDOM_MANAGER_H

#include "common/random.h"
#include "common/system.h"

namespace GoldBox {

    class RandomManager {
    public:
        static RandomManager &instance() {
            static RandomManager _instance;
            return _instance;
        }

        Common::RandomSource &getRandomSource() {
            return _randomSource;
        }

    private:
        Common::RandomSource _randomSource;

        RandomManager() : _randomSource("goldbox_random") {
            uint32 seed = g_system->getMillis(); // Get the current time in milliseconds
            _randomSource.setSeed(seed);
        }

        // Delete copy constructor and assignment operator to prevent copies
        RandomManager(const RandomManager &) = delete;
        RandomManager &operator=(const RandomManager &) = delete;
    };

} // namespace GoldBox

#endif // GOLD_BOX_RANDOM_MANAGER_H

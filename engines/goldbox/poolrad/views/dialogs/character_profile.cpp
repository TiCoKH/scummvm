/* ScummVM - Graphic Adventure Engine
 *
 * CharacterProfile dialog for displaying character details
 */
#include "goldbox/poolrad/views/dialogs/character_profile.h"
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CharacterProfile::CharacterProfile(Goldbox::Poolrad::Data::PoolradCharacter *pc, const Common::String &name)
    : Dialog(name), _poolradPc(pc) {
    // You can add any additional initialization here if needed
}

void CharacterProfile::draw() {
    if (!_poolradPc)
        return;

    drawIdentity();
    drawStats();
    drawValuables();
    drawLevelExp();
    drawCombat();
    drawItems();
    drawStatus();
}

void CharacterProfile::drawIdentity() {
    Surface s = getSurface();
    s.writeStringC(_poolradPc->name, 11, 1, 1);
    if (_poolradPc->npc) {
        int nameLen = strlen(_poolradPc->name.c_str());
        s.writeStringC("(NPC)", 14, 1 + nameLen + 3, 1);
    }
    Common::String raceClass = VmInterface::getString(Common::String::format("races.%d", _poolradPc->race));
    raceClass += "/";
    raceClass += VmInterface::getString(Common::String::format("classes.%d", _poolradPc->classType));
    s.writeStringC(raceClass, 10, 1, 3);
    s.writeStringC(VmInterface::getString(Common::String::format("alignments.%d", _poolradPc->alignment)), 10, 1, 4);
    s.writeStringC("Age", 10, 1, 5);
    s.writeStringC(Common::String::format("%d", _poolradPc->age), 10, 5, 5);
    s.writeStringC(_poolradPc->getClassName(), 10, 1, 6);
    s.writeStringC(_poolradPc->getGenderName(), 10, 10, 6);
}

void CharacterProfile::drawStats() {
    drawStat("STR", _poolradPc->abilities.strength.current, 10, 7);
    drawStat("INT", _poolradPc->abilities.intelligence.current, 10, 8);
    drawStat("WIS", _poolradPc->abilities.wisdom.current, 10, 9);
    drawStat("DEX", _poolradPc->abilities.dexterity.current, 10, 10);
    drawStat("CON", _poolradPc->abilities.constitution.current, 10, 11);
    drawStat("CHA", _poolradPc->abilities.charisma.current, 10, 12);
    drawStatSummary(25, 7);
}

void CharacterProfile::drawValuables() {
    Surface s = getSurface();
    int y = 14;
    s.writeStringC("Gold", 10, 1, y);
    s.writeStringC(Common::String::format("%d", _poolradPc->valuableItems.coinsGold), 10, 6, y);
    s.writeStringC("Gems", 10, 12, y);
    s.writeStringC(Common::String::format("%d", _poolradPc->valuableItems.gems), 10, 17, y);
    s.writeStringC("Jewelry", 10, 22, y);
    s.writeStringC(Common::String::format("%d", _poolradPc->valuableItems.jewelry), 10, 30, y);
}

void CharacterProfile::drawLevelExp() {
    Surface s = getSurface();
    s.writeStringC("Level", 10, 15, 13);
    Common::String levelStr;
    const Goldbox::Data::LevelData &levels = _poolradPc->levels;
    int levelVals[8] = {levels.cleric, levels.druid, levels.fighter, levels.paladin, levels.ranger, levels.mage, levels.thief, levels.monk};
    for (int i = 0; i < 8; ++i) {
        int lvl = levelVals[i];
        if (lvl > 0) {
            if (!levelStr.empty()) levelStr += "/";
            levelStr += Common::String::format("%d", lvl);
        }
    }
    s.writeStringC(levelStr, 10, 21, 13);
    s.writeStringC("Exp", 10, 15, 15);
    s.writeStringC(Common::String::format("%u", _poolradPc->experiencePoints), 10, 19, 15);
}

void CharacterProfile::drawCombat() {
    Surface s = getSurface();
    int y = 16;
    s.writeStringC("AC", 10, 1, y);
    s.writeStringC(Common::String::format("%d", _poolradPc->armorClass.current), 10, 4, y);
    s.writeStringC("HP", 10, 8, y);
    s.writeStringC(Common::String::format("%d/%d", _poolradPc->hitPoints.current, _poolradPc->hitPoints.max), 10, 11, y);
    s.writeStringC("THAC0", 10, 18, y);
    s.writeStringC(Common::String::format("%d", _poolradPc->thac0.current), 10, 24, y);
    s.writeStringC("Enc", 10, 1, y+1);
    s.writeStringC(Common::String::format("%d", _poolradPc->encumbrance), 10, 5, y+1);
    s.writeStringC("Move", 10, 10, y+1);
    s.writeStringC(Common::String::format("%d", _poolradPc->baseMovement), 10, 15, y+1);
}

void CharacterProfile::drawItems() {
    Surface s = getSurface();
    int y = 20;
    s.writeStringC("Weapon", 10, 15, y);
    s.writeStringC("-", 10, 10, y);
    s.writeStringC("Armor", 10, 15, y+1);
    s.writeStringC("-", 10, 10, y+1);
}

void CharacterProfile::drawStatus() {
    Surface s = getSurface();
    s.writeStringC("Status", 10, 15, 22);
    s.writeStringC(_poolradPc->getStatusName(), 10, 10, 22);
}

void CharacterProfile::drawStat(const char *label, int value, int x, int y) {
    Surface s = getSurface();
    s.writeStringC(label, 10, x, y);
    s.writeStringC(Common::String::format("%d", value), 10, x + 4, y);
    drawStatInfo(label, value, x + 8, y);
}

void CharacterProfile::drawStatInfo(const char *label, int value, int x, int y) {
    // Implement additional info display for stat if needed
}

void CharacterProfile::drawStatSummary(int x, int y) {
    // Implement stat summary display
}

void CharacterProfile::drawBattleStatus(int x, int y) {
    // Implement battle status display
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox


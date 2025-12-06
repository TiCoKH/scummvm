/* ScummVM - Graphic Adventure Engine
 *
 * CharacterProfile dialog for displaying character details
 */
#include "goldbox/poolrad/views/dialogs/character_profile.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "common/keyboard.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CharacterProfile::CharacterProfile(Goldbox::Poolrad::Data::PoolradCharacter *pc, const Common::String &name)
    : Dialog(name), _poolradPc(pc) {
}

void CharacterProfile::draw() {
    if (!_poolradPc)
        return;
	drawWindow(1, 1, 38, 22);

	drawIdentity();
    drawStats();
    drawValuables();
    drawLevelExp();
    drawCombat();
    drawItems();
    drawStatus();
	drawPortrait();
}

void CharacterProfile::handleMenuResult(bool success, Common::KeyCode key, short value) {
    // Forward any child dialog results (e.g., Yes/No prompt) to our parent view
    if (_parent)
        _parent->handleMenuResult(success, key, value);
}

// Clear and redraw only the stats area (rows 7..12, left block)
void CharacterProfile::redrawStats() {
    if (!_poolradPc)
        return;
    Surface s = getSurface();
    // Clear a conservative box covering labels and values incl. STR exceptional
    s.clearBox(1, 7, 10, 12, 0);
    drawStats();
}

// Clear and redraw only the valuables/currencies area (rows 7..14, mid-right block)
void CharacterProfile::redrawValuables() {
    if (!_poolradPc)
        return;
    Surface s = getSurface();
    // Names are aligned near column ~20 and values near 22
    // Clear a conservative region that won't hit level/exp at row 15
    s.clearBox(11, 7, 24, 14, 0);
    drawValuables();
}

// Clear and redraw only the combat area (rows 17..18, full width of content area)
void CharacterProfile::redrawCombat() {
    if (!_poolradPc)
        return;
    Surface s = getSurface();
    s.clearBox(1, 17, 38, 18, 0);
    drawCombat();
}

void CharacterProfile::drawIdentity() {
    Surface s = getSurface();
    s.writeStringC(_poolradPc->name, _poolradPc->getNameColor(), 1, 1);
    if (_poolradPc->npc < 0) {
        s.writeStringC("(NPC)", 14, 1 + _poolradPc->name.size() + 3, 1);
    }
	Common::String raceClass = VmInterface::getString(Common::String::format("stats.gender.%d", _poolradPc->gender));
    raceClass += " ";
    raceClass += VmInterface::getString(Common::String::format("stats.races.%d", _poolradPc->race));
    raceClass += " AGE ";
    raceClass += Common::String::format("%d", _poolradPc->age);
    s.writeStringC(raceClass, 15, 1, 3);
    s.writeStringC(VmInterface::getString(Common::String::format("stats.alignments.%d", _poolradPc->alignment)), 15, 1, 4);
    s.writeStringC(VmInterface::getString(Common::String::format("stats.classes.%d", _poolradPc->classType)), 15, 1, 5);
}

void CharacterProfile::drawStats() {
    Surface s = getSurface();
    s.writeStringC("STR", 10, 1, 7);
    s.writeStringC(Common::String::format("%d", _poolradPc->abilities.strength.current), 10, 5, 7);
    uint8 strEx = _poolradPc->abilities.strException.current;
    if (strEx > 0 && strEx <= 100) {
        Common::String strExStr;
        if (strEx == 100)
            strExStr = "(00)";
        else
            strExStr = Common::String::format("(%02d)", strEx);
        // Print right after the STR value
        s.writeStringC(strExStr, 10, 7, 7);
    }
    s.writeStringC("INT", 10, 1, 8);
    s.writeStringC(Common::String::format("%d", _poolradPc->abilities.intelligence.current), 10, 5, 8);
    s.writeStringC("WIS", 10, 1, 9);
    s.writeStringC(Common::String::format("%d", _poolradPc->abilities.wisdom.current), 10, 5, 9);
    s.writeStringC("DEX", 10, 1, 10);
    s.writeStringC(Common::String::format("%d", _poolradPc->abilities.dexterity.current), 10, 5, 10);
    s.writeStringC("CON", 10, 1, 11);
    s.writeStringC(Common::String::format("%d", _poolradPc->abilities.constitution.current), 10, 5, 11);
    s.writeStringC("CHA", 10, 1, 12);
    s.writeStringC(Common::String::format("%d", _poolradPc->abilities.charisma.current), 10, 5, 12);
}
void CharacterProfile::drawValuables() {
    Surface s = getSurface();
    int y = 7;
    int nameRAlign = 20;
    for (int i = Goldbox::Data::VALUABLE_COUNT - 1; i >= 0; --i) {
        uint16 count = _poolradPc->valuableItems[static_cast<Goldbox::Data::ValuableType>(i)];
        if (count != 0) {
            Common::String name = VmInterface::getString(Common::String::format("stats.currencies.%d", i));
            s.writeStringC(name, 10, nameRAlign-name.size(), y);
            s.writeStringC(Common::String::format("%d", count), 10, 22, y);
            y++;
        }
    }
}

void CharacterProfile::drawLevelExp() {
    const Goldbox::Data::LevelData &levels = _poolradPc->levels;
    Common::String levelStr;
    int classCount = 0;
    // Collect all nonzero levels
    for (int i = 0; i < 8; ++i) {
        int lvl = levels.levels[i];
        if (lvl > 0) {
            if (!levelStr.empty())
                levelStr += "/";
            levelStr += Common::String::format("%d", lvl);
            ++classCount;
        }
    }
    Surface s = getSurface();
    s.writeStringC("level " + levelStr, 15, 1, 15);

    s.writeStringC("Exp", 15, 17, 15);
    s.writeStringC(Common::String::format("%u", _poolradPc->experiencePoints), 15, 21, 15);
}

void CharacterProfile::drawCombat() {
    Surface s = getSurface();
    s.writeStringC("AC", 15, 1, 17);
    s.writeStringC(Common::String::format("%d", _poolradPc->armorClass.getCurrent()), 10, 4, 17);
    s.writeStringC("HP", 15, 1, 18);
    s.writeStringC(Common::String::format("%d", _poolradPc->hitPoints.current), 10, 4, 18);
    s.writeStringC("THAC0", 15, 9, 17);
    s.writeStringC(Common::String::format("%d", _poolradPc->thac0.getCurrent()), 10, 15, 17);
	s.writeStringC("Damage", 15, 8, 18);
    s.writeStringC(formatDamageText(), 10, 15, 18);
    s.writeStringC("Encumbrance", 15, 22, 17);
    s.writeStringC(Common::String::format("%d", _poolradPc->encumbrance), 10, 34, 17);
    s.writeStringC("Movement", 15, 25, 18);
    s.writeStringC(Common::String::format("%d", _poolradPc->movement.base), 10, 34, 18);
}

void CharacterProfile::drawItems() {
    Surface s = getSurface();
    s.writeStringC("Weapon", 15, 1, 20);
    s.writeStringC("-", 10, 8, 20);
    s.writeStringC("Armor", 15, 2, 21);
    s.writeStringC("-", 10, 8, 21);
}

void CharacterProfile::drawStatus() {
    Surface s = getSurface();
    s.writeStringC("Status", 15, 1, 22);
    s.writeStringC(VmInterface::getString(Common::String::format("stats.conditions.%d", _poolradPc->healthStatus)), 10, 8, 22);
}

void CharacterProfile::drawPortrait() {
    drawWindow(28, 1, 38, 11);
    Surface s = getSurface();

}

Common::String CharacterProfile::formatDamageText() const {
    if (!_poolradPc)
        return "";

    // Base "X d Y" format with optional modifier
    Common::String text = Common::String::format("%d", static_cast<int>(_poolradPc->curPrimaryRoll.action.roll.numDice)) +
                          "d" +
                          Common::String::format("%d", static_cast<int>(_poolradPc->curPrimaryRoll.action.roll.diceSides));

    if (_poolradPc->curPrimaryRoll.action.modifier != 0) {
        text += Common::String::format("%+d", static_cast<int>(_poolradPc->curPrimaryRoll.action.modifier));
    }

    return text;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox


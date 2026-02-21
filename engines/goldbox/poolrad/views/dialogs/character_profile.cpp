/* ScummVM - Graphic Adventure Engine
 *
 * CharacterProfile dialog for displaying character details
 */
#include "goldbox/poolrad/views/dialogs/character_profile.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/data/items/character_item.h"
#include "common/keyboard.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

// Head DAX block IDs indexed by portrait head ID (1-14)
const uint8 CharacterProfile::kHeadDaxBlockIds[14] = {
    0x00, 0x08, 0x09, 0x0D,
    0x10, 0x12, 0x16, 0x22,
    0x2D, 0x33, 0x35, 0x39,
    0x43, 0x44
};

// Body DAX block IDs indexed by portrait body ID (1-12)
const uint8 CharacterProfile::kBodyDaxBlockIds[12] = {
    0x01, 0x02, 0x03, 0x04,
    0x07, 0x08, 0x12, 0x05,
    0x1A, 0x21, 0x23, 0x25
};

using Common::String;

CharacterProfile::CharacterProfile(Goldbox::Poolrad::Data::PoolradCharacter *pc, const Common::String &name)
    : Dialog(name), _poolradPc(pc) {
    if (_poolradPc) {
    }
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
    if (_parent)
        _parent->handleMenuResult(success, key, value);
}

// Clear and redraw only the stats area (rows 7..12, left block)
void CharacterProfile::redrawStats() {
    if (!_poolradPc)
        return;
    Surface s = getSurface();
    s.clearBox(1, 7, 10, 12, 0);
    drawStats();
}

// Clear and redraw only the valuables/currencies area (rows 7..14, mid-right block)
void CharacterProfile::redrawValuables() {
    if (!_poolradPc)
        return;
    Surface s = getSurface();
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

// Clear and redraw only the name area (row 1, name and NPC indicator)
void CharacterProfile::redrawName() {
    if (!_poolradPc)
        return;
    Surface s = getSurface();
    s.clearBox(1, 1, 27, 1, 0);
    drawName();
}

void CharacterProfile::drawName() {
    Surface s = getSurface();
	s.writeStringC(1, 1, _poolradPc->getNameColor(), _poolradPc->name);
    if (_poolradPc->npc < 0) {
		s.writeStringC(1 + _poolradPc->name.size() + 3, 1, 14, "(NPC)");
    }
}

void CharacterProfile::drawIdentity() {
    Surface s = getSurface();
    drawName();
	String raceClass = VmInterface::getString(String::format("stats.gender.%d", _poolradPc->gender));
    raceClass += " ";
    raceClass += VmInterface::getString(String::format("stats.races.%d", _poolradPc->race));
    raceClass += " AGE ";
    raceClass += String::format("%d", _poolradPc->age);
    s.writeStringC(1, 3, 15, raceClass);
    s.writeStringC(1, 4, 15, VmInterface::getString(String::format("stats.alignments.%d", _poolradPc->alignment)));
    s.writeStringC(1, 5, 15, VmInterface::getString(String::format("stats.classes.%d", _poolradPc->classType)));
}

void CharacterProfile::drawStats() {
    Surface s = getSurface();
    s.writeStringC(1, 7, 10, "STR");
    s.writeStringC(5, 7, 10, String::format("%d", _poolradPc->abilities.strength.current));
    uint8 strEx = _poolradPc->abilities.strException.current;
    if (strEx > 0 && strEx <= 100) {
        String strExStr;
        if (strEx == 100)
            strExStr = "(00)";
        else
            strExStr = String::format("(%02d)", strEx);
        s.writeStringC(7, 7, 10, strExStr);
    }
    s.writeStringC(1, 8, 10, "INT");
    s.writeStringC(5, 8, 10, String::format("%d", _poolradPc->abilities.intelligence.current));
    s.writeStringC(1, 9, 10, "WIS");
    s.writeStringC(5, 9, 10, String::format("%d", _poolradPc->abilities.wisdom.current));
    s.writeStringC(1, 10, 10, "DEX");
    s.writeStringC(5, 10, 10, String::format("%d", _poolradPc->abilities.dexterity.current));
    s.writeStringC(1, 11, 10, "CON");
    s.writeStringC(5, 11, 10, String::format("%d", _poolradPc->abilities.constitution.current));
    s.writeStringC(1, 12, 10, "CHA");
    s.writeStringC(5, 12, 10, String::format("%d", _poolradPc->abilities.charisma.current));
}

void CharacterProfile::drawValuables() {
    Surface s = getSurface();
    int y = 7;
    int nameRAlign = 20;
    for (int i = Goldbox::Data::VALUABLE_COUNT - 1; i >= 0; --i) {
        uint16 count = _poolradPc->valuableItems[static_cast<Goldbox::Data::ValuableType>(i)];
        if (count != 0) {
            String name = VmInterface::getString(String::format("stats.currencies.%d", i));
            s.writeStringC(nameRAlign-name.size(), y, 10, name);
            s.writeStringC(22, y, 10, String::format("%d", count));
            y++;
        }
    }
}

void CharacterProfile::drawLevelExp() {
    const Goldbox::Data::LevelData &levels = _poolradPc->levels;
    String levelStr;
    // Collect all nonzero levels
    for (int i = 0; i < 8; ++i) {
        int lvl = levels.levels[i];
        if (lvl > 0) {
            if (!levelStr.empty())
                levelStr += "/";
            levelStr += String::format("%d", lvl);
        }
    }
    Surface s = getSurface();
    s.writeStringC(1, 15, 15, "Level");
    if (!levelStr.empty())
        s.writeStringC(7, 15, 15, levelStr);

    s.writeStringC(17, 15, 15, "Exp");
    s.writeStringC(21, 15, 15, String::format("%u", _poolradPc->experiencePoints));
}

void CharacterProfile::drawCombat() {
    Surface s = getSurface();
    s.writeStringC(1, 17, 15, "AC");
    s.writeStringC(4, 17, 10, String::format("%d", _poolradPc->armorClass.getCurrent()));
    s.writeStringC(1, 18, 15, "HP");
    s.writeStringC(4, 18, 10, String::format("%d", _poolradPc->hitPoints.current));
    s.writeStringC(9, 17, 15, "THAC0");
    s.writeStringC(15, 17, 10, String::format("%d", _poolradPc->thac0.getCurrent()));
	s.writeStringC(8, 18, 15, "Damage");
    s.writeStringC(15, 18, 10, formatDamageText());
    s.writeStringC(22, 17, 15, "Encumbrance");
    s.writeStringC(34, 17, 10, String::format("%d", _poolradPc->encumbrance));
    s.writeStringC(25, 18, 15, "Movement");
    s.writeStringC(34, 18, 10, String::format("%d", _poolradPc->movement.base));
}

void CharacterProfile::drawItems() {
    Surface s = getSurface();
    using Goldbox::Data::Items::Slot;
    const Goldbox::Data::Items::CharacterItem *weapon = _poolradPc->getEquippedItem(Slot::S_MAIN_HAND);
    if (weapon) {
        s.writeStringC(1, 20, 15, "Weapon");
        s.writeStringC(8, 20, 10, weapon->getDisplayName());
    }

    const Goldbox::Data::Items::CharacterItem *armor = _poolradPc->getEquippedItem(Slot::S_BODY_ARMOR);
    if (armor) {
        s.writeStringC(2, 21, 15, "Armor");
        s.writeStringC(8, 21, 10, armor->getDisplayName());
    }
}

void CharacterProfile::drawStatus() {
    Surface s = getSurface();
    s.writeStringC(1, 22, 15, "Status");
    s.writeStringC(8, 22, 10, VmInterface::getString(String::format("stats.conditions.%d", _poolradPc->healthStatus)));
}

void CharacterProfile::drawPortrait() {
    drawWindow(28, 1, 38, 11);
    Surface s = getSurface();
    if (!_poolradPc)
        return;

    // Load head portrait using the correct DAX block ID
    uint8 headBlockId = kHeadDaxBlockIds[_poolradPc->portrait.head - 1];
    _portraitDisplay.loadHead(headBlockId);

    // Load body portrait using the correct DAX block ID
    uint8 bodyBlockId = kBodyDaxBlockIds[_poolradPc->portrait.body - 1];
    _portraitDisplay.loadBody(bodyBlockId);

    // Render portraits at position (28, 1) within the frame
    _portraitDisplay.render(&s, 28, 1);
}

Common::String CharacterProfile::formatDamageText() const {
    if (!_poolradPc)
        return "";

    // Base "X d Y" format with optional modifier
	Common::String text = Common::String::format("%d", static_cast<int>(_poolradPc->curPrimaryRoll.action.roll.diceNum)) +
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


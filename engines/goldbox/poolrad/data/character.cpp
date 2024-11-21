#include "common/stream.h"
#include "goldbox/vm_interface.h"
#include "goldbox/poolrad/data/character.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

void Character::loadFrom(Common::SeekableReadStream &stream) {

    name = stream.readPascalString();

    stream.seek(0x10, SEEK_SET);
    strength = stream.readByte();
    intelligence = stream.readByte();
    wisdom = stream.readByte();
    dexterity = stream.readByte();
    constitution = stream.readByte();
    charisma = stream.readByte();
    ext_strength = stream.readByte();
    // Read memorized spells (Offset: 0x17, Length: 22 bytes)
    stream.read(memorized_spells, 22); 

    // Continue reading other fields in sequence
    thac0 = stream.readByte();
    race = stream.readByte();
    class_type = stream.readByte();
    age = stream.readUint16LE();
    hit_points = stream.readByte();

    // Seek to next block if there's a gap (Offset: 0x6B)
    stream.seek(0x6B);
    highest_level = stream.readByte();
    creature_size = stream.readByte();
    save_vs_paralysis = stream.readByte();
    save_vs_petrification = stream.readByte();
    save_vs_rod_staff_wand = stream.readByte();
    save_vs_breath_weapon = stream.readByte();
    save_vs_spell = stream.readByte();
    base_movement_speed = stream.readByte();
    hit_dice = stream.readByte();
    drained_levels = stream.readByte();
    drained_hp = stream.readByte();
    undead_resistance = stream.readByte();
    thief_skills_pick_pockets = stream.readByte();
    thief_skills_open_locks = stream.readByte();
    thief_skills_find_remove_traps = stream.readByte();
    thief_skills_move_silently = stream.readByte();
    thief_skills_hide_in_shadows = stream.readByte();
    thief_skills_hear_noise = stream.readByte();
    thief_skills_climb_walls = stream.readByte();
    thief_skills_read_languages = stream.readByte();

    // TODO: Change this to actual instance pointer
    // Read the 4-byte effects pointer (Offset: 0x7F)
    effects_ptr = stream.readUint32LE();

    // Continue reading fields in sequence
    unknown_byte_1 = stream.readByte();
    npc_status = stream.readByte();
    modified = stream.readByte();
    unknown_byte_2 = stream.readByte();
    unknown_byte_3 = stream.readByte();
    unknown_byte_4 = stream.readByte();
    copper = stream.readUint16LE();
    silver = stream.readUint16LE();
    electrum = stream.readUint16LE();
    gold = stream.readUint16LE();
    platinum = stream.readUint16LE();
    gems = stream.readUint16LE();
    jewels = stream.readUint16LE();
    cleric_levels = stream.readByte();
    druid_levels = stream.readByte();
    fighter_levels = stream.readByte();
    paladin_levels = stream.readByte();
    ranger_levels = stream.readByte();
    magic_user_levels = stream.readByte();
    thief_levels = stream.readByte();
    monk_levels = stream.readByte();
    gender = stream.readByte();
    monster_type = stream.readByte();
    alignment = stream.readByte();
    primary_attacks_x2 = stream.readByte();
    secondary_attacks_x2 = stream.readByte();
    pri_attack_damage_dice_number = stream.readByte();
    sec_attack_damage_dice_number = stream.readByte();
    pri_attack_damage_dice_sides = stream.readByte();
    sec_attack_damage_dice_sides = stream.readByte();
    pri_attack_damage_dice_modifier = stream.readByte();
    sec_attack_damage_dice_modifier = stream.readByte();
    armor_class = stream.readByte();
    strength_bonus_allowed = stream.readByte();
    combat_icon = stream.readByte();
    experience_points = stream.readUint32LE();
    class_item_usage_flags = stream.readByte();
    hit_points_rolled = stream.readByte();
    cleric_level1_spell_slots = stream.readByte();
    cleric_level2_spell_slots = stream.readByte();
    cleric_level3_spell_slots = stream.readByte();
    magic_user_level1_spell_slots = stream.readByte();
    magic_user_level2_spell_slots = stream.readByte();
    magic_user_level3_spell_slots = stream.readByte();
    base_xp_for_defeating_monster = stream.readUint16LE();
    bonus_xp_per_monster_hp = stream.readByte();
    head_portrait = stream.readByte();
    body_portrait = stream.readByte();
    head_icon = stream.readByte();
    weapon_icon = stream.readByte();
    unknown_byte_5 = stream.readByte();
    icon_size = stream.readByte();
    body_color = stream.readByte();
    arm_color = stream.readByte();
    leg_color = stream.readByte();
    head_color = stream.readByte();
    shield_color = stream.readByte();
    weapon_color = stream.readByte();
    spec_vulnerability_flags = stream.readByte();

    // Read items (Offset: 0xC8, Length: 56 bytes)
    stream.read(items, 56);

    // Continue reading remaining fields
    hands_used = stream.readByte();
    unknown_byte_6 = stream.readByte();
    encumbrance = stream.readUint16LE();
    unknown_byte_7 = stream.readByte();
    unknown_byte_8 = stream.readByte();
    unknown_byte_9 = stream.readByte();
    actions = stream.readUint32LE();
    unknown_byte_10 = stream.readByte();
    health_status = stream.readByte();
    in_combat = stream.readByte();
    side_in_combat = stream.readByte();
    quick_fight_flag = stream.readByte();
    thac0.set(stream.readByte());
    armor_class.set(stream.readByte());
    ac_for_rear_attacks = stream.readByte();
    pri_attacks_left = stream.readByte();
    sec_attacks_left = stream.readByte();
    current_pri_attack_damage_dice_number = stream.readByte();
    current_sec_attack_damage_dice_number = stream.readByte();
    current_pri_attack_damage_dice_sides = stream.readByte();
    current_sec_attack_damage_dice_sides = stream.readByte();
    current_pri_attack_bonus = stream.readByte();
    current_sec_attack_bonus = stream.readByte();
    hit_points.set(stream.readByte());
    current_movement = stream.readByte();
}


Character::Character() {
    // Initialize default values
    initialize();
}

void Character::initialize() {
    // Zero-initialize all fields
    *this = Character();

    // Set default values for certain fields
    // For example:
    // armor_class = 50;
    // thac0 = 40;
    // health_status = 0;
    // in_combat = 1;
    // head_portrait = 1;
    // body_portrait = 1;
    // creature_size = 1;
    // combat_icon = rand() % 256;

    // For simplicity, we will assume default initialization here
}

void Character::rollAbilityScores() {
    strength = static_cast<uint8>(VmInterface::rollDice(3, 6));
    intelligence = static_cast<uint8>(VmInterface::rollDice(3, 6));
    wisdom = static_cast<uint8>(VmInterface::rollDice(3, 6));
    dexterity = static_cast<uint8>(VmInterface::rollDice(3, 6));
    constitution = static_cast<uint8>(VmInterface::rollDice(3, 6));
    charisma = static_cast<uint8>(VmInterface::rollDice(3, 6));
}

void Character::applyRacialAdjustments() {
    switch (race) {
        case 0: // Human
            // No adjustments
            break;
        case 1: // Dwarf
            adjustAbilityForRace(constitution, +1, 3, 18);
            adjustAbilityForRace(charisma, -1, 3, 18);
            break;
        case 2: // Elf
            adjustAbilityForRace(dexterity, +1, 3, 18);
            adjustAbilityForRace(constitution, -1, 3, 18);
            break;
        case 3: // Gnome
            adjustAbilityForRace(intelligence, +1, 3, 18);
            adjustAbilityForRace(wisdom, -1, 3, 18);
            break;
        case 4: // Halfling
            adjustAbilityForRace(dexterity, +1, 3, 18);
            adjustAbilityForRace(strength, -1, 3, 18);
            break;
        case 5: // Half-Elf
            // No adjustments
            break;
        default:
            // Unknown race, no adjustments
            break;
    }
}

void Character::adjustAbilityForRace(Stat &ability, int adjustment, int minValue, int maxValue) {
    int newAbility = ability + adjustment;
    newAbility = std::clamp(newAbility, minValue, maxValue);
    ability = static_cast<uint8>(newAbility);
}

bool Character::meetsClassRequirements() const {
    // Implement class requirements checks
    // Example: Fighter requires Strength >= 9
    switch (class_type) {
        case 1: // Fighter
            return strength >= 9;
        case 2: // Ranger
            return strength >= 13 && dexterity >= 13 && wisdom >= 14;
        // Add other class requirements
        default:
            return true;
    }
}

void Character::calculateHitPoints() {
    int hitDie = 0;
    int totalLevels = 0;

    if (cleric_levels > 0) {
        hitDie += cleric_levels * 8;
        totalLevels += cleric_levels;
    }
    if (fighter_levels > 0) {
        hitDie += fighter_levels * 10;
        totalLevels += fighter_levels;
    }
    if (ranger_levels > 0) {
        hitDie += ranger_levels * 8;
        totalLevels += ranger_levels;
    }
    if (magic_user_levels > 0) {
        hitDie += magic_user_levels * 4;
        totalLevels += magic_user_levels;
    }
    if (thief_levels > 0) {
        hitDie += thief_levels * 6;
        totalLevels += thief_levels;
    }

    int averageHitDie = hitDie / totalLevels;

    // Constitution modifier
    int conModifier = 0;
    if (constitution >= 15) conModifier = 1;
    if (constitution >= 17) conModifier = 2;

    hit_points = static_cast<uint8>(VmInterface::rollDice(1, averageHitDie) + conModifier);
    if (hit_points < 1) {
        hit_points = 1;
    }
}

void Character::finalizeName() {
    // Replace spaces with -1 (or another placeholder)
    std::replace(name.begin(), name.end(), ' ', static_cast<char>(-1));
}

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox

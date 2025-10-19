
// Cleaned up version of VIEW_Items from Ghidra
// Handles item menu display and interaction for the currently selected character

#include <cstdint>
#include <cstring>
#include "GameEngine.h"  // assumed external functions and data

// View_Items strings from segment 0x4886 (mapped from 0x1af8)
static const char* kStrReady       = "Ready";
static const char* kStrUse         = " Use";
static const char* kStrTrade       = " Trade";
static const char* kStrDrop        = " Drop";
static const char* kStrHalve       = " Halve";
static const char* kStrJoin        = " Join";
static const char* kStrSell        = " Sell";
static const char* kStrId          = " Id";
static const char* kStrItems       = "Items";
static const char* kStrReadyItem   = "Ready Item";
static const char* kStrMustReady   = "Must be Readied";
static const char* kStrYour        = "Your ";
static const char* kStrGoneForever = "will be gone forever";
static const char* kStrDropPrompt  = "Drop It? ";

void VIEW_Items(bool* flag_ready) {
    Character* character = (Character*)((uintptr_t(CHAR_PTR_SEG) << 4) + uintptr_t(CHAR_PTR_OFFS));
    uint8_t menu_key = 0x20;
    uint8_t item_count_before;
    bool needs_redraw = true;

    if (character->number_of_items == 0) return;

    while (MemberOf((uint8_t*)0x1c7c0e63, menu_key) && !*flag_ready) {
        item_count_before = character->number_of_items;

        if (character->items_address.ptr != nullptr) {
            char displayBuffer[41] = {};
            char tempBuffer[240] = {};

            // Various conditions to append strings
            if (character->enabled && MemBuff2k_1->combat_state == 0 &&
                (BYTE_STATE == 0x02 || BYTE_STATE == 0x03 || BYTE_STATE == 0x04 ||
                 (BYTE_STATE == 0x05 && character->combat_address[2] != 0))) {
                strcat(tempBuffer, kStrUse);
                CopyStr(40, displayBuffer, tempBuffer);
            }

            if ((character->npc < 0x80 || !character->enabled || character->status == STATUS_ANIMATED) && BYTE_STATE != 0x05) {
                strcat(tempBuffer, kStrTrade);
                CopyStr(40, displayBuffer, tempBuffer);
            }

            strcat(tempBuffer, kStrDrop);
            CopyStr(40, displayBuffer, tempBuffer);

            if (character->number_of_items < 16) {
                strcat(tempBuffer, kStrHalve);
                CopyStr(40, displayBuffer, tempBuffer);
            }

            strcat(tempBuffer, kStrJoin);
            CopyStr(40, displayBuffer, tempBuffer);

            if ((character->npc < 0x80 || !character->enabled || character->status == STATUS_ANIMATED) && BYTE_STATE == 0x01) {
                strcat(tempBuffer, kStrSell);
                CopyStr(40, displayBuffer, tempBuffer);
            }

            if (BYTE_STATE == 0x01) {
                strcat(tempBuffer, kStrId);
                CopyStr(40, displayBuffer, tempBuffer);
            }

            // Show all items using a linked list traversal
            Item* item = character->items_address.ptr;
            while (item != nullptr) {
                FUN_10ba_0442(0, 1, 0, 0, item);
                item = item->next;
            }

            if (needs_redraw || BYTE_1c7c_6821) {
                Screen_Clear();
                Screen_DrawSymbolFrame(0, 0x16, 0x26, 1, 1);
                TXT_CharNameToScreen(1, 1, 1, character);

                Screen_StringToScreen((uint8_t*)kStrItems, 10, 1, character->name_length + 4);

                Screen_DrawWindow((uint8_t*)":", 0, 0, 0x16, 0x26, 3, 1);
                Screen_StringToScreen((uint8_t*)kStrReady, 0xf, 3, 1);

                needs_redraw = false;
                BYTE_1c7c_6821 = 0;
            }

            // Input
            menu_key = Input_VerticalMenu(
                &character->items_address, nullptr, &needs_redraw, 1,
                character->items_address.ptr, 0x16, 0x26, 5, 1, 0xf, 10, 0xd,
                (uint8_t*)displayBuffer, (uint8_t*)":"
            );

            if (character->items_address.ptr) {
                switch (menu_key) {
                    case 'R':
                        CharItemEquip(character->items_address.ptr);
                        break;
                    case 'U':
                        if (!character->items_address.ptr->usable_flag) {
                            FUN_10ba_1693((uint8_t*)kStrMustReady);
                        } else {
                            if (thunk_FUN_4e45_323d(character->items_address.ptr) ||
                                character->items_address.ptr->type < 0x80) {
                                CharItemUse(flag_ready, character->items_address.ptr);
                                if (BYTE_STATE != 0x05) *flag_ready = false;
                                if (!*flag_ready) needs_redraw = true;
                            }
                        }
                        break;
                    case 'T':
                        if (CharReadScroll(character->items_address.ptr)) {
                            CharItemTrade(character->items_address.ptr);
                            needs_redraw = true;
                        }
                        break;
                    case 'D':
                        if (CharReadScroll(character->items_address.ptr)) {
                            FUN_10ba_0442(0, 0, 0, 0, character->items_address.ptr);
                            char dropMsg[240] = {};
                            strcpy(dropMsg, kStrYour);
                            strcat(dropMsg, character->items_address.ptr->name);
                            strcat(dropMsg, kStrGoneForever);
                            FUN_1709_0768(dropMsg, 1, 0x16, 0x26, 0x15, 1);
                            if (INPUT_YesNo(0xf, 10, 0xd, (uint8_t*)kStrDropPrompt) == 'Y') {
                                FUN_10ba_1509(character->items_address.ptr, character);
                                needs_redraw = true;
                            }
                            Screen_ClearArea(0x16, 0x26, 0x15, 1);
                        }
                        break;
                    case 'H':
                        ItemShare(character->items_address.ptr);
                        break;
                    case 'J':
                        FUN_4886_189d(character->items_address.ptr);
                        break;
                    case 'S':
                        if (CharReadScroll(character->items_address.ptr)) {
                            ItemSell(character->items_address.ptr);
                        }
                        break;
                    case 'I':
                        ItemIdentify(&needs_redraw, character->items_address.ptr);
                        break;
                }
            }

            FUN_10ba_0bb8((uint32_t)character);
        }

        if (character->number_of_items != item_count_before) {
            needs_redraw = true;
        }
    }
}

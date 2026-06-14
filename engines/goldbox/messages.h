/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GOLDBOX_MESSAGES_H
#define GOLDBOX_MESSAGES_H

#include "common/array.h"
#include "common/events.h"
#include "common/str.h"

namespace Goldbox {

class UIElement;

struct Message {};

struct FocusMessage : public Message {
	UIElement *_priorView = nullptr;
	FocusMessage() : Message() {}
	FocusMessage(UIElement *priorView) : Message(),
		_priorView(priorView) {}
};

struct UnfocusMessage : public Message {};
struct MouseEnterMessage : public Message {};
struct MouseLeaveMessage : public Message {};

struct KeypressMessage : public Message, public Common::KeyState {
	KeypressMessage() : Message() {}
	KeypressMessage(const Common::KeyState &ks) :
		Message(), Common::KeyState(ks) {}
};

struct MouseMessage : public Message {
	enum Button { MB_LEFT, MB_RIGHT, MB_MIDDLE };
	Button _button;
	Common::Point _pos;

	MouseMessage() : Message(), _button(MB_LEFT) {}
	MouseMessage(Button btn, const Common::Point &pos) :
		Message(), _button(btn), _pos(pos) {}
	MouseMessage(Common::EventType type, const Common::Point &pos);
};
struct MouseDownMessage : public MouseMessage {
	MouseDownMessage() : MouseMessage() {}
	MouseDownMessage(Button btn, const Common::Point &pos) :
		MouseMessage(btn, pos) {}
	MouseDownMessage(Common::EventType type, const Common::Point &pos) :
		MouseMessage(type, pos) {}
};
struct MouseUpMessage : public MouseMessage {
	MouseUpMessage() : MouseMessage() {}
	MouseUpMessage(Button btn, const Common::Point &pos) :
		MouseMessage(btn, pos) {}
	MouseUpMessage(Common::EventType type, const Common::Point &pos) :
		MouseMessage(type, pos) {}
};
typedef MouseMessage MouseMoveMessage;

struct GameMessage : public Message {
	Common::String _name;
	int _value;
	Common::String _stringValue;

	GameMessage() : Message(), _value(-1) {}
	GameMessage(const Common::String &name) : Message(),
		_name(name), _value(-1) {}
	GameMessage(const Common::String &name, int value) : Message(),
		_name(name), _value(value) {}
	GameMessage(const Common::String &name, const Common::String &value) :
		Message(), _name(name), _stringValue(value) {}
};

struct ValueMessage : public Message {
	int _value;

	ValueMessage() : Message(), _value(0) {}
	ValueMessage(int value) : Message(),
		_value(value) {}
};

struct ActionMessage : public Message {
	int _action;
	ActionMessage() : Message(), _action(0) {
	}
	ActionMessage(int action) : Message(),
		_action(action) {
	}
};

struct MenuResultMessage : public Message {
	Common::String _targetViewName;
	bool _success;
	Common::KeyCode _keyCode;
	int _intValue;
	Common::String _stringValue;
	bool _hasIntValue;
	bool _hasStringValue;

	MenuResultMessage() : Message(),
		_success(false),
		_keyCode(Common::KEYCODE_INVALID),
		_intValue(0),
		_hasIntValue(false),
		_hasStringValue(false) {
	}

	MenuResultMessage(const Common::String &targetViewName,
			bool success,
			Common::KeyCode keyCode) : Message(),
		_targetViewName(targetViewName),
		_success(success),
		_keyCode(keyCode),
		_intValue(0),
		_hasIntValue(false),
		_hasStringValue(false) {
	}

	MenuResultMessage(const Common::String &targetViewName,
			bool success,
			Common::KeyCode keyCode,
			int intValue) : Message(),
		_targetViewName(targetViewName),
		_success(success),
		_keyCode(keyCode),
		_intValue(intValue),
		_hasIntValue(true),
		_hasStringValue(false) {
	}

	MenuResultMessage(const Common::String &targetViewName,
			bool success,
			Common::KeyCode keyCode,
			const Common::String &stringValue) : Message(),
		_targetViewName(targetViewName),
		_success(success),
		_keyCode(keyCode),
		_intValue(0),
		_stringValue(stringValue),
		_hasIntValue(false),
		_hasStringValue(true) {
	}

	MenuResultMessage(const Common::String &targetViewName,
			bool success,
			Common::KeyCode keyCode,
			int intValue,
			const Common::String &stringValue) : Message(),
		_targetViewName(targetViewName),
		_success(success),
		_keyCode(keyCode),
		_intValue(intValue),
		_stringValue(stringValue),
		_hasIntValue(true),
		_hasStringValue(true) {
	}
};

/**
 * Message posted by an ECL opcode handler to notify views/dialogs that a
 * VM memory address was written.  Consumers (e.g. InGameView) can react to
 * specific addresses without polling VM memory every frame.
 *
 * The value is stored as a raw uint16 together with a ValueType tag so that
 * both 8-bit and 16-bit, signed and unsigned writes can be represented.
 * Use the typed accessors (asUint8, asInt8, asUint16, asInt16) rather than
 * inspecting _rawValue directly.
 */
struct EclVmMessage : public Message {
	enum MessageKind {
		MK_MEMORY_WRITE = 0,
		MK_OPCODE,
		MK_SYSCALL,
		MK_STATE
	};

	enum ValueType {
		VT_UINT8 = 0,
		VT_INT8,
		VT_UINT16,
		VT_INT16
	};

	enum OpcodePhase {
		OP_ENTER = 0,
		OP_EXIT
	};

	// Canonical tags for MK_SYSCALL events.
	enum SyscallTag {
		SC_PRINT = 1,
		SC_INPUT_NUMBER,
		SC_INPUT_STRING,
		SC_DISPLAY_PICTURE,
		SC_VERTICAL_MENU,
		SC_HORIZONTAL_MENU,
		SC_START_COMBAT,
		SC_EXECUTE_PROGRAM,
		SC_CLEAR_TEXTBOX,
		SC_LOAD_SCRIPT,
		SC_LOAD_GEO,
		SC_LOAD_WALLSET,
		SC_LOAD_ICON,
		SC_MAP_DATA_READY,
		SC_SPRITE_OFF,
		SC_DELAY,
		SC_PRINT_ASYNC
	};

	// Canonical tags for MK_STATE events.
	enum StateTag {
		ST_GAME_STATE = 1,
		ST_MAP_MODE,
		ST_MENU_MODE,
		ST_COMBAT_MODE,
		ST_SCREEN_REFRESH,
		ST_POSITION_DIRTY,
		ST_SKYBOX_DIRTY,
		ST_CHARACTER_DIRTY,
		ST_STATUS_DIRTY,
		ST_INGAME_MENU_VISIBLE,
		ST_ENTER_SHOP
	};

	MessageKind _kind;
	// Generic routing/context fields for non-memory events.
	uint16 _pc;
	uint8 _opcode;
	OpcodePhase _phase;
	int16 _result;
	uint16 _tag;

	// Memory-write payload.
	uint16 _address;
	uint16 _rawValue;
	ValueType _valueType;

	EclVmMessage() : Message(),
		_kind(MK_MEMORY_WRITE),
		_pc(0),
		_opcode(0),
		_phase(OP_EXIT),
		_result(0),
		_tag(0),
		_address(0),
		_rawValue(0),
		_valueType(VT_UINT8) {
	}

	EclVmMessage(uint16 address, uint8 value) : Message(),
		_kind(MK_MEMORY_WRITE),
		_pc(0),
		_opcode(0),
		_phase(OP_EXIT),
		_result(0),
		_tag(0),
		_address(address),
		_rawValue(static_cast<uint16>(value)),
		_valueType(VT_UINT8) {
	}

	EclVmMessage(uint16 address, int8 value) : Message(),
		_kind(MK_MEMORY_WRITE),
		_pc(0),
		_opcode(0),
		_phase(OP_EXIT),
		_result(0),
		_tag(0),
		_address(address),
		_rawValue(static_cast<uint16>(static_cast<uint8>(value))),
		_valueType(VT_INT8) {
	}

	EclVmMessage(uint16 address, uint16 value) : Message(),
		_kind(MK_MEMORY_WRITE),
		_pc(0),
		_opcode(0),
		_phase(OP_EXIT),
		_result(0),
		_tag(0),
		_address(address),
		_rawValue(value),
		_valueType(VT_UINT16) {
	}

	EclVmMessage(uint16 address, int16 value) : Message(),
		_kind(MK_MEMORY_WRITE),
		_pc(0),
		_opcode(0),
		_phase(OP_EXIT),
		_result(0),
		_tag(0),
		_address(address),
		_rawValue(static_cast<uint16>(value)),
		_valueType(VT_INT16) {
	}

	static EclVmMessage makeOpcode(uint16 pc, uint8 opcode,
			OpcodePhase phase, int16 result = 0) {
		EclVmMessage msg;
		msg._kind = MK_OPCODE;
		msg._pc = pc;
		msg._opcode = opcode;
		msg._phase = phase;
		msg._result = result;
		return msg;
	}

	static EclVmMessage makeSyscall(uint16 pc, uint8 opcode,
			uint16 syscallTag, int16 result = 0) {
		EclVmMessage msg;
		msg._kind = MK_SYSCALL;
		msg._pc = pc;
		msg._opcode = opcode;
		msg._phase = OP_EXIT;
		msg._tag = syscallTag;
		msg._result = result;
		return msg;
	}

	static EclVmMessage makeSyscall(uint16 pc, uint8 opcode,
			SyscallTag syscallTag, int16 result = 0) {
		return makeSyscall(pc, opcode, static_cast<uint16>(syscallTag),
			result);
	}

	static EclVmMessage makeState(uint16 stateTag, uint16 rawValue,
			ValueType valueType = VT_UINT16) {
		EclVmMessage msg;
		msg._kind = MK_STATE;
		msg._tag = stateTag;
		msg._rawValue = rawValue;
		msg._valueType = valueType;
		return msg;
	}

	static EclVmMessage makeState(StateTag stateTag, uint16 rawValue,
			ValueType valueType = VT_UINT16) {
		return makeState(static_cast<uint16>(stateTag), rawValue, valueType);
	}

	// Optional string payload for messages that carry text data.
	Common::String _stringPayload;

	static EclVmMessage makeSyscallWithText(uint16 pc, uint8 opcode,
			SyscallTag syscallTag, const Common::String &text,
			int16 result = 0) {
		EclVmMessage msg = makeSyscall(pc, opcode, syscallTag, result);
		msg._stringPayload = text;
		return msg;
	}

	uint8 asUint8() const { return static_cast<uint8>(_rawValue & 0xFFu); }
	int8 asInt8() const { return static_cast<int8>(_rawValue & 0xFFu); }
	uint16 asUint16() const { return _rawValue; }
	int16 asInt16() const { return static_cast<int16>(_rawValue); }

	bool is8Bit() const { return _valueType == VT_UINT8 || _valueType == VT_INT8; }
	bool isSigned() const { return _valueType == VT_INT8 || _valueType == VT_INT16; }
};

} // namespace Goldbox

#endif

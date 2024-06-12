#include "alisvm.h"

namespace Alis {

AlisVM::AlisVM() : _memoryStream(nullptr, 0), _registers(16) {
    reset();
}

AlisVM::~AlisVM() {
    // Clean up resources if needed
}

void AlisVM::reset() {
    _memoryStream.seek(0);
    _registers.clear();
    _registers.resize(16);
}

void AlisVM::loadProgram(const byte *data, uint32 size) {
    _memoryStream = Common::MemoryReadStream(data, size);
}

void AlisVM::execute() {
    while (!_memoryStream.eos()) {
        fetch();
        decode();
        executeInstruction();
    }
}

void AlisVM::fetch() {
    // Fetch instruction from the memory stream
}

void AlisVM::decode() {
    // Decode the fetched instruction
}

void AlisVM::executeInstruction() {
    // Execute the decoded instruction
}

} // End of namespace Alis

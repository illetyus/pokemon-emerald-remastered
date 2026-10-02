#ifndef REMASTER_CORE_BRIDGE_HPP
#define REMASTER_CORE_BRIDGE_HPP

#include "remaster/core.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {

class RemasterCoreBridge : public RefCounted {
    GDCLASS(RemasterCoreBridge, RefCounted)

private:
    RemasterState state;

protected:
    static void _bind_methods();

public:
    RemasterCoreBridge();

    void reset();
    void step(const StringName &action);
    Dictionary snapshot() const;
    PackedByteArray save_state() const;
    bool load_state(const PackedByteArray &data);
    int64_t state_hash() const;
};

} // namespace godot

#endif

#include "remaster/emerald_script_runtime.h"

#include <string.h>

enum {
    CHECKPOINT_SIZE = 298
};

static const uint8_t kCheckpointMagic[4] = {
    (uint8_t)'R',
    (uint8_t)'2',
    (uint8_t)'S',
    (uint8_t)'C'
};

static void write_u8(uint8_t **cursor, uint8_t value)
{
    *(*cursor)++ = value;
}

static void write_u16(uint8_t **cursor, uint16_t value)
{
    uint8_t *p = *cursor;
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    *cursor += 2;
}

static void write_i16(uint8_t **cursor, int16_t value)
{
    write_u16(cursor, (uint16_t)value);
}

static void write_u32(uint8_t **cursor, uint32_t value)
{
    uint8_t *p = *cursor;
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
    *cursor += 4;
}

static void write_u64(uint8_t **cursor, uint64_t value)
{
    write_u32(cursor, (uint32_t)value);
    write_u32(cursor, (uint32_t)(value >> 32u));
}

static uint8_t read_u8(const uint8_t **cursor)
{
    return *(*cursor)++;
}

static uint16_t read_u16(const uint8_t **cursor)
{
    const uint8_t *p = *cursor;
    uint16_t value =
        (uint16_t)p[0]
        | (uint16_t)((uint16_t)p[1] << 8u);
    *cursor += 2;
    return value;
}

static int16_t read_i16(const uint8_t **cursor)
{
    return (int16_t)read_u16(cursor);
}

static uint32_t read_u32(const uint8_t **cursor)
{
    const uint8_t *p = *cursor;
    uint32_t value =
        (uint32_t)p[0]
        | ((uint32_t)p[1] << 8u)
        | ((uint32_t)p[2] << 16u)
        | ((uint32_t)p[3] << 24u);
    *cursor += 4;
    return value;
}

static uint64_t read_u64(const uint8_t **cursor)
{
    uint64_t low = read_u32(cursor);
    uint64_t high = read_u32(cursor);
    return low | (high << 32u);
}

static int program_pc_valid(
    const RemasterEmeraldScriptRegistry *registry,
    uint32_t program_index,
    uint32_t pc,
    int allow_end)
{
    const RemasterEmeraldScriptProgram *program;

    if (registry == 0
        || registry->programs == 0
        || program_index >= registry->program_count)
    {
        return 0;
    }

    program = &registry->programs[program_index];
    if (program->instructions == 0 || program->instruction_count == 0u)
        return 0;

    if (allow_end)
        return pc <= program->instruction_count;

    return pc < program->instruction_count;
}

static int runtime_checkpoint_valid(
    const RemasterEmeraldScriptRuntime *runtime)
{
    size_t i;

    if (runtime == 0 || runtime->vm.registry == 0)
        return 0;

    if (runtime->vm.status > REMASTER_EMERALD_SCRIPT_STEP_LIMIT)
        return 0;

    if (!program_pc_valid(
            runtime->vm.registry,
            runtime->vm.program_index,
            runtime->vm.pc,
            1))
    {
        return 0;
    }

    if (runtime->vm.stack_depth > REMASTER_EMERALD_SCRIPT_STACK_DEPTH)
        return 0;

    for (i = 0; i < runtime->vm.stack_depth; ++i) {
        if (!program_pc_valid(
                runtime->vm.registry,
                runtime->vm.stack[i].program_index,
                runtime->vm.stack[i].return_pc,
                1))
        {
            return 0;
        }
    }

    if (runtime->has_pending_request) {
        if (runtime->pending_request.type
                <= REMASTER_EMERALD_SCRIPT_REQUEST_NONE
            || runtime->pending_request.type
                > REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL
            || !program_pc_valid(
                runtime->vm.registry,
                runtime->pending_request.program_index,
                runtime->pending_request.pc,
                0))
        {
            return 0;
        }
    }

    return 1;
}

size_t remaster_emerald_script_runtime_checkpoint_size(void)
{
    return CHECKPOINT_SIZE;
}

int remaster_emerald_script_runtime_checkpoint_write(
    const RemasterEmeraldScriptRuntime *runtime,
    void *dst,
    size_t dst_size)
{
    uint8_t *cursor;
    size_t i;

    if (dst == 0
        || dst_size != CHECKPOINT_SIZE
        || !runtime_checkpoint_valid(runtime))
    {
        return 0;
    }

    cursor = (uint8_t *)dst;
    memcpy(cursor, kCheckpointMagic, sizeof(kCheckpointMagic));
    cursor += sizeof(kCheckpointMagic);

    write_u16(&cursor, REMASTER_EMERALD_SCRIPT_CHECKPOINT_VERSION);
    write_u8(&cursor, (uint8_t)runtime->vm.status);
    write_u8(&cursor, runtime->vm.stack_depth);
    write_u8(&cursor, runtime->vm.comparison_result);
    write_u8(&cursor, runtime->has_pending_request ? 1u : 0u);
    write_u32(&cursor, runtime->vm.program_index);
    write_u32(&cursor, runtime->vm.pc);
    write_u64(&cursor, runtime->next_request_sequence);

    write_u16(&cursor, (uint16_t)runtime->vm.error.code);
    write_u16(&cursor, (uint16_t)runtime->vm.error.opcode);
    write_u32(&cursor, runtime->vm.error.program_index);
    write_u32(&cursor, runtime->vm.error.pc);

    for (i = 0; i < REMASTER_EMERALD_SPECIAL_VAR_COUNT; ++i)
        write_u16(&cursor, runtime->vm.special_vars[i]);

    for (i = 0; i < REMASTER_EMERALD_SPECIAL_FLAG_BYTES; ++i)
        write_u8(&cursor, runtime->vm.special_flags[i]);

    for (i = 0; i < REMASTER_EMERALD_SCRIPT_STACK_DEPTH; ++i) {
        write_u32(&cursor, runtime->vm.stack[i].program_index);
        write_u32(&cursor, runtime->vm.stack[i].return_pc);
    }

    write_u16(&cursor, (uint16_t)runtime->pending_request.type);
    write_u16(&cursor, runtime->pending_request.action);
    write_u64(&cursor, runtime->pending_request.sequence);
    write_u32(&cursor, runtime->pending_request.program_index);
    write_u32(&cursor, runtime->pending_request.pc);
    write_u16(&cursor, runtime->pending_request.local_id);
    write_u16(&cursor, runtime->pending_request.result_var);
    write_u32(&cursor, runtime->pending_request.map_id);
    write_i16(&cursor, runtime->pending_request.x);
    write_i16(&cursor, runtime->pending_request.y);
    write_u16(&cursor, runtime->pending_request.value_u16);
    write_u16(&cursor, runtime->pending_request.quantity);
    write_u32(&cursor, runtime->pending_request.value_u32);

    return (size_t)(cursor - (uint8_t *)dst) == CHECKPOINT_SIZE;
}

int remaster_emerald_script_runtime_checkpoint_read(
    RemasterEmeraldScriptRuntime *runtime,
    const RemasterEmeraldScriptRegistry *registry,
    RemasterEmeraldSave *save,
    const void *src,
    size_t src_size)
{
    const uint8_t *cursor;
    RemasterEmeraldScriptRuntime temp;
    const RemasterEmeraldSpecialRegistry *special_registry;
    RemasterEmeraldScriptStatus status;
    uint8_t stack_depth;
    uint8_t comparison_result;
    uint8_t has_pending;
    uint32_t program_index;
    uint32_t pc;
    uint64_t next_request_sequence;
    uint16_t error_code;
    uint16_t error_opcode;
    uint32_t error_program_index;
    uint32_t error_pc;
    RemasterEmeraldScriptCallFrame stack[REMASTER_EMERALD_SCRIPT_STACK_DEPTH];
    uint16_t special_vars[REMASTER_EMERALD_SPECIAL_VAR_COUNT];
    uint8_t special_flags[REMASTER_EMERALD_SPECIAL_FLAG_BYTES];
    RemasterEmeraldScriptRequest pending;
    size_t i;

    if (runtime == 0
        || registry == 0
        || save == 0
        || src == 0
        || src_size != CHECKPOINT_SIZE)
    {
        return 0;
    }

    cursor = (const uint8_t *)src;
    if (memcmp(cursor, kCheckpointMagic, sizeof(kCheckpointMagic)) != 0)
        return 0;
    cursor += sizeof(kCheckpointMagic);

    if (read_u16(&cursor) != REMASTER_EMERALD_SCRIPT_CHECKPOINT_VERSION)
        return 0;

    status = (RemasterEmeraldScriptStatus)read_u8(&cursor);
    stack_depth = read_u8(&cursor);
    comparison_result = read_u8(&cursor);
    has_pending = read_u8(&cursor);
    program_index = read_u32(&cursor);
    pc = read_u32(&cursor);
    next_request_sequence = read_u64(&cursor);

    error_code = read_u16(&cursor);
    error_opcode = read_u16(&cursor);
    error_program_index = read_u32(&cursor);
    error_pc = read_u32(&cursor);

    for (i = 0; i < REMASTER_EMERALD_SPECIAL_VAR_COUNT; ++i)
        special_vars[i] = read_u16(&cursor);

    for (i = 0; i < REMASTER_EMERALD_SPECIAL_FLAG_BYTES; ++i)
        special_flags[i] = read_u8(&cursor);

    for (i = 0; i < REMASTER_EMERALD_SCRIPT_STACK_DEPTH; ++i) {
        stack[i].program_index = read_u32(&cursor);
        stack[i].return_pc = read_u32(&cursor);
    }

    memset(&pending, 0, sizeof(pending));
    pending.type = (RemasterEmeraldScriptRequestType)read_u16(&cursor);
    pending.action = read_u16(&cursor);
    pending.sequence = read_u64(&cursor);
    pending.program_index = read_u32(&cursor);
    pending.pc = read_u32(&cursor);
    pending.local_id = read_u16(&cursor);
    pending.result_var = read_u16(&cursor);
    pending.map_id = read_u32(&cursor);
    pending.x = read_i16(&cursor);
    pending.y = read_i16(&cursor);
    pending.value_u16 = read_u16(&cursor);
    pending.quantity = read_u16(&cursor);
    pending.value_u32 = read_u32(&cursor);

    if ((size_t)(cursor - (const uint8_t *)src) != CHECKPOINT_SIZE
        || status > REMASTER_EMERALD_SCRIPT_STEP_LIMIT
        || stack_depth > REMASTER_EMERALD_SCRIPT_STACK_DEPTH
        || has_pending > 1u
        || next_request_sequence == 0u
        || error_code > REMASTER_EMERALD_SCRIPT_ERROR_UNKNOWN_SPECIAL
        || error_opcode > REMASTER_EMERALD_SCRIPT_SPECIAL_VAR
        || !program_pc_valid(registry, program_index, pc, 1))
    {
        return 0;
    }

    for (i = 0; i < stack_depth; ++i) {
        if (!program_pc_valid(
                registry,
                stack[i].program_index,
                stack[i].return_pc,
                1))
        {
            return 0;
        }
    }

    if (has_pending) {
        if (status != REMASTER_EMERALD_SCRIPT_YIELDED
            || pending.type <= REMASTER_EMERALD_SCRIPT_REQUEST_NONE
            || pending.type > REMASTER_EMERALD_SCRIPT_REQUEST_SPECIAL
            || !program_pc_valid(
                registry,
                pending.program_index,
                pending.pc,
                0))
        {
            return 0;
        }
    }

    special_registry = runtime->special_registry;
    memset(&temp, 0, sizeof(temp));
    remaster_emerald_script_init_program(
        &temp.vm,
        save,
        registry,
        program_index,
        0);
    if (temp.vm.status == REMASTER_EMERALD_SCRIPT_ERROR)
        return 0;

    temp.special_registry = special_registry;
    temp.next_request_sequence = next_request_sequence;
    temp.vm.pc = pc;
    temp.vm.stack_depth = stack_depth;
    temp.vm.comparison_result = comparison_result;
    memcpy(temp.vm.stack, stack, sizeof(stack));
    memcpy(temp.vm.special_vars, special_vars, sizeof(special_vars));
    memcpy(temp.vm.special_flags, special_flags, sizeof(special_flags));

    temp.vm.error.code = (RemasterEmeraldScriptErrorCode)error_code;
    temp.vm.error.program_index = error_program_index;
    temp.vm.error.pc = error_pc;
    temp.vm.error.opcode = (RemasterEmeraldScriptOpcode)error_opcode;
    if (error_code != REMASTER_EMERALD_SCRIPT_ERROR_NONE
        && error_program_index < registry->program_count)
    {
        temp.vm.error.script_id =
            registry->programs[error_program_index].script_id;
    }

    temp.vm.status = status;
    temp.has_pending_request = has_pending;

    if (has_pending) {
        const RemasterEmeraldScriptProgram *request_program =
            &registry->programs[pending.program_index];
        const RemasterEmeraldScriptInstruction *request_instruction =
            &request_program->instructions[pending.pc];

        pending.resource_id = request_instruction->resource_id;
        temp.pending_request = pending;
    }

    *runtime = temp;
    return 1;
}

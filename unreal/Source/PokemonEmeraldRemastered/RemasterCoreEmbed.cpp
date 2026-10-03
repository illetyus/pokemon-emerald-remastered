/*
 * R0 integration unit.
 *
 * The portable gameplay implementation remains a single source of truth in
 * /core. Unreal compiles that implementation into its runtime module instead
 * of maintaining a second copy of the rules.
 */
extern "C"
{
#include "../../../core/src/core.c"
}

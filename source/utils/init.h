/**
 * @brief Copyright (C) 2022-2024 Volodymyr Atamanenko This software may be modified and distributed under the terms of the MIT license.
 * @note See `docs/source/utils/init.md:1` for detailed design rationale.
 */

/**
 * @brief Copyright (C) 2021 Andy Nguyen Copyright (C) 2021-2022 Rinnegatamante Copyright (C) 2022-2024 Volodymyr Atamanenko This software may be.
 */

#ifndef SOLOADER_INIT_H
#define SOLOADER_INIT_H

#include <so_util/so_util.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void resolve_imports(so_module *mod);

void so_patch();

void soloader_init_all();

#ifdef INSTRUMENT_BLIT_CALLS
/**
 * @brief Bug 16 (PORTING_PLAN.md), 4th perf pass.
 * @note See `docs/source/utils/init.md:30` for detailed design rationale.
 */
uint32_t patch_get_and_reset_blit_calls();
/**
 * @brief Writes "OPNAME:count,OPNAME:count,.
 * @note See `docs/source/utils/init.md:41` for detailed design rationale.
 */
void patch_format_and_reset_blit_histogram(char *buf, int buflen);
#endif

#ifdef __cplusplus
};
#endif

#endif // SOLOADER_INIT_H

/**
 * @brief Copyright (C) 2021 Andy Nguyen Copyright (C) 2021 fgsfds Copyright (C) 2022-2023 Volodymyr Atamanenko This software may be modified and.
 * @note See `docs/source/utils/dialog.md:1` for detailed design rationale.
 */

/**
 * @brief Copyright (C) 2021 Andy Nguyen Copyright (C) 2021 fgsfds Copyright (C) 2022-2023 Volodymyr Atamanenko This software may be modified and.
 */

#ifndef SOLOADER_DIALOG_H
#define SOLOADER_DIALOG_H

#ifdef __cplusplus
extern "C" {
#endif

int init_ime_dialog(const char *title, const char *initial_text);

char *get_ime_dialog_result(void);

int init_msg_dialog(const char *msg);

int get_msg_dialog_result(void);

void fatal_error(const char *fmt, ...) __attribute__((noreturn));

#ifdef __cplusplus
};
#endif

#endif // SOLOADER_DIALOG_H

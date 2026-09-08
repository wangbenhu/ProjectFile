/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup SHELL_SHELL SHELL_SHELL
 * @ingroup  Shell
 * @brief    shell
 * @details  shell header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __SHELL_H
#define __SHELL_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdarg.h>


#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * TYPEDEFS
 */
typedef void (*shell_putchar_t)(char character);

typedef void (*shell_cmd_func_t)(int argc, char *argv[]);
typedef struct {
    const char         *sc_name;
    shell_cmd_func_t    sc_function;
    const char         *description;
} shell_cmd_t;


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief      Initializer shell
 *
 * @param[in]   shell_cmd    Pointer to shell command
 *******************************************************************************
 */
extern void shell_init(const shell_cmd_t *shell_cmd);

/**
 *******************************************************************************
 * @brief    format input parameter output to shell
 *
 * @param[in] fmt format string
 * @param[in] va   variable argument
 *******************************************************************************
 */
extern int shell_vprintf(const char *fmt, va_list va);

/**
 *******************************************************************************
 * @brief exec shell cmd
 *
 * 执行传入的 shell 命令字符串。
 *
 * @param cmd 命令字符串
 *******************************************************************************
 */
extern void shell_exec(const char *cmd);

/**
 *******************************************************************************
 * @brief argv to cmdline
 *
 * @param argc number of arguments
 * @param argv arguments vector
 *******************************************************************************
 */
extern const char *shell_argv_to_cmdline(int argc, char *argv[]);

/**
 *******************************************************************************
 * @brief restore shell from sleep
 *
 *******************************************************************************
 */
extern void shell_restore(void);

#ifdef __cplusplus
}
#endif


#endif  /*__SHELL */


/** @} */

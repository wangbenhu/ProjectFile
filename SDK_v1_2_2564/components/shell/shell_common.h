/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup SHELL_CMD SHELL_CMD
 * @ingroup  Shell
 * @brief    shell system command
 * @details  shell system command
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


#ifndef __SHELL_COMMON_H
#define __SHELL_COMMON_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdbool.h>
#include "shell.h"


#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
// shell maximum arguments per command
#ifndef SHELL_MAX_ARGUMENTS
#define SHELL_MAX_ARGUMENTS                    (80)
#endif


/*******************************************************************************
 * EXTERN FUNCTIONS
 */
/**
 *******************************************************************************
 * @brief   Shell thread function.
 *
 * @param[in] line           pointer to the line buffer
 * @param[in] scp            pointer to the shell command
 *
 *******************************************************************************
 */
extern void shell_main(char *line, const shell_cmd_t *scp);

/**
 *******************************************************************************
 * @brief   Reads a whole line from the input channel.
 *
 * @param[in] line        pointer to the line buffer
 * @param[in] line_size   buffer maximum length
 *
 * @return                The operation status.
 * @retval true           the channel was reset or CTRL-D pressed.
 * @retval false          operation successful.
 *******************************************************************************
 */
extern bool shell_get_line(char c, char *line, unsigned line_size, unsigned *line_index);


#ifdef __cplusplus
}
#endif


#endif  /* __SHELL_COMMON_H */


/** @} */

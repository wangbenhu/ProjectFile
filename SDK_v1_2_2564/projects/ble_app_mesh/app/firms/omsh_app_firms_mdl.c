/* ----------------------------------------------------------------------------
 * Copyright (c) 2020-2030 OnMicro Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of OnMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * -------------------------------------------------------------------------- */

/**
 * @file     omsh_app_firms_mdl.c
 * @brief    omesh app of firms models source file
 * @date     01. OCT. 2023
 * @author   OnMicro SW Team
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */


/*******************************************************************************
 * INCLUDES
 */
#include "omsh_app.h"


/*******************************************************************************
 * EXPORTED FUNCTION DEFINITIONS
 */
/// Called by the mesh stack, upper application does not call
/// When register one model, please add it as follow
uint16_t mm_firms_init(bool reset, void *p_env, void *p_cfg)
{
    // Environment initialization
    uint8_t *p_env_cursor = (uint8_t *)p_env;

    p_env_cursor += CO_ALIGN4_HI(mm_firms_demo_init(reset, (void *)p_env_cursor, p_cfg));
    // TODO: Add when register a model

    // Return size of environment
    return ((uint32_t)p_env_cursor - (uint32_t)p_env);
}

/// Called by the mesh stack, upper application does not call
/// When register one model, please add it as follow
uint16_t mm_firms_get_env_size(void *p_cfg)
{
    uint16_t total_env_size = 0;

    total_env_size += CO_ALIGN4_HI(mm_firms_demo_get_env_size(p_cfg));
    // TODO: Add when register a model

    // Return size of environment
    return (total_env_size);
}


/** @} */
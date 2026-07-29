/* ----------------------------------------------------------------------------
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 * -------------------------------------------------------------------------- */

/**
 * @defgroup QDEC QDEC
 * @ingroup  DEVICE
 * @brief    QDEC register
 * @details  QDEC register definitions header file
 *
 * @version
 * Version 1.0
 *  - Initial release
 *
 * @{
 */

#ifndef __QDEC_REG_H
#define __QDEC_REG_H


/*******************************************************************************
 * INCLUDES
 */
#include <stdint.h>
#include "common_reg.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*******************************************************************************
 * MACROS
 */
// SHORT
#define QDEC_SHORT_SAMPLERDY_READCLRACC_POS         6
#define QDEC_SHORT_SAMPLERDY_READCLRACC_MASK        (0x1U << 6)
#define QDEC_SHORT_DBLRDY_STOP_POS                  5
#define QDEC_SHORT_DBLRDY_STOP_MASK                 (0x1U << 5)
#define QDEC_SHORT_DBLRDY_RDCLRDBL_POS              4
#define QDEC_SHORT_DBLRDY_RDCLRDBL_MASK             (0x1U << 4)
#define QDEC_SHORT_REPORTRDY_STOP_POS               3
#define QDEC_SHORT_REPORTRDY_STOP_MASK              (0x1U << 3)
#define QDEC_SHORT_REPORTRDY_RDCLRACC_POS           2
#define QDEC_SHORT_REPORTRDY_RDCLRACC_MASK          (0x1U << 2)
#define QDEC_SHORT_SAMPLERDY_STOP_POS               1
#define QDEC_SHORT_SAMPLERDY_STOP_MASK              (0x1U << 1)
#define QDEC_SHORT_REPORTRDY_READCLRACC_POS         0
#define QDEC_SHORT_REPORTRDY_READCLRACC_MASK        (0x1U << 0)

// START
#define QDEC_START_START_POS                        0
#define QDEC_START_START_MASK                       (0x1U << 0)

// STOP
#define QDEC_STOP_STOP_POS                          0
#define QDEC_STOP_STOP_MASK                         (0x1U << 0)

// READCLRACC
#define QDEC_READCLRACC_READCLRACC_POS              0
#define QDEC_READCLRACC_READCLRACC_MASK             (0x1U << 0)

// RDCLRACC
#define QDEC_RDCLRACC_RDCLRACC_POS                  0
#define QDEC_RDCLRACC_RDCLRACC_MASK                 (0x1U << 0)

// RDCLRDBL
#define QDEC_RDCLRDBL_RDCLRDBL_POS                  0
#define QDEC_RDCLRDBL_RDCLRDBL_MASK                 (0x1U << 0)

// INTST
#define QDEC_INTST_STOPPED_POS                      4
#define QDEC_INTST_STOPPED_MASK                     (0x1U << 4)
#define QDEC_INTST_DBLRDY_POS                       3
#define QDEC_INTST_DBLRDY_MASK                      (0x1U << 3)
#define QDEC_INTST_ACCOF_POS                        2
#define QDEC_INTST_ACCOF_MASK                       (0x1U << 2)
#define QDEC_INTST_REPORTRDY_POS                    1
#define QDEC_INTST_REPORTRDY_MASK                   (0x1U << 1)
#define QDEC_INTST_SAMPLERDY_POS                    0
#define QDEC_INTST_SAMPLERDY_MASK                   (0x1U << 0)

// INTST_RAW
#define QDEC_INTST_RAW_STOPPED_POS                  4
#define QDEC_INTST_RAW_STOPPED_MASK                 (0x1U << 4)
#define QDEC_INTST_RAW_DBLRDY_POS                   3
#define QDEC_INTST_RAW_DBLRDY_MASK                  (0x1U << 3)
#define QDEC_INTST_RAW_ACCOF_POS                    2
#define QDEC_INTST_RAW_ACCOF_MASK                   (0x1U << 2)
#define QDEC_INTST_RAW_REPORTRDY_POS                1
#define QDEC_INTST_RAW_REPORTRDY_MASK               (0x1U << 1)
#define QDEC_INTST_RAW_SAMPLERDY_POS                0
#define QDEC_INTST_RAW_SAMPLERDY_MASK               (0x1U << 0)

// INTEN
#define QDEC_INTEN_STOPPED_POS                      4
#define QDEC_INTEN_STOPPED_MASK                     (0x1U << 4)
#define QDEC_INTEN_DBLRDY_POS                       3
#define QDEC_INTEN_DBLRDY_MASK                      (0x1U << 3)
#define QDEC_INTEN_ACCOF_POS                        2
#define QDEC_INTEN_ACCOF_MASK                       (0x1U << 2)
#define QDEC_INTEN_REPORTRDY_POS                    1
#define QDEC_INTEN_REPORTRDY_MASK                   (0x1U << 1)
#define QDEC_INTEN_SAMPLERDY_POS                    0
#define QDEC_INTEN_SAMPLERDY_MASK                   (0x1U << 0)

// INTCLR
#define QDEC_INTCLR_STOPPED_POS                     4
#define QDEC_INTCLR_STOPPED_MASK                    (0x1U << 4)
#define QDEC_INTCLR_DBLRDY_POS                      3
#define QDEC_INTCLR_DBLRDY_MASK                     (0x1U << 3)
#define QDEC_INTCLR_ACCOF_POS                       2
#define QDEC_INTCLR_ACCOF_MASK                      (0x1U << 2)
#define QDEC_INTCLR_REPORTRDY_POS                   1
#define QDEC_INTCLR_REPORTRDY_MASK                  (0x1U << 1)
#define QDEC_INTCLR_SAMPLERDY_POS                   0
#define QDEC_INTCLR_SAMPLERDY_MASK                  (0x1U << 0)

// ENABLE
#define QDEC_ENABLE_ENABLE_POS                      0
#define QDEC_ENABLE_ENABLE_MASK                     (0x1U << 0)

// LEDPOL
#define QDEC_LEDPOL_LEDPOL_POS                      0
#define QDEC_LEDPOL_LEDPOL_MASK                     (0x1U << 0)

// SAMPLEPER
#define QDEC_SAMPLEPER_SAMPLEPER_POS                0
#define QDEC_SAMPLEPER_SAMPLEPER_MASK               (0xFU << 0)

// SAMPLE
#define QDEC_SAMPLE_SAMPLE_POS                      0
#define QDEC_SAMPLE_SAMPLE_MASK                     (0xFFFFFFFFU << 0)

// REPORTPER
#define QDEC_REPORTPER_REPORTPER_POS                0
#define QDEC_REPORTPER_REPORTPER_MASK               (0xFU << 0)

// ACC
#define QDEC_ACC_ACC_POS                            0
#define QDEC_ACC_ACC_MASK                           (0xFFFFFFFFU << 0)

// ACCREAD
#define QDEC_ACCREAD_ACCREAD_POS                    0
#define QDEC_ACCREAD_ACCREAD_MASK                   (0xFFFFFFFFU << 0)

// PSELLED
#define QDEC_PSELLED_CONNECT_POS                    31
#define QDEC_PSELLED_CONNECT_MASK                   (0x1U << 31)

// INIT
#define QDEC_INIT_A_POS                             1
#define QDEC_INIT_A_MASK                            (0x1U << 1)
#define QDEC_INIT_B_POS                             0
#define QDEC_INIT_B_MASK                            (0x1U << 0)

// DBFEN
#define QDEC_DBFEN_DBFEN_POS                        0
#define QDEC_DBFEN_DBFEN_MASK                       (0x1U << 0)

// LEDPRE
#define QDEC_LEDPRE_LEDPRE_POS                      0
#define QDEC_LEDPRE_LEDPRE_MASK                     (0x1FFU << 0)

// ACCDBL
#define QDEC_ACCDBL_ACCDBL_POS                      0
#define QDEC_ACCDBL_ACCDBL_MASK                     (0xFU << 0)

// ACCDBLREAD
#define QDEC_ACCDBLREAD_ACCDBLREAD_POS              0
#define QDEC_ACCDBLREAD_ACCDBLREAD_MASK             (0xFU << 0)

// INPUT
#define QDEC_INPUT_A_POS                            1
#define QDEC_INPUT_A_MASK                           (0x1U << 1)
#define QDEC_INPUT_B_POS                            0
#define QDEC_INPUT_B_MASK                           (0x1U << 0)

/*******************************************************************************
 * TYPEDEFS
 */
typedef struct
{
    __IO uint32_t SHORT;                /*!< offset:0x00  */
    __IO uint32_t START;                /*!< offset:0x04  */
    __O  uint32_t STOP;                 /*!< offset:0x08  */
    __O  uint32_t READCLRACC;           /*!< offset:0x0c  */
    __O  uint32_t RDCLRACC;             /*!< offset:0x10  */
    __O  uint32_t RDCLRDBL;             /*!< offset:0x14  */
    __I  uint32_t INTST;                /*!< offset:0x18  Interrupt Status */
    __I  uint32_t INTST_RAW;            /*!< offset:0x1c  Interrupt Status Raw  */
    __IO uint32_t INTEN;                /*!< offset:0x20  Interrupt Enable  */
    __IO uint32_t INTCLR;               /*!< offset:0x24  Interrupt Status Clear  */
    __IO uint32_t ENABLE;               /*!< offset:0x28  */
    __IO uint32_t LEDPOL;               /*!< offset:0x2c  */
    __IO uint32_t SAMPLEPER;            /*!< offset:0x30  */
    __I  uint32_t SAMPLE;               /*!< offset:0x34  */
    __IO uint32_t REPORTPER;            /*!< offset:0x38  */
    __I  uint32_t ACC;                  /*!< offset:0x3c  */
    __I  uint32_t ACCREAD;              /*!< offset:0x40  */
    __IO uint32_t PSELLED;              /*!< offset:0x44  */
    __IO uint32_t INIT;                 /*!< offset:0x48  */
         uint32_t RESERVED0;            /*!< offset:0x4c  */
    __IO uint32_t DBFEN;                /*!< offset:0x50  */
    __IO uint32_t LEDPRE;               /*!< offset:0x54  */
    __I  uint32_t ACCDBL;               /*!< offset:0x58  */
    __I  uint32_t ACCDBLREAD;           /*!< offset:0x5c  */
    __I  uint32_t INPUT;                /*!< offset:0x60  */
} OM_QDEC_Type;

/*******************************************************************************
 * EXTERN VARIABLES
 */


/*******************************************************************************
 * EXTERN FUNCTIONS
 */


#ifdef __cplusplus
}
#endif


#endif  /* __QDEC_REG_H */


/** @} */

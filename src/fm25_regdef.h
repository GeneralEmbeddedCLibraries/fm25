// Copyright (c) 2023 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      fm25_regdef.h
*@brief     Register description for FM25 FRAM device
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      14.11.2023
*@version   V1.0.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup FM25_REGEDEF
* @{ <!-- BEGIN GROUP -->
*
*     Register definitions of FM25 FRAM devices.
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef FM25_REGDEF_H_
#define FM25_REGDEF_H_

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *     FM25 Instruction Set
 */
typedef enum
{
    eFM25_ISA_WREN  = 0x06U,        /**<Set write enable latch - enable write operation */
    eFM25_ISA_WRITE = 0x02U,        /**<Write data to memory array beginning at selected address */
    eFM25_ISA_READ  = 0x03U,        /**<Read data from memory array beginning at selected address */
    eFM25_ISA_WRDI  = 0x04U,        /**<Reset write enable latch - disable write operation */
    eFM25_ISA_RDSR  = 0x05U,        /**<Read STATUS register */
    eFM25_ISA_WRSR  = 0x01U,        /**<Write STATUS register */
    eFM25_ISA_RDID  = 0x9FU,        /**<Read electronic signature */

    // Not all devices support following commands
    eFM25_ISA_SLEEP = 0xB9U,        /**<Enter sleep mode */
    eFM25_ISA_FSTRD = 0x0BU,        /**<Reads data from F-RAM array at 40MHz */
    eFM25_ISA_SNR   = 0xC3U,        /**<Reads 8-byte serial number */
} fm25_isa_t;

/**
 *     Status register
 */
typedef struct
{
    uint8_t res_0   : 1;    /**<Reserved */
    uint8_t wel     : 1;    /**<Write Enable Latch */
    uint8_t bp      : 2;    /**<Block protection */
    uint8_t res_1   : 3;    /**<Reserved */
    uint8_t wpen    : 1;    /**<Write Protect enable */
} fm25_status_reg_bits_t;

typedef union
{
    uint8_t u;                  /**<Unsigned access */
    fm25_status_reg_bits_t b;   /**<Bitfield access */
} fm25_status_reg_t;

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
#endif // FM25_REGDEF_H_

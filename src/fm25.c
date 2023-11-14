// Copyright (c) 2023 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      fm25.c
*@brief     API for FM25 FRAM device
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      14.11.2023
*@version   V1.0.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup FM25_API
* @{ <!-- BEGIN GROUP -->
*
*     API functions for FRAM device FM25
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "fm25.h"
#include "fm25_regdef.h"
#include "../../fm25_if.h"
#include "../../fm25_cfg.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *  Highest memory address
 */
#define FM25_MAX_ADDR                   ((uint32_t) (( 1UL << FM25_CFG_ADDR_BIT_NUM ) - 1UL ))


/**
 *     Erase value
 */
#define FM25_ERASE_VALUE                ((uint8_t)( 0xFFU ))

/**
 *     Read/Write memory command
 */
typedef union
{
    struct
    {
        uint8_t cmd;        /**<Command part of frame */
        uint8_t addr[3];    /**<Address part of frame */
    } field;
    uint32_t u;             /**<Unsigned access */
}fm25_rw_cmd_t;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *     Initialization guard
 */
static bool gb_is_init = false;

////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
////////////////////////////////////////////////////////////////////////////////
static fm25_status_t    fm25_write_enable       (void);
static fm25_status_t    fm25_write_disable      (void);
static fm25_status_t    fm25_read_status        (fm25_status_reg_t * const p_status_reg);
static fm25_status_t    fm25_read_command       (const uint32_t addr);
static fm25_status_t    fm25_write_command      (const uint32_t addr);
static void             fm25_assemble_rw_cmd    (fm25_rw_cmd_t * const p_frame, const fm25_isa_t rw_cmd, const uint32_t addr);
static bool             fm25_read_wel_flag      (void);

// TODO: Unused func...
//static fm25_status_t  fm25_write_status       (const fm25_status_reg_t * const p_status_reg);

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*        Enable write latch
*
* @return   status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static fm25_status_t fm25_write_enable(void)
{
            fm25_status_t   status  = eFM25_OK;
    const   fm25_isa_t      cmd     = eFM25_ISA_WREN;

    status = fm25_if_transmit( &cmd, 1U, ( eSPI_CS_HIGH_ON_EXIT | eSPI_CS_LOW_ON_ENTRY ));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Disable write latch
*
* @return   status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static fm25_status_t fm25_write_disable(void)
{
            fm25_status_t   status  = eFM25_OK;
    const   fm25_isa_t      cmd     = eFM25_ISA_WRDI;

    status = fm25_if_transmit( &cmd, 1U, ( eSPI_CS_HIGH_ON_EXIT | eSPI_CS_LOW_ON_ENTRY ));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Read status register from device
*
* @param[out]   p_status_reg    - Pointer to status register
* @return       status          - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static fm25_status_t fm25_read_status(fm25_status_reg_t * const p_status_reg)
{
            fm25_status_t   status  = eFM25_OK;
    const   fm25_isa_t      cmd     = eFM25_ISA_RDSR;

    status = fm25_if_transmit( &cmd, 1U, eSPI_CS_LOW_ON_ENTRY );
    status |= fm25_if_receive((uint8_t*) p_status_reg, 1, eSPI_CS_HIGH_ON_EXIT );

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Assemble read/write command
*
* @note     For FM25L04 A8 bit of address is encoded into command part
*           of the frame.
*
*           Look at the AN304 Document No. 001-87196 Rev. *E  p.7
*           figure 9. Addressing Differences Between Densities
*
*
* @param[out]   p_frame - Pointer to cmd frame
* @param[in]    rw_cmd  - Device command for read or write
* @param[in]    addr    - Start address of read or write
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void fm25_assemble_rw_cmd(fm25_rw_cmd_t * const p_frame, const fm25_isa_t rw_cmd, const uint32_t addr)
{
    FM25_ASSERT( NULL != p_frame );

    p_frame->u          = 0UL;
    p_frame->field.cmd  = rw_cmd;

    // 4kbit devices
    // 9-bit addressing
    #if ( 9 == FM25_CFG_ADDR_BIT_NUM )
    {
        // 9 bit address specialty
        // NOTE: Address bit A8 is encoded into command part of the device
        if (( addr & 0x100U ) == 0x100U )
        {
            p_frame->field.cmd |= ( 0x80U );
        }
        else
        {
            p_frame->field.cmd &= ~( 0x80U );
        }

        p_frame->field.addr[0U] = ( addr & 0xFFU );
    }

    // Devices from 16kbit to 512kbit
    // 10-bit - 16-bit addressing
    #elif ( FM25_CFG_ADDR_BIT_NUM < 16 )
    {
        p_frame->field.addr[0]    = (( addr >> 8U ) & 0xFFU );
        p_frame->field.addr[1]    = ( addr          & 0xFFU );
    }

    // Devices from 1Mbit to 4Mbit
    // 17-bit - 19-bit addressing
    #else
    {
        p_frame->field.addr[0]    = (( addr >> 16U )    & 0xFFU );
        p_frame->field.addr[1]    = (( addr >> 8U )     & 0xFFU );
        p_frame->field.addr[2]    = ( addr              & 0xFFU );
    }
    #endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Send write command to device
*
* @param[in]    addr    - Start address of write transfer
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static fm25_status_t fm25_write_command(const uint32_t addr)
{
    fm25_status_t status    = eFM25_OK;
    fm25_rw_cmd_t cmd       = { .u = 0U };

    // Enable write enable latch
    fm25_write_enable();

    // Assemble command
    fm25_assemble_rw_cmd( &cmd, eFM25_ISA_WRITE, addr );

    // 4kbit devices
    // 9-bit addressing
    // NOTE: Address bit A8 is encoded into command part of the device
    #if ( 9 == FM25_CFG_ADDR_BIT_NUM )
    {
        status = fm25_if_transmit((uint8_t*) &cmd.u, 2U, eSPI_CS_LOW_ON_ENTRY );
    }

    // Devices from 16kbit to 512kbit
    // 10-bit - 16-bit addressing
    #elif ( FM25_CFG_ADDR_BIT_NUM < 16 )
    {
        status = fm25_if_transmit((uint8_t*) &cmd.u, 3U, eSPI_CS_LOW_ON_ENTRY );
    }

    // Devices from 1Mbit to 4Mbit
    // 17-bit - 19-bit addressing
    #else
    {
        status = fm25_if_transmit((uint8_t*) &cmd.u, 4U, eSPI_CS_LOW_ON_ENTRY );
    }
    #endif

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Send read command to device
*
* @param[in]    addr    - Start address of write transfer
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static fm25_status_t fm25_read_command(const uint32_t addr)
{
    fm25_status_t status = eFM25_OK;
    fm25_rw_cmd_t cmd    = { .u = 0U };

    // Assemble command
    fm25_assemble_rw_cmd( &cmd, eFM25_ISA_READ, addr );

    // 4kbit devices
    // 9-bit addressing
    // NOTE: Address bit A8 is encoded into command part of the device
    #if ( 9 == FM25_CFG_ADDR_BIT_NUM )
    {
        status = fm25_if_transmit((uint8_t*) &cmd.u, 2U, eSPI_CS_LOW_ON_ENTRY );
    }

    // Devices from 16kbit to 512kbit
    // 10-bit - 16-bit addressing
    #elif ( FM25_CFG_ADDR_BIT_NUM < 16 )
    {
        status = fm25_if_transmit((uint8_t*) &cmd.u, 3U, eSPI_CS_LOW_ON_ENTRY );
    }

    // Devices from 1Mbit to 4Mbit
    // 17-bit - 19-bit addressing
    #else
    {
        status = fm25_if_transmit((uint8_t*) &cmd.u, 4U, eSPI_CS_LOW_ON_ENTRY );
    }
    #endif

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Read WEL flag from device
*
* @return   wel - State of Write-Enable-Latch
*/
////////////////////////////////////////////////////////////////////////////////
static bool fm25_read_wel_flag(void)
{
    bool                wel      = false;
    fm25_status_reg_t   stat_reg = { .u = 0U };

    if ( eFM25_OK == fm25_read_status( & stat_reg ))
    {
        wel = (bool) ( stat_reg.b.wel );
    }

    return wel;
}

// TODO: UNUSED FUNCTION!!!!
#if 0
////////////////////////////////////////////////////////////////////////////////
/**
*        Write to device status register
*
* @param[in]    p_status_reg    - Pointer to status register
* @return         status             - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static fm25_status_t fm25_write_status(const fm25_status_reg_t * const p_status_reg)
{
            fm25_status_t status = eFM25_OK;
    const   fm25_isa_t    cmd    = eFM25_ISA_WRSR;

    status = fm25_if_transmit( &cmd, 1, eSPI_CS_LOW_ON_ENTRY );
    status |= fm25_if_transmit((uint8_t*) p_status_reg, 1U, eSPI_CS_HIGH_ON_EXIT );

    return status;
}
#endif

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup FM25_API
* @{ <!-- BEGIN GROUP -->
*
*   Following function are part of FM25 API.
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*        Initialize FRAM device
*
* @return     status - Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
fm25_status_t fm25_init(void)
{
    fm25_status_t status = eFM25_OK;

    if ( false == gb_is_init )
    {
        // Initialize app interface
        status = fm25_if_init();

        // Enable write latch
        status |= fm25_write_enable();

        // Read WEL flag
        const bool wel_flag = fm25_read_wel_flag();

        if  (   ( eFM25_OK == status )
            &&  ( true == wel_flag ))
        {
            gb_is_init = true;

            FM25_DBG_PRINT("FM25: Init success!");
        }
        else
        {
            status = eFM25_ERROR_INIT;

            FM25_DBG_PRINT("FM25: Init error!");
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       De-initialize FRAM device
*
* @return   status - Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
fm25_status_t fm25_deinit(void)
{
    fm25_status_t status = eFM25_OK;

    if ( true == gb_is_init )
    {
        // Disable write latch
        status |= fm25_write_disable();

        // De-init interface layer
        status |= fm25_if_deinit();

        // De-init
        gb_is_init = false;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Is device initialized
*
* @param[out]   p_is_init   - Initialization flag
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
fm25_status_t fm25_is_init(bool * const p_is_init)
{
    fm25_status_t status = eFM25_OK;

    if ( NULL != p_is_init )
    {
        *p_is_init = gb_is_init;
    }
    else
    {
        status = eFM25_ERROR;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Write byte(s) to FRAM
*
* @param[in]    addr    - Start address of write
* @param[in]    size    - Size of bytes to write
* @param[in]    p_data  - Pointer to write data
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
fm25_status_t fm25_write(const uint32_t addr, const uint32_t size, const uint8_t * const p_data)
{
    fm25_status_t status = eFM25_OK;

    // Check for init
    FM25_ASSERT( true == gb_is_init );

    if ( true == gb_is_init )
    {
        // Send write command
        status = fm25_write_command( addr );

        // Send data payload
        status = fm25_if_transmit( p_data, size, eSPI_CS_HIGH_ON_EXIT );
    }
    else
    {
        status = eFM25_ERROR_INIT;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Erase byte(s) from FRAM
*
* @brief    This function erase number of bytes from FRAM device starting
*           from addr parameter. Erase value is defined by "FM25_ERASE_VALUE" macro.
*
*
* @note     Erasing size must be less than 256 bytes!
*
* @param[in]    addr    - Start address of write
* @param[in]    size    - Size of bytes to write
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
fm25_status_t fm25_erase(const uint32_t addr, const uint32_t size)
{
            fm25_status_t status            = eFM25_OK;
    static  uint8_t       erase_data[256U]  = { 0 };

    FM25_ASSERT( true == gb_is_init );
    FM25_ASSERT( size < 256U );

    // Check for init
    if ( true == gb_is_init )
    {
        if ( size < 256U )
        {
            // Prepare erase data
            for ( uint32_t i = 0U; i < 256U; i++ )
            {
                erase_data[i] = FM25_ERASE_VALUE;
            }

            // Erase memory
            status = fm25_write( addr, size, (uint8_t*) erase_data );
        }
        else
        {
            status = eFM25_ERROR;
        }
    }
    else
    {
        status = eFM25_ERROR_INIT;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*        Read byte(s) from FRAM
*
* @param[in]    addr    - Start address of write
* @param[in]    size    - Size of bytes to write
* @param[out]   p_data  - Pointer to read data
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
fm25_status_t fm25_read(const uint32_t addr, const uint32_t size, uint8_t * const p_data)
{
    fm25_status_t status = eFM25_OK;

    // Check for init
    FM25_ASSERT( true == gb_is_init );

    // Invalid inputs
    FM25_ASSERT( size > 0U );
    FM25_ASSERT(( addr + size - 1U ) <= FM25_MAX_ADDR );

    if ( true == gb_is_init )
    {
        if  (   ( size > 0U )
            &&  (( addr + size - 1U ) <= FM25_MAX_ADDR ))
        {
            // Send read command
            status = fm25_read_command( addr );

            // Send data payload
            status |= fm25_if_receive( p_data, size, eSPI_CS_HIGH_ON_EXIT );
        }
        else
        {
            status = eFM25_ERROR;
        }
    }
    else
    {
        status = eFM25_ERROR_INIT;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

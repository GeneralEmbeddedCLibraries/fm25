// Copyright (c) 2022 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      fm25.h
*@brief     API for FM25 FRAM device
*@author    Ziga Miklosic
*@date      02.08.2022
*@version	V1.0.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup FM25_API
* @{ <!-- BEGIN GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef FM25_H_
#define FM25_H_

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * 	Module version
 */
#define FM25_VER_MAJOR		( 1 )
#define FM25_VER_MINOR		( 0 )
#define FM25_VER_DEVELOP	( 0 )

/**
 * 	Status
 */
typedef enum
{
	eFM25_OK			= 0,		/**<Normal operation */
	eFM25_ERROR			= 0x01,		/**<General error */
	eFM25_ERROR_SPI		= 0x02,		/**<SPI error */
	eFM25_ERROR_INIT	= 0x04,		/**<Initialisation error */
	eFM25_ERROR_ADDR	= 0x08,		/**<Invalid memory address */
} fm25_status_t;

/**
 * 	Write protection options
 */
typedef enum
{
	eFM25_PROTECT_NONE = 0,		/**<All sectors not protected */
	eFM25_PROTECT_UPPER_1_4,	/**<Upper 1/4 sectors protected (Sector 3)*/
	eFM25_PROTECT_UPPER_1_2,	/**<Upper 1/2 sectors protected (Sector 2 & 3)*/
	eFM25_PROTECT_UPPER_ALL,	/**<All sectors protected (Sector 0, 1, 2 & 3)*/
} fm25_protect_t;

////////////////////////////////////////////////////////////////////////////////
// Functions Prototypes
////////////////////////////////////////////////////////////////////////////////
fm25_status_t fm25_init				(void);
fm25_status_t fm25_deinit			(void);
fm25_status_t fm25_is_init			(bool * const p_is_init);
fm25_status_t fm25_write			(const uint32_t addr, const uint32_t size, const uint8_t * const p_data);
fm25_status_t fm25_erase			(const uint32_t addr, const uint32_t size);
fm25_status_t fm25_read				(const uint32_t addr, const uint32_t size, uint8_t * const p_data);
fm25_status_t fm25_set_protection	(const fm25_protect_t prot_opt);

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
#endif // FM25_H_

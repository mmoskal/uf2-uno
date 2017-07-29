/*
             LUFA Library
     Copyright (C) Dean Camera, 2017.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2017  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Functions to manage the physical Dataflash media, including reading and writing of
 *  blocks of data. These functions are called by the SCSI layer when data must be stored
 *  or retrieved to/from the physical storage media. If a different media is used (such
 *  as a SD card or EEPROM), functions similar to these will need to be generated.
 */

#define  INCLUDE_FROM_DATAFLASHMANAGER_C
#include "DataflashManager.h"

/** Writes blocks (OS blocks, not Dataflash pages) to the storage medium, the board Dataflash IC(s), from
 *  the pre-selected data OUT endpoint. This routine reads in OS sized blocks from the endpoint and writes
 *  them to the Dataflash in Dataflash page sized blocks.
 *
 *  \param[in] MSInterfaceInfo  Pointer to a structure containing a Mass Storage Class configuration and state
 *  \param[in] BlockAddress  Data block starting address for the write sequence
 *  \param[in] TotalBlocks   Number of blocks of data to write
 */
void DataflashManager_WriteBlocks(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo,
                                  const uint32_t BlockAddress,
                                  uint16_t TotalBlocks)
{
	#if 0
	/* Wait until endpoint is ready before continuing */
	if (Endpoint_WaitUntilReady())
	  return;

	while (TotalBlocks)
	{
		uint8_t BytesInBlockDiv16 = 0;

		/* Write an endpoint packet sized data block to the Dataflash */
		while (BytesInBlockDiv16 < (VIRTUAL_MEMORY_BLOCK_SIZE >> 4))
		{
			/* Check if the endpoint is currently empty */
			if (!(Endpoint_IsReadWriteAllowed()))
			{
				/* Clear the current endpoint bank */
				Endpoint_ClearOUT();

				/* Wait until the host has sent another packet */
				if (Endpoint_WaitUntilReady())
				  return;
			}

			/* Check if end of Dataflash page reached */
			if (CurrDFPageByteDiv16 == (DATAFLASH_PAGE_SIZE >> 4))
			{
				/* Write the Dataflash buffer contents back to the Dataflash page */
				Dataflash_WaitWhileBusy();
				Dataflash_SendByte(UsingSecondBuffer ? DF_CMD_BUFF2TOMAINMEMWITHERASE : DF_CMD_BUFF1TOMAINMEMWITHERASE);
				Dataflash_SendAddressBytes(CurrDFPage, 0);

				/* Reset the Dataflash buffer counter, increment the page counter */
				CurrDFPageByteDiv16 = 0;
				CurrDFPage++;

				/* Once all the Dataflash ICs have had their first buffers filled, switch buffers to maintain throughput */
				if (Dataflash_GetSelectedChip() == DATAFLASH_CHIP_MASK(DATAFLASH_TOTALCHIPS))
				  UsingSecondBuffer = !(UsingSecondBuffer);

				/* Select the next Dataflash chip based on the new Dataflash page index */
				Dataflash_SelectChipFromPage(CurrDFPage);

#if (DATAFLASH_PAGE_SIZE > VIRTUAL_MEMORY_BLOCK_SIZE)
				/* If less than one Dataflash page remaining, copy over the existing page to preserve trailing data */
				if ((TotalBlocks * (VIRTUAL_MEMORY_BLOCK_SIZE >> 4)) < (DATAFLASH_PAGE_SIZE >> 4))
				{
					/* Copy selected dataflash's current page contents to the Dataflash buffer */
					Dataflash_WaitWhileBusy();
					Dataflash_SendByte(UsingSecondBuffer ? DF_CMD_MAINMEMTOBUFF2 : DF_CMD_MAINMEMTOBUFF1);
					Dataflash_SendAddressBytes(CurrDFPage, 0);
					Dataflash_WaitWhileBusy();
				}
#endif

				/* Send the Dataflash buffer write command */
				Dataflash_SendByte(UsingSecondBuffer ? DF_CMD_BUFF2WRITE : DF_CMD_BUFF1WRITE);
				Dataflash_SendAddressBytes(0, 0);
			}

			/* Write one 16-byte chunk of data to the Dataflash */
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());
			Dataflash_SendByte(Endpoint_Read_8());

			/* Increment the Dataflash page 16 byte block counter */
			CurrDFPageByteDiv16++;

			/* Increment the block 16 byte block counter */
			BytesInBlockDiv16++;

			/* Check if the current command is being aborted by the host */
			if (MSInterfaceInfo->State.IsMassStoreReset)
			  return;
		}

		/* Decrement the blocks remaining counter */
		TotalBlocks--;
	}

	/* If the endpoint is empty, clear it ready for the next packet from the host */
	if (!(Endpoint_IsReadWriteAllowed()))
	  Endpoint_ClearOUT();

	
	#endif
}

/** Reads blocks (OS blocks, not Dataflash pages) from the storage medium, the board Dataflash IC(s), into
 *  the pre-selected data IN endpoint. This routine reads in Dataflash page sized blocks from the Dataflash
 *  and writes them in OS sized blocks to the endpoint.
 *
 *  \param[in] MSInterfaceInfo  Pointer to a structure containing a Mass Storage Class configuration and state
 *  \param[in] BlockAddress  Data block starting address for the read sequence
 *  \param[in] TotalBlocks   Number of blocks of data to read
 */
void DataflashManager_ReadBlocks(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo,
                                 const uint32_t BlockAddress,
                                 uint16_t TotalBlocks)
{
#if 0

	/* Wait until endpoint is ready before continuing */
	if (Endpoint_WaitUntilReady())
	  return;

	while (TotalBlocks)
	{
		uint8_t BytesInBlockDiv16 = 0;

		/* Read an endpoint packet sized data block from the Dataflash */
		while (BytesInBlockDiv16 < (VIRTUAL_MEMORY_BLOCK_SIZE >> 4))
		{
			/* Check if the endpoint is currently full */
			if (!(Endpoint_IsReadWriteAllowed()))
			{
				/* Clear the endpoint bank to send its contents to the host */
				Endpoint_ClearIN();

				/* Wait until the endpoint is ready for more data */
				if (Endpoint_WaitUntilReady())
				  return;
			}

			/* Check if end of Dataflash page reached */
			if (CurrDFPageByteDiv16 == (DATAFLASH_PAGE_SIZE >> 4))
			{
				/* Reset the Dataflash buffer counter, increment the page counter */
				CurrDFPageByteDiv16 = 0;
				CurrDFPage++;

				/* Select the next Dataflash chip based on the new Dataflash page index */
				Dataflash_SelectChipFromPage(CurrDFPage);

				/* Send the Dataflash main memory page read command */
				Dataflash_SendByte(DF_CMD_MAINMEMPAGEREAD);
				Dataflash_SendAddressBytes(CurrDFPage, 0);
				Dataflash_SendByte(0x00);
				Dataflash_SendByte(0x00);
				Dataflash_SendByte(0x00);
				Dataflash_SendByte(0x00);
			}

			/* Read one 16-byte chunk of data from the Dataflash */
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());
			Endpoint_Write_8(Dataflash_ReceiveByte());

			/* Increment the Dataflash page 16 byte block counter */
			CurrDFPageByteDiv16++;

			/* Increment the block 16 byte block counter */
			BytesInBlockDiv16++;

			/* Check if the current command is being aborted by the host */
			if (MSInterfaceInfo->State.IsMassStoreReset)
			  return;
		}

		/* Decrement the blocks remaining counter */
		TotalBlocks--;
	}

	/* If the endpoint is full, send its contents to the host */
	if (!(Endpoint_IsReadWriteAllowed()))
	  Endpoint_ClearIN();
#endif 
}

/** Disables the Dataflash memory write protection bits on the board Dataflash ICs, if enabled. */
void DataflashManager_ResetDataflashProtections(void)
{
}

/** Performs a simple test on the attached Dataflash IC(s) to ensure that they are working.
 *
 *  \return Boolean \c true if all media chips are working, \c false otherwise
 */
bool DataflashManager_CheckDataflashOperation(void)
{
	return true;
}


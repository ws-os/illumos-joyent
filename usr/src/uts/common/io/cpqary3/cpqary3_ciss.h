/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Copyright (C) 2013 Hewlett-Packard Development Company, L.P.
 * Copyright 2016 Joyent, Inc.
 */

#ifndef	_CPQARY3_CISS_H
#define	_CPQARY3_CISS_H

#ifdef	__cplusplus
extern "C" {
#endif

/* General Boundary Defintions */
#define	CISS_INIT_TIME		90	/* Driver Defined Value */
					/* Duration to Wait for the */
					/* controller initialization */
#define	CISS_SENSEINFOBYTES	384	/* Note that this value may vary */
					/* between host implementations */
#define	CISS_MAXSGENTRIES	64
#define	CISS_MAXREPLYQS		256

/* Command Status Value */
#define	CISS_CMD_SUCCESS			0x00
#define	CISS_CMD_TARGET_STATUS			0x01
#define	CISS_CMD_DATA_UNDERRUN			0x02
#define	CISS_CMD_DATA_OVERRUN			0x03
#define	CISS_CMD_INVALID			0x04
#define	CISS_CMD_PROTOCOL_ERR			0x05
#define	CISS_CMD_HARDWARE_ERR			0x06
#define	CISS_CMD_CONNECTION_LOST		0x07
#define	CISS_CMD_ABORTED			0x08
#define	CISS_CMD_ABORT_FAILED			0x09
#define	CISS_CMD_UNSOLICITED_ABORT		0x0a
#define	CISS_CMD_TIMEOUT			0x0b
#define	CISS_CMD_UNABORTABLE			0x0c

/* Transfer Direction */
#define	CISS_XFER_NONE				0x00
#define	CISS_XFER_WRITE				0x01
#define	CISS_XFER_READ				0x02
#define	CISS_XFER_RSVD				0x03

#define	CISS_ATTR_UNTAGGED			0x00
#define	CISS_ATTR_SIMPLE			0x04
#define	CISS_ATTR_HEADOFQUEUE			0x05
#define	CISS_ATTR_ORDERED			0x06

/* CDB Type */
#define	CISS_TYPE_CMD				0x00
#define	CISS_TYPE_MSG				0x01

/*
 * I2O Space Register Offsets
 *
 * The name "I2O", and these register offsets, appear to be amongst the last
 * vestiges of a long-defunct attempt at standardising mainframe-style I/O
 * channels in the Intel server space: the Intelligent Input/Output (I2O)
 * Architecture Specification.
 *
 * The draft of version 1.5 of this specification, in section "4.2.1.5.1
 * Extensions for PCI", suggests that the following are memory offsets into
 * "the memory region specified by the first base address configuration
 * register indicating memory space (offset 10h, 14h, and so forth)".  These
 * match up with the offsets of the first two BARs in a PCI configuration space
 * type 0 header.
 *
 * The specification also calls out the Inbound Post List FIFO, write-only at
 * offset 40h; the Outbound Post List FIFO, read-only at offset 44h; the
 * Interrupt Status Register, at offset 30h; and the Interrupt Mask Register,
 * at offset 34h.
 *
 * This ill-fated attempt to increase the proprietary complexity of (and
 * presumably, thus, the gross margin on) computer systems is all but extinct.
 * The transport layer of this storage controller is all that's left of their
 * religion.
 */
#define	CISS_I2O_INBOUND_DOORBELL		0x20
#define	CISS_I2O_INTERRUPT_STATUS		0x30
#define	CISS_I2O_INTERRUPT_MASK			0x34
#define	CISS_I2O_INBOUND_POST_Q			0x40
#define	CISS_I2O_OUTBOUND_POST_Q		0x44
#define	CISS_I2O_OUTBOUND_DOORBELL_STATUS	0x9c
#define	CISS_I2O_OUTBOUND_DOORBELL_CLEAR	0xa0
#define	CISS_I2O_SCRATCHPAD			0xb0
#define	CISS_I2O_CFGTBL_CFG_OFFSET		0xb4
#define	CISS_I2O_CFGTBL_MEM_OFFSET		0xb8

/*
 * Rather than make a lot of small mappings for each part of the address
 * space we wish to access, we will make one large mapping.  If more
 * offsets are added to the I2O list above, this space should be extended
 * appropriately.
 */
#define	CISS_I2O_MAP_BASE			0x20
#define	CISS_I2O_MAP_LIMIT			0x100

/*
 * The Scratchpad Register (I2O_SCRATCHPAD) is not mentioned in the CISS
 * specification.  It serves at least two known functions:
 *	- Signalling controller readiness
 *	- Exposing a debugging code when the controller firmware locks up
 */
#define	CISS_SCRATCHPAD_INITIALISED		0xffff0000

/*
 * Outbound Doorbell Register Values.
 *
 * These are read from the Outbound Doorbell Set/Status Register
 * (CISS_I2O_OUTBOUND_DOORBELL_STATUS), but cleared by writing to the Clear
 * Register (CISS_I2O_OUTBOUND_DOORBELL_CLEAR).
 */
#define	CISS_ODR_BIT_INTERRUPT			(1UL << 0)
#define	CISS_ODR_BIT_LOCKUP			(1UL << 1)

/*
 * Inbound Doorbell Register Values.
 *
 * These are written to and read from the Inbound Doorbell Register
 * (CISS_I2O_INBOUND_DOORBELL).
 */
#define	CISS_IDR_BIT_CFGTBL_CHANGE		(1UL << 0)

/*
 * Transport Methods.
 *
 * These bit positions are used in the Configuration Table to detect controller
 * support for a particular method, via "TransportSupport"; to request that the
 * controller enable a particular method, via "HostWrite.TransportRequest"; and
 * to detect whether the controller has acknowledged the request and enabled
 * the desired method, via "TransportActive".
 *
 * See: "9.1 Configuration Table" in the CISS Specification.
 */
#define	CISS_CFGTBL_READY_FOR_COMMANDS		(1UL << 0)
#define	CISS_CFGTBL_XPORT_SIMPLE		(1UL << 1)
#define	CISS_CFGTBL_XPORT_PERFORMANT		(1UL << 2)
#define	CISS_CFGTBL_XPORT_MEMQ			(1UL << 4)

/*
 * In the Simple Transport Method, the Outbound Post Queue register is
 * repeatedly read for notifications of the completion of commands previously
 * submitted to the controller.  These macros help break up the read value into
 * its component fields: the tag number, and whether or not the command
 * completed in error.
 */
#define	CISS_OPQ_READ_TAG(x)			((x) >> 2)
#define	CISS_OPQ_READ_ERROR(x)			((x) & (1UL << 0))

/*
 * STRUCTURES
 * Command List Structure
 */

#pragma pack(1)

/*
 * Structure for Tag field in the controller command structure
 * Bit 0	: Unused
 * Bit 1 	: If set, signifies an error in processing of the command
 * Bits 2 & 3 	: Used by this driver to signify a host of situations
 * Bits 4-31 	: Used by driver to fill in tag and then used by controller
 * Bits 32-63 	: Reserved
 *
 * XXX NOOOOOOOOOOOOOOOOOOO
 */
#define	CISS_CMD_ERROR		0x2
typedef struct cpqary3_tag {
	uint32_t	reserved:1;
	uint32_t	error:1;
	uint32_t	tag_value:30;
	uint32_t	unused;
} cpqary3_tag_t;

typedef union SCSI3Addr {
	struct {
		uint8_t Bus:6;
		uint8_t Mode:2;
		uint8_t Dev;
	} PeripDev;
	struct {
		uint8_t DevMSB:6;
		uint8_t Mode:2;
		uint8_t DevLSB;
	} LogDev;
	struct {
		uint8_t Targ:6;
		uint8_t Mode:2;
		uint8_t Dev:5;
		uint8_t Bus:3;
	} LogUnit;
} SCSI3Addr_t;

typedef struct PhysDevAddr {
	uint32_t    TargetId:24;
	uint32_t    Bus:6;
	uint32_t    Mode:2;
	SCSI3Addr_t Target[2];
} PhysDevAddr_t;

typedef struct LogDevAddr {
	uint32_t	VolId:30;
	uint32_t	Mode:2;
	uint8_t		reserved[4];
} LogDevAddr_t;

typedef union LUNAddr {
	uint8_t		LunAddrBytes[8];
	SCSI3Addr_t	SCSI3Lun[4];
	PhysDevAddr_t	PhysDev;
	LogDevAddr_t	LogDev;
} LUNAddr_t;

typedef struct CommandListHeader {
	uint8_t		ReplyQueue;
	uint8_t		SGList;
	uint16_t	SGTotal;
	cpqary3_tag_t	Tag;
	LUNAddr_t	LUN;			/* 20 */
} CommandListHeader_t;

typedef struct RequestBlock {
	uint8_t	CDBLen;
	struct {
		uint8_t	Type:3;
		uint8_t	Attribute:3;
		uint8_t	Direction:2;
	} Type;
	uint16_t	Timeout;
	uint8_t		CDB[16];		/* 20 */
} RequestBlock_t;

typedef struct ErrDescriptor {
	uint64_t	Addr;
	uint32_t	Len;			/* 12 */
} ErrDescriptor_t;

typedef struct SGDescriptor {
	uint64_t	Addr;
	uint32_t	Len;
	uint32_t	Ext;			/* 16 */
} SGDescriptor_t;

typedef struct CommandList {
	CommandListHeader_t Header;		/* 20 */
	RequestBlock_t Request;			/* 20, 40 */
	ErrDescriptor_t ErrDesc;		/* 12, 52 */
	SGDescriptor_t SG[CISS_MAXSGENTRIES];	/* 16*SG_MAXENTRIES=512, 564 */
} CommandList_t;

typedef union MoreErrInfo {
	struct {
		uint8_t		Reserved[3];
		uint8_t		Type;
		uint32_t	ErrorInfo;
	} Common_Info;
	struct {
		uint8_t		Reserved[2];
		uint8_t		offense_size;
		uint8_t		offense_num;
		uint32_t	offense_value;
	} Invalid_Cmd;
} MoreErrInfo_t;

typedef struct ErrorInfo {
	uint8_t		ScsiStatus;
	uint8_t		SenseLen;
	uint16_t	CommandStatus;
	uint32_t	ResidualCnt;
	MoreErrInfo_t	MoreErrInfo;
	uint8_t		SenseInfo[CISS_SENSEINFOBYTES]; /* 256 + 24 = 280 */
} ErrorInfo_t;

/* Configuration Table Structure */
typedef struct HostWrite {
	uint32_t	TransportRequest;
	uint32_t	Upper32Addr;
	uint32_t	CoalIntDelay;
	uint32_t	CoalIntCount;
} HostWrite_t;

typedef struct CfgTable {
	uint8_t		Signature[4];
	uint32_t	SpecValence;
	uint32_t	TransportSupport;
	uint32_t	TransportActive;
	HostWrite_t	HostWrite;
	uint32_t	CmdsOutMax;
	uint32_t	BusTypes;
	uint32_t	TransportMethodOffset;
	uint8_t		ServerName[16];
	uint32_t	HeartBeat;
	/* PERF */
	uint32_t	HostDrvrSupport;	/* 0x40 offset from cfg table */
	uint32_t	MaxSGElements;		/* 0x44 offset from cfg table */
	uint32_t	MaxLunSupport;		/* 0x48 offset from cfg table */
	uint32_t	MaxPhyDevSupport;	/* 0x4C offset from cfg table */
	uint32_t	MaxPhyDrvPerLun;	/* 0x50 offset from cfg table */
	uint32_t	MaxPerfModeCmdsOutMax;	/* 0x54 offset from cfg table */
	uint32_t	MaxBlockFetchCount;	/* 0x58 offset from cfg table */
	/* PERF */
} CfgTable_t;

typedef struct CfgTrans_Perf {
	uint32_t	BlockFetchCnt[8];
	uint32_t	ReplyQSize;
	uint32_t	ReplyQCount;
	uint32_t	ReplyQCntrAddrLow32;
	uint32_t	ReplyQCntrAddrHigh32;
	uint32_t	ReplyQAddr0Low32;
	uint32_t	ReplyQAddr0High32;
} CfgTrans_Perf_t;

typedef struct CfgTrans_MemQ {
	uint32_t	BlockFetchCnt[8];
	uint32_t	CmdQSize;
	uint32_t	CmdQOffset;
	uint32_t	ReplyQSize;
	uint32_t	ReplyQCount;
	uint64_t	ReplyQCntrAddr;
	uint64_t	ReplyQAddr[CISS_MAXREPLYQS];
} CfgTrans_MemQ_t;

typedef union CfgTrans {
	CfgTrans_Perf_t	*Perf;
	CfgTrans_MemQ_t	*MemQ;
} CfgTrans_t;

#pragma pack()

#ifdef	__cplusplus
}
#endif

#endif /* _CPQARY3_CISS_H */

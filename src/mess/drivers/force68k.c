// license:BSD-3-Clause
// copyright-holders:Joakim Larsson Edström
/***************************************************************************

    Force SYS68K CPU-1/CPU-6 VME SBC drivers

    13/06/2015

 The info found on the links below is for a later revisions of the board I have
 but I hope it is somewhat compatible so I can get it up and running at least. 
 My CPU-1 board has proms from 1983 and no rev markings so probably the original.

 http://bitsavers.trailing-edge.com/pdf/forceComputers/1988_Force_VMEbus_Products.pdf
 http://www.artisantg.com/info/P_wUovN.pdf

 Some info from those documents:

Address Map
----------------------------------------------------------
Address Range     Description
----------------------------------------------------------
000 000 - 000 007 Initialisation vectors from system EPROM
000 008 - 01F FFF Dynamic RAM on CPU-1 B
000 008 - 07F FFF Dynamic RAM on CPU-1 D
080 008 - 09F FFF SYSTEM EPROM Area
OAO 000 - OBF FFF USER EPROMArea
0C0 041 - 0C0 043 ACIA (P3) Host
0C0 080 - 0C0 082 ACIA (P4) Terminal
0C0 101 - 0C0 103 ACIA (P3) Remote
0C0 401 - 0C0 42F RTC 
OEO 001 - 0E0 035 PI/T
OEO 200 - 0E0 2FF FPU
OEO 300 - 0E0 300 Reset Off
OEO 380 - 0E0 380 Reset On
100 000 - FEF FFF VMEbus addresses (A24)
FFO 000 - FFF FFF VMEbus Short I/O (A16)
----------------------------------------------------------

Interrupt sources
----------------------------------------------------------
Description                  Device  Lvl  IRQ    VME board
                             /Board      Vector  Address
----------------------------------------------------------
On board Sources
 ABORT                        Switch  7    31
 Real Time Clock (RTC)        58167A  6    30
 Parallel/Timer (PI/T)        68230   5    29
 Terminal ACIA                6850    4    28
 Remote ACIA                  6850    3    27
 Host ACIA                    6850    2    26
 ACFAIL, SYSFAIL              VME     5    29
Off board Sources (other VME boards)
 6 Port Serial I/O board      SIO     4    64-75  0xb00000
 8 Port Serial I/O board      ISIO    4    76-83  0x960000
 Disk Controller              WFC     3    119    0xb01000
 SCSI Controller              ISCSI   4    119    0xa00000
 Slot 1 Controller Board      ASCU    7    31     0xb02000
----------------------------------------------------------

10. The VMEbus
---------------
The implemented VMEbus Interface includes 24 address, 16 data, 
6 address modifier and the asynchronous control signals.
A single level bus arbiter is provided to build multi master 
systems. In addition to the bus arbiter, a separate slave bus 
arbitration allows selection of the arbitration level (0-3).

The address modifier range .,Short 110 Access« can be selected 
via a jumper for variable system generation. The 7 interrupt 
request levels of the VMEbus are fully supported from the 
SYS68K1CPU-1 B/D. For multi-processing, each IRQ signal can be 
enabled/disabled via a jumper field.

Additionally, the SYS68K1CPU-1 B/D supports the ACFAIL, SYSRESET, 
SYSFAIL and SYSCLK signal (16 MHz).


Based on the 68ksbc.c

    TODO:
    - Memory map
    - Dump ROM:s
    - Add 3 x ACIA6850 
    - Add 1 x 68230 Motorola, Parallel Interface / Timer
    - Add 1 x MM58167A RTC
    - Add 1 x Abort Switch  
    - Add configurable serial connector between ACIA:s and 
      - Real terminal emulator
      - Debug console
    - VME bus driver


****************************************************************************/

#include "emu.h"
//#include "bus/rs232/rs232.h"
#include "cpu/m68000/m68000.h"/
#include "machine/mm58167.h"
//#include "machine/6850acia.h"
//#include "machine/clock.h"

class force68k_state : public driver_device
{
public:
	force68k_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_rtc(*this, "rtc")
//		m_acia1(*this, "acia1")
//		m_acia2(*this, "acia2")
//		m_acia3(*this, "acia3")
	{
	}

//	DECLARE_WRITE_LINE_MEMBER(write_acia_clock);

private:
	required_device<cpu_device> m_maincpu;
	required_device<mm58167_device> m_rtc;
//	required_device<acia6850_device> m_acia1;
//	required_device<acia6850_device> m_acia2;
//	required_device<acia6850_device> m_acia3;
};

static ADDRESS_MAP_START(force68k_mem, AS_PROGRAM, 16, force68k_state)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x000007) AM_ROM /* Vectors mapped from System EPROM */
	AM_RANGE(0x000008, 0x01ffff) AM_RAM /* DRAM */
	AM_RANGE(0x080008, 0x09ffff) AM_ROM /* System EPROM Area */
	AM_RANGE(0x0e0401, 0x0e0421) AM_DEVREADWRITE8("rtc", mm58167_device, read, write, 0x00ff)
//	AM_RANGE(0x0e0000, 0x0e0001) AM_DEVREADWRITE8("acia", acia6850_device, status_r, control_w, 0x00ff)
//	AM_RANGE(0x0e0002, 0x0e0003) AM_DEVREADWRITE8("acia", acia6850_device, data_r, data_w, 0x00ff)
//	AM_RANGE(0x0a0000, 0x0bffff) AM_ROM /* User EPROM Area   */
//      AM_RANGE(0x0e0000, 0x0fffff) AM_READWRITE_PORT /* IO interfaces */
//      AM_RANGE(0x100000, 0xfeffff) /* VMEbus Rev B addresses (24 bits) */
//      AM_RANGE(0xff0000, 0xffffff) /* VMEbus Rev B addresses (16 bits) */
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( force68k )
INPUT_PORTS_END


#if 0
WRITE_LINE_MEMBER(force68k_state::write_acia_clock)
{
	m_acia->write_txc(state);
	m_acia->write_rxc(state);
}
#endif

static MACHINE_CONFIG_START( forcecpu1, force68k_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, 8000000) 
	MCFG_CPU_PROGRAM_MAP(force68k_mem)

/*
	MCFG_DEVICE_ADD("acia", ACIA6850, 0)
	MCFG_ACIA6850_TXD_HANDLER(DEVWRITELINE("rs232", rs232_port_device, write_txd))
	MCFG_ACIA6850_RTS_HANDLER(DEVWRITELINE("rs232", rs232_port_device, write_rts))

	MCFG_RS232_PORT_ADD("rs232", default_rs232_devices, "terminal")
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE("acia", acia6850_device, write_rxd))
	MCFG_RS232_CTS_HANDLER(DEVWRITELINE("acia", acia6850_device, write_cts))

	MCFG_DEVICE_ADD("acia_clock", CLOCK, 153600)
	MCFG_CLOCK_SIGNAL_HANDLER(WRITELINE(force68k_state, write_acia_clock))
*/
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( forcecpu6, force68k_state )
	MCFG_CPU_ADD("maincpu", M68000, 8000000)  /* Jumper B10 Mode B */
	MCFG_CPU_PROGRAM_MAP(force68k_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( forcecpu6a, force68k_state )
	MCFG_CPU_ADD("maincpu", M68000, 12500000) /* Jumper B10 Mode A */
	MCFG_CPU_PROGRAM_MAP(force68k_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( forcecpu6v, force68k_state )
	MCFG_CPU_ADD("maincpu", M68010, 8000000)  /* Jumper B10 Mode B */
	MCFG_CPU_PROGRAM_MAP(force68k_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( forcecpu6va, force68k_state )
	MCFG_CPU_ADD("maincpu", M68010, 12500000) /* Jumper B10 Mode A */
	MCFG_CPU_PROGRAM_MAP(force68k_mem)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( forcecpu6vb, force68k_state )
	MCFG_CPU_ADD("maincpu", M68010, 12500000) /* Jumper B10 Mode A */
	MCFG_CPU_PROGRAM_MAP(force68k_mem)

MACHINE_CONFIG_END

/* ROM definition */
ROM_START( force68k_rom )
	ROM_REGION(0x1000000, "maincpu", 0)
//	ROM_LOAD( "forcesys68kV1.0L.bin", 0x0000, 0x2f78, CRC(20a8d0d0) SHA1(544fd8bd8ed017115388c8b0f7a7a59a32253e43) )
ROM_END

/* Driver */

/*    YEAR  NAME          PARENT  COMPAT   MACHINE         INPUT     CLASS          INIT COMPANY                  FULLNAME          FLAGS */
COMP( 1983, force68k_rom, 0,      0,       forcecpu1,      force68k, driver_device,  0,  "Force Computers Gmbh",  "SYS68K/CPU-1",   GAME_IS_SKELETON )
COMP( 1989, force68k_rom, 0,      0,       forcecpu6,      force68k, driver_device,  0,  "Force Computers Gmbh",  "SYS68K/CPU-6",   GAME_IS_SKELETON )
COMP( 1989, force68k_rom, 0,      0,       forcecpu6a,     force68k, driver_device,  0,  "Force Computers Gmbh",  "SYS68K/CPU-6a",  GAME_IS_SKELETON )
COMP( 1989, force68k_rom, 0,      0,       forcecpu6v,     force68k, driver_device,  0,  "Force Computers Gmbh",  "SYS68K/CPU-6v",  GAME_IS_SKELETON )
COMP( 1989, force68k_rom, 0,      0,       forcecpu6va,    force68k, driver_device,  0,  "Force Computers Gmbh",  "SYS68K/CPU-6va", GAME_IS_SKELETON )
COMP( 1989, force68k_rom, 0,      0,       forcecpu6vb,    force68k, driver_device,  0,  "Force Computers Gmbh",  "SYS68K/CPU-6vb", GAME_IS_SKELETON )

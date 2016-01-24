// license:GPL2+
// copyright-holders:Felipe Sanches
/*************************************************************************
  
  This is a driver for a gambling board with a yet unknown name.
  The PCB is labeled with: WU- MARY-1A
  And there's a text string in the ROM that says: "Music by: SunKiss Chen"

  Driver by Felipe Sanches

  TODO:
  * Figure out where exactly all devices are mapped to (the devices are
    2 sound chips, the 2kb SRAM, the 8bit dipswitches,
    31 LEDs, 13 modules of double-digit 7-seg displays and 4 push-buttons).
  * we may also have user inputs from the coin slot and from the
    cabinet buttons, for making bets.
**************************************************************************/

#include "emu.h"
#include "cpu/mcs51/mcs51.h"
#include "sound/ay8910.h"
#include "machine/i8279.h"
#include "marywu.lh"

class marywu_state : public driver_device
{
public:
    marywu_state(const machine_config &mconfig, device_type type, const char *tag)
        : driver_device(mconfig, type, tag)
    { }

    DECLARE_WRITE8_MEMBER(display_7seg_data_w);
    DECLARE_WRITE8_MEMBER(multiplex_7seg_w);
private:
    uint8_t m_selected_7seg_module;
};

WRITE8_MEMBER( marywu_state::multiplex_7seg_w )
{
        m_selected_7seg_module = data;
}

WRITE8_MEMBER( marywu_state::display_7seg_data_w )
{
    static const UINT8 patterns[16] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7c, 0x07, 0x7f, 0x67, 0, 0, 0, 0, 0, 0 }; // HEF4511BP (7 seg display driver)

    output_set_digit_value(2 * m_selected_7seg_module + 0, patterns[data & 0x0F]);
    output_set_digit_value(2 * m_selected_7seg_module + 1, patterns[(data >> 4) & 0x0F]);
}

static ADDRESS_MAP_START( program_map, AS_PROGRAM, 8, marywu_state )
    AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( io_map, AS_IO, 8, marywu_state )
    AM_RANGE(0x8000, 0x87ff) AM_MIRROR(0x0100) AM_RAM /* HM6116: 2kbytes of Static RAM */
    AM_RANGE(0xb000, 0xb000) AM_MIRROR(0x0ffe) AM_DEVREADWRITE("i8279", i8279_device, data_r, data_w)
    AM_RANGE(0xb001, 0xb001) AM_MIRROR(0x0ffe) AM_DEVREADWRITE("i8279", i8279_device, status_r, cmd_w)
    AM_RANGE(0x9000, 0x9000) AM_MIRROR(0x0ffc) AM_DEVWRITE("ay1", ay8910_device, data_address_w)
    AM_RANGE(0x9001, 0x9001) AM_MIRROR(0x0ffc) AM_DEVREADWRITE("ay1", ay8910_device, data_r, data_w)
    AM_RANGE(0x9002, 0x9002) AM_MIRROR(0x0ffc) AM_DEVWRITE("ay2", ay8910_device, data_address_w)
    AM_RANGE(0x9003, 0x9003) AM_MIRROR(0x0ffc) AM_DEVREADWRITE("ay2", ay8910_device, data_r, data_w)
    AM_RANGE(MCS51_PORT_P0, MCS51_PORT_P3) AM_NOP /* FIX-ME! I am ignoring port accesses for a while. */
ADDRESS_MAP_END

static MACHINE_CONFIG_START( marywu , marywu_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu", I80C31, XTAL_10_738635MHz) //actual CPU is a Winbond w78c31b-24
    MCFG_CPU_PROGRAM_MAP(program_map)
    MCFG_CPU_IO_MAP(io_map)

    /* Keyboard & display interface */
    MCFG_DEVICE_ADD("i8279", I8279, XTAL_10_738635MHz) /* should it be perhaps a fraction of the XTAL clock ? */
    MCFG_I8279_OUT_SL_CB(WRITE8(marywu_state, multiplex_7seg_w))          // select  block of 7seg modules by multiplexing the SL scan lines
//    MCFG_I8279_IN_RL_CB(READ8(marywu_state, marywu_kbd_r))                  // kbd RL lines
    MCFG_I8279_OUT_DISP_CB(WRITE8(marywu_state, display_7seg_data_w))

    /* Video */
    MCFG_DEFAULT_LAYOUT(layout_marywu)

    /* sound hardware */
    MCFG_SPEAKER_STANDARD_MONO("mono")
    MCFG_SOUND_ADD("ay1", AY8910, XTAL_10_738635MHz) /* should it be perhaps a fraction of the XTAL clock ? */
    MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
    
    MCFG_SOUND_ADD("ay2", AY8910, XTAL_10_738635MHz) /* should it be perhaps a fraction of the XTAL clock ? */
    MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_CONFIG_END

ROM_START( marywu )
	ROM_REGION( 0x8000, "maincpu", 0 )
    ROM_LOAD( "marywu_sunkiss_chen.rom", 0x0000, 0x8000, CRC(11f67c7d) SHA1(9c1fd1a5cc6e2b0d675f0217aa8ff21c30609a0c) )
ROM_END

/*    YEAR  NAME       PARENT   MACHINE   INPUT     STATE          INIT   ROT    COMPANY       FULLNAME          FLAGS  */
GAME( ????, marywu,    0,       marywu,   0,        driver_device, 0,     ROT0, "<unknown>", "<unknown> Labeled 'WU- MARY-1A' Music by: SunKiss Chen", MACHINE_NOT_WORKING )

// license:BSD-3-Clause
// copyright-holders:AJR
/****************************************************************************

    Skeleton driver for ADM-31 terminal.

    The ADM-31 and ADM-42 Data Display Terminals were Lear Siegler, Inc.'s
    first microprocessor-based video terminals, introduced in 1978 as the
    respective successors to their earlier ADM-1A and ADM-2 "smart"
    terminals. The original ADM-31 model was apparently rebranded in 1980
    as the ADM-31 Intermediate Terminal, and the ADM-32 was released a few
    months later.

    While the ADM-31 and ADM-32 only support 2 pages of display memory, the
    ADM-42 could be upgraded to 8. Enhancements over the ADM-31 offered by
    both the ADM-42 and ADM-32 include a status line, a larger monitor and
    a detachable keyboard. Several other expansion options were offered for
    the ADM-42, including synchronous serial and parallel printer ports.

****************************************************************************/

#include "emu.h"
//#include "bus/rs232/rs232.h"
#include "cpu/m6800/m6800.h"
#include "machine/com8116.h"
#include "machine/input_merger.h"
#include "machine/6821pia.h"
#include "machine/6850acia.h"
#include "video/tms9927.h"
#include "screen.h"

class adm31_state : public driver_device
{
public:
	adm31_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_acia(*this, "acia%u", 1U)
		, m_vtac(*this, "vtac")
		, m_chargen(*this, "chargen")
	{
	}

	void adm31(machine_config &mconfig);

protected:
	virtual void machine_start() override;

private:
	u32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	void mem_map(address_map &map);

	required_device<cpu_device> m_maincpu;
	required_device_array<acia6850_device, 2> m_acia;
	required_device<crt5027_device> m_vtac;
	required_region_ptr<u8> m_chargen;
};


void adm31_state::machine_start()
{
}


u32 adm31_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	return 0;
}


void adm31_state::mem_map(address_map &map)
{
	map(0x0000, 0x03ff).ram();
	map(0x7000, 0x7000).nopw();
	map(0x7800, 0x7803).rw("pia", FUNC(pia6821_device::read), FUNC(pia6821_device::write));
	map(0x7900, 0x7900).portr("S5");
	map(0x7a00, 0x7a01).rw(m_acia[0], FUNC(acia6850_device::read), FUNC(acia6850_device::write));
	map(0x7c00, 0x7c01).rw(m_acia[1], FUNC(acia6850_device::read), FUNC(acia6850_device::write));
	map(0x7d00, 0x7d00).portr("S6");
	map(0x7e00, 0x7e00).portr("S4");
	map(0x7f00, 0x7f0f).rw(m_vtac, FUNC(crt5027_device::read), FUNC(crt5027_device::write));
	map(0x8000, 0x8fff).ram();
	map(0xe000, 0xffff).rom().region("program", 0);
}


static INPUT_PORTS_START(adm31)
	PORT_START("7D00")
	PORT_BIT(0xff, IP_ACTIVE_LOW, IPT_UNKNOWN)

	PORT_START("S4")
	PORT_DIPNAME(0x01, 0x00, "Break Key") PORT_DIPLOCATION("S4:1")
	PORT_DIPSETTING(0x01, "Disable")
	PORT_DIPSETTING(0x00, "Enable")
	PORT_DIPNAME(0x02, 0x02, "Refresh Rate") PORT_DIPLOCATION("S4:2")
	PORT_DIPSETTING(0x00, "50 Hz")
	PORT_DIPSETTING(0x02, "60 Hz")
	PORT_DIPNAME(0x1c, 0x14, "Modem Port") PORT_DIPLOCATION("S4:3,4,5")
	PORT_DIPSETTING(0x00, "7 DB, EP, 2 SB")
	PORT_DIPSETTING(0x04, "7 DB, OP, 2 SB")
	PORT_DIPSETTING(0x08, "7 DB, EP, 1 SB")
	PORT_DIPSETTING(0x0c, "7 DB, OP, 1 SB")
	PORT_DIPSETTING(0x10, "8 DB, NP, 2 SB")
	PORT_DIPSETTING(0x14, "8 DB, NP, 1 SB")
	PORT_DIPSETTING(0x18, "8 DB, EP, 1 SB")
	PORT_DIPSETTING(0x00, "8 DB, OP, 1 SB")
	PORT_DIPNAME(0x20, 0x00, DEF_STR(Unused)) PORT_DIPLOCATION("S4:6")
	PORT_DIPSETTING(0x20, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x40, 0x00, "Transmission") PORT_DIPLOCATION("S4:7")
	PORT_DIPSETTING(0x00, "Block Mode")
	PORT_DIPSETTING(0x40, "Conversation Mode")
	PORT_DIPNAME(0x80, 0x00, "Duplex") PORT_DIPLOCATION("S4:8")
	PORT_DIPSETTING(0x00, "Full Duplex")
	PORT_DIPSETTING(0x80, "Half Duplex")

	PORT_START("S5")
	PORT_DIPNAME(0x01, 0x00, DEF_STR(Unused)) PORT_DIPLOCATION("S5:1")
	PORT_DIPSETTING(0x01, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x02, 0x00, DEF_STR(Unused)) PORT_DIPLOCATION("S5:2")
	PORT_DIPSETTING(0x02, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x1c, 0x14, "Printer Port") PORT_DIPLOCATION("S5:3,4,5")
	PORT_DIPSETTING(0x00, "7 DB, EP, 2 SB")
	PORT_DIPSETTING(0x04, "7 DB, OP, 2 SB")
	PORT_DIPSETTING(0x08, "7 DB, EP, 1 SB")
	PORT_DIPSETTING(0x0c, "7 DB, OP, 1 SB")
	PORT_DIPSETTING(0x10, "8 DB, NP, 2 SB")
	PORT_DIPSETTING(0x14, "8 DB, NP, 1 SB")
	PORT_DIPSETTING(0x18, "8 DB, EP, 1 SB")
	PORT_DIPSETTING(0x00, "8 DB, OP, 1 SB")
	PORT_DIPNAME(0x20, 0x00, DEF_STR(Unused)) PORT_DIPLOCATION("S5:6")
	PORT_DIPSETTING(0x20, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x40, 0x00, DEF_STR(Unused)) PORT_DIPLOCATION("S5:7")
	PORT_DIPSETTING(0x40, DEF_STR(Off))
	PORT_DIPSETTING(0x00, DEF_STR(On))
	PORT_DIPNAME(0x80, 0x00, "Printer Port Buffer") PORT_DIPLOCATION("S5:8")
	PORT_DIPSETTING(0x80, "Disable")
	PORT_DIPSETTING(0x00, "Enable")

	PORT_START("S6")
	PORT_DIPNAME(0x7f, 0x00, "Polling Address") PORT_DIPLOCATION("S6:1,2,3,4,5,6,7")
	PORT_DIPSETTING(0x00, "00")
	PORT_DIPSETTING(0x01, "01")
	PORT_DIPSETTING(0x02, "02")
	PORT_DIPSETTING(0x03, "03")
	PORT_DIPSETTING(0x04, "04")
	PORT_DIPSETTING(0x05, "05")
	PORT_DIPSETTING(0x06, "06")
	PORT_DIPSETTING(0x07, "07")
	PORT_DIPSETTING(0x08, "08")
	PORT_DIPSETTING(0x09, "09")
	PORT_DIPSETTING(0x0a, "0A")
	PORT_DIPSETTING(0x0b, "0B")
	PORT_DIPSETTING(0x0c, "0C")
	PORT_DIPSETTING(0x0d, "0D")
	PORT_DIPSETTING(0x0e, "0E")
	PORT_DIPSETTING(0x0f, "0F")
	PORT_DIPSETTING(0x10, "10")
	PORT_DIPSETTING(0x11, "11")
	PORT_DIPSETTING(0x12, "12")
	PORT_DIPSETTING(0x13, "13")
	PORT_DIPSETTING(0x14, "14")
	PORT_DIPSETTING(0x15, "15")
	PORT_DIPSETTING(0x16, "16")
	PORT_DIPSETTING(0x17, "17")
	PORT_DIPSETTING(0x18, "18")
	PORT_DIPSETTING(0x19, "19")
	PORT_DIPSETTING(0x1a, "1A")
	PORT_DIPSETTING(0x1b, "1B")
	PORT_DIPSETTING(0x1c, "1C")
	PORT_DIPSETTING(0x1d, "1D")
	PORT_DIPSETTING(0x1e, "1E")
	PORT_DIPSETTING(0x1f, "1F")
	PORT_DIPSETTING(0x20, "20")
	PORT_DIPSETTING(0x21, "21")
	PORT_DIPSETTING(0x22, "22")
	PORT_DIPSETTING(0x23, "23")
	PORT_DIPSETTING(0x24, "24")
	PORT_DIPSETTING(0x25, "25")
	PORT_DIPSETTING(0x26, "26")
	PORT_DIPSETTING(0x27, "27")
	PORT_DIPSETTING(0x28, "28")
	PORT_DIPSETTING(0x29, "29")
	PORT_DIPSETTING(0x2a, "2A")
	PORT_DIPSETTING(0x2b, "2B")
	PORT_DIPSETTING(0x2c, "2C")
	PORT_DIPSETTING(0x2d, "2D")
	PORT_DIPSETTING(0x2e, "2E")
	PORT_DIPSETTING(0x2f, "2F")
	PORT_DIPSETTING(0x30, "30")
	PORT_DIPSETTING(0x31, "31")
	PORT_DIPSETTING(0x32, "32")
	PORT_DIPSETTING(0x33, "33")
	PORT_DIPSETTING(0x34, "34")
	PORT_DIPSETTING(0x35, "35")
	PORT_DIPSETTING(0x36, "36")
	PORT_DIPSETTING(0x37, "37")
	PORT_DIPSETTING(0x38, "38")
	PORT_DIPSETTING(0x39, "39")
	PORT_DIPSETTING(0x3a, "3A")
	PORT_DIPSETTING(0x3b, "3B")
	PORT_DIPSETTING(0x3c, "3C")
	PORT_DIPSETTING(0x3d, "3D")
	PORT_DIPSETTING(0x3e, "3E")
	PORT_DIPSETTING(0x3f, "3F")
	PORT_DIPSETTING(0x40, "40")
	PORT_DIPSETTING(0x41, "41")
	PORT_DIPSETTING(0x42, "42")
	PORT_DIPSETTING(0x43, "43")
	PORT_DIPSETTING(0x44, "44")
	PORT_DIPSETTING(0x45, "45")
	PORT_DIPSETTING(0x46, "46")
	PORT_DIPSETTING(0x47, "47")
	PORT_DIPSETTING(0x48, "48")
	PORT_DIPSETTING(0x49, "49")
	PORT_DIPSETTING(0x4a, "4A")
	PORT_DIPSETTING(0x4b, "4B")
	PORT_DIPSETTING(0x4c, "4C")
	PORT_DIPSETTING(0x4d, "4D")
	PORT_DIPSETTING(0x4e, "4E")
	PORT_DIPSETTING(0x4f, "4F")
	PORT_DIPSETTING(0x50, "50")
	PORT_DIPSETTING(0x51, "51")
	PORT_DIPSETTING(0x52, "52")
	PORT_DIPSETTING(0x53, "53")
	PORT_DIPSETTING(0x54, "54")
	PORT_DIPSETTING(0x55, "55")
	PORT_DIPSETTING(0x56, "56")
	PORT_DIPSETTING(0x57, "57")
	PORT_DIPSETTING(0x58, "58")
	PORT_DIPSETTING(0x59, "59")
	PORT_DIPSETTING(0x5a, "5A")
	PORT_DIPSETTING(0x5b, "5B")
	PORT_DIPSETTING(0x5c, "5C")
	PORT_DIPSETTING(0x5d, "5D")
	PORT_DIPSETTING(0x5e, "5E")
	PORT_DIPSETTING(0x5f, "5F")
	PORT_DIPSETTING(0x60, "60")
	PORT_DIPSETTING(0x61, "61")
	PORT_DIPSETTING(0x62, "62")
	PORT_DIPSETTING(0x63, "63")
	PORT_DIPSETTING(0x64, "64")
	PORT_DIPSETTING(0x65, "65")
	PORT_DIPSETTING(0x66, "66")
	PORT_DIPSETTING(0x67, "67")
	PORT_DIPSETTING(0x68, "68")
	PORT_DIPSETTING(0x69, "69")
	PORT_DIPSETTING(0x6a, "6A")
	PORT_DIPSETTING(0x6b, "6B")
	PORT_DIPSETTING(0x6c, "6C")
	PORT_DIPSETTING(0x6d, "6D")
	PORT_DIPSETTING(0x6e, "6E")
	PORT_DIPSETTING(0x6f, "6F")
	PORT_DIPSETTING(0x70, "70")
	PORT_DIPSETTING(0x71, "71")
	PORT_DIPSETTING(0x72, "72")
	PORT_DIPSETTING(0x73, "73")
	PORT_DIPSETTING(0x74, "74")
	PORT_DIPSETTING(0x75, "75")
	PORT_DIPSETTING(0x76, "76")
	PORT_DIPSETTING(0x77, "77")
	PORT_DIPSETTING(0x78, "78")
	PORT_DIPSETTING(0x79, "79")
	PORT_DIPSETTING(0x7a, "7A")
	PORT_DIPSETTING(0x7b, "7B")
	PORT_DIPSETTING(0x7c, "7C")
	PORT_DIPSETTING(0x7d, "7D")
	PORT_DIPSETTING(0x7e, "7E")
	PORT_DIPSETTING(0x7f, "7F")
	PORT_DIPNAME(0x80, 0x80, "Polling Option") PORT_DIPLOCATION("S6:8")
	PORT_DIPSETTING(0x80, "Disable")
	PORT_DIPSETTING(0x00, "Enable")
INPUT_PORTS_END

void adm31_state::adm31(machine_config &config)
{
	M6800(config, m_maincpu, 19.584_MHz_XTAL / 20);
	m_maincpu->set_addrmap(AS_PROGRAM, &adm31_state::mem_map);

	INPUT_MERGER_ANY_HIGH(config, "mainirq").output_handler().set_inputline(m_maincpu, M6800_IRQ_LINE);

	PIA6821(config, "pia");

	ACIA6850(config, m_acia[0]);
	m_acia[0]->irq_handler().set("mainirq", FUNC(input_merger_device::in_w<0>));

	ACIA6850(config, m_acia[1]);
	m_acia[0]->irq_handler().set("mainirq", FUNC(input_merger_device::in_w<1>));

	com8116_device &brg(COM8116(config, "brg", 5.0688_MHz_XTAL));
	brg.fr_handler().set(m_acia[0], FUNC(acia6850_device::write_rxc));
	brg.fr_handler().append(m_acia[0], FUNC(acia6850_device::write_txc));
	brg.ft_handler().set(m_acia[1], FUNC(acia6850_device::write_rxc));
	brg.ft_handler().append(m_acia[1], FUNC(acia6850_device::write_txc));

	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_raw(19.584_MHz_XTAL, 1020, 0, 800, 320, 0, 288);
	screen.set_screen_update(FUNC(adm31_state::screen_update));

	CRT5027(config, m_vtac, 19.584_MHz_XTAL / 10);
	m_vtac->set_screen("screen");
	m_vtac->set_char_width(10);
}


ROM_START(adm31)
	ROM_REGION(0x2000, "program", 0)
	ROM_LOAD("adm-31-u62.bin", 0x0000, 0x0800, CRC(57e557a5) SHA1(cb3021ab570c279cbaa673b5de8fa1ca9eb48188))
	ROM_LOAD("adm-31-u63.bin", 0x0800, 0x0800, CRC(1268a59c) SHA1(f0cd8562e0d5faebf84d8decaa848ff28f2ac637))
	ROM_LOAD("adm-31-u64.bin", 0x1000, 0x0800, CRC(8939fa00) SHA1(00f6a8a49e51a9501cd9d1e2aae366fb070a5a1d))
	ROM_LOAD("adm-31-u65.bin", 0x1800, 0x0800, CRC(53e4e2f1) SHA1(bf30241815c790de3354e1acfe84e760c889cbb1))

	ROM_REGION(0x0800, "chargen", 0)
	ROM_LOAD("chargen.bin", 0x0000, 0x0800, NO_DUMP)
ROM_END


COMP(1978, adm31, 0, 0, adm31, adm31, adm31_state, empty_init, "Lear Siegler", "ADM-31 Data Display Terminal", MACHINE_IS_SKELETON)

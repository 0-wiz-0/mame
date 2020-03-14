// license:BSD-3-Clause
// copyright-holders:Olivier Galibert

// Yamaha KS0164/30B, rompler/dsp combo

#include "emu.h"
#include "ks0164.h"


DEFINE_DEVICE_TYPE(KS0164, ks0164_device, "ks0164", "Samsung KS0164 wavetable chip")

ks0164_device::ks0164_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, KS0164, tag, owner, clock),
	  device_sound_interface(mconfig, *this),
	  device_memory_interface(mconfig, *this),
	  m_mem_region(*this, DEVICE_SELF),
	  m_cpu(*this, "cpu"),
	  m_mem_config("mem", ENDIANNESS_BIG, 16, 23)
{
}

device_memory_interface::space_config_vector ks0164_device::memory_space_config() const
{
	return space_config_vector {
		std::make_pair(0, &m_mem_config)
	};
}

void ks0164_device::device_add_mconfig(machine_config &config)
{
	KS0164CPU(config, m_cpu, DERIVED_CLOCK(1, 6));
	m_cpu->set_addrmap(AS_PROGRAM, &ks0164_device::cpu_map);
}

void ks0164_device::device_start()
{
	if(!has_configured_map(0) && m_mem_region) {
		u32 size = m_mem_region->bytes();
		u32 rend = size-1;

		// Round up to the nearest power-of-two-minus-one
		u32 rmask = rend;
		rmask |= rmask >> 1;
		rmask |= rmask >> 2;
		rmask |= rmask >> 4;
		rmask |= rmask >> 8;
		rmask |= rmask >> 16;
		// Mirror over the high bits.  rmask is a
		// power-of-two-minus-one, so the xor works
		space().install_rom(0, rend, ((1 << 23) - 1) ^ rmask, m_mem_region->base());
	}

	m_stream = stream_alloc(0, 2, 44100);
	m_mem_cache = space().cache<1, 0, ENDIANNESS_BIG>();
}

void ks0164_device::device_reset()
{
	m_bank1_select = 0;
	m_bank1_base = 0;
	m_bank2_select = 0;
	m_bank2_base = 0;
}

u16 ks0164_device::vec_r(offs_t offset, u16 mem_mask)
{
	return m_mem_cache->read_word(offset << 1, mem_mask);
}

u16 ks0164_device::rom_r(offs_t offset, u16 mem_mask)
{
	return m_mem_cache->read_word((offset << 1) + 0x80, mem_mask);
}

u16 ks0164_device::bank1_r(offs_t offset, u16 mem_mask)
{
	return m_mem_cache->read_word(((offset << 1) & 0x3fff) | m_bank1_base, mem_mask);
}

void ks0164_device::bank1_w(offs_t offset, u16 data, u16 mem_mask)
{
	m_mem_cache->write_word(((offset << 1) & 0x3fff) | m_bank1_base, data, mem_mask);
}

u16 ks0164_device::bank2_r(offs_t offset, u16 mem_mask)
{
	return m_mem_cache->read_word(((offset << 1) & 0x3fff) | m_bank2_base, mem_mask);
}

void ks0164_device::bank2_w(offs_t offset, u16 data, u16 mem_mask)
{
	m_mem_cache->write_word(((offset << 1) & 0x3fff) | m_bank2_base, data, mem_mask);
}

u16 ks0164_device::bank1_select_r()
{
	return m_bank1_select;
}

void ks0164_device::bank1_select_w(offs_t, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_bank1_select);
	m_bank1_base = m_bank1_select << 14;
}

u16 ks0164_device::bank2_select_r()
{
	return m_bank2_select;
}

void ks0164_device::bank2_select_w(offs_t, u16 data, u16 mem_mask)
{
	COMBINE_DATA(&m_bank2_select);
	m_bank2_base = m_bank2_select << 14;
}

void ks0164_device::cpu_map(address_map &map)
{
	map(0x0000, 0x001f).r(FUNC(ks0164_device::vec_r));

	map(0x0062, 0x0063).rw(FUNC(ks0164_device::bank1_select_r), FUNC(ks0164_device::bank1_select_w));
	map(0x0064, 0x0065).rw(FUNC(ks0164_device::bank2_select_r), FUNC(ks0164_device::bank2_select_w));

	map(0x0080, 0x3fff).r(FUNC(ks0164_device::rom_r));
	map(0x4000, 0x7fff).rw(FUNC(ks0164_device::bank1_r), FUNC(ks0164_device::bank1_w));
	map(0x8000, 0xbfff).rw(FUNC(ks0164_device::bank2_r), FUNC(ks0164_device::bank2_w));
	map(0xe000, 0xffff).ram();
}

void ks0164_device::sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples)
{
}

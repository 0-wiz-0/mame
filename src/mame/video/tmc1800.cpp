// license:BSD-3-Clause
// copyright-holders:Curt Coder
#include "emu.h"
#include "includes/tmc1800.h"

#include "cpu/cosmac/cosmac.h"
#include "machine/rescap.h"
#include "sound/cdp1864.h"
#include "video/cdp1861.h"
#include "speaker.h"


/* Telmac 2000 */

READ_LINE_MEMBER( tmc2000_state::rdata_r )
{
	return BIT(m_color, 2);
}

READ_LINE_MEMBER( tmc2000_state::bdata_r )
{
	return BIT(m_color, 1);
}

READ_LINE_MEMBER( tmc2000_state::gdata_r )
{
	return BIT(m_color, 0);
}

/* OSM-200 */

uint32_t osc1000b_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	return 0;
}

/* Machine Drivers */

MACHINE_CONFIG_START(tmc1800_state::tmc1800_video)
	MCFG_DEVICE_ADD(CDP1861_TAG, CDP1861, XTAL(1'750'000))
	MCFG_CDP1861_IRQ_CALLBACK(INPUTLINE(CDP1802_TAG, COSMAC_INPUT_LINE_INT))
	MCFG_CDP1861_DMA_OUT_CALLBACK(INPUTLINE(CDP1802_TAG, COSMAC_INPUT_LINE_DMAOUT))
	MCFG_CDP1861_EFX_CALLBACK(INPUTLINE(CDP1802_TAG, COSMAC_INPUT_LINE_EF1))
	MCFG_CDP1861_SCREEN_ADD(CDP1861_TAG, SCREEN_TAG, XTAL(1'750'000))
MACHINE_CONFIG_END

MACHINE_CONFIG_START(osc1000b_state::osc1000b_video)
	MCFG_SCREEN_ADD(SCREEN_TAG, RASTER)
	MCFG_SCREEN_UPDATE_DRIVER(osc1000b_state, screen_update)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_SIZE(320, 200)
	MCFG_SCREEN_VISIBLE_AREA(0, 319, 0, 199)
MACHINE_CONFIG_END

MACHINE_CONFIG_START(tmc2000_state::tmc2000_video)
	MCFG_CDP1864_SCREEN_ADD(SCREEN_TAG, XTAL(1'750'000))
	MCFG_SCREEN_UPDATE_DEVICE(CDP1864_TAG, cdp1864_device, screen_update)

	SPEAKER(config, "mono").front_center();
	CDP1864(config, m_cti, XTAL(1'750'000)).set_screen(SCREEN_TAG);
	m_cti->inlace_cb().set_constant(0);
	m_cti->int_cb().set_inputline(m_maincpu, COSMAC_INPUT_LINE_INT);
	m_cti->dma_out_cb().set_inputline(m_maincpu, COSMAC_INPUT_LINE_DMAOUT);
	m_cti->efx_cb().set_inputline(m_maincpu, COSMAC_INPUT_LINE_EF1);
	m_cti->rdata_cb().set(FUNC(tmc2000_state::rdata_r));
	m_cti->bdata_cb().set(FUNC(tmc2000_state::bdata_r));
	m_cti->gdata_cb().set(FUNC(tmc2000_state::gdata_r));
	m_cti->set_chrominance(RES_K(1.21), RES_K(2.05), RES_K(2.26), RES_K(3.92)); // RL64, RL63, RL61, RL65 (also RH62 (2K pot) in series, but ignored here)
	m_cti->add_route(ALL_OUTPUTS, "mono", 0.25);
MACHINE_CONFIG_END

MACHINE_CONFIG_START(nano_state::nano_video)
	MCFG_CDP1864_SCREEN_ADD(SCREEN_TAG, XTAL(1'750'000))
	MCFG_SCREEN_UPDATE_DEVICE(CDP1864_TAG, cdp1864_device, screen_update)

	SPEAKER(config, "mono").front_center();
	CDP1864(config, m_cti, XTAL(1'750'000)).set_screen(SCREEN_TAG);
	m_cti->inlace_cb().set_constant(0);
	m_cti->int_cb().set_inputline(m_maincpu, COSMAC_INPUT_LINE_INT);
	m_cti->dma_out_cb().set_inputline(m_maincpu, COSMAC_INPUT_LINE_DMAOUT);
	m_cti->efx_cb().set_inputline(m_maincpu, COSMAC_INPUT_LINE_EF1);
	m_cti->rdata_cb().set_constant(1);
	m_cti->bdata_cb().set_constant(1);
	m_cti->gdata_cb().set_constant(1);
	m_cti->set_chrominance(RES_K(1.21), RES_INF, RES_INF, 0); // R18 (unconfirmed)
	m_cti->add_route(ALL_OUTPUTS, "mono", 0.25);
MACHINE_CONFIG_END

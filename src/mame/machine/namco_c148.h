// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/***************************************************************************

Template for skeleton device

***************************************************************************/

#pragma once

#ifndef __NAMCO_C148DEV_H__
#define __NAMCO_C148DEV_H__



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_NAMCO_C148_ADD(_tag, _cputag, _cpumaster) \
	MCFG_DEVICE_ADD(_tag, NAMCO_C148, 0) \
	namco_c148_device::configure_device(*device, _cputag, _cpumaster);

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> namco_c148_device

class namco_c148_device : public device_t
{
public:
	// construction/destruction
	namco_c148_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	DECLARE_ADDRESS_MAP(map, 16);

	static void configure_device(device_t &device, const char *tag, bool is_master)
	{
		namco_c148_device &dev = downcast<namco_c148_device &>(device);
		dev.m_hostcpu_tag = tag;
		dev.m_hostcpu_master = is_master;
	}
	
	DECLARE_READ8_MEMBER( vblank_irq_level_r );
	DECLARE_WRITE8_MEMBER( vblank_irq_level_w );
	DECLARE_READ8_MEMBER( vblank_irq_ack_r );
	DECLARE_WRITE8_MEMBER( vblank_irq_ack_w );
	DECLARE_WRITE8_MEMBER( ext2_w );
	void vblank_irq_trigger();
	//uint8_t posirq_line();
	
protected:
	// device-level overrides
//  virtual void device_validity_check(validity_checker &valid) const;
	virtual void device_start() override;
	virtual void device_reset() override;
private:
	cpu_device *m_hostcpu;      	/**< reference to the host cpu */
	const char *m_hostcpu_tag;		/**< host cpu tag name */
	bool		m_hostcpu_master;	/**< define if host cpu is master */
	struct{
		uint8_t cpuirq;
		uint8_t exirq;
		uint8_t sciirq;
		uint8_t posirq;
		uint8_t vblank;
	}m_irqlevel;
};


// device type definition
extern const device_type NAMCO_C148;



//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************



#endif

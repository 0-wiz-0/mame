// license:BSD-3-Clause
// copyright-holders:Patrick Mackinlay

#include "interpro_ioga.h"

#define VERBOSE 0
#if VERBOSE
#define LOG_TIMER_MASK 0xff
#define LOG_TIMER(timer, ...) if (LOG_TIMER_MASK & (1 << timer)) logerror(__VA_ARGS__)
#define LOG_INTERRUPT(...) logerror(__VA_ARGS__)
#define LOG_IOGA(...) logerror(__VA_ARGS__)
#define LOG_DMA(...) logerror(__VA_ARGS__)
#else
#define LOG_TIMER_MASK 0x00
#define LOG_TIMER(timer, ...)
#define LOG_INTERRUPT(...)
#define LOG_IOGA(...)
#define LOG_DMA(...)
#endif

DEVICE_ADDRESS_MAP_START(map, 32, interpro_ioga_device)
	AM_RANGE(0x30, 0x33) AM_READWRITE(dma_fdc_real_address_r, dma_fdc_real_address_w)
	AM_RANGE(0x34, 0x37) AM_READWRITE(dma_fdc_virtual_address_r, dma_fdc_virtual_address_w)
	AM_RANGE(0x38, 0x3b) AM_READWRITE(dma_fdc_transfer_count_r, dma_fdc_transfer_count_w)
	AM_RANGE(0x3c, 0x3f) AM_READWRITE(dma_fdc_control_r, dma_fdc_control_w)

	AM_RANGE(0x5c, 0x7f) AM_READWRITE16(icr_r, icr_w, 0xffffffff)
	AM_RANGE(0x80, 0x83) AM_READWRITE16(icr18_r, icr18_w, 0x0000ffff)
	AM_RANGE(0x80, 0x83) AM_READWRITE8(softint_r, softint_w, 0x00ff0000)
	AM_RANGE(0x80, 0x83) AM_READWRITE8(nmictrl_r, nmictrl_w, 0xff000000)

	AM_RANGE(0x88, 0x8b) AM_READWRITE(timer_prescaler_r, timer_prescaler_w)
	AM_RANGE(0x8c, 0x8f) AM_READWRITE(timer0_r, timer0_w)
	AM_RANGE(0x90, 0x93) AM_READWRITE(timer1_r, timer1_w)

	AM_RANGE(0xa8, 0xab) AM_READWRITE(timer3_r, timer3_w)

	AM_RANGE(0xb0, 0xbf) AM_READWRITE16(softint_vector_r, softint_vector_w, 0xffffffff)
ADDRESS_MAP_END

// InterPro IOGA
const device_type INTERPRO_IOGA = &device_creator<interpro_ioga_device>;

interpro_ioga_device::interpro_ioga_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, INTERPRO_IOGA, "InterPro IOGA", tag, owner, clock, "ioga", __FILE__),
	m_out_nmi_func(*this),
	m_out_int_func(*this),
	m_dma_r_func{ { *this }, { *this }, { *this }, { *this } },
	m_dma_w_func{ { *this }, { *this }, { *this }, { *this } },
	m_fdc_tc_func(*this)
	{
}

void interpro_ioga_device::device_start()
{
	// resolve callbacks
	m_out_nmi_func.resolve();
	m_out_int_func.resolve();

	for (auto & r : m_dma_r_func)
		r.resolve_safe(0xff);

	for (auto & w : m_dma_w_func)
		w.resolve();

	m_fdc_tc_func.resolve();

	m_cpu = machine().device<cpu_device>("cpu");

	// allocate ioga timers
	m_timer[0] = timer_alloc(IOGA_TIMER_0);
	m_timer[1] = timer_alloc(IOGA_TIMER_1);
	m_timer[2] = timer_alloc(IOGA_TIMER_2);
	m_timer[3] = timer_alloc(IOGA_TIMER_3);

	for (auto & elem : m_timer)
		elem->enable(false);

	// allocate timer for DMA controller
	m_dma_timer = timer_alloc(IOGA_TIMER_DMA);
	m_dma_timer->adjust(attotime::never);
}

void interpro_ioga_device::device_reset()
{
	m_nmi_pending = false;

	m_interrupt_active = 0;
	m_irq_forced = 0;

	m_dma_active = false;
	m_dma_drq_state = 0;

	// configure timer 0 at 60Hz
	m_timer_reg[0] = 0;
	m_timer[0]->adjust(attotime::zero, IOGA_TIMER_0, attotime::from_hz(60));
}

/******************************************************************************
  Timers
******************************************************************************/
READ32_MEMBER(interpro_ioga_device::timer1_r)
{ 
	uint32_t result = m_timer1_count & IOGA_TIMER1_VMASK;

	// set the start bit if the timer is currently enabled
	if (m_timer[1]->enabled())
		result |= IOGA_TIMER1_START;
	else if (m_timer[1]->param())
		result |= IOGA_TIMER1_EXPIRED;

	return result;
}

READ32_MEMBER(interpro_ioga_device::timer3_r)
{
	uint32_t result = m_timer3_count & IOGA_TIMER3_VMASK;

	if (m_timer[3]->enabled())
		result |= IOGA_TIMER3_START;
	else if (m_timer[3]->param())
		result |= IOGA_TIMER3_EXPIRED;

	return result; 
}

void interpro_ioga_device::write_timer(int timer, uint32_t value, device_timer_id id)
{
	switch (id)
	{
	case IOGA_TIMER_1:
		// disable the timer
		m_timer[timer]->enable(false);

		// store the timer count value
		m_timer1_count = value;

		// start the timer if necessary
		if (value & IOGA_TIMER1_START)
		{
			LOG_TIMER(1, "timer 1: started prescaler %d value %d\n", m_prescaler & 0x7fff, value & IOGA_TIMER1_VMASK);

			// FIXME: this division by 50 is sufficient to pass iogadiag timer 1 tests
			m_timer[timer]->adjust(attotime::zero, false, attotime::from_usec((m_prescaler & 0x7fff) / 50));
		}
		break;

	case IOGA_TIMER_3:
		// stop the timer so it won't trigger while we're fiddling with it
		m_timer[timer]->enable(false);

		// write the new value to the timer register
		m_timer3_count = value & IOGA_TIMER3_VMASK;

		// start the timer if necessary
		if (value & IOGA_TIMER3_START)
		{
			LOG_TIMER(3, "timer 3: started value %d\n", value & IOGA_TIMER3_VMASK);

			m_timer[timer]->adjust(attotime::zero, false, attotime::from_hz(XTAL_25MHz));
		}
		break;

	default:
		// save the value
		m_timer_reg[timer] = value;

		// timer_set(attotime::from_usec(500), id);

		LOG_TIMER(0xf, "timer %d: set to 0x%x (%d)\n", timer, m_timer_reg[timer], m_timer_reg[timer]);
		break;
	}
}

void interpro_ioga_device::device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr)
{
	switch (id)
	{
	case IOGA_TIMER_0:
		m_timer_reg[0]++;
		set_irq_line(IOGA_TIMER0_IRQ, ASSERT_LINE);
		break;

	case IOGA_TIMER_1:
		// decrement timer count value
		m_timer1_count--;

		// check if timer has expired
		if (m_timer1_count == 0)
		{
			LOG_TIMER(1, "timer 1: stopped\n");

			// disable timer and set the zero flag
			timer.enable(false);
			timer.set_param(true);

			// throw an interrupt
			set_irq_line(IOGA_TIMER1_IRQ, ASSERT_LINE);
		}
		break;

	case IOGA_TIMER_3:
		// decrement timer count value
		m_timer3_count--;

		// check for expiry
		if (m_timer3_count == 0)
		{
			LOG_TIMER(3, "timer 3: stopped\n");

			// disable timer and set the zero flag
			timer.enable(false);
			timer.set_param(true);

			// throw an interrupt
			set_irq_line(IOGA_TIMER3_IRQ, ASSERT_LINE);
		}
		break;

	case IOGA_TIMER_DMA:
		// transfer data between device and main memory

		// TODO: figure out what indicates dma write (memory -> device)
		// TODO: devices other than floppy
		// TODO: implement multiple dma channels
		// TODO: virtual memory?
	
		if (!m_dma_active)
		{
			LOG_DMA("dma: transfer started, control 0x%08x, real address 0x%08x count 0x%08x\n", m_dma_fdc_control, m_dma_fdc_real_address, m_dma_fdc_transfer_count);
			m_dma_active = true;
		}
		address_space &space = m_cpu->space(AS_PROGRAM);

		// while the device is requesting a data transfer and the DMA count is not empty
		while (m_dma_drq_state && m_dma_fdc_transfer_count)
		{
			// transfer a byte between device and memory
			if (true)
				space.write_byte(m_dma_fdc_real_address, m_dma_r_func[IOGA_DMA_CHANNEL_FLOPPY]());
			else
				m_dma_w_func[IOGA_DMA_CHANNEL_FLOPPY](space.read_byte(m_dma_fdc_real_address));

			// increment address and decrement counter
			m_dma_fdc_real_address++;
			m_dma_fdc_transfer_count--;
		}

		// if there are no more bytes remaining, terminate the transfer
		if (m_dma_fdc_transfer_count == 0)
		{
			LOG_DMA("dma: transfer stopped, control 0x%08x, real address 0x%08x count 0x%08x\n", m_dma_fdc_control, m_dma_fdc_real_address, m_dma_fdc_transfer_count);
			LOG_DMA("dma: asserting fdc terminal count line\n");

			m_fdc_tc_func(ASSERT_LINE);
			m_fdc_tc_func(CLEAR_LINE);

			m_dma_active = false;
		}
	break;
	}
}

/*
IOGA
00: ethernet remap 003e0480 // ET_82586_BASE_ADDR or IOGA_ETH_REMAP
04 : ethernet map page 00fff4b0 // ET_82586_CNTL_REG  or IOGA_ETH_MAPPG
08 : ethernet control 000004b2 // IOGA_ETH_CTL
0C : plotter real address 00000fff

10 : plotter virtual address fffffffc
14 : plotter transfer count 003fffff
18 : plotter control ec000001
1C : plotter end - of - scanline counter ffffffff

20 : SCSI real address 00000000
24 : SCSI virtual address 007e96b8
28 : SCSI transfer count
2C : SCSI control

30 : floppy real address
34 : floppy virtual address
38 : floppy transfer count
3C : floppy control

40 : serial address 0 (003ba298)
44 : serial control 0 (01000000)
48 : serial address 1 (ffffffff)
4C : serial control 1 (01200000)

50 : serial address 2 (ffffffff)
54 : serial control 2 (01200000)

-- 16 bit
5A : SIB control(00ff)
5C : internal int  3 (timer 2) 00ff        irq 0
5E : internal int  4 (timer 3) 00ff        irq 1

60 : external int  0 (SCSI)0a20            irq 2
62 : external int  1 (floppy)0621          irq 3
64 : external int  2 (plotter)1622         irq 4
66 : external int  3 (SRX / CBUS 0) 0a02   irq 5
68 : external int  4 (SRX / CBUS 1) 0e24   irq 6
6A : external int  5 (SRX / CBUS 2) 0e25   irq 7
6C : external int  6 (VB)0c26              irq 8
6E : external int  7 0cff                  irq 9
70 : external int  8 (CBUS 3) 0cff         irq 10
72 : external int  9 (clock / calendar) 0e29  irq 11
74 : external int 10 (clock/SGA) 04fe         irq 12

76 : internal int  0 (mouse)0010              irq 13
78 : internal int  1 (timer 0) 0011 - 60Hz    irq 14
7A : internal int  2 (timer 1) 0212           irq 15
7C : internal int  5 (serial DMA) 0e13        irq 16
7E : external int 11 (serial) 0a01            irq 17
80 : external int 12 (Ethernet)162c			  irq 18 // IOGA_EXTINT12

-- 8 bit
82 : soft int 00
83 : NMI control 1b

-- 32 bit
84 : mouse status 00070000
88 : timer prescaler 05aa06da
8C : timer 0 00000022
90 : timer 1 00010005
94 : error address 7f100000
98 : error cycle type 00001911 // short?

-- 16 bit
9C: arbiter control 000a // IOGA_ARBCTL
	#define ETHC_BR_ENA	(1<<0)
	#define SCSI_BR_ENA	(1<<1)
	#define PLT_BR_ENA	(1<<2)
	#define FLP_BR_ENA	(1<<3)
	#define SER0_BR_ENA	(1<<4)
	#define SER1_BR_ENA	(1<<5)
	#define SER2_BR_ENA	(1<<6)
	#define ETHB_BR_ENA	(1<<7)
	#define ETHA_BR_ENA	(1<<8)

9E : I / O base address

-- 32 bit
A0 : timer 2 count ccc748ec
A4 : timer 2 value ffffffff
A8 : timer 3 bfffffff // timer 3 count
AC : bus timeout 445a445c

-- 16 bit
B0 : soft int 8 00ff
B2 : soft int 9 00ff
B4 : soft int 10 00ff
B6 : soft int 11 00ff
B8 : soft int 12 00ff
BA : soft int 13 00ff
BC : soft int 14 00ff
BE : soft int 15 00ff

-- 32 bit
C0 : ethernet address A 403bc002 // IOGA_ETHADDR_A
C4 : ethernet address B 403a68a0 // IOGA_ETHADDR_B
C8 : ethernet address C 4039f088 // IOGA_ETHADDR_C

		 (62) = 0x0421			// set floppy interrupts?
	(3C) &= 0xfeff ffff		// turn off and on again
	(3C) |= 0x0100 0000
	(9C) |= 0x0008
	(62) = 0x0621

	during rom boot, all interrupt vectors point at 7f10249e

	int 16 = prioritized interrupt 16, level 0, number 0, mouse interface
	17 timer 0
*/

/******************************************************************************
 Interrupts
******************************************************************************/

static const uint16_t irq_enable_mask[IOGA_INTERRUPT_COUNT] =
{
	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL | IOGA_INTERRUPT_ENABLE_INTERNAL, // external interrupt 0: SCSI
	IOGA_INTERRUPT_ENABLE_EXTERNAL | IOGA_INTERRUPT_ENABLE_INTERNAL, // external interrupt 1: floppy
	IOGA_INTERRUPT_ENABLE_EXTERNAL | IOGA_INTERRUPT_ENABLE_INTERNAL, // external interrupt 2: plotter
	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL,

	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL,

	// internal interrupt 5: serial DMA - one interrupt enable per DMA channel
	IOGA_INTERRUPT_ENABLE_EXTERNAL << 0 | IOGA_INTERRUPT_ENABLE_EXTERNAL << 1 | IOGA_INTERRUPT_ENABLE_EXTERNAL << 2,
	IOGA_INTERRUPT_ENABLE_EXTERNAL,
	IOGA_INTERRUPT_ENABLE_EXTERNAL | IOGA_INTERRUPT_ENABLE_INTERNAL // external interrupt 12: Ethernet
};

void interpro_ioga_device::set_nmi_line(int state)
{
	switch (state)
	{
	case ASSERT_LINE:

		LOG_INTERRUPT("nmi: ctrl = 0x%02x\n", m_nmictrl);

		if ((m_nmictrl & IOGA_NMI_ENABLE) == IOGA_NMI_ENABLE)
		{
			// if edge triggered mode, clear enable in
			if (m_nmictrl & IOGA_NMI_EDGE)
				m_nmictrl &= ~IOGA_NMI_ENABLE_IN;

			m_nmi_pending = true;
			update_interrupt(ASSERT_LINE);
		}
		break;

	case CLEAR_LINE:
		m_nmi_pending = false;
		update_interrupt(ASSERT_LINE);
		break;
	}
}

void interpro_ioga_device::set_irq_line(int irq, int state)
{
	LOG_INTERRUPT("set_irq_line(%d, %d)\n", irq, state);
	switch (state)
	{
	case ASSERT_LINE:
		if (m_int_vector[irq] & irq_enable_mask[irq])
		{
			// set pending bit
			m_int_vector[irq] |= IOGA_INTERRUPT_PENDING;

			// update irq line state
			update_interrupt(state);
		}
		else
			LOG_INTERRUPT("received disabled interrupt irq %d vector 0x%04x\n", irq, m_int_vector[irq]);
		break;

	case CLEAR_LINE:
		// clear pending bit
		m_int_vector[irq] &= ~IOGA_INTERRUPT_PENDING;

		// update irq line state
		update_interrupt(state);
		break;
	}
}

void interpro_ioga_device::set_irq_soft(int irq, int state)
{
	LOG_INTERRUPT("set_irq_soft(%d, %d)\n", irq, state);
	switch (state)
	{
	case ASSERT_LINE:
		// set pending bit
		if (irq < 8)
			m_softint |= 1 << irq;
		else
			m_softint_vector[irq - 8] |= IOGA_INTERRUPT_PENDING;

		update_interrupt(state);
		break;

	case CLEAR_LINE:
		// clear pending bit
		if (irq < 8)
			m_softint &= ~(1 << irq);
		else
			m_softint_vector[irq - 8] &= ~IOGA_INTERRUPT_PENDING;

		// update irq line state
		update_interrupt(state);
		break;
	}
}

IRQ_CALLBACK_MEMBER(interpro_ioga_device::inta_cb)
{
	switch (irqline)
	{
	case INPUT_LINE_IRQ0:
		// FIXME: clear pending bit - can't rely on device callbacks
		switch (m_interrupt_active)
		{
		case IOGA_INTERRUPT_INTERNAL:
		case IOGA_INTERRUPT_EXTERNAL:
			m_int_vector[m_irq_current] &= ~IOGA_INTERRUPT_PENDING;
			break;

		case IOGA_INTERRUPT_SOFT_LO:
			m_softint &= ~(1 << m_irq_current);
			break;

		case IOGA_INTERRUPT_SOFT_HI:
			m_softint_vector[m_irq_current] &= ~IOGA_INTERRUPT_PENDING;
			break;
		}

		// clear irq line
		update_interrupt(CLEAR_LINE);

		// fall through to return interrupt vector
	case -1:
		// return vector for current interrupt without clearing irq line
		switch (m_interrupt_active)
		{
		case IOGA_INTERRUPT_EXTERNAL:
		case IOGA_INTERRUPT_INTERNAL:
			return m_int_vector[m_irq_current] & 0xff;

		case IOGA_INTERRUPT_SOFT_LO: 
			return 0x8f + m_irq_current * 0x10;

		case IOGA_INTERRUPT_SOFT_HI: 
			return m_softint_vector[m_irq_current] & 0xff;
		}
		break;

	case INPUT_LINE_NMI:
		// clear pending flag
		m_nmi_pending = false;

		// clear line
		update_interrupt(CLEAR_LINE);

		// return vector
		return 0;
	}

	return 0;
}

void interpro_ioga_device::update_interrupt(int state)
{
	switch (state)
	{
	case CLEAR_LINE:
		if (m_interrupt_active)
		{
			// the cpu has acknowledged the active interrupt, deassert the nmi/irq line
			m_interrupt_active == IOGA_INTERRUPT_NMI ? m_out_nmi_func(CLEAR_LINE) : m_out_int_func(CLEAR_LINE);

			// clear the active status
			m_interrupt_active = 0;
		}
		// fall through to handle any pending interrupts

	case ASSERT_LINE:
		// if an interrupt is currently active, don't do anything
		if (m_interrupt_active == 0)
		{
			// check for pending nmi
			if (m_nmi_pending)
			{
				m_interrupt_active = IOGA_INTERRUPT_NMI;

				m_out_nmi_func(ASSERT_LINE);
				return;
			}

			// check for any pending irq
			for (int i = 0; i < IOGA_INTERRUPT_COUNT; i++)
			{
				if (m_int_vector[i] & IOGA_INTERRUPT_PENDING)
				{
					m_interrupt_active = IOGA_INTERRUPT_INTERNAL; // TODO: flag internal/external
					m_irq_current = i;

					m_out_int_func(ASSERT_LINE);
					return;
				}
			}

			// check for any pending soft interrupts (low type)
			for (int i = 0; i < 8; i++)
			{
				if (m_softint & (1 << i))
				{
					m_interrupt_active = IOGA_INTERRUPT_SOFT_LO;
					m_irq_current = i;

					m_out_int_func(ASSERT_LINE);
					return;
				}
			}

			// check for any pending soft interrupts (high type)
			for (int i = 0; i < 8; i++)
			{
				if (m_softint_vector[i] & IOGA_INTERRUPT_PENDING)
				{
					m_interrupt_active = IOGA_INTERRUPT_SOFT_HI;
					m_irq_current = i;

					m_out_int_func(ASSERT_LINE);
					return;
				}
			}
		}
		break;
	}
}

WRITE16_MEMBER(interpro_ioga_device::icr_w)
{
	LOG_INTERRUPT("interrupt vector %d set to 0x%04x at pc 0x%08x\n", offset, data, space.device().safe_pc());

	// FIXME: now that the interrupt handling only depends on IOGA_INTERRUPT_PENDING, we might be able
	// to avoid this hack
	if (data & IOGA_INTERRUPT_PENDING)
	{
		m_irq_forced |= 1 << offset;
		m_int_vector[offset] = data & ~IOGA_INTERRUPT_PENDING;
	}
	else if (m_irq_forced & 1 << offset)
	{
		m_int_vector[offset] = data;

		// clear forced flag
		m_irq_forced &= ~(1 << offset);

		// force an interrupt
		set_irq_line(offset, ASSERT_LINE);
	}
	else
		m_int_vector[offset] = data;
}

WRITE8_MEMBER(interpro_ioga_device::softint_w)
{
	// save the existing value
	uint8_t previous = m_softint;

	// store the written value
	m_softint = data;

	// force soft interrupt for any bit written from 1 to 0
	for (int i = 0; i < 8; i++)
	{
		uint8_t mask = 1 << i;

		// check for transition from 1 to 0 and force a soft interrupt
		if (previous & mask && !(data & mask))
			set_irq_soft(i, ASSERT_LINE);
	}
}

WRITE8_MEMBER(interpro_ioga_device::nmictrl_w)
{ 
	// save the existing value
	uint8_t previous = m_nmictrl;

	// store the written value
	m_nmictrl = data;

	// force an nmi when pending bit is written low
	if (previous & IOGA_NMI_PENDING && !(data & IOGA_NMI_PENDING))
		set_nmi_line(ASSERT_LINE);
}

WRITE16_MEMBER(interpro_ioga_device::softint_vector_w)
{
	// save the existing value
	uint16_t previous = m_softint_vector[offset];

	// store the written value
	m_softint_vector[offset] = data;

	// check for transition from 1 to 0 and force a soft interrupt
	if (previous & IOGA_INTERRUPT_PENDING && !(data & IOGA_INTERRUPT_PENDING))
		set_irq_soft(offset + 8, ASSERT_LINE);
}

/******************************************************************************
 DMA
******************************************************************************/
WRITE_LINE_MEMBER(interpro_ioga_device::drq)
{
	// this member is called when the device has data ready for reading via dma
	m_dma_drq_state = state;

	if (state)
	{
		// TODO: check if dma is enabled
		m_dma_timer->adjust(attotime::zero);
	}
}
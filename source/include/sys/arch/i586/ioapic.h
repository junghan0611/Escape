/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <sys/common.h>
#include <sys/spinlock.h>

class OStream;

class IOAPIC {
	IOAPIC() = delete;

	struct Instance {
		uint8_t id;
		uint8_t version;
		volatile uint32_t *addr;
		uint baseGSI;
		uint count;
	};

	static const size_t RED_COUNT		= 24;
	static const size_t MAX_IOAPICS		= 8;
	static const size_t ISA_IRQ_COUNT	= 16;
	static const size_t IOREGSEL	 	= 0x0 / sizeof(uint32_t);
	static const size_t IOWIN			= 0x10 / sizeof(uint32_t);

	enum {
		IOAPIC_REG_ID		= 0x0,
		IOAPIC_REG_VERSION	= 0x1,
		IOAPIC_REG_ARB		= 0x2,
		IOAPIC_REG_REDTBL	= 0x10
	};
	enum {
		RED_DESTMODE_PHYS	= 0 << 11,
		RED_DESTMODE_LOG	= 1 << 11,
	};
	enum {
		RED_INT_MASK_DIS	= 0 << 16,
		RED_INT_MASK_EN		= 1 << 16
	};

public:
	enum Polarity {
		RED_POL_HIGH_ACTIVE	= 0 << 13,
		RED_POL_LOW_ACTIVE	= 1 << 13,
	};
	enum TriggerMode {
		RED_TRIGGER_EDGE	= 0 << 15,
		RED_TRIGGER_LEVEL	= 1 << 15,
	};
	enum DeliveryMode {
		RED_DEL_FIXED		= 0 << 8,
		RED_DEL_LOWPRIO		= 1 << 8,
		RED_DEL_SMI			= 2 << 8,
		RED_DEL_NMI			= 4 << 8,
		RED_DEL_INIT		= 5 << 8,
		RED_DEL_EXTINT		= 7 << 8,
	};

	/**
	 * @return true if IOAPICs are present
	 */
	static bool enabled() {
		return count > 0;
	}

	/**
	 * Adds the given IOAPIC to the list of IOAPICs.
	 *
	 * @param id the id
	 * @param addr the virtual address for the MMIO space
	 * @param baseGSI the first GSI that is handled by the IOAPIC
	 */
	static void add(uint8_t id,uintptr_t addr,uint baseGSI);

	/**
	 * Inserts the given redirection entry into the corresponding IOAPIC.
	 * Atm, it is always forwarded to the BSP.
	 *
	 * @param srcIRQ the ISA IRQ to redirect
	 * @param gsi the GSI to redirect to
	 * @param delivery the delivery mode (fixed, low prio, ...)
	 * @param polarity the polarity (low or high active)
	 * @param triggerMode the trigger mode (edge or level)
	 */
	static void setRedirection(uint8_t srcIRQ,uint gsi,DeliveryMode delivery,
			Polarity polarity,TriggerMode triggerMode);

	/**
	 * @param isaIRQ the ISA IRQ
	 * @return the GSI for the given ISA IRQ
	 */
	static uint irq_to_gsi(uint8_t isaIRQ) {
		return isa2gsi[isaIRQ];
	}

	/**
	 * Masks the given GSI.
	 *
	 * @param gsi the global system interrupt
	 */
	static void mask(uint gsi) {
		IOAPIC::Instance *ioapic = get(gsi);
		uint32_t lower = read(ioapic,IOAPIC_REG_REDTBL + gsi * 2);
		write(ioapic,IOAPIC_REG_REDTBL + gsi * 2,lower | RED_INT_MASK_EN);
	}

	/**
	 * Prints the redirection tables of all IOAPICs
	 *
	 * @param os the stream
	 */
	static void print(OStream &os);

private:
	static Instance *get(uint gsi);
	static bool exists(uint vector);
	static uint32_t getMax(Instance *ioapic) {
		return (getVersion(ioapic) >> 16) & 0xff;
	}
	static uint32_t getVersion(Instance *ioapic) {
		return read(ioapic,IOAPIC_REG_VERSION);
	}

	static uint32_t read(Instance *ioapic,uint32_t reg) {
		/* we only use it in the setup-phase with a single CPU, but to be sure... */
		SpinLock::acquire(&lck);
		select(ioapic,reg);
		uint32_t val = ioapic->addr[IOWIN];
		SpinLock::release(&lck);
		return val;
	}
	static void write(Instance *ioapic,uint32_t reg,uint32_t value) {
		SpinLock::acquire(&lck);
		select(ioapic,reg);
		ioapic->addr[IOWIN] = value;
		SpinLock::release(&lck);
	}
	static void select(Instance *ioapic,uint32_t reg) {
		volatile uint8_t *addr = reinterpret_cast<volatile uint8_t*>(ioapic->addr + IOREGSEL);
		*addr = reg;
	}

	static klock_t lck;
	static size_t count;
	static Instance ioapics[MAX_IOAPICS];
	static uint isa2gsi[ISA_IRQ_COUNT];
};

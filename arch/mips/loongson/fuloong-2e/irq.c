/*
 * Copyright (C) 2007 Lemote Inc. & Insititute of Computing Technology
 * Author: Fuxin Zhang, zhangfx@lemote.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#include <linux/interrupt.h>

#include <asm/irq_cpu.h>
#include <asm/i8259.h>

#include <loongson.h>

static inline void i8259_irqdispatch(void)
{
	int irq;

	irq = i8259_irq();
	if (irq >= 0)
		do_IRQ(irq);
	else
		spurious_interrupt();
}

inline void mach_irq_dispatch(unsigned int pending)
{
	if (pending & CAUSEF_IP7)
		do_IRQ(MIPS_CPU_IRQ_BASE + 7);
	else if (pending & CAUSEF_IP2)
		bonito_irqdispatch();
	else if (pending & CAUSEF_IP5)
		i8259_irqdispatch();
#ifdef CONFIG_OPROFILE
	else if (pending & CAUSEF_IP6) /* perf counter loverflow */
		do_IRQ(LOONGSON2_PERFCNT_IRQ);
#endif
	else
		spurious_interrupt();
}

static struct irqaction cascade_irqaction = {
	.handler = no_action,
	.name = "cascade",
	.flags = IRQF_NODELAY,
};

void __init set_irq_trigger_mode(void)
{
	/* most bonito irq should be level triggered */
	LOONGSON_INTEDGE = LOONGSON_ICU_SYSTEMERR | LOONGSON_ICU_MASTERERR |
	    LOONGSON_ICU_RETRYERR | LOONGSON_ICU_MBOXES;
}

void __init mach_init_irq(void)
{
	/* init all controller
	 *   0-15         ------> i8259 interrupt
	 *   16-23        ------> mips cpu interrupt
	 *   32-63        ------> bonito irq
	 */

	/* Sets the first-level interrupt dispatcher. */
	mips_cpu_irq_init();
	init_i8259_irqs();
	bonito_irq_init();

	/* bonito irq at IP2 */
	setup_irq(MIPS_CPU_IRQ_BASE + 2, &cascade_irqaction);
	/* 8259 irq at IP5 */
	setup_irq(MIPS_CPU_IRQ_BASE + 5, &cascade_irqaction);
}

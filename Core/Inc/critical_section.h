#ifndef _CRITICAL_SECTION_H
#define _CRITICAL_SECTION_H

#define CPU_INIT_CRITICAL_SECTION()  volatile unsigned int crValue

#define CPU_ENTER_CRITICAL_SECTION()	\
	do {										\
		__asm__ (							\
			"MRS   R0, PRIMASK\n\t"			\
			"CPSID I\n\t"						\
			"STRB R0, %[output]"				\
			: [output] "=m" (crValue) :: "r0");		\
	} while(0)

#define CPU_EXIT_CRITICAL_SECTION()		\
	do{										\
		__asm__ (							\
		"ldrb r0, %[input]\n\t"					\
		"msr PRIMASK,r0;\n\t"				\
		::[input] "m" (crValue) : "r0");			\
	} while(0)

#endif // _CRITICAL_SECTION_H

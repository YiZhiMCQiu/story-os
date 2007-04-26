//    This file is part of the Story OS
//    Copyright (C) 2007  Peter Zotov
//
//    Story OS is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    Story OS is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. 

#include <multiboot.h>
#include <story.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <terminal_driver.h>
#include <hal.h>
#include <colors.h>
#include <core.h>

/*
				MEMORY MAP
0000 0000-0009 FFFF			reserved area: BIOS data and other stuff	898K
	0000 0000-0000 03FF		Real-Mode interrupt table			4*0x100
	0000 0400-0000 04FF		BIOS data area					0x100 
	0000 0500-0009 FBFF		Conventional memory (UNUSED)			0x9F700
	0009 FC00-0009 FFFF		Extended BIOS data area				0x400

000A 0000-000F FFFF			ROM area
	000A 0000-000B FFFF		VGA memory					128K
		000A 0000-000A FFFF	VGA framebuffer					64K
		000B 0000-000B 7FFF	Text monochrome					32K
		000B 8000-000B FFFF	Text color					32K
	000C 0000-000C 7FFF		VGA BIOS					32K
	000F 0000-000F FFFF		Motherboard BIOS				64K

				All my stuff: begins @ 1M
0010 0000-FEBF FFFF			Extended memory
	0020 0000-003F FFFF		Kernel & modules				4M
	0040 0000-0040 0FFF		Kernel Page Directory
	0040 1000-0040 1FFF		Memory manager
	0040 2000-XXXX XXXX		Free memory

FEC0 0000-FFFF FFFF			Internal stuff
*/

/*
				GDT
0		NULL
1		System Code
2		System Data
3		Application Code
4		Application Data
5		Application TSS
6		Scheduler TSS
*/
#define MM_POSITION			0x00100000

HAL* hal;

#ifdef _ENABLE_GDB_STUB_
extern "C" void set_debug_traps();
extern "C" void breakpoint();
#endif

/*void _test_task()
{
unsigned char* c = (unsigned char*) 0x80000000;
unsigned char* cl = (unsigned char*) 0x80000001;
int n = 0;
if(*c == '-')
 asm("int $0x31"::"a"(2),"b"(2));
for(;;)
 {
 printf("%z%c", *cl, *c);
 n++;
 if(n == 100)
  asm("int $0x31"::"a"(1),"b"(2));
 int i;
 for(i = 0; i < 1000000; i++);
 }
}*/

/*void test_tasks()
{
VirtualMemoryManager* vmm1 = new VirtualMemoryManager;
unsigned char* c1 = (unsigned char*) vmm1->physical(vmm1->alloc(1));
*c1 = '+';
*(c1+1) = LIGHTRED;
VirtualMemoryManager* vmm2 = new VirtualMemoryManager;
unsigned char* c2 = (unsigned char*) vmm2->physical(vmm2->alloc(1));
*c2 = '-';
*(c2+1) = LIGHTBLUE;
VirtualMemoryManager* vmm3 = new VirtualMemoryManager;
unsigned char* c3 = (unsigned char*) vmm3->physical(vmm3->alloc(1));
*c3 = '*';
*(c3+1) = LIGHTGREEN;

hal->taskman->create_task(hal->sys_code, hal->sys_data, 1, (unsigned int) &_test_task, 1, vmm1); //2
hal->taskman->create_task(hal->sys_code, hal->sys_data, 1, (unsigned int) &_test_task, 2, vmm2); //3
hal->taskman->create_task(hal->sys_code, hal->sys_data, 1, (unsigned int) &_test_task, 3, vmm3); //4
}
*/

extern "C" void entry(unsigned long magic, multiboot_info_t* multiboot_info)
{
int num;

MemoryManager* mm = new (MM_POSITION) MemoryManager(0x00401000, multiboot_info->mem_upper * 1024);

hal = new (mm->alloc(1)) HAL(multiboot_info);
hal->cli();

hal->mm = mm;

init_mallocator(); //malloc, free, etc

hal->terminal = new KernelTerminalDriver;
hal->terminal->clear();

if(magic != MULTIBOOT_BOOTLOADER_MAGIC)
 hal->panic("Invalid magic number! I can be booted only with multiboot bootloader, e.g. GRUB!");

printf("%zStory OS%z version %z%s (build %i)%z, Copyright (C) 2007 Peter Zotov\n", LIGHTBLUE, WHITE, LIGHTGREEN, VERSION, BUILD, WHITE);
printf("Compiled %s, %s\n", __DATE__, __TIME__);
textcolor(LIGHTGRAY);
printf("%zStory OS%z comes with ABSOLUTELY NO WARRANTY; for details type `cat warranty'\n", LIGHTBLUE, LIGHTGRAY);
printf("This is free software, and you are welcome to redistribute it\n");
printf("under certain conditions; type `cat copying' for details.\n\n");

printf("System memory amount is %z%d KB%z.\n\n", WHITE, hal->mm->free_memory() / 0x400, LIGHTGRAY);

printf("%zInitializing HAL...%z ", GREEN, LIGHTGRAY);

//printf("GDT... ");
hal->gdt = new GDT;
hal->gdt->add_descriptor(new NullDescriptor());

//0x00000000=>0xFFFFFFFF readable 32 bit code segment with dpl=0
num = hal->gdt->add_descriptor(new SegmentDescriptor(0, 0xFFFFFFFF, true, true, 0, true));
hal->sys_code = hal->gdt->make_segment(num, 0);

//0x00000000=>0xFFFFFFFF writable 32 bit data segment with dpl=0
num = hal->gdt->add_descriptor(new SegmentDescriptor(0, 0xFFFFFFFF, false, true, 0, true));
hal->sys_data = hal->gdt->make_segment(num, 0);

//0x00000000=>0xFFFFFFFF readable 32 bit code segment with dpl=3
num = hal->gdt->add_descriptor(new SegmentDescriptor(0, 0xFFFFFFFF, true, true, 3, true));
hal->app_code = hal->gdt->make_segment(num, 3);

//0x00000000=>0xFFFFFFFF writable 32 bit data segment with dpl=3
num = hal->gdt->add_descriptor(new SegmentDescriptor(0, 0xFFFFFFFF, false, true, 3, true));
hal->app_data = hal->gdt->make_segment(num, 3);

hal->gdt->install();
//printf("%zok%z\n", LIGHTGREEN, LIGHTGRAY);

//printf("IDT... ");

hal->idt = new IDT;
hal->idt->install();

//printf("%zok%z\n", LIGHTGREEN, LIGHTGRAY);

//printf("Exception handlers... ");
hal->idt->register_exceptions();
//printf("%zok%z\n", LIGHTGREEN, LIGHTGRAY);

//printf("PIC... ");
hal->pic = new PIC;
hal->pic->remap(0x20, 0x28);
hal->pic->mask(0);
//printf("%zok%z\n", LIGHTGREEN, LIGHTGRAY);

//printf("IRQ handlers... ");
hal->idt->register_irqs();
//printf("%zok%z\n", LIGHTGREEN, LIGHTGRAY);

//printf("System calls... ");
hal->syscalls = new SyscallManager();
//printf("%zok%z\n", LIGHTGREEN, LIGHTGRAY);

//printf("Clock... ");
hal->clock = new Clock();
//printf("%zok%z\n", LIGHTGREEN, LIGHTGRAY);

//printf("Paging... ");
hal->pagedir = (PageDirectory*) 0x00400000;
hal->paging = new Paging();
//printf("enabling... ");
hal->paging->enable();
//printf("%zok%z\n", LIGHTGREEN, LIGHTGRAY);

//printf("Task manager... ");
hal->taskman = new TaskManager();
//printf("%zok%z\n", LIGHTGREEN, LIGHTGRAY);

#ifdef _ENABLE_GDB_STUB_
printf("Setting up a GDB stub... ");
set_debug_traps();
printf("%zok%z\n", LIGHTGREEN, LIGHTGRAY);

printf("Issuing a breakpoint...\n");
breakpoint();
#endif

//printf("Testing VMM... ");

 {
 VirtualMemoryManager* vmm = new VirtualMemoryManager();
 vmm->load();
 //printf("alloc... ");
 unsigned int* test1 = (unsigned int*) vmm->alloc(1);
 unsigned int* test2 = (unsigned int*) vmm->physical(test1);
 *test2 = 0xDEADBEEF;
 if(*test1 != 0xDEADBEEF)
  hal->panic("VMM self-testing error!");
 //printf("free... ");
 hal->paging->load_cr3(hal->pagedir);
 //printf("delete... ");
 delete vmm;
 }

//printf("%zok%z\n", LIGHTGREEN, LIGHTGRAY);

printf("%zCOMPLETE%z\n", GREEN, LIGHTGRAY);

core = new Core(multiboot_info);

hal->pic->unmask(0);

hal->sti();
for(;;);
hal->panic("entry is running!");
}

#ifdef _ENABLE_KERNEL_SERIAL_

#define PORT 0x3f8   /* COM1 */

void init_serial() 
{
hal->outb(PORT + 1, 0x00);    // Disable all interrupts
hal->outb(PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
hal->outb(PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
hal->outb(PORT + 1, 0x00);    //                  (hi byte)
hal->outb(PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
hal->outb(PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
hal->outb(PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

int c = 0;
void serial_put(char ch)
{
if(c==0) 
 {
 init_serial(); 
 c = 1; 
 }
while (hal->inb(PORT + 5) & 0x20 == 0);
int i; for(i=0;i<100;i++);
//textcolor(LIGHTRED); putchar(ch);
hal->outb(PORT, ch);
}

char serial_get() 
{
while ((hal->inb(PORT + 5) & 1) == 0);
char ch = hal->inb(PORT);
//textcolor(YELLOW); putchar(ch);
return ch;
}

#endif

#ifndef _ENABLE_SERIAL_TERMINAL
void putchar(char ch)
{
hal->terminal->put_char(ch);
}

void textcolor(char color)
{
hal->terminal->set_color(color);
}

#else

#ifndef _ENABLE_KERNEL_SERIAL_
#error Serial terminal needs '_ENABLE_KERNEL_SERIAL_'!
#endif

void putchar(char c)
{
serial_put(c);
if(c == 0x0A)
 serial_put(0x0D);
}

void textcolor(char color)
{
}

#endif

#ifdef _ENABLE_SERIAL_GDB_STUB_

#ifndef _ENABLE_KERNEL_SERIAL_
#error Serial GDB stub needs '_ENABLE_KERNEL_SERIAL_'!
#endif

extern "C" int getDebugChar()
{
return serial_get();
}

extern "C" void putDebugChar(int ch)
{
serial_put(ch);
}

extern "C" void exceptionHandler (int exception_number, void *exception_address)
{
hal->idt->set_isr(exception_number, exception_address);
}

extern "C" void printf(char* format, ...)
{
va_list args;
textcolor(LIGHTGRAY);
va_start(args, format);
vprintf(format, args);
va_end(args);
}

#endif
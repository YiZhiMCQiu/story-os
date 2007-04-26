#ifndef _SYSTEM_H_
#define _SYSTEM_H_

void* page_alloc(int count); //kernel -> stubs.cpp, user -> system.cpp
void die(unsigned int return_code);
void wait_die(unsigned int tid);
void delay(unsigned int millis);
void wait_irq(unsigned int irq);

#endif
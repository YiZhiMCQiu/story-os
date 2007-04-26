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

#include <hal.h>
#include <stdlib.h>

extern "C" void __gxx_personality_v0()
{
hal->panic("__gxx_personality_v0 called!!!\n");
}

extern "C" void __cxa_pure_virtual()
{
hal->panic("PURE VIRTUAL FUNCTION CALLED!!!\n");
}

void* operator new(unsigned int size)
{
return calloc(size, 1);
}

void* operator new[](unsigned int size)
{
return calloc(size, 1);
}

void operator delete(void* p)
{
free(p);
}

void operator delete[](void* p)
{
free(p);
}

void* operator new(unsigned int, unsigned int p)
{
return (void*)p; 
}

void* operator new[](unsigned int, unsigned int p)
{
return (void*)p;
}

void* operator new(unsigned int, void* p)
{
return operator new(0, (unsigned int)p);
}

void* operator new[](unsigned int, void* p)
{
return operator new[](0, (unsigned int)p);
}

void* page_alloc(int count)
{
return hal->mm->alloc(count);
}
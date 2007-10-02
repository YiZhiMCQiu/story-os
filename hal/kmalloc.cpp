#include <hal.h>

char mblock0[0x1000];
struct mb
 {
 unsigned int size;
 bool free;
 mb* next;
 };
mb* first;

void kinit_malloc()
{
first = (mb*) mblock0;
first->size = 0x1000 - sizeof(mb);
first->next = 0;
first->free = true;
}

void* kmorecore(unsigned int pages)
{
return hal->mm->alloc(pages);
}

void* kmalloc(unsigned int size)
{
mb *i, *last;
for(i = first; i; i = i->next)
 {
 if(i->free)
  {
  if(i->size == size)
   {
   i->free = false;
   return (void*) ((unsigned int) i + sizeof(mb));
   }
  else if(i->size > size + sizeof(mb))
   {
   mb* n = (mb*) ((unsigned int) i + size + sizeof(mb));
   n->free = true;
   n->size = i->size - size - sizeof(mb);
   n->next = i->next;
   i->next = n;
   i->size = size;
   i->free = false;
   return (void*) ((unsigned int) i + sizeof(mb));
   }
  }
 if(i->next == NULL)
  last = i;
 }
unsigned int fullsize = bytes_to_pages(size) * 0x1000;
mb *na = (mb*) kmorecore(bytes_to_pages(size)), *nb;
if(na == NULL)
 return NULL;
na->size = size;
na->free = false;
nb = (mb*) ((unsigned int) na + sizeof(mb) + size);
nb->size = bytes_to_pages(size) * 0x1000 - size - 2 * sizeof(mb);
nb->free = true;
na->next = nb;
nb->next = NULL;
last->next = na;
return (void*) ((unsigned int) na + sizeof(mb));
}

void kfree(void* addr)
{
mb *i, *before = NULL;
for(i = first; i; i = i->next)
 {
 if((i->free == false) && ((unsigned int) i == (unsigned int) addr - sizeof(mb)))
  {
  i->free = true;
  if(i->next && i->next->free && ((unsigned int) i + i->size + sizeof(mb) == (unsigned int) i->next))
   {
   i->size += i->next->size + sizeof(mb);
   i->next = i->next->next;
   }
  if(before && before->free && ((unsigned int) before + before->size + sizeof(mb) == (unsigned int) i))
   {
   before->size += i->size + sizeof(mb);
   before->next = i->next;
   }
  return;
  }
 before = i;
 }
hal->panic("Attempt to free unallocated address %X", addr);
}

void kstate()
{
mb* i;
for(i = first; i; i = i->next)
 printf("block @ %X <=> %d: %c\n", i, i->size, i->free ? 'F' : 'A');
}
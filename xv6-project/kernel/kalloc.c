// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void reference_decrease(char *physical_adress);
int get_reference_counter(char* physical_adress);

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
  int counter;
};

struct {

  struct spinlock lock;
  struct run *freelist;
  struct run references[(PHYSTOP) / PGSIZE];
  
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);

}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  // pte_t pte;
  struct run *r;
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  r = (struct run*)pa;

  if(get_reference_counter((char*)pa) > 0){

    reference_decrease((char*) pa);

  }

  if(get_reference_counter((char*)pa) == 0){

    memset(pa, 1, PGSIZE);
    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock); 

  }
    // printf("OHHH000000\n");
    // memset(pa, 1, PGSIZE);
    // acquire(&kmem.lock);
    // r->next = kmem.freelist;
    // kmem.freelist = r;
    // release(&kmem.lock);
  // }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  // printf("LOCK AQCUIRED\n");
  r = kmem.freelist;
  // printf("LOCK AQCUIRED2\n");
  
  if(r){   
    kmem.freelist = r->next;
    // if((uint64)r->next > MAXVA){
    // printf("LOCK AQCUIRED3\n");
    // }
  }
  release(&kmem.lock);

  if(r){

    memset((char*)r, 5, PGSIZE); // fill with junk
    kmem.references[(uint64)((char*)r) / PGSIZE].counter = 1;
    // printf("LOCK AQCUIRED4\n");
  }
  // printf("ACQUIRE5\n");
  return (void*)r;

  // if(r){

  //   r = (struct run*)PA2PTE(((uint64)r - (uint64)kmem.ref_counters) * PGSIZE);

  // }

  // return (void*)r;

}

int get_reference_counter(char* physical_adress){

  int reference ;
  reference = kmem.references[((uint64)(physical_adress)) / PGSIZE].counter;
  return reference;

}

void reference_increase(char *physical_adress){

  acquire(&kmem.lock);
  kmem.references[((uint64)(physical_adress))  / PGSIZE].counter++;
  release(&kmem.lock);

}

void reference_decrease(char *physical_adress){

  if(kmem.references[((uint64)(physical_adress))  / PGSIZE].counter > 0){

    acquire(&kmem.lock);
    kmem.references[(((uint64)physical_adress)) / PGSIZE].counter--;
    release(&kmem.lock);
    
  }

}

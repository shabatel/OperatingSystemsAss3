#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"
#include "mmu.h"

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
  struct {
    union header *ptr;
    uint size;
  } s;
  Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void
free(void *ap)
{
  Header *bp, *p;

  bp = (Header*)ap - 1;
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;
  if(bp + bp->s.size == p->s.ptr){
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  } else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp){
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;
}

//task 1.3
//check if this is the start of page
//then see if it were protected
//all occur free page with regular free


static Header*
morecore(uint nu , int i)
{
  char *p;
  Header *hp;

  if((i == 0) && (nu < 4096))
    nu = 4096;
  p = sbrk(nu * sizeof(Header));
  if(p == (char*)-1)
    return 0;
  hp = (Header*)p;
  hp->s.size = nu;
  free((void*)(hp + 1));
  return freep;
}

void*
malloc(uint nbytes)
{
  Header *p, *prevp;
  uint nunits;

  nunits = (nbytes + sizeof(Header) - 1)/sizeof(Header) + 1;
  if((prevp = freep) == 0){
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }
  for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr){
    if(p->s.size >= nunits){
      if(p->s.size == nunits)
        prevp->s.ptr = p->s.ptr;
      else {
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size = nunits;
      }
      freep = prevp;

      return (void*)(p + 1);
    }
    if(p == freep)
      if((p = morecore(nunits , 0)) == 0)
        return 0;
  }
}


//task 1.1
//call morcore to give us new page space

void*
pmalloc()
{
  char *p;
  Header *hp;
  int new_size = 2 * PGSIZE + sizeof(Header); 
  p = sbrk(new_size);
  if(p == (char*)-1){
    return 0;
  }
  p = (char*)(p + sizeof(Header)); 
  p = (char*)PGROUNDUP((uint)p);
  hp = (Header*)((uint)p - 1);
//  ((Header*)((uint)p - 1))->s.size 512;
  hp->s.size = 512;
  set_as_pmalloc(p);
  return (void*)p;
}
//task 1.2
//first we check address is start of page
//then we check if pointcher (first bytes of he address are 1 or 2, when 2 means the page is alrdy protected)
//if all holds then we protect with pointcher = 2
int 
protect_page(void* ap){
  return sign_as_protected(ap);
}

int
pfree(void* ap){
  if(!check_is_protected(ap)){
    return -1;
  }
  free(ap);
  return 1;
}

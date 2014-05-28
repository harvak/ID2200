/* KTH kursID: ID2200 - Operativsystem 
Labb 3 - malloc

Skrivet av: 
Harald Vaksdal, haraldv@kth.se 
Erik Bäck, erback@kth.se 
(2014) 
Det här biblitoket skapar de inbygga biblioteksfunktionerna malloc, free och realloc

*/




#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include "brk.h" 
  
#ifndef STRATEGY
#define STRATEGY 1 /* Use strategy first as default */
#endif

#define STRATEGY_FIRST 1 
#define STRATEGY_BEST 2

#if STRATEGY < 1 || STRATEGY > 2
#error STRATEGY must contain a value over 1 and below 2
#endif





#define _GNU_SOURCE
#define NALLOC 1024




typedef long Align;                                     /* Alignar block */

union header {                                          /* Struktor flr block header */
  struct {
    union header *ptr;                                  /* Pekare på nästa block */
    unsigned size;                                      /* Storlek för specfikt block */
  } s;
  Align x;                                              
};

typedef union header Header;
 
static Header base;                                     /* Tom lista initieras */
static Header *freep = NULL;                            /* Den fria listan initieras */

/* free() frigör minne och stoppas in det in den länkade listan freep. Denna kod är tagen från: 

http://www.ict.kth.se/courses/ID2206/COURSELIB/malloc/malloc.c


*/

void free(void * ap)
{
  Header *bp, *p;

  if(ap == NULL) return;                                /* Returna om inget headern är NULL */

  bp = (Header *) ap - 1;                               /* Peka på blockets header */
  for(p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr) 
    if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
      break;                                            /* frigort ett block i början eller slutet av freep */

  if(bp + bp->s.size == p->s.ptr) {                     /* Länkar frammåtblock till nytt block i listan*/
    bp->s.size += p->s.ptr->s.size;
    bp->s.ptr = p->s.ptr->s.ptr;
  }
  else
    bp->s.ptr = p->s.ptr;
  if(p + p->s.size == bp) {                             /* Länkar bakomvarande block till nytt block*/
    p->s.size += bp->s.size;
    p->s.ptr = bp->s.ptr;
  } else
    p->s.ptr = bp;
  freep = p;  /*den nya fria länkade listan*/
}

/* morecore: ask system for more memory */

#ifdef MMAP

static void * __endHeap = 0;

void * endHeap(void)
{
  if(__endHeap == 0) __endHeap = sbrk(0);
  return __endHeap;
}
#endif

/* morecore() frågar efter mer minne från operativsystemet och lägger till block i den fria listan: 

http://www.ict.kth.se/courses/ID2206/COURSELIB/malloc/malloc.c


*/


static Header *morecore(unsigned nu)
{
  void *cp;
  Header *up;
#ifdef MMAP
  unsigned noPages;
  if(__endHeap == 0) __endHeap = sbrk(0);
#endif

  if(nu < NALLOC)
    nu = NALLOC;
#ifdef MMAP
  noPages = ((nu*sizeof(Header))-1)/getpagesize() + 1;
  cp = mmap(__endHeap, noPages*getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  nu = (noPages*getpagesize())/sizeof(Header);
  __endHeap += noPages*getpagesize();
#else
  cp = sbrk(nu*sizeof(Header));
#endif
  if(cp == (void *) -1){                                  /* Inget mer minne kunde frigöras */
    perror("failed to get more memory");
    return NULL;
  }
  up = (Header *) cp;
  up->s.size = nu;
  free((void *)(up+1));
  return freep; /*Returnerar den fria listan med mer minne som efterfrågats*/
}


/*
malloc allokerar minne från den fria litan freep. Två algoritmers tillämpas First Fit och Best Fit:

First Fit : Returnerar den första minnesplatsen som mosvarar den efterfrågade minnesallokeringen (nbytes). Detta gynnar 
vanligtvis snabbt minnesallokering framför effektivt minnesutnyttjande eftersom det inte är givet om det allokerade blocket
motsvarar storleken hos nbytes. 


Best Fit: Innebär vanligtvis en mer effektiv metod för utnyttjande av den fria minnet i listan freep eftersom alogritmen
jämför olika minnesblock till den bästa "fitten" för den efterfrågade minnesallokeringen "nbytes" har hittats och returneras därför-
Dessvärre betyder denna mer effektiva alogoritm ur ett resursutnytjingsperspektiv att allokeringsprocessen tar längre tid då
den alltså inte väljer första bästa plats som First Fit gör.


Skalet till malloc har hämtats från:

http://www.ict.kth.se/courses/ID2206/COURSELIB/malloc/malloc.c
*/
void * malloc(size_t nbytes){

  Header *p, *prevp;
  Header * morecore(unsigned);
  unsigned nunits;
  if(nbytes == 0) return NULL;

  nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) +1; /*Beräknar antalet minnesenheter som skall alloekeras*/

  if((prevp = freep) == NULL) {
    base.s.ptr = freep = prevp = &base;
    base.s.size = 0;
  }


  /*Implementerar strategin First Fit*/

  if(STRATEGY == STRATEGY_FIRST){
    for(p= prevp->s.ptr;  ; prevp = p, p = p->s.ptr) {
      if(p->s.size >= nunits) {                           /* En stornog minnesplats har hittats */
        if (p->s.size == nunits)                          /* En perfekt matchning*/
	        prevp->s.ptr = p->s.ptr;
        else {                                            /* Anpassa blocket till nbytes (nunits)*/
	        p->s.size -= nunits;
	        p += p->s.size;
	        p->s.size = nunits;
        }
        freep = prevp;
        return (void *)(p+1);                             /*Retunera blocket som har allokerats*/
      }
      if(p == freep){                                    /* Inget block som är stort nog har hittats därför anropas morecore för att fixa mer minne*/
        if((p = morecore(nunits)) == NULL)
	       return NULL;                                    /*Slut på minne*/
         }                                    
    } 
  } 
 
  /*Implementerar startegin Best Fit*/
  else if (STRATEGY == STRATEGY_BEST){
    Header *best = NULL, *prevbest;
     for(p= prevp->s.ptr;  ; prevp = p, p = p->s.ptr) {
        if (p->s.size == nunits) { /* En perfekt matchning*/
          prevbest = NULL;
          prevp-> s.ptr = p->s.ptr;
          freep = prevp;
          return (void *) (p +1);
        }

        else if(p->s.size > nunits){ /* Ett block in den fria listan passar men är inte perfekt*/
          if (best == NULL){/* Ingen tidigare best fit */
              best = p;
              prevbest = prevp;
          }

          else if(best->s.size > p->s.size) { /* En bättre fit en tidigare bästa*/
              best = p;
              prevbest = prevp;
          }

        } 

        if(p == freep) { /* Den fria listan har loopats igenom */
          if(NULL == best) { /* Ingen best fit har hittats*/
            if((NULL  == morecore(nunits))){ /* Försöker hämta mer minne */
                return NULL;
              }
          }

          else {/* En "bästa" machning*/
            best->s.size -= nunits;/*Tar bort minne från den blcoket*/
            best += best->s.size;/*Gör den nya "best" fit till header */
            best->s.size = nunits; /* Sätt blockets storlek till nunits */
            break;
          }
        
        }

     }
 
      if (best == freep)
            freep = prevp;

     return (void *)(best+1); /*Returnera den "bästa" fitten*/


  }
}

/* Biblioteksfuntkionen realloc omallokerar storleken på ett block och kopierar informationen i det gamla blocket till det nya blocket. */

void *realloc(void * ptr, size_t size){

    Header *newp; /* Den nya pekaren*/
    size_t currSize; /*Den gamla storleken*/


    if (ptr == NULL){
      return malloc(size); /* Om pekaren är null allokera ny minnesmängd*/
    }
    else if (size == 0){ /* Om size är 0 frigör blocket och lägg till i den fria listan*/
      free(ptr);
      return NULL;
    }

    newp = ((Header *)ptr) - 1; 
    currSize = (newp->s.size-1) * sizeof(Header);


    if (currSize > size){
      currSize = size;
    }

    newp = malloc(size); /*Allokera ny minnesstorlek*/
    if(newp == NULL)
      return NULL;

    memcpy(newp, ptr, currSize); /* Kopiera innehållet i den gamla pekaren till nya  med den nya minnesstorleken */
    free(ptr); /* Firgör den gamla pekaren*/

    return newp;

}

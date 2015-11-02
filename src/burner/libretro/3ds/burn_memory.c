// FB Alpha memory management module

// The purpose of this module is to offer replacement functions for standard C/C++ ones
// that allocate and free memory.  This should help deal with the problem of memory
// leaks and non-null pointers on game exit.

#include "burnint.h"

#if 0
#include "3ds.h"
#else
typedef struct {
    UINT32 base_addr;
    UINT32 size;
    UINT32 perm;
    UINT32 state;
} MemInfo;
typedef struct {
    UINT32 flags;
} PageInfo;

#define MEMOP_FREE            0x1
#define MEMOP_ALLOC           0x3
#define MEMOP_ALLOC_LINEAR    0x10003
#define MEMPERM_READ          0x1
#define MEMPERM_WRITE         0x2
#define MEMSTATE_FREE         0x0

INT32 svcQueryMemory(MemInfo* info, PageInfo* out, UINT32 addr);
INT32 svcControlMemory(UINT32* addr_out, UINT32 addr0, UINT32 addr1, UINT32 size, UINT32 op, UINT32 perm);

#endif


void wait_for_input(void);
#define DEBUG_HOLD() do{printf("%s@%s:%d.\n",__FUNCTION__, __FILE__, __LINE__);fflush(stdout);wait_for_input();}while(0)
#define DEBUG_VAR(X) printf( "%-20s: 0x%08X\n", #X, (uint32_t)(X))
#define DEBUG_VAR64(X) printf( #X"\r\t\t\t\t : 0x%016llX\n", (uint64_t)(X))

#define MAX_MEM_PTR	0x400 // more than 1024 malloc calls should be insane...

static UINT8 *memptr[MAX_MEM_PTR]; // pointer to allocated memory
static UINT32 memsize[MAX_MEM_PTR];

// this should be called early on... BurnDrvInit?

static unsigned int total_size = 0;

void BurnInitMemoryManager(void)
{
   memset (memptr, 0, MAX_MEM_PTR * sizeof(UINT8 **));
   memset (memsize, 0, MAX_MEM_PTR * sizeof(UINT32 *));
   total_size = 0;
}

// should we pass the pointer as a variable here so that we can save a pointer to it
// and then ensure it is NULL'd in BurnFree or BurnExitMemoryManager?

/* can be defined by the frontend to free memory when possible/needed */
extern __attribute((weak)) void ctr_request_free_pages(UINT32 pages);

UINT32 ctr_get_mappable_range(UINT32 pages)
{
   MemInfo mem_info;
   PageInfo page_info;
   UINT32 size = pages << 12;
   UINT32 addr = 0x10000000 - 0x1000;

   while(addr > 0x08000000)
   {
      svcQueryMemory(&mem_info, &page_info, addr);

      if((mem_info.state == MEMSTATE_FREE) && (mem_info.size > size))
         return mem_info.base_addr + mem_info.size - size;

      addr = mem_info.base_addr - 0x1000;
   }

   return 0;
}

// call instead of 'malloc'
UINT8 *BurnMalloc(INT32 size)
{
   DEBUG_VAR(size);

   if(!size)
      return NULL;

   for (INT32 i = 0; i < MAX_MEM_PTR; i++)
   {
      if (memptr[i] == NULL)
      {
         if((size&0xFFF) && (size < 0x400000))
         {
            memptr[i]  = malloc(size);
            memsize[i] = 0;
         }
         if (memptr[i] == NULL)
         {
            size = (size + 0xFFF) & ~0xFFF;

            if(ctr_request_free_pages)
               ctr_request_free_pages(size >> 12);

            if(svcControlMemory(&memptr[i], 0, 0, size, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE) < 0)
            {
               UINT32 addr = ctr_get_mappable_range(size >> 12);
               if(svcControlMemory(&memptr[i], addr, 0, size, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE) < 0)
                  {
                     memptr[i] = NULL;
                     printf("size       : 0x%08X\n", size);
                     printf("total_size : 0x%08X\n", total_size);
                     printf("BurnMalloc failed to allocate %d bytes of memory!\n", size);
                     DEBUG_HOLD();
                     exit(0);
                     return NULL;
                  }
            }
            memsize[i] = size;
            total_size += size;
         }

         memset (memptr[i], 0, size); // set contents to 0
         return memptr[i];
      }
   }

   bprintf (0, _T("BurnMalloc called too many times!\n"));

   return NULL; // Freak out!
}

void _BurnFree(void *ptr)
{
	UINT8 *mptr = (UINT8*)ptr;
   void* tmp;

   if(!ptr)
      return;

	for (INT32 i = 0; i < MAX_MEM_PTR; i++)
	{
		if (memptr[i] == mptr) {
         if(!memsize[i])
            free(memptr[i]);
         else
         {
            svcControlMemory(&tmp, memptr[i], 0, memsize[i], MEMOP_FREE, 0x0);
            total_size -= memsize[i];
            memsize[i]  = 0;
         }
         memptr[i]   = NULL;
			break;
		}
	}
}

void BurnExitMemoryManager(void)
{
   void* tmp;
	for (INT32 i = 0; i < MAX_MEM_PTR; i++)
	{
		if (memptr[i] != NULL) {
#if defined FBA_DEBUG
			bprintf(PRINT_ERROR, _T("BurnExitMemoryManager had to free mem pointer %i\n"), i);
#endif
         if(!memsize[i])
            free(memptr[i]);
         else
   			svcControlMemory(&tmp, memptr[i], 0, memsize[i], MEMOP_FREE, MEMPERM_READ | MEMPERM_WRITE);

         memptr[i] = NULL;
         memsize[i] = 0;
		}
	}
   total_size = 0;
}

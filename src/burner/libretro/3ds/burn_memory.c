// FB Alpha memory management module

// The purpose of this module is to offer replacement functions for standard C/C++ ones
// that allocate and free memory.  This should help deal with the problem of memory
// leaks and non-null pointers on game exit.

#include "burnint.h"

//#include "3ds.h"
INT32 svcControlMemory(UINT32* addr_out, UINT32 addr0, UINT32 addr1, UINT32 size, UINT32 op, UINT32 perm);
#define MEMOP_FREE            0x1
#define MEMOP_ALLOC           0x3
#define MEMOP_ALLOC_LINEAR    0x10003
#define MEMPERM_READ          0x1
#define MEMPERM_WRITE         0x2

void wait_for_input(void);
#define DEBUG_HOLD() do{printf("%s@%s:%d.\n",__FUNCTION__, __FILE__, __LINE__);fflush(stdout);wait_for_input();}while(0)
#define DEBUG_VAR(X) printf( "%-20s: 0x%08X\n", #X, (u32)(X))
#define DEBUG_VAR64(X) printf( #X"\r\t\t\t\t : 0x%016llX\n", (u64)(X))

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

// call instead of 'malloc'
UINT8 *BurnMalloc(INT32 size)
{
   size = (size + 0xFFF) & ~0xFFF;

   for (INT32 i = 0; i < MAX_MEM_PTR; i++)
   {
      if (memptr[i] == NULL) {
         if(svcControlMemory(&memptr[i], 0, 0, size, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE) < 0)
            if(svcControlMemory(&memptr[i], 0, 0, size, MEMOP_ALLOC,       MEMPERM_READ | MEMPERM_WRITE) < 0)
            {
               memptr[i] = NULL;
               printf("size       : 0x%08X\n", size);
               printf("total_size : 0x%08X\n", total_size);
               bprintf (0, _T("BurnMalloc failed to allocate %d bytes of memory!\n"), size);
               DEBUG_HOLD();
               exit(0);
               return NULL;
            }
         memsize[i] = size;
         total_size += size;

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

	for (INT32 i = 0; i < MAX_MEM_PTR; i++)
	{
		if (memptr[i] == mptr) {
         svcControlMemory(&tmp, memptr[i], 0, memsize[i], MEMOP_FREE, MEMPERM_READ | MEMPERM_WRITE);
         total_size -= memsize[i];
         memptr[i]   = NULL;
         memsize[i]  = 0;
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
			svcControlMemory(&tmp, memptr[i], 0, memsize[i], MEMOP_FREE, MEMPERM_READ | MEMPERM_WRITE);
			memptr[i] = NULL;
         memsize[i] = 0;
		}
	}
   total_size = 0;
}

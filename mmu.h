#ifndef MMU_H
#define MMU_H

#include <stdio.h>
#include <stdlib.h>

#define LOGICAL_ADDRESS_SPACE_SIZE (1 << 16)
#define TLB_SIZE 16

// Data structure definitions
typedef struct { 
    int page_number; 
    int frame_number; 
    int valid; 
    int time; 
} TLBEntry; 

typedef struct { 
    int frame_number;
    int valid; 
} PageTableEntry; 

typedef struct {
    unsigned char *data; 
    int page_number; 
    int time; // For FIFO replacement strategy
} Frame;


extern TLBEntry *tlb;
extern PageTableEntry *page_table;
extern Frame *physical_memory;
extern FILE *backing_store;

extern int page_bits;
extern int frame_bits;
extern int offset_bits;
extern int page_table_size;
extern int frame_size;
extern int total_frames;

extern int tlb_hits;
extern int page_faults;
extern int total_addresses;


void initializeSystem(int frame_bits_input, int total_frames_input);
void cleanupSystem();

int checkTLB(int page_number, int *frame_number);
void updateTLB(int page_number, int frame_number);

int checkPageTable(int page_number, int *frame_number);
void updatePageTable(int page_number, int frame_number);

// loading & replacement
int loadPage(int page_number);
int replacePage(int page_number);
void readFromBackingStore(int page_number, unsigned char *buffer);

// address translating
unsigned char translateAddress(unsigned short logical_address, int *physical_address);

void generateStatistics(const char *filename);
void printMemoryImage(FILE *file);

#endif // MMU_H

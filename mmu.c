/* Group: 28
 * Usage: A virtual memory management system imulating the process of translating 
 * from a logical address to a physical address, 
 * including TLB looking up, page table looking up, page loading, and page replacement. 
 * View more in README file
 * version: 2.2.3 C
 */

#include "mmu.h"
#include <stdio.h>
#include <stdlib.h>

/* Global Parameters */

// Used to store the most recently used page number 
// to frame number mapping to speed up the address translation process
TLBEntry *tlb = NULL; 

// Used to Maintain a mapping of all pages
// in the logical address space to physical memory frames
PageTableEntry *page_table = NULL; 

Frame *physical_memory = NULL; 
FILE *backing_store = NULL; 

int page_bits = 0; 
int frame_bits = 0; 
int offset_bits = 0; 
int page_table_size = 0; 
int frame_size = 0; 
int total_frames = 0; 

int tlb_hits = 0; 
int page_faults = 0; 
int total_addresses = 0; 

int next_frame = 0; 
int tlb_counter = 0; 
int page_counter = 0; 

/*
 * System initialization
 * 
 * frame_bits_input: Bits of the frame size
 * total_frames_input: Total frames in physical memory
 */

void initializeSystem(int frame_bits_input, int total_frames_input) { 
    int i; 
    
    frame_bits = frame_bits_input; 
    offset_bits = frame_bits; 
    page_bits = 16 - offset_bits; 
    
    frame_size = 1 << frame_bits; 
    page_table_size = 1 << page_bits; 
    total_frames = total_frames_input; 
    
    tlb = (TLBEntry *)malloc(TLB_SIZE * sizeof(TLBEntry)); 
    if (tlb == NULL) { 
        fprintf(stderr, "error in allocating TLB memory\n"); 
        exit(EXIT_FAILURE); 
    } 
    
    for (i = 0; i < TLB_SIZE; i++) { 
        tlb[i].valid = 0; 
        tlb[i].page_number = -1; 
        tlb[i].frame_number = -1; 
        tlb[i].time = -1; 
    } 
    
    // Allocate page table memory
    page_table = (PageTableEntry *)malloc(page_table_size * sizeof(PageTableEntry)); 
    if (page_table == NULL) { 
        fprintf(stderr, "Cannot allocate page tabel\n"); 
        exit(EXIT_FAILURE); 
    } 
    
    // initialte page table
    for (i = 0; i < page_table_size; i++) { 
        page_table[i].valid = 0; 
        page_table[i].frame_number = -1; 
    } 
    
    // allocate phisical memory
    physical_memory = (Frame *)malloc(total_frames * sizeof(Frame)); 
    if (physical_memory == NULL) { 
        fprintf(stderr, "Cannnot allocate physical memory\n"); 
        exit(EXIT_FAILURE); 
    } 
    
    // initiate ohysical memory
    for (i = 0; i < total_frames; i++) { 
        physical_memory[i].data = (unsigned char *)malloc(frame_size * sizeof(unsigned char)); 
        if (physical_memory[i].data == NULL) { 
            fprintf(stderr, "Unable to allocate physical memory frame data\n"); 
            exit(EXIT_FAILURE); 
        } 
        physical_memory[i].page_number = -1; 
        physical_memory[i].time = -1; 
    } 
    
    backing_store = fopen("backingstore.bin", "rb"); 
    if (backing_store == NULL) { 
        fprintf(stderr, "cannot load backingstore.bin\n"); 
        exit(EXIT_FAILURE); 
    } 
    
    tlb_hits = 0; 
    page_faults = 0; 
    total_addresses = 0; 
    
    next_frame = 0; 
    tlb_counter = 0; 
    page_counter = 0; 
} 

void cleanupSystem() { 
    int i; 
    
    // free tlb
    if (tlb != NULL) { 
        free(tlb); 
        tlb = NULL; 
    } 
    
    // free page table
    if (page_table != NULL) { 
        free(page_table); 
        page_table = NULL; 
    } 
    
    // free physical memory
    if (physical_memory != NULL) { 
        for (i = 0; i < total_frames; i++) { 
            if (physical_memory[i].data != NULL) { 
                free(physical_memory[i].data); 
                physical_memory[i].data = NULL; 
            } 
        } 
        free(physical_memory); 
        physical_memory = NULL; 
    } 
    
    // file close func fro safety
    if (backing_store != NULL) { 
        fclose(backing_store); 
        backing_store = NULL; 
    } 
} 

/**
 * check TLB
 * if found return 1, if not return 0
 */
int checkTLB(int page_number, int *frame_number) { 
    int i; 
    
    for (i = 0; i < TLB_SIZE; i++) { 
        if (tlb[i].valid && tlb[i].page_number == page_number) { 
            *frame_number = tlb[i].frame_number; 
            return 1; 
        } 
    } 
    
    return 0; 
} 

void updateTLB(int page_number, int frame_number) { 
    int i; 
    int oldest_time = tlb[0].time; 
    int oldest_index = 0; 
    
    // check whether the page number exists
    for (i = 0; i < TLB_SIZE; i++) { 
        if (tlb[i].valid && tlb[i].page_number == page_number) { 
            tlb[i].frame_number = frame_number; 
            tlb[i].time = tlb_counter++; 
            return; 
        } 
    } 
    
    // look for free entry
    for (i = 0; i < TLB_SIZE; i++) { 
        if (!tlb[i].valid) { 
            tlb[i].valid = 1; 
            tlb[i].page_number = page_number; 
            tlb[i].frame_number = frame_number; 
            tlb[i].time = tlb_counter++; 
            return; 
        } 
    } 
    
    // look for olest entry
    for (i = 1; i < TLB_SIZE; i++) { 
        if (tlb[i].time < oldest_time) { 
            oldest_time = tlb[i].time; 
            oldest_index = i; 
        } 
    } 
    
    // replace oldest entry
    tlb[oldest_index].page_number = page_number; 
    tlb[oldest_index].frame_number = frame_number; 
    tlb[oldest_index].time = tlb_counter++; 
} 

/**
 * check TLB
 * if valid return 1, if not return 0
 */
int checkPageTable(int page_number, int *frame_number) { 
    if (page_table[page_number].valid) { 
        *frame_number = page_table[page_number].frame_number; 
        return 1; 
    } 
    
    return 0; 
} 

void updatePageTable(int page_number, int frame_number) { 
    page_table[page_number].valid = 1; 
    page_table[page_number].frame_number = frame_number; 
} 

void readFromBackingStore(int page_number, unsigned char *buffer) { 
    if (fseek(backing_store, page_number * frame_size, SEEK_SET) != 0) { 
        fprintf(stderr, "Unable to locate page\n"); 
        exit(EXIT_FAILURE); 
    } 
    
    if (fread(buffer, sizeof(unsigned char), frame_size, backing_store) != frame_size) { 
        fprintf(stderr, "Unable to load page\n"); 
        exit(EXIT_FAILURE); 
    } 
} 

/**
 * load apge to free frame: 
 * return allocated frame number
 */
int loadPage(int page_number) { 
    int frame_number; 
    
    // cehck if a free frame exists
    if (next_frame < total_frames) { 
        frame_number = next_frame++; 
    } else { 
        // if none, replace
        return replacePage(page_number); 
    } 
    
    // read from backing store
    readFromBackingStore(page_number, physical_memory[frame_number].data); 
    
    // update physical memory
    physical_memory[frame_number].page_number = page_number; 
    physical_memory[frame_number].time = page_counter++; 
    
    updatePageTable(page_number, frame_number); 
    
    printf("\t [Load Page] Page %d -> Frame %d\n", page_number, frame_number); 
    
    return frame_number; 
} 

int replacePage(int page_number) { 
    int i; 
    int oldest_time = physical_memory[0].time; 
    int oldest_index = 0; 
    int replaced_page; 
    
    for (i = 1; i < total_frames; i++) { 
        if (physical_memory[i].time < oldest_time) { 
            oldest_time = physical_memory[i].time; 
            oldest_index = i; 
        } 
    } 
    
    replaced_page = physical_memory[oldest_index].page_number; 
    
    readFromBackingStore(page_number, physical_memory[oldest_index].data); 
    
    // update physical memory
    physical_memory[oldest_index].page_number = page_number; 
    physical_memory[oldest_index].time = page_counter++; 
    
    // update page table
    page_table[replaced_page].valid = 0; // mark replaced page invalid
    updatePageTable(page_number, oldest_index); 
    
    // If the replaced page is in the TLB, update the TLB
    for (i = 0; i < TLB_SIZE; i++) { 
        if (tlb[i].valid && tlb[i].page_number == replaced_page) { 
            tlb[i].valid = 1; 
            tlb[i].page_number = page_number; 
            tlb[i].frame_number = oldest_index; 
            tlb[i].time = tlb_counter++; 
            break; 
        } 
    } 
    
    printf("\t [Replace page] Frame %d: Page %d -> Page %d\n", oldest_index, replaced_page, page_number); 
    
    return oldest_index; 
} 

/**
 * Converting logical addresses to physical addresses
 * 
 * logical_address: logical address
 * physical_address: Pointer to the physical address
 *                  (used to return the calculated physical address)
 * return the byte value stored at this physical address
 */
unsigned char translateAddress(unsigned short logical_address, int *physical_address) { 
    int page_number; 
    int offset; 
    int frame_number; 
    unsigned char data; 
    
    // get page number and offste
    page_number = (logical_address >> offset_bits) & ((1 << page_bits) - 1); 
    offset = logical_address & ((1 << offset_bits) - 1); 
    
    total_addresses++; 
    
    // check TLB
    if (checkTLB(page_number, &frame_number)) { 
        tlb_hits++; 
        *physical_address = (frame_number << offset_bits) | offset; 
        data = physical_memory[frame_number].data[offset]; 
        printf("[TLB] (%d) %d -> (%d) %d: %d\n",  
               logical_address, page_number, *physical_address, frame_number, data); 
        return data; 
    } 
    
    // check page table
    if (checkPageTable(page_number, &frame_number)) { 
        *physical_address = (frame_number << offset_bits) | offset; 
        data = physical_memory[frame_number].data[offset]; 
        printf("[Page Table] (%d) %d -> (%d) %d: %d\n", logical_address, page_number, *physical_address, frame_number, data); 
        
        updateTLB(page_number, frame_number); 
        
        return data; 
    }
    
    page_faults++; 
    
    // load page
    frame_number = loadPage(page_number); 
    
    // calculate phyicale address
    *physical_address = (frame_number << offset_bits) | offset; 
    data = physical_memory[frame_number].data[offset]; 
    
    updateTLB(page_number, frame_number); 
    
    return data; 
}

void generateStatistics(const char *filename) { 
    FILE *file = fopen(filename, "w"); 
    if (file == NULL) { 
        fprintf(stderr, "cannot generate file\n"); 
        return; 
    } 
    
    // Calculating Page Error Rates and TLB Hit Rates
    double page_fault_rate = (double)page_faults / total_addresses; 
    double tlb_hit_rate = (double)tlb_hits / total_addresses; 
    
    // write file
    fprintf(file, "page-fault rate: %.1f\n\n", page_fault_rate); 
    fprintf(file, "TLB hit rate: %.1f\n\n", tlb_hit_rate); 
    fprintf(file, "Memory image:\n"); 
    
    printMemoryImage(file); 
    
    // file colse function to ensure safety
    fclose(file); 
} 

void printMemoryImage(FILE *file) { 
    int i, j; 
    int rows = (total_frames + 15) / 16; 
    
    for (i = 0; i < rows; i++) { 
        fprintf(file, "Frame %d ~ Frame %d:", i * 16, i * 16 + 15); 
        
        for (j = 0; j < 16; j++) { 
            int index = i * 16 + j; 
            if (index < total_frames) { 
                if (physical_memory[index].page_number != -1) { 
                    fprintf(file, " %d", physical_memory[index].page_number); 
                } else { 
                    fprintf(file, " -1"); 
                } 
            } 
        } 
        
        fprintf(file, "\n"); 
    } 
} 

int main() { 
    FILE *address_file; 
    unsigned short logical_address; 
    int physical_address; 
    unsigned char data; 
    int frame_bits_input; 
    int total_frames_input; 
    int total_memory_size; 
    
    // Input frame size and total number of frames
    do { 
        printf("Please enter the frame size: "); 
        scanf("%d", &frame_bits_input); 
        
        printf("Please enter the total frames number in physical memory: "); 
        scanf("%d", &total_frames_input); 
        
        total_memory_size = (1 << frame_bits_input) * total_frames_input; 
        
        if ((total_memory_size & (total_memory_size - 1)) != 0) { 
            printf("Warning: Total physical memory size must be power of 2.\n"); 
            continue; 
        } 
        
        if (total_memory_size > LOGICAL_ADDRESS_SPACE_SIZE) { 
            printf("Warning: Physical memory size must be less than logical space size (%d bytes)\n", LOGICAL_ADDRESS_SPACE_SIZE); 
            continue; 
        } 
        
        break; 
    } while (1); 
    
    initializeSystem(frame_bits_input, total_frames_input); 
    
    address_file = fopen("addresses.txt", "r"); 
    if (address_file == NULL) { 
        fprintf(stderr, "Cannot open addresses.txt\n"); 
        cleanupSystem(); 
        return EXIT_FAILURE; 
    } 
    
    while (fscanf(address_file, "%hu", &logical_address) == 1) { 
        data = translateAddress(logical_address, &physical_address); 
    } 
    
    // file close func to enseure safety
    fclose(address_file); 
    
    generateStatistics("stat.txt"); 
    
    // clean up
    cleanupSystem(); 
    
    printf("\nFinished writing to stat.txt\n"); 
    
    return EXIT_SUCCESS; 
} 

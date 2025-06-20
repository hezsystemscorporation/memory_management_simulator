# README: Virtual Memory Management System Implementation

## Producers
**Xin HE**: Responsible for Function Design and Code Implementation.   
**Zhibo FENG**: Resibonsible for Function Testing.  
And **Co-Producers** Houze HE and Zisen FENG.   

## Overview
Virtual memory is a memory management technique that gives processes the illusion of a large, contiguous address space while physically utilizing limited RAM. This implementation in C (`mmu.c`) demonstrates the core mechanisms of virtual memory: address translation, caching with a Translation Lookaside Buffer (TLB), page tables, page faults, and page replacement algorithms.

## Theoretical Background

### Virtual Memory
- **Abstraction**: Each process operates in its own logical address space, decoupled from the physical RAM layout.
- **Paging**: Logical memory is divided into fixed-size pages, while physical memory is divided into frames of the same size.
- **Demand Paging**: Pages are loaded into memory only when accessed, minimizing memory usage.

### Address Translation
- **Logical Address** (16-bit): Composed of a page number (high bits) and an offset (low bits). If the system uses `n` frame bits, then the offset is `n` bits, and the page number is `16 - n` bits.
- **Physical Address**: Constructed by concatenating the frame number (from TLB or page table) with the offset.

### Translation Lookaside Buffer (TLB)
- **Purpose**: A small, fast cache that stores recent page-to-frame mappings to reduce translation time.
- **Hit/Miss**: On a TLB hit, translation completes in one memory access; on a miss, the page table must be consulted.
- **Replacement Policy**: FIFO is used here: when the TLB is full, the oldest entry is evicted.
- **Hit Rate Impact**: A high TLB hit rate significantly improves performance by avoiding page table lookups.

### Page Table
- **Structure**: An array indexed by page number; each entry holds a frame number and a valid bit.
- **Valid Bit**: Indicates if the page is currently loaded in physical memory.
- **Lookup Cost**: A TLB miss followed by a page table lookup incurs additional memory accesses.

### Page Faults and Backing Store
- **Page Fault**: Occurs when a referenced page is not in physical memory (valid bit = 0).
- **Backing Store**: A binary file (`backingstore.bin`) simulating secondary storage; holds the full contents of all logical pages.
- **Service**: On a page fault, the required page is read from the backing store into a free frame.

### Page Replacement
- **When Needed**: If no free frames remain, a replacement algorithm selects a frame to evict.
- **FIFO Replacement**: The frame loaded earliest (smallest timestamp) is chosen for eviction. The corresponding page table entry and TLB entry are updated.

## Key Components in Implementation
1. **Data Structures**
   - `TLBEntry tlb[TLB_SIZE]`: Array of entries `{page_number, frame_number, valid, time}`.
   - `PageTableEntry page_table[2^(16-n)]`: Entries `{frame_number, valid}`.
   - `Frame physical_memory[total_frames]`: Each `Frame` holds `data[]`, `page_number`, and `time`.

2. **Initialization**
   - Compute `offset_bits = frame_bits`, `page_bits = 16 - offset_bits`.
   - Allocate and zero-initialize TLB, page table, and physical memory arrays.
   - Open `backingstore.bin` for reading pages.

3. **Translation Process**
   1. Extract `page_number = logical_address >> offset_bits`.
   2. Extract `offset = logical_address & ((1 << offset_bits) - 1)`.
   3. **TLB Lookup**: If hit, use cached frame number; else
   4. **Page Table Lookup**: If valid, use frame; else
   5. **Page Fault Handling**:
      - If free frames exist, load page; otherwise, `replacePage()`.
   6. **Update Structures**: TLB and page table with new mapping.
   7. Compute `physical_address = (frame_number << offset_bits) | offset`.

4. **Statistics**
   - **Counters**: `total_addresses`, `tlb_hits`, `page_faults`.
   - **Rates**: `TLB Hit Rate = tlb_hits / total_addresses`; `Page Fault Rate = page_faults / total_addresses`.
   - **Memory Image**: A snapshot of `physical_memory[]` mapping pages to frames, printed 16 frames per row.

## Execution Flow
1. **User Input**:
   - `frame_bits_input`: Number of offset bits (frame size = 2^k bytes).
   - `total_frames_input`: Total frames in physical memory.
   - Validation ensures physical memory size < 64 KB and is a power of two.

2. **System Setup**:
   - Call `initializeSystem(frame_bits_input, total_frames_input)`.

3. **Address File Processing**:
   - Open `addresses.txt` (list of logical addresses).
   - For each address, call `translateAddress()` and optionally log output.

4. **Results Generation**:
   - After processing all addresses, call `generateStatistics("stat.txt")`.

5. **Cleanup**:
   - Release all allocated memory and close files using `cleanupSystem()`.

## Usage
```bash
# Compile
gcc -o mmu mmu.c

# Run
./mmu
```  
And please follow the prompts to enter frame size and number of frames.

## Notes
- All time stamps (`tlb_counter`, `page_counter`) implement FIFO ordering.
- The page replacement and TLB replacement use the same FIFO concept.

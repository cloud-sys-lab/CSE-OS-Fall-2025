#include <iostream>
#include <vector>
#include <iomanip>

// Let's try to simulate virtual memory!

// going to use structs instead of bitwise operations
// but we can still account for the offset of bits and such

int PAGE_SIZE = 4096; // bytes, 4K
int PAGE_COUNT = 1024; // 1024 entries in the page table
int FRAME_SIZE = 4096; // bytes, 4K per physical frame size
int FRAME_COUNT = 1024; // 1024 physical frames


struct pageTableEntry {
    bool modifyBit = false;
    bool referenceBit = false;
    bool validBit = false;
    bool readBit = false; // will worry about permissions later
    bool writeBit = false; // ^^
    bool executeBit = false; // ^^
    bool presentBit = false; // will implement page replacement later
    int pageFrameNum = -1;
};

int virtualToPhysicalAddress (int virtualAddress, const std::vector<pageTableEntry>& pageTable) {
    // first let's calculate the offset and find the virtual page number
    int offset = virtualAddress & (PAGE_SIZE - 1); // offset is based on the size of each page
    int virtualPageNumber = virtualAddress / PAGE_SIZE; // essentially shifts the number right by 12 bits

    // now lets validate the virtual address (bounds and valid bit)
    if (virtualPageNumber >= PAGE_COUNT || virtualPageNumber < 0) {
        std::cerr << "[ERR] Attempted to access out-of-bound virtual address!" << std::endl;
        return -1;
    }

    const pageTableEntry& entry = pageTable[virtualPageNumber];

    if (!entry.validBit) {
        std::cerr << "[ERR] Page fault occurred!" << std::endl;
        return -1;
    }

    // now we can map the virtual to the physical
    int pageFrameNum = entry.pageFrameNum;
    int physicalAddress = (pageFrameNum * FRAME_SIZE) + offset;


    return physicalAddress;
}

int* readPhysicalMemory (int physicalAddress, std::vector<std::vector<int>>& physicalMemory) {
    // first lets calculate the offsets n such
    int frameNumber = physicalAddress / FRAME_SIZE; // 12 bit shift
    int byteOffset = physicalAddress % FRAME_SIZE;

    // check bounds
    if (frameNumber >= FRAME_COUNT || frameNumber < 0) {
        std::cerr << "[ERR] Attempted to access out-of-bound physical address!" << std::endl;
        return nullptr;
    }

    // make sure we're at the start of a frame
    if (byteOffset % sizeof(int) != 0 || byteOffset >= FRAME_SIZE) {
        std::cerr << "[ERR] Misaligned or out-of-bounds access within frame!" << std::endl;
        return nullptr;
    }

    // calculate index based on sizeof(int)
    int index = byteOffset / sizeof(int);

    // return pointer to the value
    return &physicalMemory[frameNumber][index];
}

void printPageTableEntry(int virtualPageNumber, const std::vector<pageTableEntry>& pageTable) {
    if (virtualPageNumber >= PAGE_COUNT) {
        std::cerr << "[ERR] Invalid page number" << std::endl;
        return;
    }

    const pageTableEntry& entry = pageTable[virtualPageNumber];
    std::cout << "Page " << virtualPageNumber << ": ";
    std::cout << "Valid = " << entry.validBit;
    std::cout << ", Present = " << entry.presentBit;
    std::cout << ", Frame = " << entry.pageFrameNum;
    std::cout << ", Referenced = " << entry.referenceBit;
    std::cout << ", Modified = " << entry.modifyBit;
    std::cout << std::endl;
}


int main() {
    // lets start by creating the pageTable and an vector representing physical memory
    std::vector<pageTableEntry> pageTable;
    pageTable.resize(PAGE_COUNT);

    std::vector<std::vector<int>> physicalMemory;
    physicalMemory.resize(FRAME_COUNT, std::vector<int>(FRAME_SIZE));

    // lets create an example entry and print it's addresses or something
    pageTable[0].validBit = true;
    pageTable[0].presentBit = true;
    pageTable[0].modifyBit = true;
    pageTable[0].referenceBit = true;
    pageTable[0].pageFrameNum = 69;

    for (int i = 0; i < FRAME_SIZE / sizeof(int); i++) {
        physicalMemory[69][i] = i + 420;
    }

    int virtualAddress = 0x0000 + (69 * sizeof(int));
    int physicalAddress = virtualToPhysicalAddress(virtualAddress, pageTable);
    int* valuePointer = readPhysicalMemory(physicalAddress, physicalMemory);

    std::cout << std::showbase << std::hex;
    std::cout << "Page Table Entry for Virtual Address " << virtualAddress << ": " << std::endl;
    printPageTableEntry(1, pageTable);

    std::cout << "Virtual Address: " << virtualAddress << std::endl;
    std::cout << "Physical Address: " << physicalAddress << std::endl;
    std::cout << "Value at Physical Address: " << std::dec << *valuePointer << std::endl;

    return 0;

}

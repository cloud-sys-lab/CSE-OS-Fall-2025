#include <iostream>
#include <vector>
#include <cstdint>
// #include <stdexcept> // will fix error handling laterrrr

// Let's try to simulate virtual memory!

// going to use structs instead of bitwise operations
// but we can still account for the offset of bits and such


struct pageTableEntry {
    bool validBit = false;
    bool presentBit = false; // will implement page replacement later
    bool referenceBit = false;
    bool modifyBit = false;
    int pageFrameNum = -1;
};

class MemoryManager {
    private:
        std::vector<pageTableEntry> pageTable;
        std::vector<uint8_t> physicalMemory;
        std::vector<bool> freeFrames;

        std::vector<uint8_t> diskStorage; // just going to simulate disk storage with a vector

        int PAGE_SIZE; // 4096 bytes, 4K per page
        int PAGE_COUNT; // 1024 entries in the page table
        int PHYSICAL_SIZE; // # of bytes of physical memory

        int clockPointer = 0; // for CLOCK page replacement

        void _initializeMemory() {
            pageTable.resize(PAGE_COUNT);
            physicalMemory.resize(PHYSICAL_SIZE);
            freeFrames.resize(PHYSICAL_SIZE / PAGE_SIZE, true);

            diskStorage.resize(PAGE_COUNT * PAGE_SIZE); // give disk space for every entry
        }

        void writeMemory(int physicalAddress, uint8_t data) {
            if (physicalAddress >= PHYSICAL_SIZE || physicalAddress < 0)
                throw "Physical address out of bounds!";

            physicalMemory[physicalAddress] = data;
        }

        uint8_t readMemory(int physicalAddress) {
            if (physicalAddress >= PHYSICAL_SIZE || physicalAddress < 0)
                throw "Physical address out of bounds!";

            return physicalMemory[physicalAddress];
        }

        int virtualToPhysicalAddress(int virtualAddress, bool writeOperation) {
            // first let's calculate the offset and find the virtual page number
            int offset = virtualAddress & (PAGE_SIZE - 1); // offset is based on the size of each page
            int virtualPageNumber = virtualAddress / PAGE_SIZE; // essentially shifts the number right by 12 bits

            // now lets validate the virtual address (bounds and valid bit)
            if (virtualPageNumber >= PAGE_COUNT || virtualPageNumber < 0)
                throw "Attempted to access out-of-bound virtual address!";

            pageTableEntry& entry = pageTable[virtualPageNumber];

            if (!entry.validBit)
                throw "Segmentation fault occurred!";

            if (!entry.presentBit) {
                _handlePageFault(virtualPageNumber);
            }

            // now we can map the virtual to the physical
            int pageFrameNum = entry.pageFrameNum;
            int physicalAddress = (pageFrameNum * PAGE_SIZE) + offset;

            if (physicalAddress >= PHYSICAL_SIZE || physicalAddress < 0)
                throw "Physical address out of bounds!";

            entry.referenceBit = true;
            if (writeOperation) entry.modifyBit = true;

            return physicalAddress;
        }

        // for securely deleting a page
        void _wipeMemoryFrame(int frameNumber){
            if (frameNumber >= (PHYSICAL_SIZE / PAGE_SIZE) || frameNumber < 0)
                throw "Invalid frame number!";

            int baseAddress = frameNumber * PAGE_SIZE;

            for (int i = 0; i < PAGE_SIZE; i++) {
                physicalMemory[baseAddress + i] = 0;
            }
        }

        void _handlePageFault(int virtualPageNumber){
            std::cout << "Page fault at VPN: " <<virtualPageNumber << std::endl;

            pageTableEntry& entry = pageTable[virtualPageNumber];

            // check for free frame, if none, page replacement
            int freeFrame = -1;
            for (int i = 0; i < freeFrames.size(); i++) {
                if (freeFrames[i]) {
                    freeFrame = i;
                    break;
                }
            }

            if (freeFrame == -1) {
                entry.pageFrameNum = _replacePage();
                _readPageFromDisk(virtualPageNumber, entry.pageFrameNum);
            } else
                entry.pageFrameNum = freeFrame;

            freeFrames[entry.pageFrameNum] = false;

            entry.presentBit = true;
        }

        // returns new frame number
        int _replacePage() {
            std::cout << "No free frame found, replacing page" << std::endl;

            int frameNumber = -1;
            int replacedVPN = -1;
            bool entryWasModified = false;

            bool ticking = true;
            while (ticking) {
                if (clockPointer == PAGE_COUNT) clockPointer = 0;

                pageTableEntry& entry = pageTable[clockPointer];

                if (entry.presentBit) {
                    if (entry.referenceBit) {
                        entry.referenceBit = false;
                    } else {
                        entry.presentBit = false;
                        frameNumber = entry.pageFrameNum;
                        replacedVPN = clockPointer;
                        entryWasModified = entry.modifyBit;
                        ticking = false;
                    }
                }

                clockPointer++;
            }

            if (entryWasModified) _writePageToDisk(replacedVPN);

            freeFrames[frameNumber] = true;
            pageTable[replacedVPN].pageFrameNum = -1;
            _wipeMemoryFrame(frameNumber);

            return frameNumber;
        }

        void _writePageToDisk(int virtualPageNumber){
            int frameNum = pageTable[virtualPageNumber].pageFrameNum;

            for (int i = 0; i < PAGE_SIZE; i++) {
                diskStorage[virtualPageNumber * PAGE_SIZE + i] = physicalMemory[frameNum * PAGE_SIZE + i];
            }
        }

        void _readPageFromDisk(int virtualPageNumber, int frameNumber){
            for (int i = 0; i < PAGE_SIZE; i++) {
                physicalMemory[frameNumber * PAGE_SIZE + i] = diskStorage[virtualPageNumber * PAGE_SIZE + i];
            }
        }

        void _deletePageFromDisk(int virtualPageNumber) {
            for (int i = 0; i < PAGE_SIZE; i++) {
                diskStorage[virtualPageNumber * PAGE_SIZE + i] = 0;
            }
        }

    public:
        MemoryManager()
            : PAGE_SIZE(4096), PAGE_COUNT(1024), PHYSICAL_SIZE(4194304){
            _initializeMemory();
        }

        MemoryManager(int page_size, int num_pages, int num_bytes)
            : PAGE_SIZE(page_size), PAGE_COUNT(num_pages), PHYSICAL_SIZE(num_bytes){
            _initializeMemory();
        }

        void allocatePage(int virtualPageNumber, int frameNumber) {
            if (virtualPageNumber >= PAGE_COUNT || virtualPageNumber < 0)
                throw "Invalid virtual page number!";
            if (frameNumber >= (PHYSICAL_SIZE / PAGE_SIZE) || frameNumber < 0)
                throw "Invalid frame number!";

            pageTableEntry& entry = pageTable[virtualPageNumber];


            entry.validBit = true;
            entry.pageFrameNum = frameNumber;
            entry.presentBit = true;

            freeFrames[frameNumber] = false;
        }

        // returns virtual memory address
        int allocateAnyPage() {
            // find open page entry
            int vpn = -1;
            for (int i = 0; i < PAGE_COUNT; i++) {
                if (pageTable[i].validBit != true) {
                    vpn = i;
                    break;
                }
            }
            if (vpn == -1) throw "No free pages!";

            // find open frame using our map of them :P
            int freeFrame = -1;
            for (int i = 0; i < freeFrames.size(); i++) {
                if (freeFrames[i]) {
                    freeFrame = i;
                    break;
                }
            }

            if (freeFrame == -1) freeFrame = _replacePage(); // page replacement here!

            allocatePage(vpn, freeFrame);

            int virtualAddress = vpn * PAGE_SIZE;
            return virtualAddress;
        }

        void writeVirtualMemory (int virtualAddress, uint8_t data) {
            int physicalAddress = virtualToPhysicalAddress(virtualAddress, true);
            writeMemory(physicalAddress, data);
        }

        uint8_t readVirtualMemory (int virtualAddress) {
            int physicalAddress = virtualToPhysicalAddress(virtualAddress, false);
            return readMemory(physicalAddress);
        }

        void deletePageTableEntry(int virtualAddress) {
            int virtualPageNumber = virtualAddress / PAGE_SIZE;
            if (virtualPageNumber >= PAGE_COUNT || virtualPageNumber < 0)
                throw "Attempted to access out-of-bound virtual address!";

            pageTableEntry& entry = pageTable[virtualPageNumber];

            if (!entry.validBit)
                throw "Can't delete invalid page";

            entry.validBit = false;

            if (entry.presentBit) {
                _wipeMemoryFrame(entry.pageFrameNum);
                entry.presentBit = false;
                freeFrames[entry.pageFrameNum] = true;
                entry.pageFrameNum = -1;

            } else {
                _deletePageFromDisk(virtualPageNumber);
            }

            entry.modifyBit = false;
            entry.referenceBit = false;
        }

        void printPageTableEntry(int virtualPageNumber) {
            if (virtualPageNumber >= PAGE_COUNT)
                throw "Invalid page number";

            const pageTableEntry& entry = pageTable[virtualPageNumber];
            std::cout << "Page " << virtualPageNumber << ": ";
            std::cout << "Valid = " << entry.validBit;
            std::cout << ", Present = " << entry.presentBit;
            std::cout << ", Frame = " << entry.pageFrameNum;
            std::cout << ", Referenced = " << entry.referenceBit;
            std::cout << ", Modified = " << entry.modifyBit;
            std::cout << std::endl;
        }
};

int main() {
    try {
        MemoryManager mm(4096, 1024, 2*4096);

        int virtualAddress = mm.allocateAnyPage();

        std::cout << std::showbase << std::hex << "Allocated page for future tests: " <<
        virtualAddress << std::endl;

        for (int i = 0; i < 4096; i++) {
            uint8_t data = i % 256;
            mm.writeVirtualMemory(virtualAddress + i, data);
        }

        int bloop[5] = {0x0485, 0x0089, 0x0a5f, 0x076e, 0x0f80};

        for (auto random : bloop) {
            int addr = random + virtualAddress;
            std::cout << std::showbase << std::hex;
            std::cout << "Page Table Entry for Virtual Address " << addr << ": " << std::endl;
            mm.printPageTableEntry(addr / 4096);

            std::cout << "Virtual Address: " << addr << std::endl;
            std::cout << "Value at Physical Address: " << std::dec <<
            static_cast<int>(mm.readVirtualMemory(addr)) << std::endl << std::endl;
        }

        std::cout << std::showbase << std::hex;
        int newAddress = mm.allocateAnyPage();

        int newAddress2 = mm.allocateAnyPage();
        std::cout << "Page Table Entry for Virtual Address " << virtualAddress << ": " << std::endl;
        mm.printPageTableEntry(virtualAddress / 4096);

        std::cout << "Page Table Entry for Virtual Address " << newAddress << ": " << std::endl;
        mm.printPageTableEntry(newAddress / 4096);

        std::cout << "Page Table Entry for Virtual Address " << newAddress2 << ": " << std::endl;
        mm.printPageTableEntry(newAddress2 / 4096);

        return 0;
    } catch (const char* error) {
        std::cerr << "[FATAL] " << error << std::endl;
        return -1;
    }
}

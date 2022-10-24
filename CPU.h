#ifndef CPU_INCLUDED
#define CPU_INCLUDED

#include <vector>
#include <iostream>
#include <bitset>

// Opcode types
#define ZERO 0x0  // Program complete after 5x in a row
#define RTYPE 0x33
#define ITYPE 0x13
#define LOADWORD 0x3
#define STOREWORD 0x23
#define BTYPE 0x63
#define JTYPE 0x67


class CPU {
public:

    explicit CPU(std::vector<uint8_t>&& iMem);  //initialize CPU
    
    void Fetch();                               // Gets instruction to work on for this clock cycle in binary form
    void Decode();                              // Decodes binary instructions 
    void Execute();                             // Executes ALU functtions 
    void Memory();                              // Accesses memory (LW, SW)
    void Writeback();                           // Writes result to register (r-type, i-type) or memory (LW)

    void clockTick();                           // Ends cycle. Updates clock and PC (branch, jalr)
    bool isFinished();                          // Checks if process is complete using ZERO op

    void printInfo();                           // Prints result total cycles, r-type instruction count, and IPC
    void printResult();                         // Prints just the result

private:
    // Processor essentials
    std::vector<uint8_t> instrMem;  
    std::vector<uint8_t> dataMem;   
    int32_t reg[32];       
    uint32_t PC;                    
    unsigned short killCounter;     
    unsigned int clockCount;        
   
    // Track number of each type of instruction
    unsigned int rTypeCount;        
    unsigned int iTypeCount;        
    unsigned int bTypeCount;
    unsigned int jalrCount;
    unsigned int swCount;           
    unsigned int lwCount;           
    
    // Track Total Instruction Count and Instruction per Cycle
    unsigned int totalInstrCount;     
    double ipc;                    

    // CPU control 
    enum class Op {                      
        ADD,
        SUB,
        ADDI,
        XOR,
        ANDI,
        SRA,
        LW,
        SW,
        BLT,
        JALR,
        ERROR,          // Opcode not recognized                
        ZE,
    };

    // Register between the Instruction Fetch and Instruction Decode
    struct IFID {
        uint32_t PC = 0;
        uint32_t instr = 0;
    } ifid;

    // Register between the Instruction Decode and Execute
    struct IDEX {
        uint32_t PC = 0;
        int32_t rs1Data = 0;
        int32_t rs2Data = 0;
        uint32_t rd = 0;
        int32_t imm = 0;
        Op op = Op::ZE;
    } idex;

    // Register between the Execute and Memory
    struct EXMEM {
        uint32_t PC = 0;
        int32_t aluResult = 0;
        int32_t rs2Data = 0;
        uint32_t rd = 0;
        Op op = Op::ZE;
    } exmem;

    // Register between the Memory and Writeback
    struct MEMWB {
        uint32_t rd = 0;
        int32_t aluResult = 0;
        int32_t memData = 0;
        Op op = Op::ZE;
    } memwb;
};


#endif //CPU_INCLUDED
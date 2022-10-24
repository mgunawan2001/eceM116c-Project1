#include "CPU.h"

using namespace std;

CPU::CPU(vector<uint8_t> &&iMem) : instrMem(move(iMem)){
    
    // Initialize register with 0
    for(int i =0; i<32; i++)
    {
        reg[i] = 0;
    }
    
    // Initialize data memory
    vector<uint8_t> dMem(4096, 0x0);
    dataMem = dMem;  
    
    // Initialize counters 
    PC = 0;
    killCounter = 0;
    rTypeCount = 0;
    iTypeCount = 0;
    bTypeCount = 0;
    jalrCount = 0;
    swCount = 0;
    lwCount = 0;
    totalInstrCount = 0;
    ipc = 0;
}

void CPU::Fetch() {
    // Fetch one instruction from the program memory in little endian form.
    if (PC + 3 < instrMem.size()) {
        uint32_t byte1 = instrMem[PC];
        uint32_t byte2 = instrMem[PC + 1];
        uint32_t byte3 = instrMem[PC + 2];
        uint32_t byte4 = instrMem[PC + 3];

        ifid.instr = (byte4 << 24) + (byte3 << 16) + (byte2 << 8) + byte1;
    }
    else {
        ifid.instr = ZERO;
    }
    ifid.PC = PC;

    // Track NOPS to know when to end
    if (ZERO == (ifid.instr & 0x7fff)) { killCounter++; }
    else { killCounter = 0; }


    //bitset<32> x(ifid.instr);
    //cout << "start of clock tick " << clockCount << ":" << endl;
    //cout << "running instruction: " << x << endl;
}

void CPU::Decode() {
    // Parse binary instruction
    uint32_t opcode = ifid.instr & 0x7f;
    uint32_t funct3 = (ifid.instr >> 12) & 0x7;
    uint32_t funct7 = (ifid.instr >> 25) & 0x7f;
    uint32_t rs1 = (ifid.instr >> 15) & 0x1f;
    uint32_t rs2 = (ifid.instr >> 20) & 0x1f;

    //cout << "opcode: " << opcode << endl;
    //cout << "rs1:" << rs1 << " rs2: " << rs2 << ".";
    // Pass required info to register between Instruction Decode and execute
    idex.PC = ifid.PC;
    idex.rs1Data = reg[rs1];
    idex.rs2Data = reg[rs2];
    idex.rd = (ifid.instr >> 7) & 0x1f;
    idex.imm = (int32_t) ifid.instr >> 20;  // Performs sign extension
    
    // RTYPE op
    if (opcode == RTYPE) {
        rTypeCount++;
        if (funct3 == 0x0) {
            if (funct7 == 0x0) { 
                idex.op = Op::ADD; 
                //cout << "add!" << endl;
            }
            else if (funct7 == 0x20) { 
                idex.op = Op::SUB; 
                //cout << "sub!" << endl;
            }
            else { idex.op = Op::ERROR; }  
        }
        else if (funct3 == 0x4) { 
            idex.op = Op::XOR; 
            //cout << "xor!" << endl;
        }
        else if (funct3 == 0x5) { 
            idex.op = Op::SRA; 
            //cout << "sra!" << endl;
        }
        else { idex.op = Op::ERROR; }  
    }

    // ITYPE op
    else if (opcode == ITYPE) 
    {
        iTypeCount++;
        if (funct3 == 0x0) { 
            idex.op = Op::ADDI; 
            //cout << "addi!" << endl;
        }
        else if (funct3 == 0x7) { 
            idex.op = Op::ANDI; 
            //cout << "andi!" << endl;
        }
        else { idex.op = Op::ERROR; }  
    }

    // BTYPE op
    else if (opcode == BTYPE) 
    {
        bTypeCount++;

        //imm
        auto imm11_5 = (int32_t)(ifid.instr & 0xfe000000);
        auto imm4_0 = (int32_t)((ifid.instr & 0xf80) << 13); 
        idex.imm = (imm11_5 + imm4_0) >> 20; 

        if (funct3 == 0x4) { idex.op = Op::BLT; }
        else { idex.op = Op::ERROR; } 

        //cout << "branch!" << endl;
    }

    // JALR op
    else if (opcode == JTYPE) 
    {
        jalrCount++;
        idex.op = Op::JALR;

        //cout << "jalr!" << endl;
    }

    // LW op
    else if (opcode == LOADWORD && funct3 == 0x2) 
    {
        lwCount++;
        idex.op = Op::LW; 

        //cout << "load!" << endl;
    }

    // SW op
    else if (opcode == STOREWORD && funct3 == 0x2) 
    {
        swCount++;

        //imm
        auto imm11_5 = (int32_t) (ifid.instr & 0xfe000000);
        auto imm4_0 = (int32_t) ((ifid.instr & 0xf80) << 13);
        idex.imm = (imm11_5 + imm4_0) >> 20; // Redefined for SW
        
        idex.op = Op::SW;

        //cout << "store!" << endl;
    }

    // ZERO opcode
    else if (ZERO == opcode) { idex.op = Op::ZE; }

    // ERROR opcode (unrecognized instruction)
    else { idex.op = Op::ERROR; }  
}

void CPU::Execute() {
    // Pass required info to register between Execute and Memory
    exmem.PC = idex.PC;
    exmem.op = idex.op;
    exmem.rs2Data = idex.rs2Data;
    exmem.rd = idex.rd;

    switch(idex.op) {
        case Op::ADD:
            exmem.aluResult = idex.rs1Data + idex.rs2Data;
            //cout << "add: " << idex.rs1Data << " + " << idex.rs2Data << " = " << exmem.aluResult<<endl;
            break;

        case Op::SUB:
            exmem.aluResult = idex.rs1Data - idex.rs2Data;
            //cout << "sub: " << idex.rs1Data << " - " << idex.rs2Data << " = " << exmem.aluResult<<endl;
            break;

        case Op::ADDI:
            exmem.aluResult = idex.rs1Data + idex.imm;
            //cout << "addi: " << idex.rs1Data << "+" << idex.imm << "=" << exmem.aluResult << endl;
            break;

        case Op::ANDI:
            exmem.aluResult = idex.rs1Data & idex.imm;
            //cout << "andi: " << idex.rs1Data << "&" << idex.imm << "=" << exmem.aluResult << endl;
            break;
        
        case Op::XOR:
            exmem.aluResult = ~(idex.rs1Data & idex.rs2Data) & ~(~idex.rs1Data & ~idex.rs2Data);
            //cout << "xor: " << idex.rs1Data << " xor " << idex.imm << "=" << exmem.aluResult << endl;
            break;

        case Op::SRA:
            exmem.aluResult = idex.rs1Data < 0 ? ~(~idex.rs1Data >> idex.rs2Data) : idex.rs1Data >> idex.rs2Data;
            //cout << "sra: " << idex.rs1Data << " sra " << idex.imm << "=" << exmem.aluResult << endl;

            break;

        case Op::LW:
        case Op::SW:
            exmem.aluResult = idex.rs1Data + idex.imm;
            break;

        case Op::ZE:
            exmem.aluResult = ZERO;
            break;

        case Op::ERROR:
            break;
    }
}

void CPU::Memory() {
    // Pass required info to register between Memory and Writeback
    memwb.rd = exmem.rd;
    memwb.aluResult = exmem.aluResult;
    memwb.op = exmem.op;


    int32_t lwByte1, lwByte2, lwByte3, lwByte4;
    uint8_t swByte1, swByte2, swByte3, swByte4;

    if(exmem.op == Op::LW){
        // Get data from memory
        lwByte1 = dataMem[exmem.aluResult];
        lwByte2 = dataMem[exmem.aluResult + 1];
        lwByte3 = dataMem[exmem.aluResult + 2];
        lwByte4 = dataMem[exmem.aluResult + 3];

        // Load into register
        memwb.memData = (lwByte4 << 24) + (lwByte3 << 16) + (lwByte2 << 8) + lwByte1;
    }
    else if (exmem.op == Op::SW) {
        // Get data from register
        swByte1 = (exmem.rs2Data >> 24) & 0xff000000;
        swByte2 = (exmem.rs2Data >> 16) & 0xff0000;
        swByte3 = (exmem.rs2Data >> 8) & 0xff00;
        swByte4 = exmem.rs2Data & 0xff;

        // Store into memory
        dataMem[exmem.aluResult] = swByte4;
        //cout << "dataMem[" << exmem.aluResult << "]: " << int(dataMem[exmem.aluResult]) << " ";
        dataMem[exmem.aluResult + 1] = swByte3;
        //cout << "dataMem[" << exmem.aluResult+1 << "]: " << int(dataMem[exmem.aluResult+1]) << " ";
        dataMem[exmem.aluResult + 2] = swByte2;
        //cout << "dataMem[" << exmem.aluResult+2 << "]: " << int(dataMem[exmem.aluResult+2]) << " ";
        dataMem[exmem.aluResult + 3] = swByte1; 
        //cout << "dataMem[" << exmem.aluResult+3 << "]: " << int(swByte1) << " ";

        //cout << endl;
    }
}

void CPU::Writeback() {

    // Nothing written to register for SW
    // Check to make sure we don't override reg[0]
    if (memwb.rd == ZERO || memwb.op == Op::SW) {
        return;
    }  

    if (memwb.op == Op::LW) {
        // Place data from meory into register
        reg[memwb.rd] = memwb.memData; 
    }
    
    else if (memwb.op != Op::ERROR) { 
        // Place ALU results into register
        reg[memwb.rd] = memwb.aluResult; 
    }
}


void CPU::clockTick() {
    //cout << "end of clocktick: " << clockCount << endl;
    clockCount++;

    // Branch to correct PC as necessary
    if (Op::BLT == memwb.op) {

        //cout << "oldpc: " << PC << ".";
        if (idex.rs1Data < idex.rs2Data) {
            PC += idex.imm;
        }
        else { 
            PC += 4; 
        }
        //cout << "newpc: " << PC << ".";
    }

    // Jump to correct PC
    if (memwb.op == Op::JALR) {
        reg[memwb.rd] = PC+4;
        PC = idex.rs1Data + idex.imm;
    }

    // Go to next instruction (PC+4)
    else if ((memwb.op != Op::BLT) && (memwb.op != Op::JALR)) {
        PC += 4;
    }

    //cout << "r1: " << reg[1] << " r2: " << reg[2] << " r3: " << reg[3] << " r4: " << reg[4] << " r5: " << reg[5] << " r6: " << reg[6] << " r7: " << reg[7] << " r8: " << reg[8] << " r9: " << reg[9] << " r10: " << reg[10] << " r11: " << reg[11] << endl;

    /*for (int i = 0; i < dataMem.size(); i++) { cout << "dataMem[" << i << "]: " << dataMem[i] << " "; }
    cout << endl;*/

    
    //cout << endl;

    return;
    
}

bool CPU::isFinished() {
    // All five stages have ZERO op codes
    if (5 == killCounter) {
        // Total all instruction types and calculate the IPC
        totalInstrCount = rTypeCount + iTypeCount + bTypeCount + jalrCount + lwCount + swCount;
        ipc = (double) totalInstrCount / clockCount;

        return true;
    }
    return false;
}

void CPU::printInfo() {
    cout << "(" << reg[10] << ", " << reg[11] << ")" << endl;
    cerr << "Total number of clock cycles: " << clockCount << endl;
    cerr << "Total r-type instructions run: " << rTypeCount << endl;
    cerr << "IPC: " << ipc << endl;
}

void CPU::printResult() { cout << "(" << reg[10] << "," << reg[11] << ")"<<endl; }

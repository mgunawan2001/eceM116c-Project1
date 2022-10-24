#include "CPU.h"

using namespace std;

CPU::CPU(vector<uint8_t> &&iMem, vector<uint8_t> &&dMem) : insMem(move(iMem)), dataMem(move(dMem)) {
    fill(registerFile, registerFile + sizeof(registerFile), 0);
    pc = 0;
    killCounter = 0;
    rTypeCount = 0;
    iTypeCount = 0;
    swCount = 0;
    lwCount = 0;
    totalInsCount = 0;
    ipc = 0;
}

unsigned int CPU::getClockCount() { return clockCount; }

void CPU::fetch() {
    // Fetch one instruction from the program memory in little endian form.
    if (pc + 3 < insMem.size()) {
        uint32_t byte1 = insMem[pc];
        uint32_t byte2 = insMem[pc + 1];
        uint32_t byte3 = insMem[pc + 2];
        uint32_t byte4 = insMem[pc + 3];

        ifidCurr.instruction = (byte4 << 24) + (byte3 << 16) + (byte2 << 8) + byte1;
    }
    else {
        ifidCurr.instruction = ZERO;
    }
    ifidCurr.pc = pc;

    // Ensures we can track 5 sequential NOPs
    if (ZERO == (ifidCurr.instruction & 0x7fff)) { ++killCounter; }
    else { killCounter = 0; }


    //bitset<32> x(ifidCurr.instruction);
    cout << "start of clock tick " << clockCount << ":" << endl;
}

void CPU::decode() {
    // Extract and examine the instruction components
    uint32_t opcode = ifidCurr.instruction & 0x7f;
    uint32_t func3 = (ifidCurr.instruction >> 12) & 0x7;
    uint32_t func7 = (ifidCurr.instruction >> 25) & 0x7f;
    uint32_t rs1 = (ifidCurr.instruction >> 15) & 0x1f;
    uint32_t rs2 = (ifidCurr.instruction >> 20) & 0x1f;

    // Update the IDEX struct
    idexCurr.pc = ifidCurr.pc;
    idexCurr.readData1 = registerFile[rs1];
    idexCurr.readData2 = registerFile[rs2];
    idexCurr.rd = (ifidCurr.instruction >> 7) & 0x1f;
    idexCurr.immediate = (int32_t) ifidCurr.instruction >> 20;  // Performs sign extension

    // RTYPE operation
    if (RTYPE == opcode) {
        ++rTypeCount;
        if (0x0 == func3) {
            if (0x0 == func7) { idexCurr.operation = Op::ADD; }
            else if (0x20 == func7) { idexCurr.operation = Op::SUB; }
            else { idexCurr.operation = Op::ERROR; }  // Unrecognized opcode
        }
        else if (0x4 == func3) { idexCurr.operation = Op::XOR; }
        else if (0x5 == func3) { idexCurr.operation = Op::SRA; }
        else if (0x6 == func3) { idexCurr.operation = Op::OR; }
        else if (0x7 == func3) { idexCurr.operation = Op::AND; }
        else { idexCurr.operation = Op::ERROR; }  // Unrecognized opcode
    }
    // ITYPE operation
    else if (ITYPE == opcode) {
        ++iTypeCount;
        if (0x0 == func3) { idexCurr.operation = Op::ADDI;}
        else if (0x6 == func3) { idexCurr.operation = Op::ORI; }
        else if (0x7 == func3) { idexCurr.operation = Op::ANDI; }
        else { idexCurr.operation = Op::ERROR; }  // Unrecognized opcode
    }
    // BTYPE operation
    else if (opcode == BTYPE) {
        ++bTypeCount;
        //immediate
        auto imm11_5 = (int32_t)(ifidCurr.instruction & 0xfe000000);
        auto imm4_0 = (int32_t)((ifidCurr.instruction & 0xf80) << 13); 
        idexCurr.immediate = (imm11_5 + imm4_0) >> 20; // Redefined for SW

        if (0x4 == func3) { idexCurr.operation = Op::BLT; }
        else { idexCurr.operation = Op::ERROR; }  // Unrecognized opcode
    }
    // JTYPE operation
    else if (opcode == JTYPE) {
        cout << "jalr! " << endl;
        ++jTypeCount;
        idexCurr.operation = Op::JALR;
    }
    // LW operation
    else if (LOADWORD == opcode && 0x2 == func3) {
        ++lwCount;
        idexCurr.operation = Op::LW; }
    // SW operation
    else if (STOREWORD == opcode && 0x2 == func3) {
        cout << "storeword! " << endl;
        ++swCount;
        auto imm11_5 = (int32_t) (ifidCurr.instruction & 0xfe000000);
        auto imm4_0 = (int32_t) ((ifidCurr.instruction & 0xf80) << 13);
        idexCurr.immediate = (imm11_5 + imm4_0) >> 20; // Redefined for SW
        idexCurr.operation = Op::SW;
    }
    // ZERO op code
    else if (ZERO == opcode) { idexCurr.operation = Op::ZE; }
    else { idexCurr.operation = Op::ERROR; }  // Unrecognized opcode
}

void CPU::execute() {
    // Update the EXMEM struct
    exmemCurr.pc = idexCurr.pc;
    exmemCurr.operation = idexCurr.operation;
    exmemCurr.readData2 = idexCurr.readData2;
    exmemCurr.rd = idexCurr.rd;

    switch(idexCurr.operation) {
        case Op::ADD:
            exmemCurr.aluResult = idexCurr.readData1 + idexCurr.readData2;
            cout << "add: " << idexCurr.readData1 << " + " << idexCurr.readData2 << " = " << exmemCurr.aluResult<<endl;
            break;

        case Op::SUB:
            exmemCurr.aluResult = idexCurr.readData1 - idexCurr.readData2;
            cout << "sub: " << idexCurr.readData1 << " - " << idexCurr.readData2 << " = " << exmemCurr.aluResult<<endl;
            break;

        case Op::OR:
            exmemCurr.aluResult = idexCurr.readData1 | idexCurr.readData2;
            break;

        case Op::AND:
            exmemCurr.aluResult = idexCurr.readData1 & idexCurr.readData2;
            break;

        case Op::ADDI:
            exmemCurr.aluResult = idexCurr.readData1 + idexCurr.immediate;
            cout << "addi: " << idexCurr.readData1 << "+" << idexCurr.immediate << "=" << exmemCurr.aluResult << endl;
            break;

        case Op::ORI:
            exmemCurr.aluResult = idexCurr.readData1 | idexCurr.immediate;
            break;

        case Op::ANDI:
            exmemCurr.aluResult = idexCurr.readData1 & idexCurr.immediate;
            break;
        
        case Op::XOR:
            exmemCurr.aluResult = ~(idexCurr.readData1 & idexCurr.readData2) & ~(~idexCurr.readData1 & ~idexCurr.readData2);
            cout << "xor: " << idexCurr.readData1 << " xor " << idexCurr.immediate << "=" << exmemCurr.aluResult << endl;
            break;

        case Op::SRA:
            exmemCurr.aluResult = idexCurr.readData1 < 0 ? ~(~idexCurr.readData1 >> idexCurr.readData2) : idexCurr.readData1 >> idexCurr.readData2;
            cout << "sra: " << idexCurr.readData1 << " sra " << idexCurr.immediate << "=" << exmemCurr.aluResult << endl;

            break;

        case Op::LW:  // Does the same as SW for the ALU result
        case Op::SW:
            exmemCurr.aluResult = idexCurr.readData1 + idexCurr.immediate;
            break;

        case Op::ZE:
            exmemCurr.aluResult = ZERO;
            break;

        case Op::ERROR:
            break;
    }
}

void CPU::memory() {
    // Update the MEMWB struct
    memwbCurr.rd = exmemCurr.rd;
    memwbCurr.aluResult = exmemCurr.aluResult;
    memwbCurr.operation = exmemCurr.operation;


    int32_t lByte1, lByte2, lByte3, lByte4;
    uint8_t sByte1, sByte2, sByte3, sByte4;
    switch(exmemCurr.operation) {
        case Op::LW:
            // Fetch 4 bytes from the data memory in little endian form.
            lByte1 = dataMem[exmemCurr.aluResult];
            lByte2 = dataMem[exmemCurr.aluResult + 1];
            lByte3 = dataMem[exmemCurr.aluResult + 2];
            lByte4 = dataMem[exmemCurr.aluResult + 3];

            // Convert to big endian and store as the aluResult
            memwbCurr.memData = (lByte4 << 24) + (lByte3 << 16) + (lByte2 << 8) + lByte1;
            break;

        case Op::SW:
            // Separate bytes of readData2
            sByte1 = (exmemCurr.readData2 >> 24) & 0xff000000;
            sByte2 = (exmemCurr.readData2 >> 16) & 0xff0000;
            sByte3 = (exmemCurr.readData2 >> 8) & 0xff00;
            sByte4 = exmemCurr.readData2 & 0xff;

            // Store in dataMem in little endian form
            dataMem[exmemCurr.aluResult] = sByte4;
            dataMem[exmemCurr.aluResult + 1] = sByte3;
            dataMem[exmemCurr.aluResult + 1] = sByte2;
            dataMem[exmemCurr.aluResult + 1] = sByte1;
            break;

        default:
            break;
    }
}

void CPU::writeback() {
    if (ZERO == memwbCurr.rd || Op::SW == memwbCurr.operation) {
        return;
    }  // Don't overwrite x0 ever! Also, SW doesn't write to a register.
    if (Op::LW == memwbCurr.operation) {
        registerFile[memwbCurr.rd] = memwbCurr.memData; }
    
    else if (Op::ERROR != memwbCurr.operation) { registerFile[memwbCurr.rd] = memwbCurr.aluResult; }
    }


void CPU::clockTick() {
    ++clockCount;
    //cout << "pc: " << pc << endl;

    if (Op::BLT == memwbCurr.operation) {
        cout << idexCurr.readData1 << " <= " << idexCurr.readData2 << " imm: " << idexCurr.immediate << endl;
        if (idexCurr.readData1 <= idexCurr.readData2) {
            pc += idexCurr.immediate;
            cout << " immediate: " << idexCurr.immediate << " new pc: " << pc << endl;
        }
        else { pc += 4; }
    }
    if (memwbCurr.operation == Op::JALR) {
        cout << "jalr rd: " << idexCurr.rd << endl;
        registerFile[memwbCurr.rd] = pc+4;
        pc = idexCurr.readData1 + idexCurr.immediate;
    }
    else if ((memwbCurr.operation != Op::BLT) && (memwbCurr.operation != Op::JALR)) {
        pc += 4;  // No branching or jumps in this version
    }
    cout << "r1: " << registerFile[1] << " r2: " << registerFile[2] << " r3: " << registerFile[3] << " r4: " << registerFile[4] << " r5: " << registerFile[5] << " r6: " << registerFile[6] << " r7: " << registerFile[7] << " r8: " << registerFile[8] << " r9: " << registerFile[9] << " r10: " << registerFile[10] << " r11: " << registerFile[11] << endl;

    
    cout << "end of clocktick: " << clockCount << endl;
    cout << endl;

   
    /*ifidCurr = ifidNext;
    idexCurr = idexNext;
    exmemCurr = exmemNext;
    memwbCurr = memwbNext;*/
    return;
    
}

bool CPU::isFinished() {
    // All five stages have ZERO op codes
    if (5 == killCounter) {
        // Total all instruction types and calculate the IPC
        totalInsCount = rTypeCount + iTypeCount + lwCount + swCount;
        ipc = (double) totalInsCount / clockCount;

        return true;
    }
    return false;
}

void CPU::printStats() {
    cout << "(" << registerFile[10] << ", " << registerFile[11] << ")" << endl;
    cerr << "Total number of clock cycles: " << clockCount << endl;
    cerr << "Total r-type instructions run: " << rTypeCount << endl;
    cerr << "IPC: " << ipc << endl;
}

#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <fstream>

using namespace std;

#define MemSize 1000 // memory size, in reality, the memory size should be 2^32, but for this lab, for the space resaon, we keep it as this large number, but the memory is still 32-bit addressable.

struct IFStruct {
    bitset<32>  PC;
    bool        nop;  
};

struct IDStruct {
    bitset<32>  Instr;
    bool        nop;  
};

struct EXStruct {
    bitset<32>  Read_data1;
    bitset<32>  Read_data2;
    bitset<16>  Imm;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bool        is_I_type;
    bool        rd_mem;
    bool        wrt_mem; 
    bool        alu_op;     //1 for addu, lw, sw, 0 for subu 
    bool        wrt_enable;
    bool        nop;  
};

struct MEMStruct {
    bitset<32>  ALUresult;
    bitset<32>  Store_data;
    bitset<5>   Rs;
    bitset<5>   Rt;    
    bitset<5>   Wrt_reg_addr;
    bool        rd_mem;
    bool        wrt_mem; 
    bool        wrt_enable;    
    bool        nop;    
};

struct WBStruct {
    bitset<32>  Wrt_data;
    bitset<5>   Rs;
    bitset<5>   Rt;     
    bitset<5>   Wrt_reg_addr;
    bool        wrt_enable;
    bool        nop;     
};

struct stateStruct {
    IFStruct    IF;
    IDStruct    ID;
    EXStruct    EX;
    MEMStruct   MEM;
    WBStruct    WB;
};

class InsMem
{
public:
    string id, ioDir;
    InsMem(string name, string ioDir) {       
        id = name;
        IMem.resize(MemSize);
        ifstream imem;
        string line;
        int i = 0;
        imem.open(ioDir + "\\imem.txt");
        if (imem.is_open())
        {
            while (getline(imem, line))
            {      
                IMem[i] = bitset<8>(line);
                i++;
            }                    
        }
        else cout << "Unable to open IMEM input file.";
        imem.close();                     
    }

    bitset<32> readInstr(bitset<32> ReadAddress) {
        // read instruction memory
        string instruction;
        instruction.append(IMem[ReadAddress.to_ulong()].to_string());
        instruction.append(IMem[ReadAddress.to_ulong() + 1].to_string());
        instruction.append(IMem[ReadAddress.to_ulong() + 2].to_string());
        instruction.append(IMem[ReadAddress.to_ulong() + 3].to_string());
        return bitset<32>(instruction);       
    }     
      
private:
    vector<bitset<8> > IMem;     
};
      
class DataMem    
{
public: 
    string id, opFilePath, ioDir;
    DataMem(string name, string ioDir) : id(name), ioDir(ioDir) {
        DMem.resize(MemSize);
        opFilePath = ioDir + "\\" + name + "_DMEMResult.txt";
        ifstream dmem;
        string line;
        int i = 0;
        dmem.open(ioDir + "\\dmem.txt");
        if (dmem.is_open())
        {
            while (getline(dmem, line))
            {      
                DMem[i] = bitset<8>(line);
                i++;
            }
        }
        else cout << "Unable to open DMEM input file.";
        dmem.close();          
    }
    
    bitset<32> readDataMem(bitset<32> Address) {
        // read data memory
        string data;
        data.append(DMem[Address.to_ulong()].to_string());
        data.append(DMem[Address.to_ulong() + 1].to_string());
        data.append(DMem[Address.to_ulong() + 2].to_string());
        data.append(DMem[Address.to_ulong() + 3].to_string());
        return bitset<32>(data);        
    }
            
    void writeDataMem(bitset<32> Address, bitset<32> WriteData) {
        // write into memory
        DMem[Address.to_ulong()] = bitset<8>(WriteData.to_string().substr(0, 8));
        DMem[Address.to_ulong() + 1] = bitset<8>(WriteData.to_string().substr(8, 8));
        DMem[Address.to_ulong() + 2] = bitset<8>(WriteData.to_string().substr(16, 8));
        DMem[Address.to_ulong() + 3] = bitset<8>(WriteData.to_string().substr(24, 8));
    }   
                     
    void outputDataMem() {
        ofstream dmemout;
        dmemout.open(opFilePath, std::ios_base::trunc);
        if (dmemout.is_open()) {
            for (int j = 0; j < 1000; j++)
            {     
                dmemout << DMem[j] << endl;
            }
                     
        }
        else cout << "Unable to open " << id << " DMEM result file." << endl;
        dmemout.close();
    }             

private:
    vector<bitset<8> > DMem;      
};

class RegisterFile
{
public:
    string outputFile;
    RegisterFile(string ioDir): outputFile(ioDir + "RFResult.txt") {
        Registers.resize(32);  
        Registers[0] = bitset<32>(0);  
    }
    
    bitset<32> readRF(bitset<5> Reg_addr) {
        // read register data
        return Registers[Reg_addr.to_ulong()];
    }
    
    void writeRF(bitset<5> Reg_addr, bitset<32> Wrt_reg_data) {
        // write register data
        if (Reg_addr != bitset<5>(0)) {
            Registers[Reg_addr.to_ulong()] = Wrt_reg_data;
        }
    }
     
    void outputRF(int cycle) {
        ofstream rfout;
        if (cycle == 0)
            rfout.open(outputFile, std::ios_base::trunc);
        else 
            rfout.open(outputFile, std::ios_base::app);
        if (rfout.is_open())
        {
            rfout << "State of RF after executing cycle:\t" << cycle << endl;
            for (int j = 0; j < 32; j++)
            {
                rfout << Registers[j] << endl;
            }
        }
        else cout << "Unable to open RF output file." << endl;
        rfout.close();               
    } 
            
private:
    vector<bitset<32> > Registers;
};

class Core {
public:
    RegisterFile myRF;
    uint32_t cycle;
    bool halted;
    string ioDir;
    struct stateStruct state, nextState;
    InsMem ext_imem;
    DataMem ext_dmem;
    uint32_t totalCycles;
    uint32_t totalInstructions;
    
    Core(string ioDir, InsMem &imem, DataMem &dmem): myRF(ioDir), ioDir(ioDir), ext_imem(imem), ext_dmem(dmem), totalCycles(0), totalInstructions(0) {}

    virtual void step() {}

    virtual void printState() {}

    void printPerformanceMetrics(string type) {
        double cpi = static_cast<double>(totalCycles) / totalInstructions;
        double ipc = static_cast<double>(totalInstructions) / totalCycles;

        std::ofstream metricsFile(ioDir + "PerformanceMetrics_" + type + ".txt");
        if (metricsFile.is_open()) {
            metricsFile << "Performance Metrics:" << std::endl;
            metricsFile << "Total Cycles: " << totalCycles << std::endl;
            metricsFile << "Total Instructions: " << totalInstructions << std::endl;
            metricsFile << "CPI: " << cpi << std::endl;
            metricsFile << "IPC: " << ipc << std::endl;
            metricsFile.close();
        }
    }
};

class SingleStageCore : public Core {
public:
    SingleStageCore(string ioDir, InsMem &imem, DataMem &dmem): Core(ioDir + "\\SS_", imem, dmem), opFilePath(ioDir + "\\StateResult_SS.txt") {}

    void step() {
        /* Fetch Instruction */
        bitset<32> instruction = ext_imem.readInstr(state.IF.PC);

		/* Decode Instruction */
		bitset<7> opcode = bitset<7>(instruction.to_ulong() & 0x7F);
		bitset<5> rs1 = bitset<5>((instruction >> 15).to_ulong() & 0x1F);
		bitset<5> rs2 = bitset<5>((instruction >> 20).to_ulong() & 0x1F);
		bitset<5> rd = bitset<5>((instruction >> 7).to_ulong() & 0x1F);
		bitset<12> imm = bitset<12>((instruction >> 20).to_ulong() & 0xFFF);

        /* Read Register File */
        bitset<32> operand1 = myRF.readRF(rs1);
        bitset<32> operand2 = myRF.readRF(rs2);

        /* Execute Instruction */
        bitset<32> result;
        bool writeback = false;
        bool isNOP = (instruction == 0xFFFFFFFF);

        if (!isNOP) {
            if (opcode == 0x33) { // ADD
                result = operand1.to_ulong() + operand2.to_ulong();
                writeback = true;
            } else if (opcode == 0x03) { // LW
                result = ext_dmem.readDataMem(operand1.to_ulong() + imm.to_ulong());
                writeback = true;
            } else if (opcode == 0x23) { // SW
                ext_dmem.writeDataMem(operand1.to_ulong() + imm.to_ulong(), operand2);
            } else if (opcode == 0x63) { // BEQ
                if (operand1 == operand2) {
                    nextState.IF.PC = state.IF.PC.to_ulong() + imm.to_ulong();
                } else {
                    nextState.IF.PC = state.IF.PC.to_ulong() + 4;
                }
            } else if (opcode == 0x6F) { // JAL
                nextState.IF.PC = state.IF.PC.to_ulong() + imm.to_ulong();
                result = state.IF.PC.to_ulong() + 4;
                writeback = true;
            } else {
                // Unsupported instruction
                isNOP = true;
            }

            totalInstructions++;
        }

        /* Writeback */
        if (writeback) {
            myRF.writeRF(rd, result);
        }

        nextState.IF.nop = isNOP;

        if (isNOP) {
            nextState.IF.PC = state.IF.PC.to_ulong() + 4;
        }

        /* Update State */
        state = nextState;

        /* Update Cycle Count */
        totalCycles++;

        if (state.IF.nop && state.IF.PC == 0) {
            halted = true;
        }

        myRF.outputRF(cycle);
        printState(nextState, cycle);

        cycle++;
    }

    void printState(stateStruct state, int cycle) {
        ofstream printstate;
        if (cycle == 0)
            printstate.open(opFilePath, std::ios_base::trunc);
        else
            printstate.open(opFilePath, std::ios_base::app);
        if (printstate.is_open()) {
            printstate << "State after executing cycle:\t" << cycle << endl;

            printstate << "IF.PC:\t" << state.IF.PC.to_ulong() << endl;
            printstate << "IF.nop:\t" << state.IF.nop << endl;
        } else {
            cout << "Unable to open SS StateResult output file." << endl;
        }
        printstate.close();
    }

private:
    string opFilePath;
};

class FiveStageCore : public Core {
public:
    FiveStageCore(string ioDir, InsMem &imem, DataMem &dmem): Core(ioDir + "\\FS_", imem, dmem), opFilePath(ioDir + "\\StateResult_FS.txt") {}

    void step() {
        /* --------------------- WB stage --------------------- */
        if (state.WB.wrt_enable) {
            myRF.writeRF(state.WB.Wrt_reg_addr, state.WB.Wrt_data);
        }

        /* --------------------- MEM stage -------------------- */
        if (state.MEM.rd_mem) {
            nextState.WB.Wrt_data = ext_dmem.readDataMem(state.MEM.ALUresult);
        } else if (state.MEM.wrt_mem) {
            ext_dmem.writeDataMem(state.MEM.ALUresult, state.MEM.Store_data);
        } else {
            nextState.WB.Wrt_data = state.MEM.ALUresult;
        }
        nextState.WB.Rs = state.MEM.Rs;
        nextState.WB.Rt = state.MEM.Rt;
        nextState.WB.Wrt_reg_addr = state.MEM.Wrt_reg_addr;
        nextState.WB.wrt_enable = state.MEM.wrt_enable;
        nextState.WB.nop = state.MEM.nop;

        /* --------------------- EX stage --------------------- */
        bool isNOP = state.EX.nop;
        if (!isNOP) {
            bitset<32> operand1 = myRF.readRF(state.EX.Rs);
            bitset<32> operand2 = state.EX.is_I_type ? bitset<32>(state.EX.Imm.to_ulong()) : myRF.readRF(state.EX.Rt);

            if (state.EX.alu_op) { // ADD
                nextState.MEM.ALUresult = operand1.to_ulong() + operand2.to_ulong();
            } else { // SUB
                nextState.MEM.ALUresult = operand1.to_ulong() - operand2.to_ulong();
            }

            nextState.MEM.Store_data = myRF.readRF(state.EX.Rt);
			nextState.MEM.Rs = state.EX.Rs;
            nextState.MEM.Rt = state.EX.Rt;
            nextState.MEM.Wrt_reg_addr = state.EX.Wrt_reg_addr;
            nextState.MEM.rd_mem = state.EX.rd_mem;
            nextState.MEM.wrt_mem = state.EX.wrt_mem;
            nextState.MEM.wrt_enable = state.EX.wrt_enable;
            nextState.MEM.nop = isNOP;

            totalInstructions++;
        } else {
            nextState.MEM.nop = true;
        }

		/* --------------------- ID stage --------------------- */
		bitset<32> instruction = state.ID.Instr;
		bitset<7> opcode = bitset<7>(instruction.to_ulong() & 0x7F);
		bitset<5> rs1 = bitset<5>((instruction >> 15).to_ulong() & 0x1F);
		bitset<5> rs2 = bitset<5>((instruction >> 20).to_ulong() & 0x1F);
		bitset<5> rd = bitset<5>((instruction >> 7).to_ulong() & 0x1F);
		bitset<12> imm = bitset<12>((instruction >> 20).to_ulong() & 0xFFF);

		isNOP = state.ID.nop;
		if (!isNOP) {
			nextState.EX.Rs = rs1;
			nextState.EX.Rt = rs2;
			nextState.EX.Imm = bitset<16>(imm.to_ulong());
            nextState.EX.Wrt_reg_addr = rd;
            nextState.EX.is_I_type = (opcode == 0x03 || opcode == 0x13 || opcode == 0x67);
            nextState.EX.rd_mem = (opcode == 0x03);
            nextState.EX.wrt_mem = (opcode == 0x23);
            nextState.EX.alu_op = (opcode == 0x33);
            nextState.EX.wrt_enable = (opcode == 0x33 || opcode == 0x03 || opcode == 0x13 || opcode == 0x67);
            nextState.EX.nop = isNOP;
        } else {
            nextState.EX.nop = true;
        }

        /* --------------------- IF stage --------------------- */
        isNOP = state.IF.nop;
        if (!isNOP) {
            nextState.ID.Instr = ext_imem.readInstr(state.IF.PC);
            nextState.IF.PC = state.IF.PC.to_ulong() + 4;
            nextState.IF.nop = false;
            nextState.ID.nop = false;
        } else {
            nextState.ID.nop = true;
            nextState.IF.PC = state.IF.PC;
            nextState.IF.nop = state.IF.nop;
        }

        if (state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop && state.WB.nop) {
            halted = true;
        }

        /* Update State */
        state = nextState;

        /* Update Cycle Count */
        totalCycles++;

        myRF.outputRF(cycle);
        printState(nextState, cycle);

        cycle++;
    }

    void printState(stateStruct state, int cycle) {
        ofstream printstate;
        if (cycle == 0)
            printstate.open(opFilePath, std::ios_base::trunc);
        else
            printstate.open(opFilePath, std::ios_base::app);
        if (printstate.is_open()) {
            printstate << "State after executing cycle:\t" << cycle << endl;

            printstate << "IF.PC:\t" << state.IF.PC.to_ulong() << endl;
            printstate << "IF.nop:\t" << state.IF.nop << endl;

            printstate << "ID.Instr:\t" << state.ID.Instr << endl;
            printstate << "ID.nop:\t" << state.ID.nop << endl;

            printstate << "EX.Read_data1:\t" << state.EX.Read_data1 << endl;
            printstate << "EX.Read_data2:\t" << state.EX.Read_data2 << endl;
            printstate << "EX.Imm:\t" << state.EX.Imm << endl;
            printstate << "EX.Rs:\t" << state.EX.Rs << endl;
            printstate << "EX.Rt:\t" << state.EX.Rt << endl;
            printstate << "EX.Wrt_reg_addr:\t" << state.EX.Wrt_reg_addr << endl;
            printstate << "EX.is_I_type:\t" << state.EX.is_I_type << endl;
            printstate << "EX.rd_mem:\t" << state.EX.rd_mem << endl;
            printstate << "EX.wrt_mem:\t" << state.EX.wrt_mem << endl;
            printstate << "EX.alu_op:\t" << state.EX.alu_op << endl;
            printstate << "EX.wrt_enable:\t" << state.EX.wrt_enable << endl;
            printstate << "EX.nop:\t" << state.EX.nop << endl;

            printstate << "MEM.ALUresult:\t" << state.MEM.ALUresult << endl;
            printstate << "MEM.Store_data:\t" << state.MEM.Store_data << endl;
            printstate << "MEM.Rs:\t" << state.MEM.Rs << endl;
            printstate << "MEM.Rt:\t" << state.MEM.Rt << endl;
            printstate << "MEM.Wrt_reg_addr:\t" << state.MEM.Wrt_reg_addr << endl;
            printstate << "MEM.rd_mem:\t" << state.MEM.rd_mem << endl;
            printstate << "MEM.wrt_mem:\t" << state.MEM.wrt_mem << endl;
            printstate << "MEM.wrt_enable:\t" << state.MEM.wrt_enable << endl;
            printstate << "MEM.nop:\t" << state.MEM.nop << endl;

            printstate << "WB.Wrt_data:\t" << state.WB.Wrt_data << endl;
            printstate << "WB.Rs:\t" << state.WB.Rs << endl;
            printstate << "WB.Rt:\t" << state.WB.Rt << endl;
            printstate << "WB.Wrt_reg_addr:\t" << state.WB.Wrt_reg_addr << endl;
            printstate << "WB.wrt_enable:\t" << state.WB.wrt_enable << endl;
            printstate << "WB.nop:\t" << state.WB.nop << endl;
        } else {
            cout << "Unable to open FS StateResult output file." << endl;
        }
        printstate.close();
    }

private:
    string opFilePath;
};

int main(int argc, char* argv[]) {
    string ioDir = "";
    if (argc == 1) {
        cout << "Enter path containing the memory files: ";
        cin >> ioDir;
    } else if (argc > 2) {
        cout << "Invalid number of arguments. Machine stopped." << endl;
        return -1;
    } else {
        ioDir = argv[1];
        cout << "IO Directory: " << ioDir << endl;
    }

    InsMem imem = InsMem("Imem", ioDir);
    DataMem dmem_ss = DataMem("SS", ioDir);
    DataMem dmem_fs = DataMem("FS", ioDir);

    SingleStageCore SSCore(ioDir, imem, dmem_ss);
    FiveStageCore FSCore(ioDir, imem, dmem_fs);

    while (1) {
        if (!SSCore.halted)
            SSCore.step();

        if (!FSCore.halted)
            FSCore.step();

        if (SSCore.halted && FSCore.halted)
            break;
    }

    // Print the performance metrics
    SSCore.printPerformanceMetrics("SS");
    FSCore.printPerformanceMetrics("FS");

    // Dump SS and FS data mem.
    dmem_ss.outputDataMem();
    dmem_fs.outputDataMem();

    return 0;
}
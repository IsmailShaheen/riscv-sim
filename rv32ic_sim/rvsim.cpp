#include <iostream>
#include <fstream>
#include "stdlib.h"
#include <iomanip>

using namespace std;

int regs[32] = { 0 };
unsigned int pc = 0x0;
char memory[8 * 1024];	// only 8KB of memory located at address 0

void emitError(const char *s)
{
	cout << s;
	exit(0);
}

void printPrefix(unsigned int instA, unsigned int instW)
{
	cout << "0x" << hex << std::setfill('0') << std::setw(8) << instA << "\t0x" << std::setw(8) << instW << dec;
}

void instDecExec(unsigned int instWord)
{
	unsigned int rd, rs1, rs2, funct3, funct7, opcode;
	unsigned int I_imm, S_imm, B_imm, U_imm, J_imm;
	unsigned int address;
	unsigned int instPC = pc - 4;

	opcode = instWord & 0x0000007F;
	rd = (instWord >> 7) & 0x0000001F;
	funct3 = (instWord >> 12) & 0x00000007;
	rs1 = (instWord >> 15) & 0x0000001F;
	rs2 = (instWord >> 20) & 0x0000001F;
	funct7 = (instWord >> 25) & 0x0000007F;

	// — inst[31] — inst[30:25] inst[24:21] inst[20]
	I_imm = ((instWord >> 20) & 0x7FF) | ((instWord >> 31) ? 0xFFFFF800 : 0x0);
	S_imm = ((instWord >> 07) & 0x01F) | ((instWord >> 20) & 0x7E0) | (((instWord >> 31) ? 0xFFFFF800 : 0x0));
	B_imm = ((instWord >> 07) & 0x01E) | ((instWord >> 20) & 0x7E0) | ((instWord << 04) & 0x800) | ((instWord >> 31) ? 0xFFFFF000 : 0x0);
	U_imm = ((instWord >> 12) & 0x0007FFFF) | ((instWord >> 31) ? 0xFFF80000 : 0x0);
	J_imm = ((instWord >> 20) & 0x7FE) | ((instWord >> 9) & 0x800) | (instWord & 0x000FF000) | ((instWord >> 31) ? 0xFFF00000 : 0x0);

	printPrefix(instPC, instWord);

	if (opcode == 0x33) {		// R Instructions
		switch (funct3) {
		case 0: if (funct7 == 32) {
			cout << "\tsub\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //Sub instruction 
			regs[rd] = regs[rs1] - regs[rs2];
		}
				else {
					cout << "\tadd\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //Add Instruction 
					regs[rd] = regs[rs1] + regs[rs2];
				}
				break;

		case 1:
			cout << "\tsll\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //sll instruction 
			regs[rd] = regs[rs1] << regs[rs2];
			break;
		case 2:
			cout << "\tslt\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //slt instruction 
			if (regs[rs1] < regs[rs2]) regs[rd] = 1;
			else regs[rd] = 0;
			break;
		case 3:
			cout << "\tsltu\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //sltu instruction
			unsigned int r1, r2;
			r1 = regs[rs1];
			r2 = regs[rs2];
			if (r1 < r2) regs[rd] = 1;
			else regs[rd] = 0;
			break;
		case 4:
			cout << "\txor\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //xor instruction
			regs[rd] = regs[rs1] ^ regs[rs2];
			break;
		case 5:
			if (funct7 == 32)
			{
				cout << "\tsra\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //sra instruction
				regs[rd] = regs[rs1] >> regs[rs2];
			}
			else
			{
				cout << "\tsrl\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //srl instruction
				regs[rd] = regs[rs1] >> regs[rs2];
			}
			break;
		case 6:
			cout << "\tor\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //or instruction
			regs[rd] = regs[rs1] | regs[rs2];
			break;
		case 7:
			cout << "\tand\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //and instruction
			regs[rd] = regs[rs1] & regs[rs2];
			break;
		default:
			cout << "\tUnkown R Instruction \n";
		}
	}
	else if (opcode == 0x13) {	// I instructions
		switch (funct3) {
		case 0:	cout << "\taddi\tx" << rd << ", x" << rs1 << ", " << hex << "0x" << (int)I_imm << "\n"; //addi instruction
			regs[rd] = regs[rs1] + (int)I_imm;
			break;
		case 1:
			cout << "\tslli\tx" << rd << ", x" << rs1 << ", " << hex << "0x" << (int)I_imm << "\n"; //slli instruction
			regs[rd] = regs[rs1] << (int)I_imm;
			break;

		case 2:
			cout << "\tslti\tx" << rd << ", x" << rs1 << ", " << hex << "0x" << (int)I_imm << "\n"; //slti instruction
			if (regs[rs1] < (int)I_imm) regs[rd] = 1;
			else regs[rd] = 0;
			break;
		case 3:
			cout << "\tsltiu\tx" << rd << ", x" << rs1 << ", " << hex << "0x" << (int)I_imm << "\n"; //sltiu instruction
			unsigned int r1;
			r1 = regs[rs1];
			if (r1 < I_imm) regs[rd] = 1;
			else regs[rd] = 0;
			break;
		case 4:
			cout << "\txori\tx" << rd << ", x" << rs1 << ", " << hex << "0x" << (int)I_imm << "\n"; //xori instruction
			regs[rd] = regs[rs1] ^ (int)I_imm;
			break;
		case 5:
			if ((int)I_imm >> 5 == 0)
			{
				cout << "\tsrli\tx" << rd << ", x" << rs1 << ", " << hex << "0x" << (int)I_imm << "\n"; //srli instruction
				regs[rd] =(unsigned int) regs[rs1] >> (I_imm & 0x1F); //unsigned imm
			}
			else
			{
				cout << "\tsrai\tx" << rd << ", x" << rs1 << ", " << hex << "0x" << (int)I_imm << "\n"; //srai instruction
				int shamt;
				shamt = I_imm & 0x1F; //get the least siginificant 5 bits of the I_imm as the shift value 
				regs[rd] = regs[rs1] >> shamt;

			}
			break;
		case 6:
			cout << "\tori\tx" << rd << ", x" << rs1 << ", " << hex << "0x" << (int)I_imm << "\n"; //ori instruction
			regs[rd] = regs[rs1] | (int)I_imm;
			break;
		case 7:
			cout << "\tandi\tx" << rd << ", x" << rs1 << ", " << hex << "0x" << (int)I_imm << "\n"; //andi instruction
			regs[rd] = regs[rs1] & (int)I_imm;
			break;
		default:
			cout << "\tUnkown I Instruction \n";
		}
	}
	else if (opcode == 0x3) { //I instructions - Load
		unsigned int address;
		switch (funct3) {
		case 0:
			cout << "\tlb\tx" << rd << ", " << hex << "0x" << (int)I_imm << "(x" << rs1 << ')' << "\n"; //lb instruction

			address = I_imm +(unsigned int) regs[rs1];
			regs[rd] = (unsigned char)memory[address] | ((memory[address]) >> 7 ? 0xFFFFFF00 : 0x0);
			break;
		case 1:
			cout << "\tlh\tx" << rd << ", " << hex << "0x" << (int)I_imm << "(x" << rs1 << ')' << "\n"; //lh instruction
			address = I_imm + (unsigned int)regs[rs1];
			regs[rd] = (unsigned char)memory[address] | (unsigned char)memory[address + 1] << 8 | ((memory[address + 1]) >> 7 ? 0xFFFF0000 : 0x0);
			break;
		case 2:
			cout << "\tlw\tx" << rd << ", " << hex << "0x" << (int)I_imm << "(x" << rs1 << ')' << "\n"; //lw instruction
			address = I_imm + (unsigned int)regs[rs1];
			regs[rd] = (unsigned char)memory[address] |
				(((unsigned char)memory[address + 1]) << 8) |
				(((unsigned char)memory[address + 2]) << 16) |
				(((unsigned char)memory[address + 3]) << 24);
			break;
		case 4:
			cout << "\tlbu\tx" << rd << ", " << hex << "0x" << (int)I_imm << "(x" << rs1 << ')' << "\n"; //lbu instruction
			address = I_imm + (unsigned int)regs[rs1];
			regs[rd] = memory[address];
			break;
		case 5:
			cout << "\tlhu\tx" << rd << ", " << hex << "0x" << (int)I_imm << "(x" << rs1 << ')' << "\n"; //lhu instruction
			address = I_imm + (unsigned int)regs[rs1];
			regs[rd] = (unsigned char)memory[address] | (unsigned char)memory[address + 1] << 8;
			break;
		default:
			cout << "\tUnkown I Instruction \n";
		}

	}
	else if (opcode == 0x23) {	// S Instruction
		switch (funct3) {
		case 0:
			cout << "\tsb\tx" << rs2 << ", " << (int)S_imm << "(x" << rs1 << ")\n";
			memory[(unsigned int)(regs[rs1] + (int)S_imm)] = char(regs[rs2] & 0x0FF);
			break;
		case 1:
			cout << "\tsh\tx" << rs2 << ", " << (int)S_imm << "(x" << rs1 << ")\n";
			memory[(unsigned int)(regs[rs1] + (int)S_imm)] = char(regs[rs2] & 0x0FF);
			memory[(unsigned int)(regs[rs1] + (int)S_imm + 1)] = char((regs[rs2] >> 8) & 0x0FF);
			break;
		case 2:
			cout << "\tsw\tx" << rs2 << ", " << (int)S_imm << "(x" << rs1 << ")\n";
			for (int i = 0; i < 4; i++)
				memory[(unsigned int)(regs[rs1] + (int)S_imm + i)] = char((regs[rs2] >> (i * 8)) & 0x0FF);
			break;
		default:
			cout << "\tUnkown S Instruction \n";
		}
	}
	else if (opcode == 0x63) {	// B Instruction
		switch (funct3) {
		case 0:
			cout << "\tbeq\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if (regs[rs1] == regs[rs2]) pc = pc + (int)B_imm - 4;
			break;
		case 1:
			cout << "\tbne\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if (regs[rs1] != regs[rs2]) pc = pc + (int)B_imm - 4;
			break;
		case 4:
			cout << "\tblt\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if (regs[rs1] < regs[rs2]) pc = pc + (int)B_imm - 4;
			break;
		case 5:
			cout << "\tbge\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if (regs[rs1] >= regs[rs2]) pc = pc + (int)B_imm - 4;
			break;
		case 6:
			cout << "\tbltu\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if ((unsigned int)regs[rs1] < (unsigned int)regs[rs2]) pc = pc + (int)B_imm - 4;
			break;
		case 7:
			cout << "\tbgeu\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if ((unsigned int)regs[rs1] >= (unsigned int)regs[rs2]) pc = pc + (int)B_imm - 4;
			break;
		default:
			cout << "\tUnkown B Instruction \n";
		}
	}
	else if (opcode == 0x37 || opcode == 0x17) {	// U instruction
		switch (opcode) {
		case 0x37:
			cout << "\tlui\tx" << rd << hex << ", 0x" << std::setw(8) << (int)U_imm << "\n" << dec;
			regs[rd] = U_imm << 12;
			break;
		case 0x17:
			cout << "\tauipc\tx" << rd << hex << ", 0x" << std::setw(8) << (int)U_imm << "\n" << dec;
			regs[rd] = pc + (U_imm << 12);
			break;
		}
	}
	else if (opcode == 0x6F) {	// J Instruction
		cout << "\tjal\tx" << rd << hex << ", 0x" << std::setw(8) << ((int)J_imm >> 1) << "\n" << dec;
		regs[rd] = pc + 4;
		pc = pc + J_imm - 4;
	}
	else {
		cout << "\tUnkown Instruction \n";
	}
}

int main(int argc, char *argv[]) {

	unsigned int instWord = 0;
	ifstream inFile;
	ofstream outFile;

	if (argc < 2) emitError("use: rv32i_sim <machine_code_file_name>\n");

	inFile.open(argv[1], ios::in | ios::binary | ios::ate);

	if (inFile.is_open())
	{
		int fsize = inFile.tellg();

		inFile.seekg(0, inFile.beg);
		if (!inFile.read(memory, fsize)) emitError("Cannot read from input file\n");

		while (true) { //32-bit instructions
					   //if (((unsigned char)memory[pc] & 0x3)==0x3)
					   //{
			instWord = (unsigned char)memory[pc] |
				(((unsigned char)memory[pc + 1]) << 8) |
				(((unsigned char)memory[pc + 2]) << 16) |
				(((unsigned char)memory[pc + 3]) << 24);
			//}
			/*else //compressed instructions
			{
			instWord = (unsigned char)memory[pc] |(((unsigned char)memory[pc + 1]) << 8);
			}*/

			pc += 4;
			// remove the following line once you have a complete simulator
			if (pc == 512) break;			// stop when PC reached address 32
			instDecExec(instWord);
		}

		// dump the registers
		for (int i = 0; i < 32; i++)
			cout << "x" << dec << i << ": \t" << "0x" << hex << std::setfill('0') << std::setw(8) << regs[i] << "\n" << dec;
		// Only for debugging dump memory
		/*for (int i = 0; i < 16; i++)
			cout << "x" << hex << std::setfill('0') << std::setw(8) << i << ": \t" << "0x" << hex << (int)memory[i] << "\n";*/

	}
	else emitError("Cannot access input file\n");

	system("pause"); // Only for debugging
	return 0;
}
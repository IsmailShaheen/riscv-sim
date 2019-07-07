#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

int regs[32] = { 0 };
unsigned int pc = 0x0;
char memory[8 * 1024];	// only 8KB of memory located at address 0
bool instFlag; 
bool exitFlag = 0;

void emitError(const char *s)
{
	cout << s;
	system("pause"); // Only for debugging
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
	unsigned int instPC;
	unsigned char instSize;
	if (instFlag) {
		instPC = pc - 4;
		instSize = 4;
	}
	else {
		instPC = pc - 2;
		instSize = 2;
	}

	if (instFlag) //32-bit instruction 
	{
		opcode = instWord & 0x0000007F;
		rd = (instWord >> 7) & 0x0000001F;
		funct3 = (instWord >> 12) & 0x00000007;
		rs1 = (instWord >> 15) & 0x0000001F;
		rs2 = (instWord >> 20) & 0x0000001F;
		funct7 = (instWord >> 25) & 0x0000007F;

		I_imm = ((instWord >> 20) & 0x7FF) | ((instWord >> 31) ? 0xFFFFF800 : 0x0);
		S_imm = ((instWord >> 07) & 0x01F) | ((instWord >> 20) & 0x7E0) | (((instWord >> 31) ? 0xFFFFF800 : 0x0));
		B_imm = ((instWord >> 07) & 0x01E) | ((instWord >> 20) & 0x7E0) | ((instWord << 04) & 0x800) | ((instWord >> 31) ? 0xFFFFF000 : 0x0);
		U_imm = ((instWord >> 12) & 0x0007FFFF) | ((instWord >> 31) ? 0xFFF80000 : 0x0);
		J_imm = ((instWord >> 20) & 0x7FE) | ((instWord >> 9) & 0x800) | (instWord & 0x000FF000) | ((instWord >> 31) ? 0xFFF00000 : 0x0);
	}
	else
	{
		unsigned char quad = instWord & 0x3;
		unsigned char cfunct3 = (instWord >> 13) & 0x7;
		switch (quad) {
		case 0: // Quadrant 0
			opcode = 0x3;
			funct3 = 2;
			rs1 = (instWord >> 7) & 0x7;
			I_imm = ((instWord >> 4) & 0x4) | ((instWord >> 7) & 0x38) | ((instWord << 1) & 0x40);
			if (cfunct3 == 2) { // c.lw Instruction
				rd = (instWord >> 2) & 0x7;
				cout << "\t\t\t\t\c.lw\tx" << rd << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "(x" << dec << rs1 << ')' << "\n"; //c.lw instruction
			}
			else if (cfunct3 == 6) { // c.sw Instruction
				rs2 = (instWord >> 2) & 0x7;
				S_imm = ((instWord >> 4) & 0x4) | ((instWord >> 7) & 0x38) | ((instWord << 1) & 0x40);
				cout << "\t\t\t\t\c.sw\tx" << rs2 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)S_imm << "(x" << rs1 << ")\n";
			}
			else {
				cout << "\tUnkown Quadrant 0 Compressed Instruction \n";
				return;
			}
			break;
		case 1: // Quadrant 1
			switch (cfunct3) {
			case 0: {// c.addi Instrcution
				opcode = 0x13;
				rd = (instWord >> 7) & 0x1F;
				funct3 = 0;
				rs1 = rd;
				I_imm = ((instWord >> 2) & 0x1F) | (((instWord >> 12) & 0x1) ? 0xFFFFFFE0 : 0x0);
				cout << "\t\t\t\t\c.addi\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //addi instruction

				break; }
			case 1: // c.jal Instrcution
				opcode = 0x6F;
				rd = 1;
				J_imm = ((instWord << 3) & 0x20) | ((instWord >> 2) & 0xE) | ((instWord << 1) & 0x80) | ((instWord >> 1) & 0x40) | ((instWord << 2) & 0x400) | ((instWord >> 1) & 0x300) | ((instWord >> 7) & 0x10) | (((instWord >> 12) & 0x1) ? 0xFFFFF800 : 0x0);
				cout << "\t\t\t\t\c.jal\tx" << rd << hex << ", 0x" << std::setw(8) << ((int)J_imm >> 1) << "\n" << dec;
				break;
			case 2: // c.li Instruction
				opcode = 0x13;
				rd = (instWord >> 7) & 0x1F;
				if (rd == 0) {
					cout << "\tHINTs Instruction Detected\n";
					return;
				}
				funct3 = 0;
				rs1 = 0;
				I_imm = ((instWord >> 2) & 0x1F) | (((instWord >> 12) & 0x1) ? 0xFFFFFFE0 : 0x0);
				cout << "\t\t\t\t\c.li\tx" << rd << ", x" << rs1 << hex << ", 0x" << std::setw(8) << ((int)I_imm >> 1) << "\n" << dec;
				break;
			case 3:
				rd = (instWord >> 7) & 0x1F;
				if (rd == 2) { // c.addi16sp Instruction
					opcode = 0x13;
					funct3 = 0;
					rs1 = rd;
					I_imm = ((instWord << 3) & 0x20) | ((instWord << 4) & 0x180) | ((instWord << 1) & 0x40) | ((instWord >> 2) & 0x10) | (((instWord >> 12) & 0x1) ? 0xFFFFFE00 : 0x0);
					cout << "\t\t\t\t\c.addi4spn\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec;
				}
				else if (rd == 0) {
					cout << "\tHINTs Instruction Detected\n";
					return;
				}
				else { // c.lui Instruction
					opcode = 0x37;
					U_imm = ((instWord << 10) & 0x1F000) | (((instWord >> 12) & 0x1) ? 0xFFFE0000 : 0x0);
					cout << "\t\t\t\t\c.lui\tx" << rd << hex << ", 0x" << std::setw(8) << ((int)U_imm >> 1) << "\n" << dec;
				}
				break;
			case 4: {
				unsigned char cfunct2 = ((instWord >> 10) & 0x3);
				switch (cfunct2) {
				case 0: // c.srli
					opcode = 0x13;
					rd = (instWord >> 7) & 0x7;
					funct3 = 5;
					rs1 = rd;
					I_imm = ((instWord >> 2) & 0x1F) | ((instWord >> 7) & 0x20);
					cout << "\t\t\t\t\c.srli\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //c.srli instruction

					break;
				case 1: // c.srai
					opcode = 0x13;
					rd = (instWord >> 7) & 0x7;
					funct3 = 5;
					rs1 = rd;
					I_imm = ((instWord >> 2) & 0x1F) | ((instWord >> 7) & 0x20) | 0x40000000;
					cout << "\t\t\t\t\c.srai\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //c.srai instruction

					break;
				case 2: // c.andi
					opcode = 0x13;
					rd = (instWord >> 7) & 0x7;
					funct3 = 7;
					rs1 = rd;
					I_imm = ((instWord >> 2) & 0x1F) | (((instWord >> 12) & 0x1) ? 0xFFFFFFE0 : 0x0);
					cout << "\t\t\t\t\c.andi\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //c.andi instruction

					break;
				case 3:
					if ((instWord >> 12) & 0x1) {
						cout << "\tReserved Instruction Detected\n";
						return;
					}
					else {
						unsigned char cfunct2_2 = ((instWord >> 5) & 0x3);
						opcode = instWord & 0x33;
						rd = (instWord >> 7) & 0x7;
						rs1 = rd;
						rs2 = (instWord >> 2) & 0x7;
						switch (cfunct2_2) {
						case 0: // c.sub
							funct3 = 0;
							funct7 = 32;
							cout << "\t\t\t\t\c.sub\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //c.sub instruction
							break;
						case 1: // c.xor
							funct3 = 4;
							funct7 = 0;
							cout << "\t\t\t\t\c.xor\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //c.xor instruction
							break;
						case 2: // c.or
							funct3 = 6;
							funct7 = 0;
							cout << "\t\t\t\t\c.or\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //c.or instruction
							break;
						case 3: // c.and
							funct3 = 7;
							funct7 = 0;
							cout << "\t\t\t\t\c.and\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //c.and instruction
							break;
						default:
							cout << "\tUnkown Quadrant 1 Compressed Instruction (cfunct3 = 0x3, cfunct2 = 0x3)\n";
							return;
							break;
						}
					}
					break;
				default:
					cout << "\tUnkown Quadrant 1 Compressed Instruction (cfunct3 = 0x3)\n";
					return;
					break;
				}
				break;
			}
			case 5: // c.j Instruction
				opcode = 0x6F;
				rd = 0;
				J_imm = ((instWord << 3) & 0x20) | ((instWord >> 2) & 0xE) | ((instWord << 1) & 0x80) | ((instWord >> 1) & 0x40) | ((instWord << 2) & 0x400) | ((instWord >> 1) & 0x300) | ((instWord >> 7) & 0x10) | (((instWord >> 12) & 0x1) ? 0xFFFFF800 : 0x0);
				cout << "\t\t\t\t\c.j\tx" << rd << hex << ", 0x" << std::setw(8) << ((int)J_imm >> 1) << "\n" << dec;
				break;
			case 6: // c.beqz
				opcode = 0x63;
				funct3 = 0;
				rs1 = ((instWord << 7) & 0x7);
				rs2 = 0;
				B_imm = ((instWord << 3) & 0x20) | ((instWord >> 2) & 0x6) | ((instWord << 1) & 0xC0) | ((instWord >> 7) & 0x18) | (((instWord >> 12) & 0x1) ? 0xFFFFFF00 : 0x0);
				cout << "\t\t\t\t\c.beqz\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
				break;
			case 7: // c.bnez
				opcode = 0x63;
				funct3 = 1;
				rs1 = ((instWord << 7) & 0x7);
				rs2 = 0;
				B_imm = ((instWord << 3) & 0x20) | ((instWord >> 2) & 0x6) | ((instWord << 1) & 0xC0) | ((instWord >> 7) & 0x18) | (((instWord >> 12) & 0x1) ? 0xFFFFFF00 : 0x0);
				cout << "\t\t\t\t\c.bnez\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
				break;
			default:
				cout << "\tUnkown Quadrant 1 Compressed Instruction \n";
				return;
				break;
			}
			break;
		case 2: // Quadrant 2
			switch (cfunct3) {
			case 0: // c.slli
				opcode = 0x13;
				rd = (instWord >> 7) & 0x1F;
				funct3 = 1;
				rs1 = rd;
				I_imm = ((instWord >> 2) & 0x1F);
				if ((rd | I_imm) == 0) {
					cout << "\tHINTs Instruction Detected\n";
					return;
				}
				if ((instWord >> 12) & 0x1) {
					cout << "\tReserved Instruction Detected\n";
					return;
				}
				cout << "\t\t\t\t\c.slli\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //c.slli instruction
				break;
			case 1:
				cout << "\tUnkown Quadrant 2 Compressed Instruction \n";
				return;
				break;
			case 2: // c.lwsp
				opcode = 0x3;
				rd = (instWord >> 7) & 0x1F;
				funct3 = 2;
				rs1 = 2;
				I_imm = ((instWord << 4) & 0xC0) | ((instWord >> 2) & 0x1C) | ((instWord >> 7) & 0x20);
				if (rd == 0) {
					cout << "\tReserved Instruction Detected\n";
					return;
				}
				cout << "\t\t\t\t\c.lwsp\tx" << rd << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "(x" << dec << rs1 << ')' << "\n"; //c.lwsp instruction

				break;
			case 3:
				cout << "\tUnkown Quadrant 2 Compressed Instruction \n";
				return;
				break;
			case 4:
				rs2 = ((instWord >> 2) & 0x1F);
				if ((instWord >> 12) & 0x1) {
					rs1 = ((instWord >> 7) & 0x1F);
					if (rs1 == 0) {
						cout << "\tHINTs Instruction Detected\n";
						return;
					}
					if (rs2 == 0) { // c.jalr
						opcode = 0x67;
						rd = 1;
						I_imm = 0;
						cout << "\t\t\t\t\c.jalr\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec;

					}
					else { // c.add
						opcode = 0x33;
						funct3 = 0;
						funct7 = 0;
						rd = rs1;
						cout << "\t\t\t\t\c.add\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n";
					}
				}
				else {
					if (rs2 == 0) { // c.jr
						opcode = 0x67;
						rs1 = ((instWord >> 7) & 0x1F);
						rd = 0;
						I_imm = 0;
						cout << "\t\t\t\t\c.jr\tx" << rd << ", x" << rs1 << ", x" << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec;
						/*if (rs1 == 0) {
							cout << "\tReserved Instruction Detected\n";
							return;
						}*/
					}
					else { // c.mv
						opcode = 0x33;
						funct3 = 0;
						funct7 = 0;
						rd = ((instWord >> 7) & 0x1F);
						rs1 = 0;
						if (rd == 0) {
							cout << "\tHINTs Instruction Detected\n";
							return;
						}
						cout << "\t\t\t\t\c.mv\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n";
					}
				}
				break;
			case 5:
				cout << "\tUnkown Quadrant 2 Compressed Instruction \n";
				return;
				break;
			case 6: // c.swsp
				opcode = 0x23;
				funct3 = 2;
				rs1 = 2;
				rs2 = ((instWord >> 2) & 0x1F);
				S_imm = ((instWord >> 1) & 0xC0) | ((instWord >> 7) & 0x3C);
				cout << "\t\t\t\t\c.swsp\tx" << rs2 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)S_imm << "(x" << rs1 << ")\n";

				break;
			case 7:
				cout << "\tUnkown Quadrant 2 Compressed Instruction \n";
				return;
				break;
			default:
				cout << "\tUnkown Quadrant 2 Compressed Instruction \n";
				return;
				break;
			}
			break;
		default:
			cout << "\tUnkown Compressed Instruction \n";
			return;
			break;
		}
	}

	printPrefix(instPC, instWord);

	if (opcode == 0x33) {		// R Instructions
		switch (funct3) {
		case 0:
			if (funct7 == 32) {
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
			if ((unsigned int)regs[rs1] < (unsigned int)regs[rs2]) regs[rd] = 1;
			else regs[rd] = 0;
			break;
		case 4:
			cout << "\txor\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //xor instruction
			regs[rd] = regs[rs1] ^ regs[rs2];
			break;
		case 5:
			if (funct7 == 32) {
				cout << "\tsra\tx" << rd << ", x" << rs1 << ", x" << rs2 << "\n"; //sra instruction
				regs[rd] = regs[rs1] >> regs[rs2];
			}
			else {
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
			break;
		}
	}
	else if (opcode == 0x13) {	// I instructions
		switch (funct3) {
		case 0:
			cout << "\taddi\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //addi instruction
			regs[rd] = regs[rs1] + (int)I_imm;
			break;
		case 1:
			cout << "\tslli\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //slli instruction
			regs[rd] = regs[rs1] << (int)I_imm;
			break;

		case 2:
			cout << "\tslti\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //slti instruction
			if (regs[rs1] < (int)I_imm) regs[rd] = 1;
			else regs[rd] = 0;
			break;
		case 3:
			cout << "\tsltiu\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //sltiu instruction
			if ((unsigned int)regs[rs1] < I_imm) regs[rd] = 1;
			else regs[rd] = 0;
			break;
		case 4:
			cout << "\txori\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //xori instruction
			regs[rd] = regs[rs1] ^ (int)I_imm;
			break;
		case 5:
			if ((int)I_imm >> 5 == 0)
			{
				cout << "\tsrli\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //srli instruction
				regs[rd] =(unsigned int) regs[rs1] >> (I_imm & 0x1F); //unsigned imm
			}
			else
			{
				cout << "\tsrai\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //srai instruction
				int shamt;
				shamt = I_imm & 0x1F; //get the least siginificant 5 bits of the I_imm as the shift value 
				regs[rd] = regs[rs1] >> shamt;
			}
			break;
		case 6:
			cout << "\tori\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //ori instruction
			regs[rd] = regs[rs1] | (int)I_imm;
			break;
		case 7:
			cout << "\tandi\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec; //andi instruction
			regs[rd] = regs[rs1] & (int)I_imm;
			break;
		default:
			cout << "\tUnkown I Instruction \n";
			break;
		}
	}
	else if (opcode == 0x3) { //I instructions - Load
		unsigned int address;
		switch (funct3) {
		case 0:
			cout << "\tlb\tx" << rd << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "(x" << dec << rs1 << ')' << "\n"; //lb instruction
			address = I_imm +(unsigned int) regs[rs1];
			regs[rd] = (unsigned char)memory[address] | ((memory[address]) >> 7 ? 0xFFFFFF00 : 0x0);
			break;
		case 1:
			cout << "\tlh\tx" << rd << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "(x" << dec << rs1 << ')' << "\n"; //lh instruction
			address = I_imm + (unsigned int)regs[rs1];
			regs[rd] = (unsigned char)memory[address] | (unsigned char)memory[address + 1] << 8 | ((memory[address + 1]) >> 7 ? 0xFFFF0000 : 0x0);
			break;
		case 2:
			cout << "\tlw\tx" << rd << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "(x" << dec << rs1 << ')' << "\n"; //lw instruction
			address = I_imm + (unsigned int)regs[rs1];
			regs[rd] = (unsigned char)memory[address] |
				(((unsigned char)memory[address + 1]) << 8) |
				(((unsigned char)memory[address + 2]) << 16) |
				(((unsigned char)memory[address + 3]) << 24);
			break;
		case 4:
			cout << "\tlbu\tx" << rd << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "(x" << dec << rs1 << ')' << "\n"; //lbu instruction
			address = I_imm + (unsigned int)regs[rs1];
			regs[rd] = memory[address];
			break;
		case 5:
			cout << "\tlhu\tx" << rd << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "(x" << dec << rs1 << ')' << "\n"; //lhu instruction
			address = I_imm + (unsigned int)regs[rs1];
			regs[rd] = (unsigned char)memory[address] | (unsigned char)memory[address + 1] << 8;
			break;
		default:
			cout << "\tUnkown I Instruction \n";
			break;
		}
	}
	else if (opcode == 0x67) { //jalr instruction 
		cout << "\tjalr\tx" << rd << ", x" << rs1 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)I_imm << "\n" << dec;
		regs[rd] = pc;
		pc = (regs[rs1] + (int)I_imm) & 0xFFFFFFFE;
	}
	else if (opcode == 0x23) {	// S Instruction
		switch (funct3) {
		case 0:
			cout << "\tsb\tx" << rs2 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)S_imm << "(x" << rs1 << ")\n";
			memory[(unsigned int)(regs[rs1] + (int)S_imm)] = char(regs[rs2] & 0x0FF);
			break;
		case 1:
			cout << "\tsh\tx" << rs2 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)S_imm << "(x" << rs1 << ")\n";
			memory[(unsigned int)(regs[rs1] + (int)S_imm)] = char(regs[rs2] & 0x0FF);
			memory[(unsigned int)(regs[rs1] + (int)S_imm + 1)] = char((regs[rs2] >> 8) & 0x0FF);
			break;
		case 2:
			cout << "\tsw\tx" << rs2 << ", 0x" << hex << std::setfill('0') << std::setw(8) << (int)S_imm << "(x" << rs1 << ")\n";
			for (int i = 0; i < 4; i++)
				memory[(unsigned int)(regs[rs1] + (int)S_imm + i)] = char((regs[rs2] >> (i * 8)) & 0x0FF);
			break;
		default:
			cout << "\tUnkown S Instruction \n";
			break;
		}
	}
	else if (opcode == 0x63) {	// B Instruction
		switch (funct3) {
		case 0:
			cout << "\tbeq\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if (regs[rs1] == regs[rs2]) pc = pc + (int)B_imm - instSize;
			break;
		case 1:
			cout << "\tbne\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if (regs[rs1] != regs[rs2]) pc = pc + (int)B_imm - instSize;
			break;
		case 4:
			cout << "\tblt\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if (regs[rs1] < regs[rs2]) pc = pc + (int)B_imm - instSize;
			break;
		case 5:
			cout << "\tbge\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if (regs[rs1] >= regs[rs2]) pc = pc + (int)B_imm - instSize;
			break;
		case 6:
			cout << "\tbltu\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if ((unsigned int)regs[rs1] < (unsigned int)regs[rs2]) pc = pc + (int)B_imm - instSize;
			break;
		case 7:
			cout << "\tbgeu\tx" << rs1 << ", x" << rs2 << ", " << hex << "0x" << std::setw(8) << ((int)B_imm >> 1) << "\n" << dec;
			if ((unsigned int)regs[rs1] >= (unsigned int)regs[rs2]) pc = pc + (int)B_imm - instSize;
			break;
		default:
			cout << "\tUnkown B Instruction \n";
			break;
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
			regs[rd] = pc - instSize + (U_imm << 12);
			break;
		}
	}
	else if (opcode == 0x6F) {	// J Instruction
		cout << "\tjal\tx" << rd << hex << ", 0x" << std::setw(8) << ((int)J_imm >> 1) << "\n" << dec;
		regs[rd] = pc;
		pc = pc + (int)J_imm - instSize;
	}
	else if (opcode == 0x73) {	// System Instruction
		if (!I_imm) {
			cout << "\tecall\n";
			int j = 0;
			switch (regs[17]) {
			case 1:
				cout << (int)regs[10] << '\n';
				break;
			case 4:
				while (memory[(unsigned int)regs[10] + j] != 0) {
					cout << (char)memory[(unsigned int)regs[10] + j++];
				}
				break;
			case 5:
				cin >> regs[10];
				break;
			case 8:
				cin.ignore();
				do {
					if ((unsigned int)regs[10] + j > (8 * 1024)) {
						cout << "Exception: The entered string is out of memory range!\n";
						return; 
					}
					memory[(unsigned int)regs[10] + j] = getchar();
					j++;
				} while (memory[(unsigned int)regs[10] + j - 1] != '\n' && j < regs[11] - 1);
				memory[(unsigned int)regs[10] + j] = 0;
				break;
			case 10:
				exitFlag = 1;
				break;
			default:
				cout << "\tUnknown system instruction service number\n";
				break;
			}
		}
		else
			cout << "\tUnkown System Instruction \n";
	}
	else {
		cout << "\tUnkown Instruction \n";
	}
}

int main(int argc, char *argv[]) {

	unsigned int instWord = 0;
	ifstream inFile;

	if (argc < 2) emitError("use: rv32i_sim <machine_code_file_name>\n");

	inFile.open(argv[1], ios::in | ios::binary | ios::ate);

	if (inFile.is_open())
	{
		int fsize = inFile.tellg();

		inFile.seekg(0, inFile.beg);
		if (!inFile.read(memory, fsize)) emitError("Cannot read from input file\n");
		
		while (true) {
			regs[0] = 0;
			if (((unsigned char)memory[pc] & 0x3) == 0x3) { // 32-bit instructions
				instFlag = true;
				instWord = (unsigned char)memory[pc] |
					(((unsigned char)memory[pc + 1]) << 8) |
					(((unsigned char)memory[pc + 2]) << 16) |
					(((unsigned char)memory[pc + 3]) << 24);
				pc += 4;
				if ((unsigned int)pc>(8 * 1024)) {
					cout << "The enetered file is out of memory constraints, we only support 8KB of memory!\n";
					break; 
				}
			}
			else { // 16-bit instructions
				instFlag = false;
				instWord = (unsigned char)memory[pc] | (((unsigned char)memory[pc + 1]) << 8);
				pc += 2;
				if ((unsigned int) pc>(8 * 1024)) {
					cout << "The enetered file is out of memory constraints, we only support 8KB of memory!\n";
					break;
				}
			}

			if (exitFlag) break;			// stop when PC reached address 512
			instDecExec(instWord);
		}

		// dump the registers
		for (int i = 0; i < 32; i++)
			cout << "x" << dec << i << ": \t" << "0x" << hex << std::setfill('0') << std::setw(8) << regs[i] << "\n" << dec;
		// dump memory (only for debugging)
		/*for (int i = 0; i < 16; i++)
			cout << "x" << hex << std::setfill('0') << std::setw(8) << i << ": \t" << "0x" << hex << (int)memory[i] << "\n";*/
		inFile.close();
	}
	else emitError("Cannot access input file\n");

	return 0;
}
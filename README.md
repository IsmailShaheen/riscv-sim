# riscv-sim
RV32IC console operated simulator

# DESIGN: 
The software is supposed to read machine code file (binary file) and decode this code to executable RISCV-32IC instructions. The way it does so is by reading and opening the names of the executable file and the binary file to be decoded from the terminal. The program then checks the first 2 bits to determine whether it is a 32-bit instruction or a compressed instruction, then it reads and encodes the instruction word, print it and execute it. 
# FUNCTIONS USED: 
void printPrefix(unsigned int instA, unsigned int instW)

•	This function prints the hexadecimal values of the program counter and the instruction word. 

void instDecExec(unsigned int instWord)

•	This function receives the instruction word that was read by the program. It then decodes this instruction word based on the type of instruction to opCodes, destination registers, source registers, immediate numbers, func3, and/or func7.

•	This function is also responsible for mapping the compressed instructions to their matching 32-bit I instructions.

•	Then the function prints and executes the decoded instructions based on their opcodes, func3, and func7. 

int main(int argc, char *argv[]) 

•	The main function receives the number of arguments and an array of characters representing the files names needed to be accessed by the program. 

•	It opens the bin file, reads the content and verify whether the instruction is a 32-bit or compressed instruction and increments the program counter. 

•	It then dumps the registers and their values to the terminal. 
void emitError(const char *s)

•	Prints Error to the screen by passing the error as a c_string



# HOW TO USE THE SIMULATOR: 

1-	Build the project in Visual Studio 

2-	Follow the path to the executable file 

3-	Open the terminal and change the directory to the location of the executable file and the binary file 

4-	Enter the name of the executable file followed by the name of the binary file

5-	Program counter, Instructions and their addresses will be printed on the terminal followed by the registers and their values 

# PROGRAM LIMITATIONS 

•	It only supports RISCV-32IC instructions 

•	Does not support EBREAK, FENCE and CSR instructions.

•	It can’t decode more than one binary file at a time.

# CHALLENGES: 

•	Mapping the compressed instructions to the 32bit instructions 

•	Managing signed and unsigned operations 




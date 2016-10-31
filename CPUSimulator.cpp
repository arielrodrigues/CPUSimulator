#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <tuple>
#include <math.h>

#define getFlag(x) std::get<0>(x)
#define getiCode(x) std::get<1>(x)
#define getPriority(x) std::get<2>(x)
#define flagIsUP std::get<0>(INT_flag)
#define f_INTCode std::get<1>(INT_flag)
#define f_Priority std::get<2>(INT_flag)
#define f_CR std::get<3>(INT_flag)
#define IE R[35] & 0x40

/*
* ARQ-2016.1
* arielrodrigues_201310015491_poxim3.cpp
* 
* Registers:
* R: IPC(37) CR(36) FR(35) ER(34) IR(33) PC(32)
* FR [IE(6) IV(5) OV(4) ZD(3) GT(2) LT(1) EQ(0)]
*/

// array of memory: bitset of 32 pos
uint32_t* memory = nullptr;
int memoryLength = 0;

// registers
uint32_t R[64];
int32_t TIMER = -1; bool watchdog;

// flag, int code, priority, cr
std::tuple<bool, uint_fast8_t, int_fast8_t, uint_fast32_t> INT_flag(false, 0, 0, 0);

// interruptions stack
struct context {
	uint32_t FR, IPC;
	std::tuple<bool, int_fast8_t, int_fast8_t, uint_fast32_t> INT_flag;
};
context INTStack[3];
bool INTRoutine = false;

// FPU
struct fpu {
	float Xf, Yf, Zf;
	uint32_t X, Y, Z, controle;
	uint32_t ciclos = 0;
};
fpu Fpu;

// Cache
struct _block {
	uint32_t block[4] = {0};
	bool okay = false;
	int age = 0;
};
struct _cache {
	uint8_t identity;
	_block line[8];
};
_cache c_data[2], c_instructions[2];
uint8_t c_dataHit = 0, c_dataMiss = 0, 
	c_instructionsHit = 0, c_instructionsMiss = 0;

// out file
std::stringstream SSOUT, TERMINAL;
uint32_t bufferTerminal;

void ReadFile(std::string);
void saveContext(uint32_t FR);
void ULA();
void OPType_U(uint_fast8_t, uint32_t);
void OPType_F(uint_fast8_t, uint32_t);
void OPType_S(uint_fast8_t, uint32_t, bool*);
void INTManager(bool *okay);
void FPUManager();
void FPU();
void Watchdog();
void WriteToFile(std::string);
std::string getHexformat(uint64_t, int);

// main :)
int main(int argc, char *argv[]) {
	using namespace std;
	string inFileName = argv[1], outFileName = argv[2];
	ReadFile(inFileName);
	ULA();
	WriteToFile(outFileName);

//	delete[] memory;
	return 0;
}

// put the file in memory
void filetoMem(std::ifstream* file, std::string* fileInMemory) {
	file->seekg(0, std::ios::end);
	fileInMemory->resize(file->tellg());
	file->seekg(0);
	char* aux = new char[fileInMemory->size()];
	file->read(aux, fileInMemory->size());
	fileInMemory[0] = aux;
	delete[] aux;
	file->close();
}

// puts instructions in memory and free file in memory
void InstoMem(std::string* fileInMemory) {
	using namespace std;
	char token = '0';
	unsigned int i = 0;
	unsigned long hexAux;
	auto pos = string::npos;

	do {
		pos = fileInMemory->find(token);
		if (pos != string::npos) {
			try {
				hexAux = stoul(fileInMemory->substr(pos, pos + 10), NULL, 16);
			} catch (std::out_of_range) {
				std::cout << "eita lele" << std::endl;
			}
			memory[i++] = hexAux;
			*fileInMemory = fileInMemory->substr(11);
		} 
	} while (pos != string::npos);
	if (fileInMemory->length() > 0) fileInMemory->clear();
}

// readFile and saves all hexvalues in memory array
void ReadFile(std::string fileName) {
	using namespace std;
	ifstream file(fileName.c_str(), ifstream::in);
	std::string fileInMemory;
	if (!file.is_open()) {
		cout << "Unable to open file." << endl;
		exit(EXIT_FAILURE);
	} else {
		// puts the file in the memory
		filetoMem(&file, &fileInMemory);
		memoryLength = [fileinMem = fileInMemory]()->uint32_t { auto j = 0, i = 0;
		for (; i <= fileinMem.length(); j += fileinMem[i++] == '\n'); return j; }();
		memory = new uint32_t[memoryLength];
		InstoMem(&fileInMemory);
	}
}

// return OPType: U, F or S
int getOPType(uint_fast8_t OP, bool okay) {
	// exceptions
	if ((OP == 11) || (OP == 25)) return 'U';
	if (((OP == 20) || (OP == 22)) || ((OP > 36) && (OP < 41))) return 'F';
	if (OP == 63) return 'S';

	if (OP < 26) {
		if (OP % 2 != 0) return 'F';
		else return 'U';
	} if ((OP > 25) && (OP < 37)) return 'S';
	else {
		R[35] = R[35] | 0x20; R[37] = R[32] + 1;
		INT_flag = std::make_tuple(true, 3, 0, R[32]);
		saveContext(R[35]);
		return '0';
	}
}

_block MemToCache() {

/*	_block MemToCache() {
	struct _block {
	uint32_t block[4] = {0};
	bool okay = false;
	int age = 0;
};
*/
	_block bloco;
	bloco.
}

// cache reading manager
uint32_t cacheReadingManager() {
	uint32_t identity = (R[32] & 0xFF000000) >> 24,
			 line = (R[32] & 0x00e00000) >> 21,
			 word = (R[32] & 0x00180000) >> 19;
	//poe for each aqui 
	if (!c_data[0].line[line].okay && !c_data[1].line[line].okay) {
		c_dataMiss++;
	} else if (c_data[0].line[line].okay) {
		if (c_data[0].line[line].identity == identity) {
			c_dataHit++;
			//pega dado 
		} else { // isso n é um else
			if (c_data[1].line[line].okay) {
				if (c_data[1].line[line].identity == identity) {
					c_dataHit++;
					//pega dado 
				}
			}
			//substitui dado na cache
		}
	}

}

// cache writing manager
void cacheWritingManager() {
	
}

std::string cacheStatistics() {
	std::stringstream out;
	uint32_t c_data_total = c_dataHit + c_dataMiss,
		c_instructions_total = c_instructionsHit + c_instructionsMiss,
		d_hit = round(c_dataHit / c_data_total * 100),
		d_miss = round(c_dataMiss / c_data_total * 100),
		ins_hit = round(c_instructionsHit / c_instructions_total * 100),
		ins_miss = round(c_instructionsMiss / c_instructions_total * 100);

	out << "[CACHE D STATISTICS] #Hit = " << c_dataHit << "(" << d_hit 
		<< "%), #Miss = " << d_miss << "(" << d_miss << "%)\n" 
		<< "[CACHE I STATISTICS] #Hit = " << c_instructionsHit << "(" << ins_hit
		<< "%), #Miss = " << c_instructionsMiss << "(" << ins_miss << "%)";
	return out.str();
}

// sort contexts in stack by priority
void sortContext() {
	for (auto i = 0; i < 3; i++)
		for (auto j = i + 1; j < 3; j++)
			if ((getPriority(INTStack[j].INT_flag) < getPriority(INTStack[i].INT_flag)
				&& getFlag(INTStack[j].INT_flag))
				|| (!getFlag(INTStack[i].INT_flag) && getFlag(INTStack[j].INT_flag))) {
				auto aux = INTStack[i]; INTStack[i] = INTStack[j]; INTStack[j] = aux;
			}
}

// save CPU context before get a level down
void saveContext(uint32_t FR) {
	uint_fast8_t i = 0;
	for (; i < 3, std::get<0>(INTStack[i].INT_flag); i++)
		if (getiCode(INTStack[i].INT_flag) == getiCode(INT_flag)) break;
	INTStack[i].FR = FR; INTStack[i].INT_flag = INT_flag; INTStack[i].IPC = R[37];
	if (!INTRoutine) INTRoutine = true; 
	else sortContext();
}
 
// returns last context by priority
context returnContext() {
	context aux;
	if (getFlag(INTStack[0].INT_flag)) {
		aux = INTStack[0];
		INT_flag = INTStack[0].INT_flag;
		INTStack[0].FR = 0x0; INTStack[0].INT_flag = std::make_tuple(false, 0, 0, 0);
	}
	sortContext();
	if (!getFlag(INTStack[0].INT_flag)) INTRoutine = false;
	return aux;
}

// interruptions manager
void INTManager(bool *okay) {
	std::stringstream result; uint_fast8_t OP;
	uint_fast32_t IR = memory[3];
	context context = returnContext();
	flagIsUP = false; R[36] = f_CR; 

	switch (f_INTCode) {
		case(0): SSOUT << "[HARDWARE INTERRUPTION 1]\n"; IR = memory[1]; break;
		case(1): SSOUT << "[SOFTWARE INTERRUPTION]\n"; break;
		case(2): SSOUT << "[HARDWARE INTERRUPTION 2]\n"; IR = memory[2]; break;
		case(3): SSOUT << "[INVALID INSTRUCTION @ " << getHexformat(R[32] << 2, 8) <<
			"]\n[SOFTWARE INTERRUPTION]\n"; break;
	}
	OP = (IR & 0xFC000000) >> 26;
	Watchdog();
	switch (getOPType(OP, okay)) {
		case 'U': OPType_U(OP, IR); break;
		case 'S': OPType_S(OP, IR, okay); break;
		case 'F': OPType_F(OP, IR); break;
	} 
	if (INTRoutine) {
		R[35] = context.FR; R[37] = context.IPC;
	}
}

// checks if CPU still alive?
void Watchdog() {
	if (watchdog) {
		if (TIMER > 0x0) TIMER--;
		else if (TIMER == 0x0 && IE) {
			watchdog = false; R[37] = R[32];
			INT_flag = std::make_tuple(true, 0, 0, 0xE1AC04DA);
			saveContext(R[35]);
		} 
	}
}

// float unity manager
void FPUManager() {
	if (Fpu.ciclos > 0)	--Fpu.ciclos;
	else if (Fpu.ciclos == 0 && IE) {
		Fpu.ciclos = -1; R[37] = R[32];
		INT_flag = std::make_tuple(true, 2, 2, 0x01EEE754);
		saveContext(R[35]);
	}
}

// executes instructions in memory
void ULA() {
	auto okay = true; 
	SSOUT << "[START OF SIMULATION]\n";

	for (R[32] = 0; okay; R[0] = 0) {
		R[33] = memory[R[32]];
		auto OP = (R[33] & 0xFC000000) >> 26;
		switch (getOPType(OP, okay)) {
			case ('U'): OPType_U(OP, R[33]); R[32]++; break;
			case ('F'): OPType_F(OP, R[33]); R[32]++; break;
			case ('S'): OPType_S(OP, R[33], &okay); break;
			default: break;
		}
		Watchdog(); FPUManager();
		if (flagIsUP)
			if (f_Priority <= 0 || IE) INTManager(&okay);
	}
	if (TERMINAL.tellp() > 0) SSOUT << "[TERMINAL]\n" << TERMINAL.str() << '\n';
	SSOUT << "[END OF SIMULATION]\n" << cacheStatistics();
}

// return register name indexing by number
std::string getRformat(uint64_t n, bool uppercase) {
	using namespace std;
	if ((n < 32) || (n > 37)) 
		return (uppercase) ? 'R' + to_string(n): 'r' + to_string(n);
	switch (n) {
		case (32): return (uppercase) ? "PC" : "pc";
		case (33): return (uppercase) ? "IR" : "ir";
		case (34): return (uppercase) ? "ER" : "er";
		case (35): return (uppercase) ? "FR" : "fr";
		case (36): return (uppercase) ? "CR" : "cr";
		case (37): return (uppercase) ? "IPC" : "ipc";
	}
}

// return string in hex format
std::string getHexformat(uint64_t r, int nzeros) {
	std::stringstream ssformated;
	ssformated << "0x" << std::hex << std::setfill('0') << std::uppercase << std::setw(nzeros) << r;
	return std::string(ssformated.str());
}

// all operations of type U
void OPType_U(uint_fast8_t OP, uint32_t instruction) {
	uint64_t z = (instruction & 0x7C00) >> 10, x = (instruction & 0x3E0) >> 5,
		y = (instruction & 0x1F); uint32_t e = (instruction & 0x38000) >> 15;
	if ((e & 0x4) >> 2) z = static_cast<uint64_t>((1 << 5) | z);
	if ((e & 0x2) >> 1) x = static_cast<uint64_t>((1 << 5) | x);
	if (e & 0x1) y = static_cast<uint64_t>((1 << 5) | y);
	auto temp = static_cast<uint64_t>(0);
	std::stringstream result;
	using namespace std;

	switch (OP) {
	case (0):
		if (x == 0 && y == 0 && z == 0) return;
		result << "add " << getRformat(z, false) << ", " << getRformat(x, false) << ", "
			<< getRformat(y, false) << '\n';
		temp = static_cast<uint64_t>(R[x] + R[y]);
		R[z] = temp & 0xFFFFFFFF;
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;		
		result << "[U] FR = " << getHexformat(R[35], 8) << ", " << getRformat(z, true) << " = " << getRformat(x, true)
			<< " + " << getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (2):
		result << "sub " << getRformat(z, false) << ", " << getRformat(x, false) << ", "
			<< getRformat(y, false) << '\n';
		temp = static_cast<uint64_t>(R[x] - R[y]);
		R[z] = temp & 0xFFFFFFFF;
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		result << "[U] FR = " << getHexformat(R[35], 8) << ", " << getRformat(z, true) << " = " << getRformat(x, true)
			<< " - " << getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (4):
		result << "mul " << getRformat(z, false) << ", " << getRformat(x, false) << ", "
			<< getRformat(y, false) << '\n';
		temp = static_cast<uint64_t>(R[x]) * R[y];
		R[z] = temp & 0xFFFFFFFF;
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		result << "[U] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " <<
			getRformat(z, true) << " = " << getRformat(x, true) << " * " << getRformat(y, true) << " = " <<
			getHexformat(R[z], 8);
		break;
	case (6):
		(R[y] != 0) ? (R[35] = R[35] & 0xF7) : (R[35] = R[35] | 0x8);
		result << "div " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		if (!(R[35] & 0x8)) { R[z] = R[x] / R[y]; R[34] = R[x] % R[y]; }
		else { R[34] = 0x0; R[37] = R[32] + 1;
			if (IE) { INT_flag = make_tuple(true, 1, -1, 0x1); saveContext(R[35]); }
		}
		result << "[U] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " <<
			getRformat(z, true) << " = " << getRformat(x, true) << " / " << getRformat(y, true) << " = "
			<< getHexformat(R[z], 8);
		if (SSOUT) {}
		break;
	case (8):
		result << "cmp " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		if (R[x] == R[y]) R[35] = R[35] | 0x1; else R[35] = R[35] & 0xFFFFFFFE;
		if (R[x] < R[y]) R[35] = R[35] | 0x2; else R[35] = R[35] & 0xFFFFFFFD;
		if (R[x] > R[y]) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[U] FR = " << getHexformat(R[35], 8);
		break;
	case (10):
		result << "shl " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << dec << y << '\n';
		temp = static_cast<uint64_t>(R[34] * 0x100000000);
		temp = static_cast<uint64_t>((temp | R[x]) << y + 1);
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		if (R[34] != 0) R[35] = R[35] | 0x4;
		result << "[U] ER = " << getHexformat(R[34], 8) << ", " << getRformat(z, true)
			<< " = " << getRformat(x, true) << " << " << dec << (y + 1) << " = " << getHexformat(R[z], 8);
		break;
	case (11):
		result << "shr " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << dec << y << '\n';
		temp = static_cast<uint64_t>(static_cast<uint64_t>(R[34]) << 32 | (0xFFFFFFFF & static_cast<uint64_t>(R[x])));
		temp = (temp >> (y + 1));
		R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		R[z] = temp & 0xFFFFFFFF;
		result << "[U] ER = " << getHexformat(R[34], 8) << ", " << getRformat(z, true) << " = " << getRformat(x, true)
			<< " >> " << dec << (y + 1) << " = " << getHexformat(R[z], 8);;
		break;
	case (12):
		result << "and " << getRformat(z, false) << ", " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		R[z] = R[x] & R[y];
		result << "[U] " << getRformat(z, true) << " = " << getRformat(x, true) << " & " << getRformat(y, true)
			<< " = " << getHexformat(R[z], 8);
		break;
	case (14):
		result << "not " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		R[x] = ~R[y];
		result << "[U] " << getRformat(x, true) << " = ~" << getRformat(y, true) << " = " << getHexformat(R[x], 8);
		break;
	case (16):
		result << "or " << getRformat(z, false) << ", " << getRformat(x, false) << ", " <<
			getRformat(y, false) << '\n';
		R[z] = R[x] | R[y];
		result << "[U] " << getRformat(z, true) << " = " << getRformat(x, true) << " | " <<
			getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (18):
		result << "xor " << getRformat(z, false) << ", " << getRformat(x, false) << ", " <<
			getRformat(y, false) << '\n';
		R[z] = R[x] ^ R[y];
		result << "[U] " << getRformat(z, true) << " = " << getRformat(x, true) << " ^ " <<
			getRformat(y, true) << " = " << getHexformat(R[z], 8);
		break;
	case (24):
		result << "push " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		(x < 32) ? memory[R[x]] = R[y]: 0;
		result << "[U] MEM[" << getRformat(x, true) << "--] = " << getRformat(y, true) << " = "
			<< getHexformat(memory[R[x]--], 8);
		break;
	case (25):
		result << "pop " << getRformat(x, false) << ", " << getRformat(y, false) << '\n';
		R[x] = memory[++R[y]];
		result << "[U] " << getRformat(x, true) << " = MEM[++" << getRformat(y, true) << "] = "
			<< getHexformat(R[x], 8);
	}
	if (result.tellp() > 0)  SSOUT << result.str() << '\n';
}

// all operations of type F
void OPType_F(uint_fast8_t OP, uint32_t instruction) {
	uint16_t IM16 = (instruction & 0x3FFFC00) >> 10;
	uint32_t x = (instruction & 0x3E0) >> 5, y = (instruction & 0x1F);
	auto temp = static_cast<uint64_t>(0);
	std::stringstream result;
	using namespace std;

	switch (OP) {
	case (1):
		result << "addi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		temp = static_cast<uint64_t>(R[y] + IM16);
		R[x] = (temp & 0xFFFFFFFF);
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		result << "[F] FR = " << getHexformat(R[35], 8) << ", " << getRformat(x, true) << " = " << getRformat(y, true)
			<< " + " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (3):
		result << "subi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		temp = static_cast<uint64_t>(R[y] - IM16);
		R[x] = temp & 0xFFFFFFFF;
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		result << "[F] FR = " << getHexformat(R[35], 8) << ", " << getRformat(x, true) << " = " << getRformat(y, true)
			<< " - " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (5):
		result << "muli " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		temp = static_cast<uint64_t>(R[y] * IM16);
		R[x] = temp & 0xFFFFFFFF;
		if (temp >= 0xFFFFFFFF) R[35] = R[35] | 0x10; else R[35] = R[35] & 0xFFFFFFEF;
		if (temp > 0xFFFFFFFF) R[34] = (temp & 0xFFFFFFFF00000000) >> 32;
		result << "[F] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " << getRformat(x, true)
			<< " = " << getRformat(y, true) << " * " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (7):
		(IM16 != 0) ? R[35] = R[35] & 0xF7 : R[35] = R[35] | 0x8;
		result << "divi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		if (R[35] & 0x7) { R[34] = R[y] % IM16; R[x] = R[y] / IM16; }
		else { R[34] = 0x0; R[37] = R[32] + 1;
			if (IE) { INT_flag = make_tuple(true, 1, -1, 0x1); saveContext(R[35]); }
		}
		result << "[F] FR = " << getHexformat(R[35], 8) << ", ER = " << getHexformat(R[34], 8) << ", " << getRformat(x, true)
			<< " = " << getRformat(y, true) << " / " << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (9):
		result << "cmpi " << getRformat(x, false) << ", " << IM16 << '\n';
		if (R[x] == IM16) R[35] = R[35] | 0x1; else R[35] = R[35] & 0xFFFFFFFE;
		if (R[x] < IM16) R[35] = R[35] | 0x2; else R[35] = R[35] & 0xFFFFFFFD;
		if (R[x] > IM16) R[35] = R[35] | 0x4; else R[35] = R[35] & 0xFFFFFFFB;
		result << "[F] FR = " << getHexformat(R[35], 8);
		break;
	case (13):
		result << "andi " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		R[x] = R[y] & IM16;
		result << "[F] " << getRformat(x, true) << " = " << getRformat(y, true) << " & " << getHexformat(IM16, 4) << " = "
			<< getHexformat(R[x], 8);
		break;
	case (15):
		result << "noti " << getRformat(x, false) << ", " << IM16 << '\n';
		R[x] = ~IM16;
		result << "[F] " << getRformat(x, true) << " = ~" << getHexformat(IM16, 4) << " = " << getHexformat(R[x], 8);
		break;
	case (17):
		result << "ori " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		R[x] = R[y] | IM16;
		result << "[F] " << getRformat(x, true) << " = " << getRformat(y, true) << " | " << getHexformat(IM16, 4)
			<< " = " << getHexformat(R[x], 8);
		break;
	case (19):
		result << "xori " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << IM16 << '\n';
		R[x] = R[y] ^ IM16;
		result << "[F] " << getRformat(x, true) << " = " << getRformat(y, true) << " ^ " << getHexformat(IM16, 4)
			<< " = " << getHexformat(R[x], 8);
		break;
	case (20):
		result << "ldw " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << getHexformat(IM16, 4) << '\n';
 		switch ((R[y] + IM16) << 2) {
		case (0x888B): R[x] = bufferTerminal; break;
		case (0x8800): R[x] = Fpu.X; break;
		case (0x8804): R[x] = Fpu.Y; break;
		case (0x8808): R[x] = Fpu.Z; break;
		case (0x880C): R[x] = Fpu.controle; break;
		default: R[x] = static_cast<uint64_t>(memory[(R[y] + IM16)]);
		}
		result << "[F] " << getRformat(x, true) << " = MEM[(" << getRformat(y, true) << " + " << getHexformat(IM16, 4)
			<< ") << 2]" << " = " << getHexformat(R[x], 8);
		break;
	case (21):
		result << "ldb " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << getHexformat(IM16, 4) << '\n';
		switch ((R[y] + IM16) >> 2) {
			case (0x8800): R[x] = Fpu.X; break;
			case (0x8804): R[x] = Fpu.Y; break;
			case (0x8808): R[x] = Fpu.Z; break;
			case (0x880C): R[x] = Fpu.controle; break;
			case (0x8888):
				switch ((R[y] + IM16) % 4) {
					case 3: R[x] = (memory[0x888B] & 0x000000FF); break;
					case 2: R[x] = (memory[0x888B] & 0x0000FF00) >> 8; break;
					case 1: R[x] = (memory[0x888B] & 0x00FF0000) >> 16; break;
					case 0: R[x] = (memory[0x888B] & 0xFF000000) >> 24; 
				} break;
			default:
				switch ((R[y] + IM16) % 4) {
					case 3: R[x] = (memory[(R[y] + IM16) >> 2] & 0x000000FF);	break;
					case 2: R[x] = (memory[(R[y] + IM16) >> 2] & 0x0000FF00) >> 8; break;
					case 1:	R[x] = (memory[(R[y] + IM16) >> 2] & 0x00FF0000) >> 16; break;
					case 0: R[x] = (memory[(R[y] + IM16) >> 2] & 0xFF000000) >> 24;
				} break;
		}
		result << "[F] " << getRformat(x, true) << " = MEM[" << getRformat(y, true) << " + " << getHexformat(IM16, 4) << "] = "
			<< getHexformat(R[x], 2);
		break;
	case (22):
		result << "stw " << getRformat(x, false) << ", " << getHexformat(IM16, 4) << ", " << getRformat(y, false) << '\n';
		switch ((R[x] + IM16) << 2) {
			case (0x8800): Fpu.Xf = static_cast<float>(R[y]); break;
			case (0x8804): Fpu.Yf = static_cast<float>(R[y]); break;
			case (0x8808): Fpu.Zf = static_cast<float>(R[y]); break;
			case (0x880C): Fpu.controle = R[y]; FPU(); break;
			case (0x888B): bufferTerminal = R[y] & 0x1F;
						   TERMINAL << static_cast<char>(bufferTerminal); break;
			case (0x8080): watchdog = R[y] & 0x80000000; TIMER = R[y] & 0x7FFFFFFF; break;
			default: memory[(R[x] + IM16)] = R[y];
		}
		result << "[F] MEM[(" << getRformat(x, true) << " + " << getHexformat(IM16, 4) << ") << 2]" << " = " << getRformat(y, true)
			<< " = " << getHexformat(R[y], 8);
		break;
	case (23):
		result << "stb " << getRformat(x, false) << ", " << getHexformat(IM16, 4) << ", " << getRformat(y, false) << '\n';
		switch (R[x] + IM16) {
		case (0x8800): Fpu.X = R[y]; break;
		case (0x8804): Fpu.Y = R[y]; break;
		case (0x8808): Fpu.Z = R[y]; break;
		case (0x880C): Fpu.controle = R[y]; FPU(); break;
		case (0x888B): if ((R[x] + IM16) % 4 == 3) {
			temp = (bufferTerminal & 0xFFFFFF00) | (R[y] & 0x000000FF);
			temp = temp & 0x000000FF;
			TERMINAL << static_cast<char>(temp);
		} break;
		default:
			switch ((R[x] + IM16) % 4) {
			case 3:
				memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0xFFFFFF00) | R[y];
				temp = memory[(R[x] + IM16) >> 2] & 0x000000FF; break;
			case 2:
				memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0xFFFF00FF) | (R[y] << 8);
				temp = (memory[(R[x] + IM16) >> 2] & 0x0000FF00) >> 8; break;
			case 1:
				memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0xFF00FFFF) | (R[y] << 16);
				temp = (memory[(R[x] + IM16) >> 2] & 0x00FF0000) >> 16; break;
			default:
				memory[(R[x] + IM16) >> 2] = (memory[(R[x] + IM16) >> 2] & 0x00FFFFFF) | (R[y] << 24);
				temp = (memory[(R[x] + IM16) >> 2] & 0xFF000000) >> 24;
			}
		}
		result << "[F] MEM[" << getRformat(x, true) << " + " << getHexformat(IM16, 4) << "] = " << getRformat(y, true)
			<< " = " << getHexformat(temp, 2);
		break;
	case (37):
		result << "call " << getRformat(x, false) << ", " << getRformat(y, false) << ", " << getHexformat(IM16, 4) << '\n';
		R[x] = ++R[32]; R[0] = 0x0; R[32] = R[y] + IM16;
		result << "[F] " << getRformat(x, true) << " = (PC + 4) >> 2 = " << getHexformat(R[x], 8) << ", PC = (" << getRformat(y, true)
			<< " + " << getHexformat(IM16, 4) << ") << 2 = " << getHexformat(R[32]-- << 2, 8);
		break;
	case (38):
		result << "ret " << getRformat(x, false) << '\n';
		R[32] = R[x];
		result << "[F] PC = " << getRformat(x, true) << " << 2 = " << getHexformat(R[32]-- << 2, 8);
		break;
	case (39):
		result << "isr " << getRformat(x, false) << ", "
			<< getRformat(y, false) << ", " << getHexformat(IM16, 4) << '\n';
		R[x] = R[37]; R[y] = R[36]; R[32] = IM16;
		result << "[F] " << getRformat(x, true) << " = IPC >> 2 = " << getHexformat(R[37], 8) <<
			", " << getRformat(y, true) << " = CR = " << getHexformat(R[36], 8) << ", PC = "
			<< getHexformat(R[32] << 2, 8);
		break;
	case (40):
		result << "reti " << getRformat(x, false) << '\n';
		R[32] = R[x];
		result << "[F] PC = " << getRformat(x, true) << " << 2 = " << getHexformat(R[32]-- << 2, 8);
		if (INTRoutine) { 
			sortContext(); context context = returnContext(); 
			R[35] = context.FR; R[37] = context.IPC;
		}
	}
	if (result.tellp() > 0)  SSOUT << result.str() << '\n';
}

// all operations of type S
void OPType_S(uint_fast8_t OP, uint32_t instruction, bool *okay) {
	auto IM26 = (instruction & 0x3FFFFFF); auto sucess = false;
	bool EQ = R[35] & 0x1, LT = R[35] & 0x2, GT = R[35] & 0x4,
		ZD = R[35] & 0x8, IV = R[35] & 0x20;
	std::stringstream result;
	using namespace std;

	switch (OP) {
	case (26):
		result << "bun "; sucess = true; break;
	case (27):
		result << "beq "; if (EQ) sucess = true; break;
	case (28):
		result << "blt "; if (LT) sucess = true; break;
	case (29):
		result << "bgt "; if (GT) sucess = true; break;
	case (30):
		result << "bne "; if (!EQ) sucess = true; break;
	case (31):
		result << "ble "; if (LT || EQ) sucess = true; break;
	case (32):
		result << "bge "; if (GT || EQ) sucess = true; break;
	case (33):
		result << "bzd "; if (ZD) sucess = true; break;
	case (34):
		result << "bnz "; if (!ZD) sucess = true; break;
	case (35):
		result << "biv "; if (IV) sucess = true; break;
	case (36):
		result << "bni "; if (!IV) sucess = true; break;
	default: 	
		if (OP == 63) {
			R[36] = IM26;
			if (IM26 == 0) { R[32] = 0x0; *okay = false; }
			else {
				R[37] = ++R[32]; R[32] = 0xC;
				INT_flag = make_tuple(true, 1, 1, IM26); 
				saveContext(R[35]);
			}
			result << "int " << R[36] << '\n' << "[S] CR = " << getHexformat(R[36], 8) <<
				", PC = " << getHexformat(R[32], 8);
			if (result.tellp() > 0)  SSOUT << result.str() << '\n';
			return;
		}
	}

	if (sucess) R[32] = IM26; else R[32]++;
	result << getHexformat(IM26, 8) << '\n' << "[S] PC = " << getHexformat(R[32] << 2, 8);
	if (result.tellp() > 0)  SSOUT << result.str() << '\n';
}

// all operations of fpu
void FPU() {
	auto status = true;
	auto *X_ = reinterpret_cast<uint32_t*>(&Fpu.Xf), 
		 *Y_ = reinterpret_cast<uint32_t*>(&Fpu.Yf),
		 *Z_ = reinterpret_cast<uint32_t*>(&Fpu.Zf);
	int32_t exp_x = (*X_ & 0x7F800000) >> 23, exp_y = (*Y_ & 0x7F800000) >> 23;
	uint32_t ciclos = abs(exp_x - exp_y) + 1; uint_fast8_t OP = Fpu.controle & 0xFFFFFF1F;

	switch(OP) {
		case 0: return;
		case 1: Fpu.Zf = Fpu.Xf + Fpu.Yf; break;
		case 2: Fpu.Zf = Fpu.Xf - Fpu.Yf; break;
		case 3: Fpu.Zf = Fpu.Xf * Fpu.Yf; break;
		case 4: if (Fpu.Yf == 0) status = false;
				else Fpu.Zf = Fpu.Xf / Fpu.Yf; break;
		case 5: Fpu.Xf = Fpu.Zf; break;
		case 6: Fpu.Yf = Fpu.Zf; break;
		case 7: Fpu.Z = ceil(Fpu.Zf); break;
		case 8: Fpu.Z = floor(Fpu.Zf); break;
		case 9: Fpu.Z = round(Fpu.Zf); break;
	default: 
		Fpu.ciclos = 1;
		Fpu.controle = 0x20;
		return;
	}
	Fpu.X = *X_; Fpu.Y = *Y_; 
	if (OP < 6) Fpu.Z = *Z_;
	Fpu.controle = status? 0x0: 0x20;
	Fpu.ciclos = (OP < 5)?  ciclos: 1;
}

// write out file
void WriteToFile(std::string outFileName) {
	using namespace std;
	ofstream file(outFileName.c_str(), ofstream::out);
	if (!file.is_open()) {
		cout << "Unable to write to file." << endl;
	}
	else {
		file << SSOUT.str();
		SSOUT.clear();
	}
	file.close();
}
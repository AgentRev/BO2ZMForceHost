#include <windows.h>
#include <vector>
#include <sstream>
#include <string>
using namespace std;

union dword
{
	DWORD i;
	float f;
};

const DWORD searchNamesBegin = 0x00A00000;
const DWORD searchNamesEnd = 0x00D00000;
const DWORD searchValuesBegin = 0x02980000;
const DWORD searchRawValBegin = 0x02900000;
const DWORD namesBufferSize = 0x10000;
const DWORD valuesBufferSize = 0x100000;

const DWORD searchScoreBegin = 0x02000000;
const DWORD searchScoreEnd = 0x03000000;

const char moneyVar[] = { 0xFF,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0 }; // Money signature
const char b93ammoVar[] = { 0x5A,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,2,0,0,0,1,0,0,0 }; // B23R pistol ammo signature

bool varsSet;
bool staticDone;

int playerNumbers = 4;

vector<string> vecNames;
vector< vector<dword> > vecValues; // Subvector index 0 = address once found, 1 = value

void AddVar(char name[], int value) { vecNames.push_back(name); vector<dword> oneVal(2); dword uVal; uVal.i = value; oneVal[1] = uVal; vecValues.push_back(oneVal); }
void AddVar(char name[], float value) { vecNames.push_back(name); vector<dword> oneVal(2); dword uVal; uVal.f = value; oneVal[1] = uVal; vecValues.push_back(oneVal); }

DWORD ProcID()
{
	DWORD pid;
	HWND hwnd = FindWindowA(NULL, "Call of Duty®: Black Ops II - Zombies");
	GetWindowThreadProcessId(hwnd, &pid);
	if (!pid) varsSet = false;
	return pid;
}

unsigned int VarIndex(string cvar)
{
	for (unsigned int i = 0; i < vecNames.size(); i++)
		if (vecNames[i].compare(cvar) == 0)
			return i;

	return 0;
}

DWORD FindOffset(DWORD varAdd, char* valuesBuffer)
{
	for (DWORD x = 0; x < valuesBufferSize; x += sizeof(DWORD))
		if (*(DWORD*)(valuesBuffer + x) == varAdd)
			return searchValuesBegin + x + 0x18;

	return 0;
}

void qosVals()
{
	DWORD pid = ProcID();

	if (pid)
	{
		int buffer1;
		int buffer2;

		HANDLE hProc = OpenProcess(PROCESS_VM_READ, false, pid);

		ReadProcessMemory(hProc, (void*)vecValues[VarIndex("qosMaxAllowedPing")][0].i, &buffer1, sizeof buffer1, NULL);
		ReadProcessMemory(hProc, (void*)vecValues[VarIndex("qosPreferredPing")][0].i, &buffer2, sizeof buffer2, NULL);
		printf("\nHigh: %i, Low: %i", buffer1, buffer2);

		CloseHandle(hProc);
	}
}

bool FindVars()
{
	DWORD pid = ProcID();

	if (pid)
	{
		char* namesBuffer = new char[namesBufferSize];
		char* valuesBuffer = new char[valuesBufferSize];

		HANDLE hProc = OpenProcess(PROCESS_VM_READ, false, pid);

		for (DWORD i = searchNamesBegin; i < searchNamesEnd; i += namesBufferSize)
		{
			ReadProcessMemory(hProc, (void*)i, namesBuffer, namesBufferSize, NULL);
			ReadProcessMemory(hProc, (void*)searchValuesBegin, valuesBuffer, valuesBufferSize, NULL);

			for (DWORD j = 0; j < namesBufferSize; j += sizeof(DWORD))
			{

				for (DWORD k = 0; k < vecNames.size(); k++)
				{
					if (!strcmp(namesBuffer + j, vecNames[k].c_str()))
					{
						dword uVal;
						uVal.i = FindOffset(i + j, valuesBuffer);
						vecValues[k][0] = uVal;
						break;
					}
				}
			}
		}

		delete[] namesBuffer;
		delete[] valuesBuffer;

		CloseHandle(hProc);
		return true;
	}

	return false;
}

DWORD FindScore(const char scoreVar[], size_t arrSize)
{
	DWORD pid = ProcID();
	DWORD scoreValAddy = 0;

	if (pid)
	{
		HANDLE hProc = OpenProcess(PROCESS_VM_READ, false, pid);

		DWORD buffSize = searchScoreEnd - searchScoreBegin;
		char* valuesBuffer = new char[buffSize];

		ReadProcessMemory(hProc, (void*)searchScoreBegin, valuesBuffer, buffSize, NULL);

		for (DWORD i = 0; i < buffSize; i += sizeof(DWORD))
		{
			if (!memcmp(valuesBuffer + i, scoreVar, arrSize))
			{
				scoreValAddy = searchScoreBegin + i + arrSize;
				break;
			}
		}

		delete[] valuesBuffer;

		CloseHandle(hProc);
	}

	return scoreValAddy;
}

int ReadScore(DWORD scoreValAddy)
{
	DWORD pid = ProcID();

	if (pid)
	{
		HANDLE hProc = OpenProcess(PROCESS_VM_READ, false, pid);

		int score = 0;

		ReadProcessMemory(hProc, (void*)scoreValAddy, &score, sizeof score, NULL);

		CloseHandle(hProc);

		return score;
	}

	return 0;
}

bool ModScore(DWORD scoreValAddy, int mod)
{
	DWORD pid = ProcID();

	if (pid && scoreValAddy)
	{
		HANDLE hProc = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, false, pid);

		int score = 0;

		ReadProcessMemory(hProc, (void*)scoreValAddy, &score, sizeof score, NULL);

		score += mod;

		WriteProcessMemory(hProc, (void*)scoreValAddy, &score, sizeof score, NULL);

		CloseHandle(hProc);
		return true;
	}

	return false;
}

void WriteVals()
{
	DWORD pid = ProcID();

	if (pid)
	{
		HANDLE hProc = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, false, pid);

		for (DWORD i = 0; i < vecValues.size(); i++)
			WriteProcessMemory(hProc, (void*)vecValues[i][0].i, &vecValues[i][1].i, sizeof(DWORD), NULL);

		CloseHandle(hProc);
	}
}
void WriteStaticVals()
{
	DWORD pid = ProcID();

	if (pid)
	{
		int nbFound = 0;
		char* valuesBuffer = new char[valuesBufferSize];
		HANDLE hProc = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, false, pid);

		ReadProcessMemory(hProc, (void*)searchRawValBegin, valuesBuffer, valuesBufferSize, NULL);
		
		const char dvarNames[] = "party_minplayers 2;party_maxplayers 4;";

		for (DWORD j = 0; j < valuesBufferSize; j += 1)
		{
			if (!strncmp(valuesBuffer + j, dvarNames, sizeof dvarNames - 1))
			{
				stringstream value;
				value << "party_minplayers " << playerNumbers << ";party_maxplayers " << playerNumbers << ";";

				string svalue = value.str();

				WriteProcessMemory(hProc, (void*)(searchRawValBegin + j),  svalue.c_str(), svalue.length(), NULL);
				nbFound++;
			}
		}

		delete[] valuesBuffer;

		staticDone = true;
		CloseHandle(hProc);
	}
}

void main()
{
	SetConsoleTitleA("BO2 ZM DVar Modder");
	printf("Press PAUSE to exit.\n"
	       "Locating dvars...");

	//AddVar("g_compassShowEnemies", true); // false
	//AddVar("migration_forceHost", true); // -1
	//AddVar("cl_connectionAttempts", 0); // 10
	//AddVar("reliableTimeoutTime", 5000); // 20000
	//AddVar("sv_timeout", 15); // 20
	//AddVar("cl_migrationPingTime", 0); //10

	//AddVar("party_minplayers", 1); // 4-18

	//AddVar("terriblePing", 3); // 300
	//AddVar("goodPing", 2); // 150
	//AddVar("excellentPing", 1); // 75

	//AddVar("terriblePing", 180); // 300
	//AddVar("goodPing", 120); // 150
	//AddVar("excellentPing", 60); // 75

	//AddVar("party_mergingEnabled", false); // true
	//AddVar("party_mergingJitter", 1); // 15000
	//AddVar("partyMigrate_disabled", false); // false
	//AddVar("cl_maxppf", 0); // 5

	//AddVar("cl_timeout", 20.f); // 20.f

	//AddVar("sessionSearchMaxAttempts", 1); // 5

	AddVar("cg_fov", 90.f); // 65.f - 80.f

	//AddVar("cl_connectTimeout", 30.f); // 90.f

	//AddVar("migration_forceHost", true); // -1
	AddVar("qosMaxAllowedPing", 1); // 300
	AddVar("qosPreferredPing", 1); // 75

	//AddVar("sv_connectTimeout", 30); // 90

	AddVar("party_minplayers", playerNumbers); // 2
	AddVar("party_maxplayers", playerNumbers); // 4

	//AddVar("sv_privateClients", playerNumbers);
	//AddVar("sv_privateClientsForClients", playerNumbers);

	//AddVar("sv_maxclients", playerNumbers); // 4

	//AddVar("com_maxclients", playerNumbers); // 4

	//AddVar("party_playerCount", 1);
	//AddVar("sv_reconnectlimit", 10);
	//AddVar("sv_timeout", 30); // 20

	varsSet = FindVars();

	printf("\nRunning.");

	DWORD moneyAddy = 0;
	DWORD b93ammoAddy = 0;

	while (true)
	{
		if (GetAsyncKeyState(VK_PAUSE))
			break;


		if (GetAsyncKeyState(VK_MULTIPLY))
		{
			moneyAddy = FindScore(moneyVar, sizeof moneyVar);
			printf("\n%X : %i", moneyAddy, ReadScore(moneyAddy));
		}

		if (GetAsyncKeyState(VK_ADD))
			ModScore(moneyAddy, 1000);

		if (GetAsyncKeyState(VK_SUBTRACT))
			ModScore(moneyAddy, -1000);


		if (GetAsyncKeyState(VK_DIVIDE))
		{
			b93ammoAddy = FindScore(b93ammoVar, sizeof b93ammoVar);
			printf("\n%X : %i", b93ammoAddy, ReadScore(b93ammoAddy)); 
		}

		if (GetAsyncKeyState(VK_NUMPAD8))
			ModScore(b93ammoAddy, 99);

		if (GetAsyncKeyState(VK_NUMPAD9))
			ModScore(b93ammoAddy, -33);


		if (!varsSet)
			varsSet = FindVars();
		else
			WriteVals();

		Sleep(250);
	}
}
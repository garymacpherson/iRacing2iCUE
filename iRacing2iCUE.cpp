#pragma warning(disable:4996) //_CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <math.h>

#include "irsdk_defines.h"
#include "irsdk_client.h"
#include "yaml_parser.h"
#include "iRacing2iCUE.h"
#include "iCUEHandler.cpp"

HANDLE hDataValidEvent = NULL;

iCUEHandler iCUE;

irsdkCVar g_SessionFlags("SessionFlags"); // (int) irsdk_Flags, bitfield

const int g_maxCars = 64;
const int g_maxNameLen = 64;

struct DriverEntry
{
	int carIdx;
	int carClassId;
	char driverName[g_maxNameLen];
	char teamName[g_maxNameLen];
	char carNumStr[10]; // the player car number as a character string so we can handle 001 and other oddities
};

// updated for each driver as they cross the start/finish line
DriverEntry g_driverTableTable[g_maxCars];

bool init()
{
	// trap ctrl-c
	signal(SIGINT, ex_program);

	// bump priority up so we get time from the sim
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	// ask for 1ms timer so sleeps are more precise
	timeBeginPeriod(1);

	// startup event broadcaster
	hDataValidEvent = CreateEvent(NULL, true, false, IRSDK_DATAVALIDEVENTNAME);

	//****Note, put your init logic here

	return true;
}

void deInit()
{
	printf("Shutting down.\n\n");

	// shutdown
	if (hDataValidEvent)
	{
		//make sure event not left triggered (probably redundant)
		ResetEvent(hDataValidEvent);
		CloseHandle(hDataValidEvent);
		hDataValidEvent = NULL;
	}

	timeEndPeriod(1);
}

void ex_program(int sig)
{
	(void)sig;

	printf("recieved ctrl-c, exiting\n\n");

	timeEndPeriod(1);

	signal(SIGINT, SIG_DFL);
	exit(0);
}

bool processYAMLLiveString()
{
	static DWORD lastTime = 0;
	bool wasUpdated = false;

	//****Note, your code goes here
	// can write to disk, parse, etc

	// output file once every 1 seconds
	DWORD minTime = (DWORD)(1.0f * 1000);
	DWORD curTime = timeGetTime(); // millisecond resolution
	if (abs((long long)(curTime - lastTime)) > minTime)
	{
		lastTime = curTime;

		const char* yamlStr = generateLiveYAMLString();
		// validate string
		if (yamlStr && yamlStr[0])
		{
			FILE* f = fopen("liveStr.txt", "w");
			if (f)
			{
				fputs(yamlStr, f);
				fclose(f);
				f = NULL;
				wasUpdated = true;
			}
		}
	}

	return wasUpdated;
}

const char* generateLiveYAMLString()
{
	//****Warning, shared static memory!
	static const int m_len = 50000;
	static char tstr[m_len] = "";
	int len = 0;

	// Start of YAML file
	len += _snprintf(tstr + len, m_len - len, "---\n");

	len += _snprintf(tstr + len, m_len - len, " SessionFlags: %d\n", g_SessionFlags.getInt()); // irsdk_Flags, bitfield
	len += _snprintf(tstr + len, m_len - len, "\n");

	// End of YAML file
	len += _snprintf(tstr + len, m_len - len, "...\n");

	// terminate string in case we blew off the end of the array.
	tstr[m_len - 1] = '\0';

	// make sure we are not close to running out of room
	// if this triggers then double m_len
	assert(len < (m_len - 256));

	return tstr;
}

// called only when it changes
void processYAMLSessionString(const char* yamlStr)
{
	// validate string
	if (yamlStr && yamlStr[0])
	{
		FILE* f = fopen("sessionStr.txt", "w");
		if (f)
		{
			fputs(yamlStr, f);
			fclose(f);
			f = NULL;
		}

		//---

		// Pull some driver info into a local array

		char tstr[256];
		for (int i = 0; i < g_maxCars; i++)
		{
			// skip the rest if carIdx not found
			sprintf(tstr, "DriverInfo:Drivers:CarIdx:{%d}", i);
			if (parseYamlInt(yamlStr, tstr, &(g_driverTableTable[i].carIdx)))
			{
				sprintf(tstr, "DriverInfo:Drivers:CarIdx:{%d}CarClassID:", i);
				parseYamlInt(yamlStr, tstr, &(g_driverTableTable[i].carClassId));

				sprintf(tstr, "DriverInfo:Drivers:CarIdx:{%d}UserName:", i);
				parseYamlStr(yamlStr, tstr, g_driverTableTable[i].driverName, sizeof(g_driverTableTable[i].driverName) - 1);

				sprintf(tstr, "DriverInfo:Drivers:CarIdx:{%d}TeamName:", i);
				parseYamlStr(yamlStr, tstr, g_driverTableTable[i].teamName, sizeof(g_driverTableTable[i].teamName) - 1);

				sprintf(tstr, "DriverInfo:Drivers:CarIdx:{%d}CarNumber:", i);
				parseYamlStr(yamlStr, tstr, g_driverTableTable[i].carNumStr, sizeof(g_driverTableTable[i].carNumStr) - 1);

				// TeamID
			}
		}


		//---

		//****Note, your code goes here
		// can write to disk, parse, etc

	}
}

bool parseYamlInt(const char* yamlStr, const char* path, int* dest)
{
	if (dest)
	{
		(*dest) = 0;

		if (yamlStr && path)
		{
			int count;
			const char* strPtr;

			if (parseYaml(yamlStr, path, &strPtr, &count))
			{
				(*dest) = atoi(strPtr);
				return true;
			}
		}
	}

	return false;
}

bool parseYamlStr(const char* yamlStr, const char* path, char* dest, int maxCount)
{
	if (dest && maxCount > 0)
	{
		dest[0] = '\0';

		if (yamlStr && path)
		{
			int count;
			const char* strPtr;

			if (parseYaml(yamlStr, path, &strPtr, &count))
			{
				// strip leading quotes
				if (*strPtr == '"')
				{
					strPtr++;
					count--;
				}

				int l = min(count, maxCount);
				strncpy(dest, strPtr, l);
				dest[l] = '\0';

				// strip trailing quotes
				if (l >= 1 && dest[l - 1] == '"')
					dest[l - 1] = '\0';

				return true;
			}
		}
	}

	return false;
}

void run()
{
	// wait up to 16 ms for start of session or new data
	if (irsdkClient::instance().waitForData(16))
	{
		bool wasUpdated = false;

		if (processYAMLLiveString())
			wasUpdated = true;

		// only process session string if it changed
		if (irsdkClient::instance().wasSessionStrUpdated())
		{
			processYAMLSessionString(irsdkClient::instance().getSessionStr());
			wasUpdated = true;
		}

		// notify clients
		if (wasUpdated && hDataValidEvent)
			PulseEvent(hDataValidEvent);

		updateDisplay();
	}
	// else we did not grab data, do nothing

	// pump our connection status
	monitorConnectionStatus();

	//****Note, add your own additional loop processing here
	// for anything not dependant on telemetry data (keeping a UI running, etc)
}

void updateDisplay()
{
	/*printf(" flags: ");
	printFlags(g_SessionFlags.getInt());
	printf("\n");*/
	iCUE.updateiCUE(getFlagColour(g_SessionFlags.getInt()));
}

char getFlagColour(int flags)
{
	// organised by priority
	// eg. basically nothing matters if checkered is already dropped
	// starting lights have priority over everything

	if (flags & irsdk_startHidden) {
		return 'k';
	};
	if (flags & irsdk_startReady) {
		return 'k';
	};
	if (flags & irsdk_startSet) {
		return 'r';
	}
	if (flags & irsdk_startGo) {
		return 'k';
	};
	if (flags & irsdk_green) {
		return 'g';
	};
	if (flags & irsdk_blue) {
		return 'b';
	};
	if (flags & irsdk_red) {
		return 'r';
	};
	if (flags & irsdk_yellow) {
		return 'y';
	};
	if (flags & irsdk_yellowWaving) {
		return 'y';
	};
	if (flags & irsdk_debris) {
		return 'y';
	};
	if (flags & irsdk_caution) {
		return 'y';
	};
	if (flags & irsdk_cautionWaving) {
		return 'y';
	};
	if (flags & irsdk_white) {
		return 'w';
	};
	if (flags & irsdk_black) {
		return 'k';
	};
	if (flags & irsdk_repair) {
		return 'k';
	};
	if (flags & irsdk_disqualify) {
		return 'k';
	};

	// global flags
	//if (flags & irsdk_checkered) printf("checkered ");
	//if (flags & irsdk_crossed) printf("crossed ");
	//if (flags & irsdk_oneLapToGreen) printf("oneLapToGreen ");
	//if (flags & irsdk_greenHeld) printf("greenHeld ");
	//if (flags & irsdk_tenToGo) printf("tenToGo ");
	//if (flags & irsdk_fiveToGo) printf("fiveToGo ");
	//if (flags & irsdk_randomWaving) printf("randomWaving ");
	
	//// drivers black flags
	//if (flags & irsdk_servicible) printf("servicible ");
	//if (flags & irsdk_furled) printf("furled ");

	// start lights
}

void monitorConnectionStatus()
{
	// keep track of connection status
	static bool wasConnected = false;

	bool isConnected = irsdkClient::instance().isConnected();
	if (wasConnected != isConnected)
	{
		if (isConnected)
		{
			printf("Connected to iRacing              \n");
		}
		else
			printf("Lost connection to iRacing        \n");

		//****Note, put your connection handling here

		wasConnected = isConnected;
	}
}

int main(int argc, char* argv[])
{
	printf("iRacing2iCUE, press any key to exit\n");

	if (init())
	{
		while (!_kbhit())
		{
			run();
		}

		deInit();
	}
	else
		printf("init failed\n");

	return 0;
}

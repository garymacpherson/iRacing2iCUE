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

void run()
{
	if (irsdkClient::instance().waitForData(16))
	{
		updateDisplay();
	}

	monitorConnectionStatus();
}

void updateDisplay()
{
	printFlags(g_SessionFlags.getInt());
	char flagColour = getFlagColour(g_SessionFlags.getInt());
	iCUE.updateiCUE(flagColour);

}

void printFlags(int flags)
{
	// global flags
	if (flags & irsdk_checkered) printf("checkered ");
	if (flags & irsdk_white) printf("white ");
	if (flags & irsdk_green) printf("green ");
	if (flags & irsdk_yellow) printf("yellow ");
	if (flags & irsdk_red) printf("red ");
	if (flags & irsdk_blue) printf("blue ");
	if (flags & irsdk_debris) printf("debris ");
	if (flags & irsdk_crossed) printf("crossed ");
	if (flags & irsdk_yellowWaving) printf("yellowWaving ");
	if (flags & irsdk_oneLapToGreen) printf("oneLapToGreen ");
	if (flags & irsdk_greenHeld) printf("greenHeld ");
	if (flags & irsdk_tenToGo) printf("tenToGo ");
	if (flags & irsdk_fiveToGo) printf("fiveToGo ");
	if (flags & irsdk_randomWaving) printf("randomWaving ");
	if (flags & irsdk_caution) printf("caution ");
	if (flags & irsdk_cautionWaving) printf("cautionWaving ");

	// drivers black flags
	if (flags & irsdk_black) printf("black ");
	if (flags & irsdk_disqualify) printf("disqualify ");
	if (flags & irsdk_servicible) printf("servicible ");
	if (flags & irsdk_furled) printf("furled ");
	if (flags & irsdk_repair) printf("repair ");

	// start lights
	if (flags & irsdk_startHidden) printf("startHidden ");
	if (flags & irsdk_startReady) printf("startReady ");
	if (flags & irsdk_startSet) printf("startSet ");
	if (flags & irsdk_startGo) printf("startGo ");
	printf("\n");
}

char getFlagColour(int flags)
{
	// organised by priority
	// eg. basically nothing matters if checkered is already dropped
	// starting lights have priority over everything

	//if (flags & irsdk_startHidden) {
	//	return 'k';
	//};
	if (flags & irsdk_startReady) {
		return 'k';
	};
	if (flags & irsdk_startSet) {
		return 'r';
	}
	if (flags & irsdk_startGo) {
		return 'g';
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
	return 'k';

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
	printf("iRacingFlags2LED, press any key to exit\n");

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

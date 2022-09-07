#pragma warning(disable:4996) //_CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <conio.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>
#include <locale>

#pragma comment(lib, "Ws2_32.lib")

#include "irsdk_defines.h"
#include "irsdk_client.h"
#include "iRacingFlagsRGB.h"
#include "base64.h"


HANDLE hDataValidEvent = NULL;

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

	std::string disableColourControl = base64_encode("{\"colours\":\"default\",\"animate\":false}");
	setLEDColour(disableColourControl);

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
	std::string disableColourControl = base64_encode("{\"colours\":\"default\",\"animate\":false}");
	setLEDColour(disableColourControl);

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
	const char* flagColour = getFlagColour(g_SessionFlags.getInt());
	std::string foo = base64_encode(flagColour);
	setLEDColour(foo);
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

const char* getFlagColour(int flags)
{
	// animated colours should be in sets of 2 or 6.
	// static colours can be in 2s or 3s

	if (flags & irsdk_startSet) {
		return "{\"colours\":[\"red\"],\"animate\":false}";
	}
	if (flags & irsdk_startGo) {
		return "{\"colours\":[\"green\",\"black\",\"green\",\"black\",\"green\",\"black\"],\"animate\":true}";
	};
	if (flags & irsdk_green) {
		return "{\"colours\":[\"green\"],\"animate\":false}";
	};
	if (flags & irsdk_blue) {
		return "{\"colours\":[\"blue\",\"yellow\",\"blue\"],\"animate\":false}";
	};
	if (flags & irsdk_red) {
		return "{\"colours\":[\"red\"],\"animate\":false}";
	};
	if (flags & irsdk_yellow) {
		return "{\"colours\":[\"yellow\"],\"animate\":false}";
	};
	if (flags & irsdk_yellowWaving) {
		return "{\"colours\":[\"yellow\",\"white\"],\"animate\":true}";
	};
	if (flags & irsdk_debris) {
		return "{\"colours\":[\"yellow\"],\"animate\":false}";
	};
	if (flags & irsdk_caution) {
		return "{\"colours\":[\"yellow\"],\"animate\":false}";
	};
	if (flags & irsdk_cautionWaving) {
		return "{\"colours\":[\"yellow\",\"white\"],\"animate\":true}";
	};
	if (flags & irsdk_white) {
		return "{\"colours\":[\"white\"],\"animate\":false}";
	};
	if (flags & irsdk_black) {
		return "{\"colours\":[\"white\"],\"animate\":false}";
	};
	if (flags & irsdk_repair) {
		return "{\"colours\":[\"black\",\"orange\",\"black\"],\"animate\":true}";
	};
	if (flags & irsdk_disqualify) {
		return "{\"colours\":[\"black\",\"red\",\"black\"],\"animate\":true}";
	};
	return "{\"colours\":[\"black\"],\"animate\":false}";

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
	if (wasConnected != isConnected) {
		if (isConnected) {
			printf("Connected to iRacing              \n");
		}
		else {
			printf("Lost connection to iRacing        \n");
			std::string disableColourControl = base64_encode("{\"colours\":\"default\",\"animate\":false}");
			setLEDColour(disableColourControl);
		}

		wasConnected = isConnected;
	}
}

std::string website_HTML;
std::locale local;
std::string currentColour;
char buffer[10000];
int i = 0;

void setLEDColour(std::string base64ColourJSONObject) {
	WSADATA wsaData;
	SOCKET Socket;
	SOCKADDR_IN SockAddr;
	struct hostent* host;
	std::string get_http;

	if (base64ColourJSONObject == currentColour) {
		return;
	}

	get_http = "GET /canvas/event?sender=iRacing&event=" + base64ColourJSONObject + " HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cout << "WSAStartup failed.\n";
		return;
	}

	Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	host = gethostbyname("localhost");

	SockAddr.sin_port = htons(16034);
	SockAddr.sin_family = AF_INET;
	SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

	if (connect(Socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0) {
		std::cout << "Could not connect";
		return;
	}
	send(Socket, get_http.c_str(), strlen(get_http.c_str()), 0);

	int nDataLength;
	while ((nDataLength = recv(Socket, buffer, 10000, 0)) > 0) {
		int i = 0;
		while (buffer[i] >= 32 || buffer[i] == '\n' || buffer[i] == '\r') {

			website_HTML += buffer[i];
			i += 1;
		}
	}

	closesocket(Socket);
	WSACleanup();

	currentColour = base64ColourJSONObject;
}

int main(int argc, char* argv[])
{
	printf("iRacingFlagsRGB, press any key to exit\n");

	std::string defaultColour = base64_encode("{\"colours\":[\"black\"],\"animate\":false}");
	setLEDColour(defaultColour);

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

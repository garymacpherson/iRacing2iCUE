#pragma once
#include <string>

void ex_program(int sig);

void updateDisplay();

void printFlags(int flags);

const char* getFlagColour(int flags);

void monitorConnectionStatus();

void setLEDColour(std::string base64ColourJSONObject);
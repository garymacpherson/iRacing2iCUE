#pragma once

void ex_program(int sig);

void updateDisplay();

void printFlags(int flags);

const char* getFlagColour(int flags);

void monitorConnectionStatus();

void setLEDColour(const char* colour);

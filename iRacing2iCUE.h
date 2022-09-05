#pragma once

bool init();

void deInit();

void ex_program(int sig);

bool processYAMLLiveString();

const char* generateLiveYAMLString();

bool parseYamlInt(const char* yamlStr, const char* path, int* dest);

bool parseYamlStr(const char* yamlStr, const char* path, char* dest, int maxCount);

void updateDisplay();

void printFlags(int flags);

void monitorConnectionStatus();

#pragma once

void GetEnvironmentVariablePath(const char *varName, char *value, const bool isError);
void copy_file(const char *oDstFilePath, const char *oSrcFilePath);
long get_file_size(const char *filename);
char* load_file(const char *filename);
void getLowerExtention(const char *filename, char *lowext, char *filetype);

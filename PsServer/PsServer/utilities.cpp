#include "utilities.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>

void GetEnvironmentVariablePath (const char *varName, char *value, const bool isError)
{
	char *path = getenv(varName);
	if (!path)
	{
		if (isError)
			printf("Error: Environment variable: %s not found.\n", varName);
		else
			printf("Environment variable: %s not found.\n", varName);
		return;
	}

	strcat(value, path);

	int lastId = strlen(path) - 1;
	
	if ('\\' != path[lastId])
		strcat(value, "\\");

}

void copy_file(const char *oDstFilePath, const char *oSrcFilePath)
{
	std::ifstream ifstr(oDstFilePath, std::ios::binary);
	std::ofstream ofstr(oSrcFilePath, std::ios::binary);

	ofstr << ifstr.rdbuf();
}

long get_file_size(const char *filename)
{
	FILE *fp;

	fp = fopen(filename, "rb");
	if (fp)
	{
		long size;

		if ((0 != fseek(fp, 0, SEEK_END)) || (-1 == (size = ftell(fp))))
			size = 0;

		fclose(fp);

		return size;
	}
	else
		return 0;
}

char* load_file(const char *filename)
{
	FILE *fp;
	char *buffer;
	long size;

	size = get_file_size(filename);
	if (0 == size)
		return NULL;

	fp = fopen(filename, "rb");
	if (!fp)
		return NULL;

	buffer = (char*)malloc(size + 1);
	if (!buffer)
	{
		fclose(fp);
		return NULL;
	}
	buffer[size] = '\0';

	if (size != (long)fread(buffer, 1, size, fp))
	{
		free(buffer);
		buffer = NULL;
	}

	fclose(fp);
	return buffer;
}

bool is_char_num(const char *str)
{
	int i = 0;
	while (str[i] != '\0') {
		if (str[i] < '0'|| str[i] > '9')
			return false;
		i++;
	}

	return true;
}

char lower(char c) {
	if ('A' <= c && c <= 'Z') {
		c = c + ('a' - 'A');
	}
	return c;
}

void lowerstring(const char *in, char *out)
{
	int i = 0;
	while (in[i] != '\0') {
		out[i] = lower(in[i]);
		i++;
	}
	out[i] = '\0';
}

void get_creo_ext(const char *filename, size_t numlen, char *creoext, char *filetype)
{
	const char *exExts[] = { ".prt.", ".asm.", ".neu." };

	char lowFlename[_MAX_PATH];
	lowerstring(filename, lowFlename);

	for (const char* ext : exExts)
	{
		const char *dotExt = strstr(filename, ext);
		if (NULL != dotExt)
		{
			size_t len = strlen(dotExt);
			if (len == strlen(ext) + numlen)
			{
				strcpy(creoext, (char*)&dotExt[1]);

				size_t typeLen = strlen(ext) - 2;
				for (int i = 0; i < typeLen; i++)
				{
					filetype[i] = ext[i + 1];
				}
				filetype[typeLen] = '\0';

				return;
			}

		}
	}
}

void getLowerExtention(const char *filename, char *lowext, char *filetype) {
	const char *dotExt = strrchr(filename, '.');

	char ext[256];
	strcpy(ext, (char*)&dotExt[1]);

	if (is_char_num(ext))
	{
		// Pro/E, Creo
		char creoExt[256] = { '\0' };
		get_creo_ext(filename, strlen(ext), creoExt, filetype);

		if (strlen(creoExt))
			lowerstring(creoExt, lowext);
		
	}
	else
	{
		lowerstring(ext, lowext);
		strcpy(filetype, lowext);
	}
}
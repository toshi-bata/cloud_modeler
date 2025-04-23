#pragma once
#include "A3DSDKIncludes.h"
#include "parasolid_kernel.h"

class CExProcess
{
public:
	CExProcess();
	~CExProcess();

private:

public:
	bool Init();
	void Terminate();
	int LoadFile(const char* file_name, int &iPartCnt, PK_PART_t* &parts);
	int ExportFile(const int n_parts, const PK_PART_t* parts, const char *filePath);
};


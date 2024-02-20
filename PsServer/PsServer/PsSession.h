#pragma once
#include "parasolid_kernel.h"
#include <vector>
#include <map>
#include <string>

class CPsSession
{
public:
	CPsSession(const char *workingDir);
	~CPsSession();

private:
	// for undo/redo
	PK_PARTITION_t	m_defaultPartition = PK_ENTITY_null;
	PK_PARTITION_t	m_currentPartition = PK_ENTITY_null;

	// Multi session support
	const char* m_pcWorkingDir;
	struct markDic {
		std::vector<PK_MARK_t> marks;
		int iCurrentMark;
	};
	std::map <std::string, markDic> m_psMarks;
	std::string m_activeSession;

public:
	bool Init();
	void Terminate();

	static void StartFrustrum(int * ifail);
	static void StopFrustrum(int * ifail);
	static void GetMemory(int * nBytes, char * * memory, int * ifail);
	static void ReturnMemory(int * nBytes, char * * memory, int * ifail);
	static PK_ERROR_code_t PKerrorHandler(PK_ERROR_sf_t* error);
	void CheckAndHandleErrors();
	int Reset();

	// for undo/redo
	PK_PARTITION_t GetPkPartition() { return m_currentPartition; };
	int SetPsMark();
	int Undo();
	int Redo();

	// Multi session support
	int CreateNewSession(const char *activeSession);
	int DumpActiveSession();
	int LoadBackSession(std::string session);
	int GotoTheLastMark();
};


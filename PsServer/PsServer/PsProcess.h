#pragma once
#include "parasolid_kernel.h"
#include "PsSession.h"
#include "ExProcess.h"

#include <vector>

class CPsProcess
{
public:
	CPsProcess(const char *workingDir);
	~CPsProcess();

private:
	const double m_dScale = 1000.0;
	const char* m_pcWorkingDir;
	CPsSession* m_pPsSession;
	char m_pcFilePathForDL[_MAX_PATH] = { '\0' };

	struct leafInfo {
		PK_BODY_t body;
		PK_TRANSF_sf_t netMatrix;
	};
	std::map<PK_INSTANCE_t, leafInfo> m_bodyInstance_netMatrix;

	std::vector<float> PsSolidToFloatArray(const PK_BODY_t body);
	int LoadPsFile(const char* fileName, const char* fileType, int &iCnt, PK_PART_t* &parts);
	double getFaceArea(const PK_FACE_t face);
	void setDefaultMatrix(PK_TRANSF_sf_t &transf_sf);
	void setMatrixToArr(const PK_TRANSF_sf_t transf_sf, std::vector<float> &floatArray);
	void setBasisSet(const double *offset, const double *dir, const double *axis, PK_AXIS2_sf_s &basis_set);
	PK_BODY_t createDummyBody();
	int getInstanceCount(const PK_ENTITY_t target);
	PK_ERROR_code_t transformBody(const PK_BODY_t body, PK_TRANSF_t transf);
	PK_ASSEMBLY_t findTopAssy();
	void traverseInstance(PK_PART_t part, PK_TRANSF_sf_t currentMatrix, char* treeData);
	int createIstance(const PK_BODY_t body);

public:
	bool Init();
	void Terminate();

	int Undo() { return m_pPsSession->Undo(); };
	int Redo() { return m_pPsSession->Redo(); };
	int SaveCAD(std::string activeSession, const char cFormat);
	void Downloaded();
	int Reset() { return m_pPsSession->Reset(); };

	void LoadFile(const char* activeSession, const char* fileName, const char* fileType, char* treeData);
	std::vector<float> RequestBody(const PK_ENTITY_t entity);
	std::vector<float> CreateBlock(const double *size, const double *offset, const double *dir, const double *axis);
	std::vector<float> CreateCylinder(const double rad, const double height, const double *offset, const double *dir, const double *axis);
	std::vector<float> CreatePrism(const double rad, const double height, const int num, const double *offset, const double *dir, const double *axis);
	std::vector<float> CreateCone(const double topR, const double bottomR, const double height, const double *offset, const double *dir, const double *axis);
	std::vector<float> CreateTorus(const double majorR, const double minorR, const double *offset, const double *dir, const double *axis);
	std::vector<float> CreateSphere(const double rad, const double *offset);
	std::vector<float> Blend(const char cType, const double size, const int edgeCnt, const PK_EDGE_t *edges);
	std::vector<float> Hollow(const double thisckness, const PK_ENTITY_t entity, const int faceCnt, const PK_FACE_t *pierceFaces);
	std::vector<float> Offset(const double value, const PK_ENTITY_t entity, const int faceCnt, const PK_FACE_t *offsetFaces);
	std::vector<float> ImprintRo(const double dOffset, const int edgeCnt, const PK_EDGE_t *edges);
	std::vector<float> ImprintFace(const PK_FACE_t targetFace, const int faceCnt, const PK_FACE_t *toolFaces);
	std::vector<float> Boolean(const char cType, const PK_ENTITY_t targetEntity, const int toolCnt, const PK_ENTITY_t* toolEntities);
	std::vector<float> CopyFace(const int faceCnt, const PK_ENTITY_t* faces);
	std::vector<float> DeleteFace(const int faceCnt, const PK_ENTITY_t* faces);
	int AlignZ(const int faceCnt, const PK_ENTITY_t *faces);
	int DeleteEntity(const PK_ENTITY_t entity);
	int SetTransform(const PK_ENTITY_t entity, const PK_TRANSF_sf_t transf_sf);

	std::vector<float> MassProps(const PK_BODY_t body);
	std::vector<float> FR_Holes(const double dMaxDia, const PK_BODY_t body);
	std::vector<float> FR_Concaves(const double dMinAng, const double dMaxAng, const PK_BODY_t body);
	std::vector<float> EdgeInfo(const PK_EDGE_t edge);

	void ComputeCollision(const int i_target, std::vector<float>& collisionArray);
	void ComputeCollision(std::vector<float>& collisionArray);
	std::vector<float> ComputeSilhouette(const double *ray, const double *pos);

	// Multi session support
	int BodySave(PK_BODY_t body, std::string activeSession);
	int CreateNewSession(const char *activeSession) { return m_pPsSession->CreateNewSession(activeSession); };
	int DumpActiveSession() { return m_pPsSession->DumpActiveSession(); };
	int LoadBackSession(std::string session) { return m_pPsSession->LoadBackSession(session); };
	int GotoTheLastMark() { return m_pPsSession->GotoTheLastMark(); };
};


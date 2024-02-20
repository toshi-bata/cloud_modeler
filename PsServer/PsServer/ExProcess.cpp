#include "stdafx.h"

#define INITIALIZE_A3D_API
#include "ExProcess.h"

CExProcess::CExProcess()
{
	Init();
}

CExProcess::~CExProcess()
{
	Terminate();
}

bool CExProcess::Init()
{
	m_pHoopsExchangeLoader = new A3DSDKHOOPSExchangeLoader(_T(""));

	if (A3D_SUCCESS == m_pHoopsExchangeLoader->m_eSDKStatus)
	{
		printf("    HOOPS Exchanged Loaded\n");

		return true;
	}
	else
	{
		printf("    HOOPS Exchanged loading Failed\n");
		return false;
	}
}

void CExProcess::Terminate()
{
	if (A3D_SUCCESS == A3DDllTerminate())
	{
		A3DSDKUnloadLibrary();
		printf("    HOOPS Exchanged Unloaded\n");
	}

}

int CExProcess::LoadFile(const char* file_name, int &iPartCnt, PK_PART_t* &parts)
{
	A3DStatus status;

	A3DImport sImport(file_name);

	// export to parasolid with the bridge 
	sImport.m_sLoadData.m_sSpecifics.m_sParasolid.m_bKeepParsedEntities = true;
	sImport.m_sLoadData.m_sSpecifics.m_sStep.m_bHealOrientations = true;
	sImport.m_sLoadData.m_sGeneral.m_eReadGeomTessMode = kA3DReadGeomAndTess;
	sImport.m_sLoadData.m_sTessellation.m_eTessellationLevelOfDetail = kA3DTessLODMedium;

	status = m_pHoopsExchangeLoader->Import(sImport);
	if (status != A3D_SUCCESS && status != A3D_LOAD_MISSING_COMPONENTS)
		return status;

	A3DMiscPKMapper* pPkMapper = NULL;

	A3DRWParamsExportParasolidData sExportOptions;
	sExportOptions.m_bBinary = false;
	sExportOptions.m_bBStrictAssemblyStructure = false;
	sExportOptions.m_bExplodeMultiBodies = false;
	sExportOptions.m_bMakePointsWithCoordinateSystems = false;
	sExportOptions.m_bSaveSolidsAsFaces = false;
	A3D_INITIALIZE_DATA(A3DRWParamsExportParasolidData, sExportOptions);

	A3DRWParamsTranslateToPkPartsData sParams;
	A3D_INITIALIZE_DATA(A3DRWParamsTranslateToPkPartsData, sParams);
	sParams.m_pMapper = &pPkMapper;

	A3DAsmModelFile* pModelFile = m_pHoopsExchangeLoader->m_psModelFile;

	status = A3DAsmModelFileTranslateToPkParts(pModelFile, &sExportOptions, &sParams, &iPartCnt, &parts);

	return status;
}

int CExProcess::ExportFile(const int n_parts, const PK_PART_t *parts, const char* filePath)
{
	A3DStatus status;
	
	// Parasolid => Exchange
	A3DAsmModelFile* pModelFile;
	A3DRWParamsLoadData sRWParams;
	A3DMiscPKMapper *pPkMapper;
	A3D_INITIALIZE_DATA(A3DRWParamsLoadData, sRWParams);
	sRWParams.m_sGeneral.m_bReadSolids = true;
	sRWParams.m_sGeneral.m_bReadSurfaces = true;

	status = A3DPkPartsTranslateToA3DAsmModelFile(n_parts, (int*)parts, &sRWParams, &pModelFile, &pPkMapper);
	
	m_pHoopsExchangeLoader->m_psModelFile = pModelFile;
	
	status = m_pHoopsExchangeLoader->Export(A3DExport(filePath));

	return status;
}


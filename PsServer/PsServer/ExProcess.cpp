#include "stdafx.h"

#define INITIALIZE_A3D_API
#include "ExProcess.h"
#include "hoops_license.h"

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
	A3DStatus iRet;
	
	wchar_t bin_dir[2048];
	swprintf(bin_dir, _T(""));
	wprintf(_T("Exchange bin dir=\"%s\"\n"), bin_dir);
	if (!A3DSDKLoadLibrary(bin_dir))
		return false;

	iRet = A3DLicPutUnifiedLicense(HOOPS_LICENSE);
	if (iRet != A3D_SUCCESS)
		return false;

	A3DInt32 iMajorVersion = 0, iMinorVersion = 0;
	iRet = A3DDllGetVersion(&iMajorVersion, &iMinorVersion);
	if (iRet != A3D_SUCCESS)
		return false;

	iRet = A3DDllInitialize(A3D_DLL_MAJORVERSION, A3D_DLL_MINORVERSION);
	if (iRet != A3D_SUCCESS)
		return iRet;

	if (iRet == A3D_SUCCESS)
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

	// Init import options
	A3DRWParamsLoadData sLoadData;
	A3D_INITIALIZE_DATA(A3DRWParamsLoadData, sLoadData);
	sLoadData.m_sGeneral.m_bReadSolids = true;
	sLoadData.m_sGeneral.m_bReadSurfaces = true;
	sLoadData.m_sGeneral.m_bReadWireframes = true;
	sLoadData.m_sGeneral.m_bReadPmis = true;
	sLoadData.m_sGeneral.m_bReadAttributes = true;
	sLoadData.m_sGeneral.m_bReadHiddenObjects = false;
	sLoadData.m_sGeneral.m_bReadConstructionAndReferences = false;
	sLoadData.m_sGeneral.m_bReadActiveFilter = false;
	sLoadData.m_sGeneral.m_eReadingMode2D3D = kA3DRead_3D;
	sLoadData.m_sGeneral.m_eReadGeomTessMode = kA3DReadGeomAndTess;
	sLoadData.m_sGeneral.m_eDefaultUnit = kA3DUnitUnknown;
	sLoadData.m_sTessellation.m_eTessellationLevelOfDetail = kA3DTessLODMedium;
	sLoadData.m_sAssembly.m_bUseRootDirectory = true;
	sLoadData.m_sMultiEntries.m_bLoadDefault = true;
	sLoadData.m_sPmi.m_bAlwaysSubstituteFont = false;
	sLoadData.m_sPmi.m_pcSubstitutionFont = (char*)"Myriad CAD";
	sLoadData.m_sSpecifics.m_sParasolid.m_bKeepParsedEntities = true;
	sLoadData.m_sSpecifics.m_sStep.m_bHealOrientations = true;

	A3DAsmModelFile * pModelFile;
	status = A3DAsmModelFileLoadFromFile(file_name, &sLoadData, &pModelFile);
	
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

	A3DRWParamsExportPrcData sExportData;
	A3D_INITIALIZE_DATA(A3DRWParamsExportPrcData, sExportData);

	A3DAsmModelFileExportToPrcFile(pModelFile, &sExportData, filePath, NULL);

	return status;
}


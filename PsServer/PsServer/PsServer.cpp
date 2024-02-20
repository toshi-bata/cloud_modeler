// PsServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>
#include <sys/types.h>
#include <microhttpd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <vector>
#include "PsProcess.h"
#include <string>
#include <algorithm>
#include <time.h>
#include <map>
#include "utilities.h"

#define MAX_PARAM_SIZE 256
#define POSTBUFFERSIZE 1024

std::string ACTIVE_SESSION;
std::map<std::string, time_t> active_sessions; 
std::map<std::string, std::string> _params;

static char *_port;
static char _pcWorkingDir[_MAX_PATH] = { '\0' };
static CPsProcess* _pPsProcess;

struct connection_info_struct
{
	int connectiontype;
	char *sessionId;
	char *filePath;
	char *fileType;
	FILE * pFile;
	struct MHD_PostProcessor *postprocessor;
}; 

#define GET             0
#define POST            1


void deleteWorkingFiles(std::string session)
{
	{
		char filePath[_MAX_PATH];
		sprintf_s(filePath, _MAX_PATH, "%s%s.snp_txt", _pcWorkingDir, session.c_str());
		remove(filePath);
	}
	{
		char filePath[_MAX_PATH];
		sprintf_s(filePath, _MAX_PATH, "%s%s.X_T", _pcWorkingDir, session.c_str());
		remove(filePath);
	}
	{
		char filePath[_MAX_PATH];
		sprintf_s(filePath, _MAX_PATH, "%s%s.sfr", _pcWorkingDir, session.c_str());
		remove(filePath);
	}
	{
		char filePath[_MAX_PATH];
		sprintf_s(filePath, _MAX_PATH, "%s%s.tet", _pcWorkingDir, session.c_str());
		remove(filePath);
	}
	{
		char filePath[_MAX_PATH];
		sprintf_s(filePath, _MAX_PATH, "%s%s.bdf", _pcWorkingDir, session.c_str());
		remove(filePath);
	}
	{
		char filePath[_MAX_PATH];
		sprintf_s(filePath, _MAX_PATH, "%s%s.op2", _pcWorkingDir, session.c_str());
		remove(filePath);
	}
}

template <typename List>
void split(const std::string& s, const std::string& delim, List& result)
{
	result.clear();

	using string = std::string;
	string::size_type pos = 0;

	while (pos != string::npos)
	{
		string::size_type p = s.find(delim, pos);

		if (p == string::npos)
		{
			result.push_back(s.substr(pos));
			break;
		}
		else {
			result.push_back(s.substr(pos, p - pos));
		}

		pos = p + delim.size();
	}
}

int sendResponseSuccess(struct MHD_Connection *connection)
{
	struct MHD_Response *response;
	int ret = MHD_NO;

	response = MHD_create_response_from_buffer(0, (void*)NULL, MHD_RESPMEM_MUST_COPY);
	MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return ret;
}

int sendResponseText(struct MHD_Connection *connection, const char* responseText)
{
	struct MHD_Response *response;
	int ret = MHD_NO;

	response = MHD_create_response_from_buffer(strlen(responseText), (void*)responseText, MHD_RESPMEM_PERSISTENT);

	MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return ret;
}

int sendResponseFloatArr(struct MHD_Connection *connection, std::vector<float> &floatArray)
{
	struct MHD_Response *response;
	int ret = MHD_NO;

	if (0 < floatArray.size())
	{
		response = MHD_create_response_from_buffer(floatArray.size() * sizeof(float), (void*)&floatArray[0], MHD_RESPMEM_MUST_COPY);
		std::vector<float>().swap(floatArray);
	}
	else
	{
		return MHD_NO;
	}

	MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*");

	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);

	return ret;
}

static int
iterate_post(void *coninfo_cls, enum MHD_ValueKind kind, const char *key,
	const char *filename, const char *content_type,
	const char *transfer_encoding, const char *data,
	uint64_t off, size_t size)
{
	struct connection_info_struct *con_info = (connection_info_struct*)coninfo_cls;

	if ((size > 0) && (size <= MAX_PARAM_SIZE))
	{
		if (0 == strcmp(key, "sessionId"))
		{
			char *session;
			session = (char*)malloc(MAX_PARAM_SIZE);
			if (!session) return MHD_NO;

			sprintf_s(session, MAX_PARAM_SIZE, "%s", data);
			con_info->sessionId = session;
		}
		else
			_params.insert(std::make_pair(std::string(key), std::string(data)));

	}

	return MHD_YES;
}

bool paramStrToStr(const char *key, std::string &sVal)
{
	sVal = _params[std::string(key)];
	if (sVal.empty()) return false;

	return true;
}

bool paramStrToInt(const char *key, int &iVal)
{
	std::string sVal = _params[std::string(key)];
	if (sVal.empty()) return false;

	iVal = std::atoi(sVal.c_str());

	return true;
}

bool paramStrToDbl(const char *key, double &dVal)
{
	std::string sVal = _params[std::string(key)];
	if (sVal.empty()) return false;

	dVal = std::atof(sVal.c_str());

	return true;
}

bool paramStrToXYZ(const char *key, double *&dXYZ)
{
	std::string sVal = _params[std::string(key)];
	if (sVal.empty()) return false;

	std::vector<std::string> strArr;
	split(sVal.c_str(), ",", strArr);

	int entCnt = strArr.size();

	if (3 != entCnt)
		return false;

	dXYZ = new double[entCnt];

	dXYZ[0] = std::atof(strArr[0].c_str());
	dXYZ[1] = std::atof(strArr[1].c_str());
	dXYZ[2] = std::atof(strArr[2].c_str());

	return true;
}

bool paramStrToChr(const char *key, char &cha)
{
	std::string sVal = _params[std::string(key)];
	if (sVal.empty()) return false;

	cha = sVal.c_str()[0];

	return true;
}

bool paramStrToPkEntities(const char *key, int &entCnt, PK_ENTITY_t *&entities)
{
	std::string sVal = _params[std::string(key)];
	if (sVal.empty()) return true;

	std::vector<std::string> strArr;
	split(sVal.c_str(), ",", strArr);

	entCnt = strArr.size();

	if (0 == entCnt)
		return false;

	entities = new PK_ENTITY_t[entCnt];

	for (int i = 0; i < entCnt; i++)
		entities[i] = std::atoi(strArr[i].c_str());

	return true;
}

bool paramStrToPkTransf(const char* key, PK_TRANSF_sf_t &transf_sf)
{
	std::string sVal = _params[std::string(key)];
	if (sVal.empty()) return false;

	std::vector<std::string> strArr;
	split(sVal.c_str(), ",", strArr);

	int cnt = strArr.size();

	if (16 != cnt) return false;

	for (int ii = 0; ii < 4; ii++)
	{
		for (int jj = 0; jj < 4; jj++)
		{
			double el = std::atof(strArr[ii * 4 + jj].c_str());

			if (3 > jj && 3 == ii)
				el /= 1000;

			transf_sf.matrix[jj][ii] = el;
		}
	}

	return true;
}

int answer_to_connection(void *cls, struct MHD_Connection *connection,
	const char *url,
	const char *method, const char *version,
	const char *upload_data,
	size_t *upload_data_size, void **con_cls)
{

	printf("\n--- New %s request for %s using version %s\n", method, url, version);

	// Update active session's time
	if (0 < active_sessions.count(ACTIVE_SESSION))
		active_sessions[ACTIVE_SESSION] = time(0);

	std::vector<std::string> cmds;
	split(url, "/", cmds);

	if (NULL == *con_cls)
	{
		struct connection_info_struct *con_info;
		con_info = (connection_info_struct*)malloc(sizeof(struct connection_info_struct));
		if (NULL == con_info) return MHD_NO;
		con_info->sessionId = NULL;
		con_info->pFile = 0;

		// If the new request is a POST, the postprocessor must be created now.
		// In addition, the type of the request is stored for convenience.
		if (0 == strcmp(method, "POST"))
		{
			con_info->postprocessor
				= MHD_create_post_processor(connection, POSTBUFFERSIZE,
				(MHD_PostDataIterator)iterate_post, (void*)con_info);

			if (NULL == con_info->postprocessor)
			{
				// File drop

				//Session ID should be used to ensure the proper context is being modified.
				const char *pcSession = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "session_id");
				if (NULL == pcSession)
					return MHD_NO;

				printf("    File Upload Initiated\n");

				char lowExt[256];
				char fileType[256];
				getLowerExtention((char*)&url[1], lowExt, fileType);

				char filePath[_MAX_PATH];
				sprintf_s(filePath, sizeof(filePath), "%s%s.%s", _pcWorkingDir, pcSession, lowExt);
				
				con_info->pFile = fopen(filePath, "wb");
				con_info->filePath = (char*)malloc(sizeof(char) * strlen(filePath));
				con_info->fileType = (char*)malloc(sizeof(char) * strlen(fileType));
				strcpy(con_info->filePath, filePath);
				strcpy(con_info->fileType, fileType);
			}
			con_info->connectiontype = POST;
		}
		else con_info->connectiontype = GET;

		*con_cls = (void*)con_info;
		return MHD_YES;
	}

	if (strcmp(method, "POST") == 0)
	{
		struct connection_info_struct *con_info = (connection_info_struct*)*con_cls;

		if (*upload_data_size != 0)
		{
			if (NULL != con_info->postprocessor)
				MHD_post_process(con_info->postprocessor, upload_data, *upload_data_size);
			else
			{
				printf("    Processing file chunk\n");
				fwrite(upload_data, 1, *upload_data_size, con_info->pFile);
			}
			*upload_data_size = 0;

			return MHD_YES;
		}
		else if (NULL != con_info->pFile)
		{
			// File uploaded
			fclose(con_info->pFile);

			// Import file
			char* filename = con_info->filePath;
			char* filetype = con_info->fileType;
			
			printf("    File uploaded:%s\n", filename);
			
			char treeData[51200];
			_pPsProcess->LoadFile(ACTIVE_SESSION.c_str(), filename, filetype, treeData);

			// Remove the uploaded file
			remove(filename);

			return sendResponseText(connection, treeData);
		}
		else if (NULL != con_info->sessionId)
		{
			printf("Session ID      : %s\n", con_info->sessionId);
			printf("Previous Session: %s\n", ACTIVE_SESSION.c_str());

			// IF WE ARE CHANGING SESSIONS - Save current and load old
			if (ACTIVE_SESSION != std::string(con_info->sessionId))
			{
				// Dump active session to File.
				if (0 < active_sessions.count(ACTIVE_SESSION))
				{
					_pPsProcess->GotoTheLastMark();
					_pPsProcess->DumpActiveSession();
				}

				if (0 < active_sessions.count(std::string(con_info->sessionId)))
				{
					// If new session was previously active - load it back in.
					if (0 != _pPsProcess->LoadBackSession(std::string(con_info->sessionId)))
						return MHD_NO;
				}

				ACTIVE_SESSION = std::string(con_info->sessionId);
			}

			// Commnds
			if (0 == strcmp(url, "/Create"))
			{
				// Create a new Session
				active_sessions[ACTIVE_SESSION] = time(0);
				_pPsProcess->CreateNewSession(con_info->sessionId);
				printf("    Created a new Session\n");

				return sendResponseSuccess(connection);
			}
			else if (0 == strcmp(url, "/Reset"))
			{
				deleteWorkingFiles(con_info->sessionId);

				if (0 == _pPsProcess->Reset())
					return sendResponseSuccess(connection);
				else
					return MHD_NO;
			}
			else if (0 == strcmp(url, "/RequestBody"))
			{
				int body;
				if (!paramStrToInt("body", body)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->RequestBody(body);

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (0 == strcmp(url, "/CreateSolid"))
			{
				char cShape;
				double offset[3], dir[3], axis[3];
				std::vector<float> floatArray;

				if (!paramStrToChr("shape", cShape)) return MHD_NO;
				if (!paramStrToDbl("xOff", offset[0])) return MHD_NO;
				if (!paramStrToDbl("yOff", offset[1])) return MHD_NO;
				if (!paramStrToDbl("zOff", offset[2])) return MHD_NO;
				if (!paramStrToDbl("xDir", dir[0])) return MHD_NO;
				if (!paramStrToDbl("yDir", dir[1])) return MHD_NO;
				if (!paramStrToDbl("zDir", dir[2])) return MHD_NO;
				if (!paramStrToDbl("xAxis", axis[0])) return MHD_NO;
				if (!paramStrToDbl("yAxis", axis[1])) return MHD_NO;
				if (!paramStrToDbl("zAxis", axis[2])) return MHD_NO;

				switch (cShape)
				{
				case 'B':
				{
					double size[3];
					if (!paramStrToDbl("xSize", size[0])) return MHD_NO;
					if (!paramStrToDbl("ySize", size[1])) return MHD_NO;
					if (!paramStrToDbl("zSize", size[2])) return MHD_NO;


					floatArray = _pPsProcess->CreateBlock(size, offset, dir, axis);
				} break;
				case 'Y':
				{
					double rad, height;
					if (!paramStrToDbl("r", rad)) return MHD_NO;
					if (!paramStrToDbl("h", height)) return MHD_NO;

					floatArray = _pPsProcess->CreateCylinder(rad, height, offset, dir, axis);
				} break;
				case 'P':
				{
					double rad, height;
					int num;
					if (!paramStrToDbl("r", rad)) return MHD_NO;
					if (!paramStrToDbl("h", height)) return MHD_NO;
					if (!paramStrToInt("n", num)) return MHD_NO;

					floatArray = _pPsProcess->CreatePrism(rad, height, num, offset, dir, axis);
				} break;
				case 'C':
				{
					double topR, bottomR, height;
					if (!paramStrToDbl("topR", topR)) return MHD_NO;
					if (!paramStrToDbl("bottomR", bottomR)) return MHD_NO;
					if (!paramStrToDbl("h", height)) return MHD_NO;

					floatArray = _pPsProcess->CreateCone(topR, bottomR, height, offset, dir, axis);
				} break;
				case 'T':
				{
					double majorR, minorR;
					if (!paramStrToDbl("majorR", majorR)) return MHD_NO;
					if (!paramStrToDbl("minerR", minorR)) return MHD_NO;

					floatArray = _pPsProcess->CreateTorus(majorR, minorR, offset, dir, axis);
				} break;
				case 'S':
				{
					double rad;
					if (!paramStrToDbl("r", rad)) return MHD_NO;

					floatArray = _pPsProcess->CreateSphere(rad, offset);
				} break;
				default:
					break;
				}

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (0 == strcmp(url, "/Blend"))
			{
				char cType;
				double size;
				int edgeCnt = 0;
				PK_ENTITY_t *edges = NULL;
				if (!paramStrToChr("type", cType)) return MHD_NO;
				if (!paramStrToDbl("size", size)) return MHD_NO;
				if (!paramStrToPkEntities("entities", edgeCnt, edges)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->Blend(cType, size, edgeCnt, edges);
				
				delete[] edges;

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (0 == strcmp(url, "/Boolean"))
			{
				char cType;
				PK_ENTITY_t targetEntity;
				int toolCnt;
				PK_ENTITY_t* toolEntities = NULL;
				if (!paramStrToChr("type", cType)) return MHD_NO;
				if (!paramStrToInt("targetEntity", targetEntity)) return MHD_NO;
				if (!paramStrToPkEntities("toolEntities", toolCnt, toolEntities)) return MHD_NO;
				
				std::vector<float> floatArray = _pPsProcess->Boolean(cType, targetEntity, toolCnt, toolEntities);

				delete[] toolEntities;

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (0 == strcmp(url, "/Hollow"))
			{
				double thickness;
				int faceCnt = 0;
				PK_FACE_t* pierceFaces = NULL;
				PK_ENTITY_t targetEntity;
				if (!paramStrToDbl("thickness", thickness)) return MHD_NO;
				if (!paramStrToPkEntities("pierceFaces", faceCnt, pierceFaces)) return MHD_NO;
				if (!paramStrToInt("targetEntity", targetEntity)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->Hollow(thickness, targetEntity, faceCnt, pierceFaces);

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (0 == strcmp(url, "/Offset"))
			{
				double value;
				int faceCnt = 0;
				PK_FACE_t* offsetFaces = NULL;
				PK_ENTITY_t targetEntity;
				if (!paramStrToDbl("value", value)) return MHD_NO;
				if (!paramStrToPkEntities("offsetFaces", faceCnt, offsetFaces)) return MHD_NO;
				if (!paramStrToInt("targetEntity", targetEntity)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->Offset(value, targetEntity, faceCnt, offsetFaces);

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (0 == strcmp(url, "/ImprintRo"))
			{
				double dOffset;
				int edgeCnt;
				PK_ENTITY_t *edges = NULL;
				if (!paramStrToDbl("offset", dOffset)) return MHD_NO;
				if (!paramStrToPkEntities("entities", edgeCnt, edges)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->ImprintRo(dOffset, edgeCnt, edges);

				delete[] edges;

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (0 == strcmp(url, "/ImprintFace"))
			{
				PK_FACE_t targetFace;
				int faceCnt;
				PK_ENTITY_t *toolFaces;
				if (!paramStrToInt("target", targetFace)) return MHD_NO;
				if (!paramStrToPkEntities("entities", faceCnt, toolFaces)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->ImprintFace(targetFace, faceCnt, toolFaces);

				delete[] toolFaces;

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (0 == strcmp(url, "/CopyFace"))
			{
				int faceCnt;
				PK_ENTITY_t* faces;
				if (!paramStrToPkEntities("entities", faceCnt, faces)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->CopyFace(faceCnt, faces);

				delete[] faces;

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (0 == strcmp(url, "/DeleteFace"))
			{
				int faceCnt;
				PK_ENTITY_t *faces;
				if (!paramStrToPkEntities("entities", faceCnt, faces)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->DeleteFace(faceCnt, faces);

				delete[] faces;

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (strcmp(url, "/DeleteBody") == 0)
			{
				int body;
				if (!paramStrToInt("body", body)) return MHD_NO;

				if (0 == _pPsProcess->DeleteEntity(body))
					return sendResponseSuccess(connection);
				else
					return MHD_NO;
			}
			else if (strcmp(url, "/Undo") == 0)
			{
				if (0 == _pPsProcess->Undo())
					return sendResponseSuccess(connection);
				else
					return MHD_NO;
			}
			else if (strcmp(url, "/Redo") == 0)
			{
				if (0 == _pPsProcess->Redo())
					return sendResponseSuccess(connection);
				else
					return MHD_NO;
			}
			else if (strcmp(url, "/MassProps") == 0)
			{
				int body;
				if (!paramStrToInt("body", body)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->MassProps(body);

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (strcmp(cmds[1].c_str(), "EdgeInfo") == 0)
			{
				PK_EDGE_t edge;
				if (!paramStrToInt("edge", edge)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->EdgeInfo(edge);

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (strcmp(url, "/FR_Holes") == 0)
			{
				double dMaxDia;
				PK_BODY_t body;
				if (!paramStrToDbl("size", dMaxDia)) return MHD_NO;
				if (!paramStrToInt("body", body)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->FR_Holes(dMaxDia, body);

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (strcmp(url, "/FR_Concaves") == 0)
			{
				double dMinAng;
				double dMaxAng;
				PK_BODY_t body;
				if (!paramStrToDbl("min", dMinAng)) return MHD_NO;
				if (!paramStrToDbl("max", dMaxAng)) return MHD_NO;
				if (!paramStrToInt("body", body)) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->FR_Concaves(dMinAng, dMaxAng, body);

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (0 == strcmp(url, "/CheckCollision"))
			{
				int target;
				if (!paramStrToInt("target", target)) return MHD_NO;


				std::vector<float> collisionArray;
				collisionArray.push_back(-1);

				if (-1 == target)
					_pPsProcess->ComputeCollision(collisionArray);
				else
					_pPsProcess->ComputeCollision(target, collisionArray);

				return sendResponseFloatArr(connection, collisionArray);
			}
			else if (0 == strcmp(url, "/Silhouette"))
			{
				double ray[3], pos[3];
				if (!paramStrToDbl("xRay", ray[0])) return MHD_NO;
				if (!paramStrToDbl("yRay", ray[1])) return MHD_NO;
				if (!paramStrToDbl("zRay", ray[2])) return MHD_NO;

				if (!paramStrToDbl("xPos", pos[0])) return MHD_NO;
				if (!paramStrToDbl("yPos", pos[1])) return MHD_NO;
				if (!paramStrToDbl("zPos", pos[2])) return MHD_NO;

				std::vector<float> floatArray = _pPsProcess->ComputeSilhouette(ray, pos);

				return sendResponseFloatArr(connection, floatArray);
			}
			else if (0 == strcmp(url, "/Transform"))
			{
				PK_ENTITY_t entity;
				PK_TRANSF_sf_t transf_sf;
				if (!paramStrToInt("entity", entity)) return MHD_NO;
				if (!paramStrToPkTransf("matrix", transf_sf)) return MHD_NO;

				if (0 == _pPsProcess->SetTransform(entity, transf_sf))
					return sendResponseSuccess(connection);
				else
					return MHD_NO;
			}
			else if (0 == strcmp(url, "/DownloadCAD"))
			{
				char cForm;
				if (!paramStrToChr("format", cForm)) return MHD_NO;

				int iRet = _pPsProcess->SaveCAD(con_info->sessionId, cForm);

				if (0 == iRet)
					return sendResponseSuccess(connection);
				else
					return MHD_NO;
			}
			else if (0 == strcmp(url, "/Downloaded"))
			{
				_pPsProcess->Downloaded();

				return sendResponseSuccess(connection);
			}
		}
	}
	else if (strcmp(method, "GET") == 0)
	{
		return sendResponseSuccess(connection);
	}

	return MHD_NO;
}

void request_completed(void *cls, struct MHD_Connection *connection,
	void **con_cls,
	enum MHD_RequestTerminationCode toe)
{
	struct connection_info_struct *con_info = (connection_info_struct *)*con_cls;

	if (NULL == con_info) return;
	if (con_info->connectiontype == POST)
	{
		if (NULL != con_info->postprocessor)
			MHD_destroy_post_processor(con_info->postprocessor);

		con_info->filePath = NULL;
		con_info->fileType = NULL;

		if (_params.size())
		{
			_params.clear();
			std::map<std::string, std::string>(_params).swap(_params);
		}
	}

	free(con_info);
	*con_cls = NULL;
}


int main(int argc, char ** argv)
{
	if (argc != 2) {
		printf("%s PORT\n",
			argv[0]);
		return 1;
	}

	_port = argv[1];
	printf("PORT: %s\n", _port);

	// Get working dir
	GetEnvironmentVariablePath("PS_SERVER_WORKING_DIR", _pcWorkingDir, true);
	if (0 == strlen(_pcWorkingDir))
		return 1;

	_pPsProcess = new CPsProcess(_pcWorkingDir);

	struct MHD_Daemon *daemon;
	
	daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, atoi(argv[1]), NULL, NULL,
		(MHD_AccessHandlerCallback)&answer_to_connection, NULL,
		MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL,
		MHD_OPTION_END);

	getchar();

	MHD_stop_daemon(daemon);
	printf("MHD_stop_daemon");
	
	delete _pPsProcess;

	printf("Terminate");

	return 0;

}


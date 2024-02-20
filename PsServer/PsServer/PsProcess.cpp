#include "stdafx.h"
#include "PsProcess.h"
#include "utilities.h"
#include <iterator>
#include "ps_utilities.h"

#define PI 3.14159265359

bool isPsFile(const char *lowExt)
{
	const char *psExts[] = { "x_t", "xmt_txt", "x_b", "xmt_bin" };

	for (const char* ext : psExts)
	{
		if (0 == strcmp(lowExt, ext))
			return true;
	}
	return false;
}

bool isExFile(const char *lowExt)
{
	const char *exExts[] = {
		"sat", "sab", "ipt", "iam", "model", "dlv", "exp", "session", "CATPart", "CATProduct", "CATShape","cgr",
		"3dxml", "dwg", "asm", "neu", "prt", "xas", "xpr", "arc", "unv", "mf1", "pkg",
		"ifc",	"ifczip", "igs", "iges", "jt", "pdf", "prc", "3dm", "pwd", "psm",
		"par", "sldprt", "sldasm", "stp", "step", "stpz", "vda"};

	for (const char* ext : exExts)
	{
		if (0 == strcmp(lowExt, ext))
			return true;
	}
	return false;
}


PK_TRANSF_sf_t multiplyMatrix(PK_TRANSF_sf_t& matrix1, PK_TRANSF_sf_t& matrix2)
{
	PK_TRANSF_sf_t result;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			result.matrix[i][j] = 0.;
			for (int k = 0; k < 4; k++)
				result.matrix[i][j] += matrix1.matrix[i][k] * matrix2.matrix[k][j];
		}
	}
	return result;
}

CPsProcess::CPsProcess(const char *workingDir)
	:m_pcWorkingDir(workingDir)
{
	Init();
}

CPsProcess::~CPsProcess()
{
	Terminate();
}

bool CPsProcess::Init()
{
	m_pPsSession = new CPsSession(m_pcWorkingDir);
	if (m_pPsSession->Init())
	{
		printf("Parasolid initialized\n");
		return true;
	}
	else
		return false;
}

void CPsProcess::Terminate()
{
	delete m_pPsSession;
	printf("    Parasolid terminated\n");
}

std::vector<float> CPsProcess::PsSolidToFloatArray(PK_BODY_t body)
{
	PK_ERROR_code_t error_code;

	std::vector<float> floatArray;
	floatArray.push_back((float)body);

	std::vector<int> edgeTagArr;

	int iFaceCnt = 0;
	PK_FACE_t *faces = NULL;
	PK_BODY_ask_faces(body, &iFaceCnt, &faces);
	for (int i = 0; i < iFaceCnt; i++)
	{
		PK_TOPOL_facet_2_o_t opts;
		PK_TOPOL_facet_2_o_m(opts);
		opts.choice.facet_fin = PK_LOGICAL_true;
		opts.choice.fin_data = PK_LOGICAL_true;
		opts.choice.data_point_idx = PK_LOGICAL_true;
		opts.choice.facet_face = PK_LOGICAL_true;
		opts.choice.point_vec = PK_LOGICAL_true;
		opts.choice.normal_vec = PK_LOGICAL_true;
		opts.choice.data_normal_idx = PK_LOGICAL_true;
		opts.choice.fin_edge = PK_LOGICAL_true;
		opts.control.shape = PK_facet_shape_any_c;
#ifndef _DEBUG
		opts.control.is_surface_plane_ang = PK_LOGICAL_true;
		opts.control.surface_plane_ang = PI * 10 / 180;
		opts.control.is_curve_chord_ang = PK_LOGICAL_true;
		opts.control.curve_chord_ang = PI * 10 / 180;
#endif
		PK_TOPOL_facet_2_r_t facetTables;
		PK_FACE_t face = faces[i];

		PK_ATTDEF_t colour_attdef = 0;
		int n_attribs = 0;
		PK_ATTRIB_t* aAttributes = NULL;

		PK_ATTDEF_find("SDL/TYSA_COLOUR", &colour_attdef);

		error_code = PK_ENTITY_ask_attribs(face, colour_attdef, &n_attribs, &aAttributes);
		float dR = 0.5;
		float dG = 0.5;
		float dB = 0.5;

		if (NULL != aAttributes)
		{
			double *dVal;
			int iNum = 0;
			error_code = PK_ATTRIB_ask_doubles(aAttributes[0], 0, &iNum, &dVal);
			if (3 == iNum)
			{
				dR = (float)dVal[0];
				dG = (float)dVal[1];
				dB = (float)dVal[2];
			}
		}

		error_code = PK_TOPOL_facet_2(1, &face, NULL, &opts, &facetTables);

		PK_TOPOL_fctab_facet_fin_t *facetFinTable;
		PK_TOPOL_fctab_fin_data_t *finDataTable;
		PK_TOPOL_fctab_data_point_t *dataPointTable;
		PK_TOPOL_fctab_data_normal_t *dataNormalTable;
		PK_TOPOL_fctab_point_vec_t *pointVecTable;
		PK_TOPOL_fctab_normal_vec_t *normalVecTable;
		PK_TOPOL_fctab_facet_face_t *facetFaceTable;
		PK_TOPOL_fctab_fin_edge_t *finEdgeTable;

		/// For each of the facet tables
		for (int i = 0; i < facetTables.number_of_tables; i++)
		{
			/// Assign the correct table to the facet table variables
			if (facetTables.tables[i].fctab == PK_TOPOL_fctab_facet_fin_c)
			{
				/// If this is the facet-fin table
				facetFinTable = facetTables.tables[i].table.facet_fin;
#ifdef _DEBUG
				for (int j = 0; j < facetFinTable->length; j++)
				{
					int facet = facetFinTable->data[j].facet;
					int fin = facetFinTable->data[j].fin;
					if (fin == -1)
						double v = 0;
				}
#endif
			}
			else if (facetTables.tables[i].fctab == PK_TOPOL_fctab_fin_data_c)
			{
				/// If this is the fin-data table
				finDataTable = facetTables.tables[i].table.fin_data;
				for (int j = 0; j < finDataTable->length; j++)
				{
					int data = finDataTable->data[j];
					double v = 0;
				}
			}
			else if (facetTables.tables[i].fctab == PK_TOPOL_fctab_data_point_c)
			{
				/// If this is the data-point table
				dataPointTable = facetTables.tables[i].table.data_point_idx;
#ifdef _DEBUG
				for (int j = 0; j < dataPointTable->length; j++)
				{
					int point = dataPointTable->point[j];
					double v = 0;
				}
#endif
			}
			else if (facetTables.tables[i].fctab == PK_TOPOL_fctab_data_normal_c)
			{
				/// If this is the data-normal table
				dataNormalTable = facetTables.tables[i].table.data_normal_idx;
#ifdef _DEBUG
				for (int j = 0; j < dataNormalTable->length; j++)
				{
					int n = dataNormalTable->normal[j];
					double v = 0;
				}
#endif
			}
			else if (facetTables.tables[i].fctab == PK_TOPOL_fctab_point_vec_c)
			{
				/// If this is the point-vector table
				pointVecTable = facetTables.tables[i].table.point_vec;
#ifdef _DEBUG
				for (int j = 0; j < pointVecTable->length; j++)
				{
					double x = pointVecTable->vec[j].coord[0];
					double y = pointVecTable->vec[j].coord[1];
					double z = pointVecTable->vec[j].coord[2];
					double v = 0;
				}
#endif
			}
			else if (facetTables.tables[i].fctab == PK_TOPOL_fctab_normal_vec_c)
			{
				/// If this is the normal-vector table
				normalVecTable = facetTables.tables[i].table.normal_vec;
#ifdef _DEBUG
				for (int j = 0; j < normalVecTable->length; j++)
				{
					double x = normalVecTable->vec[j].coord[0];
					double y = normalVecTable->vec[j].coord[1];
					double z = normalVecTable->vec[j].coord[2];
					double v = 0;
				}
#endif
			}
			else if (facetTables.tables[i].fctab == PK_TOPOL_fctab_facet_face_c)
			{
				/// If this is the facet-face table
				facetFaceTable = facetTables.tables[i].table.facet_face;
#ifdef _DEBUG
				for (int j = 0; j < facetFaceTable->length; j++)
				{
					PK_FACE_t f = facetFaceTable->face[j];
					double v = 0;
				}
#endif
			}
			else if (facetTables.tables[i].fctab == PK_TOPOL_fctab_fin_edge_c)
			{
				/// If this is the fin-edge table
				finEdgeTable = facetTables.tables[i].table.fin_edge;
#ifdef _DEBUG
				for (int j = 0; j < finEdgeTable->length; j++)
				{
					PK_TOPOL_fcstr_fin_edge_t data = finEdgeTable->data[j];
					PK_EDGE_t edge = data.edge;
					int fin = data.fin;
					double v = 0;
				}
#endif
			}
		}

		std::vector<float> faceFloatArray;
		std::vector<float> normalFloatArray;
		/// For each item in the facet-fin table
		for (int i = 0; i < facetFinTable->length; i++)
		{
			/// Get the fin ID
			int finID = facetFinTable->data[i].fin;

			// Get the index of the fin
			int finIndex = finDataTable->data[finID];

			/// Get the point of the fin
			int point = dataPointTable->point[finIndex];

			/// Get the vector that specifies the point in space
			/// Add the vertex to the vertices list 
			faceFloatArray.push_back(float(pointVecTable->vec[point].coord[0] * m_dScale));
			faceFloatArray.push_back(float(pointVecTable->vec[point].coord[1] * m_dScale));
			faceFloatArray.push_back(float(pointVecTable->vec[point].coord[2] * m_dScale));

			// Get the normal of the fin
			int n = dataNormalTable->normal[finIndex];
			normalFloatArray.push_back((float)normalVecTable->vec[n].coord[0]);
			normalFloatArray.push_back((float)normalVecTable->vec[n].coord[1]);
			normalFloatArray.push_back((float)normalVecTable->vec[n].coord[2]);
		}
		floatArray.push_back((float)face);

		floatArray.push_back(dR);
		floatArray.push_back(dG);
		floatArray.push_back(dB);

		floatArray.push_back((float)(facetFinTable->length * 3));
		floatArray.reserve(floatArray.size() + faceFloatArray.size());
		std::copy(faceFloatArray.begin(), faceFloatArray.end(), std::back_inserter(floatArray));

		floatArray.reserve(floatArray.size() + normalFloatArray.size());
		std::copy(normalFloatArray.begin(), normalFloatArray.end(), std::back_inserter(floatArray));

		// Edge
		std::vector<int> faceEdgeTagArr;

		int indexOfEdgeCnt = (int)floatArray.size();
		floatArray.push_back(0);

		std::vector<float> edgeFloatArray;
		int iCurrentEdgeId = 0;
		int iEdgeCnt = 0;
		for (int i = 0; i < finEdgeTable->length; i++)
		{
			int iEdge = finEdgeTable->data[i].edge;

			bool found = std::count(edgeTagArr.begin(), edgeTagArr.end(), iEdge) > 0;
			if (!found)
			{
				floatArray.push_back((float)iEdge);
			
				found = std::count(faceEdgeTagArr.begin(), faceEdgeTagArr.end(), iEdge) > 0;
				if (!found)
					faceEdgeTagArr.push_back(iEdge);

				/// Get the fin ID
				int finID = finEdgeTable->data[i].fin;

				int m = finID % 3;
				int finIndex0;
				if (m != 0)
					finIndex0 = finDataTable->data[finID - 1];
				else
					finIndex0 = finDataTable->data[finID + 2];
				int finIndex1 = finDataTable->data[finID];

				int point0 = dataPointTable->point[finIndex0];
				int point1 = dataPointTable->point[finIndex1];

				edgeFloatArray.push_back(float(pointVecTable->vec[point0].coord[0] * m_dScale));
				edgeFloatArray.push_back(float(pointVecTable->vec[point0].coord[1] * m_dScale));
				edgeFloatArray.push_back(float(pointVecTable->vec[point0].coord[2] * m_dScale));

				edgeFloatArray.push_back(float(pointVecTable->vec[point1].coord[0] * m_dScale));
				edgeFloatArray.push_back(float(pointVecTable->vec[point1].coord[1] * m_dScale));
				edgeFloatArray.push_back(float(pointVecTable->vec[point1].coord[2] * m_dScale));

				floatArray.push_back((float)edgeFloatArray.size());
				floatArray.reserve(floatArray.size() + edgeFloatArray.size());
				std::copy(edgeFloatArray.begin(), edgeFloatArray.end(), std::back_inserter(floatArray));
				std::vector<float>().swap(edgeFloatArray);

				iEdgeCnt++;
			}
		}
		floatArray[indexOfEdgeCnt] = (float)iEdgeCnt;

		edgeTagArr.reserve(edgeTagArr.size() + faceEdgeTagArr.size());
		std::copy(faceEdgeTagArr.begin(), faceEdgeTagArr.end(), std::back_inserter(edgeTagArr));
		std::vector<int>().swap(faceEdgeTagArr);

	}

	return floatArray;
}

int CPsProcess::LoadPsFile(const char* fileName, const char* fileType, int &iCnt, PK_PART_t* &parts)
{
	PK_ERROR_code_t error_code;

	PK_PART_receive_o_t receive_opts;
	PK_PART_receive_o_m(receive_opts);

	if (strstr(fileType, "x_t") || strstr(fileType, "xmt_txt")) {
		receive_opts.transmit_format = PK_transmit_format_text_c;
	}
	else if (strstr(fileType, "x_b") || strstr(fileType, "xmt_bin")) {
		receive_opts.transmit_format = PK_transmit_format_binary_c;
	}
	else {
		return -1;
	}

	error_code = PK_PART_receive(fileName, &receive_opts, &iCnt, &parts);

	if (PK_ERROR_no_errors != error_code)
	{
		m_pPsSession->CheckAndHandleErrors();
		return -1;
	}

	return 0;
}

double CPsProcess::getFaceArea(const PK_FACE_t topol)
{
	PK_ERROR_code_t error_code;
	double dArea = 0;

	PK_TOPOL_eval_mass_props_o_t mass_opts;
	PK_TOPOL_eval_mass_props_o_m(mass_opts);
	mass_opts.mass = PK_mass_c_of_g_c;
	double amount[3], mass[3], c_of_g[9], m_of_i[27], periphery[3];

	error_code = PK_TOPOL_eval_mass_props(1, &topol, 1, &mass_opts, amount, mass, c_of_g, m_of_i, periphery);

	if (PK_ERROR_no_errors == error_code)
		dArea = amount[0];

	return dArea;
}

void CPsProcess::setDefaultMatrix(PK_TRANSF_sf_t &transf_sf)
{
	// Set default matrix
	transf_sf.matrix[0][0] = 1;
	transf_sf.matrix[1][0] = 0;
	transf_sf.matrix[2][0] = 0;
	transf_sf.matrix[3][0] = 0;

	transf_sf.matrix[0][1] = 0;
	transf_sf.matrix[1][1] = 1;
	transf_sf.matrix[2][1] = 0;
	transf_sf.matrix[3][1] = 0;

	transf_sf.matrix[0][2] = 0;
	transf_sf.matrix[1][2] = 0;
	transf_sf.matrix[2][2] = 1;
	transf_sf.matrix[3][2] = 0;

	transf_sf.matrix[0][3] = 0;
	transf_sf.matrix[1][3] = 0;
	transf_sf.matrix[2][3] = 0;
	transf_sf.matrix[3][3] = 1;
}

void CPsProcess::setMatrixToArr(const PK_TRANSF_sf_t transf_sf, std::vector<float> &floatArray)
{
	for (int ii = 0; ii < 4; ii++)
	{
		for (int jj = 0; jj < 4; jj++)
		{
			double el = transf_sf.matrix[jj][ii];

			if (3 == ii && 3 != jj)
				el *= m_dScale;

			floatArray.push_back((float)el);
		}
	}
}

PK_ASSEMBLY_t CPsProcess::findTopAssy()
{
	PK_ERROR_code_t error_code;
	PK_ASSEMBLY_t topAssem = PK_ENTITY_null;

	PK_PARTITION_t partition;
	int n_assy = 0;
	PK_ASSEMBLY_t* assems;
	PK_SESSION_ask_curr_partition(&partition);
	error_code = PK_PARTITION_ask_assemblies(partition, &n_assy, &assems);

	// chile-parent map
	std::map<PK_ASSEMBLY_t, PK_ASSEMBLY_t> child_parent;

	// Search assemblies and regiter to the map
	for (int i = 0; i < n_assy; i++)
	{
		PK_ASSEMBLY_t assem = assems[i];
		PK_CLASS_t ent_class;
		PK_ENTITY_ask_class(assem, &ent_class);

		if (PK_CLASS_assembly == ent_class)
			child_parent.insert(std::make_pair(assem, PK_ENTITY_null));
	}

	for (auto itr = child_parent.begin(); itr != child_parent.end(); ++itr)
	{
		PK_ASSEMBLY_t assem = itr->first;

		int n_inst;
		PK_INSTANCE_t *instances;
		PK_ASSEMBLY_ask_instances(assem, &n_inst, &instances);

		for (int i = 0; i < n_inst; i++)
		{
			PK_INSTANCE_t instance = instances[i];

			PK_INSTANCE_sf_s instance_sf;
			error_code = PK_INSTANCE_ask(instance, &instance_sf);

			// If instance part exists in the map, its pearent becomes current assem 
			PK_CLASS_t ent_class;
			PK_ENTITY_ask_class(instance_sf.part, &ent_class);
			PK_PART_t part = instance_sf.part;

			if (PK_CLASS_assembly == ent_class)
			{
				for (auto itr2 = child_parent.begin(); itr2 != child_parent.end(); ++itr2)
				{
					if (part == itr2->first)
						itr2->second = assem;
				}
			}
		}
	}

	//Assem which doesn't have a parent becomes top assem
	for (auto itr = child_parent.begin(); itr != child_parent.end(); ++itr)
	{
		if (PK_ENTITY_null == itr->second)
		{
			topAssem = itr->first;
			return topAssem;
		}
	}
	return topAssem;
}

static int level = 1;

void askNameAttribute(PK_ENTITY_t entity, char *in_name)
{
	PK_ERROR_code_t error_code;

	PK_CLASS_t ent_class;
	PK_ENTITY_ask_class(entity, &ent_class);

	int n_attribs = 0;
	PK_ATTRIB_t* attribs = NULL;
	PK_ATTDEF_t attdef = 0;

	PK_ATTDEF_find("SDL/TYSA_NAME", &attdef);
	error_code = PK_ENTITY_ask_attribs(entity, attdef, &n_attribs, &attribs);

	if (0 < n_attribs)
	{
		char *name;
		PK_ATTRIB_ask_string(attribs[0], 0, &name);
		sprintf(in_name, "%s", name);
	}
	else
	{
		sprintf(in_name, "(no-name)");
	}

	printf("%s, Tag: %d, Class: %d\n", in_name, entity, ent_class);

}

void CPsProcess::traverseInstance(PK_PART_t part, PK_TRANSF_sf_t currentMatrix, char* treeData)
{
	PK_ERROR_code_t error_code;
	level++;


	int n_inst;
	PK_INSTANCE_t *instances;
	PK_ASSEMBLY_ask_instances(part, &n_inst, &instances);

	if (n_inst)
	{
		if (NULL != treeData)
			strcat(treeData, ",\"children\":[");

		for (int i = 0; i < n_inst; i++)
		{
			PK_INSTANCE_t instance = instances[i];

			PK_INSTANCE_sf_s instance_sf;
			error_code = PK_INSTANCE_ask(instance, &instance_sf);

			for (int i = 0; i < level; i++)
				printf("+");
			printf("(%d) ", instance);

			char name[256];
			askNameAttribute(instance_sf.part, name);

			if (NULL != treeData)
			{
				if (0 < i)
					strcat(treeData, ",");

				char data[256];
				sprintf(data, "{\"text\":\"%s\",\"tag\":\"%d\",\"transf\":[", name, instance);
				strcat(treeData, data);
			}

			// Get transform
			PK_TRANSF_sf_t thisMatrix;
			setDefaultMatrix(thisMatrix);

			if (PK_ENTITY_null != instance_sf.transf)
			{
				error_code = PK_TRANSF_ask(instance_sf.transf, &thisMatrix);

				// Set trnsform
				if (NULL != treeData)
				{
					for (int ii = 0; ii < 4; ii++)
					{
						for (int jj = 0; jj < 4; jj++)
						{
							double dEl = thisMatrix.matrix[jj][ii];

							if (3 == ii && 3 != jj)
								dEl *= m_dScale;

							char cEl[256];
							sprintf(cEl, "%f", dEl);

							strcat(treeData, cEl);

							if (3 == ii && 3 == jj)
								strcat(treeData, "]");
							else
								strcat(treeData, ",");
						}
					}
				}
			}
			else
			{
				if (NULL != treeData)
					strcat(treeData, "]");
			}

			PK_TRANSF_sf_t netMatrix = multiplyMatrix(currentMatrix, thisMatrix);

			PK_CLASS_t ent_class;
			PK_ENTITY_ask_class(instance_sf.part, &ent_class);
			if (PK_CLASS_assembly == ent_class)
			{
				traverseInstance(instance_sf.part, netMatrix, treeData);

				if (NULL != treeData)
					strcat(treeData, "}");
			}
			else
			{
				if (NULL != treeData)
					strcat(treeData, "}");

				// Register net matrix map table
				leafInfo leaf;
				leaf.body = instance_sf.part;
				leaf.netMatrix = netMatrix;
				m_bodyInstance_netMatrix.insert(std::make_pair(instance, leaf));
			}
		}
		if (NULL != treeData)
			strcat(treeData, "]");
	}

	level--;
}

void CPsProcess::LoadFile(const char* activeSession, const char* fileName, const char* fileType, char* treeData)
{
	int iRet = -1;
	int iCnt;
	PK_PART_t* parts;

	{
		// CAD model
		if (isPsFile(fileType))
			iRet = LoadPsFile(fileName, fileType, iCnt, parts);
		else if (isExFile(fileType))
		{
			CExProcess exProcess = CExProcess();
			iRet = exProcess.LoadFile(fileName, iCnt, parts);
		}

		if (0 == iRet)
		{
			PK_ERROR_code_t error_code;

			int n_assy = 0;
			PK_ASSEMBLY_t *assems;
			PK_PARTITION_t partition;

			PK_SESSION_ask_curr_partition(&partition);
			error_code = PK_PARTITION_ask_assemblies(partition, &n_assy, &assems);


			if (0 == n_assy)
			{
				// Return part body info
				if (1 == iCnt)
				{
					PK_PART_t part = parts[0];
					PK_CLASS_t ent_class;
					PK_ENTITY_ask_class(part, &ent_class);
					if (PK_CLASS_body == ent_class)
					{
						PK_BODY_t solid_body = parts[0];

						char name[256];
						askNameAttribute(part, name);

						if (NULL != treeData)
							sprintf(treeData, "{\"type\":\"CAD\",\"data\":{\"text\":\"%s\",\"tag\":\"%d\"}}", name, part);
					}
				}
			}
			else
			{
				PK_ASSEMBLY_t topAssem = findTopAssy();
				char name[256];
				askNameAttribute(topAssem, name);

				if (NULL != treeData)
					sprintf(treeData, "{\"type\":\"CAD\",\"data\":{\"text\":\"%s\",\"tag\":\"%d\"", name, topAssem);

				PK_TRANSF_sf_t currentMatrix;
				setDefaultMatrix(currentMatrix);

				m_bodyInstance_netMatrix.clear();
				traverseInstance(topAssem, currentMatrix, treeData);

				if (NULL != treeData)
					strcat(treeData, "}}");
			}

			m_pPsSession->SetPsMark();
		}
	}
}

std::vector<float> CPsProcess::RequestBody(const PK_ENTITY_t entity)
{
	PK_ERROR_code_t error_code;

	// Make working partation
	PK_PARTITION_t partition;
	error_code = PK_PARTITION_create_empty(&partition);
	error_code = PK_PARTITION_set_current(partition);

	PK_BODY_t body = entity;

	PK_CLASS_t ent_class;
	error_code = PK_ENTITY_ask_class(entity, &ent_class);
	if (PK_CLASS_instance == ent_class)
	{
		struct PK_INSTANCE_sf_s instance_sf;
		error_code = PK_INSTANCE_ask(entity, &instance_sf);

		body = instance_sf.part;
	}

	std::vector<float> floatArray = PsSolidToFloatArray(body);
	
	// Return to main partition and delete working patation
	error_code = PK_PARTITION_set_current(m_pPsSession->GetPkPartition());

	PK_PARTITION_delete_o_t delete_opts;
	PK_PARTITION_delete_o_m(delete_opts);
	delete_opts.delete_non_empty = PK_LOGICAL_true;
	error_code = PK_PARTITION_delete(partition, &delete_opts);

	return floatArray;
}

void CPsProcess::setBasisSet(const double *offset, const double *dir, const double *axis, PK_AXIS2_sf_s &basis_set)
{
	basis_set.location.coord[0] = offset[0] / m_dScale;
	basis_set.location.coord[1] = offset[1] / m_dScale;
	basis_set.location.coord[2] = offset[2] / m_dScale;
	basis_set.axis.coord[0] = axis[0];
	basis_set.axis.coord[1] = axis[1];
	basis_set.axis.coord[2] = axis[2];
	basis_set.ref_direction.coord[0] = dir[0];
	basis_set.ref_direction.coord[1] = dir[1];
	basis_set.ref_direction.coord[2] = dir[2];
}

int CPsProcess::createIstance(const PK_BODY_t body)
{
	PK_ERROR_code_t error_code;
	
	// Find top assem
	PK_ASSEMBLY_t topAssem = PK_ENTITY_null;
	topAssem = findTopAssy();

	if (PK_ENTITY_null == topAssem)
		error_code = PK_ASSEMBLY_create_empty(&topAssem);

	PK_INSTANCE_sf_t instance_sf;
	PK_INSTANCE_t instance;

	instance_sf.assembly = topAssem;
	instance_sf.part = body;
	instance_sf.transf = PK_ENTITY_null;

	error_code = PK_INSTANCE_create(&instance_sf, &instance);

	return (int)instance;
}

std::vector<float> CPsProcess::CreateBlock(const double *size, const double *offset, const double *dir, const double *axis)
{
	std::vector<float> floatArray;
	PK_ERROR_code_t error_code;

	PK_BODY_t solid_body = PK_ENTITY_null;
	PK_AXIS2_sf_s basis_set;

	setBasisSet(offset, dir, axis, basis_set);
	error_code = PK_BODY_create_solid_block(size[0] / m_dScale, size[1] / m_dScale, size[2] / m_dScale, &basis_set, &solid_body);
	
	if (PK_ERROR_no_errors == error_code)
	{
		int instance = createIstance(solid_body);

		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(solid_body);

		floatArray.insert(floatArray.begin(), instance);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}
	return floatArray;
}

std::vector<float> CPsProcess::CreateCylinder(const double rad, const double height, const double *offset, const double *dir, const double *axis)
{
	std::vector<float> floatArray;
	PK_ERROR_code_t error_code;

	PK_BODY_t solid_body = PK_ENTITY_null;
	PK_AXIS2_sf_s basis_set;

	setBasisSet(offset, dir, axis, basis_set);
	error_code = PK_BODY_create_solid_cyl(rad / m_dScale, height / m_dScale, &basis_set, &solid_body);

	if (PK_ERROR_no_errors == error_code)
	{
		int instance = createIstance(solid_body);
		
		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(solid_body);

		floatArray.insert(floatArray.begin(), instance);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}
	return floatArray;
}

std::vector<float> CPsProcess::CreatePrism(const double rad, const double height, const int num, const double *offset, const double *dir, const double *axis)
{
	std::vector<float> floatArray;
	PK_ERROR_code_t error_code;

	PK_BODY_t solid_body = PK_ENTITY_null;
	PK_AXIS2_sf_s basis_set;

	setBasisSet(offset, dir, axis, basis_set);
	error_code = PK_BODY_create_solid_prism(rad / m_dScale, height / m_dScale, num, &basis_set, &solid_body);

	if (PK_ERROR_no_errors == error_code)
	{
		int instance = createIstance(solid_body);
		
		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(solid_body);

		floatArray.insert(floatArray.begin(), instance);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}
	return floatArray;
}

std::vector<float> CPsProcess::CreateCone(const double topR, const double bottomR, const double height, const double *offset, const double *dir, const double *axis)
{
	std::vector<float> floatArray;
	PK_ERROR_code_t error_code;

	PK_BODY_t solid_body = PK_ENTITY_null;
	PK_AXIS2_sf_s basis_set;

	double semiAng = atan((bottomR - topR) / height);

	setBasisSet(offset, dir, axis, basis_set);
	error_code = PK_BODY_create_solid_cone(topR / m_dScale, height / m_dScale, semiAng, &basis_set, &solid_body);

	if (PK_ERROR_no_errors == error_code)
	{
		int instance = createIstance(solid_body);
		
		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(solid_body);

		floatArray.insert(floatArray.begin(), instance);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}
	return floatArray;
}

std::vector<float> CPsProcess::CreateTorus(const double majorR, const double minorR, const double *offset, const double *dir, const double *axis)
{
	std::vector<float> floatArray;
	PK_ERROR_code_t error_code;

	PK_BODY_t solid_body = PK_ENTITY_null;
	PK_AXIS2_sf_s basis_set;

	setBasisSet(offset, dir, axis, basis_set);
	error_code = PK_BODY_create_solid_torus(majorR / m_dScale, minorR / m_dScale, &basis_set, &solid_body);

	if (PK_ERROR_no_errors == error_code)
	{
		int instance = createIstance(solid_body);
		
		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(solid_body);

		floatArray.insert(floatArray.begin(), instance);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}
	return floatArray;
}

std::vector<float> CPsProcess::CreateSphere(const double rad, const double *offset)
{
	std::vector<float> floatArray;
	PK_ERROR_code_t error_code;

	PK_BODY_t solid_body = PK_ENTITY_null;
	PK_AXIS2_sf_s basis_set;
	
	double dir[3] = { 1, 0, 0 };
	double axis[3] = { 0, 0, 1 };

	setBasisSet(offset, dir, axis, basis_set);
	error_code = PK_BODY_create_solid_sphere(rad / m_dScale, &basis_set, &solid_body);

	if (PK_ERROR_no_errors == error_code)
	{
		int instance = createIstance(solid_body);
		
		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(solid_body);

		floatArray.insert(floatArray.begin(), instance);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}
	return floatArray;
}

std::vector<float> CPsProcess::Blend(const char cType, const double size, const int edgeCnt, const PK_EDGE_t *edges)
{
	std::vector<float> floatArray;

	PK_ERROR_code_t			error_code = PK_ERROR_no_errors;
	PK_BODY_t				body;
	int						n_blend_edges = 0;
	PK_EDGE_t				*blend_edges = NULL;
	PK_BODY_fix_blends_o_t	optionsFix;
	int						n_blends = 0;
	PK_FACE_t				*blends = NULL;
	PK_FACE_array_t			*unders = NULL;
	int						*topols = NULL;
	PK_blend_fault_t		fault = PK_blend_fault_no_fault_c;
	PK_EDGE_t				fault_edge = PK_ENTITY_null;
	PK_TOPOL_t				fault_topol = PK_ENTITY_null;

	PK_EDGE_ask_body(edges[0], &body);

	switch (cType)
	{
	case 'R':
	{
		PK_EDGE_set_blend_constant_o_t options;
		PK_EDGE_set_blend_constant_o_m(options);

		options.properties.propagate = PK_blend_propagate_yes_c;
		options.properties.ov_cliff_end = PK_blend_ov_cliff_end_yes_c;

		error_code = PK_EDGE_set_blend_constant(edgeCnt, edges, size / m_dScale, &options, &n_blend_edges, &blend_edges);
	}
	break;
	case 'C':
	{
		PK_EDGE_set_blend_chamfer_o_t options;
		PK_EDGE_set_blend_chamfer_o_m(options);

		options.properties.propagate = PK_blend_propagate_yes_c;

		error_code = PK_EDGE_set_blend_chamfer(edgeCnt, edges, size / m_dScale, size / m_dScale, NULL, &options, &n_blend_edges, &blend_edges);
	}
	break;
	default:
		break;
	}

	if (PK_ERROR_no_errors == error_code && n_blend_edges)
	{
		PK_MEMORY_free(blend_edges);

		PK_BODY_fix_blends_o_m(optionsFix);
		error_code = PK_BODY_fix_blends(body, &optionsFix, &n_blends, &blends, &unders, &topols, &fault, &fault_edge, &fault_topol);

		if (PK_ERROR_no_errors == error_code)
		{
			if (n_blends)
			{
				PK_MEMORY_free(blends);
				PK_MEMORY_free(unders);
				PK_MEMORY_free(topols);
			}

			m_pPsSession->SetPsMark();
			floatArray = PsSolidToFloatArray(body);
		}
		else
		{
			m_pPsSession->CheckAndHandleErrors();
		}
	}

	return floatArray;
}

std::vector<float> CPsProcess::Hollow(const double thisckness, const PK_ENTITY_t entity, const int faceCnt, const PK_FACE_t* pierceFaces)
{
	std::vector<float> floatArray;

	PK_ERROR_code_t error_code;
	PK_CLASS_t entity_class;
	PK_TOPOL_track_r_t   tracking;
	PK_TOPOL_local_r_t   results;
	PK_BODY_hollow_o_t   hollow_opts;
	PK_BODY_hollow_o_m(hollow_opts);
	PK_FACE_contains_vectors_o_t options;
	int i, n_faces;

	PK_BODY_t body = entity;

	PK_ENTITY_ask_class(entity, &entity_class);

	if (PK_CLASS_instance == entity_class)
	{
		struct PK_INSTANCE_sf_s instance_sf;
		error_code = PK_INSTANCE_ask(entity, &instance_sf);

		body = instance_sf.part;
	}

	if (faceCnt)
	{
		hollow_opts.n_pierce_faces = faceCnt;
		hollow_opts.pierce_faces = pierceFaces;
	}

	error_code = PK_BODY_hollow_2(body, thisckness / m_dScale, 1.0e-06, &hollow_opts, &tracking, &results);

	if (PK_ERROR_no_errors == error_code)
	{
		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(body);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}

	return floatArray;
}

std::vector<float> CPsProcess::Offset(const double value, const PK_ENTITY_t entity, const int faceCnt, const PK_FACE_t* offsetFaces)
{
	std::vector<float> floatArray;

	PK_ERROR_code_t error_code;
	PK_CLASS_t entity_class;
	PK_FACE_offset_o_s face_offset_opts;
	PK_BODY_offset_o_t body_offset_opts;
	PK_TOPOL_track_r_t tracking;
	PK_TOPOL_local_r_t local_res;

	PK_BODY_t body = entity;

	PK_ENTITY_ask_class(entity, &entity_class);

	if (PK_CLASS_instance == entity_class)
	{
		struct PK_INSTANCE_sf_s instance_sf;
		error_code = PK_INSTANCE_ask(entity, &instance_sf);

		body = instance_sf.part;
	}

	if (0 < faceCnt)
	{
		PK_FACE_offset_o_m(face_offset_opts);
		double* offsets = new double(faceCnt);
		for (int i = 0; i < faceCnt; i++)
			offsets[i] = value / m_dScale;

		error_code = PK_FACE_offset_2(faceCnt, offsetFaces, offsets, 1.0e-06, &face_offset_opts, &tracking, &local_res);
	}
	else
	{
		PK_BODY_offset_o_m(body_offset_opts);
		error_code = PK_BODY_offset_2(body, value / m_dScale, 1.0e-06, &body_offset_opts, &tracking, &local_res);

	}

	if (PK_ERROR_no_errors == error_code)
	{
		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(body);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}
	return floatArray;
}

std::vector<float> CPsProcess::ImprintRo(const double dOffset, const int edgeCnt, const PK_EDGE_t *edges)
{
	std::vector<float> floatArray;
	
	PK_ERROR_code_t error_code;
	PK_CIRCLE_sf_t circle_sf;
	PK_CPCURVE_t curve;
	PK_CLASS_t curve_class;
	PK_CURVE_t* wire_circle_arrar;
	PK_INTERVAL_t* interval_array;
	PK_CURVE_project_o_t  project_opts;
	PK_CURVE_project_r_t  project_results;
	PK_ENTITY_track_r_t   project_tracking;

	PK_BODY_t body;
	error_code = PK_EDGE_ask_body(edges[0], &body);

	wire_circle_arrar = new PK_CPCURVE_t[edgeCnt];
	interval_array = new PK_INTERVAL_t[edgeCnt];

	PK_CURVE_project_o_m(project_opts);
	project_opts.function = PK_proj_function_imprint_c;

	for (int i = 0; i < edgeCnt; i++)
	{
		PK_EDGE_ask_curve(edges[i], &curve);
		PK_ENTITY_ask_class(curve, &curve_class);

		if (PK_CLASS_circle == curve_class)
		{
			error_code = PK_CIRCLE_ask(curve, &circle_sf);
			circle_sf.radius += dOffset / m_dScale;
			error_code = PK_CIRCLE_create(&circle_sf, &wire_circle_arrar[i]);
			error_code = PK_CURVE_ask_interval(wire_circle_arrar[i], &interval_array[i]);
		}
	}

	error_code = PK_CURVE_project(edgeCnt, wire_circle_arrar, interval_array, 1, &body, &project_opts, &project_results, &project_tracking);

	delete[] wire_circle_arrar;
	delete[] interval_array;

	if (PK_ERROR_no_errors == error_code)
	{
		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(body);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}

	return floatArray;
}

std::vector<float> CPsProcess::ImprintFace(const PK_FACE_t targetFace, const int faceCnt, const PK_FACE_t *toolFace)
{
	std::vector<float> floatArray;
	
	PK_ERROR_code_t error_code;
	PK_EDGE_t *edges = NULL;
	PK_CURVE_t *curve_array;
	PK_INTERVAL_t *interval_array = NULL;
	PK_INTERVAL_t *bounds = NULL;
	PK_VECTOR_t *vectors = NULL;
	double *ts_1 = NULL;
	double *ts_2 = NULL;

	PK_intersect_vector_t *types = NULL;

	PK_SURF_t surface;
	PK_FACE_ask_surf(targetFace, &surface);

	PK_CLASS_t entity_class;
	error_code = PK_ENTITY_ask_class(surface, &entity_class);
	if (PK_CLASS_plane == entity_class)
	{
		// Get normal dir
		PK_PLANE_sf_t plane_sf;
		error_code = PK_PLANE_ask(surface, &plane_sf);

		PK_BODY_t body;
		PK_FACE_ask_body(targetFace, &body);

		bool bFlg = false;
		for (int i = 0; i < faceCnt; i++)
		{
			PK_FACE_t tool_face = toolFace[i];
			int n_edges;
			error_code = PK_FACE_ask_edges(tool_face, &n_edges, &edges);

			curve_array = new PK_CURVE_t[n_edges];
			interval_array = new PK_INTERVAL_t[n_edges];
			bounds = new PK_INTERVAL_t[n_edges];

			for (int j = 0; j < n_edges; j++)
			{
				error_code = PK_EDGE_ask_curve(edges[j], &curve_array[j]);
				error_code = PK_ENTITY_ask_class(curve_array[j], &entity_class);
				error_code = PK_CURVE_ask_interval(curve_array[j], &interval_array[j]);
			}

			PK_CURVE_intersect_curve_o_t intersectOpts;
			PK_CURVE_intersect_curve_o_m(intersectOpts);
			for (int j = 0; j < n_edges; j++)
			{
				int jj = j + 1;
				if (n_edges == jj)
					jj = 0;
				int n_vectors = 0;
				error_code = PK_CURVE_intersect_curve(curve_array[j], interval_array[j], curve_array[jj], interval_array[jj], &intersectOpts,
					&n_vectors, &vectors, &ts_1, &ts_2, &types);
				bounds[j].value[1] = ts_1[0];
				bounds[jj].value[0] = ts_2[0];
			}

			for (int j = 0; j < n_edges; j++)
			{
				double st = bounds[j].value[0];
				double en = bounds[j].value[1];

				if (st > en)
				{
					bounds[j].value[0] = en;
					bounds[j].value[1] = st;
				}
			}

			PK_CURVE_project_o_t  project_opts;
			PK_CURVE_project_r_t  project_results;
			PK_ENTITY_track_r_t   project_tracking;

			PK_CURVE_project_o_m(project_opts);
			project_opts.function = PK_proj_function_imprint_c;
			project_opts.have_direction = PK_LOGICAL_true;
			project_opts.direction = { -plane_sf.basis_set.axis.coord[0], -plane_sf.basis_set.axis.coord[1], -plane_sf.basis_set.axis.coord[2] };
			error_code = PK_CURVE_project(n_edges, curve_array, bounds, 1, &body, &project_opts, &project_results, &project_tracking);

			if (PK_ERROR_no_errors == error_code)
				bFlg = true;
			else
				m_pPsSession->CheckAndHandleErrors();

		}

		if (bFlg)
		{
			m_pPsSession->SetPsMark();
			floatArray = PsSolidToFloatArray(body);
		}
	}

	return floatArray;
}

PK_BODY_t CPsProcess::createDummyBody()
{
	PK_ERROR_code_t error_code;

	PK_BODY_t body = PK_ENTITY_null;

	PK_POINT_t point;
	PK_POINT_sf_t point_sf;
	point_sf.position.coord[0] = 0.;
	point_sf.position.coord[1] = 0.;
	point_sf.position.coord[2] = 0.;
	PK_POINT_create(&point_sf, &point);
	error_code = PK_POINT_make_minimum_body(point, &body);

	return body;
}

int CPsProcess::getInstanceCount(const PK_ENTITY_t target)
{
	PK_ERROR_code_t error_code;
	int cnt = 0;

	int n_assy = 0;
	PK_ASSEMBLY_t *assems;
	PK_PARTITION_t partition;

	PK_SESSION_ask_curr_partition(&partition);
	error_code = PK_PARTITION_ask_assemblies(partition, &n_assy, &assems);

	for (int i = 0; i < n_assy; i++)
	{
		int n_inst;
		PK_INSTANCE_t *instances;
		error_code = PK_ASSEMBLY_ask_instances(assems[i], &n_inst, &instances);
		for (int j = 0; j < n_inst; j++)
		{
			struct PK_INSTANCE_sf_s instance_sf;
			error_code = PK_INSTANCE_ask(instances[j], &instance_sf);
			if (target == instance_sf.part)
				cnt++;
		}
	}

	return cnt;
}

PK_ERROR_code_t CPsProcess::transformBody(const PK_BODY_t body, PK_TRANSF_t transf)
{
	PK_ERROR_code_t error_code;
	
	PK_BODY_transform_o_t transf_opts;
	PK_TOPOL_track_r_t tracking;
	PK_TOPOL_local_r_t local_res;
	
	PK_BODY_transform_o_m(transf_opts);
	error_code = PK_BODY_transform_2(body, transf, 1.0e-06, &transf_opts, &tracking, &local_res);

	return error_code;
}

std::vector<float> CPsProcess::Boolean(const char cType, const PK_ENTITY_t targetEntity, const int toolCnt, const PK_ENTITY_t* toolEntities)
{
	std::vector<float> floatArray;
	
	PK_ERROR_code_t error_code;
	PK_BODY_boolean_o_t options;
	PK_TOPOL_track_r_t tracking;
	PK_boolean_r_t results;

	PK_BODY_boolean_o_m(options);
	
	switch (cType)
	{
	case 'U':
	{
		options.function = PK_boolean_unite;
	} break;
	case 'S':
	{
		options.function = PK_boolean_subtract_c;
	} break;
	case 'I':
	{
		options.function = PK_boolean_intersect;
	} break;
	default: break;
	}

	PK_CLASS_t ent_class;
	
	PK_BODY_t targetBody_proto;
	PK_BODY_t *toolBodies_proto;
	PK_BODY_t dummy_body = PK_ENTITY_null;
	std::vector<PK_ENTITY_t> deleteEntityArr;

	toolBodies_proto = new PK_BODY_t[toolCnt];
	for (int i = 0; i < toolCnt; i++)
	{
		toolBodies_proto[i] = toolEntities[i];
	
		error_code = PK_ENTITY_ask_class(toolEntities[i], &ent_class);
		if (PK_CLASS_instance == ent_class)
		{
			struct PK_INSTANCE_sf_s instance_sf;
			error_code = PK_INSTANCE_ask(toolEntities[i], &instance_sf);

			int instCnt = getInstanceCount(instance_sf.part);

			if (1 == instCnt)
			{
				// Make it independent 
				if (PK_ENTITY_null == dummy_body)
					dummy_body = createDummyBody();

				error_code = PK_INSTANCE_change_part(toolEntities[i], dummy_body);
				toolBodies_proto[i] = instance_sf.part;
			}
			else
			{
				// Create a copy body
				PK_ENTITY_copy_o_t copy_opts;
				PK_ENTITY_track_r_t en_tracking;
				PK_ENTITY_copy_o_m(copy_opts);

				error_code = PK_ENTITY_copy_2(instance_sf.part, &copy_opts, &toolBodies_proto[i], &en_tracking);
			}

			if (PK_ENTITY_null != instance_sf.transf)
				error_code = transformBody(toolBodies_proto[i], instance_sf.transf);

			deleteEntityArr.push_back(toolEntities[i]);
		}
	}

	for (int i = 0; i < deleteEntityArr.size(); i++)
	{
		error_code = PK_ENTITY_delete(1, &deleteEntityArr[i]);
	}
	dummy_body = PK_ENTITY_null;

	targetBody_proto = targetEntity;

	error_code = PK_ENTITY_ask_class(targetEntity, &ent_class);
	if (PK_CLASS_instance == ent_class)
	{
		struct PK_INSTANCE_sf_s instance_sf;
		error_code = PK_INSTANCE_ask(targetEntity, &instance_sf);
		
		int instCnt = getInstanceCount(instance_sf.part);

		if (1 == instCnt)
		{
			// Make it independent 
			dummy_body = createDummyBody();
			error_code = PK_INSTANCE_change_part(targetEntity, dummy_body);
			targetBody_proto = instance_sf.part;

			if (PK_ENTITY_null != instance_sf.transf)
				error_code = transformBody(targetBody_proto, instance_sf.transf);

			error_code = PK_BODY_boolean_2(targetBody_proto, toolCnt, toolBodies_proto, &options, &tracking, &results);

			error_code = PK_INSTANCE_change_part(targetEntity, instance_sf.part);

			error_code = PK_ENTITY_delete(1, &dummy_body);

		}
		else
		{
			// Create a copy body
			PK_ENTITY_copy_o_t copy_opts;
			PK_ENTITY_track_r_t en_tracking;
			PK_ENTITY_copy_o_m(copy_opts);

			error_code = PK_ENTITY_copy_2(instance_sf.part, &copy_opts, &targetBody_proto, &en_tracking);

			if (PK_ENTITY_null != instance_sf.transf)
				error_code = transformBody(targetBody_proto, instance_sf.transf);

			error_code = PK_BODY_boolean_2(targetBody_proto, toolCnt, toolBodies_proto, &options, &tracking, &results);

			error_code = PK_INSTANCE_change_part(targetEntity, targetBody_proto);
		}
	}
	else
	{
		error_code = PK_BODY_boolean_2(targetBody_proto, toolCnt, toolBodies_proto, &options, &tracking, &results);
	}


	if (PK_ERROR_no_errors == error_code)
	{
		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(targetBody_proto);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}

	return floatArray;
}

std::vector<float> CPsProcess::CopyFace(const int faceCnt, const PK_ENTITY_t* faces)
{
	std::vector<float> floatArray;

	PK_ERROR_code_t error_code;
	PK_BODY_t body;
	PK_FACE_ask_body(faces[0], &body);

	PK_BODY_t solid_body;
	error_code = PK_FACE_make_sheet_body(faceCnt, faces, &solid_body);

	if (PK_ERROR_no_errors == error_code)
	{
		int instance = createIstance(solid_body);

		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(solid_body);

		floatArray.insert(floatArray.begin(), instance);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}
	return floatArray;
}

std::vector<float> CPsProcess::DeleteFace(const int faceCnt, const PK_ENTITY_t *faces)
{
	std::vector<float> floatArray;
	
	PK_ERROR_code_t error_code;
	PK_BODY_t body;
	PK_FACE_ask_body(faces[0], &body);

	PK_FACE_delete_o_t del_ots;
	PK_FACE_delete_o_m(del_ots);
	PK_TOPOL_track_r_t track;
	error_code = PK_FACE_delete_2(faceCnt, faces, &del_ots, &track);

	if (PK_ERROR_no_errors == error_code)
	{
		m_pPsSession->SetPsMark();
		floatArray = PsSolidToFloatArray(body);
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
	}

	return floatArray;
}

int CPsProcess::AlignZ(const int faceCnt, const PK_ENTITY_t* faces)
{
	PK_ERROR_code_t error_code;

	PK_FACE_t face = faces[0];

	PK_BODY_t body;
	PK_FACE_ask_body(face, &body);

	PK_TRANSF_sf_t netTrans_sf;
	PK_TRANSF_t netTransf;
	for (auto itr = m_bodyInstance_netMatrix.begin(); itr != m_bodyInstance_netMatrix.end(); ++itr)
	{
		leafInfo leaf = itr->second;
		if (body == leaf.body)
		{
			netTrans_sf = leaf.netMatrix;
			error_code = PK_TRANSF_create(&netTrans_sf, &netTransf);
			break;
		}
	}

	PK_SURF_t surf;
	PK_FACE_ask_surf(face, &surf);

	PK_PLANE_sf_t plane_sf;
	error_code = PK_PLANE_ask(surf, &plane_sf);

	PK_VECTOR_t axis = plane_sf.basis_set.axis;

	int n_edges;
	PK_EDGE_t* edges;
	PK_FACE_ask_edges(face, &n_edges, &edges);

	PK_EDGE_t edge = edges[0];

	PK_CURVE_t curve;
	PK_EDGE_ask_curve(edge, &curve);

	PK_CIRCLE_sf_t circle_sf;
	PK_CIRCLE_ask(curve, &circle_sf);

	PK_VECTOR_t center = circle_sf.basis_set.location;
	
	//axis.coord[0] *= -1;
	//axis.coord[1] *= -1;
	//axis.coord[2] *= -1;
	
	double offset = 0.008;
	center.coord[0] += axis.coord[0] * offset;
	center.coord[1] += axis.coord[1] * offset;
	center.coord[2] += axis.coord[2] * offset;

	axis.coord[0] *= -1;
	axis.coord[1] *= -1;
	axis.coord[2] *= -1;

	error_code = PK_VECTOR_transform(center, netTransf, &center);

	error_code = PK_VECTOR_transform_direction(axis, netTransf, &axis);
	error_code = PK_VECTOR_normalise(axis, &axis);

	PK_VECTOR_t alignAxis;
	alignAxis.coord[0] = 0;
	alignAxis.coord[1] = 0;
	alignAxis.coord[2] = 1;

	double dot = PK_VECTOR_dot(axis, alignAxis);
	double angle = acos(dot);

	PK_VECTOR_t cross = PK_VECTOR_cross(axis, alignAxis);
	error_code = PK_VECTOR_normalise(cross, &cross);

	PK_TRANSF_t rotTransf1, rotTransf2, rotTransf, traTransf, topTransf;
	PK_VECTOR_t org, zAxis;
	double pi = 3.1415926535897932384626433832795;

	org.coord[0] = 0.0;
	org.coord[1] = 0.0;
	org.coord[2] = 0.0;
	PK_TRANSF_create_rotation(org, cross, angle, &rotTransf1);

	zAxis.coord[0] = 0.0;
	zAxis.coord[1] = 0.0;
	zAxis.coord[2] = 1.0;
	PK_TRANSF_create_rotation(org, zAxis, - pi / 2.0, &rotTransf2);
	//PK_TRANSF_create_rotation(org, zAxis, 0.0, &rotTransf2);

	error_code = PK_TRANSF_transform(rotTransf1, rotTransf2, &rotTransf);

	PK_VECTOR_t rotCenter;
	error_code = PK_VECTOR_transform(center, rotTransf, &center);

	center.coord[0] *= -1;
	center.coord[1] *= -1;
	center.coord[2] *= -1;

	error_code = PK_TRANSF_create_translation(center, &traTransf);

	error_code = PK_TRANSF_transform(rotTransf, traTransf, &topTransf);

	PK_TRANSF_sf_t topTransf_sf;
	error_code = PK_TRANSF_ask(topTransf, &topTransf_sf);

	// Apply matrix
	PK_ASSEMBLY_t topAssem = findTopAssy();

	int n_instances;
	PK_INSTANCE_t *instances;
	error_code = PK_ASSEMBLY_ask_instances(topAssem, &n_instances, &instances);

	for (int i = 0; i < n_instances; i++)
	{
		PK_INSTANCE_sf_t instance_sf;
		PK_TRANSF_t transf;
		
		error_code = PK_INSTANCE_ask(instances[i], &instance_sf);

		if (PK_ENTITY_null != instance_sf.transf)
		{
			error_code = PK_TRANSF_transform(instance_sf.transf, topTransf, &transf);

			error_code = PK_INSTANCE_replace_transf(instances[i], transf);
		}
		else
		{
			error_code = PK_TRANSF_create(&topTransf_sf, &transf);
			error_code = PK_INSTANCE_replace_transf(instances[i], transf);
		}

	}

	return 0;
}

int CPsProcess::DeleteEntity(const PK_ENTITY_t entity)
{
	PK_ERROR_code_t error_code;

	error_code = PK_ENTITY_delete(1, &entity);

	if (PK_ERROR_no_errors == error_code)
	{
		m_pPsSession->SetPsMark();
		return 0;
	}
	else
	{
		m_pPsSession->CheckAndHandleErrors();
		return -1;
	}
}

int CPsProcess::SetTransform(const PK_ENTITY_t entity, const PK_TRANSF_sf_t transf_sf)
{
	PK_ERROR_code_t error_code;

	PK_CLASS_t ent_class;
	PK_ENTITY_ask_class(entity, &ent_class);
	if (PK_CLASS_instance != ent_class)
		return -1;

	PK_TRANSF_t transf;
	error_code = PK_TRANSF_create(&transf_sf, &transf);

	error_code = PK_INSTANCE_replace_transf(entity, transf);

	if (PK_ERROR_no_errors == error_code)
	{
		m_pPsSession->SetPsMark();
		return 0;
	}
	else
		return -1;
}

std::vector<float> CPsProcess::MassProps(const PK_BODY_t body)
{
	std::vector<float> floatArray;

	PK_ERROR_code_t error_code;
	PK_TOPOL_eval_mass_props_o_t mass_opts;
	PK_TOPOL_eval_mass_props_o_m(mass_opts);
	mass_opts.mass = PK_mass_c_of_g_c;
	double amount[3], mass[3], c_of_g[9], m_of_i[27], periphery[3];

	error_code = PK_TOPOL_eval_mass_props(1, &body, 1, &mass_opts, amount, mass, c_of_g, m_of_i, periphery);

	if (PK_ERROR_no_errors == error_code)
	{
		floatArray.push_back(float(mass[0] * m_dScale * m_dScale * m_dScale));
		floatArray.push_back(float(periphery[0] * m_dScale * m_dScale));
		floatArray.push_back(float(c_of_g[0] * m_dScale));
		floatArray.push_back(float(c_of_g[1] * m_dScale));
		floatArray.push_back(float(c_of_g[2] * m_dScale));
	}
	return floatArray;
}

std::vector<float> CPsProcess::FR_Holes(const double dMaxDia, const PK_BODY_t body)
{
	std::vector<float> floatArray;
	
	PK_ERROR_code_t error_code;
	int n_edges = 0, face_count = 0;
	PK_EDGE_t *edges = NULL;
	PK_EDGE_ask_convexity_o_t convexity_opts;
	PK_emboss_convexity_t convexity;

	PK_CPCURVE_t curve;
	PK_CLASS_t curve_class;
	PK_CIRCLE_sf_s circle;
	PK_VERTEX_t vertices[2];
	double rad = 0;
	PK_FACE_t* faces;

	error_code = PK_BODY_ask_edges(body, &n_edges, &edges);
	if (n_edges != 0)
	{
		// Find all of the concave edges by looping through every edge on the body.
		for (int i = 0; i < n_edges; i++)
		{

			PK_EDGE_ask_convexity_o_m(convexity_opts);
			error_code = PK_EDGE_ask_convexity(edges[i], &convexity_opts, &convexity);

			// Find bottom circleer edge
			if (convexity == PK_EDGE_convexity_concave_c)
			{
				PK_EDGE_ask_curve(edges[i], &curve);
				PK_ENTITY_ask_class(curve, &curve_class);

				if (PK_CLASS_circle == curve_class)
				{
					error_code = PK_EDGE_ask_vertices(edges[i], vertices);

					//if (PK_ENTITY_null == vertices[0] && PK_ENTITY_null == vertices[1])
					{
						error_code = PK_CIRCLE_ask(curve, &circle);

						rad = circle.radius;
						if (dMaxDia / m_dScale > rad)
						{
							//printf("Edge: %d, Rad: %.2f\n", edges[i], rad * 1000);
							error_code = PK_EDGE_ask_faces(edges[i], &face_count, &faces);
							for (int i = 0; i < face_count; i++)
								floatArray.push_back((float)faces[i]);
						}
					}
				}
			}
		}

		// Free memory.
		PK_MEMORY_free(edges);
		if (face_count)
			PK_MEMORY_free(faces);
	}

	return floatArray;
}

std::vector<float> CPsProcess::FR_Concaves(const double dMinAng, const double dMaxAng, const PK_BODY_t body)
{
	std::vector<float> floatArray;
	
	PK_ERROR_code_t error_code;
	int n_edges = 0, n_faces = 0;
	PK_EDGE_t *edges = NULL;
	PK_FACE_t *faces = NULL;
	PK_SURF_t surface;
	PK_LOGICAL_t logical;
	PK_VECTOR_t p[4];
	PK_VECTOR_t normals[2];
	PK_UV_t uv;

	PK_EDGE_ask_convexity_o_t convexity_opts;
	PK_emboss_convexity_t convexity;

	error_code = PK_BODY_ask_edges(body, &n_edges, &edges);
	if (n_edges != 0)
	{
		PK_EDGE_ask_convexity_o_m(convexity_opts);

		// Find all of the concave edges by looping through every edge on the body.
		for (int i = 0; i < n_edges; i++)
		{

			error_code = PK_EDGE_ask_convexity(edges[i], &convexity_opts, &convexity);

			if (convexity == PK_EDGE_convexity_concave_c)
			{
				error_code = PK_EDGE_ask_faces(edges[i], &n_faces, &faces);
				if (2 == n_faces)
				{
					for (int j = 0; j < n_faces; j++)
					{
						error_code = PK_FACE_ask_oriented_surf(faces[j], &surface, &logical);

						uv.param[0] = 0;
						uv.param[1] = 0;
						error_code = PK_SURF_eval_with_normal(surface, uv, 1, 1, PK_LOGICAL_false, p, &normals[j]);
					}

					double dot = (normals[0].coord[0] * normals[1].coord[0]) + (normals[0].coord[1] * normals[1].coord[1]) + (normals[0].coord[2] * normals[1].coord[2]);
					double angle = acos(dot) / PI * 180;

					if (dMinAng < angle && angle < dMaxAng)
						floatArray.push_back(float(edges[i]));
				}
			}
		}

		// Free memory.
		PK_MEMORY_free(edges);
		PK_MEMORY_free(faces);
	}
	
	return floatArray;
}

void CPsProcess::ComputeCollision(const int target, std::vector<float>& collisionArray)
{
	PK_ERROR_code_t error_code;

	// Find top assem
	PK_ASSEMBLY_t topAssem = PK_ENTITY_null;
	topAssem = findTopAssy();

	if (PK_ENTITY_null == topAssem)
		return;

	// Make working partation
	PK_PARTITION_t partition;
	error_code = PK_PARTITION_create_empty(&partition);
	error_code = PK_PARTITION_set_current(partition);

	// Get flat body - net matrix list
	PK_TRANSF_sf_t currentMatrix;
	setDefaultMatrix(currentMatrix);

	m_bodyInstance_netMatrix.clear();
	traverseInstance(topAssem, currentMatrix, NULL);

	int targetId = -1;
	std::vector<PK_BODY_t> aPkBody;
	std::vector<PK_TRANSF_t> aPkTransf;

	for (auto itr = m_bodyInstance_netMatrix.begin(); itr != m_bodyInstance_netMatrix.end(); ++itr)
	{
		if (target == itr->first)
			targetId = std::distance(m_bodyInstance_netMatrix.begin(), itr);

		leafInfo leaf = itr->second;
		aPkBody.push_back(leaf.body);

		PK_TRANSF_t transf;
		PK_TRANSF_create(&leaf.netMatrix, &transf);
		aPkTransf.push_back(transf);
	}

	int i_bodies = m_bodyInstance_netMatrix.size() - 1;
	PK_BODY_t* bodies = new PK_BODY_t[i_bodies];
	PK_TRANSF_t* transfs = new PK_TRANSF_t[i_bodies];

	int cnt = 0;
	for (int i = 0; i < aPkBody.size(); i++)
	{
		if (i != targetId)
		{
			if (i_bodies <= cnt)
				return;

			bodies[cnt] = aPkBody[i];
			transfs[cnt] = aPkTransf[i];
			cnt++;
		}
	}

	PK_TOPOL_clash_o_t clash_opts;
	PK_TOPOL_clash_o_m(clash_opts);
	clash_opts.mul_tool_tf = PK_LOGICAL_true;

	int n_clashes = 0;
	PK_TOPOL_clash_t* clashes = NULL;

#ifdef _DEBUG
	//PK_PART_t 
	//PK_SESSION_set_journalling(PK_LOGICAL_false);
	//PK_DEBUG_report_start_o_t debugRep;
	//PK_DEBUG_report_start_o_m(debugRep);
	//error_code = PK_DEBUG_report_start("C:\\temp\\pk_debug.xml", &debugRep);
#endif

	// Fast crash test
	error_code = PK_TOPOL_clash(1, &aPkBody[targetId], &aPkTransf[targetId], i_bodies, bodies, transfs, &clash_opts, &n_clashes, &clashes);

#ifdef _DEBUG
	//error_code = PK_DEBUG_report_stop();
#endif

	if (n_clashes)
	{
		// Compute clash with specific opitons
		clash_opts.find_all = PK_LOGICAL_true;
		clash_opts.find_intersect = PK_LOGICAL_true;
		error_code = PK_TOPOL_clash(1, &aPkBody[targetId], &aPkTransf[targetId], i_bodies, bodies, transfs, &clash_opts, &n_clashes, &clashes);

		std::vector<PK_BODY_t> clash_bodies;
		std::vector<PK_BODY_t> touch_bodies;
		std::vector<int> clash_bodie_id;
		std::vector<int> touch_bodie_id;
		for (int i = 0; i < n_clashes; i++)
		{
			PK_TOPOL_clash_t clash = clashes[i];
			PK_TOPOL_clash_type_t clash_type = clash.clash_type;

			PK_TOPOL_t target = clash.target;
			PK_BODY_t target_body;
			error_code = PK_FACE_ask_body(target, &target_body);

			PK_TOPOL_t tool = clash.tool;
			PK_BODY_t tool_body;
			error_code = PK_FACE_ask_body(tool, &tool_body);

			if (PK_TOPOL_clash_interfere == clash_type || PK_TOPOL_clash_a_in_b == clash_type || PK_TOPOL_clash_b_in_a == clash_type)
			{
				if (PK_TOPOL_clash_a_in_b == clash_type || PK_TOPOL_clash_b_in_a == clash_type)
					tool_body = clash.tool;

				// Find target bodies;
				auto result = std::find(clash_bodies.begin(), clash_bodies.end(), tool_body);
				if (clash_bodies.end() == result)
				{
					clash_bodies.push_back(tool_body);

					// Get body id
					auto itr = std::find(aPkBody.begin(), aPkBody.end(), tool_body);
					const int id = std::distance(aPkBody.begin(), itr);
					clash_bodie_id.push_back(id);
				}
			}
			//else if (PK_TOPOL_clash_abut_no_class == clash_type)
			//{
			//	PK_CLASS_t pk_class;
			//	error_code = PK_ENTITY_ask_class(clash.tool, &pk_class);

			//	// Find target bodies;
			//	auto result = std::find(touch_bodies.begin(), touch_bodies.end(), tool_body);
			//	if (touch_bodies.end() == result)
			//	{
			//		touch_bodies.push_back(tool_body);

			//		// Get body id
			//		auto itr = std::find(aPkBody.begin(), aPkBody.end(), tool_body);
			//		const int id = std::distance(aPkBody.begin(), itr);
			//		touch_bodie_id.push_back(id);
			//	}
			//}
		}


		// Create clash intersection bodies
		if (clash_bodies.size())
		{
			// Copy target body
			PK_ENTITY_copy_o_t copy_opts;
			PK_ENTITY_copy_o_m(copy_opts);
			PK_BODY_t target_body;
			PK_ENTITY_track_r_t entity_tracking;
			error_code = PK_ENTITY_copy_2(aPkBody[targetId], &copy_opts, &target_body, &entity_tracking);
			error_code = PK_ENTITY_track_r_f(&entity_tracking);

			// Set transform
			PK_BODY_transform_o_t transf_opts;
			PK_BODY_transform_o_m(transf_opts);
			PK_TOPOL_track_r_t topol_tracking;
			PK_TOPOL_local_r_t topol_results;
			error_code = PK_BODY_transform_2(target_body, aPkTransf[targetId], 1.0e-06, &transf_opts, &topol_tracking, &topol_results);
			error_code = PK_TOPOL_track_r_f(&topol_tracking);
			error_code = PK_TOPOL_local_r_f(&topol_results);

			// Copy tool bodies
			PK_BODY_t* tool_bodies = new PK_BODY_t[clash_bodies.size()];
			for (int i = 0; i < clash_bodies.size(); i++)
			{
				error_code = PK_ENTITY_copy_2(clash_bodies[i], &copy_opts, &tool_bodies[i], &entity_tracking);
				error_code = PK_ENTITY_track_r_f(&entity_tracking);

				// Set transform
				error_code = PK_BODY_transform_2(tool_bodies[i], aPkTransf[clash_bodie_id[i]], 1.0e-06, &transf_opts, &topol_tracking, &topol_results);
				error_code = PK_TOPOL_track_r_f(&topol_tracking);
				error_code = PK_TOPOL_local_r_f(&topol_results);
			}

			PK_BODY_boolean_o_t bool_opts;
			PK_BODY_boolean_o_m(bool_opts);
			bool_opts.function = PK_boolean_intersect_c;
			PK_TOPOL_track_r_t tracking;
			PK_boolean_r_t results;
			error_code = PK_BODY_boolean_2(target_body, clash_bodies.size(), tool_bodies, &bool_opts, &tracking, &results);

			if (PK_ERROR_no_errors == error_code && 0 < results.n_bodies)
			{
				collisionArray[0] = results.n_bodies;

				// Get facet data
				for (int i = 0; i < results.n_bodies; i++)
				{
					std::vector<float> floatArr = PsSolidToFloatArray(results.bodies[i]);

					collisionArray.push_back(floatArr.size());

					std::copy(floatArr.begin(), floatArr.end(), std::back_inserter(collisionArray));
				}
#ifdef _DEBUG
				//{
				//	char  *path = "C:\\temp\\debug";
				//	PK_PART_transmit_o_t transmit_opts;
				//	PK_PART_transmit_o_m(transmit_opts);
				//	transmit_opts.transmit_format = PK_transmit_format_text_c;

				//	error_code = PK_PART_transmit(results.n_bodies, results.bodies, path, &transmit_opts);
				//}
#endif
			}
			else
			{
				collisionArray[0] = 0;
				collisionArray.push_back(0);
			}

			// Delete copy body
			error_code = PK_ENTITY_delete(results.n_bodies, results.bodies);

			//Clear tracking and results.
			PK_TOPOL_track_r_f(&tracking);
			PK_boolean_r_f(&results);

		}
		else
		{
			collisionArray[0] = 0;
			collisionArray.push_back(0);
		}
	}
	else
	{
		collisionArray[0] = 0;

		// Compute min distance if no collision
		PK_ENTITY_range_o_t entity_renge_opts;
		PK_ENTITY_range_o_m(entity_renge_opts);
		PK_ENTITY_range_r_t entity_renge_result;
		error_code = PK_ENTITY_range(1, &aPkBody[targetId], &aPkTransf[targetId], i_bodies, bodies, transfs, &entity_renge_opts, &entity_renge_result);

		if (PK_ERROR_no_errors == error_code)
		{
			collisionArray.push_back(entity_renge_result.distances[0] * 1000.0);

			// Transform vectors
			PK_VECTOR1_t orgVect;

			orgVect = entity_renge_result.ends_1[0].vector;
			PK_VECTOR1_t vect1;
			error_code = PK_VECTOR_transform(orgVect, aPkTransf[targetId], &vect1);

			orgVect = entity_renge_result.ends_2[0].vector;
			PK_VECTOR1_t vect2;
			error_code = PK_VECTOR_transform(orgVect, transfs[entity_renge_result.ends_2[0].index], &vect2);

			collisionArray.push_back(vect1.coord[0] * 1000.0);
			collisionArray.push_back(vect1.coord[1] * 1000.0);
			collisionArray.push_back(vect1.coord[2] * 1000.0);
			collisionArray.push_back(vect2.coord[0] * 1000.0);
			collisionArray.push_back(vect2.coord[1] * 1000.0);
			collisionArray.push_back(vect2.coord[2] * 1000.0);
		}
	}

	//error_code = PK_MEMORY_free(bodies);
	//error_code = PK_MEMORY_free(transfs);

	// Return to main partition and delete working patation
	error_code = PK_PARTITION_set_current(m_pPsSession->GetPkPartition());

	PK_PARTITION_delete_o_t delete_opts;
	PK_PARTITION_delete_o_m(delete_opts);
	delete_opts.delete_non_empty = PK_LOGICAL_true;
	error_code = PK_PARTITION_delete(partition, &delete_opts);

}

void CPsProcess::ComputeCollision(std::vector<float>& collisionArray)
{
	PK_ERROR_code_t error_code;
	collisionArray[0] = 0;

	// Find top assem
	PK_ASSEMBLY_t topAssem = PK_ENTITY_null;
	topAssem = findTopAssy();

	if (PK_ENTITY_null == topAssem)
		return;
	
	// Make working partation
	PK_PARTITION_t partition;
	error_code = PK_PARTITION_create_empty(&partition);
	error_code = PK_PARTITION_set_current(partition);

	// Get flat body - net matrix list
	PK_TRANSF_sf_t currentMatrix;
	setDefaultMatrix(currentMatrix);

	m_bodyInstance_netMatrix.clear();
	traverseInstance(topAssem, currentMatrix, NULL);

	std::vector<PK_BODY_t> aPkBody;
	std::vector<PK_TRANSF_t> aPkTransf;

	for (auto itr = m_bodyInstance_netMatrix.begin(); itr != m_bodyInstance_netMatrix.end(); ++itr)
	{
		leafInfo leaf = itr->second;
		aPkBody.push_back(leaf.body);

		PK_TRANSF_t transf;
		PK_TRANSF_create(&leaf.netMatrix, &transf);
		aPkTransf.push_back(transf);
	}

	int to_collision = 0;

	PK_TOPOL_clash_o_t clash_opts;
	PK_TOPOL_clash_o_m(clash_opts);
	clash_opts.mul_tool_tf = PK_LOGICAL_true;
	clash_opts.find_all = PK_LOGICAL_true;
	clash_opts.find_intersect = PK_LOGICAL_true;

	for (int ii = 0; ii < aPkBody.size(); ii++)
	{
		int i_target = ii;
		int i_bodies = aPkBody.size() - 1 - ii;
		PK_BODY_t* bodies = new PK_BODY_t[i_bodies];
		PK_TRANSF_t* transfs = new PK_TRANSF_t[i_bodies];

		int cnt = 0;
		for (int jj = ii + 1; jj < aPkBody.size(); jj++)
		{
			bodies[cnt] = aPkBody[jj];
			transfs[cnt] = aPkTransf[jj];
			cnt++;
		}

		int n_clashes = 0;
		PK_TOPOL_clash_t* clashes = NULL;

		error_code = PK_TOPOL_clash(1, &aPkBody[i_target], &aPkTransf[i_target], i_bodies, bodies, transfs, &clash_opts, &n_clashes, &clashes);

		// Get clash bodies
		std::vector<PK_BODY_t> clash_bodies;
		std::vector<int> clash_bodie_id;
		for (int i = 0; i < n_clashes; i++)
		{
			PK_TOPOL_clash_t clash = clashes[i];
			PK_TOPOL_clash_type_t clash_type = clash.clash_type;

			PK_TOPOL_t target = clash.target;
			PK_BODY_t target_body;
			error_code = PK_FACE_ask_body(target, &target_body);

			PK_TOPOL_t tool = clash.tool;
			PK_BODY_t tool_body;
			error_code = PK_FACE_ask_body(tool, &tool_body);

			if (PK_TOPOL_clash_interfere == clash_type || PK_TOPOL_clash_a_in_b == clash_type || PK_TOPOL_clash_b_in_a == clash_type)
			{
				if (PK_TOPOL_clash_a_in_b == clash_type || PK_TOPOL_clash_b_in_a == clash_type)
					tool_body = clash.tool;

				// Find target bodies;
				auto result = std::find(clash_bodies.begin(), clash_bodies.end(), tool_body);
				if (clash_bodies.end() == result)
				{
					clash_bodies.push_back(tool_body);

					// Get body id
					auto itr = std::find(aPkBody.begin(), aPkBody.end(), tool_body);
					const int id = std::distance(aPkBody.begin(), itr);
					clash_bodie_id.push_back(id);
				}
			}
		}

		// Create clash intersection bodies
		if (clash_bodies.size())
		{
			// Copy target body
			PK_ENTITY_copy_o_t copy_opts;
			PK_ENTITY_copy_o_m(copy_opts);
			PK_BODY_t target_body;
			PK_ENTITY_track_r_t entity_tracking;
			error_code = PK_ENTITY_copy_2(aPkBody[i_target], &copy_opts, &target_body, &entity_tracking);
			error_code = PK_ENTITY_track_r_f(&entity_tracking);

			// Set transform
			if (PK_ENTITY_null != aPkTransf[i_target])
			{
				PK_BODY_transform_o_t transf_opts;
				PK_BODY_transform_o_m(transf_opts);
				PK_TOPOL_track_r_t topol_tracking;
				PK_TOPOL_local_r_t topol_results;
				error_code = PK_BODY_transform_2(target_body, aPkTransf[i_target], 1.0e-06, &transf_opts, &topol_tracking, &topol_results);
				error_code = PK_TOPOL_track_r_f(&topol_tracking);
				error_code = PK_TOPOL_local_r_f(&topol_results);
			}

			// Copy tool bodies
			PK_BODY_t* tool_bodies = new PK_BODY_t[clash_bodies.size()];
			for (int i = 0; i < clash_bodies.size(); i++)
			{
				error_code = PK_ENTITY_copy_2(clash_bodies[i], &copy_opts, &tool_bodies[i], &entity_tracking);
				error_code = PK_ENTITY_track_r_f(&entity_tracking);

				// Set transform
				if (PK_ENTITY_null != aPkTransf[clash_bodie_id[i]])
				{
					PK_BODY_transform_o_t transf_opts;
					PK_BODY_transform_o_m(transf_opts);
					PK_TOPOL_track_r_t topol_tracking;
					PK_TOPOL_local_r_t topol_results;
					error_code = PK_BODY_transform_2(tool_bodies[i], aPkTransf[clash_bodie_id[i]], 1.0e-06, &transf_opts, &topol_tracking, &topol_results);
					error_code = PK_TOPOL_track_r_f(&topol_tracking);
					error_code = PK_TOPOL_local_r_f(&topol_results);
				}
			}

			PK_BODY_boolean_o_t bool_opts;
			PK_BODY_boolean_o_m(bool_opts);
			bool_opts.function = PK_boolean_intersect_c;
			PK_TOPOL_track_r_t tracking;
			PK_boolean_r_t results;
			error_code = PK_BODY_boolean_2(target_body, clash_bodies.size(), tool_bodies, &bool_opts, &tracking, &results);

			if (PK_ERROR_no_errors == error_code && 0 < results.n_bodies)
			{
				collisionArray[0] += results.n_bodies;

				// Get facet data
				for (int i = 0; i < results.n_bodies; i++)
				{
					std::vector<float> floatArr = PsSolidToFloatArray(results.bodies[i]);

					collisionArray.push_back(floatArr.size());

					std::copy(floatArr.begin(), floatArr.end(), std::back_inserter(collisionArray));
				}
#ifdef _DEBUG
				//{
				//	char  *path = "C:\\temp\\debug";
				//	PK_PART_transmit_o_t transmit_opts;
				//	PK_PART_transmit_o_m(transmit_opts);
				//	transmit_opts.transmit_format = PK_transmit_format_text_c;

				//	error_code = PK_PART_transmit(results.n_bodies, results.bodies, path, &transmit_opts);
				//}
#endif
			}

			// Delete copy body
			error_code = PK_ENTITY_delete(results.n_bodies, results.bodies);

			//Clear tracking and results.
			PK_TOPOL_track_r_f(&tracking);
			PK_boolean_r_f(&results);

		}
	}

	if (1 == collisionArray.size())
		collisionArray.push_back(0);

	// Return to main partition and delete working patation
	error_code = PK_PARTITION_set_current(m_pPsSession->GetPkPartition());

	PK_PARTITION_delete_o_t delete_opts;
	PK_PARTITION_delete_o_m(delete_opts);
	delete_opts.delete_non_empty = PK_LOGICAL_true;
	error_code = PK_PARTITION_delete(partition, &delete_opts);
}

std::vector<float> CPsProcess::ComputeSilhouette(const double* ray, const double* pos)
{
	PK_ERROR_code_t error_code;

	std::vector<float> foloatArr;

	// Find top assem
	PK_ASSEMBLY_t topAssem = PK_ENTITY_null;
	topAssem = findTopAssy();

	if (PK_ENTITY_null == topAssem)
		return foloatArr;

	// Make working partation
	PK_PARTITION_t partition;
	error_code = PK_PARTITION_create_empty(&partition);
	error_code = PK_PARTITION_set_current(partition);

	PK_VECTOR1_t view_direction = { ray[0], ray[1], ray[2] };

	// Create outline
	PK_BODY_make_curves_outline_o_s outline_opt;
	PK_BODY_make_curves_outline_o_m(outline_opt);
	outline_opt.project = PK_outline_project_plane_c;
	PK_VECTOR1_t posiiton = { pos[0] / 1000, pos[1] / 1000, pos[2] / 1000
};
	outline_opt.project_position = posiiton;
	outline_opt.want_body = PK_LOGICAL_true;
	outline_opt.tolerance = 1.0e-5;
	outline_opt.body_dimension = PK_TOPOL_dimension_2_c;

	int n_curves = 0;
	PK_CURVE_t* curves;
	PK_INTERVAL_t* intervals;
	PK_TOPOL_t* topols;
	int* outlines;
	double* curve_tolerance;
	double max_separation;

	// Get flat body - net matrix list
	PK_TRANSF_sf_t currentMatrix;
	setDefaultMatrix(currentMatrix);

	m_bodyInstance_netMatrix.clear();
	traverseInstance(topAssem, currentMatrix, NULL);

	int n_body = m_bodyInstance_netMatrix.size();

	PK_BODY_t* bodies = new PK_BODY_t[n_body];
	PK_TRANSF_t* transfs = new PK_TRANSF_t[n_body];

	int counter = 0;
	for (auto itr = m_bodyInstance_netMatrix.begin(); itr != m_bodyInstance_netMatrix.end(); ++itr)
	{
		leafInfo leaf = itr->second;

		PK_TRANSF_t transf;
		PK_TRANSF_create(&leaf.netMatrix, &transf);

		bodies[counter] = leaf.body;
		transfs[counter] = transf;

		counter++;
	}

	error_code = PK_BODY_make_curves_outline(n_body, bodies, transfs, view_direction, &outline_opt, &n_curves, &curves, &intervals, &topols, &outlines, &curve_tolerance, &max_separation);

	if (PK_ERROR_no_errors == error_code && n_curves)
	{
		int n_parts;
		PK_PART_t* parts;
		error_code = PK_PARTITION_ask_bodies(partition, &n_parts, &parts);

		PK_BODY_t body = parts[0];

		// Get facet data
		foloatArr = PsSolidToFloatArray(body);

		// Get surface area
		PK_TOPOL_eval_mass_props_o_t mass_opts;
		PK_TOPOL_eval_mass_props_o_m(mass_opts);
		mass_opts.mass = PK_mass_c_of_g_c;
		double amount[1], mass[1], c_of_g[3], m_of_i[9], periphery[1];

		error_code = PK_TOPOL_eval_mass_props(1, &body, 1, &mass_opts, amount, mass, c_of_g, m_of_i, periphery);

		foloatArr.push_back(amount[0]);


#ifdef _DEBUG
		{
			//char  *path = "C:\\temp\\debug";
			//PK_PART_transmit_o_t transmit_opts;
			//PK_PART_transmit_o_m(transmit_opts);
			//transmit_opts.transmit_format = PK_transmit_format_text_c;

			//error_code = PK_PART_transmit(1, &body, path, &transmit_opts);
		}
#endif

		error_code = PK_ENTITY_delete(1, &body);

	}

	// Return to main partition and delete working patation
	error_code = PK_PARTITION_set_current(m_pPsSession->GetPkPartition());

	PK_PARTITION_delete_o_t delete_opts;
	PK_PARTITION_delete_o_m(delete_opts);
	delete_opts.delete_non_empty = PK_LOGICAL_true;
	error_code = PK_PARTITION_delete(partition, &delete_opts);

	return foloatArr;
}

std::vector<float> CPsProcess::EdgeInfo(const PK_EDGE_t edge)
{
	std::vector<float> floatArray;
	PK_ERROR_code_t error_code;
	PK_CPCURVE_t curve;
	PK_CLASS_t curve_class;
	PK_CIRCLE_sf_s circle;
	double rad = 0;

	PK_EDGE_ask_curve(edge, &curve);
	PK_ENTITY_ask_class(curve, &curve_class);

	if (PK_CLASS_circle == curve_class)
	{
		floatArray.push_back(PK_CLASS_circle);

		error_code = PK_CIRCLE_ask(curve, &circle);

		rad = circle.radius;
		floatArray.push_back(float(rad * m_dScale));
		floatArray.push_back(float(circle.basis_set.location.coord[0] * m_dScale));
		floatArray.push_back(float(circle.basis_set.location.coord[1] * m_dScale));
		floatArray.push_back(float(circle.basis_set.location.coord[2] * m_dScale));
		floatArray.push_back(float(circle.basis_set.axis.coord[0]));
		floatArray.push_back(float(circle.basis_set.axis.coord[1]));
		floatArray.push_back(float(circle.basis_set.axis.coord[2]));
	}

	return floatArray;
}

int ExportViaExchange(const int n_parts, const PK_PART_t *parts, const char *filePath)
{
	CExProcess exProcess = CExProcess();

	return exProcess.ExportFile(n_parts, parts, filePath);
}

int CPsProcess::SaveCAD(std::string activeSession, const char cFormat)
{
	PK_ERROR_code_t error_code;

	int n_parts = 0;
	PK_PART_t *parts = NULL;

	int n_assy = 0;
	PK_ASSEMBLY_t *assems;
	PK_PARTITION_t partition;

	PK_SESSION_ask_curr_partition(&partition);
	error_code = PK_PARTITION_ask_assemblies(partition, &n_assy, &assems);

	if (0 < n_assy)
	{
		PK_ASSEMBLY_t topAssy = findTopAssy();

		if (PK_ENTITY_null != topAssy)
		{
			n_parts = 1;
			parts = new PK_PART_t[1];
			parts[0] = topAssy;
		}
	}
	else
	{
		PK_SESSION_ask_parts(&n_parts, &parts);
	}

	if (0 < n_parts)
	{
		PK_PART_transmit_o_t transmit_opts;
		PK_PART_transmit_o_m(transmit_opts);
		transmit_opts.transmit_format = PK_transmit_format_text_c;
		transmit_opts.transmit_version = 320;

		switch (cFormat)
		{
		case 'X': 
		{
			sprintf(m_pcFilePathForDL, "..\\%s.x_t", activeSession.c_str());
			error_code = PK_PART_transmit(n_parts, parts, m_pcFilePathForDL, &transmit_opts);

			if (PK_ERROR_no_errors == error_code)
				return 0;
			else
			{
				m_pPsSession->CheckAndHandleErrors();
				return -1;
			}
		} break;
		case 'S': 
		{
			sprintf(m_pcFilePathForDL, "..\\%s.stp", activeSession.c_str());
			if (0 == ExportViaExchange(n_parts, parts, m_pcFilePathForDL))
				return 0;

		} break;
		case 'P': 
		{
			sprintf(m_pcFilePathForDL, "..\\%s.prc", activeSession.c_str());
			if (0 == ExportViaExchange(n_parts, parts, m_pcFilePathForDL))
				return 0;
		} break;
		default: break;
		}


	}

	return -1;
}

void CPsProcess::Downloaded()
{
	if (strlen(m_pcFilePathForDL))
	{
		remove(m_pcFilePathForDL);
		m_pcFilePathForDL[0] = '\0';
	}
}

int CPsProcess::BodySave(PK_BODY_t body, std::string activeSession)
{
	PK_ERROR_code_t error_code;

	PK_PART_transmit_o_t transmit_opts;
	PK_PART_transmit_o_m(transmit_opts);
	transmit_opts.transmit_format = PK_transmit_format_text_c;

	error_code = PK_PART_transmit(1, &body, activeSession.c_str(), &transmit_opts);

	if (PK_ERROR_no_errors == error_code)
		return 0;
	else
	{
		m_pPsSession->CheckAndHandleErrors();
		return -1;
	}

	return -1;
}

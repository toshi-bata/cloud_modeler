#include "stdafx.h"

#include "PsSession.h"
#include "parasolid_kernel.h"
#include "frustrum_ifails.h"
#include <stdlib.h>
#include <string>

// The following are frustrum function declarations

extern void StartFileFrustrum(int *);
extern void AbortFrustrum(int *);
extern void StopFileFrustrum(int *);
extern void OpenReadFrustrumFile(const int *, const int *, const char *, const int *,
	const int *, int *, int *);
extern void OpenWriteFrustrumFile(const int *, const int *, const char *, const int *,
	const char *, const int *, int *, int *);
extern void CloseFrustrumFile(const int *, const int *, const int *, int *);
extern void ReadFromFrustrumFile(const int *, const int *, const int *, char *, int *,
	int *);
extern void WriteToFrustrumFile(const int *, const int *, const int *, const char *,
	int *);

// The following are for the delta frustrum

extern "C" {
	PK_ERROR_code_t FRU_delta_open_for_write(PK_PMARK_t, PK_DELTA_t *);
	PK_ERROR_code_t FRU_delta_open_for_read(PK_DELTA_t);
	PK_ERROR_code_t FRU_delta_write(PK_DELTA_t, unsigned, const char *);
	PK_ERROR_code_t FRU_delta_read(PK_DELTA_t, unsigned, char *);
	PK_ERROR_code_t FRU_delta_delete(PK_DELTA_t);
	PK_ERROR_code_t FRU_delta_close(PK_DELTA_t);
	int FRU__delta_init(int action);
}

CPsSession::CPsSession(const char *workingDir)
	:m_pcWorkingDir(workingDir)
{
}


CPsSession::~CPsSession()
{
	// Close down Parasolid
	Terminate();
}

// Starts up Parasolid Session, returns true is successful, false otherwise.

bool CPsSession::Init()
{
	PK_SESSION_frustrum_t fru;
	PK_SESSION_frustrum_o_m(fru);

	fru.fstart = StartFrustrum;
	fru.fabort = AbortFrustrum;
	fru.fstop = StopFrustrum;
	fru.fmallo = GetMemory;
	fru.fmfree = ReturnMemory;
	fru.ffoprd = OpenReadFrustrumFile;
	fru.ffopwr = OpenWriteFrustrumFile;
	fru.ffclos = CloseFrustrumFile;
	fru.ffread = ReadFromFrustrumFile;
	fru.ffwrit = WriteToFrustrumFile;

	PK_SESSION_register_frustrum(&fru);

	// Register Delta Frustrum

	PK_DELTA_frustrum_t delta_fru;

	delta_fru.open_for_write_fn = FRU_delta_open_for_write;
	delta_fru.open_for_read_fn = FRU_delta_open_for_read;
	delta_fru.close_fn = FRU_delta_close;
	delta_fru.write_fn = FRU_delta_write;
	delta_fru.read_fn = FRU_delta_read;
	delta_fru.delete_fn = FRU_delta_delete;

	PK_DELTA_register_callbacks(delta_fru);
	// Register Error Handler
	PK_ERROR_frustrum_t errorFru;
	errorFru.handler_fn = PKerrorHandler;
	PK_ERROR_register_callbacks(errorFru);

	// Starts the modeller

	PK_SESSION_start_o_t options;
	PK_SESSION_start_o_m(options);

	// By default session journalling is turned off, to enable set options.journal_file 
	// to where you want this data to be written to.
	// eg. 
	// options.journal_file = "c:\\temp\\test";

	// PK_SESSION_start also initialises the following interface parameters:

	//		PK_SESSION_set_check_arguments    PK_LOGICAL_true
	//		PK_SESSION_set_check_self_int     PK_LOGICAL_true
	//		PK_SESSION_set_check_continuity   PK_LOGICAL_true
	//		PK_SESSION_set_general_topology   PK_LOGICAL_false
	//		PK_SESSION_set_swept_spun_surfs   PK_LOGICAL_false
	//		PK_SESSION_set_tag_limit          0 (ie: no limit)
	//		PK_SESSION_set_angle_precision    0.00000000001
	//		PK_SESSION_set_precision          0.00000001

	PK_SESSION_start(&options);
	PK_SESSION_set_roll_forward(true);

	PK_SESSION_ask_curr_partition(&m_defaultPartition);

	// Create current partation
	PK_PARTITION_create_empty(&m_currentPartition);
	PK_PARTITION_set_current(m_currentPartition);

	// Check to see if it all started up OK
	PK_LOGICAL_t was_error = PK_LOGICAL_true;
	PK_ERROR_sf_t error_sf;
	PK_ERROR_ask_last(&was_error, &error_sf);
	if (was_error)
		return false;

	m_activeSession = "";

	return true;
}

void CPsSession::Terminate()
{
	PK_SESSION_stop();
	return;
}

void CPsSession::StartFrustrum(int * ifail)
{
	*ifail = FR_no_errors;
	FRU__delta_init(1);
	StartFileFrustrum(ifail);
}

void CPsSession::StopFrustrum(int * ifail)
{

	*ifail = FR_no_errors;
	FRU__delta_init(2);
	StopFileFrustrum(ifail);
}

void CPsSession::GetMemory(int * nBytes, char * * memory, int * ifail)
{

	*memory = new char[*nBytes];
	*ifail = (*memory) ? FR_no_errors : FR_memory_full;
}

void CPsSession::ReturnMemory(int * nBytes, char * * memory, int * ifail)
{
	delete[] * memory;
	*ifail = FR_no_errors;
}

PK_ERROR_code_t CPsSession::PKerrorHandler(PK_ERROR_sf_t* error)
{
	char text[500];
	sprintf_s(text, 500, "PK error: %s returned %s.", error->function,
		error->code_token);
	//AfxMessageBox(CString(text));
	return error->code;
}

// Utility function written to check for any errors and handle the errors according to their priorities
void CPsSession::CheckAndHandleErrors()
{
	// Declaration and initialisation of inputs for PK_THREAD_ask_last_error
	PK_LOGICAL_t was_error = PK_LOGICAL_false;
	PK_ERROR_sf_t error_sf;

	// Ask the last error returned in the current thread
	PK_THREAD_ask_last_error(&was_error, &error_sf);

	if (was_error == PK_LOGICAL_true)
	{
		char DisplayMessage[300] = "";

		// Add message to DisplayMessage using DisplayMessage by concatenate two strings using strcat_s method
		strcat_s(DisplayMessage, "Parasolid has returned an error\nFunction: ");

		// Concatenate function name to DisplayMessage
		strcat_s(DisplayMessage, error_sf.function);

		// Concatenate a string to DisplayMessage
		strcat_s(DisplayMessage, "\nError: ");

		// Concatenate error code token to DisplayMessage
		strcat_s(DisplayMessage, error_sf.code_token);

		// Checks whether the severity of the error is mild
		if (error_sf.severity == PK_ERROR_mild)
		{
			// MILD ERROR
			// No actions required in case of a mild error

			// Concatenate a string to DisplayMessage
			strcat_s(DisplayMessage, "\nSeverity: PK_ERROR_mild");

			// Display the details about the error in a message box
			printf(DisplayMessage);

			// Clears the elements of DisplayMessage char array
			memset(DisplayMessage, 0, strlen(DisplayMessage));

			// Display the message in a message box
			printf("Since this is a mild error no actions required");
		}

		// Checks whether the severity of the error is serious
		else if (error_sf.severity == PK_ERROR_serious)
		{
			// SERIOUS ERROR
			// Must roll back to a valid state of the model

			// Concatenate a string to DisplayMessage
			strcat_s(DisplayMessage, "\nSeverity: PK_ERROR_serious");

			// Display the details about the error in a message box
			printf(DisplayMessage);

			// Clears the elements of DisplayMessage char array
			memset(DisplayMessage, 0, strlen(DisplayMessage));

			// Display the message in a message box
			printf("Since this is a serious error roll back is required");

			// Declaration and initialisation of arguments for PK_SESSION_ask_curr_partition
			// 'parition' will hold the current partition
			PK_PARTITION_t partition = PK_ENTITY_null;

			// Ask current partition
			PK_SESSION_ask_curr_partition(&partition);

			// Declaration and initialisation of arguments for PK_PARTITION_ask_pmark
			// 'is_at_pmark' will indicate whether the partition is at a pmark
			PK_LOGICAL_t is_at_pmark = PK_LOGICAL_false;

			// 'pmark1' will hold the tag of the most recent set or rolled to pmark
			PK_PMARK_t pmark1 = PK_ENTITY_null;

			// Ask the most recent set or rolled to pmark in the current partition
			PK_PARTITION_ask_pmark(partition, &pmark1, &is_at_pmark);

			// Declaration and initialisation of PK_PMARK_goto returned arguments
			int n_new_entities = 0;
			PK_ENTITY_t *new_entities = NULL;
			int n_mod_entities = 0;
			PK_ENTITY_t *mod_entities = NULL;
			int n_del_entities = 0;
			PK_ENTITY_t *del_entities = NULL;

			// Rolling back to the state where most recent pmark is created in the current partition.
			// Note that PK_PMARK_goto is a deprecated function, any new code should use the function that has superseded it.
			PK_PMARK_goto(pmark1, &n_new_entities, &new_entities, &n_mod_entities, &mod_entities, &n_del_entities, &del_entities);

			// Free up memory used by PK_PMARK_goto returned arguments
			if (n_new_entities)
				PK_MEMORY_free(new_entities);
			if (n_mod_entities)
				PK_MEMORY_free(mod_entities);
			if (n_del_entities)
				PK_MEMORY_free(del_entities);

			// Add a message to DisplayMessage
			strcat_s(DisplayMessage, "Rolled back to the recent pmark");

			// Display the message in a message box
			printf(DisplayMessage);

			// Clears the elements of DisplayMessage char array
			memset(DisplayMessage, 0, strlen(DisplayMessage));

		}

		// Checks whether the severity of the error is fatal
		else if (error_sf.severity == PK_ERROR_fatal)
		{
			// FATAL ERROR
			// The parasolid session must be stopped and restarted

			// Concatenate a string to DisplayMessage
			strcat_s(DisplayMessage, "\nSeverity: PK_ERROR_fatal");

			// Display the details about the error in a message box
			printf(DisplayMessage);

			// Clears the elements of DisplayMessage char array
			memset(DisplayMessage, 0, strlen(DisplayMessage));

		}

		// If the severity is none of the above
		else
		{
			printf("\nSeverity Not Defined");
		}

		// Clears the last error in the current thread
		PK_THREAD_clear_last_error(&was_error);

		printf("\nThe error is handled and it is cleared ");
	}
	else
	{
		printf("\nNo errors raised ");
	}
}

int CPsSession::Reset()
{
	PK_ERROR_code_t error_code;
	
	error_code = PK_PARTITION_set_current(m_defaultPartition);

	// Delete current partition
	if (PK_ENTITY_null != m_currentPartition)
	{
		PK_PARTITION_delete_o_t delete_opts;
		PK_PARTITION_delete_o_m(delete_opts);
		delete_opts.delete_non_empty = PK_LOGICAL_true;
		error_code = PK_PARTITION_delete(m_currentPartition, &delete_opts);
	}

	// Create current partation
	error_code = PK_PARTITION_create_empty(&m_currentPartition);
	error_code = PK_PARTITION_set_current(m_currentPartition);

	// Reset marks
	m_psMarks[m_activeSession].iCurrentMark = -1;

	return SetPsMark();
}

int CPsSession::SetPsMark()
{
	PK_ERROR_code_t error_code;

	m_psMarks[m_activeSession].iCurrentMark++;

	while (m_psMarks[m_activeSession].iCurrentMark < m_psMarks[m_activeSession].marks.size()) {
		m_psMarks[m_activeSession].marks.pop_back();
	}

	PK_PMARK_t mark;
	error_code = PK_MARK_create(&mark);

	if (PK_ERROR_no_errors == error_code)
	{
		m_psMarks[m_activeSession].marks.push_back(mark);
		return 0;
	}
	else
	{
		CheckAndHandleErrors();
		return -1;
	}
}

int CPsSession::Undo()
{
	PK_ERROR_code_t error_code;
	PK_MARK_t mark = PK_ENTITY_null;

	int currentMark = m_psMarks[m_activeSession].iCurrentMark;
	currentMark--;

	if (0 > currentMark)
		return -1;

	m_psMarks[m_activeSession].iCurrentMark = currentMark;
	mark = m_psMarks[m_activeSession].marks[currentMark];

	if (PK_ENTITY_null != mark)
	{
		PK_MARK_goto_o_t goto_ots;
		PK_MARK_goto_r_t goto_result;
		PK_MARK_goto_o_m(goto_ots);

		error_code = PK_MARK_goto_2(mark, &goto_ots, &goto_result);

		if (PK_ERROR_no_errors != error_code)
		{
			CheckAndHandleErrors();
			return -1;
		}

		return 0;
	}

	return -1;
}

int CPsSession::Redo()
{
	PK_ERROR_code_t error_code;
	PK_MARK_t mark = PK_ENTITY_null;

	int currentMark = m_psMarks[m_activeSession].iCurrentMark;
	currentMark++;

	if (m_psMarks[m_activeSession].marks.size() <= currentMark)
		return -1;

	m_psMarks[m_activeSession].iCurrentMark = currentMark;
	mark = m_psMarks[m_activeSession].marks[currentMark];

	if (PK_ENTITY_null != mark)
	{
		PK_MARK_goto_o_t goto_ots;
		PK_MARK_goto_r_t goto_result;
		PK_MARK_goto_o_m(goto_ots);

		error_code = PK_MARK_goto_2(mark, &goto_ots, &goto_result);

		if (PK_ERROR_no_errors != error_code)
		{
			CheckAndHandleErrors();
			return -1;
		}

		return 0;
	}

	return -1;
}

int CPsSession::CreateNewSession(const char *activeSession)
{
	if (NULL == activeSession) {
		return -1;
	}

	m_activeSession = std::string(activeSession);

	// Create Mark array of the session if it is not there
	if (0 == m_psMarks.count(m_activeSession))
	{
		std::vector<PK_MARK_t> newMarks;
		markDic newEntry = { newMarks, -1 };
		m_psMarks.insert(m_psMarks.begin(), std::pair<std::string, markDic>(m_activeSession, newEntry));

		return SetPsMark();
	}
	else
	{
		return 0;
	}
}

int CPsSession::DumpActiveSession()
{
	PK_ERROR_code_t error_code;

	// If no session we have no need for marks
	if (m_activeSession.empty()) {
		return -1;
	}

	PK_SESSION_transmit_o_t transmit_opts;
	PK_SESSION_transmit_o_m(transmit_opts);
	transmit_opts.transmit_format = PK_transmit_format_text_c;
	transmit_opts.transmit_user_fields = false;
	transmit_opts.transmit_deltas = PK_PARTITION_xmt_deltas_all_c;
	transmit_opts.transmit_marks = PK_SESSION_xmt_marks_all_c;

	/* SAME THE CURRENTLY ACTIVE SESSION */
	char filePath[_MAX_PATH];
	sprintf_s(filePath, _MAX_PATH, "%s%s", m_pcWorkingDir, (m_activeSession + ".snp_txt").c_str());
	error_code = PK_SESSION_transmit(filePath, &transmit_opts);

	int n_parts;
	PK_PART_t *parts;
	PK_SESSION_ask_parts(&n_parts, &parts);
	if (n_parts > 0)
	{
		PK_ENTITY_delete(n_parts, parts);
		PK_MEMORY_free(parts);
		n_parts = 0;
	}

	if (PK_ERROR_no_errors == error_code)
	{
		printf("    Saving image for active session\n");
		return 0;
	}
	else
	{
		CheckAndHandleErrors();
		return -1;
	}
}

int CPsSession::LoadBackSession(std::string session)
{
	PK_ERROR_code_t error_code;

	PK_SESSION_receive_o_t receive_opts;
	PK_SESSION_receive_o_m(receive_opts);
	receive_opts.transmit_format = PK_transmit_format_text_c;

	char filePath[_MAX_PATH];
	sprintf_s(filePath, _MAX_PATH, "%s%s", m_pcWorkingDir, (session + ".snp_txt").c_str());
	error_code = PK_SESSION_receive(filePath, &receive_opts);

	if (PK_ERROR_no_errors == error_code)
	{
		m_activeSession = session;

		// Remove the dump file
		remove(filePath);

		printf("    Loading previously active session\n");
		return 0;
	}
	else
	{
		CheckAndHandleErrors();
		return -1;
	}
}

int CPsSession::GotoTheLastMark()
{
	PK_ERROR_code_t error_code;
	PK_MARK_t mark = PK_ENTITY_null;

	int currentMark = m_psMarks[m_activeSession].iCurrentMark;
	mark = m_psMarks[m_activeSession].marks[currentMark];

	if (PK_ENTITY_null != mark)
	{
		PK_MARK_goto_o_t goto_ots;
		PK_MARK_goto_r_t goto_result;
		PK_MARK_goto_o_m(goto_ots);

		error_code = PK_MARK_goto_2(mark, &goto_ots, &goto_result);

		if (PK_ERROR_no_errors != error_code)
		{
			CheckAndHandleErrors();
			return -1;
		}

		return 0;
	}

	return -1;
}
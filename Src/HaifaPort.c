#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <time.h>

//random time between 5 and 3000 msec between actions
#define MAX_SLEEP_TIME 3000 //Miliseconds (3 second)
#define MIN_SLEEP_TIME 5    //Miliseconds 

#define BUFFER_SIZE 50
#define STRING_SIZE 256
#define True 1
#define False 0

char buffer[BUFFER_SIZE];
DWORD written, read;
DWORD ThreadId;
DWORD WINAPI Vessel(PVOID);

HANDLE ReadHandleIn, WriteHandleIn, ReadHandleOut, WriteHandleOut;
HANDLE HaifaMutex, mutexPrint;

int* trdIDVessel;
HANDLE* SemOfVessel;
HANDLE* Vessels;

//Function declaration
int initGlobalData();
void cleanupGlobalData();
int calcSleepTime();
void Sails_to_Eilat(int);
const char* getTimeString();
void consolePrint(char*);
void PipesInitialization();
void EndOfAllThread();
void CreateVessThread();
void ReadAllThreadReturn();

int numOfVessel;
int correntVessInHaifa;
int currentVesselId;

SYSTEMTIME timeToPrint;//system time paramter to holds time data
STARTUPINFO si;
PROCESS_INFORMATION pi;
BOOL success;

int main(int argc, char *argv[])
{
	char convertToPrint[STRING_SIZE];
	if (argc != 2)
	{
		printf("Must Pass Positive Integer Parameters");
		exit(1);
	}
	numOfVessel = atoi(argv[1]);
	correntVessInHaifa = 0;

	if ((numOfVessel < 2) && (numOfVessel > 50))
	{
		printf("Arguments Error");
		exit(0);
	}
	
	TCHAR ProcessName[256];

	PipesInitialization();//Initialization the pipes and handles

	
	wcscpy(ProcessName, L"..\\..\\EilatPort\\Debug\\EilatPort.exe");
	/* create the child process */
	if (!CreateProcess(NULL, // No module name (use command line).
		ProcessName,// Command line.
		NULL, // Process handle not inheritable.
		NULL, // Thread handle not inheritable.
		TRUE, /* inherit handles */
		0, // No creation flags.
		NULL,  // Use parent's environment block.
		NULL,// Use parent's starting directory.
		&si,  // Pointer to STARTUPINFO structure.
		&pi)) // Pointer to PROCESS_INFORMATION structure.
	{
		fprintf(stderr, "Process Creation Failed\n");
		return -1;
	}
	printf("[%s] HaifaPort : Eilat Port connected\n", getTimeString());

	printf("[%s]HaifaPort: request  confirmation for %d Vessels \n", getTimeString(), numOfVessel);

	/* Haifa port now sends ships to the pipe(souze) */
	if (!WriteFile(WriteHandleIn, argv[1], BUFFER_SIZE, &written, NULL))
		fprintf(stderr, "Error writing to pipe\n");


	/* Read response from Eilat */
	success = ReadFile(ReadHandleOut, buffer, BUFFER_SIZE, &read, NULL);
	if (!success)
	{
		fprintf(stderr, "Haifa from Eilat: Error reading from pipe\n");
		return -1;
	}
	if (strcmp(buffer, "False")== 1)
		{
			printf("HaifaPort: request didn't approved  ,number of vaseals unvalid! \n");
			exit(0);
		}
	else
	{

		printf("[%s] HaifaPort: request approved  , continue... \n", getTimeString());
		if (initGlobalData(numOfVessel) == False)
		{
			printf("main::Unexpected Error in Global Semaphore Creation\n");
			return -1;
		}

		// Create all Vessels Threads. Report if Error occurred!
		CreateVessThread();
		// Read all Vessels Threads Return from Eilat. Report if Error occurred!
		ReadAllThreadReturn();
		
	}
	// wait for the child,Vessel and cleanupGlobalData.
	EndOfAllThread();

	exit(0);
	
}
//Thread function for each Vessel
DWORD WINAPI Vessel(PVOID Param)
{
	char convertToPrint[STRING_SIZE];
	srand((unsigned int)time(NULL));
	//Get the Unique ID from Param and keep it in a local variable
	int VesselID = *(int*)Param;

	sprintf(convertToPrint, "[%s] Vessel %d starts sailing @ Haifa Port \n", getTimeString(), VesselID);
	consolePrint(convertToPrint);

	Sleep(calcSleepTime());

	Sails_to_Eilat(VesselID);
	 
	return 0;
}
//function that send the vessels frome Haifa
void Sails_to_Eilat(int Vesselid) {
	char convertToPrint[STRING_SIZE];
	char message[BUFFER_SIZE];
	//Access to state DB is done exclusively (protected by mutex)
	//mutex.P()
	WaitForSingleObject(HaifaMutex, INFINITE);

	sprintf(message, "%d", Vesselid);
	sprintf(convertToPrint, "[%s] Vessel %d - entering Canal: Med. Sea ==> Red Sea \n", getTimeString(), Vesselid);
	consolePrint(convertToPrint);

	Sleep(calcSleepTime());

	/* the parent now wants to write to the pipe */
	if (!WriteFile(WriteHandleIn, message, BUFFER_SIZE, &written, NULL))
		fprintf(stderr, "Error writing to pipe \n %d \n", GetLastError());

	//mutex.V() - release mutex
	if (!ReleaseMutex(HaifaMutex))
		fprintf(stderr, "HaifaPort -> CrossToEilat::Unexpected error mutex.V(), error num - %d\n", GetLastError());

	//sem[Vesselid].P() - Down on Scheduling Constraint Semaphore for Vessel Vesselid.
	//if test was successful for it (i.e. it started sailing), then sem[Vesselid].P() is successfull and Vessel proceeds
	//otherwise, sem[Vesselid].P() puts Vessel on Semaphore Waiting queue, until signaled by a neighbour Vessel that 
	//finished return back!
	WaitForSingleObject(SemOfVessel[Vesselid - 1], INFINITE);
	sprintf(convertToPrint, "[%s] Vessel %d done sailing @ Haifa Port \n", getTimeString(), Vesselid);
	consolePrint(convertToPrint);
}
//generic function to randomise a Sleep time between MIN_SLEEP_TIME and MAX_SLEEP_TIME msec
int calcSleepTime() {
	srand((unsigned int)time(NULL));
	int sleepTime = rand() % (MAX_SLEEP_TIME)+ MIN_SLEEP_TIME;
	return sleepTime;
}
//Write protection function of two Process.
void consolePrint(char *string)
{
	WaitForSingleObject(mutexPrint, INFINITE);
	fprintf(stderr, "%s", string);
	if (!ReleaseMutex(mutexPrint))
		printf("PrintToConsole::Unexpected error mutex.V()\n");
}
//generic function to print system time information
const char* getTimeString()
{
	GetLocalTime(&timeToPrint);
	static char currentLocalTime[20];
	sprintf(currentLocalTime, "%02d:%02d:%02d", timeToPrint.wHour, timeToPrint.wMinute, timeToPrint.wSecond);
	return currentLocalTime;

}
//Function to Initialise global data (mainly for creating the 
//Semaphore Haldles before all Threads start .
int initGlobalData() {

	HaifaMutex = CreateMutex(NULL, FALSE, NULL);
	if (HaifaMutex == NULL)
	{
		printf("initGlobalData::Unexpected Error in HaifaMutex Creation\n");
		return False;
	}
	trdIDVessel = (int*)malloc(numOfVessel * sizeof(int));
	if (trdIDVessel == NULL)
	{
		fprintf(stderr, "Allocating trdIDVessel\n");
		return False;
	}
	SemOfVessel = (HANDLE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (numOfVessel * sizeof(HANDLE)));
	if (SemOfVessel == NULL)
	{
		fprintf(stderr, "Allocating SemOfVessel\n");
		return False;
	}
	Vessels = (HANDLE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (numOfVessel * sizeof(HANDLE)));
	if (Vessels == NULL)
	{
		fprintf(stderr, "Allocating Vessels\n");
		return False;
	}

	for (int i = 0; i < numOfVessel; i++)
	{

		SemOfVessel[i] = CreateSemaphore(NULL, 0, 1, NULL);
		if (SemOfVessel[i] == NULL)
		{
			printf("initGlobalData::Unexpected Error in SemOfVessel Creation\n");
			return False;
		}
	}
	return True;
}
//Function to clean global data and closing Threads after all Threads finish.
void cleanupGlobalData() {
	int i;
	CloseHandle(HaifaMutex);
	CloseHandle(mutexPrint);

	for (i = 0; i < numOfVessel; i++)
	{
		CloseHandle(SemOfVessel[i]);
		CloseHandle(Vessels[i]);
	}
	free(trdIDVessel);

	HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, Vessels);
	HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, SemOfVessel);

	

}
//Initialization the pipes and handles
void PipesInitialization()
{

	/* set up security attributes so that pipe handles are inherited */
	SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL,TRUE };
	ZeroMemory(&pi, sizeof(pi));
	/* allocate memory */

	mutexPrint = CreateMutex(NULL, FALSE, TEXT("ConsoleMutex"));
	if (mutexPrint == NULL)
	{
		return False;
	}
	/* create the pipe of Haifa */
	if (!CreatePipe(&ReadHandleIn, &WriteHandleIn, &sa, 0)) {
		fprintf(stderr, "Create Haifa to Eilat Pipe Failed\n");
		return 1;
	}
	// Ensure the write handle to the pipe for StdinWrite is not inherited. 
	if (!SetHandleInformation(WriteHandleIn, HANDLE_FLAG_INHERIT, 0))
		fprintf(stderr, "WriteHandleIn SetHandleInformation\n");

	/* create the pipe Eilat*/
	if (!CreatePipe(&ReadHandleOut, &WriteHandleOut, &sa, 0)) {
		fprintf(stderr, "Create Eilat to Haifa Pipe Failed\n");
		return 1;
	}

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	if (!SetHandleInformation(ReadHandleOut, HANDLE_FLAG_INHERIT, 0))
		fprintf(stderr, "StdoutRead SetHandleInformation\n");

	/* establish the START_INFO structure for the child process */
	GetStartupInfo(&si);
	si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	si.hStdOutput = WriteHandleOut;

	/* redirect the standard input to the read end of the pipe */
	si.hStdInput = ReadHandleIn;
	si.dwFlags = STARTF_USESTDHANDLES;

}
// wait for the child,Vessel and cleanupGlobalData.
void EndOfAllThread()
{
	char convertToPrint[STRING_SIZE];

	/* wait for the child to exit */
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Wait for ALL Vessel to finish
	WaitForMultipleObjects(numOfVessel, Vessels, True, INFINITE);

	sprintf(convertToPrint, "[%s] Haifa Port : All Vessels Threads are done \n", getTimeString());
	consolePrint(convertToPrint);

	CloseHandle(WriteHandleIn);
	CloseHandle(ReadHandleOut);


	sprintf(convertToPrint, "[%s] Haifa Port: Haifa Port: Exiting... \n", getTimeString());
	consolePrint(convertToPrint);

	cleanupGlobalData();
	/* close all handles */

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
// Create all Vessels Threads. Report if Error occurred!
void CreateVessThread()
{
	for (int i = 0; i < numOfVessel; i++)
	{
		trdIDVessel[i] = i + 1;
		Vessels[i] = CreateThread(NULL, 0, Vessel, &trdIDVessel[i], 0, &ThreadId);
		if (Vessels[i] == NULL)
		{
			printf("main::Unexpected Error in Vessel %d Creation\n", i);
			return 1;
		}
	}

}
// Read all Vessels Threads Return from Eilat. Report if Error occurred!
void ReadAllThreadReturn()
{
	char convertToPrint[STRING_SIZE];
	while (correntVessInHaifa < numOfVessel)
	{
		success = ReadFile(ReadHandleOut, buffer, BUFFER_SIZE, &read, NULL);
		if (!success)
		{
			fprintf(stderr, "HaifaPort: error reading from pipe , error %d \n", GetLastError());
			return -1;
		}
		else if ((currentVesselId = atoi(buffer)) != 0)
		{

			sprintf(convertToPrint, "[%s] Vessel %s - exiting Canal: Red Sea ==> Med. Sea \n", getTimeString(), buffer);
			consolePrint(convertToPrint);

			if (!ReleaseSemaphore(SemOfVessel[currentVesselId - 1], 1, NULL)) {
				printf("HaifaPort:ReleaseSemaphore error sem[%d].V \n", currentVesselId);
			}

			correntVessInHaifa++;

		}
	}
}










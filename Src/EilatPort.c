#define _CRT_SECURE_NO_WARNINGS
#define _CRT_RAND_S

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

//random time between 5 and 3000 msec between actions
#define MAX_SLEEP_TIME 3000 //Miliseconds (3 second)
#define MIN_SLEEP_TIME 5    //Miliseconds 

#define BUFFER_SIZE 50
#define STRING_SIZE 256

#define MinCargoWeight 5
#define MaxCargoWeight 50

SYSTEMTIME timeToPrint;

#define True 1
#define False 0
BOOL success;

//state array, holds current state of each Philosopher in the System
char buffer[BUFFER_SIZE];
char buffer2[BUFFER_SIZE];


//int state[N];

HANDLE WriteHandle, ReadHandle;
DWORD written, read;
DWORD WINAPI Vessel(PVOID);
DWORD WINAPI Crane(PVOID);

int number_of_cranes;
int CatchCranesInAdt;
HANDLE* Cranes;
int* CranesID;
int* ArrCranes;
int* Toneweights;


int numOfVessel, NumOfInVessInEilat;
int* trdIDVessel;
HANDLE* Vessels;
DWORD ThreadId;

HANDLE mutexPrint;
HANDLE mutexwrite, mutexAdt, mutexExitAdt;
HANDLE semAdt, *semVesselId;
int ADTFree, vesselIdEnter;

HANDLE mutexBarrier, *SemBarrier;
int SizeSemBarrier, VessInBarrier, QueueIN, QueueOut;

//Function declaration
int initGlobalData();
void cleanupGlobalData();
int calcSleepTime();
int randCargo();
void exitFromBarrier();
const char* getTimeString();
void Sails_to_Haifa(int);
int Divisor(int);
int primeornot(int);
void enterToBarrier(int);
int enterVesselToUnloading(int);
void exitADT(int);
void consolePrint(char*);
void ReadAllThreadHaifa();
void EndOfAllThread();
void ConfirmationAndWritingHaifa();
void InitializationVariables();
void PrintAndRelease(int);



int main(int argc, char *argv[])
{
	char convertToPrint[STRING_SIZE];
	
	InitializationVariables();

	success = ReadFile(ReadHandle, buffer, BUFFER_SIZE, &read, NULL);

	if (success)
		fprintf(stderr, "[%s] EilatPort: Received request message of - %s - Vessels \n", getTimeString(), buffer);
	else {
		fprintf(stderr, "EilatPort: error reading from pipe\n");
		return -1;
	}
	numOfVessel = atoi(buffer);
	
	ConfirmationAndWritingHaifa();

	number_of_cranes = Divisor(numOfVessel);
	SizeSemBarrier = numOfVessel / number_of_cranes;

	sprintf(convertToPrint, "[%s] EilatPort: CraneSize %d \n", getTimeString(), number_of_cranes);
	consolePrint(convertToPrint);

	if (initGlobalData() == False)
	{
		fprintf(stderr, "EilatPort: Error while initiating global data \n");
		return -1;
	}

	ReadAllThreadHaifa();

	EndOfAllThread();

	return 0;

}
//Thread function for each Crane
DWORD WINAPI Crane(PVOID Param)
{
	//Get the Unique ID from Param and keep it in a local variable
	int CraneID = *(int*)Param;
	char convertToPrint[STRING_SIZE];
	sprintf(convertToPrint, "[%s] Crane - %d has been created  \n", getTimeString(), CraneID);
	consolePrint(convertToPrint);
	for (int i = 0; i < SizeSemBarrier; i++)
	{
		/*srand(time(NULL));*/
		WaitForSingleObject(semAdt, INFINITE);

		sprintf(convertToPrint, "[%s] vessel %d - unloaded  %d Tons at Crane %d\n", getTimeString(), ArrCranes[CraneID - 1], Toneweights[CraneID - 1], CraneID);
		consolePrint(convertToPrint);

		Sleep(calcSleepTime());


		sprintf(convertToPrint, "[%s] vessel %d Finished unloading the weighte \n", getTimeString(), ArrCranes[CraneID - 1]);
		consolePrint(convertToPrint);


		if (!ReleaseSemaphore(semVesselId[CraneID - 1], 1, NULL))
			fprintf(stderr, "EilatPort->Crane :: Unexpected error sem.V()\n");


	}


	return 0;
}
//Thread function for each Vessel
DWORD WINAPI Vessel(PVOID Param)
{

	//Get the Unique ID from Param and keep it in a local variable
	int VesselID = *(int*)Param;
	int crain;

	enterToBarrier(VesselID);

	Sleep(calcSleepTime());

	crain = enterVesselToUnloading(VesselID);
	//Error Handling
	if (crain == -1)
	{
		printf("Unexpected Error Thread %d Entering Crain\n", VesselID);
		return 1;
	}

	exitADT(VesselID);


	Sails_to_Haifa(VesselID);

	Sleep(calcSleepTime());



	return 0;

}
//Check if the number of vessels we received of
// Anonymous pipe is a prime number.
int primeornot(int numOfVessel)
{
	int i;
	for (i = 2; i <= numOfVessel / 2; i++)
	{
		if (numOfVessel % i == 0)
		{
			return 0;
		}
	}
	return 1; // loop has ended with no divisors --> prime!!
}
//Check if num of crane number is divisible.
int Divisor(int numOfVessel) {
	int random_Crane;
	while (True)
	{
		srand(time(NULL));
		random_Crane = rand() % (numOfVessel / 2) + 2;
		if (numOfVessel % random_Crane == 0)
		{
			return random_Crane;
		}
	}
}
//Vessel has finished unloading in Eilat,and returns to back to Haifa.
void Sails_to_Haifa(int Vesselid)
{

	char convertToPrint[STRING_SIZE];
	//Access to state DB is done exclusively (protected by mutex)
	//mutexPipe.P()
	WaitForSingleObject(mutexwrite, INFINITE);

	sprintf(buffer2, "%d", Vesselid);
	sprintf(convertToPrint, "[%s] Vessel %d entering Canal: Red Sea ==> Med. Sea \n", getTimeString(), Vesselid);
	consolePrint(convertToPrint);

	Sleep(calcSleepTime());

	/* the parent now wants to write to the pipe */
	if (!WriteFile(WriteHandle, buffer2, BUFFER_SIZE, &written, NULL))
		fprintf(stderr, "Eilat ::Error writing to pipe\n");

	//mutexPipe.V() - release mutex
	if (!ReleaseMutex(mutexwrite))
		printf("Eilat :: Unexpected error mutexPipe.V()\n");


}
//Realization of the Barrier as an array of semaphores the size of several 
//iterations for Unloading, each position in the array has a semaphore
//the size of the random of crane in the main and 
//The first m of vessels who enter are the first to be released(realization of fifo).
void enterToBarrier(int VesselIdIn)

{
	char convertToPrint[STRING_SIZE];
	WaitForSingleObject(mutexBarrier, INFINITE);
	VessInBarrier++;//how many threads entered so far,initialize to zero when the Barrier[QueueIn] is full

	if ((ADTFree == True) && (QueueIN == QueueOut) && (VessInBarrier == number_of_cranes))
	{
		QueueIN++;
		VessInBarrier = 0;
		//mutexBarrier.V() - release mutex
		if (!ReleaseMutex(mutexBarrier))
			printf("Eilat :: Unexpected error mutexPipe.V()\n");
		exitFromBarrier();
	}
	else
	{
		if ((ADTFree == True) && (QueueIN > QueueOut))
		{
			exitFromBarrier();
			if (VessInBarrier == number_of_cranes)
			{
				QueueIN++;
				VessInBarrier = 0;
				PrintAndRelease(VesselIdIn);
				WaitForSingleObject(SemBarrier[QueueIN - 1], INFINITE);
			}
			else
			{
				PrintAndRelease(VesselIdIn);
				WaitForSingleObject(SemBarrier[QueueIN], INFINITE);
			}
		}
		else
		{
			if (VessInBarrier == number_of_cranes)
			{
				QueueIN++;
				VessInBarrier = 0;
				PrintAndRelease(VesselIdIn);
				WaitForSingleObject(SemBarrier[QueueIN - 1], INFINITE);
			}
			else
			{
				PrintAndRelease(VesselIdIn);
				WaitForSingleObject(SemBarrier[QueueIN], INFINITE);
			}
		}
	}

	sprintf(convertToPrint, "[%s] Vessel %d -Released Barrier\n", getTimeString(), VesselIdIn);
	consolePrint(convertToPrint);

}
//Function for vesselID entry to the unloadingQuay,settles down at Crane crane and Tone weight random.
int enterVesselToUnloading(int VessID)
{

	char convertToPrint[STRING_SIZE];
	int availableCrain;

	WaitForSingleObject(mutexAdt, INFINITE);

	sprintf(convertToPrint, "[%s] Vessel %d -Entered the unloadingQuay\n", getTimeString(), VessID);
	consolePrint(convertToPrint);

	for (availableCrain = 0; availableCrain < number_of_cranes; availableCrain++)
		if (ArrCranes[availableCrain] == -1)
		{
			ArrCranes[availableCrain] = VessID;
			CatchCranesInAdt++;
			break;
		}
	Sleep(calcSleepTime());


	sprintf(convertToPrint, "[%s] Vessel %d -settles down at Crane %d  \n", getTimeString(), VessID, availableCrain + 1);
	consolePrint(convertToPrint);
	Toneweights[availableCrain] = randCargo();

	sprintf(convertToPrint, "[%s] Vessel %d - weight is %d tons  \n", getTimeString(), VessID, Toneweights[availableCrain]);
	consolePrint(convertToPrint);


	// mutex.V() - release mute
	if (!ReleaseMutex(mutexAdt)) {
		printf("UnloadingQuay::Unexpected error mutex.V()\n");
	}
	//wait all vessels (num of cranes) settles down at Crane.
	if (CatchCranesInAdt == number_of_cranes)
	{
		if (!ReleaseSemaphore(semAdt, number_of_cranes, NULL))
			printf("UnloadingQuay::Unexpected error semAdt.V()\n");
	}
	//semVesselId[availableCrain] wait to release to exit from unloadingQuay
	// and return back to Haifa.
	WaitForSingleObject(semVesselId[availableCrain], INFINITE);
	return availableCrain;

}
//Functions for the departure of a vesselId from the unloadingQuay and leaving the crane he was stationed.
void exitADT(int VessID)
{
	int availableCrain;
	char convertToPrint[STRING_SIZE];
	WaitForSingleObject(mutexExitAdt, INFINITE);

	Sleep(calcSleepTime());
	for (availableCrain = 0; availableCrain < number_of_cranes; availableCrain++)
		if (ArrCranes[availableCrain] == VessID)
		{
			ArrCranes[availableCrain] = -1;
			Toneweights[availableCrain] = -1;
			CatchCranesInAdt--;
			sprintf(convertToPrint, "[%s] Vessel %d left Crane %d and Exiting the UnloadingQuay\n", getTimeString(), VessID, availableCrain + 1);
			consolePrint(convertToPrint);

			break;
		}


	// mutex.V() - release mutex
	if (!ReleaseMutex(mutexExitAdt))
	{
		printf("UnloadingQuay::Unexpected error mutexExitAdt.V()\n");
	}
	if (CatchCranesInAdt == 0)
	{
		ADTFree = True;
		if ((NumOfInVessInEilat == numOfVessel) && (QueueIN>QueueOut))
		{

			exitFromBarrier();
		}
	}

}
//Another iteration of release of m NumOfCranes.
void exitFromBarrier()
{
	char convertToPrint[STRING_SIZE];
	sprintf(convertToPrint, "[%s] Barrier Release Iteration\n", getTimeString());
	consolePrint(convertToPrint);

	if (!ReleaseSemaphore(SemBarrier[QueueOut], number_of_cranes, NULL))
		fprintf(stderr, "exitBarrier::Unexpected error Barrier[%d].V()\n", QueueOut);
	QueueOut++;
	ADTFree = False;
}
//Function to Initialise global data (mainly for creating the 
//Semaphore Haldles before all Threads start .
int initGlobalData() {
	int i;

	mutexwrite = CreateMutex(NULL, FALSE, NULL);
	if (mutexwrite == NULL)
	{
		printf("initGlobalData::Unexpected Error in mutexwrite Creation\n");
		return False;
	}
	mutexBarrier = CreateMutex(NULL, FALSE, NULL);
	if (mutexBarrier == NULL)
	{
		printf("initGlobalData::Unexpected Error in mutexBarrier Creation\n");
		return False;
	}
	mutexAdt = CreateMutex(NULL, FALSE, NULL);
	if (mutexAdt == NULL)
	{
		printf("initGlobalData::Unexpected Error in mutexAdt Creation\n");
		return False;
	}
	mutexExitAdt = CreateMutex(NULL, FALSE, NULL);
	if (mutexExitAdt == NULL)
	{
		printf("initGlobalData::Unexpected Error in mutexExitAdt Creation\n");
		return False;
	}
	trdIDVessel = (int*)malloc(numOfVessel * sizeof(int));
	if (trdIDVessel == NULL)
	{
		printf("initGlobalData:: Error allocating trdIDVessel");
		return False;
	}
	Toneweights = (int*)malloc(number_of_cranes * sizeof(int));
	if (Toneweights == NULL)
	{
		printf("initGlobalData:: Error allocating Toneweights");
		return False;
	}
	Vessels = (HANDLE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (numOfVessel * sizeof(HANDLE)));
	if (Vessels == NULL)
	{
		fprintf(stderr, "Allocating Vessels\n");
	}
	semVesselId = (HANDLE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (number_of_cranes * sizeof(HANDLE)));
	if (semVesselId == NULL)
	{
		fprintf(stderr, "initGlobalData:: Error creating semVesselId\n");
		return False;
	}

	SemBarrier = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SizeSemBarrier * sizeof(HANDLE)));
	if (SemBarrier == NULL)
	{
		fprintf(stderr, "initGlobalData:: Error allocating Barrier\n");
		return False;
	}
	semAdt = CreateSemaphore(NULL, 0, number_of_cranes, NULL);
	if (semAdt == NULL)
	{
		printf("initGlobalData:: Error creating semAdt");
		return False;
	}
	Cranes = (HANDLE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (number_of_cranes * sizeof(HANDLE)));
	if (Cranes == NULL)
	{
		printf("initGlobalData:: Error allocating Cranes");
		return False;
	}
	CranesID = (int*)malloc(number_of_cranes * sizeof(int));
	if (CranesID == NULL)
	{
		printf("initGlobalData:: Error allocating CranesID");
		return False;
	}
	ArrCranes = (int*)malloc(number_of_cranes * sizeof(int));
	if (ArrCranes == NULL)
	{
		printf("initGlobalData:: Error allocating CranesArr");
		return False;
	}


	for (i = 0; i < number_of_cranes; i++)
	{
		Toneweights[i] = -1;
		CranesID[i] = i + 1;
		ArrCranes[i] = -1;
		Cranes[i] = CreateThread(NULL, 0, Crane, &CranesID[i], 0, &ThreadId);
		semVesselId[i] = CreateSemaphore(NULL, 0, 1, NULL);
	}
	for (i = 0; i < SizeSemBarrier; i++)
	{
		SemBarrier[i] = CreateSemaphore(NULL, 0, number_of_cranes, NULL);
		if (SemBarrier[i] == NULL)
		{
			return False;
		}
	}

	return True;

}
//Function to clean global data and closing Threads after all Threads finish.
void cleanupGlobalData() {
	int i;
	CloseHandle(mutexAdt);
	CloseHandle(mutexBarrier);
	CloseHandle(mutexwrite);
	CloseHandle(mutexExitAdt);
	CloseHandle(semAdt);

	for (i = 0; i < SizeSemBarrier; i++)
	{
		CloseHandle(SemBarrier[i]);
	}

	for (i = 0; i < number_of_cranes; i++)
	{
		CloseHandle(semVesselId[i]);
		CloseHandle(Cranes[i]);

	}

	for (i = 0; i < numOfVessel; i++)
	{
		CloseHandle(Vessels[i]);
	}

	free(Toneweights);
	free(ArrCranes);
	free(trdIDVessel);
	free(CranesID);

	HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, SemBarrier);
	HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, Vessels);
	HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, Cranes);
	HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, semVesselId);


}
//generic function to randomise a Sleep time between MIN_SLEEP_TIME and MAX_SLEEP_TIME msec
int calcSleepTime() {
	srand(time(NULL));
	int sleepTime = rand() % (MAX_SLEEP_TIME)+MIN_SLEEP_TIME;
	return sleepTime;
}
//generic function to randomise a cargo weight between MinCargoWeight and MaxCargoWeight
int randCargo()
{
	int randTon;
	srand(time(NULL));
	randTon= rand() % (MaxCargoWeight)+MinCargoWeight;
	return randTon;
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
// Read all Vessels Threads from Anonymous pipe of Haifa and created all thread vessel.
void ReadAllThreadHaifa()
{
	char convertToPrint[STRING_SIZE];
	while (NumOfInVessInEilat <numOfVessel)
	{
		success = ReadFile(ReadHandle, buffer, BUFFER_SIZE, &read, NULL);
		if (!success)
		{
			fprintf(stderr, "Eilat ::Error Reading from pipe\n");
			return -1;
		}
		sprintf(convertToPrint, "[%s] Vessel %s arrived @ Eilat Port \n", getTimeString(), buffer);
		consolePrint(convertToPrint);
		vesselIdEnter = atoi(buffer);
		trdIDVessel[vesselIdEnter - 1] = vesselIdEnter;
		NumOfInVessInEilat++;
		Vessels[vesselIdEnter - 1] = CreateThread(NULL, 0, Vessel, &trdIDVessel[vesselIdEnter - 1], 0, &ThreadId);


	}
}
// wait for all Thread of Cranes,Vessels and call cleanupGlobalData function.
void EndOfAllThread()
{
	char convertToPrint[STRING_SIZE];
	WaitForMultipleObjects(numOfVessel, Vessels, TRUE, INFINITE);

	sprintf(convertToPrint, "[%s] Eilat Port:  All Vassel Threads are done \n", getTimeString());
	consolePrint(convertToPrint);


	WaitForMultipleObjects(number_of_cranes, Cranes, TRUE, INFINITE);

	sprintf(convertToPrint, "[%s] Eilat Port: All Crane Threads are done \n", getTimeString());
	consolePrint(convertToPrint);

	cleanupGlobalData();

	sprintf(convertToPrint, "[%s] Eilat Port: Eilat Port: Exiting... \n", getTimeString());
	consolePrint(convertToPrint);
}
//Checking the correctness of the number of vessels and returning a response to Haifa Process.
void ConfirmationAndWritingHaifa()
{
	int Prime = primeornot(numOfVessel);
	if (Prime == 0)
	{
		strcpy(buffer2, "False");
	}
	else
	{
		strcpy(buffer2, "True");
		ExitProcess(1);
	}
	//Write to the pipe
	if (!WriteFile(WriteHandle, buffer2, BUFFER_SIZE, &written, NULL))
		fprintf(stderr, "Error writing to pipe\n");

	mutexPrint = OpenMutex(MUTEX_ALL_ACCESS, FALSE, TEXT("ConsoleMutex"));
	if (mutexPrint == NULL)
	{
		fprintf(stderr, "initGlobalData:: Error creating mutexPipe\n");
		return False;
	}

}
//Initialization the pipes,handles and variables
void InitializationVariables()
{
	NumOfInVessInEilat = 0, CatchCranesInAdt = 0;
	VessInBarrier = 0;
	QueueIN = 0;
	QueueOut = 0;
	ADTFree = True;

	ReadHandle = GetStdHandle(STD_INPUT_HANDLE);
	WriteHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	/* have the child read from the pipe */
	if ((WriteHandle == INVALID_HANDLE_VALUE) || (ReadHandle == INVALID_HANDLE_VALUE))
		ExitProcess(1);

}
void PrintAndRelease(int VesselIdIn)
{
	char convertToPrint[STRING_SIZE];
	sprintf(convertToPrint, "[%s] Vessel %d -Entered the Barrier\n", getTimeString(), VesselIdIn);
	consolePrint(convertToPrint);
	//mutexBarrier.V() - release mutex
	if (!ReleaseMutex(mutexBarrier))
		printf("Eilat :: Unexpected error mutexPipe.V()\n");
}

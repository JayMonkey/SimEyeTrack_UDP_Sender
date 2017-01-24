// Precomilied headers 
#include "stdafx.h"

// Standrad Includes
#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <assert.h>
#include <eyex/EyeX.h>

#pragma comment (lib, "Tobii.EyeX.Client.lib")
#pragma comment(lib,"ws2_32.lib") // Winsock Library

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) ) 	// Custom type def options for opentrack datagram
#define USE_GAZE				  // Comment Out to use Fixation data

// global variables
PACK(
	struct OT_UDP { // opentrack UDP datagram structure
	double x;
	double y;
	double z;
	double yaw;
	double pitch;
	double roll;
});

// Screen Resolution

TCHAR SERVER[16] = "";
int PORT = 0;
int Screen_Pixels_X = 0;
int Screen_Pixels_Y = 0;

int Mid_Ponit_X = 0;
int Mid_Ponit_Y = 0;

// Structure for IP and socket data
struct sockaddr_in si_other;
int s, slen = sizeof(si_other);
WSADATA wsa;

static TX_HANDLE g_hGlobalInteractorSnapshot = TX_EMPTY_HANDLE;

#ifdef USE_GAZE

// GAZE: ID of the global interactor that provides our data stream; must be unique within the application.
static const TX_STRING InteractorId = "Twilight Sparkle";

// GAZE: Initializes g_hGlobalInteractorSnapshot with an interactor that has the Gaze Data behavior.

BOOL InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext)
{
	TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
	TX_GAZEPOINTDATAPARAMS params = { TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED };
	BOOL success;

	success = txCreateGlobalInteractorSnapshot(
		hContext,
		InteractorId,
		&g_hGlobalInteractorSnapshot,
		&hInteractor) == TX_RESULT_OK;
	success &= txCreateGazePointDataBehavior(hInteractor, &params) == TX_RESULT_OK;

	txReleaseObject(&hInteractor);

	return success;
}

// GAZE: Handles an event from the Gaze Point data stream.

void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior)
{
	TX_GAZEPOINTDATAEVENTPARAMS eventParams;
	if (txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams) == TX_RESULT_OK) {
		TX_REAL X_POS = eventParams.X;
		TX_REAL Y_POS = eventParams.Y;
		if (X_POS > Mid_Ponit_X) {
			X_POS = X_POS - Mid_Ponit_X;
		}
		else {
			X_POS = Mid_Ponit_X - X_POS;
			X_POS = -X_POS;
		}

		if (Y_POS > Mid_Ponit_Y) {
			Y_POS = Y_POS - Mid_Ponit_Y;
			Y_POS = -Y_POS;
		}
		else {
			Y_POS = Mid_Ponit_Y - Y_POS;
		}

		if (X_POS > Mid_Ponit_X) X_POS = Mid_Ponit_X;
		if (X_POS < -Mid_Ponit_X) X_POS = -Mid_Ponit_X;

		if (Y_POS > Mid_Ponit_Y) Y_POS = Mid_Ponit_Y;
		if (Y_POS < -Mid_Ponit_Y) Y_POS = -Mid_Ponit_Y;

		struct OT_UDP OT_DATA;

		OT_DATA.x = 0.0;
		OT_DATA.y = 0.0;
		OT_DATA.z = 0.0;
		OT_DATA.roll = 0.0;
		OT_DATA.yaw = 0.05 * X_POS;
		OT_DATA.pitch = 0.06 * Y_POS;

		const long uBufSize = sizeof(OT_DATA);
		const char pBuffer[48];
		memcpy(pBuffer, &OT_DATA, uBufSize);

		//send the datagram
		if (sendto(s, pBuffer, uBufSize, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
		{
			printf("\nsendto() failed with error code : %d", WSAGetLastError());
			printf("\nCheck config file (jwrs_eyetrack.ini)\n");
			exit(EXIT_FAILURE);
		}
	}
	else {
		printf("Failed to interpret gaze data event packet.\n");
	}
}

// GAZE: Callback function invoked when an event has been received from the EyeX Engine.

void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
	TX_HANDLE hEvent = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior = TX_EMPTY_HANDLE;

	txGetAsyncDataContent(hAsyncData, &hEvent);

	if (txGetEventBehavior(hEvent, &hBehavior, TX_BEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK) {
		OnGazeDataEvent(hBehavior);
		txReleaseObject(&hBehavior);
	}

	txReleaseObject(&hEvent);
}

#else

// FIXATION: ID of the global interactor that provides our data stream; must be unique within the application.

static const TX_STRING InteractorId = "Rainbow Dash";

// FIXATION: Initializes g_hGlobalInteractorSnapshot with an interactor that has the Fixation Data behavior.

BOOL InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext)
{
	TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
	TX_FIXATIONDATAPARAMS params = { TX_FIXATIONDATAMODE_SENSITIVE };
	BOOL success;

	success = txCreateGlobalInteractorSnapshot(
		hContext,
		InteractorId,
		&g_hGlobalInteractorSnapshot,
		&hInteractor) == TX_RESULT_OK;
	success &= txCreateFixationDataBehavior(hInteractor, &params) == TX_RESULT_OK;

	txReleaseObject(&hInteractor);

	return success;
}

// FIXATION: Handles an event from the fixation data stream.

void OnFixationDataEvent(TX_HANDLE hFixationDataBehavior)
{
	TX_FIXATIONDATAEVENTPARAMS eventParams;
	TX_FIXATIONDATAEVENTTYPE eventType;
	char* eventDescription;

	if (txGetFixationDataEventParams(hFixationDataBehavior, &eventParams) == TX_RESULT_OK) {
		eventType = eventParams.EventType;

		eventDescription = (eventType == TX_FIXATIONDATAEVENTTYPE_DATA) ? "Data"
			: ((eventType == TX_FIXATIONDATAEVENTTYPE_END) ? "End"
				: "Begin");

		TX_REAL X_POS = eventParams.X;
		TX_REAL Y_POS = eventParams.Y;
		if (X_POS > Mid_Ponit_X) {
			X_POS = X_POS - Mid_Ponit_X;
		}
		else {
			X_POS = Mid_Ponit_X - X_POS;
			X_POS = -X_POS;
		}

		if (Y_POS > Mid_Ponit_Y) {
			Y_POS = Y_POS - Mid_Ponit_Y;
			Y_POS = -Y_POS;
		}
		else {
			Y_POS = Mid_Ponit_Y - Y_POS;
		}

		if (X_POS > Mid_Ponit_X) X_POS = Mid_Ponit_X;
		if (X_POS < -Mid_Ponit_X) X_POS = -Mid_Ponit_X;

		if (Y_POS > Mid_Ponit_Y) Y_POS = Mid_Ponit_Y;
		if (Y_POS < -Mid_Ponit_Y) Y_POS = -Mid_Ponit_Y;

		struct OT_UDP OT_DATA;

		OT_DATA.x = 0.0;
		OT_DATA.y = 0.0;
		OT_DATA.z = 0.0;
		OT_DATA.roll = 0.0;
		OT_DATA.yaw = 0.05 * X_POS;
		OT_DATA.pitch = 0.06 * Y_POS;

		const long uBufSize = sizeof(OT_DATA);
		const char pBuffer[48];
		memcpy(pBuffer, &OT_DATA, uBufSize);

		//send the datagram
		if (sendto(s, pBuffer, uBufSize, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
		{
			printf("\nsendto() failed with error code : %d", WSAGetLastError());
			printf("\nCheck config file (jwrs_eyetrack.ini)\n");
			exit(EXIT_FAILURE);
		}
	} else {
		printf("Failed to interpret fixation data event packet.\n");
	}
}


// FIXATION: Callback function invoked when an event has been received from the EyeX Engine.

void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
	TX_HANDLE hEvent = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior = TX_EMPTY_HANDLE;

	txGetAsyncDataContent(hAsyncData, &hEvent);

	if (txGetEventBehavior(hEvent, &hBehavior, TX_BEHAVIORTYPE_FIXATIONDATA) == TX_RESULT_OK) {
		OnFixationDataEvent(hBehavior);
		txReleaseObject(&hBehavior);
	}

	txReleaseObject(&hEvent);
}
#endif

// GENERIC: Callback function invoked when a snapshot has been committed.

void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param)
{
	TX_RESULT result = TX_RESULT_UNKNOWN;
	txGetAsyncDataResultCode(hAsyncData, &result);
	assert(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
}

// GENERIC: Callback function invoked when the status of the connection to the EyeX Engine has changed.

void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
{
	switch (connectionState) {
	case TX_CONNECTIONSTATE_CONNECTED: {
		BOOL success;
		printf("Connected\n");
		success = txCommitSnapshotAsync(g_hGlobalInteractorSnapshot, OnSnapshotCommitted, NULL) == TX_RESULT_OK;
		if (!success) {
			printf("Failed to initialize the data stream.\n");
		}
		else
		{
			printf("\n**********************************\n");
			printf("***** Press any key to quit  *****\n");
			printf("**********************************\n");
		}
	}
	break;

	case TX_CONNECTIONSTATE_DISCONNECTED:
		printf("Closing Connection to EyeX Engine\n");
		break;

	case TX_CONNECTIONSTATE_TRYINGTOCONNECT:
		printf("\nConnecting to the EyeX Engine - ");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
		printf("The connection state is now SERVER_VERSION_TOO_LOW: this application requires a more recent version of the EyeX Engine to run.\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
		printf("The connection state is now SERVER_VERSION_TOO_HIGH: this application requires an older version of the EyeX Engine to run.\n");
		break;
	}
}

// Application entry point.
 
int main(int argc, char* argv[])
{

	printf("\n\n\n\n\n\n\n\n\n\n");
	printf("*************************************\n");
	printf("*** JW_RS Eye Tracking MIddleware ***\n");
#ifdef _WIN64
	printf("***        x64 Version 1.0        ***\n");
#else
	printf("***        x86 Version 1.0        ***\n");
#endif
	printf("*** (c) CM-Media Productions 2017 ***\n");
	printf("*************************************\n");

	GetPrivateProfileString(_T("config"), _T("target"), _T("notfound"), SERVER, 16, _T("./jwrs_eyetrack.ini"));
	PORT = GetPrivateProfileInt(_T("config"), _T("port"), _T("999999"), _T("./jwrs_eyetrack.ini"));
	Screen_Pixels_X = GetPrivateProfileInt(_T("config"), _T("res_x"), _T("9999"), _T("./jwrs_eyetrack.ini"));
	Screen_Pixels_Y = GetPrivateProfileInt(_T("config"), _T("res_y"), _T("9999"), _T("./jwrs_eyetrack.ini"));

	Mid_Ponit_X = (Screen_Pixels_X / 2);
	Mid_Ponit_Y = (Screen_Pixels_Y / 2);

	//Initialise winsock
	printf("\nInitialising Winsock - ");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d/n/n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("OK\n\n");

	//create socket
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	printf("Creating UDP stream to Host: ");
	printf(SERVER);
	printf(" Port: %d \n",PORT);
	printf("Screen Tracking Resolution X=%d,Y=%d \n", Screen_Pixels_X, Screen_Pixels_Y);

	//setup up ip address structure
	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);
	
	TX_CONTEXTHANDLE hContext = TX_EMPTY_HANDLE;
	TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;
	TX_TICKET hEventHandlerTicket = TX_INVALID_TICKET;
	BOOL success;

	// initialize and enable the context that is our link to the EyeX Engine.

	success = txInitializeEyeX(TX_EYEXCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL, NULL) == TX_RESULT_OK;
	success &= txCreateContext(&hContext, TX_FALSE) == TX_RESULT_OK;
	success &= InitializeGlobalInteractorSnapshot(hContext);
	success &= txRegisterConnectionStateChangedHandler(hContext, &hConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL) == TX_RESULT_OK;
	success &= txRegisterEventHandler(hContext, &hEventHandlerTicket, HandleEvent, NULL) == TX_RESULT_OK;
	success &= txEnableConnection(hContext) == TX_RESULT_OK;

	// Track Eyes until a key is pressed.
	if (success) {
		printf("\nEyeX Initialization successful - ");

#ifdef USE_GAZE
		printf("Using Gaze Tracking\n");
#else
		printf("Using Fixation Tracking\n");
#endif

	} else {
		printf("\nEyeX Initialization failed.\n");
	}
	_getch();
	printf("\nShuting Down JWRS Eye Tracking Middleware .....\n\n");
	Sleep(2000);

	// disable and delete the context.
	txDisableConnection(hContext);
	txReleaseObject(&g_hGlobalInteractorSnapshot);
	success = txShutdownContext(hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE) == TX_RESULT_OK;
	success &= txReleaseContext(&hContext) == TX_RESULT_OK;
	success &= txUninitializeEyeX() == TX_RESULT_OK;
	if (!success) {
		printf("EyeX could not be shut down cleanly.\nDid you remember to release all handles?\n");
	}

	return 0;
}



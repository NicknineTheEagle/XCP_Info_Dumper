#include "stdafx.h"
#include <vector>
#include "Utilities.h"
#include "Detour.h"

unsigned char hexToByte(const std::string& hex) {
	return static_cast<unsigned char>(std::stoul(hex, nullptr, 16));
}

_GUID parseGuidString(const std::string& str) {
	if (str.length() != 36 ||
		str[8] != '-' || str[13] != '-' || str[18] != '-' || str[23] != '-') {
		throw std::invalid_argument("Invalid GUID format");
	}

	_GUID guid = {};

	guid.Data1 = std::stoul(str.substr(0, 8), nullptr, 16);
	guid.Data2 = std::stoul(str.substr(9, 4), nullptr, 16);
	guid.Data3 = std::stoul(str.substr(14, 4), nullptr, 16);

	for (int i = 0; i < 2; ++i) {
		guid.Data4[i] = hexToByte(str.substr(19 + i * 2, 2));
	}
	for (int i = 0; i < 6; ++i) {
		guid.Data4[2 + i] = hexToByte(str.substr(24 + i * 2, 2));
	}

	return guid;
}

bool readGuidsFromFile(std::vector<_GUID> &outList)
{
	Byrom_Dbg("Mounting HDD1");
	_STRING sLinkName, sDeviceName;
	RtlInitAnsiString(&sLinkName, "\\System??\\XCPHDD1:");
	RtlInitAnsiString(&sDeviceName, "\\Device\\Harddisk0\\Partition1");
	ObCreateSymbolicLink(&sLinkName, &sDeviceName);

	Byrom_Dbg("Opening xcpguids.txt");
	HANDLE handle = CreateFile("XCPHDD1:\\xcpguids.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (handle == INVALID_HANDLE_VALUE)
	{
		DWORD errorCode = GetLastError();
		Byrom_Dbg("Failed to open xcpguids.txt (error %d)!!!", errorCode);
		ObDeleteSymbolicLink(&sLinkName);
		return false;
	}

	Byrom_Dbg("Reading GUID list from xcpguids.txt");
	std::string str;

	while (true)
	{
		char ch;
		DWORD readBytes;
		if (ReadFile(handle, &ch, 1, &readBytes, NULL) == FALSE)
			break;

		if (readBytes != 0 && ch != '\r' && ch != '\n')
		{
			str.push_back(ch);
		}
		else if (!str.empty())
		{
			if (str.length() == 36 &&
				str[8] == '-' && str[13] == '-' && str[18] == '-' && str[23] == '-')
			{
				outList.push_back(parseGuidString(str));
			}
			str.clear();
		}

		if (readBytes == 0)
			break;
	}

	Byrom_Dbg("Parsed %d GUID entries", outList.size());
	Byrom_Dbg("Closing xcpguids.txt");
	XCloseHandle(handle);
	Byrom_Dbg("Unmounting HDD1");
	ObDeleteSymbolicLink(&sLinkName);
	return true;
}

VOID DoInfoPrint(PFIND_MEDIA_INSTANCE_URLS_RESPONSE FMIRR)
{
	for (DWORD i = 0; i < FMIRR->dwMediaInstanceIdsCount; i++)
	{
		PMEDIA_INSTANCE_URLS PMIU = &(FMIRR->pMediaInstanceIds[i]);

		if (bIsDevkit) // Printing on devkit works correctly with new lines etc. Bundling it all together in one print ensures it doesn't get split up by other prints in kdnet
		{
			char MediaID[50];
			sprintf(MediaID, "MediaID: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", PMIU->MediaID.Data1, PMIU->MediaID.Data2, PMIU->MediaID.Data3, PMIU->MediaID.Data4[0], PMIU->MediaID.Data4[1], PMIU->MediaID.Data4[2], PMIU->MediaID.Data4[3], PMIU->MediaID.Data4[4], PMIU->MediaID.Data4[5], PMIU->MediaID.Data4[6], PMIU->MediaID.Data4[7]);

			char MediaInstanceID[60];
			sprintf(MediaInstanceID, "MediaInstanceID: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", PMIU->MediaInstanceID.Data1, PMIU->MediaInstanceID.Data2, PMIU->MediaInstanceID.Data3, PMIU->MediaInstanceID.Data4[0], PMIU->MediaInstanceID.Data4[1], PMIU->MediaInstanceID.Data4[2], PMIU->MediaInstanceID.Data4[3], PMIU->MediaInstanceID.Data4[4], PMIU->MediaInstanceID.Data4[5], PMIU->MediaInstanceID.Data4[6], PMIU->MediaInstanceID.Data4[7]);

			char Url[110];
			sprintf(Url, "Url: %s", PMIU->pUrls[0].pszUrl);

			char Key[45];
			sprintf(Key, "Key: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", PMIU->rgbSymKey[0], PMIU->rgbSymKey[1], PMIU->rgbSymKey[2], PMIU->rgbSymKey[3], PMIU->rgbSymKey[4], PMIU->rgbSymKey[5],
				PMIU->rgbSymKey[6], PMIU->rgbSymKey[7], PMIU->rgbSymKey[8], PMIU->rgbSymKey[9], PMIU->rgbSymKey[10], PMIU->rgbSymKey[11], PMIU->rgbSymKey[12], PMIU->rgbSymKey[13], PMIU->rgbSymKey[14], PMIU->rgbSymKey[15]);

			Byrom_Dbg("=== XCP Info ===\n%s\n%s\n%s\n%s\n====================", MediaID, MediaInstanceID, Url, Key);

			if (bPrintAddInfo)
			{
				char PackageSize[55];
				sprintf(PackageSize, "Package Size: 0x%016X (%llu bytes)", PMIU->qwPackageSize, PMIU->qwPackageSize);

				char InstallSize[55];
				sprintf(InstallSize, "Install Size: 0x%016X (%llu bytes)", PMIU->qwInstallSize, PMIU->qwInstallSize);

				char PackageType[30];
				sprintf(PackageType, "Package Type: 0x%08X", PMIU->dwPackageType);

				char UrlCount[30];
				sprintf(UrlCount, "Url Count: 0x%08X", PMIU->dwUrlCount);

				char Url0[110];
				sprintf(Url0, "Url[0]: %s", PMIU->pUrls[0].pszUrl);

				char Url1[110];
				sprintf(Url1, "Url[1]: %s", PMIU->dwUrlCount > 1 ? PMIU->pUrls[1].pszUrl : "NULL");

				char Url2[110];
				sprintf(Url2, "Url[2]: %s", PMIU->dwUrlCount > 2 ? PMIU->pUrls[2].pszUrl : "NULL");

				Byrom_Dbg("=== Additional Info ===\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n=======================", PackageSize, InstallSize, PackageType, UrlCount, Url0, Url1, Url2);
			}
		}
		else // Jtag version of xbdm gets weird when printing content containing new lines so we do it all individually
		{
			Byrom_Dbg("=== XCP Info ===");
			Byrom_Dbg("MediaID: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", PMIU->MediaID.Data1, PMIU->MediaID.Data2, PMIU->MediaID.Data3, PMIU->MediaID.Data4[0], PMIU->MediaID.Data4[1], PMIU->MediaID.Data4[2], PMIU->MediaID.Data4[3], PMIU->MediaID.Data4[4], PMIU->MediaID.Data4[5], PMIU->MediaID.Data4[6], PMIU->MediaID.Data4[7]);
			Byrom_Dbg("MediaInstanceID: %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", PMIU->MediaInstanceID.Data1, PMIU->MediaInstanceID.Data2, PMIU->MediaInstanceID.Data3, PMIU->MediaInstanceID.Data4[0], PMIU->MediaInstanceID.Data4[1], PMIU->MediaInstanceID.Data4[2], PMIU->MediaInstanceID.Data4[3], PMIU->MediaInstanceID.Data4[4], PMIU->MediaInstanceID.Data4[5], PMIU->MediaInstanceID.Data4[6], PMIU->MediaInstanceID.Data4[7]);
			Byrom_Dbg("Url: %s", PMIU->pUrls[0].pszUrl);
			Byrom_Dbg("Key: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", PMIU->rgbSymKey[0], PMIU->rgbSymKey[1], PMIU->rgbSymKey[2], PMIU->rgbSymKey[3], PMIU->rgbSymKey[4], PMIU->rgbSymKey[5],
				PMIU->rgbSymKey[6], PMIU->rgbSymKey[7], PMIU->rgbSymKey[8], PMIU->rgbSymKey[9], PMIU->rgbSymKey[10], PMIU->rgbSymKey[11], PMIU->rgbSymKey[12], PMIU->rgbSymKey[13], PMIU->rgbSymKey[14], PMIU->rgbSymKey[15]);
			Byrom_Dbg("====================");

			if (bPrintAddInfo)
			{
				Byrom_Dbg("=== Additional Info ===");
				Byrom_Dbg("Package Size: 0x%016X (%llu bytes)", PMIU->qwPackageSize, PMIU->qwPackageSize);
				Byrom_Dbg("Install Size: 0x%016X (%llu bytes)", PMIU->qwInstallSize, PMIU->qwInstallSize);
				Byrom_Dbg("Package Type: 0x%08X", PMIU->dwPackageType);
				Byrom_Dbg("Url Count: 0x%08X", PMIU->dwUrlCount);
				Byrom_Dbg("Url[0]: %s", PMIU->pUrls[0].pszUrl);
				Byrom_Dbg("Url[1]: %s", PMIU->dwUrlCount > 1 ? PMIU->pUrls[1].pszUrl : "NULL");
				Byrom_Dbg("Url[2]: %s", PMIU->dwUrlCount > 2 ? PMIU->pUrls[2].pszUrl : "NULL");
				Byrom_Dbg("=======================");
			}
		}
	}
}

extern Detour XOnlineFindMediaInstanceUrlsDetour;
extern HRESULT XOnlineFindMediaInstanceUrlsHook(unsigned long unk1, _GUID* GUID1, unsigned long unk2, _FIND_MEDIA_INSTANCE_URLS_RESPONSE* FMIRR, _XOVERLAPPED* overlapped);

// Quick fix - For whatever reason any type of prints done inside the findmediainst hook don't work when using xbWatson.
// This just does it in a system thread instead.
DWORD g_status[1];
VOID DoInfoQuery(void *)
{
	auto Original = XOnlineFindMediaInstanceUrlsDetour.GetOriginal<decltype(&XOnlineFindMediaInstanceUrlsHook)>();

	std::vector<_GUID> fullGuidList;
	readGuidsFromFile(fullGuidList);

	// Query GUIDs in batches of 10.
	for (size_t i = 0; i < fullGuidList.size(); i += 10)
	{
		std::vector<_GUID> myGuids;
		for (size_t j = i; j < i + 10 && j < fullGuidList.size(); j++)
		{
			myGuids.push_back(fullGuidList[j]);
		}

		size_t myOutSize = sizeof(_FIND_MEDIA_INSTANCE_URLS_RESPONSE) +
			sizeof(_MEDIA_INSTANCE_URLS) * myGuids.size() +
			sizeof(_TYPED_MEDIA_URL) * myGuids.size() +
			2048 * myGuids.size();
		std::vector<unsigned char> outBuf(myOutSize);
		_FIND_MEDIA_INSTANCE_URLS_RESPONSE* myFMIRR = (_FIND_MEDIA_INSTANCE_URLS_RESPONSE*)outBuf.data();

		_XOVERLAPPED XOXO = {0};

		Byrom_Dbg("Calling XOnlineFindMediaInstanceUrls");
		Original(myGuids.size(), myGuids.data(), myOutSize, myFMIRR, &XOXO);

		int timeoutcounter = 0;
		while (!XHasOverlappedIoCompleted(&XOXO))
		{
			timeoutcounter++;
			if (timeoutcounter >= 15)
			{
				Byrom_Dbg("XOnlineFindMediaInstanceUrls timed out!!!");
				return;
			}
			//Byrom_Dbg("[FindMedInst Hook] Waiting for XOverlapped to complete");
			Sleep(500);
		}

		//Byrom_Dbg("[FindMedInst Hook] XOverlappedIoCompleted");
		if (myFMIRR->dwMediaInstanceIdsCount != 0)
		{
			DoInfoPrint(myFMIRR);
		}
		//else
		//	Byrom_Dbg("[FindMedInst Hook] MediaInstanceIdsCount is 0! Invalid GUID provided???\n");

		Sleep(1000);
	}

	Byrom_Dbg("Finished dumping XCP info! :D");

	g_status[0] = 1;
	__dcbst(0, g_status);
	__sync();
	__isync();
}


VOID SysPrintXCPInfo()
{
	HANDLE pthread;
	DWORD pthreadid;
	DWORD sta;
	g_status[0] = 0;
	__dcbst(0, g_status);
	__sync();
	__isync();

	sta = ExCreateThread(&pthread, 0, &pthreadid, (PVOID)XapiThreadStartup, (LPTHREAD_START_ROUTINE)DoInfoQuery, NULL, 0x2);
	XSetThreadProcessor(pthread, 4);
	SetThreadPriority(pthread, THREAD_PRIORITY_TIME_CRITICAL);
	ResumeThread(pthread);
	CloseHandle(pthread);

	// wait for thread to run it's course
	//while (g_status[0] == 0)
	//{
	//	Sleep(100);
	//}
}

Detour XOnlineFindMediaInstanceUrlsDetour;
#define XOnlineFindMediaInstanceUrls_Addr_DEVKIT 0x819CC760  // DEVKIT 17489
#define XOnlineFindMediaInstanceUrls_Addr_RETAIL 0x81826580 // RETAIL 17559
HRESULT XOnlineFindMediaInstanceUrlsHook(unsigned long unk1, _GUID* GUID1, unsigned long unk2, _FIND_MEDIA_INSTANCE_URLS_RESPONSE* FMIRR, _XOVERLAPPED* overlapped)
{
	// Prints in here DO NOT WORK! Pointless to have them enabled.
	
	SysPrintXCPInfo();
	
	//auto Original = XOnlineFindMediaInstanceUrlsDetour.GetOriginal<decltype(&XOnlineFindMediaInstanceUrlsHook)>();
	//return Original(unk1, GUID1, unk2, FMIRR, overlapped);
	return ERROR_CAN_NOT_COMPLETE;
}

// These 2 hooks are added to catch title updates since those don't go through the above hook
// Better solutions may exist but I already had this

// # void *__fastcall CXHttp::XHttpOpenRequestUsingMemory(CXHttp *__hidden this, void *, const char *, const char *, const char *, const char *, const char **, void *, unsigned int, unsigned int)
Detour XAMXHttpOpenRequestUsingMemoryDetour;
#define XAM_XHttpOpenRequestUsingMemory_Addr_DEVKIT 0x81A1CD90 // DEVKIT 17489
#define XAM_XHttpOpenRequestUsingMemory_Addr_RETAIL 0x81858238 // RETAIL 17559
DWORD XAMXHttpOpenRequestUsingMemoryHook(DWORD hConnect, const CHAR* pcszVerb, const CHAR* pcszObjectName, const CHAR* pcszVersion, const CHAR* pcszReferrer, const CHAR** ppReserved, PVOID UNKNOWN, DWORD UNKNOWN2, DWORD dwFlags)
{
	auto Original = XAMXHttpOpenRequestUsingMemoryDetour.GetOriginal<decltype(&XAMXHttpOpenRequestUsingMemoryHook)>();

	if (strstr(pcszObjectName, ".xcp")) // Only print for xcp files
	{
		Byrom_Dbg("[XHttp Hook] Url: %s", pcszObjectName);
	}

	DWORD Result = Original(hConnect, pcszVerb, pcszObjectName, pcszVersion, pcszReferrer, ppReserved, UNKNOWN, UNKNOWN2, dwFlags);
	return Result;
}

// void __cdecl CXCabCryptHelper::InitializeDecryption(struct _RC4_SHA_HEADER const *, unsigned long, unsigned char const *, unsigned long)
Detour InitializeDecryptionDetour;
#define InitializeDecryption_Addr_DEVKIT 0x81B0BA10 // DEVKIT 17489
#define InitializeDecryption_Addr_RETAIL 0x818F8A78 // RETAIL 17559
PBYTE PreviousKey = 0;
VOID InitializeDecryptionHook(DWORD CXCabCryptHelper_p,  const RC4_SHA_HEADER* shahead, DWORD unk1, const PBYTE pbKey, DWORD cbKey)
{
	auto Original = InitializeDecryptionDetour.GetOriginal<decltype(&InitializeDecryptionHook)>();

	if (pbKey != PreviousKey) // Prevent it spamming the same key
	{
		PreviousKey = pbKey;
		Byrom_Dbg("[InitDecryption Hook] Key: %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
			pbKey[0], pbKey[1], pbKey[2], pbKey[3], pbKey[4], pbKey[5], pbKey[6], pbKey[7], pbKey[8], pbKey[9], pbKey[10], pbKey[11], pbKey[12], pbKey[13], pbKey[14], pbKey[15]);
	}

	Original(CXCabCryptHelper_p, shahead, unk1, pbKey, cbKey);
}

VOID SetupHooks()
{
	Byrom_Dbg("Applying hooks");

	// Catch most content here
	XOnlineFindMediaInstanceUrlsDetour = Detour((void*)(bIsDevkit ? XOnlineFindMediaInstanceUrls_Addr_DEVKIT : XOnlineFindMediaInstanceUrls_Addr_RETAIL), XOnlineFindMediaInstanceUrlsHook);
	XOnlineFindMediaInstanceUrlsDetour.Install();

	// Catch title update with these
	XAMXHttpOpenRequestUsingMemoryDetour = Detour((void*)(bIsDevkit ? XAM_XHttpOpenRequestUsingMemory_Addr_DEVKIT : XAM_XHttpOpenRequestUsingMemory_Addr_RETAIL), XAMXHttpOpenRequestUsingMemoryHook);
	XAMXHttpOpenRequestUsingMemoryDetour.Install();

	InitializeDecryptionDetour = Detour((void*)(bIsDevkit ? InitializeDecryption_Addr_DEVKIT : InitializeDecryption_Addr_RETAIL), InitializeDecryptionHook);
	InitializeDecryptionDetour.Install();
}

VOID RemoveHooks()
{
	Byrom_Dbg("Removing hooks");

	XOnlineFindMediaInstanceUrlsDetour.Remove();
	XAMXHttpOpenRequestUsingMemoryDetour.Remove();
	InitializeDecryptionDetour.Remove();
}




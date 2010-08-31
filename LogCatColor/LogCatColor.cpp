// LogCatColor.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include <Windows.h>
#include <WinCon.h>
#include <stdio.h>
#include <MLang.h>
#include <comdef.h>

#define	FOREGROUND_YELLOW	(FOREGROUND_RED | FOREGROUND_GREEN)
#define FOREGROUND_CYAN		(FOREGROUND_BLUE | FOREGROUND_GREEN)
#define FOREGROUND_MAGENTA	(FOREGROUND_RED | FOREGROUND_BLUE)
#define FOREGROUND_WHITE	(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define	BACKGROUND_YELLOW	(BACKGROUND_RED | BACKGROUND_GREEN)
#define BACKGROUND_CYAN		(BACKGROUND_BLUE | BACKGROUND_GREEN)
#define BACKGROUND_MAGENTA	(BACKGROUND_RED | BACKGROUND_BLUE)
#define BACKGROUND_WHITE	(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)
#define BACKGROUND_MASK		(BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY)


BOOL TestInstallPackage(LPCSTR line)
{
	return (strncmp(line, "D/PackageManager", 16)==0 
		&& strstr(line, "New package installed in")!=NULL);
}

BOOL GetInstalledPackageName(LPCSTR line, LPSTR cmp, int cmp_size)
{
	LPCSTR start = strstr(line, "/data/app/");
	if (start != NULL) {
		start += 10;
		LPCSTR end = strstr(start, ".apk");
		if (end != NULL) {
			strncpy_s(cmp, cmp_size, start, end - start);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL TestStartActivity(LPCSTR line, LPCSTR cmp, LPSTR pid, int pid_size) 
{
	if (strncmp(line, "I/ActivityManager", 17)==0
		&& strstr(line, "Start proc") != NULL
		&& strstr(line, cmp) != NULL) {

		LPCSTR start = strstr(line, ": pid=");
		if (start != NULL) {
			start += 6;
			LPCSTR end = strstr(start, " ");
			if (end != NULL) {
				strncpy_s(pid, pid_size, start, end - start);
				return TRUE;
			}
		}
	}
	return FALSE;
}

BOOL CheckPID(LPCSTR line, LPCSTR pid)
{
	if (pid[0] == '\0') return FALSE;

	// �ŏ��� '(' ��T��
	for (; *line != '\0'; line++) {
		if (*line == '(') {
			line++;
			break;
		}
	}
	// '('�̎��̃X�y�[�X�𖳎�
	for (; *line == ' '; line++);

	// pid����v���邩�m�F
	for (; *line == *pid; line++, pid++);

	return (*line==')' && *pid == '\0');
}

void HighlightPrint(HANDLE hConsole, WORD attr, LPCSTR line, LPCSTR keyword)
{
	// �p�b�P�[�W�����܂ޕ������n�C���C�g
	SetConsoleTextAttribute(hConsole, attr);
	WriteConsole(hConsole, "\n", 1, NULL, 0);
	if (keyword[0] != '\0') {
		LPCSTR start;
		while (start = strstr(line, keyword)) {
			WriteConsole(hConsole, line, start - line, NULL, 0);
			SetConsoleTextAttribute(hConsole, attr << 4);
			line = start;
			start += strlen(keyword);
			WriteConsole(hConsole, line, start - line, NULL, 0);
			SetConsoleTextAttribute(hConsole, attr);
			line = start;
		}
	}
	WriteConsole(hConsole, line, strlen(line), NULL, 0);
}


int main(int argc, CHAR* argv[])
{
	OleInitialize(0);

	// �R���\�[���̃n���h�����擾
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// IMultiLanguage�̃C���X�^���X�쐬
	IMultiLanguagePtr im = NULL;
	HRESULT hr = im.CreateInstance(__uuidof(CMultiLanguage));
	if (FAILED(hr)) {
		fprintf(stderr, "IMultiLanguage�̃C���X�^���X�����Ȃ�����: %x\n", hr);
		return 1;
	}

	// �W�����͂̓ǂݏo���̃��[�v
	{
		const UINT bufSize = 10000;
		CHAR bufSrc[bufSize+1] = {0};
		CHAR bufDst[bufSize+1] = {0};
		CHAR cmp[100] = {0};
		CHAR pid[100] = {0};
		DWORD convertMode = 0;

		BOOL bStartActivity = FALSE;
		while (gets_s(bufSrc, bufSize) != NULL) {
			UINT bufSrcSize = bufSize;
			UINT bufDstSize = bufSize;
			hr = im->ConvertString(&convertMode, 65001, 932, 
				(BYTE*)&bufSrc, &bufSrcSize, (BYTE*)&bufDst, &bufDstSize);
			if (FAILED(hr)) {
				fprintf(stderr, "IMultiLanguage::ConvertString�����s����: %x\n", hr);
				continue;
			}

			WORD attr = FOREGROUND_WHITE;
			switch (bufSrc[0]) {
			case L'W':
				attr = FOREGROUND_YELLOW | FOREGROUND_INTENSITY;
				break;
			case L'I':
				attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
				if (bStartActivity && TestStartActivity(bufSrc, cmp, pid, sizeof(pid))) {	
					// �C���X�g�[�����ꂽ�p�b�P�[�W��pid���擾
				}
				break;
			case L'D':
				attr = FOREGROUND_CYAN | FOREGROUND_INTENSITY;
				if (TestInstallPackage(bufSrc)) {	// �p�b�P�[�W�̃C���X�g�[�������o
					if (GetInstalledPackageName(bufSrc, cmp, sizeof(cmp))) {
						// �C���X�g�[�����ꂽ�p�b�P�[�W�����擾
						//printf("===%s===\n", cmp);
						//attr = BACKGROUND_CYAN | BACKGROUND_INTENSITY;
						bStartActivity = TRUE;
					}
				}
				break;
			case L'V':
				attr = FOREGROUND_WHITE;
				break;
			case L'E':
				attr = FOREGROUND_RED | FOREGROUND_INTENSITY;
				break;
			default:
				attr = FOREGROUND_WHITE;
				break;
			}

			// pid���ۑ�����Ă���ꍇ�A���O���o�͂��Ă���pid���m�F���āA��v����ꍇ�͍s�S�̂��n�C���C�g
			if (CheckPID(bufSrc, pid)) {
				HighlightPrint(hStdOut, attr, bufDst, pid);
				//SetConsoleTextAttribute(hStdOut, attr << 4);
				//printf("\n%s", bufDst);
			}
			else {
				HighlightPrint(hStdOut, attr, bufDst, cmp);
			}
		}
	}
	
	SetConsoleTextAttribute(hStdOut, FOREGROUND_INTENSITY);
	printf("\n");
	return 0;
}


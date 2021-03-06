// LogCatColor.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
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

	// 最初の '(' を探す
	for (; *line != '\0'; line++) {
		if (*line == '(') {
			line++;
			break;
		}
	}
	// '('の次のスペースを無視
	for (; *line == ' '; line++);

	// pidが一致するか確認
	for (; *line == *pid; line++, pid++);

	return (*line==')' && *pid == '\0');
}

void HighlightPrint(HANDLE hConsole, WORD attr, LPCSTR line, LPCSTR keyword)
{
	// パッケージ名を含む部分をハイライト
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

	// コンソールのハンドルを取得
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// IMultiLanguageのインスタンス作成
	IMultiLanguagePtr im = NULL;
	HRESULT hr = im.CreateInstance(__uuidof(CMultiLanguage));
	if (FAILED(hr)) {
		fprintf(stderr, "IMultiLanguageのインスタンスが作れなかった: %x\n", hr);
		return 1;
	}

	// 標準入力の読み出しのループ
	{
		const CHAR *validLogType = "VDIWEF";
		const UINT bufSize = 10000;
		CHAR bufSrc[bufSize+1] = {0};
		CHAR bufDst[bufSize+1] = {0};
		CHAR cmp[100] = {0};
		CHAR pid[100] = {0};
		DWORD convertMode = 0;

		BOOL bStartActivity = FALSE;
		int errorCount = 0;
		while (true) {
			if (gets_s(bufSrc, bufSize) == NULL) {
				// 文字列として異常なデータ(0x20以下のコードなど)を受信すると、
				// 終端に達していないのにNULLが返されることがあり、この場合の判別方法が不明なので
				// 暫定的に、NULLが返された回数をカウントして、1000回続いたら終了とみなす
				fseek(stdin, 1, SEEK_CUR);
				errorCount++;
				if (errorCount > 1000) {
					break;
				}
				continue;
			}
			errorCount = 0;

			UINT bufSrcSize = bufSize;
			UINT bufDstSize = bufSize;
			hr = im->ConvertString(&convertMode, 65001, 932, 
				(BYTE*)&bufSrc, &bufSrcSize, (BYTE*)&bufDst, &bufDstSize);
			if (FAILED(hr)) {
				fprintf(stderr, "IMultiLanguage::ConvertStringが失敗した: %x\n", hr);
				continue;
			}

			WORD attr = FOREGROUND_WHITE;

			CHAR logType;
			if (strchr(validLogType, bufSrc[0]) != NULL) {
				logType = bufSrc[0];
			}
			else if (strchr(validLogType, bufSrc[19]) != NULL) {	// -v time
				logType = bufSrc[19];
			}
			else if (strchr(validLogType, bufSrc[31]) != NULL) {	// -v threadtime
				logType = bufSrc[31];
			}

			switch (logType) {
			case 'W':
				attr = FOREGROUND_YELLOW | FOREGROUND_INTENSITY;
				break;
			case 'I':
				attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
				if (bStartActivity && TestStartActivity(bufSrc, cmp, pid, sizeof(pid))) {	
					// インストールされたパッケージのpidを取得
				}
				break;
			case 'D':
				attr = FOREGROUND_CYAN | FOREGROUND_INTENSITY;
				if (TestInstallPackage(bufSrc)) {	// パッケージのインストールを検出
					if (GetInstalledPackageName(bufSrc, cmp, sizeof(cmp))) {
						// インストールされたパッケージ名を取得
						//printf("===%s===\n", cmp);
						//attr = BACKGROUND_CYAN | BACKGROUND_INTENSITY;
						bStartActivity = TRUE;
					}
				}
				break;
			case 'V':
				attr = FOREGROUND_WHITE;
				break;
			case 'E':
				attr = FOREGROUND_RED | FOREGROUND_INTENSITY;
				break;
			default:
				attr = FOREGROUND_WHITE;
				break;
			}

			// pidが保存されている場合、ログを出力しているpidを確認して、一致する場合は行全体をハイライト
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


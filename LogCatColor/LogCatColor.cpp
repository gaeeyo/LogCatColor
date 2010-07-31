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
		const UINT bufSize = 10000;
		CHAR bufSrc[bufSize+1] = {0};
		CHAR bufDst[bufSize+1] = {0};
		DWORD convertMode = 0;

		while (gets_s(bufSrc, bufSize) != NULL) {
			UINT bufSrcSize = bufSize;
			UINT bufDstSize = bufSize;
			hr = im->ConvertString(&convertMode, 65001, 932, 
				(BYTE*)&bufSrc, &bufSrcSize, (BYTE*)&bufDst, &bufDstSize);
			if (FAILED(hr)) {
				fprintf(stderr, "IMultiLanguage::ConvertStringが失敗した: %x\n", hr);
				continue;
			}

			switch (bufSrc[0]) {
			case L'W':
				SetConsoleTextAttribute(hStdOut, FOREGROUND_YELLOW | FOREGROUND_INTENSITY);
				break;
			case L'I':
				SetConsoleTextAttribute(hStdOut, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
				break;
			case L'D':
				SetConsoleTextAttribute(hStdOut, FOREGROUND_CYAN | FOREGROUND_INTENSITY);
				break;
			case L'V':
				SetConsoleTextAttribute(hStdOut, FOREGROUND_WHITE);
				break;
			case L'E':
				SetConsoleTextAttribute(hStdOut, FOREGROUND_RED | FOREGROUND_INTENSITY);
				break;
			default:
				SetConsoleTextAttribute(hStdOut, FOREGROUND_WHITE);
				break;
			}
			puts(bufDst);
		}
	}
	
	SetConsoleTextAttribute(hStdOut, FOREGROUND_INTENSITY);
	return 0;
}


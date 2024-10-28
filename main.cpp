#include<Windows.h>
#ifdef _DEBUG
#include<iostream>
#endif

using namespace std;

///@brief コンソール画面にフォーマット付き文字列を表示
///@param format フォーマット(%dとか%fとかの)
///@param 可変長引数
///@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);

#endif // DEBUG
}

#ifdef _DEBUG

int main() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	DebugOutputFormatString("Show window test.");
	getchar();
	return 0;
}
#endif // DEBUG
DebugOutputFormatString("show window test.");
getchar();
return 0;
}


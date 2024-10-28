#include <Windows.h>
#include <tchar.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include <vector>


#ifdef _DEBUG
#include <iostream>
#endif

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace std;

// ウィンドウの幅と高さを指定
const int32_t window_width = 1280;
const int32_t window_height = 720;

#pragma region ID3D12定義

ID3D12Device* _dev = nullptr;
IDXGIFactory6* _dxgiFactory = nullptr;
IDXGISwapChain4* _swapchain = nullptr;
ID3D12CommandAllocator* _cmdAllocator = nullptr;
ID3D12GraphicsCommandList* _cmdList = nullptr;
ID3D12CommandQueue* _cmdQueue = nullptr;
#pragma endregion


HRESULT D3D12CreateDevice(
	IUnknown* pAdapter,//ひとまずはnull
	D3D_FEATURE_LEVEL MinimumFreatureLevel,//最低限必要なフィーチャーレベル
	REFIID riid,//後述
	void** ppDevice//後述
);

///@brief コンソール画面にフォーマット付き文字列を表示
///@param format フォーマット(%dとか%fとかの)
///@param 可変長引数
///@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);  // 修正: printf ではなく vprintf を使う
	va_end(valist);
#endif // DEBUG
}

// WindowProcedure のプロトタイプ宣言
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


// ウィンドウプロシージャ
LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	// ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY) {
		PostQuitMessage(0);  // OSに対して「このアプリは終わる」と伝える
		return 0;
	}
	return DefWindowProc(hwnd, msg, wparam, lparam);  // 規定の処理を行う
}



#ifdef _DEBUG
int main() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	DebugOutputFormatString("Show window test.\n");

	// ウィンドウクラスの生成
	WNDCLASSEX w = {};
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure;  // コールバック関数の指定
	w.lpszClassName = _T("DX12Sample");
	w.hInstance = GetModuleHandle(nullptr);  // 修正: w.hinstance -> w.hInstance

	RegisterClassEx(&w);  // アプリケーションクラス
	RECT wrc{ 0, 0, window_width, window_height };  // ウィンドウサイズを決める

	// 関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

#pragma region Direct3D初期化



	//Direct3Dデバイスの初期化
	D3D_FEATURE_LEVEL featureLevel;


	D3D_FEATURE_LEVEL levels[] = {
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_0,
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0,
	};

	//アダプターの列挙用
	std::vector <IDXGIAdapter*>adapters;
	//ここに特定の名前を持つアダプターにオブジェクトが入る
	IDXGIAdapter* tmpAdapter = nullptr;
	//DXGIFactoryオブジェクト生成
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));

	for (int i = 0;
		_dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
		++i) {
		adapters.push_back(tmpAdapter);

	}

	for (auto adpt:adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);//アダプターの説明オブジェクト取得
		std::wstring strDesc = adesc.Description;

		//探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA")!=std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}

	for (auto lv : levels) {

		if (D3D12CreateDevice(nullptr, lv, IID_PPV_ARGS(&_dev)) == S_OK) {
			featureLevel = lv;
			break;//生成可能なバージョンが見つかったらループを断ち切る
		}

	}
	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,_cmdAllocator,nullptr, IID_PPV_ARGS(&_cmdList));
#pragma endregion

#pragma region コマンドキュー
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	
	//タイムアウトなし
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	//アダプターを一つしか使わないときは0
	cmdQueueDesc.NodeMask = 0;

	//プライオリティは特に指定なし
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	//コマンドリストと合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	//キュー作成
	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));
#pragma endregion



	// ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName,
		_T("DX12テスト"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wrc.right - wrc.left,
		wrc.bottom - wrc.top,
		nullptr,
		nullptr,
		w.hInstance,
		nullptr);
	// ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);



	// メッセージループ
	MSG msg = {};
	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		//アプリケーションが終わるときにメッセージがWM_QUITになる
		if (msg.message == WM_QUIT) {
			break;
		}
	}

	//もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}



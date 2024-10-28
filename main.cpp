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

ID3D12Device*				_dev = nullptr;
IDXGIFactory6*				_dxgiFactory = nullptr;
IDXGISwapChain4*			_swapchain = nullptr;
ID3D12CommandAllocator*		_cmdAllocator = nullptr;
ID3D12GraphicsCommandList*	_cmdList = nullptr;
ID3D12CommandQueue*			_cmdQueue = nullptr;
ID3D12DescriptorHeap*		 rtvHeaps = nullptr;
ID3D12Fence* _fence = nullptr;
UINT64	_fenceVal = 0;
#pragma endregion




HRESULT D3D12CreateDevice(
	IUnknown* pAdapter,//ひとまずはnull
	D3D_FEATURE_LEVEL MinimumFreatureLevel,//最低限必要なフィーチャーレベル
	REFIID riid,//後述
	void** ppDevice//後述
);

#pragma region SwapChain
//SwapChain生成
HRESULT CreateSwapChainForHwnd(
	IUnknown* pDevice,//コマンドキューオブジェクト
	HWND hwnd,//ウィンドウハンドル
	const DXGI_SWAP_CHAIN_DESC1* pDesc,//スワップチェーン設定
	const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,//スワップチェーン設定
	
	IDXGIOutput* pRestrictToOutput,//これもnullptr
	IDXGISwapChain** ppSwapChain//スワップチェーンオブジェクト取得用

);

#pragma endregion

#pragma region fence
HRESULT CreateFence(
	UINT64 InitialiValue,//初期価値(0)
	D3D12_FENCE_FLAGS Flags,//とりあえずD3D12_FENCE_FLAG_NONEでよい
	REFIID riid,
	void** ppFence
);
#pragma endregion






///@brief コンソール画面にフォーマット付き文字列を表示
///@param format フォーマット(%dとか%fとかの)
///@param 可変長引数
///@remarksこの関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...) {
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	vprintf(format, valist);						// 修正: printf ではなく vprintf を使う
	va_end(valist);
#endif // DEBUG
}

void CreateRenderTargetView(
	ID3D12Resource* pResource,//バッファー
	const D3D12_RENDER_TARGET_VIEW_DESC* pDesc,		//今回はnullptr
	D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor		//ディスクリプタヒープハンドル
);

void OMSetRenderTargets(
	UINT numRTVDescriptors,//レンダーターゲット数
	const D3D12_CPU_DESCRIPTOR_HANDLE* pRTVHandles,	//レンダーターゲットハンドルの先頭アドレス

	BOOL RTsSingleHandleToDescriptorRange,			//複数時に連続してるか
	const D3D12_CPU_DESCRIPTOR_HANDLE				//深度ステンシルバッファビューハンドル
	* pDepthStencilDescriptor
);

void ExecuteCommandLists(
	UINT NumCommandLists,							//実行するコマンドリスト数(1でおｋ)
	ID3D12CommandList* const* ppCommandLists		//コマンドリスト配列の先頭アドレス

);

void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(
		IID_PPV_ARGS(&debugLayer));

	debugLayer->EnableDebugLayer();					//デバッグレイヤーを有効化
	debugLayer->Release();							//有効化したらインタフェースを解放する
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
	DebugOutputFormatString("S_OK.\n");

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



#pragma region デバッグレイヤー

#ifdef _DEBUG
	//デバッグレイヤーをオン
	EnableDebugLayer();
#endif // DEBUG

#ifdef _DEBUG
	CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
	CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
#endif
	

#pragma endregion

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

#pragma region DescriptorHeap作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//レンダーターゲットビュー(RTV)
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//表裏の２つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;//特に指定なし
#pragma endregion



	// ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(w.lpszClassName,
		_T("DX12"),
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


#pragma region SwapChain
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};

	swapchainDesc.Width = window_width;
	swapchainDesc.Height = window_height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;

	//バックバッファーは伸び縮み可能
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;

	//フリップ後は速やか破棄
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	//特に指定なし
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	//ウィンドウー＞フルスクリーン切り替え可能
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue,
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)&_swapchain);

	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);

	std::vector<ID3D12Resource*>_backBuffers(swcDesc.BufferCount);

	//DescriptorHeapのリザルト
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	//DescriptorHandle
	D3D12_CPU_DESCRIPTOR_HANDLE handle
		= rtvHeaps->GetCPUDescriptorHandleForHeapStart();

	//ポインタをずらす
	for (int idx = 0; idx < swcDesc.BufferCount; ++idx) {
	result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));

	handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	_dev->CreateRenderTargetView(_backBuffers[idx], nullptr, handle);
	}

	result = _cmdAllocator->Reset();

	


	

	
#pragma endregion





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

		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();

		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

		//画面クリア
		float clearColor[] = { 89.0f / 255.0f, 136.0f / 255.0f, 187.0f / 255.0f, 1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

		//命令のクローズ
		_cmdList->Close();

		// コマンドリスト配列を用意して実行
		ID3D12CommandList* cmdLists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdLists);  // コマンドリストを実行キューに送信

		//フリップ
		_swapchain->Present(1, 0);//垂直同期のため1

		//コマンドリスト、アロケータ両方リセット
		_cmdAllocator->Reset();//キューをクリア
		_cmdList->Reset(_cmdAllocator, nullptr);//再びコマンドリストをためる準備


		
	}

	//もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}



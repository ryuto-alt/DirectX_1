
#ifdef _DEBUG
#include <iostream>
#endif
#include <Windows.h>
#include <tchar.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <d3dx12.h>

#include<d3dcompiler.h>

#include"Vector3.h"

#include "externals/imgui/imgui.h"


#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")




using namespace std;
using namespace DirectX;

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
ID3D12DescriptorHeap* rtvHeaps = nullptr;

ID3D12Fence* _fence = nullptr;
UINT64						_fenceVal = 0;
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

#pragma region Signal
HRESULT Signal(
	ID3D12Fence* pFence,//先ほど作ったフェンスオブジェ
	UINT64 Value		//GPUの処理が完了した後になっているべき値(フェンス値)
);
#pragma endregion

#pragma region SetEventOnCompletion
HRESULT SetEventOnCompletion(
	UINT64 Value,//この値になったらイベントを発生させる
	HANDLE hEvent//発生させるイベント
);
#pragma endregion

#pragma region Matrices
struct MatricesData
{
	XMMATRIX world;
	XMMATRIX viewproj;
};

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
//リソースバリア
void ResourceBarrier(
	UINT NumBarriers,//設定バリアの数
	const D3D12_RESOURCE_BARRIER* pBarriers//設定バリア構造体アドレス
);



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
///アライメントに揃えたサイズを返す
///@param size 元のサイズ
///@param alignment アライメントサイズ
///@return アライメントをそろえたサイズ
size_t AlignmentedSize(size_t size, size_t alignment)
{
	return size + alignment - size & alignment;
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

	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);//アダプターの説明オブジェクト取得
		std::wstring strDesc = adesc.Description;

		//探したいアダプターの名前を確認
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
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
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));
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


#pragma region texDescriptorHeap作成
	ID3D12DescriptorHeap* basicDescHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};

	//シェーダーから見えるように
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	//マスクは0
	descHeapDesc.NodeMask = 0;

	//ビューは今のところ一つだけ
	descHeapDesc.NumDescriptors = 2;

	//シェーダーリソースビュー用
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//生成
	result = _dev->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&basicDescHeap));

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

	//SRGBレンダーターゲットビュー設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	//ポインタをずらす
	for (int idx = 0; idx < swcDesc.BufferCount; ++idx) {
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));

		handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		_dev->CreateRenderTargetView(_backBuffers[idx], &rtvDesc, handle);
	}

	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));
	//result = _cmdAllocator->Reset();

#pragma endregion



#pragma region Vertices
#pragma pack(push, 1)
	struct PMDVertex
	{
		XMFLOAT3 pos;
		XMFLOAT3 normal;
		XMFLOAT2 uv;
		unsigned short noneNo[22];
		unsigned char boneWeight;
		unsigned char edgeFlg;
	};//合計38バイト

#pragma pack(pop)




	//PMDヘッダ構造体
	struct PMDHeader {
		float version; //例：00 00 80 3F == 1.00
		char model_name[20];//モデル名
		char comment[256];//モデルコメント
	};

	char signature[3];
	PMDHeader pmdheader = {};
	constexpr size_t pmdvertex_size = 38;
	auto fp = fopen("Model/初音ミク.pmd", "rb");
	fread(signature, sizeof(signature), 1, fp);
	fread(&pmdheader, sizeof(pmdheader), 1, fp);

	unsigned int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, fp);
	std::vector<unsigned char> vertices(vertNum * pmdvertex_size);
	fread(vertices.data(), vertices.size(), 1, fp);

#pragma endregion





#pragma region heapprop(vertices)

	D3D12_HEAP_PROPERTIES heapprop = {};//ヒーププロパティ

	heapprop.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resdesc = {};

	resdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resdesc.Width = sizeof(vertices);//頂点情報が入るだけのサイズ
	resdesc.Height = 1;
	resdesc.DepthOrArraySize = 1;
	resdesc.MipLevels = 1;
	resdesc.Format = DXGI_FORMAT_UNKNOWN;
	resdesc.SampleDesc.Count = 1;
	resdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* vertBuff = nullptr;

	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	assert(SUCCEEDED(result));
#pragma endregion

#pragma region テクスチャデータの作成
	struct TexRGBA
	{
		unsigned char R, G, B, A;
	};

	std::vector<TexRGBA>texturedata(256 * 256);

	for (auto& rgba : texturedata) {
		rgba.R = rand() % 256;
		rgba.G = rand() % 256;
		rgba.B = rand() % 256;
		rgba.A = 255;//aは1.0とする
	}
#pragma endregion


#pragma region PMDHeader

	D3D12_HEAP_PROPERTIES ModelHeapProp = {};//ヒーププロパティ

	D3D12_RESOURCE_DESC ModelDesc = {};


	ModelHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	ModelDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(vertices[0]));
	result = _dev->CreateCommittedResource(
		&ModelHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&ModelDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff)
	);

	assert(SUCCEEDED(result));

#pragma endregion

#pragma region VertMap

	//頂点情報のコピー
	unsigned char* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	std::copy(std::begin(vertices), std::end(vertices), vertMap);
	vertBuff->Unmap(0, nullptr);
#pragma endregion

#pragma region VS,PSシェーダー読み込み
	ID3DBlob* _vsBlob = nullptr;
	ID3DBlob* _psBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	//VertexShader
	result = D3DCompileFromFile(
		L"BasicVertexShader.hlsl",//シェーダー名
		nullptr,//defineなし
		D3D_COMPILE_STANDARD_FILE_INCLUDE,//インクルードはデフォルト
		"BasicVS", "vs_5_0",//関数はBasicVS 対象シェーダーはvs_5_0
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ用および最適化なし
		0,
		&_vsBlob, &errorBlob	//エラー時はerrorBlobにメッセージが入る
	);
	//PixelShader
	result = D3DCompileFromFile(
		L"BasicPixelShader.hlsl",//シェーダー名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"BasicPS", "ps_5_0",//関数はBasicPS
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//デバッグ用および最適化なし
		0,
		&_psBlob, &errorBlob

	);


#pragma endregion

#pragma region inputLayOut



	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{
		"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0,
	},
		{
		"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT, 0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0,
	},

	{//uv追加
		"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,
		0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0

	},
		{
		"BONE_NO",0,DXGI_FORMAT_R16G16_UINT, 0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0,
	},
		{
		"WEIGHT",0,DXGI_FORMAT_R8_UINT, 0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0,
	},
		{
		"EDGE_FLG",0,DXGI_FORMAT_R8_UINT, 0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0,
	},
	};
#pragma endregion

#pragma region パイプライン

	//パイプライン
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpipeline = {};
	gpipeline.pRootSignature = nullptr;//あとで設定する

	gpipeline.VS.pShaderBytecode = _vsBlob->GetBufferPointer();
	gpipeline.VS.BytecodeLength = _vsBlob->GetBufferSize();
	gpipeline.PS.pShaderBytecode = _psBlob->GetBufferPointer();
	gpipeline.PS.BytecodeLength = _psBlob->GetBufferSize();

	//サンプルマスク
	//デフォルトのサンプルマスクを表す定数
	gpipeline.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//
	gpipeline.BlendState.AlphaToCoverageEnable = false;
	gpipeline.BlendState.IndependentBlendEnable = false;

	D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

	//加算や乗算、αブレンディングは使用しない
	renderTargetBlendDesc.BlendEnable = false;

	//論理演算は使用しない
	renderTargetBlendDesc.LogicOpEnable = false;
	renderTargetBlendDesc.RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	gpipeline.BlendState.RenderTarget[0] = renderTargetBlendDesc;

	//まだアンチエイリアスは使わないためfalse
	gpipeline.RasterizerState.MultisampleEnable = false;
	gpipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない
	gpipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//中身を塗りつぶす
	gpipeline.RasterizerState.DepthClipEnable = true;//深度方向のクリッピングは有効に

	gpipeline.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
	gpipeline.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列の要素数

	gpipeline.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//カットなし

	//三角形で構成
	gpipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	gpipeline.NumRenderTargets = 1;//今は１のみ
	gpipeline.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; //0～１に正規化されたRGBA

	gpipeline.SampleDesc.Count = 1;//サンプリングは１ピクセルにつき１
	gpipeline.SampleDesc.Quality = 0;//クオリティは最低


#pragma region 深度バッファ

	D3D12_RESOURCE_DESC depthResDesc = {};
	depthResDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResDesc.Width = window_width;
	depthResDesc.Height = window_height;
	depthResDesc.DepthOrArraySize = 1;
	depthResDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResDesc.SampleDesc.Count = 1;
	depthResDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	//深度値用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp = {};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	depthHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	depthHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	ID3D12Resource* depthBuffer = nullptr;
	result = _dev->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&depthBuffer));
	assert(SUCCEEDED(result));


	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ID3D12DescriptorHeap* dsvHeap = nullptr;
	result = _dev->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));
	assert(SUCCEEDED(result));

	//深度ビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;


	_dev->CreateDepthStencilView(
		depthBuffer,
		&dsvDesc,
		dsvHeap->GetCPUDescriptorHandleForHeapStart()
	);
	gpipeline.DepthStencilState.DepthEnable = true;
	gpipeline.DepthStencilState.StencilEnable = false;
	gpipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpipeline.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpipeline.DSVFormat = DXGI_FORMAT_D32_FLOAT;


#pragma endregion

#pragma region rootSignature作成

	ID3D12RootSignature* rootsignature = nullptr;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//ディスクリプタテーブル
	D3D12_DESCRIPTOR_RANGE descTblRange[2] = {};
	descTblRange[0].NumDescriptors = 1;//テクスチャ一つ
	descTblRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;//種別はテクスチャ
	descTblRange[0].BaseShaderRegister = 0;//0版スロットから
	descTblRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	descTblRange[1].NumDescriptors = 1;
	descTblRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descTblRange[1].BaseShaderRegister = 0;
	descTblRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//ルートパラメーター
	D3D12_ROOT_PARAMETER rootparam = {};

	rootparam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

	//配列先頭アドレス
	rootparam.DescriptorTable.pDescriptorRanges = &descTblRange[0];

	//ディスクリプタレンジ数
	rootparam.DescriptorTable.NumDescriptorRanges = 2;

	//すべてのシェーダーから見える
	rootparam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootSignatureDesc.pParameters = &rootparam;//ルートパラメーター先頭アドレス
	rootSignatureDesc.NumParameters = 1;

	//rootparam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	////ピクセルシェーダーから見える
	//rootparam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	////ディスクリプタレンジのアドレス
	//rootparam[0].DescriptorTable.pDescriptorRanges = &descTblRange[0];
	////ディスクリプタレンジ数
	//rootparam[0].DescriptorTable.NumDescriptorRanges = 1;

	//rootparam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//rootparam[1].DescriptorTable.pDescriptorRanges = &descTblRange[1];
	//rootparam[1].DescriptorTable.NumDescriptorRanges = 1;
	//rootparam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;


	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};

	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//ボーダーは黒
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;//みっぷマップ最大値
	samplerDesc.MinLOD = 0.0f;//みっぷマップ最小値
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//ピクセルシェーダーから見える
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//リサンプリングしない

	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;


	//バイナリコードの作成
	ID3DBlob* rootSigBlob = nullptr;
	result = D3D12SerializeRootSignature(
		&rootSignatureDesc,//ルートシグネチャ設定
		D3D_ROOT_SIGNATURE_VERSION_1_0,//ルートシグネチャバージョン
		&rootSigBlob,//シェーダーを作ったときと同じ
		&errorBlob //エラー処理も同じ
	);

	result = _dev->CreateRootSignature(
		0,//nodemask 0でよい
		rootSigBlob->GetBufferPointer(),//シェーダーの時と同様
		rootSigBlob->GetBufferSize(),//シェーダーの時と同様
		IID_PPV_ARGS(&rootsignature)
	);

	rootSigBlob->Release();//不要になったので解放

	gpipeline.pRootSignature = rootsignature;
#pragma endregion

	ID3D12PipelineState* _pipelinestate = nullptr;
	result = _dev->CreateGraphicsPipelineState(
		&gpipeline, IID_PPV_ARGS(&_pipelinestate)
	);
#pragma endregion

#pragma region ビューポート
	D3D12_VIEWPORT viewport = {};

	viewport.Width = window_width;//出力先の幅
	viewport.Height = window_height;//出力先の高さ
	viewport.TopLeftX = 0;			//出力先の左上座標X
	viewport.TopLeftY = 0;			//出力先の左上座標Y
	viewport.MaxDepth = 1.0f;		//深度最大値
	viewport.MinDepth = 0.0f;		//深度最小値
#pragma endregion

#pragma region しざー矩形

	D3D12_RECT scissorrect = {};

	scissorrect.top = 0;//切り抜き上座標
	scissorrect.left = 0;//切り抜き左座標
	scissorrect.right = scissorrect.left + window_width;//切り抜き右座標
	scissorrect.bottom = scissorrect.top + window_height;//切り抜き下座標
#pragma endregion

#pragma region WICテクスチャ
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	result = LoadFromWICFile(L"img/uvChecker.png", WIC_FLAGS_NONE, &metadata, scratchImg);
	auto img = scratchImg.GetImage(0, 0, 0);//生データ抽出

#pragma endregion




#pragma region HeapProp(texture)
	D3D12_HEAP_PROPERTIES texheapprop = {};//ヒーププロパティ

	texheapprop.Type = D3D12_HEAP_TYPE_CUSTOM;
	texheapprop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	texheapprop.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	texheapprop.CreationNodeMask = 0;
	texheapprop.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC resDesc = {};

	resDesc.Format = metadata.format;//DXGI_FORMAT_R8G8B8A8_UNORM;//RGBAフォーマット
	resDesc.Width = static_cast<UINT>(metadata.width);//幅
	resDesc.Height = static_cast<UINT>(metadata.height);//高さ
	resDesc.DepthOrArraySize = 1;//2Dで配列でもないので１
	resDesc.SampleDesc.Count = 1;//通常テクスチャなのでアンチェリしない
	resDesc.SampleDesc.Quality = 0;//
	resDesc.MipLevels = 1;//ミップマップしないのでミップ数は１つ
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;//2Dテクスチャ用
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;//レイアウトについては決定しない
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし

	ID3D12Resource* texBuff = nullptr;

	result = _dev->CreateCommittedResource(
		&texheapprop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texBuff));

	result = texBuff->WriteToSubresource(
		0,
		nullptr,//全領域へコピー
		img->pixels,//元データアドレス
		static_cast<UINT>(img->rowPitch),//1ラインサイズ
		static_cast<UINT>(img->slicePitch)//全サイズ
	);

#pragma endregion




#pragma region 定数バッファの作成
	XMMATRIX matrix = XMMatrixIdentity();

	// ワールド行列
	auto worldMat = XMMatrixRotationY(XM_PIDIV4);

	// カメラ位置を変更
	XMFLOAT3 eye(0.0f, 20.0f, -15.0f);  // カメラ位置を下げる
	XMFLOAT3 target(0.0f, 15.0f, 0.0f); // ターゲットを少し上に調整
	XMFLOAT3 up(0.0f, 1.0f, 0.0f);     // 上方向はそのまま

	// ビュー行列
	auto viewMat = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	// 射影行列
	auto projMat = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,
		static_cast<float>(window_width) / static_cast<float>(window_height),
		1.0f,
		100.0f
	);



	ID3D12Resource* constBuff = nullptr;
	heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(MatricesData) + 0xff) & ~0xff);


	_dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuff)
	);

	MatricesData* mapMatrix;
	result = constBuff->Map(0, nullptr, (void**)&mapMatrix);
	mapMatrix->world = worldMat;
	mapMatrix->viewproj = viewMat * projMat;

	auto basicHeapHandle = basicDescHeap->GetCPUDescriptorHandleForHeapStart();
	basicHeapHandle.ptr += _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constBuff->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constBuff->GetDesc().Width;

	//定数バッファビューの作成
	_dev->CreateConstantBufferView(&cbvDesc, basicHeapHandle);
#pragma endregion

	D3D12_HEAP_PROPERTIES uploadHeapProp = {};
	uploadHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;//Upload用
	uploadHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	uploadHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	uploadHeapProp.CreationNodeMask = 0;//単一アダプタのため0
	uploadHeapProp.VisibleNodeMask = 0;//単一アダプタのため0

	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;//単なるバッファとして
	auto pixelsize = scratchImg.GetPixelsSize();
	resDesc.Width = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * img->height;
	resDesc.Height = 1;//
	resDesc.DepthOrArraySize = 1;//
	resDesc.MipLevels = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;//連続したデータですよ
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;//とくにフラグなし
	resDesc.SampleDesc.Count = 1;//通常テクスチャなのでアンチェリしない
	resDesc.SampleDesc.Quality = 0;//


	//中間バッファ作成
	ID3D12Resource* uploadbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&uploadHeapProp,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,//CPUから書き込み可能
		nullptr,
		IID_PPV_ARGS(&uploadbuff)
	);

	//リソース設定(変数は使いまわし)
	resDesc.Format = metadata.format;
	resDesc.Width = static_cast<UINT>(metadata.width);//幅
	resDesc.Height = static_cast<UINT>(metadata.height);//高さ
	resDesc.DepthOrArraySize = static_cast<UINT16>(metadata.arraySize);//2Dで配列でもないので１
	resDesc.MipLevels = static_cast<UINT16>(metadata.mipLevels);//ミップマップしないのでミップ数は１つ
	resDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);//2Dテクスチャ用
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	//テクスチャバッファ作成
	ID3D12Resource* texbuff = nullptr;
	result = _dev->CreateCommittedResource(
		&texheapprop,
		D3D12_HEAP_FLAG_NONE,//特に指定なし
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,//コピー先
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);
	uint8_t* mapforImg = nullptr;//image->pixelsと同じ型にする
	result = uploadbuff->Map(0, nullptr, (void**)&mapforImg);//マップ

	std::copy_n(img->pixels, img->slicePitch, mapforImg);
	uploadbuff->Unmap(0, nullptr);//アンマップ

	D3D12_TEXTURE_COPY_LOCATION src = {}, dst = {};;

	//コピー元
	src.pResource = uploadbuff;
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint.Offset = 0;
	src.PlacedFootprint.Footprint.Width = metadata.width;
	src.PlacedFootprint.Footprint.Height = metadata.height;
	src.PlacedFootprint.Footprint.Depth = metadata.depth;
	src.PlacedFootprint.Footprint.RowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	src.PlacedFootprint.Footprint.Format = img->format;


	dst.pResource = texBuff;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = 0;

	auto srcAddress = img->pixels;
	auto rowPitch = AlignmentedSize(img->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	for (int y = 0; y < img->height; ++y) {
		std::copy_n(srcAddress, rowPitch, mapforImg);

		srcAddress += img->rowPitch;
		mapforImg += rowPitch;
	}

#pragma region vbViewの作成（頂点バッファービュー）

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();//バッファーの仮想アドレス
	vbView.SizeInBytes = vertices.size();//全バイト数
	vbView.StrideInBytes = pmdvertex_size;//1頂点あたりのバイト数
#pragma endregion




#pragma region indices
	std::vector<unsigned short> indices;

	unsigned int indicesNum;
	fread(&indicesNum, sizeof(indicesNum), 1, fp);

	indices.resize(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, fp);

#pragma endregion


#pragma region IndexBuffer

	ID3D12Resource* idxBuff = nullptr;
	D3D12_HEAP_PROPERTIES indicesHeapProp = {};//ヒーププロパティ
	D3D12_RESOURCE_DESC indicesDesc = {};

	heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	resdesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));

	result = _dev->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&idxBuff)
	);
	assert(SUCCEEDED(result));

	//作ったバッファにインデックスデータをコピー
	unsigned short* mappedIdx = nullptr;
	idxBuff->Map(0, nullptr, (void**)&mappedIdx);

	std::copy(std::begin(indices), std::end(indices), mappedIdx);

	idxBuff->Unmap(0, nullptr);

	//インデックスバッファービューを作成
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = idxBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indices.size() * sizeof(indices[0]);

#pragma endregion


#pragma region srcDesc
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	_dev->CreateShaderResourceView(
		texBuff,
		&srvDesc,
		basicDescHeap->GetCPUDescriptorHandleForHeapStart()
	);
#pragma endregion






	// メッセージループ
	MSG msg = {};
	unsigned int frame = 0;
	float angle = 0.0f;

	while (true) {

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//もうアプリケーションが終わるって時にmessageがWM_QUITになる
		if (msg.message == WM_QUIT) {
			break;
		}
		worldMat = XMMatrixRotationY(angle);
		mapMatrix->world = worldMat;
		mapMatrix->viewproj = viewMat * projMat;
		angle += 0.01f;



		//DirectX処理
		//バックバッファのインデックスを取得
		auto bbIdx = _swapchain->GetCurrentBackBufferIndex();

		D3D12_RESOURCE_BARRIER BarrierDesc = {};
		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		BarrierDesc.Transition.pResource = _backBuffers[bbIdx];
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		_cmdList->ResourceBarrier(1, &BarrierDesc);

		_cmdList->SetPipelineState(_pipelinestate);


		//レンダーターゲットを指定
		auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
		rtvH.ptr += bbIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		auto dsvH = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		_cmdList->OMSetRenderTargets(1, &rtvH, false, &dsvH);

		//画面クリア
		float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
		_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);
		++frame;
		_cmdList->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		_cmdList->RSSetViewports(1, &viewport);
		_cmdList->RSSetScissorRects(1, &scissorrect);
		_cmdList->SetGraphicsRootSignature(rootsignature);

		_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_cmdList->IASetVertexBuffers(0, 1, &vbView);
		_cmdList->IASetIndexBuffer(&ibView);

		_cmdList->SetGraphicsRootSignature(rootsignature);
		_cmdList->SetDescriptorHeaps(1, &basicDescHeap);
		_cmdList->SetGraphicsRootDescriptorTable(0, basicDescHeap->GetGPUDescriptorHandleForHeapStart());

	
		_cmdList->DrawIndexedInstanced(indicesNum, 1, 0, 0, 0);
		//_cmdList->DrawInstanced(vertNum, 1, 0, 0);
		BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		_cmdList->ResourceBarrier(1, &BarrierDesc);

		//命令のクローズ
		_cmdList->Close();



		//コマンドリストの実行
		ID3D12CommandList* cmdlists[] = { _cmdList };
		_cmdQueue->ExecuteCommandLists(1, cmdlists);
		////待ち
		_cmdQueue->Signal(_fence, ++_fenceVal);

		if (_fence->GetCompletedValue() != _fenceVal) {
			auto event = CreateEvent(nullptr, false, false, nullptr);
			_fence->SetEventOnCompletion(_fenceVal, event);
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
		_cmdAllocator->Reset();//キューをクリア
		_cmdList->Reset(_cmdAllocator, _pipelinestate);//再びコマンドリストをためる準備



		//フリップ
		_swapchain->Present(1, 0);

	}
	//もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);

	return 0;
}
#include "win32.h"
#include <string>
#include <system_error>
#include <cassert>
#include <sstream>
#include <vector>
#include <windows.h>
#include <wrl/client.h>

#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

#include <directxmath.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

#define COM_CHECK(expr) do { HRESULT _hr = (expr); if (FAILED(_hr)) throw_com_error(#expr, _hr); } while (0)

namespace skirmish {

void throw_system_error(const std::string& what, const unsigned error_code = GetLastError())
{
    assert(error_code != ERROR_SUCCESS);
    throw std::system_error(error_code, std::system_category(), what);
}

void throw_com_error(const std::string& what, HRESULT hr) {
    assert(FAILED(hr));
    std::ostringstream oss;
    oss << what << ". HRESULT = 0x" << std::hex << hr;
    throw std::runtime_error(oss.str());
}

std::string wide_to_mb(const wchar_t* wide) {
    auto convert = [wide](char* mb, int mblen) { return WideCharToMultiByte(CP_ACP, 0, wide, -1, mb, mblen, nullptr, nullptr); };
    const int size = convert(nullptr, 0);
    if (!size) {
        throw_system_error("WideCharToMultiByte");
    }
    std::string result(size, '\0');
    if (!convert(&result[0], size)) {
        throw_system_error("WideCharToMultiByte");
    }
    assert(result.back() == '\0');
    result.pop_back();
    return result;
}

template<typename Derived>
class window_base {
public:
    explicit window_base() {
    }

    ~window_base() {
        if (hwnd_) {
            assert(IsWindow(hwnd_));
            DestroyWindow(hwnd_);
        }
        assert(!hwnd_);
    }

    void show() {
        assert(hwnd_);
        ShowWindow(hwnd_, SW_SHOWDEFAULT);
    }

    HWND hwnd() {
        assert(hwnd_);
        return hwnd_;
    }

protected:
    void create() {
        assert(!hwnd_);
        register_class();

        if (!CreateWindow(Derived::class_name(), L"Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(nullptr), this)) {
            throw_system_error("CreateWindow");
        }
        assert(hwnd_);
    }

private:
    HWND hwnd_ = nullptr;

    static void register_class() {
        WNDCLASS wc ={0,};
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = s_wndproc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = GetModuleHandle(nullptr);
        wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszMenuName  = nullptr;
        wc.lpszClassName = Derived::class_name();

        if (!RegisterClass(&wc)) {
            // Todo: allow class to already be registered
            throw_system_error("RegisterClass");
        }
    }

    static LRESULT CALLBACK s_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        window_base* self = nullptr;
        if (msg == WM_NCCREATE) {
            auto cs = *reinterpret_cast<const CREATESTRUCT*>(lparam);
            self = reinterpret_cast<window_base*>(cs.lpCreateParams);
            self->hwnd_ = hwnd;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        } else {
            self = reinterpret_cast<window_base*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (msg == WM_NCDESTROY && self) {
            auto ret = DefWindowProc(hwnd, msg, wparam, lparam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
            self->hwnd_ = nullptr;
            return ret;
        }

        return self ? self->wndproc(hwnd, msg, wparam, lparam) : DefWindowProc(hwnd, msg, wparam, lparam);
    }

    LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
        assert(hwnd_ == hwnd);
        switch (msg) {
        case WM_CHAR:
            SendMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case WM_PAINT:
            static_cast<Derived*>(this)->on_paint();
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        }
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
};

ComPtr<ID3DBlob> compile_shader(const char* data, const char* entry_point, const char* target)
{
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error_messages;
    HRESULT hr = D3DCompile(data, strlen(data), nullptr, nullptr, nullptr, entry_point, target, dwShaderFlags, 0, &blob, &error_messages);
    if (FAILED(hr)) {
        std::string errmsg = "D3DCompile failed";
        if (error_messages) {
            errmsg += ": ";
            errmsg += reinterpret_cast<const char*>(error_messages->GetBufferPointer());
        }
        throw_com_error(errmsg, hr);
    }

    return blob;
}

// HRESULT CreateVertexShader(
// [in]            const void               *pShaderBytecode,
// [in]                  SIZE_T             BytecodeLength,
// [in, optional]        ID3D11ClassLinkage *pClassLinkage,
// [out, optional]       ID3D11VertexShader **ppVertexShader
// );

template<typename ShaderType>
struct shader_traits;

template<>
struct shader_traits<ID3D11VertexShader> {
    static constexpr const char* const target = "vs_4_0";
    static constexpr auto create_function = &ID3D11Device::CreateVertexShader;
};

template<>
struct shader_traits<ID3D11PixelShader> {
    static constexpr const char* const target = "ps_4_0";
    static constexpr auto create_function = &ID3D11Device::CreatePixelShader;
};

template<typename ShaderType>
void create_shader(ID3D11Device* device, const char* source, const char* entry_point, ShaderType** shader, ComPtr<ID3DBlob>* blob_out = nullptr)
{
    auto blob = compile_shader(source, entry_point, shader_traits<ShaderType>::target);
    COM_CHECK((device->*shader_traits<ShaderType>::create_function)(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, shader));
    if (blob_out) {
        *blob_out = std::move(blob);
    }
}

ComPtr<ID3D11Buffer> create_buffer(ID3D11Device* device, D3D11_BIND_FLAG bind_flag, const void* data, UINT data_size)
{
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage            = D3D11_USAGE_DEFAULT;
    bd.ByteWidth        = data_size;
    bd.BindFlags        = bind_flag;
    bd.CPUAccessFlags   = 0;

    D3D11_SUBRESOURCE_DATA init_data;
    ZeroMemory(&init_data, sizeof(init_data));
    init_data.pSysMem = data;

    ComPtr<ID3D11Buffer> buffer;
    COM_CHECK(device->CreateBuffer(&bd, data ? &init_data : nullptr, &buffer));

    return buffer;
}

const char shader_source[] = 
R"(
//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register( b0 )
{
	matrix World;
	matrix View;
	matrix Projection;
}

//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS( float4 Pos : POSITION, float4 Color : COLOR )
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul( Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Color = Color;
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( VS_OUTPUT input ) : SV_Target
{
    return input.Color;
}
)";

class my_object {
public:
    explicit my_object(ID3D11Device* device) {
        ComPtr<ID3DBlob> vs_blob;
        create_shader(device, shader_source, "VS", vs.GetAddressOf(), &vs_blob);
        create_shader(device, shader_source, "PS", ps.GetAddressOf());

        // Define the input layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        UINT numElements = ARRAYSIZE(layout);

        // Create the input layout
        COM_CHECK(device->CreateInputLayout(layout, numElements, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &vertex_layout));

        // Create vertex buffer
        std::vector<SimpleVertex> vertices =
        {
            {XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
            {XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
            {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)},
            {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
            {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)},
            {XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)},
            {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)},
            {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)},
        };
        vertex_buffer = create_buffer(device, D3D11_BIND_VERTEX_BUFFER, vertices.data(), static_cast<UINT>(vertices.size() * sizeof(vertices[0])));

        // Create index buffer
        std::vector<WORD> indices =
        {
            3,1,0,
            2,1,3,

            0,5,4,
            1,5,0,

            3,4,7,
            0,4,3,

            1,6,5,
            2,6,1,

            2,7,6,
            3,7,2,

            6,4,5,
            7,4,6,
        };
        index_count = static_cast<UINT>(indices.size());
        index_buffer = create_buffer(device, D3D11_BIND_INDEX_BUFFER, indices.data(), static_cast<UINT>(indices.size() * sizeof(indices[0])));

        // Create constant buffer
        constant_buffer = create_buffer(device, D3D11_BIND_CONSTANT_BUFFER, nullptr, sizeof(ConstantBuffer));
    }

    void draw(ID3D11DeviceContext* immediate_context) {
        // Set the input layout
        immediate_context->IASetInputLayout(vertex_layout.Get());

        // Set vertex buffer
        UINT stride = sizeof(SimpleVertex);
        UINT offset = 0;
        ID3D11Buffer* vertex_buffers[] = { vertex_buffer.Get() };
        immediate_context->IASetVertexBuffers(0, _countof(vertex_buffers), vertex_buffers, &stride, &offset);

        // Set index buffer
        immediate_context->IASetIndexBuffer(index_buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

        // Set primitive topology
        immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        LARGE_INTEGER freq, count;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&count);
        static LONGLONG init = count.QuadPart;

        const float t = static_cast<float>(static_cast<double>(count.QuadPart - init) / freq.QuadPart);

        // Initialize the world matrix
        XMMATRIX world = XMMatrixRotationRollPitchYaw(t * 0.1f, t * -0.5f, t * 1.2f);

        // Initialize the view matrix
        XMVECTOR Eye = XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
        XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMMATRIX view = XMMatrixLookAtLH(Eye, At, Up);

        // Initialize the projection matrix
        XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, /*width / (FLOAT)height*/ 640.0f/480.0f, 0.01f, 100.0f);

        // Update constant buffer
        ConstantBuffer cb;
        cb.mWorld = XMMatrixTranspose(world);
        cb.mView = XMMatrixTranspose(view);
        cb.mProjection = XMMatrixTranspose(projection);
        immediate_context->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &cb, 0, 0);

        // Set constant buffer and shaders
        ID3D11Buffer* constant_buffers[] = { constant_buffer.Get() };
        immediate_context->VSSetConstantBuffers(0, _countof(constant_buffers), constant_buffers);

        immediate_context->VSSetShader(vs.Get(), nullptr, 0);
        immediate_context->PSSetShader(ps.Get(), nullptr, 0);

        // Draw!
        immediate_context->DrawIndexed(index_count, 0, 0);        // 36 vertices needed for 12 triangles in a triangle list

    }

    my_object(const my_object&) = delete;
    my_object& operator=(const my_object&) = delete;

private:
    ComPtr<ID3D11VertexShader>  vs;
    ComPtr<ID3D11PixelShader>   ps;
    ComPtr<ID3D11InputLayout>   vertex_layout;
    ComPtr<ID3D11Buffer>        vertex_buffer;
    ComPtr<ID3D11Buffer>        index_buffer;
    ComPtr<ID3D11Buffer>        constant_buffer;
    UINT                        index_count;

    struct SimpleVertex {
        XMFLOAT3 Pos;
        XMFLOAT4 Color;
    };

    struct ConstantBuffer {
        XMMATRIX mWorld;
        XMMATRIX mView;
        XMMATRIX mProjection;
    };
};



class d3d11_window::impl : public window_base<d3d11_window::impl> {
public:
    explicit impl(unsigned width, unsigned height) {
        create();

        // Don't allow maximize/resize
        SetWindowLong(hwnd(), GWL_STYLE, GetWindowLong(hwnd(), GWL_STYLE) & ~(WS_MAXIMIZEBOX | WS_SIZEBOX));

        // Adjust window size to match width/height
        RECT rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
        AdjustWindowRectEx(&rect, GetWindowLong(hwnd(), GWL_STYLE), FALSE, GetWindowLong(hwnd(), GWL_EXSTYLE));
        SetWindowPos(hwnd(), HWND_TOP, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE);

        //
        // Create device and swap chain
        //
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 1;
        sd.BufferDesc.Width = width;
        sd.BufferDesc.Height = height;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd();
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        const auto device_flags  = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
        const auto feature_level = D3D_FEATURE_LEVEL_11_0;
        COM_CHECK(D3D11CreateDeviceAndSwapChain(
            nullptr,                               // pAdapter
            D3D_DRIVER_TYPE_HARDWARE,              // DriverType
            nullptr,                               // Software
            device_flags,                          // Flags
            &feature_level,                        // pFeatureLevels
            1,                                     // FeatureLevels
            D3D11_SDK_VERSION,                     // SDKVersion
            &sd,                                   // pSwapChainDesc
            &swap_chain,                           // ppSwapChain
            &device,                               // ppDevice
            nullptr,                               // pFeatureLevel
            &immediate_context                     // ppImmediateContext
        ));

        // Get a pointer to the back buffer
        ComPtr<ID3D11Texture2D> back_buffer;
        COM_CHECK(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)));

        // Create a render-target view
        COM_CHECK(device->CreateRenderTargetView(back_buffer.Get(), nullptr, &render_target_view));

        // Bind the view
        ID3D11RenderTargetView* targets[] = {render_target_view.Get()};
        immediate_context->OMSetRenderTargets(_countof(targets), targets, nullptr);

        // Setup viewport
        D3D11_VIEWPORT vp;
        vp.Width = static_cast<FLOAT>(width);
        vp.Height = static_cast<FLOAT>(height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        immediate_context->RSSetViewports(1, &vp);

        obj_ = std::make_unique<my_object>(device.Get());
    }

    static const wchar_t* class_name() {
        return L"d3d11_window";
    }

    void on_paint() {
        float clear_color[4] = {0.0f, 0.125f, 0.6f, 1.0f}; // RGBA
        immediate_context->ClearRenderTargetView(render_target_view.Get(), clear_color);
        obj_->draw(immediate_context.Get());
        swap_chain->Present(0, 0);
    }

private:
    ComPtr<IDXGISwapChain>          swap_chain;
    ComPtr<ID3D11Device>            device;
    ComPtr<ID3D11DeviceContext>     immediate_context;
    ComPtr<ID3D11RenderTargetView>  render_target_view;
    std::unique_ptr<my_object>      obj_;
};

d3d11_window::d3d11_window() : impl_(new impl{640, 480})
{
}

d3d11_window::~d3d11_window() = default;

void d3d11_window::show()
{
    impl_->show();
}

int run_message_loop(const on_idle_func& on_idle)
{
    assert(on_idle);

    for (;;) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return static_cast<int>(msg.wParam);
            }
     
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        on_idle();
    }
}


} // namespace skirmish
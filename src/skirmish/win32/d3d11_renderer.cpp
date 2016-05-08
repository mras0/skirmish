#include "d3d11_renderer.h"
#include <cassert>
#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <Windows.h>
#include <wrl/client.h>

#include <d3d11.h>
#include <d3dcompiler.h>

#include <directxmath.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;

#define COM_CHECK(expr) do { HRESULT _hr = (expr); if (FAILED(_hr)) throw_com_error(#expr, _hr); } while (0)

namespace {

template<typename T, typename tag>
XMVECTOR make_xmvec(const skirmish::vec<3, T, tag>& v, const T& w) {
    return XMVectorSet(v.x(), v.y(), v.z(), w);
}

} // unnamed namespace

namespace skirmish {

void throw_com_error(const std::string& what, HRESULT hr) {
    assert(FAILED(hr));
    std::ostringstream oss;
    oss << what << ". HRESULT = 0x" << std::hex << hr;
    throw std::runtime_error(oss.str());
}

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

struct ConstantBuffer {
    XMMATRIX mWorld;
    XMMATRIX mView;
    XMMATRIX mProjection;
};

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

Texture2D the_texture;

SamplerState the_texture_sampler;

//--------------------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
    float4 Tex0 : TEXCOORD0;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS( float4 Pos : POSITION, float2 Tex : TEXCOORD0 /*, float4 Color : COLOR */)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul( Pos, World );
    output.Pos = mul( output.Pos, View );
    output.Pos = mul( output.Pos, Projection );
    output.Color.rgb = 1;//abs(Pos.z*4);
    output.Color.a = 1;
    output.Tex0 = float4(Tex.xy, 0, 0);
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( VS_OUTPUT input ) : SV_Target
{
    return the_texture.Sample(the_texture_sampler, input.Tex0.xy) * input.Color;
}
)";

class d3d11_render_context {
public:
    ID3D11DeviceContext* immediate_context;
    ConstantBuffer       contants;
};

class d3d11_create_context {
public:
    ID3D11Device* device;
};

class d3d11_texture::impl {
public:
    explicit impl(ID3D11Device* device, const util::array_view<uint32_t>& rgba_data, uint32_t width, uint32_t height) {
        D3D11_TEXTURE2D_DESC tex_desc;
        ZeroMemory(&tex_desc, sizeof(tex_desc));
        tex_desc.Width              = width;
        tex_desc.Height             = height;
        tex_desc.MipLevels          = 1;
        tex_desc.ArraySize          = 1;
        tex_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        tex_desc.SampleDesc.Count   = 1; // Default
        tex_desc.SampleDesc.Quality = 0; // Default
        tex_desc.Usage              = D3D11_USAGE_DEFAULT;
        tex_desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
        tex_desc.CPUAccessFlags     = 0; // No CPU access after creation
        tex_desc.MiscFlags          = 0;

        D3D11_SUBRESOURCE_DATA initial_data = { &rgba_data[0], width*sizeof(rgba_data[0]), 0};
        ComPtr<ID3D11Texture2D> texture;
        COM_CHECK(device->CreateTexture2D(&tex_desc, &initial_data, texture.GetAddressOf()));

        D3D11_SHADER_RESOURCE_VIEW_DESC view_desc;
        ZeroMemory(&view_desc, sizeof(view_desc));
        view_desc.Format = tex_desc.Format;
        view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        view_desc.Texture2D.MipLevels = tex_desc.MipLevels;
        view_desc.Texture2D.MostDetailedMip = 0;
        COM_CHECK(device->CreateShaderResourceView(texture.Get(), &view_desc, view.GetAddressOf()));
    }

    ComPtr<ID3D11ShaderResourceView> view;
};

d3d11_texture::d3d11_texture(d3d11_renderer& renderer, const util::array_view<uint32_t>& rgba_data, uint32_t width, uint32_t height) : impl_(new impl{renderer.create_context().device, rgba_data, width, height})
{
}

d3d11_texture::~d3d11_texture() = default;

ID3D11ShaderResourceView * d3d11_texture::view()
{
    return impl_->view.Get();
}

class d3d11_simple_obj::impl {
public:
    explicit impl(d3d11_renderer& renderer, const util::array_view<simple_vertex>& vertices, const util::array_view<uint16_t>& indices) {
        auto device = renderer.create_context().device;
        ComPtr<ID3DBlob> vs_blob;
        create_shader(device, shader_source, "VS", vs.GetAddressOf(), &vs_blob);
        create_shader(device, shader_source, "PS", ps.GetAddressOf());

        // Define the input layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION" , 0 , DXGI_FORMAT_R32G32B32_FLOAT , 0, 0                            , D3D11_INPUT_PER_VERTEX_DATA , 0 },
            { "TEXCOORD" , 0 , DXGI_FORMAT_R32G32_FLOAT    , 0, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_VERTEX_DATA , 0 },
            //{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        UINT numElements = ARRAYSIZE(layout);

        static_assert(sizeof(simple_vertex) == 5*sizeof(float), "");

        // Create the input layout
        COM_CHECK(device->CreateInputLayout(layout, numElements, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &vertex_layout));

        // Create vertex buffer
        vertex_buffer = create_buffer(device, D3D11_BIND_VERTEX_BUFFER, vertices.data(), static_cast<UINT>(vertices.size() * sizeof(vertices[0])));

        // Create index buffer
        index_count = static_cast<UINT>(indices.size());
        index_buffer = create_buffer(device, D3D11_BIND_INDEX_BUFFER, indices.data(), static_cast<UINT>(indices.size() * sizeof(indices[0])));

        // Create constant buffer
        constant_buffer = create_buffer(device, D3D11_BIND_CONSTANT_BUFFER, nullptr, sizeof(ConstantBuffer));

        // Create sampler
        D3D11_SAMPLER_DESC sampler_desc;
        ZeroMemory(&sampler_desc, sizeof(sampler_desc));
        sampler_desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler_desc.AddressU =  D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressV =  D3D11_TEXTURE_ADDRESS_WRAP;
        sampler_desc.AddressW =  D3D11_TEXTURE_ADDRESS_WRAP;
        device->CreateSamplerState(&sampler_desc, sampler_state.GetAddressOf());


        // This might not be a great idea?
        device->GetImmediateContext(immediate_context.GetAddressOf());

        transform = world_transform::identity();
    }

    void do_render(d3d11_render_context& render_context) {
        ConstantBuffer constants = render_context.contants;
        static_assert(sizeof(transform) == sizeof(XMMATRIX), "");
        memcpy(&constants.mWorld, &transform, sizeof(transform));

        // Update constant buffer
        immediate_context->UpdateSubresource(constant_buffer.Get(), 0, nullptr, &constants, 0, 0);

        // Set constant buffer and shaders
        ID3D11Buffer* constant_buffers[] = { constant_buffer.Get() };
        immediate_context->VSSetConstantBuffers(0, _countof(constant_buffers), constant_buffers);

        // Set the input layout
        immediate_context->IASetInputLayout(vertex_layout.Get());

        // Set vertex buffer
        UINT stride = sizeof(simple_vertex);
        UINT offset = 0;
        ID3D11Buffer* vertex_buffers[] = { vertex_buffer.Get() };
        immediate_context->IASetVertexBuffers(0, _countof(vertex_buffers), vertex_buffers, &stride, &offset);

        // Set index buffer
        immediate_context->IASetIndexBuffer(index_buffer.Get(), DXGI_FORMAT_R16_UINT, 0);

        // Set primitive topology
        immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        ID3D11ShaderResourceView* shader_resources[] = { texture_view.Get() };
        immediate_context->PSSetShaderResources(0, _countof(shader_resources), shader_resources);

        // Set sampler
        ID3D11SamplerState* sampler_states[] = { sampler_state.Get() };
        immediate_context->PSSetSamplers(0, _countof(sampler_states), sampler_states);

        immediate_context->VSSetShader(vs.Get(), nullptr, 0);
        immediate_context->PSSetShader(ps.Get(), nullptr, 0);

        // Draw!
        immediate_context->DrawIndexed(index_count, 0, 0);
    }

    void update_vertices(const util::array_view<simple_vertex>& vertices) {
        immediate_context->UpdateSubresource(vertex_buffer.Get(), 0, nullptr, vertices.data(), 0, 0);
    }

    void set_world_transform(const world_transform& xform) {
        transform = xform;
    }

    void set_texture(d3d11_texture& texture) {
        texture_view.Reset();
        texture_view = texture.view();
    }

private:
    ComPtr<ID3D11VertexShader>       vs;
    ComPtr<ID3D11PixelShader>        ps;
    ComPtr<ID3D11InputLayout>        vertex_layout;
    ComPtr<ID3D11Buffer>             vertex_buffer;
    ComPtr<ID3D11Buffer>             index_buffer;
    ComPtr<ID3D11Buffer>             constant_buffer;
    ComPtr<ID3D11ShaderResourceView> texture_view;
    ComPtr<ID3D11SamplerState>       sampler_state;
    ComPtr<ID3D11DeviceContext>      immediate_context;
    UINT                             index_count;
    world_transform                  transform;
};

d3d11_simple_obj::d3d11_simple_obj(d3d11_renderer& renderer, const util::array_view<simple_vertex>& vertices, const util::array_view<uint16_t>& indices) : impl_(new impl{renderer, vertices, indices}) {
}

d3d11_simple_obj::~d3d11_simple_obj() = default;

void d3d11_simple_obj::update_vertices(const util::array_view<simple_vertex>& vertices) {
    impl_->update_vertices(vertices);
}

void d3d11_simple_obj::set_texture(d3d11_texture& texture)
{
    impl_->set_texture(texture);
}

void d3d11_simple_obj::set_world_transform(const world_transform & xform)
{
    impl_->set_world_transform(xform);
}

void d3d11_simple_obj::do_render(d3d11_render_context& context) {
    impl_->do_render(context);
}

class d3d11_renderer::impl {
public:
    explicit impl(win32_main_window& window) {
        auto hwnd = window.native_handle();

        RECT client_rect;
        GetClientRect(hwnd, &client_rect);
        const int width = client_rect.right - client_rect.left;
        const int height = client_rect.bottom - client_rect.top;


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
        sd.OutputWindow = hwnd;
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
            &swap_chain_,                          // ppSwapChain
            &device_,                              // ppDevice
            nullptr,                               // pFeatureLevel
            &immediate_context_                    // ppImmediateContext
        ));

        // Get a pointer to the back buffer
        ComPtr<ID3D11Texture2D> back_buffer;
        COM_CHECK(swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer)));

        // Create a render-target view
        COM_CHECK(device_->CreateRenderTargetView(back_buffer.Get(), nullptr, &render_target_view_));

        // Set depth test state
        D3D11_DEPTH_STENCIL_DESC ds_test_desc;
        ZeroMemory(&ds_test_desc, sizeof(ds_test_desc));
        ds_test_desc.DepthEnable = TRUE;
        ds_test_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        ds_test_desc.DepthFunc = D3D11_COMPARISON_LESS;
        ComPtr<ID3D11DepthStencilState> ds_test;
        COM_CHECK(device_->CreateDepthStencilState(&ds_test_desc, &ds_test));
        immediate_context_->OMSetDepthStencilState(ds_test.Get(), 0);

        
        // Create depth buffer
        D3D11_TEXTURE2D_DESC back_buffer_desc;
        back_buffer->GetDesc(&back_buffer_desc);

        D3D11_TEXTURE2D_DESC depth_desc;
        depth_desc.Width = back_buffer_desc.Width;
        depth_desc.Height = back_buffer_desc.Height;
        depth_desc.MipLevels = 1;
        depth_desc.ArraySize = 1;
        depth_desc.Format = DXGI_FORMAT_D32_FLOAT;
        depth_desc.SampleDesc.Count = 1;
        depth_desc.SampleDesc.Quality = 0;
        depth_desc.Usage = D3D11_USAGE_DEFAULT;
        depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depth_desc.CPUAccessFlags = 0;
        depth_desc.MiscFlags = 0;
        ComPtr<ID3D11Texture2D> depth_stencil;
        COM_CHECK(device_->CreateTexture2D(&depth_desc, nullptr, &depth_stencil));

        // Create depth buffer view
        COM_CHECK(device_->CreateDepthStencilView(depth_stencil.Get(), nullptr, &depth_stencil_view_));

        // Bind the views
        ID3D11RenderTargetView* targets[] = {render_target_view_.Get()};
        immediate_context_->OMSetRenderTargets(_countof(targets), targets, depth_stencil_view_.Get());

        // Setup viewport
        D3D11_VIEWPORT vp;
        vp.Width = static_cast<FLOAT>(width);
        vp.Height = static_cast<FLOAT>(height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        immediate_context_->RSSetViewports(1, &vp);

        create_context_.device = device_.Get();

        LARGE_INTEGER freq, count;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&count);
        static LONGLONG init = count.QuadPart;


        // Initialize constant buffer
        XMMATRIX world = XMMatrixIdentity();

        // Initialize the view matrix
        XMVECTOR Eye = XMVectorSet(2.0f, 2.0f, 2.0f, 0.0f);
        XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
        XMVECTOR Up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        XMMATRIX view = XMMatrixLookAtLH(Eye, At, Up);

        // Initialize the projection matrix
        XMMATRIX projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, /*width / (FLOAT)height*/ 640.0f/480.0f, 0.01f, 100.0f);
        constants_.mWorld = XMMatrixTranspose(world);
        constants_.mView = XMMatrixTranspose(view);
        constants_.mProjection = XMMatrixTranspose(projection);

    }

    d3d11_create_context& create_context() {
        return create_context_;
    }

    void set_view(const world_pos& camera_pos, const world_pos& camera_target) {
        XMMATRIX view = XMMatrixLookAtLH(make_xmvec(camera_pos, 0.0f), make_xmvec(camera_target, 0.0f), make_xmvec(world_up, 0.0f));
        constants_.mView = XMMatrixTranspose(view);
    }

    void render() {
        // Clear render target
        float clear_color[4] = {0.0f, 0.125f, 0.6f, 1.0f}; // RGBA
        immediate_context_->ClearRenderTargetView(render_target_view_.Get(), clear_color);

        // Clear depth buffer
        immediate_context_->ClearDepthStencilView(depth_stencil_view_.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

        d3d11_render_context render_context {
            immediate_context_.Get(),
            constants_,
        };
        for (auto r : renderables_) {
            r->do_render(render_context);
        }

        swap_chain_->Present(0, 0);
    }

    void add_renderable(d3d11_renderable& r) {
        renderables_.push_back(&r);
    }

private:
    ComPtr<IDXGISwapChain>          swap_chain_;
    ComPtr<ID3D11Device>            device_;
    ComPtr<ID3D11DeviceContext>     immediate_context_;
    ComPtr<ID3D11RenderTargetView>  render_target_view_;
    ComPtr<ID3D11DepthStencilView>  depth_stencil_view_;
    XMMATRIX                        view_matrix;
    XMMATRIX                        projection_matrix;
    std::vector<d3d11_renderable*>  renderables_;
    d3d11_create_context            create_context_;
    ConstantBuffer                  constants_;
};

d3d11_renderer::d3d11_renderer(win32_main_window& window) : impl_(new impl{window})
{
}

d3d11_renderer::~d3d11_renderer() = default;

d3d11_create_context& d3d11_renderer::create_context()
{
    return impl_->create_context();
}

void d3d11_renderer::set_view(const world_pos& camera_pos, const world_pos& camera_target)
{
    impl_->set_view(camera_pos, camera_target);
}

void d3d11_renderer::render()
{
    impl_->render();
}

void d3d11_renderer::add_renderable(d3d11_renderable& r)
{
    impl_->add_renderable(r);
}

} // namespace skirmish

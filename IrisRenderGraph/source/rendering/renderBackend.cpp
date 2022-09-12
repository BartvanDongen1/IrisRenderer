#include "rendering/renderBackend.h"
#include "rendering/window.h"
#include "helper.h"

#include "rendering\renderTarget.h"
#include "rendering\descriptorHeap.h"
#include "rendering/drawable/pipelineObject.h"
#include "rendering/drawable/mesh.h"
#include "rendering/drawable/constBuffer.h"
#include "rendering/drawable/texture.h"
#include "rendering/drawable/sampler.h"
#include "rendering/drawable/drawable.h"
#include "rendering/debugModelBuilder.h"
#include "engine/camera.h"

#include "rendering/renderGraph/renderPassGraph.h"

// -------------------------------------------

#include "rendering/imgui-docking/imgui.h"
#include "imgui-docking/imgui_impl_dx12.h"
#include "imgui-docking/imgui_impl_win32.h"
#include "rendering/d3dx12.h"

// -------------------------------------------

#include <glm/glm.hpp>
#include <stdexcept>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

using namespace Microsoft::WRL;

// -------------------------------------------

// Adapter info.
bool useWarpDevice = false;

static const UINT frameCount = 2;

// Pipeline objects.
CD3DX12_VIEWPORT viewport;
CD3DX12_RECT scissorRect;

ComPtr<IDXGISwapChain3> swapChain;
ComPtr<ID3D12Device> device;

ComPtr<ID3D12Resource> renderTargets[frameCount];
ComPtr<ID3D12Resource> depthStencil;

ComPtr<ID3D12CommandAllocator> commandAllocators[frameCount];
ComPtr<ID3D12CommandQueue> commandQueue;

ComPtr<ID3D12DescriptorHeap> rtvHeap;
UINT rtvDescriptorSize{ 0 };

ComPtr<ID3D12DescriptorHeap> dsvHeap;
UINT dsvDescriptorSize{ 0 };

constexpr auto cbvSrvUavHeapMaxSize = 1024;
constexpr auto samplerHeapMaxSize = 1024;

ComPtr<ID3D12DescriptorHeap> cbvSrvUavHeap;
UINT cbvSrvUavDescriptorSize{ 0 };
int cbvSrvUavDescriptorCount{ 0 };

ComPtr<ID3D12DescriptorHeap> samplerHeap;
UINT samplerDescriptorSize{ 0 };
int samplerDescriptorCount{ 0 };

ComPtr<ID3D12GraphicsCommandList> commandList;

// Synchronization objects.
UINT frameIndex{ 0 };
HANDLE fenceEvent{ 0 };
ComPtr<ID3D12Fence> fence;
UINT64 fenceValues[frameCount]{ 0 };

// ImGui stuff
static ID3D12DescriptorHeap* pd3dSrvDescHeap = NULL;
ImGuiIO* io;

// debug drawing stuff
Microsoft::WRL::ComPtr<ID3D12RootSignature> debugRootSignature;
Microsoft::WRL::ComPtr<ID3D12PipelineState> debugPipelineState;

Microsoft::WRL::ComPtr<ID3DBlob> debugVertexShader;
Microsoft::WRL::ComPtr<ID3DBlob> debugPixelShader;

Microsoft::WRL::ComPtr<ID3D12Resource> debugVertexBuffer;
D3D12_VERTEX_BUFFER_VIEW debugVertexBufferView{ NULL };
std::vector<D3D12_INPUT_ELEMENT_DESC> debugVertexLayout;

Microsoft::WRL::ComPtr<ID3D12Resource> debugConstantBuffer;
D3D12_CONSTANT_BUFFER_VIEW_DESC debugConstantBufferDescription{ 0 };

UINT8* debugCbvDataBegin = nullptr;
void* debugConstantBufferData = nullptr;
size_t debugConstantBufferSize{ 0 };

int debugConstBufferDescriptorIndex{ 0 };

struct DebugConstBufferStruct
{
    glm::mat4 viewProjectionMatrix;

    float padding[48];
};

DebugConstBufferStruct debugBuffer;

//render graph
RenderPassGraph* graph;

// -------------------------------------------

void GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter = false);

void waitForGpu();

void initDebugVaiables();

// -------------------------------------------

bool RenderBackend::loadPipeline()
{
    frameIndex = 0;

    viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(Window::getWidth()), static_cast<float>(Window::getHeight()) };
    scissorRect = CD3DX12_RECT{ 0, 0, static_cast<LONG>(Window::getWidth()), static_cast<LONG>(Window::getHeight()) };
    rtvDescriptorSize = 0;

    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    if (useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&device)
        ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&device)
        ));
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = frameCount;
    swapChainDesc.Width = Window::getWidth();
    swapChainDesc.Height = Window::getHeight();
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> mySwapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        Window::getHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &mySwapChain
    ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Window::getHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(mySwapChain.As(&swapChain));
    frameIndex = swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = frameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Describe and create a Depth Stencil View (DSV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));

        dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        // Describe and create a cbvSrvUav descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
        cbvSrvUavHeapDesc.NumDescriptors = cbvSrvUavHeapMaxSize;
        cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(device->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(&cbvSrvUavHeap)));

        cbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // Describe and create a sampler descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
        samplerHeapDesc.NumDescriptors = samplerHeapMaxSize;
        samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&samplerHeap)));

        samplerDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT i = 0; i < frameCount; i++)
        {
            ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
            device->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, rtvDescriptorSize);

            ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocators[i])));
        }
    }

    // Create the depth stencil view.
    {
        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        const CD3DX12_HEAP_PROPERTIES depthStencilHeapProps(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_RESOURCE_DESC depthStencilTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, Window::getWidth(), Window::getHeight(), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

        ThrowIfFailed(device->CreateCommittedResource(
            &depthStencilHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilTextureDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&depthStencil)
        ));

        device->CreateDepthStencilView(depthStencil.Get(), &depthStencilDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }

    // create ImGui descriptor heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 2;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pd3dSrvDescHeap)) != S_OK)
            return false;
    }

    initDebugVaiables();

    return true;
}

void RenderBackend::loadAssets()
{
    // Create the command list.
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[frameIndex].Get(), nullptr, IID_PPV_ARGS(&commandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(commandList->Close());

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(device->CreateFence(fenceValues[frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fenceValues[frameIndex]++;

        // Create an event handle to use for frame synchronization.
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        waitForGpu();
    }
}

void RenderBackend::initImGui()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO(); (void)io;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    //io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    //io->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(Window::getHwnd());
    ImGui_ImplDX12_Init(device.Get(), frameCount,
        DXGI_FORMAT_R8G8B8A8_UNORM, pd3dSrvDescHeap,
        pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

}

void RenderBackend::shutdown()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    waitForGpu();

    // imgui cleanup
    if (pd3dSrvDescHeap) { pd3dSrvDescHeap->Release(); pd3dSrvDescHeap = NULL; }

    CloseHandle(fenceEvent);
}

void RenderBackend::beginFrame()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(commandAllocators[frameIndex]->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(commandList->Reset(commandAllocators[frameIndex].Get(), nullptr));

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // Indicate that the back buffer will be used as a render target.
    auto ResourceBarrier1 = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &ResourceBarrier1);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // set descriptor heaps
    std::vector<ID3D12DescriptorHeap*> ppHeaps;

    ppHeaps.push_back(samplerHeap.Get());
    ppHeaps.push_back(cbvSrvUavHeap.Get());

    commandList->SetDescriptorHeaps(static_cast<UINT>(ppHeaps.size()), ppHeaps.data());
}

void RenderBackend::endFrame()
{
    // Indicate that the back buffer will now be used to present.
    auto ResourceBarrier2 = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &ResourceBarrier2);

    ThrowIfFailed(commandList->Close());

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(swapChain->Present(0, 0));

    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = fenceValues[frameIndex];
    ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));

    // Update the frame index.
    frameIndex = swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (fence->GetCompletedValue() < fenceValues[frameIndex])
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent));
        WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    fenceValues[frameIndex] = currentFenceValue + 1;
}

void RenderBackend::renderDebugModel(Camera* aCamera)
{
    void* myVertices = DebugModelBuilder::getVertexData();
    int myVertexCount = DebugModelBuilder::getVertexCount();;

    // update vertex buffer
    UINT8* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    ThrowIfFailed(debugVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
    memcpy(pVertexDataBegin, myVertices, sizeof(PointVertex) * myVertexCount);
    debugVertexBuffer->Unmap(0, nullptr);

    //update const buffer
    debugBuffer.viewProjectionMatrix = aCamera->getProjectionMatrix() * aCamera->getViewMatrix();
    memcpy(debugCbvDataBegin, &debugBuffer, sizeof(DebugConstBufferStruct));

    //draw debug lines
    commandList->SetPipelineState(debugPipelineState.Get());
    commandList->SetGraphicsRootSignature(debugRootSignature.Get());

    // set const buffer
    CD3DX12_GPU_DESCRIPTOR_HANDLE descriptorHandle(cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart(), debugConstBufferDescriptorIndex, cbvSrvUavDescriptorSize);
    commandList->SetGraphicsRootDescriptorTable(0, descriptorHandle);

    // Record commands.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList->IASetVertexBuffers(0, 1, &debugVertexBufferView);
    commandList->DrawInstanced(myVertexCount, 1, 0, 0);
}

void RenderBackend::renderImGui(int aFPS)
{
    // Start the Dear ImGui frame
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool windowOpen = true;

    ImGui::Begin("settings", &windowOpen, ImGuiWindowFlags_None);
    ImGui::Text("current FPS: %i", aFPS);
    ImGui::End();

    // Rendering
    ImGui::Render();

    // Render Dear ImGui graphics
    commandList->SetDescriptorHeaps(1, &pd3dSrvDescHeap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

    // Update and Render additional Platform Windows
    if (io->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(NULL, (void*)commandList.Get());
    }
}

void RenderBackend::presentRenderTarget(RenderTarget* aRenderTarget)
{
    // transition resources to copy source and dest
    D3D12_RESOURCE_STATES rtvStateBefore = aRenderTarget->rtvState;
    aRenderTarget->transitionRtvTo(commandList.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);

    TransitionResource(commandList.Get(), renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);

    commandList.Get()->CopyResource(renderTargets[frameIndex].Get(), aRenderTarget->rtvResource.Get());

    // transition resources back
    aRenderTarget->transitionRtvTo(commandList.Get(), rtvStateBefore);
    TransitionResource(commandList.Get(), renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void RenderBackend::setGraphForResources(RenderPassGraph* aGraph)
{
    graph = aGraph;
}

RenderTarget* RenderBackend::createRenderTarget(RenderTargetDesc aDesc)
{
    RenderTarget* myRenderTarget = new RenderTarget();

    // create descriptor heaps
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&myRenderTarget->rtvHeap)));

    myRenderTarget->rtvHeapSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = 1;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&myRenderTarget->dsvHeap)));

    myRenderTarget->dsvHeapSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // set formats
    myRenderTarget->rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    myRenderTarget->dsvFormat = DXGI_FORMAT_D32_FLOAT;

    // set states 
    myRenderTarget->rtvState = D3D12_RESOURCE_STATE_RENDER_TARGET;
    myRenderTarget->dsvState = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    //set scissorRect
    myRenderTarget->scissorRect = CD3DX12_RECT{ 0, 0, static_cast<LONG>(aDesc.size[0]), static_cast<LONG>(aDesc.size[1]) };
    myRenderTarget->viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(aDesc.size[0]), static_cast<float>(aDesc.size[1]) };


    // set size
    myRenderTarget->width = aDesc.size[0];
    myRenderTarget->height = aDesc.size[1];

    // create rtv resource
    D3D12_RENDER_TARGET_VIEW_DESC renderTargetDesc = {};
    renderTargetDesc.Format = myRenderTarget->rtvFormat;
    renderTargetDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(myRenderTarget->rtvFormat,
        static_cast<UINT64>(myRenderTarget->width),
        static_cast<UINT64>(myRenderTarget->height),
        1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    float clearColor[4]{ 0,0,0,1 };

    D3D12_CLEAR_VALUE clearValue = { myRenderTarget->rtvFormat, {} };
    memcpy(clearValue.Color, clearColor, sizeof(clearValue.Color));

    ThrowIfFailed(
        device->CreateCommittedResource(
            &heapProperties, 
            D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
            &desc,
            myRenderTarget->rtvState, 
            &clearValue,
            IID_GRAPHICS_PPV_ARGS(myRenderTarget->rtvResource.ReleaseAndGetAddressOf()))
    );

    SetDebugObjectName(myRenderTarget->rtvResource.Get(), L"RenderTexture RT");

    device->CreateRenderTargetView(myRenderTarget->rtvResource.Get(), &renderTargetDesc, myRenderTarget->rtvHeap->GetCPUDescriptorHandleForHeapStart());

    // create dsv resource
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = myRenderTarget->dsvFormat;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = myRenderTarget->dsvFormat;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    const CD3DX12_HEAP_PROPERTIES depthStencilHeapProps(D3D12_HEAP_TYPE_DEFAULT);

    const CD3DX12_RESOURCE_DESC depthStencilTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(myRenderTarget->dsvFormat,
        static_cast<UINT64>(myRenderTarget->width),
        static_cast<UINT64>(myRenderTarget->height),
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    ThrowIfFailed(device->CreateCommittedResource(
        &depthStencilHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilTextureDesc,
        myRenderTarget->dsvState,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(&myRenderTarget->dsvResource)
    ));

    device->CreateDepthStencilView(myRenderTarget->dsvResource.Get(), &depthStencilDesc, myRenderTarget->dsvHeap->GetCPUDescriptorHandleForHeapStart());

    SetDebugObjectName(myRenderTarget->dsvResource.Get(), L"depthTexture");

    return myRenderTarget;
}

PipelineObject* RenderBackend::createPipelineObject(PipelineObjectDesc aDesc)
{
    PipelineObject* myPipelineObject = new PipelineObject();

    // init root signature
    {
        std::vector<CD3DX12_DESCRIPTOR_RANGE1> myRanges;
        int rangeIndex = 0;

        for (int i = 0; i < aDesc.constBuffers.size(); i++)
        {
            CD3DX12_DESCRIPTOR_RANGE1 myRange{};
            myRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, i, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
            myRanges.push_back(myRange);
        }

        for (int i = 0; i < aDesc.textures.size(); i++)
        {
            CD3DX12_DESCRIPTOR_RANGE1 myRange{};
            myRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, i, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
            myRanges.push_back(myRange);
        }

        for (int i = 0; i < aDesc.samplers.size(); i++)
        {
            CD3DX12_DESCRIPTOR_RANGE1 myRange{};
            myRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, i);
            myRanges.push_back(myRange);
        }


        std::vector<CD3DX12_ROOT_PARAMETER1> myRootParameters;

        int myCbvSrvUavCount = 0;
        int mySamplerCount = 0;
        
        for (int i = 0; i < aDesc.constBuffers.size(); i++)
        {
            CD3DX12_ROOT_PARAMETER1 myCBVRootParameter;
            myCBVRootParameter.InitAsDescriptorTable(1, &myRanges[rangeIndex++], D3D12_SHADER_VISIBILITY_ALL);
            myRootParameters.push_back(myCBVRootParameter);

            ConstBuffer* myBuffer = initConstBuffer(aDesc.constBuffers[i]);

            myCbvSrvUavCount++;

            CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), cbvSrvUavDescriptorCount, cbvSrvUavDescriptorSize);
            device.Get()->CopyDescriptorsSimple(1, cbvHandle, myBuffer->descriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            myPipelineObject->descriptors.insert({ aDesc.constBuffers[i].name, myBuffer });

            DescriptorInfo myDescriptorInfo{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cbvSrvUavDescriptorCount };
            myPipelineObject->descriptorInfo.push_back(myDescriptorInfo);

            cbvSrvUavDescriptorCount++;
        }

        for (int i = 0; i < aDesc.textures.size(); i++)
        {
            CD3DX12_ROOT_PARAMETER1 mySRVRootParameter;
            mySRVRootParameter.InitAsDescriptorTable(1, &myRanges[rangeIndex++], D3D12_SHADER_VISIBILITY_PIXEL);
            myRootParameters.push_back(mySRVRootParameter);

            Texture* myTexture;
            if (aDesc.textures[i].renderTargetName == "")
            {
                // normal texture
                myTexture = initTexture(aDesc.textures[i]);
            }
            else
            {
                // texture from render target
                if (aDesc.textures[i].useDepthBuffer)
                {
                    myTexture = graph->getResource(aDesc.textures[i].renderTargetName)->getDsvTexture();
                }
                else
                {
                    myTexture = graph->getResource(aDesc.textures[i].renderTargetName)->getRtvTexture();
                }
            }

            myCbvSrvUavCount++;

            CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), cbvSrvUavDescriptorCount, cbvSrvUavDescriptorSize);
            device.Get()->CopyDescriptorsSimple(1, srvHandle, myTexture->descriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            myPipelineObject->descriptors.insert({ aDesc.textures[i].name, myTexture });

            DescriptorInfo myDescriptorInfo{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cbvSrvUavDescriptorCount };
            myPipelineObject->descriptorInfo.push_back(myDescriptorInfo);

            cbvSrvUavDescriptorCount++;
        }

        myPipelineObject->cbvSrvUavCount = myCbvSrvUavCount;

        for (int i = 0; i < aDesc.samplers.size(); i++)
        {
            CD3DX12_ROOT_PARAMETER1 mySamplerRootParameter;
            mySamplerRootParameter.InitAsDescriptorTable(1, &myRanges[rangeIndex++], D3D12_SHADER_VISIBILITY_PIXEL);
            myRootParameters.push_back(mySamplerRootParameter);

            Sampler* mySampler = initSampler(aDesc.samplers[i]);

            mySamplerCount++;

            CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle(samplerHeap->GetCPUDescriptorHandleForHeapStart(), samplerDescriptorCount, samplerDescriptorSize);
            device.Get()->CopyDescriptorsSimple(1, samplerHandle, mySampler->descriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

            DescriptorInfo myDescriptorInfo{ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, samplerDescriptorCount };
            myPipelineObject->descriptorInfo.push_back(myDescriptorInfo);

            myPipelineObject->descriptors.insert({ aDesc.samplers[i].name, mySampler });

            samplerDescriptorCount++;
        }

        myPipelineObject->samplerCount = mySamplerCount;

        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        /* IMPLIMENT FLAGS LATER */

        // Allow input layout and deny uneccessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(static_cast<UINT>(myRootParameters.size()), myRootParameters.data(), 0, nullptr, rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&myPipelineObject->rootSignature)));
    }

    // load shaders
    {
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        ThrowIfFailed(D3DCompileFromFile(aDesc.vertexShader, nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &myPipelineObject->vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(aDesc.pixelShader, nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &myPipelineObject->pixelShader, nullptr));
    }

    int myVertexStride = 0;
    std::vector<D3D12_INPUT_ELEMENT_DESC> myVertexLayout;
    for (int i = 0; i < aDesc.vertexLayout.size(); i++)
    {
        switch (aDesc.vertexLayout[i])
        {
        case vertexType::Position3:
        {
            D3D12_INPUT_ELEMENT_DESC element1 = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, static_cast<UINT>(myVertexStride), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
            myVertexLayout.push_back(element1);
            myVertexStride += 3 * sizeof(float);
            break;
        }

        case vertexType::Normal3:
        {
            D3D12_INPUT_ELEMENT_DESC element1 = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, static_cast<UINT>(myVertexStride), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
            myVertexLayout.push_back(element1);
            myVertexStride += 3 * sizeof(float);
            break;
        }

        case vertexType::TexCoord2:
        {
            D3D12_INPUT_ELEMENT_DESC element1 = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, static_cast<UINT>(myVertexStride), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
            myVertexLayout.push_back(element1);
            myVertexStride += 2 * sizeof(float);
            break;
        }
        default:
            assert(0); // ????? 
        }
    }

    // init pipeline
    {
        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { myVertexLayout.data(), static_cast<unsigned int>(myVertexLayout.size()) };
        psoDesc.pRootSignature = myPipelineObject->rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(myPipelineObject->vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(myPipelineObject->pixelShader.Get());
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        //psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&myPipelineObject->pipelineState)));
    }

    return myPipelineObject;
}

Mesh* RenderBackend::createMesh(MeshDesc aDesc)
{
    Mesh* myMesh = new Mesh();

    int myVertexStride = 0;

    for (int i = 0; i < aDesc.vertexLayout.size(); i++)
    {
        switch (aDesc.vertexLayout[i])
        {
        case vertexType::Position3:
        {
            D3D12_INPUT_ELEMENT_DESC element1 = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, static_cast<UINT>(myVertexStride), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
            myMesh->vertexLayout.push_back(element1);
            myVertexStride += 3 * sizeof(float);
            break;
        }

        case vertexType::Normal3:
        {
            D3D12_INPUT_ELEMENT_DESC element1 = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, static_cast<UINT>(myVertexStride), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
            myMesh->vertexLayout.push_back(element1);
            myVertexStride += 3 * sizeof(float);
            break;
        }

        case vertexType::TexCoord2:
        {
            D3D12_INPUT_ELEMENT_DESC element1 = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, static_cast<UINT>(myVertexStride), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
            myMesh->vertexLayout.push_back(element1);
            myVertexStride += 2 * sizeof(float);
            break;
        }
        default:
            assert(0); // ????? 
        }
    }

    myMesh->vertexStride = myVertexStride;

    assert(aDesc.dataSize > 0);

    myMesh->dataSize = aDesc.dataSize;
    myMesh->data = malloc(aDesc.dataSize);
    memcpy(myMesh->data, aDesc.data, aDesc.dataSize);

    myMesh->vertexCount = static_cast<int>(myMesh->dataSize) / myMesh->vertexStride;

    auto myHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto myResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(myMesh->dataSize);

    ThrowIfFailed(device->CreateCommittedResource
    (
        &myHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &myResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&myMesh->vertexBuffer)
    ));

    // Copy the triangle data to the vertex buffer.
    UINT8* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    ThrowIfFailed(myMesh->vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
    memcpy(pVertexDataBegin, myMesh->data, myMesh->dataSize);
    myMesh->vertexBuffer->Unmap(0, nullptr);

    // Initialize the vertex buffer view.
    myMesh->vertexBufferView.BufferLocation = myMesh->vertexBuffer->GetGPUVirtualAddress();
    myMesh->vertexBufferView.StrideInBytes = myMesh->vertexStride;
    myMesh->vertexBufferView.SizeInBytes = static_cast<UINT>(myMesh->dataSize);

    return myMesh;
}

void RenderBackend::clearRenderTarget(RenderTarget* aRenderTarget)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle(aRenderTarget->rtvHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle(aRenderTarget->dsvHeap->GetCPUDescriptorHandleForHeapStart());

    const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(rtvDescriptorHandle, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void RenderBackend::beginScene(RenderTarget* aRenderTarget)
{
    aRenderTarget->beginScene(commandList.Get());

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(aRenderTarget->rtvHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(aRenderTarget->dsvHeap->GetCPUDescriptorHandleForHeapStart());
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    commandList->RSSetScissorRects(1, &aRenderTarget->scissorRect);
    commandList->RSSetViewports(1, &aRenderTarget->viewport);
}

void RenderBackend::endScene(RenderTarget* aRenderTarget)
{
    aRenderTarget->endScene(commandList.Get());

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvHeap->GetCPUDescriptorHandleForHeapStart());
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    commandList->RSSetScissorRects(1, &scissorRect);
    commandList->RSSetViewports(1, &viewport);
}

void RenderBackend::drawDrawable(Drawable* aDrawable)
{
    commandList->SetPipelineState(aDrawable->pipeline->pipelineState.Get());

    // Set necessary state.
    commandList->SetGraphicsRootSignature(aDrawable->pipeline->rootSignature.Get());

    int myRootParameterIndex = 0;

    for (int i = 0; i < aDrawable->pipeline->descriptorInfo.size(); i++)
    {
        if (aDrawable->pipeline->descriptorInfo[i].type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
        {
            CD3DX12_GPU_DESCRIPTOR_HANDLE descriptorHandle(cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart(), aDrawable->pipeline->descriptorInfo[i].index, cbvSrvUavDescriptorSize);
            commandList->SetGraphicsRootDescriptorTable(myRootParameterIndex++, descriptorHandle);
        }
        else
        {
            CD3DX12_GPU_DESCRIPTOR_HANDLE descriptorHandle(samplerHeap->GetGPUDescriptorHandleForHeapStart(), aDrawable->pipeline->descriptorInfo[i].index, samplerDescriptorSize);
            commandList->SetGraphicsRootDescriptorTable(myRootParameterIndex++, descriptorHandle);
        }
    }

    // Record commands.
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &aDrawable->mesh->vertexBufferView);
    commandList->DrawInstanced(aDrawable->mesh->vertexCount, 1, 0, 0);
}

ConstBuffer* RenderBackend::initConstBuffer(ConstBufferDesc aDesc)
{
    ConstBuffer* myBuffer = new ConstBuffer();

    if (aDesc.graphBufferName != "")
    {
        GraphBufferResource* myResource = graph->bufferResourceMap.find(aDesc.graphBufferName)->second;

        aDesc.size = myResource->size;
        aDesc.data = myResource->data;
    }

    myBuffer->constantBufferSize = aDesc.size;
    myBuffer->constantBufferData = aDesc.data;

    auto heapUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(aDesc.size);

    ThrowIfFailed(device->CreateCommittedResource(
        &heapUpload,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&myBuffer->constantBuffer)));

    // Map and initialize the constant buffer. We don't unmap this until the
    // app closes. Keeping things mapped for the lifetime of the resource is okay.
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    ThrowIfFailed(myBuffer->constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&myBuffer->pCbvDataBegin)));

    // Describe and create a constant buffer view.
    myBuffer->description = {};
    myBuffer->description.BufferLocation = myBuffer->constantBuffer->GetGPUVirtualAddress();
    myBuffer->description.SizeInBytes = static_cast<UINT>(aDesc.size);

    // copy initial data to the buffer if provided
    if (aDesc.data != nullptr)
    {
    }

    //create the descriptor heap for the const buffer
    D3D12_DESCRIPTOR_HEAP_DESC bufferHeapDesc = {};
    bufferHeapDesc.NumDescriptors = 1;
    bufferHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    bufferHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&bufferHeapDesc, IID_PPV_ARGS(&myBuffer->descriptorHeap)));

    myBuffer->descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(myBuffer->descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    device->CreateConstantBufferView(&myBuffer->description, cbvHandle);

    return myBuffer;
}

Texture* RenderBackend::initTexture(TextureDesc aDesc)
{
    Texture* myTexture = new Texture();

    //todo: figure out a way to upload all textures to GPU at once

    ThrowIfFailed(commandAllocators[frameIndex]->Reset());
    ThrowIfFailed(commandList->Reset(commandAllocators[frameIndex].Get(), nullptr));

    ComPtr<ID3D12Resource> textureUploadHeap;

    // Describe and create a Texture2D.
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = aDesc.width;
    textureDesc.Height = aDesc.height;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    auto textureHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(device->CreateCommittedResource(
        &textureHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&myTexture->texture)));

    if (aDesc.data != nullptr)
    {
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(myTexture->texture.Get(), 0, 1);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

        auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        // Create the GPU upload buffer.
        ThrowIfFailed(device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&textureUploadHeap)));

        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = aDesc.data;
        textureData.RowPitch = static_cast<long>(aDesc.width) * aDesc.bytesPerPixel;
        textureData.SlicePitch = textureData.RowPitch * aDesc.height;

        auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(myTexture->texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        UpdateSubresources(commandList.Get(), myTexture->texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
        commandList->ResourceBarrier(1, &resourceBarrier);
    }

    // Describe and create a SRV for the texture.
    myTexture->description = {};
    myTexture->description.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    myTexture->description.Format = textureDesc.Format;
    myTexture->description.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    myTexture->description.Texture2D.MipLevels = 1;

    ThrowIfFailed(commandList->Close());

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
    commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    waitForGpu();

    //create the descriptor heap for the texture
    D3D12_DESCRIPTOR_HEAP_DESC textureHeapDesc = {};
    textureHeapDesc.NumDescriptors = 1;
    textureHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    textureHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&textureHeapDesc, IID_PPV_ARGS(&myTexture->descriptorHeap)));

    myTexture->descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(myTexture->descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    device->CreateShaderResourceView(myTexture->texture.Get(), &myTexture->description, srvHandle);

    return myTexture;
}

Texture* RenderBackend::getTextureFromRenderTarget(RenderTarget* aRenderTarget, bool useDSV)
{
    Texture* myTexture = new Texture();

    DXGI_FORMAT myFormat;
    D3D12_RESOURCE_FLAGS myFlags;

    // share renderTarget and texture data
    if (useDSV)
    {
        myTexture->texture = aRenderTarget->dsvResource;
        myFormat = DXGI_FORMAT_R32_FLOAT;
        myFlags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    else
    {
        myTexture->texture = aRenderTarget->rtvResource;
        myFormat = aRenderTarget->rtvFormat;
        myFlags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    
    // Describe and create a Texture2D.
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = myFormat;
    textureDesc.Width = aRenderTarget->width;
    textureDesc.Height = aRenderTarget->height;
    textureDesc.Flags = myFlags;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    // Describe and create a SRV for the texture.
    myTexture->description = {};
    myTexture->description.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    myTexture->description.Format = textureDesc.Format;
    myTexture->description.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    myTexture->description.Texture2D.MipLevels = 1;

    //create the descriptor heap for the texture
    D3D12_DESCRIPTOR_HEAP_DESC textureHeapDesc = {};
    textureHeapDesc.NumDescriptors = 1;
    textureHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    textureHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&textureHeapDesc, IID_PPV_ARGS(&myTexture->descriptorHeap)));

    myTexture->descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(myTexture->descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    device->CreateShaderResourceView(myTexture->texture.Get(), &myTexture->description, srvHandle);

    return myTexture;
}

Sampler* RenderBackend::initSampler(SamplerDesc aDesc)
{
    Sampler* mySampler = new Sampler();

    // Describe and create the point clamping sampler
    mySampler->description = {};
    mySampler->description.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    mySampler->description.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    mySampler->description.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    mySampler->description.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    mySampler->description.MipLODBias = 0.0f;
    mySampler->description.MaxAnisotropy = 0;
    mySampler->description.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    mySampler->description.BorderColor[0] = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    mySampler->description.MinLOD = 0;
    mySampler->description.MaxLOD = D3D12_FLOAT32_MAX;

    //create the descriptor heap for the sampler
    D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
    samplerHeapDesc.NumDescriptors = 1;
    samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&mySampler->descriptorHeap)));

    mySampler->descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    CD3DX12_CPU_DESCRIPTOR_HANDLE samplerHandle(mySampler->descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    device->CreateSampler(&mySampler->description, samplerHandle);

    return mySampler;
}

void GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter,
    bool requestHighPerformanceAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    if (adapter.Get() == nullptr)
    {
        for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    *ppAdapter = adapter.Detach();
}

// Wait for pending GPU work to complete.
void waitForGpu()
{
    // Schedule a Signal command in the queue.
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValues[frameIndex]));

    // Wait until the fence has been processed.
    ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent));
    WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    fenceValues[frameIndex]++;
}

void initDebugVaiables()
{
    //init root signature
    {
        CD3DX12_DESCRIPTOR_RANGE1 myRange{ };
        myRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

        std::vector<CD3DX12_ROOT_PARAMETER1> myRootParameters;

        CD3DX12_ROOT_PARAMETER1 myCBVRootParameter;
        myCBVRootParameter.InitAsDescriptorTable(1, &myRange, D3D12_SHADER_VISIBILITY_VERTEX);
        myRootParameters.push_back(myCBVRootParameter);

        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        // Allow input layout and deny uneccessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(myRootParameters.size(), myRootParameters.data(), 0, nullptr, rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&debugRootSignature)));
    }
    
    // load shaders
    {
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

        ThrowIfFailed(D3DCompileFromFile(L"resources/shaders/debugLine.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &debugVertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(L"resources/shaders/debugLine.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &debugPixelShader, nullptr));
    }

    int myVertexStride = 0;
    std::vector<D3D12_INPUT_ELEMENT_DESC> myVertexLayout;

    D3D12_INPUT_ELEMENT_DESC element1 = { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(myVertexStride), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    myVertexLayout.push_back(element1);
    myVertexStride += 4 * sizeof(float);

    D3D12_INPUT_ELEMENT_DESC element2 = { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, static_cast<UINT>(myVertexStride), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
    myVertexLayout.push_back(element2);
    myVertexStride += 4 * sizeof(float);

    // init pipeline
    {
        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { myVertexLayout.data(), static_cast<unsigned int>(myVertexLayout.size()) };
        psoDesc.pRootSignature = debugRootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(debugVertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(debugPixelShader.Get());
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // Less-equal depth test w/ writes; no stencil
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&debugPipelineState)));
    }

    // init const buffer
    {
        auto heapUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(DebugConstBufferStruct));

        ThrowIfFailed(device->CreateCommittedResource(
            &heapUpload,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&debugConstantBuffer)));

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(debugConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&debugCbvDataBegin)));

        // Describe and create a constant buffer view.
        debugConstantBufferDescription = {};
        debugConstantBufferDescription.BufferLocation = debugConstantBuffer->GetGPUVirtualAddress();
        debugConstantBufferDescription.SizeInBytes = static_cast<UINT>(sizeof(DebugConstBufferStruct));

        CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), cbvSrvUavDescriptorCount, cbvSrvUavDescriptorSize);
        device->CreateConstantBufferView(&debugConstantBufferDescription, cbvHandle);

        debugConstBufferDescriptorIndex = cbvSrvUavDescriptorCount++;
    }

    // init vertex buffer
    {
        auto myHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto myResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(PointVertex) * MAX_VERTICES);

        ThrowIfFailed(device->CreateCommittedResource
        (
            &myHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &myResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&debugVertexBuffer)
        ));

        // Initialize the vertex buffer view.
        debugVertexBufferView.BufferLocation = debugVertexBuffer->GetGPUVirtualAddress();
        debugVertexBufferView.StrideInBytes = myVertexStride;
        debugVertexBufferView.SizeInBytes = static_cast<UINT>(sizeof(PointVertex) * MAX_VERTICES);
    }
}

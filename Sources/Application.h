#pragma once
#include <dxgi.h>
#include <memory>
#include <D3DX12/d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;
class Game;
struct CreateWindowParams;

class Application : public std::enable_shared_from_this<Application>
{
public:
	static std::shared_ptr<Application> Singleton;
	static std::shared_ptr<Application> GetApplication();
	~Application();
	Application();

	static UINT GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type);

	int Run(std::shared_ptr<Game> game, CreateWindowParams* Params);
	ComPtr<IDXGIAdapter> GetAdapter() { return adapter_; }
	static ComPtr<ID3D12Device> GetDevice() { return device_; }
private:
	static ComPtr<IDXGIAdapter> FindAdapter();
	static ComPtr<ID3D12Device> CreateDevice(ComPtr<IDXGIAdapter> adapter);

	static ComPtr<IDXGIAdapter> adapter_;
	static ComPtr<ID3D12Device> device_;
};

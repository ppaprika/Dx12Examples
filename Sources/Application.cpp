
#include "Application.h"
#include "Game.h"
#include "Helpers.h"


std::shared_ptr<Application> Application::Singleton;
ComPtr<ID3D12Device> Application::device_;
ComPtr<IDXGIAdapter> Application::adapter_;

Application::~Application()
{
	adapter_.Reset();
	device_.Reset();
}

int Application::Run(std::shared_ptr<Game> game, CreateWindowParams* Params)
{
	return game->Run(shared_from_this(), Params);
}

std::shared_ptr<Application> Application::GetApplication()
{
	if(!Singleton)
	{
		Singleton = std::make_shared<Application>();
	}
	return Singleton;
}

Application::Application()
{
	adapter_ = FindAdapter();
	device_ = CreateDevice(adapter_);
}

UINT Application::GetDescriptorSize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	return device_->GetDescriptorHandleIncrementSize(type);
}

ComPtr<IDXGIAdapter> Application::FindAdapter()
{
	ComPtr<IDXGIFactory> factory;
	ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&factory)));
	ComPtr<IDXGIAdapter> value;

	int i = 0;
	SIZE_T maxMemory = 0;
	ComPtr<IDXGIAdapter> adapter;
	ComPtr<IDXGIAdapter1> adapter1;
	while (factory->EnumAdapters(i, &adapter) == S_OK)
	{
		adapter.As(&adapter1);
		DXGI_ADAPTER_DESC1 adapterDesc;
		adapter1.Get()->GetDesc1(&adapterDesc);
		if (adapterDesc.DedicatedVideoMemory > maxMemory)
		{
			maxMemory = adapterDesc.DedicatedVideoMemory;
			value = adapter;
		}
		++i;
	}

	return value;
}

ComPtr<ID3D12Device> Application::CreateDevice(ComPtr<IDXGIAdapter> adapter)
{
	ComPtr<ID3D12Device> value;

	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&value)));

	return value;
}

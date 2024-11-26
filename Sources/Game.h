#pragma once

#include <chrono>
#include <dxgi.h>
#include <memory>
#include <D3DX12/d3d12.h>
#include <wrl/client.h>

#include "UploadBuffer.h"


class UploadBuffer;
class Application;
class Window;
struct CreateWindowParams;
using Microsoft::WRL::ComPtr;


class Game : public std::enable_shared_from_this<Game>
{
public:
	Game();
	virtual ~Game();

	virtual void Update();
	virtual void Render() {}

	int Run(std::shared_ptr<Application> App, CreateWindowParams* Params = nullptr);
	virtual void Init() {};

	// input
	virtual void LButtonDown() {}
	virtual void LButtonUp() {}
	virtual void RButtonDown() {}
	virtual void RButtonUp() {}
	virtual void MouseMove() {}

	static std::weak_ptr<Game> GlobalGame;
	static LRESULT StaticWinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam);
	virtual LRESULT WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam);

	void SetShowFps(bool NewShowFps) { _showFps = NewShowFps; }
	ComPtr<ID3D12Device> GetDevice();

	void UpdateBufferResource(
		ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** pDestinationResource,
		ID3D12Resource** pIntermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags, ComPtr<ID3D12Device> device);

	// todo make it private
public:
	HWND _hwnd = nullptr;
	UINT _message = 0;
	WPARAM _wParam = 0;
	LPARAM _lParam = 0;

	std::weak_ptr<Application> _app;
	std::shared_ptr<Window> _window;

	bool _showFps = true;
	std::chrono::time_point<std::chrono::steady_clock> _lastTick;

	std::shared_ptr<UploadBuffer> _uploadBuffer;

	std::shared_ptr<UploadBuffer::Memory> vertex_buffer_memory;
	std::shared_ptr<UploadBuffer::Memory> index_buffer_memory;
};

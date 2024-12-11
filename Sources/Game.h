#pragma once

#include <chrono>
#include <dxgi.h>
#include <memory>
#include <D3DX12/d3d12.h>
#include <wrl/client.h>

//#include "DirectCommandList.h"
#include "UploadBuffer.h"


class UploadBuffer;
class Application;
class Window;
struct CreateWindowParams;
class DirectCommandList;
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
	virtual void Release();

	// input
	virtual void LButtonDown() {}
	virtual void LButtonUp() {}
	virtual void RButtonDown() {}
	virtual void RButtonUp() {}
	virtual void MouseMove() {}

	static std::weak_ptr<Game> GlobalGame;
	static LRESULT StaticWinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam);
	virtual LRESULT WinProc(HWND InHwnd, UINT InMessage, WPARAM InWParam, LPARAM InLParam);

	void SetShowFps(bool NewShowFps) { show_fps_ = NewShowFps; }
	ComPtr<ID3D12Device> GetDevice();

	void Flush();

	// deprecated
	void UpdateBufferResource(
		ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** pDestinationResource,
		ID3D12Resource** pIntermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags, ComPtr<ID3D12Device> device);

	// todo make it private

	float GetDeltaTimeInSec() const { return delta_time.count() / 1000.f; }
protected:
	std::weak_ptr<Application> app_;
	bool show_fps_ = true;
	std::chrono::time_point<std::chrono::steady_clock> last_tick_;
	std::chrono::duration<double, std::milli> delta_time;
	std::shared_ptr<UploadBuffer> upload_buffer_;

	// command lists
	std::shared_ptr<DirectCommandList> direct_command_list_;
};

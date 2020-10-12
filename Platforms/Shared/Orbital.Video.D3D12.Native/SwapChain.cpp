#include "SwapChain.h"
#include "Utils.h"
#include "Texture.h"

extern "C"
{
	ORBITAL_EXPORT SwapChain* Orbital_Video_D3D12_SwapChain_Create(Device* device, SwapChainType type)
	{
		SwapChain* handle = (SwapChain*)calloc(1, sizeof(SwapChain));
		handle->device = device;
		handle->type = type;
		return handle;
	}

	ORBITAL_EXPORT int Orbital_Video_D3D12_SwapChain_Init(SwapChain* handle, HWND hWnd, UINT width, UINT height, UINT bufferCount, int fullscreen, SwapChainFormat format)
	{
		if (handle->type == SwapChainType::SwapChainType_MultiGPU_AFR)
		{
			bufferCount = handle->device->nodeCount;// if multi-gpu buffer count matches GPU count
			handle->nodeCount = handle->device->nodeCount;
		}
		else
		{
			handle->nodeCount = 1;// if type is single-gpu force to single node
		}
		handle->bufferCount = bufferCount;
		if (!GetNative_SwapChainFormat(format, &handle->format)) return false;

		// check format support
		D3D12_FEATURE_DATA_FORMAT_INFO formatInfo = {};
		formatInfo.Format = handle->format;
		if (FAILED(handle->device->device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo, sizeof(D3D12_FEATURE_DATA_FORMAT_INFO)))) return 0;

		// create swap-chain
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = bufferCount;
		swapChainDesc.Width = width;
		swapChainDesc.Height = height;
		swapChainDesc.Format = handle->format;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;// swap-chains do not support msaa
		swapChainDesc.SampleDesc.Quality = 0;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
		fullscreenDesc.Windowed = fullscreen == 0;
		fullscreenDesc.RefreshRate.Numerator = 0;
		fullscreenDesc.RefreshRate.Denominator = 0;
		fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER::DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		fullscreenDesc.Scaling = DXGI_MODE_SCALING::DXGI_MODE_SCALING_UNSPECIFIED;

		IDXGISwapChain1* swapChain = NULL;
		DeviceNode* primaryDeviceNode = &handle->device->nodes[0];// always use primary device node to create swap-chain
		if (FAILED(handle->device->instance->factory->CreateSwapChainForHwnd(primaryDeviceNode->commandQueue, hWnd, &swapChainDesc, &fullscreenDesc, NULL, &swapChain))) return 0;
		handle->swapChain = (IDXGISwapChain3*)swapChain;
		if (FAILED(handle->device->instance->factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER))) return 0;

		// create nodes
		handle->nodes = (SwapChainNode*)calloc(handle->nodeCount, sizeof(SwapChainNode));
		handle->resources = (ID3D12Resource**)calloc(bufferCount, sizeof(ID3D12Resource*));
		for (UINT n = 0; n != handle->nodeCount; ++n)
		{
			handle->nodes[n].resourceState = D3D12_RESOURCE_STATE_PRESENT;// set default state

			// create memory heaps
			if (handle->type == SwapChainType::SwapChainType_SingleGPU_Standard)// single GPU standard swap buffers
			{
				// create render targets views
				D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
				rtvHeapDesc.NodeMask = primaryDeviceNode->mask;
				rtvHeapDesc.NumDescriptors = bufferCount;
				rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				if (FAILED(handle->device->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&handle->nodes[n].resourceHeap)))) return 0;// only one resource heap for single GPU
				UINT resourceHeapSize = handle->device->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

				D3D12_CPU_DESCRIPTOR_HANDLE resourceDescCPUHandle = handle->nodes[n].resourceHeap->GetCPUDescriptorHandleForHeapStart();
				for (UINT i = 0; i != bufferCount; ++i)
				{
					if (FAILED(handle->swapChain->GetBuffer(i, IID_PPV_ARGS(&handle->resources[i])))) return 0;
					handle->device->device->CreateRenderTargetView(handle->resources[i], nullptr, resourceDescCPUHandle);
					handle->nodes[n].resourceDescCPUHandle = resourceDescCPUHandle;
					resourceDescCPUHandle.ptr += resourceHeapSize;
				}
			}
			else if (handle->type == SwapChainType::SwapChainType_MultiGPU_AFR)// multi GPU AFR buffers
			{
				// create render targets views
				D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
				rtvHeapDesc.NodeMask = handle->device->nodes[n].mask;
				rtvHeapDesc.NumDescriptors = 1;
				rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				if (FAILED(handle->device->device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&handle->nodes[n].resourceHeap)))) return 0;
				UINT resourceHeapSize = handle->device->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

				D3D12_CPU_DESCRIPTOR_HANDLE resourceDescCPUHandle = handle->nodes[n].resourceHeap->GetCPUDescriptorHandleForHeapStart();
				if (FAILED(handle->swapChain->GetBuffer(n, IID_PPV_ARGS(&handle->resources[n])))) return 0;
				handle->device->device->CreateRenderTargetView(handle->resources[n], nullptr, resourceDescCPUHandle);
				handle->nodes[n].resourceDescCPUHandle = resourceDescCPUHandle;
			}
			else
			{
				return 0;
			}

			//// create helpers for synchronous buffer operations
			//if (FAILED(handle->device->device->CreateCommandList(handle->device->nodes[0].mask, D3D12_COMMAND_LIST_TYPE_DIRECT, handle->device->nodes[0].commandAllocator, nullptr, IID_PPV_ARGS(&handle->nodes[n].internalCommandList)))) return 0;
			//if (FAILED(handle->nodes[n].internalCommandList->Close())) return 0;// make sure this is closed as it defaults to open for writing

			//if (FAILED(handle->device->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&handle->nodes[n].internalFence)))) return 0;
			//handle->nodes[n].internalFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			//if (handle->nodes[n].internalFenceEvent == NULL) return 0;

			//// make sure fence values start at 1 so they don't match 'GetCompletedValue' when its first called
			//handle->nodes[n].internalFenceValue = 1;
		}

		// create helpers for synchronous buffer operations
		if (FAILED(handle->device->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&handle->internalCommandAllocator)))) return 0;

		if (FAILED(handle->device->device->CreateCommandList(primaryDeviceNode->mask, D3D12_COMMAND_LIST_TYPE_DIRECT, handle->internalCommandAllocator, nullptr, IID_PPV_ARGS(&handle->internalCommandList)))) return 0;
		if (FAILED(handle->internalCommandList->Close())) return 0;// make sure this is closed as it defaults to open for writing

		if (FAILED(handle->device->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&handle->internalFence)))) return 0;
		handle->internalFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (handle->internalFenceEvent == NULL) return 0;

		// make sure fence values start at 1 so they don't match 'GetCompletedValue' when its first called
		handle->internalFenceValue = 1;

		return 1;
	}

	ORBITAL_EXPORT void Orbital_Video_D3D12_SwapChain_Dispose(SwapChain* handle)
	{
		if (handle->nodes != NULL)
		{
			for (UINT n = 0; n != handle->nodeCount; ++n)
			{
				for (UINT n = 0; n != handle->nodeCount; ++n)
				{
					if (handle->nodes[n].resourceHeap != NULL)
					{
						handle->nodes[n].resourceHeap->Release();
						handle->nodes[n].resourceHeap = NULL;
					}
				}

				//// dispose present helpers
				//if (handle->nodes[n].internalFenceEvent != NULL)
				//{
				//	CloseHandle(handle->nodes[n].internalFenceEvent);
				//	handle->nodes[n].internalFenceEvent = NULL;
				//}

				//if (handle->nodes[n].internalFence != NULL)
				//{
				//	handle->nodes[n].internalFence->Release();
				//	handle->nodes[n].internalFence = NULL;
				//}

				//if (handle->nodes[n].internalCommandList != NULL)
				//{
				//	handle->nodes[n].internalCommandList->Release();
				//	handle->nodes[n].internalCommandList = NULL;
				//}
			}

			free(handle->nodes);
			handle->nodes = NULL;
		}
		
		// dispose present helpers
		if (handle->internalFenceEvent != NULL)
		{
			CloseHandle(handle->internalFenceEvent);
			handle->internalFenceEvent = NULL;
		}

		if (handle->internalFence != NULL)
		{
			handle->internalFence->Release();
			handle->internalFence = NULL;
		}

		if (handle->internalCommandList != NULL)
		{
			handle->internalCommandList->Release();
			handle->internalCommandList = NULL;
		}

		if (handle->internalCommandAllocator != NULL)
		{
			handle->internalCommandAllocator->Release();
			handle->internalCommandAllocator = NULL;
		}

		if (handle->resources != NULL)
		{
			for (UINT i = 0; i != handle->bufferCount; ++i)
			{
				if (handle->resources[i] != NULL)
				{
					handle->resources[i]->Release();
					handle->resources[i] = NULL;
				}
			}
			handle->resources = NULL;
		}

		if (handle->swapChain != NULL)
		{
			handle->swapChain->Release();
			handle->swapChain = NULL;
		}

		free(handle);
	}

	ORBITAL_EXPORT void Orbital_Video_D3D12_SwapChain_BeginFrame(SwapChain* handle, int* currentNodeIndex, int* lastNodeIndex)
	{
		*lastNodeIndex = handle->currentNodeIndex;
		handle->currentRenderTargetIndex = handle->swapChain->GetCurrentBackBufferIndex();
		if (handle->nodeCount == 1) handle->currentNodeIndex = 0;
		else handle->currentNodeIndex = handle->currentRenderTargetIndex;
		*currentNodeIndex = handle->currentNodeIndex;

		// reset command list and copy resource
		DeviceNode* primaryDeviceNode = &handle->device->nodes[0];
		handle->internalCommandList->Reset(handle->internalCommandAllocator, NULL);
	}

	ORBITAL_EXPORT void Orbital_Video_D3D12_SwapChain_Present(SwapChain* handle)
	{
		//UINT currentNodeIndex = handle->currentNodeIndex;
		//UINT n = 0;//handle->currentNodeIndex;
		////if (handle->nodes[currentNodeIndex].resourceState != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT)
		//{
		//	// reset command list and copy resource
		//	handle->nodes[currentNodeIndex].internalCommandList->Reset(handle->device->nodes[currentNodeIndex].commandAllocator, NULL);

		//	Orbital_Video_D3D12_SwapChain_ChangeState(handle, currentNodeIndex, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, handle->nodes[currentNodeIndex].internalCommandList);

		//	// change resource state to present
		//	Orbital_Video_D3D12_SwapChain_ChangeState(handle, currentNodeIndex, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT, handle->nodes[currentNodeIndex].internalCommandList);

		//	// close command list
		//	handle->nodes[currentNodeIndex].internalCommandList->Close();

		//	// execute operations
		//	ID3D12CommandList* commandLists[1] = { handle->nodes[currentNodeIndex].internalCommandList };
		//	handle->device->nodes[n].commandQueue->ExecuteCommandLists(1, commandLists);
		//	WaitForFence(handle->device, currentNodeIndex, handle->nodes[currentNodeIndex].internalFence, handle->nodes[currentNodeIndex].internalFenceEvent, handle->nodes[currentNodeIndex].internalFenceValue);
		//}
		//handle->swapChain->Present(0, 0);

		// make sure swap-chain surface is in present state
		UINT currentNodeIndex = handle->currentNodeIndex;
		DeviceNode* primaryDeviceNode = &handle->device->nodes[0];
		//Orbital_Video_D3D12_SwapChain_ChangeState(handle, currentNodeIndex, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT, handle->internalCommandList);

		// close command list
		if (FAILED(handle->internalCommandList->Close()))
		{
			return;
		}

		// execute operations
		ID3D12CommandList* commandLists[1] = { handle->internalCommandList };
		//primaryDeviceNode->commandQueue->ExecuteCommandLists(1, commandLists);
		//WaitForFence_CommandQueue(primaryDeviceNode->commandQueue, handle->internalFence, handle->internalFenceEvent, handle->internalFenceValue);

		handle->swapChain->Present(0, 0);
	}

	ORBITAL_EXPORT void Orbital_Video_D3D12_SwapChain_ResolveRenderTexture(SwapChain* handle, Texture* srcRenderTexture)
	{
		UINT activeNodeIndex = handle->currentNodeIndex;
		Orbital_Video_D3D12_Texture_ChangeState(srcRenderTexture, activeNodeIndex, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_SOURCE, handle->internalCommandList);
		Orbital_Video_D3D12_SwapChain_ChangeState(handle, activeNodeIndex, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RESOLVE_DEST, handle->internalCommandList);
		handle->internalCommandList->ResolveSubresource(handle->resources[handle->currentRenderTargetIndex], 0, srcRenderTexture->nodes[activeNodeIndex].resource, 0, srcRenderTexture->format);
	}

	ORBITAL_EXPORT void Orbital_Video_D3D12_SwapChain_CopyTexture(SwapChain* handle, Texture* srcTexture)
	{
		UINT activeNodeIndex = handle->currentNodeIndex;
		Orbital_Video_D3D12_Texture_ChangeState(srcTexture, activeNodeIndex, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE, handle->internalCommandList);

		//// reset command list and copy resource
		//handle->device->nodes[activeNodeIndex].internalMutex->lock();
		//handle->device->nodes[activeNodeIndex].internalCommandList->Reset(handle->device->nodes[activeNodeIndex].internalCommandAllocator, NULL);
		//Orbital_Video_D3D12_Texture_ChangeState(srcTexture, activeNodeIndex, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE, handle->device->nodes[activeNodeIndex].internalCommandList);
		//// close command list
		//handle->device->nodes[activeNodeIndex].internalCommandList->Close();

		//// execute operations
		//ID3D12CommandList* commandLists[1] = { handle->device->nodes[activeNodeIndex].internalCommandList };
		//handle->device->nodes[activeNodeIndex].commandQueue->ExecuteCommandLists(1, commandLists);
		//WaitForFence(handle->device, activeNodeIndex, handle->device->nodes[activeNodeIndex].internalFence, handle->device->nodes[activeNodeIndex].internalFenceEvent, handle->device->nodes[activeNodeIndex].internalFenceValue);

		//// release temp resource
		//handle->device->nodes[activeNodeIndex].internalMutex->unlock();

		Orbital_Video_D3D12_SwapChain_ChangeState(handle, activeNodeIndex, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, handle->internalCommandList);
		handle->internalCommandList->CopyResource(handle->resources[handle->currentRenderTargetIndex], srcTexture->nodes[activeNodeIndex].resource);
	}

	ORBITAL_EXPORT void Orbital_Video_D3D12_SwapChain_CopyTextureRegion(SwapChain* handle, Texture* srcTexture, int srcX, int srcY, int dstX, int dstY, int width, int height, int srcMipmapLevel)
	{
		UINT activeNodeIndex = handle->currentNodeIndex;
		Orbital_Video_D3D12_Texture_ChangeState(srcTexture, activeNodeIndex, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_SOURCE, handle->internalCommandList);
		Orbital_Video_D3D12_SwapChain_ChangeState(handle, activeNodeIndex, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, handle->internalCommandList);

		D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
		dstLoc.pResource = handle->resources[handle->currentRenderTargetIndex];

		D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
		srcLoc.pResource = srcTexture->nodes[activeNodeIndex].resource;
		srcLoc.SubresourceIndex = srcMipmapLevel;

		D3D12_BOX srcBox;
		srcBox.left = srcX;
		srcBox.right = srcX + width;
		srcBox.top = srcY;
		srcBox.bottom = srcY + height;
		srcBox.front = 0;
		srcBox.back = 0;

		handle->internalCommandList->CopyTextureRegion(&dstLoc, dstX, dstY, 0, &srcLoc, &srcBox);
	}
}

void Orbital_Video_D3D12_SwapChain_ChangeState(SwapChain* handle, UINT nodeIndex, D3D12_RESOURCE_STATES state, ID3D12GraphicsCommandList* commandList)
{
	SwapChainNode* activeNode = &handle->nodes[nodeIndex];
	if (activeNode->resourceState == state) return;
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = handle->resources[handle->currentRenderTargetIndex];
	barrier.Transition.StateBefore = activeNode->resourceState;
	barrier.Transition.StateAfter = state;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);
	activeNode->resourceState = state;
}
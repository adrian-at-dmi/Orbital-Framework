#include "VertexBuffer.h"

extern "C"
{
	ORBITAL_EXPORT VertexBuffer* Orbital_Video_D3D12_VertexBuffer_Create(Device* device, VertexBufferMode mode)
	{
		VertexBuffer* handle = (VertexBuffer*)calloc(1, sizeof(VertexBuffer));
		handle->device = device;
		handle->mode = mode;
		return handle;
	}

	ORBITAL_EXPORT int Orbital_Video_D3D12_VertexBuffer_Init(VertexBuffer* handle, void* vertices, uint32_t vertexCount, uint32_t vertexSize)
	{
		uint64_t bufferSize = vertexSize * vertexCount;

		// create nodes
		handle->nodes = (VertexBufferNode*)calloc(handle->device->nodeCount, sizeof(VertexBufferNode));
		for (UINT n = 0; n != handle->device->nodeCount; ++n)
		{
			// create buffer
			D3D12_HEAP_PROPERTIES heapProperties = {};
			if (handle->mode == VertexBufferMode_GPUOptimized) heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			else if (handle->mode == VertexBufferMode_Write) heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
			else if (handle->mode == VertexBufferMode_Read) heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
			else return 0;
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProperties.CreationNodeMask = handle->device->nodes[n].mask;
			heapProperties.VisibleNodeMask = handle->device->nodes[n].mask;

			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Alignment = 0;
			resourceDesc.Width = bufferSize;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1, 
			resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			handle->nodes[n].resourceState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			if (vertices != NULL && handle->mode == VertexBufferMode_GPUOptimized) handle->nodes[n].resourceState = D3D12_RESOURCE_STATE_COPY_DEST;// init for gpu copy
			else if (handle->mode == VertexBufferMode_Read) handle->nodes[n].resourceState = D3D12_RESOURCE_STATE_COPY_DEST;// init for CPU read
			else if (handle->mode == VertexBufferMode_Write) handle->nodes[n].resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;// init for frequent cpu writes
			if (FAILED(handle->device->device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, handle->nodes[n].resourceState, nullptr, IID_PPV_ARGS(&handle->nodes[n].resource)))) return 0;

			// upload cpu buffer to gpu
			if (vertices != NULL)
			{
				// allocate gpu upload buffer if needed
				bool useUploadBuffer = false;
				ID3D12Resource* uploadResource = handle->nodes[n].resource;
				if (heapProperties.Type != D3D12_HEAP_TYPE_UPLOAD)
				{
					useUploadBuffer = true;
					uploadResource = NULL;
					heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
					if (FAILED(handle->device->device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&uploadResource)))) return 0;
				}

				// copy CPU memory to GPU
				UINT8* gpuDataPtr;
				D3D12_RANGE readRange = {};
				if (FAILED(uploadResource->Map(0, &readRange, reinterpret_cast<void**>(&gpuDataPtr))))
				{
					if (useUploadBuffer) uploadResource->Release();
					return 0;
				}
				memcpy(gpuDataPtr, vertices, bufferSize);
				uploadResource->Unmap(0, nullptr);

				// copy upload buffer to default buffer
				if (useUploadBuffer)
				{
					// reset command list and copy resource
					handle->device->nodes[n].internalMutex->lock();
					handle->device->nodes[n].internalCommandList->Reset(handle->device->nodes[n].internalCommandAllocator, NULL);
					handle->device->nodes[n].internalCommandList->CopyResource(handle->nodes[n].resource, uploadResource);

					// close command list
					handle->device->nodes[n].internalCommandList->Close();

					// execute operations
					ID3D12CommandList* commandLists[1] = { handle->device->nodes[n].internalCommandList };
					handle->device->nodes[n].commandQueue->ExecuteCommandLists(1, commandLists);
					WaitForFence(handle->device, n, handle->device->nodes[n].internalFence, handle->device->nodes[n].internalFenceEvent, handle->device->nodes[n].internalFenceValue);

					// release temp resource
					uploadResource->Release();
					handle->device->nodes[n].internalMutex->unlock();
				}
			}

			// create view
			handle->nodes[n].resourceView.BufferLocation = handle->nodes[n].resource->GetGPUVirtualAddress();
			handle->nodes[n].resourceView.StrideInBytes = vertexSize;
			handle->nodes[n].resourceView.SizeInBytes = bufferSize;
		}

		return 1;
	}

	ORBITAL_EXPORT void Orbital_Video_D3D12_VertexBuffer_Dispose(VertexBuffer* handle)
	{
		if (handle->nodes != NULL)
		{
			for (UINT n = 0; n != handle->device->nodeCount; ++n)
			{
				if (handle->nodes[n].resource != NULL)
				{
					handle->nodes[n].resource->Release();
					handle->nodes[n].resource = NULL;
				}
			}

			free(handle->nodes);
			handle->nodes = NULL;
		}

		free(handle);
	}
}

void Orbital_Video_D3D12_VertexBuffer_ChangeState(VertexBuffer* handle, UINT nodeIndex, D3D12_RESOURCE_STATES state, ID3D12GraphicsCommandList* commandList)
{
	VertexBufferNode* activeNode = &handle->nodes[nodeIndex];
	if (activeNode->resourceState == state) return;
	if (handle->mode == VertexBufferMode_Read || handle->mode == VertexBufferMode_Write) return;
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE::D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = activeNode->resource;
	barrier.Transition.StateBefore = activeNode->resourceState;
	barrier.Transition.StateAfter = state;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	commandList->ResourceBarrier(1, &barrier);
	activeNode->resourceState = state;
}
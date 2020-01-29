﻿using System;
using System.Runtime.InteropServices;

namespace Orbital.Video.D3D12
{
	public sealed class VertexBuffer : VertexBufferBase
	{
		internal IntPtr handle;

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static extern IntPtr Orbital_Video_D3D12_VertexBuffer_Create(IntPtr device, VertexBufferMode mode);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static unsafe extern int Orbital_Video_D3D12_VertexBuffer_Init(IntPtr handle, void* vertices, uint vertexCount, uint vertexSize, VertexBufferLayout_NativeInterop* layout);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static extern void Orbital_Video_D3D12_VertexBuffer_Dispose(IntPtr handle);

		public VertexBuffer(Device device, VertexBufferMode mode)
		{
			handle = Orbital_Video_D3D12_VertexBuffer_Create(device.handle, mode);
		}

		public unsafe bool Init(uint vertexCount, uint vertexSize, VertexBufferLayout layout)
		{
			
			using (var layoutNative = new VertexBufferLayout_NativeInterop(ref layout))
			{
				return Orbital_Video_D3D12_VertexBuffer_Init(handle, null, vertexCount, vertexSize, &layoutNative) != 0;
			}
		}

		#if CS_7_3
		public unsafe bool Init<T>(T[] vertices, VertexBufferLayout layout) where T : unmanaged
		{
			vertexCount = vertices.Length;
			vertexSize = Marshal.SizeOf<T>();
			using (var layoutNative = new VertexBufferLayout_NativeInterop(ref layout))
			{
				fixed (T* verticesPtr = vertices)
				{
					return Orbital_Video_D3D12_VertexBuffer_Init(handle, verticesPtr, (uint)vertices.LongLength, (uint)vertexSize, &layoutNative) != 0;
				}
			}
		}
		#else
		public unsafe bool Init<T>(T[] vertices, VertexBufferLayout layout) where T : struct
		{
			vertexCount = vertices.Length;
			vertexSize = Marshal.SizeOf<T>();
			using (var layoutNative = new VertexBufferLayout_NativeInterop(ref layout))
			{
				byte[] verticesDataCopy = new byte[vertexSize * vertices.Length];
				var gcHandle = GCHandle.Alloc(vertices, GCHandleType.Pinned);
				Marshal.Copy(gcHandle.AddrOfPinnedObject(), verticesDataCopy, 0, verticesDataCopy.Length);
				gcHandle.Free();
				fixed (byte* verticesPtr = verticesDataCopy)
				{
					return Orbital_Video_D3D12_VertexBuffer_Init(handle, verticesPtr, (uint)vertices.LongLength, (uint)vertexSize, &layoutNative) != 0;
				}
			}
		}
		#endif

		public override void Dispose()
		{
			if (handle != IntPtr.Zero)
			{
				Orbital_Video_D3D12_VertexBuffer_Dispose(handle);
				handle = IntPtr.Zero;
			}
		}
	}
}

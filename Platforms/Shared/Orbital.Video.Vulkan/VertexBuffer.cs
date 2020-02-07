﻿using System;
using System.Runtime.InteropServices;

namespace Orbital.Video.Vulkan
{
	public sealed class VertexBuffer : VertexBufferBase
	{
		public readonly Device deviceVulkan;
		internal IntPtr handle;

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static extern IntPtr Orbital_Video_Vulkan_VertexBuffer_Create(IntPtr device);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static unsafe extern int Orbital_Video_Vulkan_VertexBuffer_Init(IntPtr handle, void* vertices, uint vertexCount, uint vertexSize);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static extern void Orbital_Video_Vulkan_VertexBuffer_Dispose(IntPtr handle);

		public VertexBuffer(Device device)
		: base(device)
		{
			deviceVulkan = device;
			handle = Orbital_Video_Vulkan_VertexBuffer_Create(device.handle);
		}

		#if CS_7_3
		public unsafe bool Init<T>(T[] vertices) where T : unmanaged
		{
			vertexCount = vertices.Length;
			vertexSize = Marshal.SizeOf<T>();
			fixed (T* verticesPtr = vertices)
			{
				return Orbital_Video_Vulkan_VertexBuffer_Init(handle, verticesPtr, (uint)vertices.LongLength, (uint)vertexSize) != 0;
			}
		}
		#else
		public unsafe bool Init<T>(T[] vertices) where T : struct
		{
			vertexCount = vertices.Length;
			vertexSize = Marshal.SizeOf<T>();
			byte[] verticesDataCopy = new byte[Marshal.SizeOf<T>() * vertices.Length];
			var gcHandle = GCHandle.Alloc(vertices, GCHandleType.Pinned);
			Marshal.Copy(gcHandle.AddrOfPinnedObject(), verticesDataCopy, 0, verticesDataCopy.Length);
			gcHandle.Free();
			fixed (byte* verticesPtr = verticesDataCopy)
			{
				return Orbital_Video_Vulkan_VertexBuffer_Init(handle, verticesPtr, (uint)vertices.LongLength, (uint)vertexSize) != 0;
			}
		}
		#endif

		public override void Dispose()
		{
			if (handle != IntPtr.Zero)
			{
				Orbital_Video_Vulkan_VertexBuffer_Dispose(handle);
				handle = IntPtr.Zero;
			}
		}
	}
}

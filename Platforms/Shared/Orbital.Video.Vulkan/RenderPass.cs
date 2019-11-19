﻿using System;
using System.Runtime.InteropServices;
using Orbital.Numerics;

namespace Orbital.Video.Vulkan
{
	[StructLayout(LayoutKind.Sequential)]
	struct RenderPassDescNative
	{
		public byte clearColor, clearDepthStencil;
		public Vec4 clearColorValue;
		public float depthValue, stencilValue;
	}

	public sealed class RenderPass : RenderPassBase
	{
		internal IntPtr handle;
		private readonly SwapChain swapChain;

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static extern IntPtr Orbital_Video_Vulkan_RenderPass_Create_WithSwapChain(IntPtr device, IntPtr swapChain);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static unsafe extern int Orbital_Video_Vulkan_RenderPass_Init(IntPtr handle, RenderPassDescNative* desc);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static extern void Orbital_Video_Vulkan_RenderPass_Dispose(IntPtr handle);

		public RenderPass(SwapChain swapChain)
		{
			this.swapChain = swapChain;
			handle = Orbital_Video_Vulkan_RenderPass_Create_WithSwapChain(swapChain.deviceVulkan.handle, swapChain.handle);
			this.swapChain.renderPasses.Add(this);
		}

		public unsafe bool Init(RenderPassDesc desc)
		{
			var descNative = new RenderPassDescNative()
			{
				clearColor = (byte)(desc.clearColor ? 1 : 0),
				clearDepthStencil = (byte)(desc.clearDepthStencil ? 1 : 0),
				clearColorValue = desc.clearColorValue,
				depthValue = desc.depthValue,
				stencilValue = desc.stencilValue
			};
			return Orbital_Video_Vulkan_RenderPass_Init(handle, &descNative) != 0;
		}

		public override void Dispose()
		{
			swapChain.renderPasses.Remove(this);

			if (handle != IntPtr.Zero)
			{
				Orbital_Video_Vulkan_RenderPass_Dispose(handle);
				handle = IntPtr.Zero;
			}
		}

		internal void ResizeFrameBuffer()
		{
			// TODO: invoke native method to resize frameBuffer objects
		}
	}
}
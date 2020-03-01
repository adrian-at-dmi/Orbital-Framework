﻿using System;

namespace Orbital.Video.D3D12
{
	public sealed class RenderTexture2D : Texture2D
	{
		public readonly RenderTextureUsage usage;
		public DepthStencil depthStencil { get; private set; }

		public RenderTexture2D(Device device, RenderTextureUsage usage, TextureMode mode)
		: base(device, mode)
		{
			isRenderTexture = true;
			this.usage = usage;
		}

		public bool Init(int width, int height, TextureFormat format)
		{
			return Init(width, height, format, null, true);
		}

		public override bool Init(int width, int height, TextureFormat format, byte[] data)
		{
			return Init(width, height, format, data, true);
		}

		public bool Init(int width, int height, TextureFormat format, StencilUsage stencilUsage, DepthStencilFormat depthStencilFormat, DepthStencilMode depthStencilMode)
		{
			depthStencil = new DepthStencil(deviceD3D12, stencilUsage, depthStencilMode);
			if (!depthStencil.Init(width, height, depthStencilFormat)) return false;
			return Init(width, height, format, null, true);
		}

		public bool Init(int width, int height, TextureFormat format, byte[] data, StencilUsage stencilUsage, DepthStencilFormat depthStencilFormat, DepthStencilMode depthStencilMode)
		{
			depthStencil = new DepthStencil(deviceD3D12, stencilUsage, depthStencilMode);
			if (!depthStencil.Init(width, height, depthStencilFormat)) return false;
			return Init(width, height, format, data, true);
		}

		#region RenderTexture Methods
		public override DepthStencilBase GetDepthStencil()
		{
			return depthStencil;
		}

		public override RenderPassBase CreateRenderPass(RenderPassDesc desc)
		{
			var abstraction = new RenderPass(deviceD3D12);
			if (!abstraction.Init(desc, this))
			{
				abstraction.Dispose();
				throw new Exception("Failed to create RenderPass");
			}
			return abstraction;
		}

		public override RenderPassBase CreateRenderPass(RenderPassDesc desc, DepthStencilBase depthStencil)
		{
			var abstraction = new RenderPass(deviceD3D12);
			if (!abstraction.Init(desc, this, (DepthStencil)depthStencil))
			{
				abstraction.Dispose();
				throw new Exception("Failed to create RenderPass");
			}
			return abstraction;
		}
		#endregion
	}
}
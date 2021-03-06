﻿using Orbital.Numerics;
using System;

namespace Orbital.Video
{
	public struct RenderPassRenderTargetDesc
	{
		/// <summary>
		/// Clear render-target if true
		/// </summary>
		public bool clearColor;

		/// <summary>
		/// Color to clear render-target with
		/// </summary>
		public Color4F clearColorValue;

		public static RenderPassRenderTargetDesc CreateDefault(Color4F clearColorValue)
		{
			var result = new RenderPassRenderTargetDesc()
			{
				clearColor = true,
				clearColorValue = clearColorValue
			};
			return result;
		}
	}

	public struct RenderPassDepthStencilDesc
	{
		/// <summary>
		/// Clear depth if true
		/// </summary>
		public bool clearDepth;

		/// <summary>
		/// Clear stencil if true
		/// </summary>
		public bool clearStencil;

		/// <summary>
		/// 0-1 depth value to clear depth with.
		/// This is normally set to 1.
		/// </summary>
		public float depthValue;
		
		/// <summary>
		/// 0-1 stencil value to clear stencil with.
		/// This is normally set to 255.
		/// </summary>
		public float stencilValue;

		/// <summary>
		/// Creates default settings for standard depth testing.
		/// </summary>
		/// <param name="clearDepth">True to clear depth</param>
		public static RenderPassDepthStencilDesc CreateDefault(bool clearDepth)
		{
			var result = new RenderPassDepthStencilDesc()
			{
				clearDepth = clearDepth,
				depthValue = 1,
				stencilValue = 1
			};
			return result;
		}

		/// <summary>
		/// Creates default settings for standard depth and stencil testing.
		/// </summary>
		/// <param name="clearDepth">True to clear depth</param>
		/// <param name="clearStencil">True to clear stencil</param>
		public static RenderPassDepthStencilDesc CreateDefault(bool clearDepth, bool clearStencil)
		{
			var result = new RenderPassDepthStencilDesc()
			{
				clearDepth = clearDepth,
				clearStencil = clearStencil,
				depthValue = 1,
				stencilValue = 1
			};
			return result;
		}
	}

	public struct RenderPassDesc
	{
		public RenderPassRenderTargetDesc[] renderTargetDescs;
		public RenderPassDepthStencilDesc depthStencilDesc;

		public static RenderPassDesc CreateDefault(int renderTargetCount)
		{
			var result = new RenderPassDesc()
			{
				renderTargetDescs = new RenderPassRenderTargetDesc[renderTargetCount],
				depthStencilDesc = RenderPassDepthStencilDesc.CreateDefault(true)
			};
			for (int i = 0; i != renderTargetCount; ++i) result.renderTargetDescs[i] = RenderPassRenderTargetDesc.CreateDefault(Color4F.black);
			return result;
		}

		public static RenderPassDesc CreateDefault(Color4F clearColorValue, int renderTargetCount)
		{
			var result = new RenderPassDesc()
			{
				renderTargetDescs = new RenderPassRenderTargetDesc[renderTargetCount],
				depthStencilDesc = RenderPassDepthStencilDesc.CreateDefault(true)
			};
			for (int i = 0; i != renderTargetCount; ++i) result.renderTargetDescs[i] = RenderPassRenderTargetDesc.CreateDefault(clearColorValue);
			return result;
		}

		public static RenderPassDesc CreateDefault(Color4F clearColorValue, int renderTargetCount, bool clearDepthStencil)
		{
			var result = new RenderPassDesc()
			{
				renderTargetDescs = new RenderPassRenderTargetDesc[renderTargetCount],
				depthStencilDesc = RenderPassDepthStencilDesc.CreateDefault(clearDepthStencil)
			};
			for (int i = 0; i != renderTargetCount; ++i) result.renderTargetDescs[i] = RenderPassRenderTargetDesc.CreateDefault(clearColorValue);
			return result;
		}
	}

	public abstract class RenderPassBase : IDisposable
	{
		public readonly DeviceBase device;
		public int renderTargetCount { get; private set; }

		public RenderPassBase(DeviceBase device)
		{
			this.device = device;
		}

		protected void InitBase(ref RenderPassDesc desc, int renderTargetCount)
		{
			if (desc.renderTargetDescs == null) throw new ArgumentException("Must contain 'renderTargetDescs'");
			if (desc.renderTargetDescs.Length != renderTargetCount) throw new ArgumentException("'renderTargetDescs' length must match render targets length");
			this.renderTargetCount = renderTargetCount;
		}

		protected void ValidateMSAARenderTextures(Texture2DBase[] renderTextures)
		{
			var level = renderTextures[0].msaaLevel;
			for (int i = 1; i < renderTextures.Length; ++i)
			{
				if (renderTextures[i].msaaLevel != level) throw new ArgumentException("All render-texture MSAA-Levels must match");
			}
		}

		public abstract void Dispose();
	}
}

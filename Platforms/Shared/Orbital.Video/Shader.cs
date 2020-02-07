﻿using System;

namespace Orbital.Video
{
	public enum ShaderType
	{
		/// <summary>
		/// Vertex Shader
		/// </summary>
		VS,

		/// <summary>
		/// Pixel Shader
		/// </summary>
		PS,

		/// <summary>
		/// Hull Shader
		/// </summary>
		HS,

		/// <summary>
		/// Domain Shader
		/// </summary>
		DS,

		/// <summary>
		/// Geometry Shader
		/// </summary>
		GS
	}

	public abstract class ShaderBase : IDisposable
	{
		public readonly DeviceBase device;

		public ShaderBase(DeviceBase device)
		{
			this.device = device;
		}

		public abstract void Dispose();

		/// <summary>
		/// Gets shader type
		/// </summary>
		public abstract ShaderType GetShaderType();
	}
}

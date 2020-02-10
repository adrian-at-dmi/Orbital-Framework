﻿using System;
using System.Runtime.InteropServices;
using Orbital.Numerics;

namespace Orbital.Video.D3D12
{
	public sealed class ConstantBuffer : ConstantBufferBase
	{
		public readonly Device deviceD3D12;
		internal IntPtr handle;

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static extern IntPtr Orbital_Video_D3D12_ConstantBuffer_Create(IntPtr device, ConstantBufferMode mode);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static unsafe extern int Orbital_Video_D3D12_ConstantBuffer_Init(IntPtr handle, uint size, int* alignedSize, void* initialData);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static extern void Orbital_Video_D3D12_ConstantBuffer_Dispose(IntPtr handle);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static unsafe extern int Orbital_Video_D3D12_ConstantBuffer_BeginUpdate(IntPtr handle);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static unsafe extern void Orbital_Video_D3D12_ConstantBuffer_EndUpdate(IntPtr handle);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static unsafe extern void Orbital_Video_D3D12_ConstantBuffer_Update(IntPtr handle, void* data, uint dataSize, uint dstOffset);

		[DllImport(Instance.lib, CallingConvention = Instance.callingConvention)]
		private static unsafe extern void Orbital_Video_D3D12_ConstantBuffer_UpdateArray(IntPtr handle, void* data, uint dataElementSize, uint dataElementCount, uint offset, uint srcStride, uint dstStride);

		public ConstantBuffer(Device device, ConstantBufferMode mode)
		: base(device)
		{
			deviceD3D12 = device;
			handle = Orbital_Video_D3D12_ConstantBuffer_Create(device.handle, mode);
		}

		public unsafe bool Init(int size)
		{
			int alignedSize;
			int result = Orbital_Video_D3D12_ConstantBuffer_Init(handle, (uint)size, &alignedSize, null);
			this.size = alignedSize;
			return result != 0;
		}

		public unsafe bool Init<T>() where T : struct
		{
			int alignedSize;
			int result = Orbital_Video_D3D12_ConstantBuffer_Init(handle, (uint)Marshal.SizeOf<T>(), &alignedSize, null);
			size = alignedSize;
			return result != 0;
		}

		#if CS_7_3
		public unsafe bool Init<T>(T initialData) where T : unmanaged
		{
			int alignedSize;
			int result = Orbital_Video_D3D12_ConstantBuffer_Init(handle, (uint)Marshal.SizeOf<T>(), &alignedSize, &initialData);
			size = alignedSize;
			return result != 0;
		}
		#else
		public unsafe bool Init<T>(T initialData) where T : struct
		{
			int alignedSize;
			TypedReference reference = __makeref(initialData);
			byte* ptr = (byte*)*((IntPtr*)&reference);
			#if MONO
			ptr += Marshal.SizeOf(typeof(RuntimeTypeHandle));
			#endif
			int result = Orbital_Video_D3D12_ConstantBuffer_Init(handle, (uint)Marshal.SizeOf<T>(), &alignedSize, ptr);
			size = alignedSize;
			return result != 0;
		}
		#endif

		public override void Dispose()
		{
			if (handle != IntPtr.Zero)
			{
				Orbital_Video_D3D12_ConstantBuffer_Dispose(handle);
				handle = IntPtr.Zero;
			}
		}

		public override bool BeginUpdate()
		{
			return Orbital_Video_D3D12_ConstantBuffer_BeginUpdate(handle) != 0;
		}

		public override void EndUpdate()
		{
			Orbital_Video_D3D12_ConstantBuffer_EndUpdate(handle);
		}

		#if CS_7_3
		public unsafe override void Update<T>(T data)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &data, (uint)Marshal.SizeOf<T>(), 0);
		}

		public unsafe override void Update<T>(T data, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &data, (uint)Marshal.SizeOf<T>(), (uint)offset);
		}

		public unsafe override void Update<T>(T[] data, int offset)
		{
			fixed (T* dataPtr = data)
			{
				Orbital_Video_D3D12_ConstantBuffer_Update(handle, dataPtr, (uint)(Marshal.SizeOf<T>() * data.Length), (uint)offset);
			}
		}

		public unsafe override void Update<T>(T data, ShaderEffectVariableMapping variable)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &data, (uint)Marshal.SizeOf<T>(), (uint)variable.offset);
		}

		public unsafe override void Update<T>(T[] data, ShaderEffectVariableMapping variable)
		{
			uint srcStride = (uint)ShaderEffectBase.VariableTypeToSrcStride(variable.type);
			uint dstStride = (uint)ShaderEffectBase.VariableTypeToDstStride(variable.type);
			fixed (T* dataPtr = data)
			{
				Orbital_Video_D3D12_ConstantBuffer_UpdateArray(handle, dataPtr, (uint)Marshal.SizeOf<T>(), (uint)data.Length, (uint)variable.offset, srcStride, dstStride);
			}
		}
		#else
		public unsafe override void Update<T>(T data)
		{
			TypedReference reference = __makeref(data);
			byte* ptr = (byte*)*((IntPtr*)&reference);
			#if MONO
			ptr += Marshal.SizeOf(typeof(RuntimeTypeHandle));
			#endif
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, ptr, (uint)Marshal.SizeOf<T>(), 0);
		}

		public unsafe override void Update<T>(T data, int offset)
		{
			TypedReference reference = __makeref(data);
			byte* ptr = (byte*)*((IntPtr*)&reference);
			#if MONO
			ptr += Marshal.SizeOf(typeof(RuntimeTypeHandle));
			#endif
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, ptr, (uint)Marshal.SizeOf<T>(), (uint)offset);
		}

		public unsafe override void Update<T>(T[] data, int offset)
		{
			GCHandle gcHandle = GCHandle.Alloc(data, GCHandleType.Pinned);
			IntPtr dataPtr = gcHandle.AddrOfPinnedObject();
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, (void*)dataPtr, (uint)(Marshal.SizeOf<T>() * data.Length), (uint)offset);
			gcHandle.Free();
		}

		public unsafe override void Update<T>(T data, ShaderEffectVariableMapping variable)
		{
			TypedReference reference = __makeref(data);
			byte* ptr = (byte*)*((IntPtr*)&reference);
			#if MONO
			ptr += Marshal.SizeOf(typeof(RuntimeTypeHandle));
			#endif
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, ptr, (uint)Marshal.SizeOf<T>(), (uint)variable.offset);
		}

		public unsafe override void Update<T>(T[] data, ShaderEffectVariableMapping variable)
		{
			uint srcStride = (uint)ShaderEffectBase.VariableTypeToSrcStride(variable.type);
			uint dstStride = (uint)ShaderEffectBase.VariableTypeToDstStride(variable.type);
			GCHandle gcHandle = GCHandle.Alloc(data, GCHandleType.Pinned);
			IntPtr dataPtr = gcHandle.AddrOfPinnedObject();
			Orbital_Video_D3D12_ConstantBuffer_UpdateArray(handle, (void*)dataPtr, (uint)Marshal.SizeOf<T>(), (uint)data.Length, (uint)variable.offset, srcStride, dstStride);
			gcHandle.Free();
		}
		#endif

		public override unsafe void Update(void* data, int dataSize, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, data, (uint)dataSize, (uint)offset);
		}

		public override unsafe void Update(float value, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &value, sizeof(float), (uint)offset);
		}

		public override unsafe void Update(Vec2 vector, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &vector, (uint)Marshal.SizeOf<Vec2>(), (uint)offset);
		}

		public override unsafe void Update(Vec3 vector, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &vector, (uint)Marshal.SizeOf<Vec3>(), (uint)offset);
		}

		public override unsafe void Update(Vec4 vector, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &vector, (uint)Marshal.SizeOf<Vec4>(), (uint)offset);
		}

		public override unsafe void Update(Mat2 matrix, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &matrix, (uint)Marshal.SizeOf<Mat2>(), (uint)offset);
		}

		public override unsafe void Update(Mat2x3 matrix, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &matrix, (uint)Marshal.SizeOf<Mat2x3>(), (uint)offset);
		}

		public override unsafe void Update(Mat3 matrix, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &matrix, (uint)Marshal.SizeOf<Mat3>(), (uint)offset);
		}

		public override unsafe void Update(Mat3x2 matrix, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &matrix, (uint)Marshal.SizeOf<Mat3x2>(), (uint)offset);
		}

		public override unsafe void Update(Mat4 matrix, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &matrix, (uint)Marshal.SizeOf<Mat4>(), (uint)offset);
		}

		public override unsafe void Update(Quat quaternion, int offset)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &quaternion, (uint)Marshal.SizeOf<Quat>(), (uint)offset);
		}

		public override unsafe void Update(Color4 color, int offset)
		{
			var colorVec = color.ToVec4();
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &colorVec, (uint)Marshal.SizeOf<Vec4>(), (uint)offset);
		}

		public override unsafe void Update(float value, ShaderEffectVariableMapping variable)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &value, sizeof(float), (uint)variable.offset);
		}

		public override unsafe void Update(Vec2 vector, ShaderEffectVariableMapping variable)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &vector, (uint)Marshal.SizeOf<Vec2>(), (uint)variable.offset);
		}

		public override unsafe void Update(Vec3 vector, ShaderEffectVariableMapping variable)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &vector, (uint)Marshal.SizeOf<Vec3>(), (uint)variable.offset);
		}

		public override unsafe void Update(Vec4 vector, ShaderEffectVariableMapping variable)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &vector, (uint)Marshal.SizeOf<Vec4>(), (uint)variable.offset);
		}

		public override unsafe void Update(Mat2 matrix, ShaderEffectVariableMapping variable)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &matrix, (uint)Marshal.SizeOf<Mat2>(), (uint)variable.offset);
		}

		public override unsafe void Update(Mat2x3 matrix, ShaderEffectVariableMapping variable)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &matrix, (uint)Marshal.SizeOf<Mat2x3>(), (uint)variable.offset);
		}

		public override unsafe void Update(Mat3 matrix, ShaderEffectVariableMapping variable)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &matrix, (uint)Marshal.SizeOf<Mat3>(), (uint)variable.offset);
		}

		public override unsafe void Update(Mat3x2 matrix, ShaderEffectVariableMapping variable)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &matrix, (uint)Marshal.SizeOf<Mat3x2>(), (uint)variable.offset);
		}

		public override unsafe void Update(Mat4 matrix, ShaderEffectVariableMapping variable)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &matrix, (uint)Marshal.SizeOf<Mat4>(), (uint)variable.offset);
		}

		public override unsafe void Update(Quat quaternion, ShaderEffectVariableMapping variable)
		{
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &quaternion, (uint)Marshal.SizeOf<Quat>(), (uint)variable.offset);
		}

		public override unsafe void Update(Color4 color, ShaderEffectVariableMapping variable)
		{
			var colorVec = color.ToVec4();
			Orbital_Video_D3D12_ConstantBuffer_Update(handle, &colorVec, (uint)Marshal.SizeOf<Vec4>(), (uint)variable.offset);
		}
	}
}

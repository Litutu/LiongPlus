// File: Bitmap.cpp
// Author: Rendong Liang (Liong)

#include "Bitmap.hpp"

namespace LiongPlus
{
	namespace Media
	{
		using std::swap;
		// Public

		Bitmap::Bitmap(const Image& image)
			: _Buffer()
			, _PixelType(image.GetPixelType())
			, _Size(image.GetSize())
		{
		}
		Bitmap::Bitmap(const Bitmap& instance)
			: _Buffer()
			, _PixelType(instance._PixelType)
			, _Size(instance._Size)
		{
		}
		Bitmap::Bitmap(Bitmap&& instance)
			: _Buffer()
			, _PixelType(instance._PixelType)
			, _Size(instance._Size)
		{
			swap(_Buffer, instance._Buffer);
			swap(_PixelType, instance._PixelType);
			swap(_Size, instance._Size);
		}
		Bitmap::Bitmap(Buffer&& buffer, Size size, PixelType pixelType)
			: _Buffer(std::forward<Buffer>(buffer))
			, _PixelType(pixelType)
			, _Size(size)
		{
		}
		Bitmap::~Bitmap()
		{
		}

		Bitmap& Bitmap::operator=(const Bitmap& instance)
		{
			_Size = instance.GetSize();
			_PixelType = instance.GetPixelType();
			_Buffer = Buffer(instance._Buffer.Length());
			memcpy(_Buffer.Field(), instance._Buffer.Field(), instance._Buffer.Length());
			return *this;
		}
		Bitmap& Bitmap::operator=(Bitmap&& instance)
		{
			swap(_Size, instance._Size);
			swap(_PixelType, instance._PixelType);
			swap(_Buffer, instance._Buffer);
			return *this;
		}

		// Static

		Image* Bitmap::FromMemory(MemoryStream& stream, Size size, PixelType pixelType)
		{

			size_t length = CalculateDataLength(size, pixelType);
			Buffer buffer = stream.Read(length);
			return new Bitmap(std::move(buffer), size, pixelType);
		}

		// Derived from [LiongFramework::Media::Image]

		Buffer Bitmap::GetChunk(Point position, Size size) const
		{
			if (position.X < 0 || position.X + size.Width > _Size.Width ||
				position.Y < 0 || position.Y + size.Height > _Size.Height)
				return nullptr;

			Buffer buffer(CalculateDataLength(size, _PixelType));

			size_t pixelLength = CalculatePixelLength(_PixelType);
			size_t lineData = size.Width * pixelLength;
			size_t lineOffset = (_Size.Width - size.Width) * pixelLength;
			Byte* pos = const_cast<Byte*>(_Buffer.Field()) + // Origin
				(position.X + position.Y * _Size.Width) * pixelLength; // Offset

			while (size.Height-- > 0)
			{
				memcpy(buffer.Field(), pos, lineData); // Copy linear pixels in the same row.
				pos += lineOffset;
			}

			return buffer;
		}

		size_t Bitmap::GetInterpretedLength(PixelType pixelType) const
		{
			return CalculateDataLength(_Size, pixelType);
		}

		Buffer Bitmap::GetPixel(Point position) const
		{
			size_t pixelLength = CalculatePixelLength(_PixelType);
			Buffer pixel = Buffer(pixelLength);
			memcpy(pixel.Field(), _Buffer.Field() + position.Y * _Size.Width + position.X, pixelLength);

			return pixel;
		}

		PixelType Bitmap::GetPixelType() const
		{
			return _PixelType;
		}

		Size Bitmap::GetSize() const
		{
			return _Size;
		}

		bool Bitmap::IsEmpty() const
		{
			return _Size.Width == 0 || _Size.Height == 0;
		}

		Buffer Bitmap::Interpret(PixelType pixelType) const
		{
			if (pixelType == _PixelType)
			{
				Byte* buffer = new Byte[_Buffer.Length()];
				memcpy(buffer, _Buffer.Field(), _Buffer.Length());
				return buffer;
			}

			switch (CalculatePixelLength(_PixelType))
			{
			case 1:
				return InterpretMonoTo(pixelType);
			case 3:
				return InterpretTriTo(pixelType);
			case 4:
				return InterpretQuadTo(pixelType);
			}
			return nullptr;
		}

		// Private

		Buffer Bitmap::InterpretMonoTo(PixelType pixelType) const
		{
			if (pixelType < 4) return nullptr; // Mono
			else if (pixelType == PixelType::Rgba) // Quad
				return InterpretMonoToQuad((long)pixelType);
			else // Tri
				return InterpretMonoToTri(pixelType == PixelType::Rgb
					? (long)pixelType - 1
					: 3 - (long)pixelType);
		}

		Buffer Bitmap::InterpretTriTo(PixelType pixelType) const
		{
			if (pixelType == PixelType::Rgba) // Quad
				return InterpretTriToQuad((pixelType != 4));
			else if ((long)pixelType < 4 && pixelType != PixelType::Alpha) // Mono
				return InterpretTriToMono(_PixelType == PixelType::Rgb);
			else // Tri
				return InterpretTriToTri();
		}

		Buffer Bitmap::InterpretQuadTo(PixelType pixelType) const
		{
			if (pixelType < 4) // Mono
				return InterpretQuadToMono((long)pixelType);
			else // Tri
				return InterpretQuadToTri((pixelType != 4));
		}

		Buffer Bitmap::InterpretMonoToTri(size_t factorOffset) const
		{
			if (factorOffset < 0)
				return nullptr;
			Buffer buffer(_Size.Width * _Size.Height * 3);

			const Byte* source = _Buffer.Field();
			for (size_t i = 0; i < _Buffer.Length(); ++i)
				buffer[i * 3 + factorOffset] = source[i];
			return buffer;
		}

		Buffer Bitmap::InterpretMonoToQuad(size_t factorOffset) const
		{
			Buffer buffer(_Size.Width * _Size.Height * 4);
			const Byte* source = _Buffer.Field();
			for (size_t i = 0; i < _Buffer.Length(); ++i)
			{
				buffer[i * 4 + factorOffset] = source[i];
				buffer[i * 4 + 3] = (Byte)0xFF;
			}
			return buffer;
		}

		Buffer Bitmap::InterpretTriToMono(size_t factorOffset) const
		{
			long pixelCount = _Size.Width * _Size.Height;
			Buffer buffer(pixelCount);
			const Byte* source = _Buffer.Field();
			for (int i = 0; i < pixelCount; ++i)
				buffer[i] = source[i * 3 + factorOffset];
			return buffer;
		}

		Buffer Bitmap::InterpretTriToTri() const
		{
			long pixelCount = _Size.Width * _Size.Height * 3;
			Buffer buffer(pixelCount);
			const Byte* source = _Buffer.Field();
			for (int i = 0; i < pixelCount; i += 3)
			{
				buffer[i] = source[i + 2];
				buffer[i + 1] = source[i + 1];
				buffer[i + 2] = source[i];
			}
			return buffer;
		}

		Buffer Bitmap::InterpretTriToQuad(bool shouldInverse) const
		{
			long pixelCount = _Size.Width * _Size.Height;
			Buffer buffer(pixelCount * 4);
			const Byte* source = _Buffer.Field();
			if (shouldInverse)
			{
				for (long i = 0; i < pixelCount; ++i)
				{
					buffer[i * 4] = source[i * 3];
					buffer[i * 4 + 1] = source[i * 3 + 1];
					buffer[i * 4 + 2] = source[i * 3 + 2];
					buffer[i * 4 + 3] = (Byte)0xFF;
				}
			}
			else
			{
				for (long i = 0; i < pixelCount; ++i)
				{
					buffer[i * 4] = source[i * 3 + 2];
					buffer[i * 4 + 1] = source[i * 3 + 1];
					buffer[i * 4 + 2] = source[i * 3];
					buffer[i * 4 + 3] = (Byte)0xFF;
				}
			}
			return buffer;
		}

		Buffer Bitmap::InterpretQuadToMono(size_t factorOffset) const
		{
			long pixelCount = _Size.Width * _Size.Height;
			Buffer buffer(pixelCount);
			const Byte* source = _Buffer.Field();
			for (int i = 0; i < pixelCount; ++i)
				buffer[i] = source[i * 4 + factorOffset];
			return buffer;
		}

		Buffer Bitmap::InterpretQuadToTri(bool shouldInverse) const
		{
			long pixelCount = _Size.Width * _Size.Height;
			Buffer buffer(pixelCount * 3);
			const Byte* source = _Buffer.Field();
			if (shouldInverse)
			{
				for (long i = 0; i < pixelCount; ++i)
				{
					buffer[i * 3] = source[i * 4];
					buffer[i * 3 + 1] = source[i * 4 + 1];
					buffer[i * 3 + 2] = source[i * 4 + 2];
				}
			}
			else
			{
				for (long i = 0; i < pixelCount; ++i)
				{
					buffer[i * 3] = source[i * 4 + 2];
					buffer[i * 3 + 1] = source[i * 4 + 1];
					buffer[i * 3 + 2] = source[i * 4];
				}
			}
			return buffer;
		}

		// Static

		size_t Bitmap::CalculatePixelLength(PixelType pixelType)
		{
			switch (pixelType)
			{
			case PixelType::Rgba:
				return 4;
			case PixelType::Bgr:
			case PixelType::Rgb:
				return 3;
			case PixelType::Alpha:
			case PixelType::Red:
			case PixelType::Green:
			case PixelType::Blue:
				return 1;
			default:
				return 0;
			}
		}

		size_t Bitmap::CalculateDataLength(Size size, PixelType pixelType)
		{
			return size.Width * size.Height * CalculatePixelLength(pixelType);
		}
	}
}
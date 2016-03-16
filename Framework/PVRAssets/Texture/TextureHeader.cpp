/*!*********************************************************************************************************************
\file         PVRAssets\Texture\TextureHeader.cpp
\author       PowerVR by Imagination, Developer Technology Team
\copyright    Copyright (c) Imagination Technologies Limited.
\brief         Implementation of methods of the TextureHeader class.
***********************************************************************************************************************/
//!\cond NO_DOXYGEN
#include <cstring>

#include "PVRCore/Maths.h"
#include "PVRAssets/Texture/TextureHeader.h"
#include "PVRAssets/Texture/TextureFormats.h"
#include "PVRAssets/FileIO/FileDefinesDDS.h"
#include "PVRAssets/Texture/PixelFormat.h"
#include "PVRCore/Log.h"
#include <algorithm>
using std::string;
using std::map;
namespace pvr {
using namespace types;
namespace assets {

TextureHeader::TextureHeader()
{
	m_header.flags            = 0;
	m_header.pixelFormat      = CompressedPixelFormat::NumCompressedPFs;
    m_header.colorSpace       = ColorSpace::lRGB;
	m_header.channelType      = VariableType::UnsignedByteNorm;
	m_header.height           = 1;
	m_header.width            = 1;
	m_header.depth            = 1;
	m_header.numberOfSurfaces = 1;
	m_header.numberOfFaces    = 1;
	m_header.mipMapCount      = 1;
	m_header.metaDataSize     = 0;
}

TextureHeader::TextureHeader(const TextureHeader& rhs)
{
	//Copy the header over.
	m_header = rhs.m_header;
	m_metaDataMap = rhs.m_metaDataMap;
}

TextureHeader::TextureHeader(TextureHeader::Header& header) : m_header(header){}

TextureHeader::TextureHeader(Header fileHeader, uint32 metaDataCount, TextureMetaData* metaData)
	: m_header(fileHeader)
{
	if (metaData)
	{
		for (uint32 i = 0; i < metaDataCount; ++i)
		{
			addMetaData(metaData[i]);
		}
	}
}

TextureHeader::TextureHeader(PixelFormat pixelFormat, uint32 width, uint32 height, uint32 depth, uint32 mipMapCount,
	types::ColorSpace::Enum colorSpace, VariableType::Enum channelType,uint32 numberOfSurfaces, uint32 numberOfFaces,
	uint32 flags, TextureMetaData* metaData, uint32 metaDataSize)
{
	m_header.pixelFormat = pixelFormat;
	m_header.width = width, m_header.height = height, m_header.depth = depth;
	m_header.mipMapCount = mipMapCount;
	m_header.colorSpace = colorSpace;
	m_header.channelType = channelType;
	m_header.numberOfSurfaces = numberOfSurfaces;
	m_header.numberOfFaces = numberOfFaces;
	m_header.flags = flags;
	if (metaData)
	{
		for (uint32 i = 0; i < metaDataSize; ++i)
		{
			addMetaData(metaData[i]);
		}
	}
}



TextureHeader& TextureHeader::operator=(const TextureHeader& rhs)
{
	//If it equals itself, return early.
	if (&rhs == this)
	{
		return *this;
	}

	//Copy the header over.
	m_header = rhs.m_header;
	m_metaDataMap = rhs.m_metaDataMap;

	//Return
	return *this;
}

TextureHeader::Header TextureHeader::getFileHeader() const
{
	return m_header;
}

TextureHeader::Header& TextureHeader::getFileHeaderAccess()
{
	return m_header;
}


const string TextureHeader::getCubeMapOrder() const
{
	//Make sure the meta block exists
	//uint32 fourCCIndex = m_metaDataMap.find(Header::PVRv3);
	map<uint32, map<uint32, TextureMetaData> >::const_iterator foundFourCC = m_metaDataMap.find(Header::PVRv3);
	if (getNumberOfFaces() > 1)
	{
		if (foundFourCC != m_metaDataMap.end())
		{

			map<uint32, TextureMetaData>::const_iterator foundMetaData = (foundFourCC->second).find(
			      TextureMetaData::IdentifierCubeMapOrder);
			if (foundMetaData != (foundFourCC->second).end())
			{
				char8 cubeMapOrder[7];
				cubeMapOrder[6] = 0;
				memcpy(cubeMapOrder, (foundMetaData->second).getData(), 6);
				return string(cubeMapOrder);
			}
		}

		string defaultOrder("XxYyZz");

		// Remove characters for faces that don't exist
		defaultOrder.resize(defaultOrder.size() - 6 - getNumberOfFaces());

		return defaultOrder;
	}

	return string("");
}


PixelFormat TextureHeader::getPixelFormat() const
{
	PixelFormat pt(m_header.pixelFormat);
	return pt;
}

uint32 TextureHeader::getBitsPerPixel() const
{
	if (getPixelFormat().getPart().High != 0)
	{
		return getPixelFormat().getPixelTypeChar()[4] + getPixelFormat().getPixelTypeChar()[5] + getPixelFormat().getPixelTypeChar()[6] +
		       getPixelFormat().getPixelTypeChar()[7];
	}
	else
	{
		switch (getPixelFormat().getPixelTypeId())
		{
		case CompressedPixelFormat::BW1bpp:
			return 1;
		case CompressedPixelFormat::PVRTCI_2bpp_RGB:
		case CompressedPixelFormat::PVRTCI_2bpp_RGBA:
		case CompressedPixelFormat::PVRTCII_2bpp:
			return 2;
		case CompressedPixelFormat::PVRTCI_4bpp_RGB:
		case CompressedPixelFormat::PVRTCI_4bpp_RGBA:
		case CompressedPixelFormat::PVRTCII_4bpp:
		case CompressedPixelFormat::ETC1:
		case CompressedPixelFormat::EAC_R11:
		case CompressedPixelFormat::ETC2_RGB:
		case CompressedPixelFormat::ETC2_RGB_A1:
		case CompressedPixelFormat::DXT1:
		case CompressedPixelFormat::BC4:
			return 4;
		case CompressedPixelFormat::DXT2:
		case CompressedPixelFormat::DXT3:
		case CompressedPixelFormat::DXT4:
		case CompressedPixelFormat::DXT5:
		case CompressedPixelFormat::BC5:
		case CompressedPixelFormat::EAC_RG11:
		case CompressedPixelFormat::ETC2_RGBA:
			return 8;
		case CompressedPixelFormat::YUY2:
		case CompressedPixelFormat::UYVY:
		case CompressedPixelFormat::RGBG8888:
		case CompressedPixelFormat::GRGB8888:
			return 16;
		case CompressedPixelFormat::SharedExponentR9G9B9E5:
			return 32;
		case CompressedPixelFormat::NumCompressedPFs:
			return 0;
		}
	}
	return 0;
}

void TextureHeader::getMinDimensionsForFormat(uint32& minX, uint32& minY, uint32& minZ) const
{
	if (getPixelFormat().getPart().High != 0)
	{
		// Non-compressed formats all return 1.
		minX = minY = minZ = 1;
	}
	else
	{
		// Default
		minX = minY = minZ = 1;

		switch (getPixelFormat().getPixelTypeId())
		{
		case CompressedPixelFormat::DXT1:
		case CompressedPixelFormat::DXT2:
		case CompressedPixelFormat::DXT3:
		case CompressedPixelFormat::DXT4:
		case CompressedPixelFormat::DXT5:
		case CompressedPixelFormat::BC4:
		case CompressedPixelFormat::BC5:
		case CompressedPixelFormat::ETC1:
		case CompressedPixelFormat::ETC2_RGB:
		case CompressedPixelFormat::ETC2_RGBA:
		case CompressedPixelFormat::ETC2_RGB_A1:
		case CompressedPixelFormat::EAC_R11:
		case CompressedPixelFormat::EAC_RG11:
			minX = 4;
			minY = 4;
			minZ = 1;
			break;
		case CompressedPixelFormat::PVRTCI_4bpp_RGB:
		case CompressedPixelFormat::PVRTCI_4bpp_RGBA:
			minX = 8;
			minY = 8;
			minZ = 1;
			break;
		case CompressedPixelFormat::PVRTCI_2bpp_RGB:
		case CompressedPixelFormat::PVRTCI_2bpp_RGBA:
			minX = 16;
			minY = 8;
			minZ = 1;
			break;
		case CompressedPixelFormat::PVRTCII_4bpp:
			minX = 4;
			minY = 4;
			minZ = 1;
			break;
		case CompressedPixelFormat::PVRTCII_2bpp:
			minX = 8;
			minY = 4;
			minZ = 1;
			break;
		case CompressedPixelFormat::UYVY:
		case CompressedPixelFormat::YUY2:
		case CompressedPixelFormat::RGBG8888:
		case CompressedPixelFormat::GRGB8888:
			minX = 2;
			minY = 1;
			minZ = 1;
			break;
		case CompressedPixelFormat::BW1bpp:
			minX = 8;
			minY = 1;
			minZ = 1;
			break;
		//Error
		case CompressedPixelFormat::NumCompressedPFs:
			break;
		}
	}
}

ColorSpace::Enum TextureHeader::getColorSpace() const
{
	return static_cast<ColorSpace::Enum>(m_header.colorSpace);
}

VariableType::Enum TextureHeader::getChannelType() const
{
	return static_cast<VariableType::Enum>(m_header.channelType);
}


void TextureHeader::setBumpMap(float bumpScale, string bumpOrder)
{
    if(bumpOrder.find_first_not_of("xyzh") != std::string::npos)
    {
        assertion(false ,  "Invalid Bumpmap order string");
        pvr::Log("Invalid Bumpmap order string");
        return;
    }
    //Get a reference to the meta data block.
	TextureMetaData& bumpMetaData = m_metaDataMap[Header::PVRv3][TextureMetaData::IdentifierBumpData];

	//Check if it's already been set or not.
	if (bumpMetaData.getData())
	{
		m_header.metaDataSize -= bumpMetaData.getTotalSizeInMemory();
	}

	// Initialize and clear the bump map data
	byte bumpData[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	//Copy the floating point scale and character order into the bumpmap data
	memcpy(bumpData, &bumpScale, 4);
	memcpy(bumpData + 4, bumpOrder.c_str(), (std::min)(bumpOrder.length(), size_t(4)));

	bumpMetaData = TextureMetaData(Header::PVRv3, TextureMetaData::IdentifierBumpData, 8, bumpData);

	//Increment the meta data size.
	m_header.metaDataSize += bumpMetaData.getTotalSizeInMemory();
}

const TextureMetaData::AxisOrientation TextureHeader::getOrientation(TextureMetaData::Axis axis) const
{
	//Make sure the meta block exists

	std::map<uint32, std::map<uint32, TextureMetaData> >::const_iterator foundIdentifer = m_metaDataMap.find(Header::PVRv3);
	if (foundIdentifer != m_metaDataMap.end())
	{
		std::map<uint32, TextureMetaData>::const_iterator foundTexMetaData = foundIdentifer->second.find(
		      TextureMetaData::IdentifierTextureOrientation);
		if (foundTexMetaData != foundIdentifer->second.end())
		{
			return (TextureMetaData::AxisOrientation)
			       (foundTexMetaData->second.getData()[axis]);
		}
	}


	return (TextureMetaData::AxisOrientation)0; //Default is the flag values.
}

bool TextureHeader::getDirectXGIFormat(uint32& dxgiFormat, bool& notAlpha) const
{
	// Default value in case of errors
	dxgiFormat = texture_dds::DXGI_FORMAT_UNKNOWN;
	notAlpha = false;
	if (getPixelFormat().getPart().High == 0)
	{
		if (getPixelFormat().getPixelTypeId() == CompressedPixelFormat::RGBG8888)
		{
			dxgiFormat = texture_dds::DXGI_FORMAT_R8G8_B8G8_UNORM;
			return true;
		}
		if (getPixelFormat().getPixelTypeId() == CompressedPixelFormat::GRGB8888)
		{
			dxgiFormat = texture_dds::DXGI_FORMAT_G8R8_G8B8_UNORM;
			return true;
		}
		if (getPixelFormat().getPixelTypeId() == CompressedPixelFormat::BW1bpp)
		{
			dxgiFormat = texture_dds::DXGI_FORMAT_R1_UNORM;
			return true;
		}
		if (getChannelType() == VariableType::UnsignedIntegerNorm || getChannelType() == VariableType::UnsignedShortNorm ||
		    getChannelType() == VariableType::UnsignedByteNorm)
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case CompressedPixelFormat::BC1:
				if (getColorSpace() == ColorSpace::sRGB)
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_BC1_UNORM_SRGB;
					return true;
				}
				else
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_BC1_UNORM;
					return true;
				}
			case CompressedPixelFormat::BC2:
				if (getColorSpace() == ColorSpace::sRGB)
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_BC2_UNORM_SRGB;
					return true;
				}
				else
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_BC2_UNORM;
					return true;
				}
			case CompressedPixelFormat::BC3:
				if (getColorSpace() == ColorSpace::sRGB)
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_BC3_UNORM_SRGB;
					return true;
				}
				else
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_BC3_UNORM;
					return true;
				}
			case CompressedPixelFormat::BC4:
				dxgiFormat = texture_dds::DXGI_FORMAT_BC4_UNORM;
				return true;
			case CompressedPixelFormat::BC5:
				dxgiFormat = texture_dds::DXGI_FORMAT_BC5_UNORM;
				return true;
			}
		}
		else if (getChannelType() == VariableType::SignedIntegerNorm || getChannelType() == VariableType::SignedShortNorm ||
		         getChannelType() == VariableType::SignedByteNorm)
		{
			if (getPixelFormat().getPixelTypeId() == CompressedPixelFormat::BC4)
			{
				dxgiFormat = texture_dds::DXGI_FORMAT_BC4_SNORM;
				return true;
			}
			if (getPixelFormat().getPixelTypeId() == CompressedPixelFormat::BC5)
			{
				dxgiFormat = texture_dds::DXGI_FORMAT_BC5_SNORM;
				return true;
			}
		}
	}
	else
	{
		switch (getChannelType())
		{
		case VariableType::SignedFloat:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType4<'r', 'g', 'b', 'x', 32, 32, 32, 32>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 32, 32, 32, 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32G32B32A32_FLOAT;
				return true;
			case GeneratePixelType3<'r', 'g', 'b', 32, 32, 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32G32B32_FLOAT;
				return true;
			case GeneratePixelType2<'r', 'g', 32, 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32G32_FLOAT;
				return true;
			case GeneratePixelType1<'r', 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32_FLOAT;
				return true;
			case GeneratePixelType4<'r', 'g', 'b', 'x', 16, 16, 16, 16>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16G16B16A16_FLOAT;
				return true;
			case GeneratePixelType2<'r', 'g', 16, 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16G16_FLOAT;
				return true;
			case GeneratePixelType1<'r', 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16_FLOAT;
				return true;
			case GeneratePixelType3<'r', 'g', 'b', 11, 11, 10>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R11G11B10_FLOAT;
				return true;

			}
			break;
		}
		case VariableType::UnsignedByte:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType4<'r', 'g', 'b', 'x', 8, 8, 8, 8>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R8G8B8A8_UINT;
				return true;
			case GeneratePixelType2<'r', 'g', 8, 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R8G8_UINT;
				return true;
			case GeneratePixelType1<'r', 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R8_UINT;
				return true;
			}
			break;
		}
		case VariableType::UnsignedByteNorm:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType4<'r', 'g', 'b', 'x', 8, 8, 8, 8>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID:
			{
				if (getColorSpace() == ColorSpace::sRGB)
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
					return true;
				}
				else
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_R8G8B8A8_UNORM;
					return true;
				}
			}
			case GeneratePixelType4<'b', 'g', 'r', 'a', 8, 8, 8, 8>::ID:
			{
				if (getColorSpace() == ColorSpace::sRGB)
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
					return true;
				}
				else
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_B8G8R8A8_UNORM;
					return true;
				}
			}
			case GeneratePixelType4<'b', 'g', 'r', 'x', 8, 8, 8, 8>::ID:
			{
				notAlpha = true;
				if (getColorSpace() == ColorSpace::sRGB)
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
					return true;
				}
				else
				{
					dxgiFormat = texture_dds::DXGI_FORMAT_B8G8R8X8_UNORM;
					return true;
				}
			}
			case GeneratePixelType2<'r', 'g', 8, 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R8G8_UNORM;
				return true;
			case GeneratePixelType1<'r', 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R8_UNORM;
				return true;
			case GeneratePixelType1<'x', 8>::ID:
				notAlpha = true;
			case GeneratePixelType1<'a', 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_A8_UNORM;
				return true;
			}
			break;
		}
		case VariableType::SignedByte:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType4<'r', 'g', 'b', 'x', 8, 8, 8, 8>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R8G8B8A8_SINT;
				return true;
			case GeneratePixelType2<'r', 'g', 8, 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R8G8_SINT;
				return true;
			case GeneratePixelType1<'r', 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R8_SINT;
				return true;
			}
			break;
		}
		case VariableType::SignedByteNorm:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType4<'r', 'g', 'b', 'x', 8, 8, 8, 8>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R8G8B8A8_SNORM;
				return true;
			case GeneratePixelType2<'r', 'g', 8, 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R8G8_SNORM;
				return true;
			case GeneratePixelType1<'r', 8>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R8_SNORM;
				return true;
			}
			break;
		}
		case VariableType::UnsignedShort:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType4<'r', 'g', 'b', 'x', 16, 16, 16, 16>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16G16B16A16_UINT;
				return true;
			case GeneratePixelType2<'r', 'g', 16, 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16G16_UINT;
				return true;
			case GeneratePixelType1<'r', 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16_UINT;
				return true;
			}
			break;
		}
		case VariableType::UnsignedShortNorm:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType4<'r', 'g', 'b', 'x', 16, 16, 16, 16>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16G16B16A16_UNORM;
				return true;
			case GeneratePixelType2<'r', 'g', 16, 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16G16_UNORM;
				return true;
			case GeneratePixelType1<'r', 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16_UNORM;
				return true;
			case GeneratePixelType3<'r', 'g', 'b', 5, 6, 5>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_B5G6R5_UNORM;
				return true;
			case GeneratePixelType4<'x', 'r', 'g', 'b', 5, 5, 5, 1>::ID:
				notAlpha = true;
			case GeneratePixelType4<'a', 'r', 'g', 'b', 5, 5, 5, 1>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_B5G5R5A1_UNORM;
				return true;
			case GeneratePixelType4<'x', 'r', 'g', 'b', 4, 4, 4, 4>::ID:
				notAlpha = true;
			case GeneratePixelType4<'a', 'r', 'g', 'b', 4, 4, 4, 4>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_B4G4R4A4_UNORM;
				return true;
			}
			break;
		}
		case VariableType::SignedShort:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType4<'r', 'g', 'b', 'x', 16, 16, 16, 16>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16G16B16A16_SINT;
				return true;
			case GeneratePixelType2<'r', 'g', 16, 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16G16_SINT;
				return true;
			case GeneratePixelType1<'r', 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16_SINT;
				return true;
			}
			break;
		}
		case VariableType::SignedShortNorm:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType4<'r', 'g', 'b', 'x', 16, 16, 16, 16>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16G16B16A16_SNORM;
				return true;
			case GeneratePixelType2<'r', 'g', 16, 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16G16_SNORM;
				return true;
			case GeneratePixelType1<'r', 16>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R16_SNORM;
				return true;
			}
			break;
		}
		case VariableType::UnsignedInteger:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType4<'r', 'g', 'b', 'x', 32, 32, 32, 32>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 32, 32, 32, 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32G32B32A32_UINT;
				return true;
			case GeneratePixelType3<'r', 'g', 'b', 32, 32, 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32G32B32_UINT;
				return true;
			case GeneratePixelType2<'r', 'g', 32, 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32G32_UINT;
				return true;
			case GeneratePixelType1<'r', 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32_UINT;
				return true;
			case GeneratePixelType4<'r', 'g', 'b', 'x', 10, 10, 10, 2>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 10, 10, 10, 2>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R10G10B10A2_UINT;
				return true;

			}
			break;
		}
		case VariableType::UnsignedIntegerNorm:
		{
			if (getPixelFormat().getPixelTypeId() == GeneratePixelType4<'r', 'g', 'b', 'a', 10, 10, 10, 2>::ID)
			{
				dxgiFormat = texture_dds::DXGI_FORMAT_R10G10B10A2_UNORM;
				return true;
			}
			if (getPixelFormat().getPixelTypeId() == GeneratePixelType4<'r', 'g', 'b', 'x', 10, 10, 10, 2>::ID)
			{
				notAlpha = true;
				dxgiFormat = texture_dds::DXGI_FORMAT_R10G10B10A2_UNORM;
				return true;
			}
			break;
		}
		case VariableType::SignedInteger:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType4<'r', 'g', 'b', 'x', 32, 32, 32, 32>::ID:
				notAlpha = true;
			case GeneratePixelType4<'r', 'g', 'b', 'a', 32, 32, 32, 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32G32B32A32_SINT;
				return true;
			case GeneratePixelType3<'r', 'g', 'b', 32, 32, 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32G32B32_SINT;
				return true;
			case GeneratePixelType2<'r', 'g', 32, 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32G32_SINT;
				return true;
			case GeneratePixelType1<'r', 32>::ID:
				dxgiFormat = texture_dds::DXGI_FORMAT_R32_SINT;
				return true;
			}
			break;
		}
		default:
		{
		}
		}
	}

	// Default return value if no errors.
	return false;
}

bool TextureHeader::getDirect3DFormat(uint32& d3dFormat) const
{
	//Default return value if unrecognised
	d3dFormat = texture_dds::D3DFMT_UNKNOWN;

	if (getPixelFormat().getPart().High == 0)
	{
		switch (getPixelFormat().getPixelTypeId())
		{
		case CompressedPixelFormat::DXT1:
			d3dFormat = texture_dds::D3DFMT_DXT1;
			return true;
		case CompressedPixelFormat::DXT2:
			d3dFormat = texture_dds::D3DFMT_DXT2;
			return true;
		case CompressedPixelFormat::DXT3:
			d3dFormat = texture_dds::D3DFMT_DXT3;
			return true;
		case CompressedPixelFormat::DXT4:
			d3dFormat = texture_dds::D3DFMT_DXT4;
			return true;
		case CompressedPixelFormat::DXT5:
			d3dFormat = texture_dds::D3DFMT_DXT5;
			return true;
		case CompressedPixelFormat::PVRTCI_2bpp_RGB:
		case CompressedPixelFormat::PVRTCI_2bpp_RGBA:
			d3dFormat = texture_dds::D3DFMT_PVRTC2;
			return true;
		case CompressedPixelFormat::PVRTCI_4bpp_RGB:
		case CompressedPixelFormat::PVRTCI_4bpp_RGBA:
			d3dFormat = texture_dds::D3DFMT_PVRTC4;
			return true;
		case CompressedPixelFormat::YUY2:
			d3dFormat = texture_dds::D3DFMT_YUY2;
			return true;
		case CompressedPixelFormat::UYVY:
			d3dFormat = texture_dds::D3DFMT_UYVY;
			return true;
		case CompressedPixelFormat::RGBG8888:
			d3dFormat = texture_dds::D3DFMT_R8G8_B8G8;
			return true;
		case CompressedPixelFormat::GRGB8888:
			d3dFormat = texture_dds::D3DFMT_G8R8_G8B8;
			return true;
		}
	}
	else
	{
		switch (getChannelType())
		{
		case VariableType::SignedFloat:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType1<'r', 16>::ID:
				d3dFormat = texture_dds::D3DFMT_R16F;
				return true;
			case GeneratePixelType2<'g', 'r', 16, 16>::ID:
				d3dFormat = texture_dds::D3DFMT_G16R16F;
				return true;
			case GeneratePixelType4<'a', 'b', 'g', 'r', 16, 16, 16, 16>::ID:
				d3dFormat = texture_dds::D3DFMT_A16B16G16R16F;
				return true;
			case GeneratePixelType1<'r', 32>::ID:
				d3dFormat = texture_dds::D3DFMT_R32F;
				return true;
			case GeneratePixelType2<'r', 'g', 32, 32>::ID:
				d3dFormat = texture_dds::D3DFMT_G32R32F;
				return true;
			case GeneratePixelType4<'a', 'b', 'g', 'r', 32, 32, 32, 32>::ID:
				d3dFormat = texture_dds::D3DFMT_A32B32G32R32F;
				return true;
			}
			break;
		}
		case VariableType::UnsignedIntegerNorm:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType3<'r', 'g', 'b', 8, 8, 8>::ID:
				d3dFormat = texture_dds::D3DFMT_R8G8B8;
				return true;
			case GeneratePixelType4<'a', 'r', 'g', 'b', 8, 8, 8, 8>::ID:
				d3dFormat = texture_dds::D3DFMT_A8R8G8B8;
				return true;
			case GeneratePixelType4<'x', 'r', 'g', 'b', 8, 8, 8, 8>::ID:
				d3dFormat = texture_dds::D3DFMT_X8R8G8B8;
				return true;
			case GeneratePixelType2<'a', 'l', 8, 8>::ID:
				d3dFormat = texture_dds::D3DFMT_A8L8;
				return true;
			case GeneratePixelType1<'a', 8>::ID:
				d3dFormat = texture_dds::D3DFMT_A8;
				return true;
			case GeneratePixelType1<'l', 8>::ID:
				d3dFormat = texture_dds::D3DFMT_L8;
				return true;
			case GeneratePixelType2<'a', 'l', 4, 4>::ID:
				d3dFormat = texture_dds::D3DFMT_A4L4;
				return true;
			case GeneratePixelType3<'r', 'g', 'b', 3, 3, 2>::ID:
				d3dFormat = texture_dds::D3DFMT_R3G3B2;
				return true;
			case GeneratePixelType1<'l', 16>::ID:
				d3dFormat = texture_dds::D3DFMT_L16;
				return true;
			case GeneratePixelType2<'g', 'r', 16, 16>::ID:
				d3dFormat = texture_dds::D3DFMT_G16R16;
				return true;
			case GeneratePixelType4<'a', 'b', 'g', 'r', 16, 16, 16, 16>::ID:
				d3dFormat = texture_dds::D3DFMT_A16B16G16R16;
				return true;
			case GeneratePixelType4<'a', 'r', 'g', 'b', 4, 4, 4, 4>::ID:
				d3dFormat = texture_dds::D3DFMT_A4R4G4B4;
				return true;
			case GeneratePixelType4<'a', 'r', 'g', 'b', 1, 5, 5, 5>::ID:
				d3dFormat = texture_dds::D3DFMT_A1R5G5B5;
				return true;
			case GeneratePixelType4<'x', 'r', 'g', 'b', 1, 5, 5, 5>::ID:
				d3dFormat = texture_dds::D3DFMT_X1R5G5B5;
				return true;
			case GeneratePixelType3<'r', 'g', 'b', 5, 6, 5>::ID:
				d3dFormat = texture_dds::D3DFMT_R5G6B5;
				return true;
			case GeneratePixelType4<'a', 'r', 'g', 'b', 8, 3, 3, 2>::ID:
				d3dFormat = texture_dds::D3DFMT_A8R3G3B2;
				return true;
			case GeneratePixelType4<'a', 'b', 'g', 'r', 2, 10, 10, 10>::ID:
				d3dFormat = texture_dds::D3DFMT_A2B10G10R10;
				return true;
			case GeneratePixelType4<'a', 'r', 'g', 'b', 2, 10, 10, 10>::ID:
				d3dFormat = texture_dds::D3DFMT_A2R10G10B10;
				return true;
			}
			break;
		}
		case VariableType::UnsignedByteNorm:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType3<'r', 'g', 'b', 8, 8, 8>::ID:
				d3dFormat = texture_dds::D3DFMT_R8G8B8;
				return true;
			case GeneratePixelType4<'a', 'r', 'g', 'b', 8, 8, 8, 8>::ID:
				d3dFormat = texture_dds::D3DFMT_A8R8G8B8;
				return true;
			case GeneratePixelType4<'x', 'r', 'g', 'b', 8, 8, 8, 8>::ID:
				d3dFormat = texture_dds::D3DFMT_X8R8G8B8;
				return true;
			case GeneratePixelType2<'a', 'l', 8, 8>::ID:
				d3dFormat = texture_dds::D3DFMT_A8L8;
				return true;
			case GeneratePixelType1<'a', 8>::ID:
				d3dFormat = texture_dds::D3DFMT_A8;
				return true;
			case GeneratePixelType1<'l', 8>::ID:
				d3dFormat = texture_dds::D3DFMT_L8;
				return true;
			case GeneratePixelType2<'a', 'l', 4, 4>::ID:
				d3dFormat = texture_dds::D3DFMT_A4L4;
				return true;
			case GeneratePixelType3<'r', 'g', 'b', 3, 3, 2>::ID:
				d3dFormat = texture_dds::D3DFMT_R3G3B2;
				return true;
			}
			break;
		}
		case VariableType::UnsignedShortNorm:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType1<'l', 16>::ID:
				d3dFormat = texture_dds::D3DFMT_L16;
				return true;
			case GeneratePixelType2<'g', 'r', 16, 16>::ID:
				d3dFormat = texture_dds::D3DFMT_G16R16;
				return true;
			case GeneratePixelType4<'a', 'b', 'g', 'r', 16, 16, 16, 16>::ID:
				d3dFormat = texture_dds::D3DFMT_A16B16G16R16;
				return true;
			case GeneratePixelType4<'a', 'r', 'g', 'b', 4, 4, 4, 4>::ID:
				d3dFormat = texture_dds::D3DFMT_A4R4G4B4;
				return true;
			case GeneratePixelType4<'a', 'r', 'g', 'b', 1, 5, 5, 5>::ID:
				d3dFormat = texture_dds::D3DFMT_A1R5G5B5;
				return true;
			case GeneratePixelType4<'x', 'r', 'g', 'b', 1, 5, 5, 5>::ID:
				d3dFormat = texture_dds::D3DFMT_X1R5G5B5;
				return true;
			case GeneratePixelType3<'r', 'g', 'b', 5, 6, 5>::ID:
				d3dFormat = texture_dds::D3DFMT_R5G6B5;
				return true;
			case GeneratePixelType4<'a', 'r', 'g', 'b', 8, 3, 3, 2>::ID:
				d3dFormat = texture_dds::D3DFMT_A8R3G3B2;
				return true;
			}
			break;
		}
		case VariableType::SignedIntegerNorm:
		{
			switch (getPixelFormat().getPixelTypeId())
			{
			case GeneratePixelType2<'g', 'r', 8, 8>::ID:
				d3dFormat = texture_dds::D3DFMT_V8U8;
				return true;
			case GeneratePixelType4<'x', 'l', 'g', 'r', 8, 8, 8, 8>::ID:
				d3dFormat = texture_dds::D3DFMT_X8L8V8U8;
				return true;
			case GeneratePixelType4<'a', 'b', 'g', 'r', 8, 8, 8, 8>::ID:
				d3dFormat = texture_dds::D3DFMT_Q8W8V8U8;
				return true;
			case GeneratePixelType3<'l', 'g', 'r', 6, 5, 5>::ID:
				d3dFormat = texture_dds::D3DFMT_L6V5U5;
				return true;
			case GeneratePixelType2<'g', 'r', 16, 16>::ID:
				d3dFormat = texture_dds::D3DFMT_V16U16;
				return true;
			case GeneratePixelType4<'a', 'b', 'g', 'r', 2, 10, 10, 10>::ID:
				d3dFormat = texture_dds::D3DFMT_A2W10V10U10;
				return true;
			}
			break;
		}
		default:
			break;
		}
	}

	//Return false if format recognised
	return false;
}

uint32 TextureHeader::getWidth(uint32 uiMipLevel) const
{
	//If MipLevel does not exist, return no uiDataSize.
	if (uiMipLevel > m_header.mipMapCount)
	{
		return 0;
	}
	return std::max<uint32>(m_header.width >> uiMipLevel, 1);
}

uint32 TextureHeader::getHeight(uint32 uiMipLevel) const
{
	//If MipLevel does not exist, return no uiDataSize.
	if (uiMipLevel > m_header.mipMapCount)
	{
		return 0;
	}
	return std::max<uint32>(m_header.height >> uiMipLevel, 1);
}

uint32 TextureHeader::getDepth(uint32 uiMipLevel) const
{
	//If MipLevel does not exist, return no uiDataSize.
	if (uiMipLevel > m_header.mipMapCount)
	{
		return 0;
	}
	return std::max<uint32>(m_header.depth >> uiMipLevel, 1);
}

uint32 TextureHeader::getTextureSize(int32 iMipLevel, bool bAllSurfaces, bool bAllFaces) const
{
	return (uint32)(((uint64)8 * (uint64)getDataSize(iMipLevel, bAllSurfaces,
	                      bAllFaces)) / (uint64)getBitsPerPixel());
}

uint32 TextureHeader::getDataSize(int32 iMipLevel, bool bAllSurfaces, bool bAllFaces) const
{
	//The smallest divisible sizes for a pixel format
	uint32 uiSmallestWidth = 1;
	uint32 uiSmallestHeight = 1;
	uint32 uiSmallestDepth = 1;

	//Get the pixel format's minimum dimensions.
	getMinDimensionsForFormat(uiSmallestWidth, uiSmallestHeight, uiSmallestDepth);

	//Needs to be 64-bit integer to support 16kx16k and higher sizes.
	uint64 uiDataSize = 0;
	if (iMipLevel == -1)
	{
		for (uint32 uiCurrentMIP = 0; uiCurrentMIP < getNumberOfMIPLevels(); ++uiCurrentMIP)
		{
			//Get the dimensions of the current MIP Map level.
			uint32 uiWidth = getWidth(uiCurrentMIP);
			uint32 uiHeight = getHeight(uiCurrentMIP);
			uint32 uiDepth = getDepth(uiCurrentMIP);
			uint32  bpp = getBitsPerPixel();


			//If pixel format is compressed, the dimensions need to be padded.
			if (getPixelFormat().getPart().High == 0)
			{
				uiWidth = uiWidth + ((-1 * uiWidth) % uiSmallestWidth);
				uiHeight = uiHeight + ((-1 * uiHeight) % uiSmallestHeight);
				uiDepth = uiDepth + ((-1 * uiDepth) % uiSmallestDepth);
			}

			//Add the current MIP Map's data size to the total.

			uiDataSize += bpp * (uint64)uiWidth * (uint64)uiHeight * (uint64)uiDepth;
		}
	}
	else
	{
		//Get the dimensions of the specified MIP Map level.
		uint32 uiWidth = getWidth(iMipLevel);
		uint32 uiHeight = getHeight(iMipLevel);
		uint32 uiDepth = getDepth(iMipLevel);

		//If pixel format is compressed, the dimensions need to be padded.
		if (getPixelFormat().getPart().High == 0)
		{
			uiWidth = uiWidth + ((-1 * uiWidth) % uiSmallestWidth);
			uiHeight = uiHeight + ((-1 * uiHeight) % uiSmallestHeight);
			uiDepth = uiDepth + ((-1 * uiDepth) % uiSmallestDepth);
		}

		//Work out the specified MIP Map's data size
		uiDataSize = (uint64)getBitsPerPixel() * (uint64)uiWidth * (uint64)uiHeight * (uint64)uiDepth;
	}

	//The number of faces/surfaces to register the size of.
	uint32 numfaces = (bAllFaces ? getNumberOfFaces() : 1);
	uint32 numsurfs = (bAllSurfaces ? getNumberOfArrayMembers() : 1);

	//Multiply the data size by number of faces and surfaces specified, and return.
	return (uint32)(uiDataSize / 8) * numsurfs * numfaces;
}

ptrdiff_t TextureHeader::getDataOffset(uint32 mipMapLevel/*= 0*/, uint32 arrayMember/*= 0*/,
                                       uint32 face/*= 0*/) const
{
	//Initialize the offSet value.
	uint32 uiOffSet = 0;

	//Error checking
	if ((int32)mipMapLevel == c_pvrTextureAllMIPMaps)
	{
		return 0;
	}

	if (mipMapLevel >= getNumberOfMIPLevels() || arrayMember >= getNumberOfArrayMembers() || face >= getNumberOfFaces())
	{
		return 0;
	}

	//File is organised by MIP Map levels, then surfaces, then faces.

	//Get the start of the MIP level.
	if (mipMapLevel != 0)
	{
		//Get the size for all MIP Map levels up to this one.
		for (uint32 uiCurrentMIPMap = 0; uiCurrentMIPMap < mipMapLevel; ++uiCurrentMIPMap)
		{
			uiOffSet += getDataSize(uiCurrentMIPMap, true, true);
		}
	}

	//Get the start of the array.
	if (arrayMember != 0)
	{
		uiOffSet += arrayMember * getDataSize(mipMapLevel, false, true);
	}

	//Get the start of the face.
	if (face != 0)
	{
		uiOffSet += face * getDataSize(mipMapLevel, false, false);
	}

	//Return the data pointer plus whatever offSet has been specified.
	return uiOffSet;
}

uint32 TextureHeader::getNumberOfArrayMembers() const
{
	return m_header.numberOfSurfaces;
}

uint32 TextureHeader::getNumberOfMIPLevels() const
{
	return m_header.mipMapCount;
}

uint32 TextureHeader::getNumberOfFaces() const
{
	return m_header.numberOfFaces;
}

const map<uint32, map<uint32, TextureMetaData> >* const TextureHeader::getMetaDataMap() const
{
	return &m_metaDataMap;
}


bool TextureHeader::isFileCompressed() const
{
	return (m_header.flags & Header::CompressedFlag) != 0;
}

bool TextureHeader::isPreMultiplied() const
{
	return (m_header.flags & Header::PremultipliedFlag) != 0;
}

uint32 TextureHeader::getMetaDataSize() const
{
	return m_header.metaDataSize;
}

void TextureHeader::setPixelFormat(PixelFormat pixelFormat)
{
	m_header.pixelFormat = pixelFormat.getPixelTypeId();
}

void TextureHeader::setColorSpace(ColorSpace::Enum colorSpace)
{
	m_header.colorSpace = colorSpace;
}

void TextureHeader::setChannelType(VariableType::Enum varType)
{
	m_header.channelType = varType;
}

bool TextureHeader::setopenGLFormat(uint32 glInternalFormat, uint32, uint32 glType)
{
	/*	Try to determine the format. This code is naive, and only checks the data that matters (e.g. glInternalFormat first, then glType
		if it needs more information).
	*/
	switch (glInternalFormat)
	{
	//Unsized internal formats
	case GL_RED:
	{
		setColorSpace(ColorSpace::lRGB);
		switch (glType)
		{
		case GL_UNSIGNED_BYTE:
		{
			setChannelType(VariableType::UnsignedByteNorm);
			setPixelFormat(GeneratePixelType1<'r', 8>::ID);
			return true;
		}
		case GL_BYTE:
		{
			setChannelType(VariableType::SignedByteNorm);
			setPixelFormat(GeneratePixelType1<'r', 8>::ID);
			return true;
		}
		case GL_UNSIGNED_SHORT:
		{
			setChannelType(VariableType::UnsignedShortNorm);
			setPixelFormat(GeneratePixelType1<'r', 16>::ID);
			return true;
		}
		case GL_SHORT:
		{
			setChannelType(VariableType::SignedShortNorm);
			setPixelFormat(GeneratePixelType1<'r', 16>::ID);
			return true;
		}
		case GL_UNSIGNED_INT:
		{
			setChannelType(VariableType::UnsignedIntegerNorm);
			setPixelFormat(GeneratePixelType1<'r', 32>::ID);
			return true;
		}
		case GL_INT:
		{
			setChannelType(VariableType::SignedIntegerNorm);
			setPixelFormat(GeneratePixelType1<'r', 32>::ID);
			return true;
		}
		}
		break;
	}
	case GL_RG:
	{
		setColorSpace(ColorSpace::lRGB);
		switch (glType)
		{
		case GL_UNSIGNED_BYTE:
		{
			setChannelType(VariableType::UnsignedByteNorm);
			setPixelFormat(GeneratePixelType2<'r', 'g', 8, 8>::ID);
			return true;
		}
		case GL_BYTE:
		{
			setChannelType(VariableType::SignedByteNorm);
			setPixelFormat(GeneratePixelType2<'r', 'g', 8, 8>::ID);
			return true;
		}
		case GL_UNSIGNED_SHORT:
		{
			setChannelType(VariableType::UnsignedShortNorm);
			setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
			return true;
		}
		case GL_SHORT:
		{
			setChannelType(VariableType::SignedShortNorm);
			setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
			return true;
		}
		case GL_UNSIGNED_INT:
		{
			setChannelType(VariableType::UnsignedIntegerNorm);
			setPixelFormat(GeneratePixelType2<'r', 'g', 32, 32>::ID);
			return true;
		}
		case GL_INT:
		{
			setChannelType(VariableType::SignedIntegerNorm);
			setPixelFormat(GeneratePixelType2<'r', 'g', 32, 32>::ID);
			return true;
		}
		}
		break;
	}
	case GL_RGB:
	{
		setColorSpace(ColorSpace::lRGB);
		switch (glType)
		{
		case GL_UNSIGNED_BYTE_3_3_2:
		{
			setChannelType(VariableType::UnsignedByteNorm);
			setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 3, 3, 2>::ID);
			return true;
		}
		case GL_UNSIGNED_BYTE:
		{
			setChannelType(VariableType::UnsignedByteNorm);
			setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 8, 8, 8>::ID);
			return true;
		}
		case GL_BYTE:
		{
			setChannelType(VariableType::SignedByteNorm);
			setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 8, 8, 8>::ID);
			return true;
		}
		case GL_UNSIGNED_SHORT:
		{
			setChannelType(VariableType::UnsignedShortNorm);
			setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 16, 16, 16>::ID);
			return true;
		}
		case GL_SHORT:
		{
			setChannelType(VariableType::SignedShortNorm);
			setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 16, 16, 16>::ID);
			return true;
		}
		case GL_UNSIGNED_INT:
		{
			setChannelType(VariableType::UnsignedIntegerNorm);
			setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 32, 32, 32>::ID);
			return true;
		}
		case GL_INT:
		{
			setChannelType(VariableType::SignedIntegerNorm);
			setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 32, 32, 32>::ID);
			return true;
		}
		case GL_UNSIGNED_SHORT_5_6_5:
		{
			setChannelType(VariableType::UnsignedShortNorm);
			setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 5, 6, 5>::ID);
			return true;
		}
		}
		break;
	}
	case GL_RGBA:
	{
		setColorSpace(ColorSpace::lRGB);
		switch (glType)
		{
		case GL_UNSIGNED_BYTE:
		{
			setChannelType(VariableType::UnsignedByteNorm);
			setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
			return true;
		}
		case GL_BYTE:
		{
			setChannelType(VariableType::SignedByteNorm);
			setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
			return true;
		}
		case GL_UNSIGNED_SHORT:
		{
			setChannelType(VariableType::UnsignedShortNorm);
			setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
			return true;
		}
		case GL_SHORT:
		{
			setChannelType(VariableType::SignedShortNorm);
			setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
			return true;
		}
		case GL_UNSIGNED_INT:
		{
			setChannelType(VariableType::UnsignedIntegerNorm);
			setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 32, 32, 32, 32>::ID);
			return true;
		}
		case GL_INT:
		{
			setChannelType(VariableType::SignedIntegerNorm);
			setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 32, 32, 32, 32>::ID);
			return true;
		}
		case GL_UNSIGNED_SHORT_5_5_5_1:
		{
			setChannelType(VariableType::UnsignedShortNorm);
			setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 5, 5, 5, 1>::ID);
			return true;
		}
		case GL_UNSIGNED_SHORT_4_4_4_4:
		{
			setChannelType(VariableType::UnsignedShortNorm);
			setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 4, 4, 4, 4>::ID);
			return true;
		}
		}
		break;
	}
	case GL_BGRA:
	{
		setColorSpace(ColorSpace::lRGB);
		if (glType == GL_UNSIGNED_BYTE)
		{
			setChannelType(VariableType::UnsignedByteNorm);
			setPixelFormat(GeneratePixelType4<'b', 'g', 'r', 'a', 8, 8, 8, 8>::ID);
			return true;
		}
		break;
	}
	case GL_LUMINANCE_ALPHA:
	{
		setColorSpace(ColorSpace::lRGB);
		switch (glType)
		{
		case GL_UNSIGNED_BYTE:
		{
			setChannelType(VariableType::UnsignedByteNorm);
			setPixelFormat(GeneratePixelType2<'l', 'a', 8, 8>::ID);
			return true;
		}
		case GL_BYTE:
		{
			setChannelType(VariableType::SignedByteNorm);
			setPixelFormat(GeneratePixelType2<'l', 'a', 8, 8>::ID);
			return true;
		}
		case GL_UNSIGNED_SHORT:
		{
			setChannelType(VariableType::UnsignedShortNorm);
			setPixelFormat(GeneratePixelType2<'l', 'a', 16, 16>::ID);
			return true;
		}
		case GL_SHORT:
		{
			setChannelType(VariableType::SignedShortNorm);
			setPixelFormat(GeneratePixelType2<'l', 'a', 16, 16>::ID);
			return true;
		}
		case GL_UNSIGNED_INT:
		{
			setChannelType(VariableType::UnsignedIntegerNorm);
			setPixelFormat(GeneratePixelType2<'l', 'a', 32, 32>::ID);
			return true;
		}
		case GL_INT:
		{
			setChannelType(VariableType::SignedIntegerNorm);
			setPixelFormat(GeneratePixelType2<'l', 'a', 32, 32>::ID);
			return true;
		}
		}
		break;
	}
	case GL_LUMINANCE:
	{
		setColorSpace(ColorSpace::lRGB);
		switch (glType)
		{
		case GL_UNSIGNED_BYTE:
		{
			setChannelType(VariableType::UnsignedByteNorm);
			setPixelFormat(GeneratePixelType1<'l', 8>::ID);
			return true;
		}
		case GL_BYTE:
		{
			setChannelType(VariableType::SignedByteNorm);
			setPixelFormat(GeneratePixelType1<'l', 8>::ID);
			return true;
		}
		case GL_UNSIGNED_SHORT:
		{
			setChannelType(VariableType::UnsignedShortNorm);
			setPixelFormat(GeneratePixelType1<'l', 16>::ID);
			return true;
		}
		case GL_SHORT:
		{
			setChannelType(VariableType::SignedShortNorm);
			setPixelFormat(GeneratePixelType1<'l', 16>::ID);
			return true;
		}
		case GL_UNSIGNED_INT:
		{
			setChannelType(VariableType::UnsignedIntegerNorm);
			setPixelFormat(GeneratePixelType1<'l', 32>::ID);
			return true;
		}
		case GL_INT:
		{
			setChannelType(VariableType::SignedIntegerNorm);
			setPixelFormat(GeneratePixelType1<'l', 32>::ID);
			return true;
		}
		}
		break;
	}
	case GL_ALPHA:
	{
		setColorSpace(ColorSpace::lRGB);
		switch (glType)
		{
		case GL_UNSIGNED_BYTE:
		{
			setChannelType(VariableType::UnsignedByteNorm);
			setPixelFormat(GeneratePixelType1<'a', 8>::ID);
			return true;
		}
		case GL_BYTE:
		{
			setChannelType(VariableType::SignedByteNorm);
			setPixelFormat(GeneratePixelType1<'a', 8>::ID);
			return true;
		}
		case GL_UNSIGNED_SHORT:
		{
			setChannelType(VariableType::UnsignedShortNorm);
			setPixelFormat(GeneratePixelType1<'a', 16>::ID);
			return true;
		}
		case GL_SHORT:
		{
			setChannelType(VariableType::SignedShortNorm);
			setPixelFormat(GeneratePixelType1<'a', 16>::ID);
			return true;
		}
		case GL_UNSIGNED_INT:
		{
			setChannelType(VariableType::UnsignedIntegerNorm);
			setPixelFormat(GeneratePixelType1<'a', 32>::ID);
			return true;
		}
		case GL_INT:
		{
			setChannelType(VariableType::SignedIntegerNorm);
			setPixelFormat(GeneratePixelType1<'a', 32>::ID);
			return true;
		}
		}
		break;
	}
	case GL_ALPHA8:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'a', 8>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_ALPHA8_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'a', 8>::ID);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case GL_ALPHA16:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'a', 16>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_ALPHA16_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'a', 16>::ID);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case GL_ALPHA16F_ARB:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'a', 16>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_ALPHA32F_ARB:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'a', 32>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_LUMINANCE8:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'l', 8>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_LUMINANCE8_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'l', 8>::ID);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case GL_LUMINANCE16:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'l', 16>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_LUMINANCE16_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'l', 16>::ID);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case GL_LUMINANCE16F_ARB:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'l', 16>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_LUMINANCE32F_ARB:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'l', 32>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_LUMINANCE8_ALPHA8:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'l', 'a', 8, 8>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_LUMINANCE8_ALPHA8_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'l', 'a', 8, 8>::ID);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case GL_LUMINANCE_ALPHA16F_ARB:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'l', 'a', 16, 16>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_LUMINANCE_ALPHA32F_ARB:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'l', 'a', 32, 32>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_R8:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 8>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_R8_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 8>::ID);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case GL_R16:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 16>::ID);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}
	case GL_R16_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 16>::ID);
		setChannelType(VariableType::SignedShortNorm);
		return true;
	}
	case GL_R16F:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 16>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_R32F:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 32>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_R8UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 8>::ID);
		setChannelType(VariableType::UnsignedByte);
		return true;
	}
	case GL_R8I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 8>::ID);
		setChannelType(VariableType::SignedByte);
		return true;
	}
	case GL_R16UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 16>::ID);
		setChannelType(VariableType::UnsignedShort);
		return true;
	}
	case GL_R16I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 16>::ID);
		setChannelType(VariableType::SignedShort);
		return true;
	}
	case GL_R32UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 32>::ID);
		setChannelType(VariableType::UnsignedInteger);
		return true;
	}
	case GL_R32I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType1<'r', 32>::ID);
		setChannelType(VariableType::SignedInteger);
		return true;
	}
	case GL_RG8:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 8, 8>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_RG8_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 8, 8>::ID);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case GL_RG16:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}
	case GL_RG16_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
		setChannelType(VariableType::SignedShortNorm);
		return true;
	}
	case GL_RG16F:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_RG32F:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 32, 32>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_RG8UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 8, 8>::ID);
		setChannelType(VariableType::UnsignedByte);
		return true;
	}
	case GL_RG8I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 8, 8>::ID);
		setChannelType(VariableType::SignedByte);
		return true;
	}
	case GL_RG16UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
		setChannelType(VariableType::UnsignedShort);
		return true;
	}
	case GL_RG16I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
		setChannelType(VariableType::SignedShort);
		return true;
	}
	case GL_RG32UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 32, 32>::ID);
		setChannelType(VariableType::UnsignedInteger);
		return true;
	}
	case GL_RG32I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType2<'r', 'g', 32, 32>::ID);
		setChannelType(VariableType::SignedInteger);
		return true;
	}
	case GL_R3_G3_B2:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 3, 3, 2>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_RGB565:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 5, 6, 5>::ID);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}
	case GL_RGB8:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 8, 8, 8>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_RGB8_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 8, 8, 8>::ID);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case GL_SRGB8:
	{
		setColorSpace(ColorSpace::sRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 8, 8, 8>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_RGB16:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 16, 16, 16>::ID);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}
	case GL_RGB16_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 16, 16, 16>::ID);
		setChannelType(VariableType::SignedShortNorm);
		return true;
	}
	case GL_RGB10:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'x', 10, 10, 10, 2>::ID);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}
	case GL_R11F_G11F_B10F:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 11, 11, 10>::ID);
		setChannelType(VariableType::UnsignedFloat);
		return true;
	}
	case GL_RGB9_E5:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::SharedExponentR9G9B9E5);
		setChannelType(VariableType::UnsignedFloat);
		return true;
	}
	case GL_RGB16F:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 16, 16, 16>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_RGB32F:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 32, 32, 32>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_RGB8UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 8, 8, 8>::ID);
		setChannelType(VariableType::UnsignedByte);
		return true;
	}
	case GL_RGB8I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 8, 8, 8>::ID);
		setChannelType(VariableType::SignedByte);
		return true;
	}
	case GL_RGB16UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 16, 16, 16>::ID);
		setChannelType(VariableType::UnsignedShort);
		return true;
	}
	case GL_RGB16I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 16, 16, 16>::ID);
		setChannelType(VariableType::SignedShort);
		return true;
	}
	case GL_RGB32UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 32, 32, 32>::ID);
		setChannelType(VariableType::UnsignedInteger);
		return true;
	}
	case GL_RGB32I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 32, 32, 32>::ID);
		setChannelType(VariableType::SignedInteger);
		return true;
	}
	case GL_RGBA8:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_RGBA8_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case GL_SRGB8_ALPHA8:
	{
		setColorSpace(ColorSpace::sRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_RGBA16:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}
	case GL_RGBA16_SNORM:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
		setChannelType(VariableType::SignedShortNorm);
		return true;
	}
	case GL_RGB5_A1:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 5, 5, 5, 1>::ID);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}
	case GL_RGBA4:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 4, 4, 4, 4>::ID);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}
	case GL_RGB10_A2:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 10, 10, 10, 2>::ID);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}
	case GL_RGBA16F:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_RGBA32F:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 32, 32, 32, 32>::ID);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case GL_RGBA8UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
		setChannelType(VariableType::UnsignedByte);
		return true;
	}
	case GL_RGBA8I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
		setChannelType(VariableType::SignedByte);
		return true;
	}
	case GL_RGB10_A2UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 10, 10, 10, 2>::ID);
		setChannelType(VariableType::UnsignedInteger);
		return true;
	}
	case GL_RGBA16UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
		setChannelType(VariableType::UnsignedShort);
		return true;
	}
	case GL_RGBA16I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
		setChannelType(VariableType::SignedShort);
		return true;
	}
	case GL_RGBA32I:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 32, 32, 32, 32>::ID);
		setChannelType(VariableType::UnsignedInteger);
		return true;
	}
	case GL_RGBA32UI:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 32, 32, 32, 32>::ID);
		setChannelType(VariableType::SignedInteger);
		return true;
	}
	case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::PVRTCI_2bpp_RGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::PVRTCI_2bpp_RGBA);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::PVRTCI_4bpp_RGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::PVRTCI_4bpp_RGBA);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::PVRTCII_2bpp);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::PVRTCII_4bpp);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_ETC1_RGB8_OES:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::ETC1);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::DXT1);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::DXT3);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::DXT5);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_SRGB8_ETC2:
	{
		setColorSpace(ColorSpace::sRGB);
		setPixelFormat(CompressedPixelFormat::ETC2_RGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RGB8_ETC2:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::ETC2_RGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
	{
		setColorSpace(ColorSpace::sRGB);
		setPixelFormat(CompressedPixelFormat::ETC2_RGBA);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RGBA8_ETC2_EAC:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::ETC2_RGBA);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	{
		setColorSpace(ColorSpace::sRGB);
		setPixelFormat(CompressedPixelFormat::ETC2_RGB_A1);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
	{
		setColorSpace(ColorSpace::lRGB);
		setPixelFormat(CompressedPixelFormat::ETC2_RGB_A1);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_SIGNED_R11_EAC:
	{
		setColorSpace(ColorSpace::sRGB);
		setPixelFormat(CompressedPixelFormat::EAC_R11);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_R11_EAC:
	{
		setColorSpace(ColorSpace::sRGB);
		setPixelFormat(CompressedPixelFormat::EAC_R11);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_SIGNED_RG11_EAC:
	{
		setColorSpace(ColorSpace::sRGB);
		setPixelFormat(CompressedPixelFormat::EAC_RG11);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case GL_COMPRESSED_RG11_EAC:
	{
		setColorSpace(ColorSpace::sRGB);
		setPixelFormat(CompressedPixelFormat::EAC_RG11);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	}

	//Return false if format isn't found/valid.
	return false;
}

bool TextureHeader::setDirect3DFormat(uint32 d3dFormat)
{
	switch (d3dFormat)
	{
	case texture_dds::D3DFMT_R8G8B8:
	{
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_A8R8G8B8:
	{
		setPixelFormat(GeneratePixelType4<'a', 'r', 'g', 'b', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_X8R8G8B8:
	{
		setPixelFormat(GeneratePixelType4<'x', 'r', 'g', 'b', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_R5G6B5:
	{
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 5, 6, 5>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_A1R5G5B5:
	{
		setPixelFormat(GeneratePixelType4<'a', 'r', 'g', 'b', 1, 5, 5, 5>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_X1R5G5B5:
	{
		setPixelFormat(GeneratePixelType4<'x', 'r', 'g', 'b', 1, 5, 5, 5>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_A4R4G4B4:
	{
		setPixelFormat(GeneratePixelType4<'a', 'r', 'g', 'b', 4, 4, 4, 4>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_R3G3B2:
	{
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 3, 3, 2>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_A8:
	{
		setPixelFormat(GeneratePixelType1<'a', 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_A8R3G3B2:
	{
		setPixelFormat(GeneratePixelType4<'a', 'r', 'g', 'b', 8, 3, 3, 2>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_X4R4G4B4:
	{
		setPixelFormat(GeneratePixelType4<'a', 'r', 'g', 'b', 4, 4, 4, 4>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_A2B10G10R10:
	{
		setPixelFormat(GeneratePixelType4<'a', 'b', 'g', 'r', 2, 10, 10, 10>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case  texture_dds::D3DFMT_A8B8G8R8:
	{
		setPixelFormat(GeneratePixelType4<'a', 'b', 'g', 'r', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case  texture_dds::D3DFMT_X8B8G8R8:
	{
		setPixelFormat(GeneratePixelType4<'x', 'b', 'g', 'r', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_A2R10G10B10:
	{
		setPixelFormat(GeneratePixelType4<'a', 'r', 'g', 'b', 2, 10, 10, 10>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_A16B16G16R16:
	{
		setPixelFormat(GeneratePixelType4<'a', 'b', 'g', 'r', 16, 16, 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_L8:
	{
		setPixelFormat(GeneratePixelType1<'l', 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_A8L8:
	{
		setPixelFormat(GeneratePixelType2<'a', 'l', 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_A4L4:
	{
		setPixelFormat(GeneratePixelType2<'a', 'l', 4, 4>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_V8U8:
	{
		setPixelFormat(GeneratePixelType2<'g', 'r', 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_L6V5U5:
	{
		setPixelFormat(GeneratePixelType3<'l', 'g', 'r', 6, 5, 5>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_X8L8V8U8:
	{
		setPixelFormat(GeneratePixelType4<'x', 'l', 'g', 'r', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_Q8W8V8U8:
	{
		setPixelFormat(GeneratePixelType4<'a', 'b', 'g', 'r', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_V16U16:
	{
		setPixelFormat(GeneratePixelType2<'g', 'r', 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_A2W10V10U10:
	{
		//Mixed format...
		setPixelFormat(GeneratePixelType4<'a', 'b', 'g', 'r', 2, 10, 10, 10>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_UYVY:
	{
		setPixelFormat(CompressedPixelFormat::UYVY);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_R8G8_B8G8:
	{
		setPixelFormat(CompressedPixelFormat::RGBG8888);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_YUY2:
	{
		setPixelFormat(CompressedPixelFormat::YUY2);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_G8R8_G8B8:
	{
		setPixelFormat(CompressedPixelFormat::GRGB8888);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_DXT1:
	{
		setPixelFormat(CompressedPixelFormat::DXT1);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		setIsPreMultiplied(false);
		return true;
	}

	case texture_dds::D3DFMT_DXT2:
	{
		setPixelFormat(CompressedPixelFormat::DXT2);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		setIsPreMultiplied(true);
		return true;
	}

	case texture_dds::D3DFMT_DXT3:
	{
		setPixelFormat(CompressedPixelFormat::DXT3);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		setIsPreMultiplied(false);
		return true;
	}

	case texture_dds::D3DFMT_DXT4:
	{
		setPixelFormat(CompressedPixelFormat::DXT4);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		setIsPreMultiplied(true);
		return true;
	}

	case texture_dds::D3DFMT_DXT5:
	{
		setPixelFormat(CompressedPixelFormat::DXT5);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_L16:
	{
		setPixelFormat(GeneratePixelType1<'l', 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_G16R16:
	{
		setPixelFormat(GeneratePixelType2<'g', 'r', 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_Q16W16V16U16:
	{
		setPixelFormat(GeneratePixelType4<'a', 'b', 'g', 'r', 16, 16, 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedIntegerNorm);
		setIsPreMultiplied(false);
		return true;
	}

	case texture_dds::D3DFMT_R16F:
	{
		setPixelFormat(GeneratePixelType1<'r', 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		setIsPreMultiplied(false);
		return true;
	}

	case texture_dds::D3DFMT_G16R16F:
	{
		setPixelFormat(GeneratePixelType2<'g', 'r', 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		setIsPreMultiplied(false);
		return true;
	}

	case texture_dds::D3DFMT_A16B16G16R16F:
	{
		setPixelFormat(GeneratePixelType4<'a', 'b', 'g', 'r', 16, 16, 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		setIsPreMultiplied(false);
		return true;
	}

	case texture_dds::D3DFMT_R32F:
	{
		setPixelFormat(GeneratePixelType1<'r', 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		setIsPreMultiplied(false);
		return true;
	}

	case texture_dds::D3DFMT_G32R32F:
	{
		setPixelFormat(GeneratePixelType2<'g', 'r', 32, 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		setIsPreMultiplied(false);
		return true;
	}

	case texture_dds::D3DFMT_A32B32G32R32F:
	{
		setPixelFormat(GeneratePixelType4<'a', 'b', 'g', 'r', 32, 32, 32, 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_PVRTC2:
	{
		setPixelFormat(CompressedPixelFormat::PVRTCI_2bpp_RGBA);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		setIsPreMultiplied(false);
		return true;
	}
	case texture_dds::D3DFMT_PVRTC4:
	{
		setPixelFormat(CompressedPixelFormat::PVRTCI_4bpp_RGBA);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		setIsPreMultiplied(false);
		return true;
	}
	}
	return false;
}

bool TextureHeader::setDirectXGIFormat(uint32 dxgiFormat)
{
	switch (dxgiFormat)
	{
	case texture_dds::DXGI_FORMAT_R32G32B32A32_FLOAT:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 32, 32, 32, 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R32G32B32A32_UINT:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 32, 32, 32, 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedInteger);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R32G32B32A32_SINT:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 32, 32, 32, 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedInteger);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R32G32B32_FLOAT:
	{
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 32, 32, 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R32G32B32_UINT:
	{
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 32, 32, 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedInteger);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R32G32B32_SINT:
	{
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 32, 32, 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedInteger);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R16G16B16A16_FLOAT:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R16G16B16A16_UNORM:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R16G16B16A16_UINT:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedShort);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R16G16B16A16_SNORM:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedShortNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R16G16B16A16_SINT:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 16, 16, 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedShort);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R32G32_FLOAT:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 32, 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R32G32_UINT:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 32, 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedInteger);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R32G32_SINT:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 32, 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedInteger);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R10G10B10A2_UNORM:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 10, 10, 10, 2>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R10G10B10A2_UINT:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 10, 10, 10, 2>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedInteger);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R11G11B10_FLOAT:
	{
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 11, 11, 10>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R8G8B8A8_UNORM:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::sRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R8G8B8A8_UINT:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByte);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R8G8B8A8_SNORM:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R8G8B8A8_SINT:
	{
		setPixelFormat(GeneratePixelType4<'r', 'g', 'b', 'a', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedByte);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R16G16_FLOAT:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R16G16_UNORM:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_R16G16_UINT:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedShort);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R16G16_SNORM:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedShortNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R16G16_SINT:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 16, 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedShort);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R32_FLOAT:
	{
		setPixelFormat(GeneratePixelType1<'r', 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R32_UINT:
	{
		setPixelFormat(GeneratePixelType1<'r', 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedInteger);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R32_SINT:
	{
		setPixelFormat(GeneratePixelType1<'r', 32>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedInteger);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R8G8_UNORM:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R8G8_UINT:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByte);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R8G8_SNORM:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R8G8_SINT:
	{
		setPixelFormat(GeneratePixelType2<'r', 'g', 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedByte);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R16_FLOAT:
	{
		setPixelFormat(GeneratePixelType1<'r', 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R16_UNORM:
	{
		setPixelFormat(GeneratePixelType1<'r', 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R16_UINT:
	{
		setPixelFormat(GeneratePixelType1<'r', 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedShort);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R16_SNORM:
	{
		setPixelFormat(GeneratePixelType1<'r', 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedShortNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R16_SINT:
	{
		setPixelFormat(GeneratePixelType1<'r', 16>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedShort);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R8_UNORM:
	{
		setPixelFormat(GeneratePixelType1<'r', 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R8_UINT:
	{
		setPixelFormat(GeneratePixelType1<'r', 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByte);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R8_SNORM:
	{
		setPixelFormat(GeneratePixelType1<'r', 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R8_SINT:
	{
		setPixelFormat(GeneratePixelType1<'r', 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedByte);
		return true;
	}
	case texture_dds::DXGI_FORMAT_A8_UNORM:
	{
		setPixelFormat(GeneratePixelType1<'r', 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R1_UNORM:
	{
		setPixelFormat(CompressedPixelFormat::BW1bpp);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	{
		setPixelFormat(CompressedPixelFormat::SharedExponentR9G9B9E5);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case texture_dds::DXGI_FORMAT_R8G8_B8G8_UNORM:
	{
		setPixelFormat(CompressedPixelFormat::RGBG8888);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_G8R8_G8B8_UNORM:
	{
		setPixelFormat(CompressedPixelFormat::GRGB8888);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_BC1_UNORM:
	{
		setPixelFormat(CompressedPixelFormat::DXT1);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_BC1_UNORM_SRGB:
	{
		setPixelFormat(CompressedPixelFormat::DXT1);
		setColorSpace(ColorSpace::sRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_BC2_UNORM:
	{
		setPixelFormat(CompressedPixelFormat::DXT3);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_BC2_UNORM_SRGB:
	{
		setPixelFormat(CompressedPixelFormat::DXT3);
		setColorSpace(ColorSpace::sRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_BC3_UNORM:
	{
		setPixelFormat(CompressedPixelFormat::DXT5);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_BC3_UNORM_SRGB:
	{
		setPixelFormat(CompressedPixelFormat::DXT5);
		setColorSpace(ColorSpace::sRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_BC4_UNORM:
	{
		setPixelFormat(CompressedPixelFormat::BC4);
		setColorSpace(ColorSpace::sRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_BC4_SNORM:
	{
		setPixelFormat(CompressedPixelFormat::BC4);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedIntegerNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_BC5_UNORM:
	{
		setPixelFormat(CompressedPixelFormat::BC5);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}

	case texture_dds::DXGI_FORMAT_BC5_SNORM:
	{
		setPixelFormat(CompressedPixelFormat::BC5);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::SignedIntegerNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_B5G6R5_UNORM:
	{
		setPixelFormat(GeneratePixelType3<'r', 'g', 'b', 5, 6, 5>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_B5G5R5A1_UNORM:
	{
		setPixelFormat(GeneratePixelType4<'a', 'r', 'g', 'b', 5, 5, 5, 1>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_B8G8R8A8_UNORM:
	{
		setPixelFormat(GeneratePixelType4<'b', 'g', 'r', 'a', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_B8G8R8X8_UNORM:
	{
		setPixelFormat(GeneratePixelType4<'b', 'g', 'r', 'x', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	{
		setPixelFormat(GeneratePixelType4<'b', 'g', 'r', 'a', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::sRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
	{
		setPixelFormat(GeneratePixelType4<'b', 'g', 'r', 'x', 8, 8, 8, 8>::ID);
		setColorSpace(ColorSpace::sRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_BC6H_SF16:
	{
		setPixelFormat(CompressedPixelFormat::BC7);
		setColorSpace(ColorSpace::sRGB);
		setChannelType(VariableType::SignedFloat);
		return true;
	}
	case texture_dds::DXGI_FORMAT_BC7_UNORM:
	{
		setPixelFormat(CompressedPixelFormat::BC7);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_BC7_UNORM_SRGB:
	{
		setPixelFormat(CompressedPixelFormat::BC7);
		setColorSpace(ColorSpace::sRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_YUY2:
	{
		setPixelFormat(CompressedPixelFormat::YUY2);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedIntegerNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_AI44:
	{
		setPixelFormat(GeneratePixelType2<'a', 'i', 4, 4>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_IA44:
	{
		setPixelFormat(GeneratePixelType2<'i', 'a', 4, 4>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedByteNorm);
		return true;
	}
	case texture_dds::DXGI_FORMAT_B4G4R4A4_UNORM:
	{
		setPixelFormat(GeneratePixelType4<'a', 'r', 'g', 'b', 4, 4, 4, 4>::ID);
		setColorSpace(ColorSpace::lRGB);
		setChannelType(VariableType::UnsignedShortNorm);
		return true;
	}
	}

	return false;
}


void TextureHeader::setOrientation(TextureMetaData::AxisOrientation eAxisOrientation)
{
	//Get a reference to the meta data block.
	TextureMetaData& orientationMetaData = m_metaDataMap[Header::PVRv3][TextureMetaData::IdentifierTextureOrientation];

	//Check if it's already been set or not.
	if (orientationMetaData.getData())
	{
		m_header.metaDataSize -= orientationMetaData.getTotalSizeInMemory();
	}

	// Set the orientation data
	byte orientationData[3];

	//Check for left/right (x-axis) orientation
	if ((eAxisOrientation & TextureMetaData::AxisOrientationLeft) > 0)
	{
		orientationData[TextureMetaData::AxisAxisX] = TextureMetaData::AxisOrientationLeft;
	}
	else
	{
		orientationData[TextureMetaData::AxisAxisX] = TextureMetaData::AxisOrientationRight;
	}

	//Check for up/down (y-axis) orientation
	if ((eAxisOrientation & TextureMetaData::AxisOrientationUp) > 0)
	{
		orientationData[TextureMetaData::AxisAxisY] = TextureMetaData::AxisOrientationUp;
	}
	else
	{
		orientationData[TextureMetaData::AxisAxisY] = TextureMetaData::AxisOrientationDown;
	}

	//Check for in/out (z-axis) orientation
	if ((eAxisOrientation & TextureMetaData::AxisOrientationOut) > 0)
	{
		orientationData[TextureMetaData::AxisAxisZ] = TextureMetaData::AxisOrientationOut;
	}
	else
	{
		orientationData[TextureMetaData::AxisAxisZ] = TextureMetaData::AxisOrientationIn;
	}

	// Update the meta data block
	orientationMetaData = TextureMetaData(Header::PVRv3, TextureMetaData::IdentifierTextureOrientation, 3, orientationData);

	// Check that the meta data was created successfully.
	if (orientationMetaData.getDataSize() != 0)
	{
		// Increment the meta data size.
		m_header.metaDataSize += orientationMetaData.getTotalSizeInMemory();
	}
	else
	{
		// Otherwise remove it.
		m_metaDataMap.erase(TextureMetaData::IdentifierTextureOrientation);
	}
}


void TextureHeader::setCubeMapOrder(string cubeMapOrder)
{
    if(cubeMapOrder.find_first_not_of("xXyYzZ") != std::string::npos)
    {
        assertion(false , "Invalid cubemap order string");
        pvr::Log("Invalid cubemap order string");
        return;
    }

    //Get a reference to the meta data block.
	TextureMetaData& cubeOrderMetaData = m_metaDataMap[Header::PVRv3][TextureMetaData::IdentifierCubeMapOrder];

	//Check if it's already been set or not.
	if (cubeOrderMetaData.getData())
	{
		m_header.metaDataSize -= cubeOrderMetaData.getTotalSizeInMemory();
	}

	cubeOrderMetaData = TextureMetaData(Header::PVRv3, TextureMetaData::IdentifierCubeMapOrder,
                          (std::min)((uint32)cubeMapOrder.length(), 6u),reinterpret_cast<const byte*>(cubeMapOrder.data()));

	//Increment the meta data size.
	m_header.metaDataSize += cubeOrderMetaData.getTotalSizeInMemory();
}

void TextureHeader::setWidth(uint32 newWidth)
{
	m_header.width = newWidth;
}

void TextureHeader::setHeight(uint32 newHeight)
{
	m_header.height = newHeight;
}

void TextureHeader::setDepth(uint32 newDepth)
{
	m_header.depth = newDepth;
}

void TextureHeader::setNumberOfArrayMembers(uint32 newNumMembers)
{
	m_header.numberOfSurfaces = newNumMembers;
}

void TextureHeader::setNumberOfMIPLevels(uint32 newNumMIPLevels)
{
	m_header.mipMapCount = newNumMIPLevels;
}

void TextureHeader::setNumberOfFaces(uint32 newNumFaces)
{
	m_header.numberOfFaces = newNumFaces;
}

void TextureHeader::setIsFileCompressed(bool isFileCompressed)
{
	if (isFileCompressed)
	{
		m_header.flags |= Header::CompressedFlag;
	}
	else
	{
		m_header.flags &= !Header::CompressedFlag;
	}
}

void TextureHeader::setIsPreMultiplied(bool isPreMultiplied)
{
	if (isPreMultiplied)
	{
		m_header.flags |= Header::PremultipliedFlag;
	}
	else
	{
		m_header.flags &= !Header::PremultipliedFlag;
	}
}

void TextureHeader::addMetaData(const TextureMetaData& metaData)
{
	// Get a reference to the meta data block.
	TextureMetaData& currentMetaData = m_metaDataMap[metaData.getFourCC()][metaData.getKey()];

	// Check if it's already been set or not.
	if (currentMetaData.getData())
	{
		m_header.metaDataSize -= currentMetaData.getTotalSizeInMemory();
	}

	// Set the meta data block
	currentMetaData = metaData;

	// Increment the meta data size.
	m_header.metaDataSize += currentMetaData.getTotalSizeInMemory();
}

bool TextureHeader::isBumpMap()const
{
	const std::map<uint32, TextureMetaData>& dataMap = m_metaDataMap.at(Header::PVRv3);
	return (dataMap.find(TextureMetaData::IdentifierBumpData) != dataMap.end());
}

}
}
//!\endcond

/*!*********************************************************************************************************************
\file         PVRApi\Vulkan\EffectApi.cpp
\author       PowerVR by Imagination, Developer Technology Team
\copyright    Copyright (c) Imagination Technologies Limited.
\brief         OpenGL ES implementations of the EffectApi class.
***********************************************************************************************************************/
//!\cond NO_DOXYGEN
#include "PVRApi/EffectApi.h"
#include "PVRCore/Maths.h"
#include "PVRCore/IGraphicsContext.h"
#include "PVRCore/StringFunctions.h"
#include "PVRNativeApi/Vulkan/NativeObjectsVk.h"
#include "PVRNativeApi/Vulkan/VulkanBindings.h"
#include "PVRApi/ApiObjects/DescriptorSet.h"
#include <functional>
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>

using std::vector;
namespace pvr {
namespace api {
namespace impl {

uint32 EffectApi_::loadSemantics(
  const IGraphicsContext*,
  bool isAttribute)
{
	uint32 semanticIdx, nCount, nCountUnused;
	int32 nLocation;

	/*
		Loop over the parameters searching for their semantics. If
		found/recognised, it should be placed in the output array.
	*/
	nCount = 0;
	nCountUnused = 0;
	CommandBuffer cb = m_context->createCommandBufferOnDefaultPool();
	cb->bindPipeline(m_pipe);
	IndexedArray<EffectApiSemantic, StringHash>& mySemanticList = isAttribute ? m_attributes : m_uniforms;
	std::vector<assets::EffectSemantic>& myAssetEffectSemantics = isAttribute ? m_assetEffect.attributes : m_assetEffect.uniforms;
	for (semanticIdx = 0; semanticIdx < myAssetEffectSemantics.size(); ++semanticIdx)
	{
		assets::EffectSemantic& assetSemantic = myAssetEffectSemantics[semanticIdx];
		// Semantic found for this parameter.
		if (isAttribute)
		{
			nLocation = m_pipe->getAttributeLocation(assetSemantic.variableName.c_str());
		}
		else
		{
			nLocation = m_pipe->getUniformLocation(assetSemantic.variableName.c_str());
		}

		if (nLocation != -1)
		{
			EffectApiSemantic semanticToInsert;
			semanticToInsert.location = nLocation;
			semanticToInsert.semanticIndex = semanticIdx;
			semanticToInsert.variableName = assetSemantic.variableName;
			if (!isAttribute && strings::startsWith(assetSemantic.semantic.str(), "TEXTURE"))
			{
				//IF the string is TEXTUREXXXX, the texture ordinal is the texture unit.
				cb->setUniform(nLocation, assetSemantic.semantic.length() >= 8 ? atoi(assetSemantic.semantic.c_str() + 7) : 0);
			}
			mySemanticList.insertAt(semanticIdx, assetSemantic.semantic, std::move(semanticToInsert));

			++nCount;
		}
		else
		{
			Log(Log.Warning, "[EffectFile: %s Effect: %s] Variable not used by GLSL code: Semantic:%s VariableName:%s",
			    m_assetEffect.fileName.c_str(), m_assetEffect.getMaterial().getEffectName().c_str(),
			    assetSemantic.semantic.c_str(), assetSemantic.variableName.c_str());
			++nCountUnused;
		}
	}
	cb->submit();
	cb->clear();

	return nCount;
}

void EffectApi_::setTexture(uint32 nIdx, const api::TextureView& tex)
{
	using namespace assets;
	if (nIdx < (uint32)m_effectTexSamplers.size())
	{
		if (!tex.isValid()) { return; }// validate
		// Get the texture details from the PFX Parser. This contains details such as mipmapping and filter modes.
		//const StringHash& TexName = m_parser->getEffect(m_effect).textures[nIdx].name;
		//int32 iTexIdx = m_parser->findTextureByName(TexName);
		if (types::TextureDimension::Texture2D != m_effectTexSamplers[nIdx].getTextureType()) { return; }
		m_effectTexSamplers[nIdx].texture = tex;
	}
}

void EffectApi_::setDefaultUniformValue(const char8* const pszName, const assets::EffectSemanticData& psDefaultValue)
{
	using namespace assets;
	CommandBuffer cb = m_context->createCommandBufferOnDefaultPool();
	uint32 nLocation = m_pipe->getUniformLocation(pszName);
	switch (psDefaultValue.type)
	{
	case types::SemanticDataType::Mat2:
		cb->setUniform(nLocation, glm::make_mat2(psDefaultValue.dataF32));
		break;
	case types::SemanticDataType::Mat3:
		cb->setUniform(nLocation, glm::make_mat3(psDefaultValue.dataF32));
		break;
	case types::SemanticDataType::Mat4:
		cb->setUniform(nLocation, glm::make_mat4(psDefaultValue.dataF32));
		break;
	case types::SemanticDataType::Vec2:
		cb->setUniform(nLocation, glm::make_vec2(psDefaultValue.dataF32));
		break;
	case types::SemanticDataType::RGB:
	case types::SemanticDataType::Vec3:
		cb->setUniform(nLocation, glm::make_vec3(psDefaultValue.dataF32));
		break;
	case types::SemanticDataType::RGBA:
	case types::SemanticDataType::Vec4:
		cb->setUniform(nLocation, glm::make_vec4(psDefaultValue.dataF32));
		break;
	case types::SemanticDataType::IVec2:
	{
		glm::ivec2 vec;
		memcpy(glm::value_ptr(vec), psDefaultValue.dataI32, sizeof(vec));
		cb->setUniform(nLocation, vec);
		break;
	}
	case types::SemanticDataType::IVec3:
	{
		glm::ivec3 vec;
		memcpy(glm::value_ptr(vec), psDefaultValue.dataI32, sizeof(vec));
		cb->setUniform(nLocation, vec);
		break;
	}
	case types::SemanticDataType::IVec4:
	{
		glm::ivec4 vec;
		memcpy(glm::value_ptr(vec), psDefaultValue.dataI32, sizeof(vec));
		cb->setUniform(nLocation, vec);
		break;
	}
	case types::SemanticDataType::BVec2:
	{
		glm::ivec2 vec(psDefaultValue.dataBool[0] != 0, psDefaultValue.dataBool[1] != 0);
		cb->setUniform(nLocation, vec);
		break;
	}
	case types::SemanticDataType::BVec3:
	{
		glm::ivec3 vec(psDefaultValue.dataBool[0] != 0, psDefaultValue.dataBool[1] != 0, psDefaultValue.dataBool[2] != 0);
		cb->setUniform(nLocation, vec);
		break;
	}
	break;
	case types::SemanticDataType::BVec4:
	{
		glm::ivec4 vec(psDefaultValue.dataBool[0] != 0, psDefaultValue.dataBool[1] != 0, psDefaultValue.dataBool[2] != 0, psDefaultValue.dataBool[3] != 0);
		cb->setUniform(nLocation, vec);
		break;
	}
	break;
	case types::SemanticDataType::Float:
		cb->setUniform(nLocation, psDefaultValue.dataF32[0]);
		break;
	case types::SemanticDataType::Int1:
		cb->setUniform(nLocation, psDefaultValue.dataI32[0]);
		break;
	case types::SemanticDataType::Bool1:
		cb->setUniform(nLocation, int32(psDefaultValue.dataBool[0] != 0));
		break;
	default:
		break;
	}
}

Result::Enum EffectApi_::buildSemanticTables(uint32& uiUnknownSemantics)
{
	/*if (!m_context)
	{
		Log(Log.Debug, "Valid Context not set");
		return Result::NotInitialized;
	}*/
	loadSemantics(NULL, false);
	loadSemantics(NULL, true);

	return Result::Success;
}


EffectApi_::EffectApi_(GraphicsContext& context, AssetLoadingDelegate& effectDelegate) :
	m_isLoaded(false), m_delegate(&effectDelegate), m_context(context) {}

Result::Enum EffectApi_::init(const assets::Effect& effect, api::GraphicsPipelineCreateParam& pipeDesc)
{
	uint32	 i;
	m_assetEffect = effect;
	Result::Enum pvrRslt;

	//--- Initialize each Texture
	for (i = 0; i < effect.textures.size(); ++i)
	{
		m_effectTexSamplers.insertAt(i, effect.textures[i].name, EffectApiTextureSampler());
		api::TextureView tex2d = m_context->createTextureView(api::TextureStore());
		m_effectTexSamplers[i].texture = api::TextureView(tex2d);

		m_effectTexSamplers[i].name = effect.textures[i].name;
		m_effectTexSamplers[i].fileName = effect.textures[i].fileName;
		m_effectTexSamplers[i].flags = 0;
		m_effectTexSamplers[i].unit = effect.textures[i].unit;

		// create the sampler
		api::SamplerCreateParam samplerDesc;
		samplerDesc.minificationFilter = effect.textures[i].minFilter;
		samplerDesc.magnificationFilter = effect.textures[i].magFilter;
		samplerDesc.mipMappingFilter = effect.textures[i].mipFilter;

		samplerDesc.wrapModeU = effect.textures[i].wrapS;
		samplerDesc.wrapModeV = effect.textures[i].wrapT;
		samplerDesc.wrapModeW = effect.textures[i].wrapR;
		m_effectTexSamplers[i].sampler = m_context->createSampler(samplerDesc);
	}

	//--- register the custom semantics and load the requested textures
	if ((pvrRslt = loadTexturesForEffect()) != Result::Success) { return pvrRslt; }

	if (pipeDesc.pipelineLayout.isNull())
	{
		//--- create the descriptor set layout and pipeline layout
		pvr::api::DescriptorSetLayoutCreateParam descSetLayoutInfo;
		for (pvr::uint32 i = 0; i < (pvr::uint32)m_effectTexSamplers.size(); ++i)
		{
			descSetLayoutInfo.setBinding(i, types::DescriptorType::CombinedImageSampler,
			                             0, types::ShaderStageFlags::Fragment);
		}
		m_descriptorSetLayout = m_context->createDescriptorSetLayout(descSetLayoutInfo);
		PipelineLayoutCreateParam pipeLayoutCreateInfo;
		pipeLayoutCreateInfo.addDescSetLayout(m_descriptorSetLayout);
		pipeDesc.pipelineLayout = m_context->createPipelineLayout(pipeLayoutCreateInfo);
	}

	//--- create the descriptor set
	pvr::api::DescriptorSetUpdate descriptorSetInfo;
	for (pvr::uint16 i = 0; i < m_effectTexSamplers.size(); ++i)
	{
		descriptorSetInfo.setCombinedImageSamplerAtIndex(i, m_effectTexSamplers[i].unit, m_effectTexSamplers[i].texture, m_effectTexSamplers[i].sampler);
	}
	if (m_effectTexSamplers.size())
	{
		m_descriptorSet = m_context->createDescriptorSetOnDefaultPool(m_descriptorSetLayout);
		if (pvr::Result::Success != m_descriptorSet->update(descriptorSetInfo))
		{
			Log("DescriptorSet update failed");
			return pvr::Result::UnknownError;
		}
	}
	//--- construct the pipeline
	api::Shader vertShader;
	api::Shader fragmentShader;

	//--- Load the shaders
	pvrRslt = loadShadersForEffect(vertShader, fragmentShader);
	if (pvrRslt != Result::Success) { return pvrRslt; }
	pipeDesc.vertexShader.setShader(vertShader);
	pipeDesc.fragmentShader.setShader(fragmentShader);

	//--- create and validate pipeline
	m_pipe = m_context->createParentableGraphicsPipeline(pipeDesc);
	if (!m_pipe.isValid()) { return Result::NotInitialized; }

//--- Build uniform table
	pvrRslt = buildSemanticTables(m_numUnknownUniforms);
	if (pvrRslt != Result::Success) { return pvrRslt; }

	m_isLoaded = true;
	return pvrRslt;
}

Result::Enum EffectApi_::loadTexturesForEffect()
{
	Result::Enum pvrRslt = Result::Success;

	for (IndexedArray<EffectApiTextureSampler>::iterator it = m_effectTexSamplers.begin(); it != m_effectTexSamplers.end(); ++it)
	{
		pvrRslt = it->value.init(*m_delegate);
		if (pvrRslt != Result::Success) { return pvrRslt; }
	}
	return Result::Success;
}

void EffectApi_::destroy()
{
	m_effectTexSamplers.clear();
	m_isLoaded = false;
}

Result::Enum EffectApi_::loadShadersForEffect(api::Shader& vertexShader, api::Shader& fragmentShader)
{
	using namespace assets;
	// initialize attributes to default values
	string vertShaderSrc;
	string fragShaderSrc;

	// create vertex shader stream from source/ binary.
	BufferStream vertexShaderData(
	  m_assetEffect.vertexShader.glslFile.c_str(),
	  m_assetEffect.vertexShader.glslCode.c_str(),
	  m_assetEffect.vertexShader.glslCode.length());

	// create fragment shader stream from source/ binary.
	BufferStream fragmentShaderData(
	  m_assetEffect.fragmentShader.glslFile.c_str(),
	  m_assetEffect.fragmentShader.glslCode.c_str(),
	  m_assetEffect.fragmentShader.glslCode.length());

	if (vertexShaderData.getSize() == 0)
	{
		Log(Log.Error, "Effect File: [%s] -- Could not find vertex shader [%s] when processing effect [%s]",
		    m_assetEffect.fileName.c_str(), m_assetEffect.vertexShader.name.c_str(), m_assetEffect.material.getEffectName().c_str());
	}
	if (fragmentShaderData.getSize() == 0)
	{
		Log(Log.Error, "Effect File: [%s] -- Could not find fragment shader [%s]  when processing effect [%s]",
		    m_assetEffect.fileName.c_str(), m_assetEffect.fragmentShader.name.c_str(), m_assetEffect.material.getEffectName().c_str());
	}

#if defined(GL_SGX_BINARY_IMG)
	if (isVertShaderBinary)	{ vertShaderBinFmt = assets::ShaderBinaryFormat::ImgSgx; }
	if (isFragShaderBinary)	{ fragShaderBinFmt = assets::ShaderBinaryFormat::ImgSgx; }
#endif
	// load the vertex and fragment shader

	vertexShader = m_context->createShader(vertexShaderData, types::ShaderType::VertexShader, 0, 0);
	fragmentShader = m_context->createShader(fragmentShaderData, types::ShaderType::FragmentShader, 0, 0);

	if (vertexShader.isNull())
	{
		Log(Log.Error, "Effect File: [%s] -- Vertex Shader [%s] compilation error when processing effect [%s]",
		    m_assetEffect.fileName.c_str(), m_assetEffect.vertexShader.name.c_str(), m_assetEffect.material.getEffectName().c_str());
	}
	if (fragmentShader.isNull())
	{
		Log(Log.Error, "Effect File: [%s] -- Fragment Shader [%s] compilation error when processing effect [%s]",
		    m_assetEffect.fileName.c_str(), m_assetEffect.fragmentShader.name.c_str(), m_assetEffect.material.getEffectName().c_str());
	}
	return ((vertexShader.isValid() && fragmentShader.isValid()) ? Result::Success : Result::UnknownError);
}

void EffectApi_::setSampler(uint32 index, api::Sampler sampler)
{
	if (!sampler.isValid()) { return; }
	m_effectTexSamplers[index].sampler = sampler;
}

}// namespace impl
}// namespace api
}// namespace pvr
//!\endcond
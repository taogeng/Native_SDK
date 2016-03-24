/*!*********************************************************************************************************************
\File         VulkanBumpMap.cpp
\Title        Bump mapping
\Author       PowerVR by Imagination, Developer Technology Team
\Copyright    Copyright (c) Imagination Technologies Limited.
\brief		  Shows how to perform tangent space bump mapping
***********************************************************************************************************************/
#include "PVRShell/PVRShell.h"
#include "PVRApi/PVRApi.h"
#include "PVRUIRenderer/PVRUIRenderer.h"
using namespace pvr;
using namespace types;
const float32 RotateY = glm::pi<float32>() / 150;
const glm::vec4 LightDir(.24f, .685f, -.685f, 0.0f);

/*!*********************************************************************************************************************
 shader attributes
 ***********************************************************************************************************************/
// vertex attributes
namespace VertexAttrib {
enum Enum
{
	VertexArray, NormalArray, TexCoordArray, TangentArray, numAttribs
};
}

const utils::VertexBindings VertexAttribBindings[] =
{
	{ "POSITION",	0 },
	{ "NORMAL",		1 },
	{ "UV0",		2 },
	{ "TANGENT",	3 },
};

// shader uniforms
namespace Uniform {
enum Enum {	MVPMatrix, LightDir, NumUniforms };
}


/*!*********************************************************************************************************************
 Content file names
 ***********************************************************************************************************************/

// Source and binary shaders
const char FragShaderSrcFile[]		= "FragShader_VK.fsh.spv";
const char VertShaderSrcFile[]		= "VertShader_VK.vsh.spv";

// PVR texture files
const char StatueTexFile[]			= "Marble.pvr";
const char StatueNormalMapFile[]	= "MarbleNormalMap.pvr";

const char ShadowTexFile[]			= "Shadow.pvr";
const char ShadowNormalMapFile[]	= "ShadowNormalMap.pvr";

// POD scene files
const char SceneFile[]				= "scene.pod";



/*!*********************************************************************************************************************
 Class implementing the Shell functions.
 ***********************************************************************************************************************/
class OGLESBumpMap : public Shell
{
	struct UboPerMeshData
	{
		glm::mat4 mvpMtx;
		glm::vec3 lightDirModel;
	};

	// Print3D class used to display text
	ui::UIRenderer	uiRenderer;

	// 3D Model
	assets::ModelHandle scene;

	// Projection and view matrix
	glm::mat4 viewProj;

	struct DeviceResources
	{
		std::vector<api::Buffer> vbo;
		std::vector<api::Buffer> ibo;
		api::DescriptorSetLayout texLayout;
		api::DescriptorSetLayout uboLayoutDynamic;
		api::PipelineLayout pipelayout;
		api::DescriptorSet texDescSet;

		api::GraphicsPipeline pipe;
		std::vector<api::CommandBuffer> commandBuffer;// per swapchain
		std::vector<api::SecondaryCommandBuffer> uiCmdBuffer;// per swapchain
		Multi<api::Fbo> fboOnScreen;// per swapchain
		std::vector<pvr::utils::StructuredMemoryView> ubo;//per swapchain
		std::vector<api::DescriptorSet> uboDescSet;
	};

	GraphicsContext context;
	api::AssetStore assetManager;
	// The translation and Rotate parameter of Model
	float32 angleY;
	std::auto_ptr<DeviceResources> deviceResource;
public:
	OGLESBumpMap() {}
	virtual Result::Enum initApplication();
	virtual Result::Enum initView();
	virtual Result::Enum releaseView();
	virtual Result::Enum quitApplication();
	virtual Result::Enum renderFrame();

	bool createImageSamplerDescriptor();
	bool createUbo();
	bool loadPipeline();
	void loadVbos();
	void drawMesh(api::CommandBuffer& cmdBuffer, int i32NodeIndex);
	void recordCommandBuffer();
};

/*!*********************************************************************************************************************
\return	return true if no error occurred
\brief	Loads the textures required for this training course
***********************************************************************************************************************/
bool OGLESBumpMap::createImageSamplerDescriptor()
{
	api::TextureView texBase;
	api::TextureView texNormalMap;

	// create the bilinear sampler
	assets::SamplerCreateParam samplerInfo;
	samplerInfo.magnificationFilter = SamplerFilter::Linear;
	samplerInfo.minificationFilter = SamplerFilter::Linear;
	samplerInfo.mipMappingFilter = SamplerFilter::Nearest;
	api::Sampler samplerMipBilinear = context->createSampler(samplerInfo);

	samplerInfo.mipMappingFilter = SamplerFilter::Linear;
	api::Sampler samplerTrilinear = context->createSampler(samplerInfo);

	if (!assetManager.getTextureWithCaching(getGraphicsContext(), StatueTexFile,	&texBase, NULL) ||
	        !assetManager.getTextureWithCaching(getGraphicsContext(), StatueNormalMapFile, &texNormalMap, NULL))
	{
		setExitMessage("ERROR: Failed to load texture.");
		return false;
	}
	// create the descriptor set
	api::DescriptorSetUpdate descSetCreateInfo;
	descSetCreateInfo
	.setCombinedImageSampler(0, texBase, samplerMipBilinear)
	.setCombinedImageSampler(1, texNormalMap, samplerTrilinear);
	deviceResource->texDescSet = context->createDescriptorSetOnDefaultPool(deviceResource->texLayout);
	if (!deviceResource->texDescSet.isValid())
	{
		setExitMessage("ERROR: Failed to create Combined Image Sampler Descriptor set.");
		return false;
	}
	deviceResource->texDescSet->update(descSetCreateInfo);
	return true;
}

bool OGLESBumpMap::createUbo()
{
	api::DescriptorSetUpdate descUpdate;
	deviceResource->ubo.resize(getPlatformContext().getSwapChainLength());
	deviceResource->uboDescSet.resize(getPlatformContext().getSwapChainLength());
	for (pvr::uint32 i = 0; i < getPlatformContext().getSwapChainLength(); ++i)
	{
		deviceResource->ubo[i].addEntryPacked("MVPMatrix", pvr::GpuDatatypes::mat4x4);
		deviceResource->ubo[i].addEntryPacked("LightDirModel", pvr::GpuDatatypes::vec3);
		auto buffer = context->createBuffer(deviceResource->ubo[i].getAlignedTotalSize(), BufferBindingUse::UniformBuffer);
		deviceResource->ubo[i].connectWithBuffer(context->createBufferView(buffer, 0, deviceResource->ubo[i].getAlignedElementSize()),
		        pvr::BufferViewTypes::UniformBufferDynamic);
		deviceResource->uboDescSet[i] = context->createDescriptorSetOnDefaultPool(deviceResource->uboLayoutDynamic);
		descUpdate.setDynamicUbo(0, deviceResource->ubo[i].getConnectedBuffer());
		deviceResource->uboDescSet[i]->update(descUpdate);
	}
	return true;
}

/*!*********************************************************************************************************************
\return	 Return true if no error occurred
\brief	Loads and compiles the shaders and create a pipeline
***********************************************************************************************************************/
bool OGLESBumpMap::loadPipeline()
{
	api::pipelineCreation::ColorBlendAttachmentState colorAttachemtState;
	api::GraphicsPipelineCreateParam pipeInfo;
	colorAttachemtState.blendEnable = true;

	//--- create the texture-sampler descriptor set layout
	{
		api::DescriptorSetLayoutCreateParam descSetLayoutInfo;
		descSetLayoutInfo
		.setBinding(0, DescriptorType::CombinedImageSampler, 1, ShaderStageFlags::Fragment)/*binding 0*/
		.setBinding(1, DescriptorType::CombinedImageSampler, 1, ShaderStageFlags::Fragment);/*binding 1*/
		deviceResource->texLayout = context->createDescriptorSetLayout(descSetLayoutInfo);
	}

	//--- create the ubo descriptorset layout
	{
		api::DescriptorSetLayoutCreateParam descSetLayoutInfo;
		descSetLayoutInfo.setBinding(0, DescriptorType::UniformBufferDynamic, 1, ShaderStageFlags::Vertex); /*binding 0*/
		deviceResource->uboLayoutDynamic = context->createDescriptorSetLayout(descSetLayoutInfo);
	}

	//--- create the pipeline layout
	{
		api::PipelineLayoutCreateParam pipeLayoutInfo;
		pipeLayoutInfo
		.addDescSetLayout(deviceResource->texLayout)/*set 0*/
		.addDescSetLayout(deviceResource->uboLayoutDynamic);/*set 1*/
		deviceResource->pipelayout = context->createPipelineLayout(pipeLayoutInfo);
	}

	pipeInfo.colorBlend.addAttachmentState(colorAttachemtState);
	pipeInfo.vertexShader = context->createShader(*getAssetStream(VertShaderSrcFile), ShaderType::VertexShader);
	pipeInfo.fragmentShader = context->createShader(*getAssetStream(FragShaderSrcFile), ShaderType::FragmentShader);

	const assets::Mesh& mesh = scene->getMesh(0);
	pipeInfo.inputAssembler.setPrimitiveTopology(mesh.getPrimitiveType());
	pipeInfo.pipelineLayout = deviceResource->pipelayout;
	pipeInfo.renderPass = deviceResource->fboOnScreen[0]->getRenderPass();
	pipeInfo.subPass = 0;
	// Enable z-buffer test. We are using a projection matrix optimized for a floating point depth buffer,
	// so the depth test and clear value need to be inverted (1 becomes near, 0 becomes far).
	pipeInfo.depthStencil.setDepthTestEnable(true).setDepthCompareFunc(ComparisonMode::Less).setDepthWrite(true);
	utils::createInputAssemblyFromMesh(mesh, VertexAttribBindings, sizeof(VertexAttribBindings) / sizeof(VertexAttribBindings[0]), pipeInfo);
	deviceResource->pipe = context->createGraphicsPipeline(pipeInfo);
	return (deviceResource->pipe.isValid());
}

/*!*********************************************************************************************************************
\return	Return Result::Success if no error occurred
\brief	Code in initApplication() will be called by Shell once per run, before the rendering context is created.
		Used to initialize variables that are not dependent on it	(e.g. external modules, loading meshes, etc.)
		If the rendering context is lost, initApplication() will not be called again.
***********************************************************************************************************************/
Result::Enum OGLESBumpMap::initApplication()
{
	// Load the scene
	assetManager.init(*this);
	if (!assetManager.loadModel(SceneFile, scene))
	{
		this->setExitMessage("ERROR: Couldn't load the .pod file\n");
		return Result::NotInitialized;
	}
	angleY = 0.0f;
	return Result::Success;
}

/*!*********************************************************************************************************************
\return	Return Result::Success if no error occurred
\brief	Code in quitApplication() will be called by PVRShell once per run, just before exiting the program.
		If the rendering context is lost, quitApplication() will not be called.x
***********************************************************************************************************************/
Result::Enum OGLESBumpMap::quitApplication() {	return Result::Success;}

/*!*********************************************************************************************************************
\return	Return Result::Success if no error occurred
\brief	Code in initView() will be called by Shell upon initialization or after a change in the rendering context.
		Used to initialize variables that are dependent on the rendering context (e.g. textures, vertex buffers, etc.)
***********************************************************************************************************************/
Result::Enum OGLESBumpMap::initView()
{
	context = getGraphicsContext();
	deviceResource.reset(new DeviceResources());
	// load the vbo and ibo data
	utils::appendSingleBuffersFromModel(getGraphicsContext(), *scene, deviceResource->vbo, deviceResource->ibo);
	deviceResource->fboOnScreen = context->createOnScreenFboSet();
	// load the pipeline
	if (!loadPipeline()) {	return Result::UnknownError;	}
	if (!createImageSamplerDescriptor()) { return Result::UnknownError; }
	if (!createUbo()) { return Result::UnknownError; }

	//	Initialize UIRenderer
	if (uiRenderer.init(context, deviceResource->fboOnScreen[0]->getRenderPass(), 0) != Result::Success)
	{
		this->setExitMessage("ERROR: Cannot initialize UIRenderer\n");
		return Result::UnknownError;
	}

	uiRenderer.getDefaultTitle()->setText("BumpMap");
	uiRenderer.getDefaultTitle()->commitUpdates();
	glm::vec3 from, to, up;
	float32 fov;
	scene->getCameraProperties(0, fov, from, to, up);

	// Is the screen rotated
	bool bRotate = this->isScreenRotated() && this->isFullScreen();

	//	Calculate the projection and rotate it by 90 degree if the screen is rotated.
	viewProj = (bRotate ?
	            math::perspectiveFov(getApiType(), fov, (float)this->getHeight(), (float)this->getWidth(),
	                                 scene->getCamera(0).getNear(), scene->getCamera(0).getFar(), glm::pi<float32>() * .5f) :
	            math::perspectiveFov(getApiType(), fov, (float)this->getWidth(),	(float)this->getHeight(),
	                                 scene->getCamera(0).getNear(), scene->getCamera(0).getFar()));

	viewProj = viewProj * glm::lookAt(from, to, up);
	recordCommandBuffer();
	return Result::Success;
}

/*!*********************************************************************************************************************
\brief	Code in releaseView() will be called by PVRShell when theapplication quits or before a change in the rendering context.
\return	Return Result::Success if no error occurred
***********************************************************************************************************************/
Result::Enum OGLESBumpMap::releaseView()
{
	deviceResource.reset();
	uiRenderer.release();
	scene.reset();
	assetManager.releaseAll();
	return Result::Success;
}

/*!*********************************************************************************************************************
\return	Return Result::Success if no error occurred
\brief	Main rendering loop function of the program. The shell will call this function every frame.
***********************************************************************************************************************/
Result::Enum OGLESBumpMap::renderFrame()
{
	// Calculate the model matrix
	glm::mat4 mModel = glm::rotate(angleY, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::vec3(1.8f));
	angleY += -RotateY * 0.05f  * getFrameTime();

	// Set light Direction in model space
	//	The inverse of a rotation matrix is the transposed matrix
	//	Because of v * M = transpose(M) * v, this means:
	//	v * R == inverse(R) * v
	//	So we don't have to actually invert or transpose the matrix
	//	to transform back from world space to model space

	// update the ubo
	{
		UboPerMeshData srcWrite;
		srcWrite.lightDirModel = glm::vec3(LightDir * mModel);
		srcWrite.mvpMtx = viewProj * mModel * scene->getWorldMatrix(scene->getNode(0).getObjectId());
		deviceResource->ubo[getPlatformContext().getSwapChainIndex()].map();
		deviceResource->ubo[getPlatformContext().getSwapChainIndex()].setValue(0, srcWrite.mvpMtx);
		deviceResource->ubo[getPlatformContext().getSwapChainIndex()].setValue(1, srcWrite.lightDirModel);
		deviceResource->ubo[getPlatformContext().getSwapChainIndex()].unmap();
	}
	deviceResource->commandBuffer[getPlatformContext().getSwapChainIndex()]->submit();
	return Result::Success;
}

/*!*********************************************************************************************************************
\brief	Draws a assets::Mesh after the model view matrix has been set and	the material prepared.
\param	nodeIndex	Node index of the mesh to draw
***********************************************************************************************************************/
void OGLESBumpMap::drawMesh(api::CommandBuffer& cmdBuffer, int nodeIndex)
{
	uint32 meshId = scene->getNode(nodeIndex).getObjectId();
	const assets::Mesh& mesh = scene->getMesh(meshId);

	// bind the VBO for the mesh
	cmdBuffer->bindVertexBuffer(deviceResource->vbo[meshId], 0, 0);

	//	The geometry can be exported in 4 ways:
	//	- Indexed Triangle list
	//	- Non-Indexed Triangle list
	//	- Indexed Triangle strips
	//	- Non-Indexed Triangle strips
	if (mesh.getNumStrips() == 0)
	{
		// Indexed Triangle list
		if (deviceResource->ibo[meshId].isValid())
		{
			cmdBuffer->bindIndexBuffer(deviceResource->ibo[meshId], 0, mesh.getFaces().getDataType());
			cmdBuffer->drawIndexed(0, mesh.getNumFaces() * 3, 0, 0, 1);
		}
		else
		{
			// Non-Indexed Triangle list
			cmdBuffer->drawArrays(0, mesh.getNumFaces() * 3, 0, 1);
		}
	}
	else
	{
		for (int i = 0; i < (int)mesh.getNumStrips(); ++i)
		{
			int offset = 0;
			if (deviceResource->ibo[meshId].isValid())
			{
				// Indexed Triangle strips
				cmdBuffer->bindIndexBuffer(deviceResource->ibo[meshId], 0,
				                           mesh.getFaces().getDataType());
				cmdBuffer->drawIndexed(0, mesh.getStripLength(i) + 2, offset * 2, 0, 1);
			}
			else
			{
				// Non-Indexed Triangle strips
				cmdBuffer->drawArrays(0, mesh.getStripLength(i) + 2, 0, 1);
			}
			offset += mesh.getStripLength(i) + 2;
		}
	}
}

/*!*********************************************************************************************************************
\brief	Pre record the commands
***********************************************************************************************************************/
void OGLESBumpMap::recordCommandBuffer()
{
	deviceResource->commandBuffer.resize(getPlatformContext().getSwapChainLength());
	deviceResource->uiCmdBuffer.resize(getPlatformContext().getSwapChainLength());
	for (pvr::uint32 i = 0; i < getPlatformContext().getSwapChainLength(); ++i)
	{
		deviceResource->commandBuffer[i] = context->createCommandBufferOnDefaultPool();
		api::CommandBuffer cmdBuffer = deviceResource->commandBuffer[i];
		cmdBuffer->beginRecording();
		cmdBuffer->beginRenderPass(deviceResource->fboOnScreen[i], Rectanglei(0, 0, getWidth(), getHeight()), false,
		                           glm::vec4(0.00, 0.70, 0.67, 1.f));
		pvr::uint32 dynamicOffset = deviceResource->ubo[i].getAlignedElementArrayOffset(0);
		// enqueue the static states which wont be changed through out the frame
		cmdBuffer->bindPipeline(deviceResource->pipe);
		cmdBuffer->bindDescriptorSet(deviceResource->pipelayout, 0, deviceResource->texDescSet, 0);
		cmdBuffer->bindDescriptorSet(deviceResource->pipelayout, 1, deviceResource->uboDescSet[i], &dynamicOffset, 1);
		drawMesh(cmdBuffer, 0);

		// record the uirenderer commands
		api::SecondaryCommandBuffer uiCmdBuffer = context->createSecondaryCommandBufferOnDefaultPool();
		deviceResource->uiCmdBuffer[i] = uiCmdBuffer;
		uiRenderer.beginRendering(uiCmdBuffer);
		uiRenderer.getDefaultTitle()->render();
		uiRenderer.getSdkLogo()->render();
		uiRenderer.endRendering();
		cmdBuffer->enqueueSecondaryCmds(uiCmdBuffer);
		cmdBuffer->endRenderPass();
		cmdBuffer->endRecording();
	}
}

/*!*********************************************************************************************************************
\return	Return an auto ptr to the demo supplied by the user
\brief	This function must be implemented by the user of the shell.	The user should return its
		Shell object defining the behavior of the application.
***********************************************************************************************************************/
std::auto_ptr<Shell> pvr::newDemo() {	return std::auto_ptr<Shell>(new OGLESBumpMap()); }

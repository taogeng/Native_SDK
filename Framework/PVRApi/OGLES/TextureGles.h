/*!*********************************************************************************************************************
\file         PVRApi/OGLES/TextureGles.h
\author       PowerVR by Imagination, Developer Technology Team
\copyright    Copyright (c) Imagination Technologies Limited.
\brief         Contains OpenGL ES specific implementation of the Texture class. Use only if directly using OpenGL ES calls.
Provides the definitions allowing to move from the Framework object Texture2D to the underlying OpenGL ES Texture.
***********************************************************************************************************************/
#pragma once
#include "PVRApi/ApiObjects/Texture.h"
#include "PVRNativeApi/OGLES/NativeObjectsGles.h"
namespace pvr {

namespace api {
namespace gles {
class TextureStoreGles_ : public native::HTexture_, public impl::TextureStore_
{
public:
	/*!*******************************************************************************************
	\brief Return the basic dimensioning of the texture (1D/2D/3D).
	\return The TextureDimension
	**********************************************************************************************/
	types::TextureDimension::Enum getDimensions() const;

	/*!*******************************************************************************************
	\brief Check if this texture is allocated.
	\return true if the texture is allocated. Otherwise, the texture is empty and must be constructed.
	**********************************************************************************************/
	bool isAllocated() const { return handle != 0; }

	/*!*******************************************************************************************
	\brief Constructor.
	\param context The GraphicsContext where this Texture will belong
	**********************************************************************************************/
	TextureStoreGles_(GraphicsContext& context) : TextureStore_(context), HTexture_(0, 0), m_sampler(0) {}

	/*!*******************************************************************************************
	\brief Constructor. Use to wrap a preexisting, underlying texture object.
	\param context The GraphicsContext where this Texture will belong
	\param texture An already existing texture object of the underlying API. The underlying object
	will be then owned by this TextureStore object, destroying it when done. If shared
	semantics are required, use the overload accepting a smart pointer object.
	\description NOTE: This object will take ownership of the passed texture object, destroying it in its destructor.
	**********************************************************************************************/
	TextureStoreGles_(GraphicsContext& context, const native::HTexture_& texture) : TextureStore_(context), HTexture_(texture), m_sampler(0) {}

	/*!*******************************************************************************************
	\brief Destructor. Will properly release all resources held by this object.
	**********************************************************************************************/
	~TextureStoreGles_();


	/*!************************************************************************************************************
	\brief	Return the API's texture handle object.
	\param red    The swizzling that will be applied on the texel's "red" channel before that is returned to the shader.
	\param green    The swizzling that will be applied on the texel's "green" channel before that is returned to the shader.
	\param blue    The swizzling that will be applied on the texel's "blue" channel before that is returned to the shader.
	\param alpha    The swizzling that will be applied on the texel's "alpha" channel before that is returned to the shader.
	***************************************************************************************************************/
	void setSwizzle(types::Swizzle::Enum red = types::Swizzle::Identity, types::Swizzle::Enum green = types::Swizzle::Identity,
	                types::Swizzle::Enum blue = types::Swizzle::Identity, types::Swizzle::Enum alpha = types::Swizzle::Identity);


	mutable const impl::Sampler_* m_sampler;// for ES2
};

typedef RefCountedResource<gles::TextureStoreGles_> TextureStoreGles;

class TextureViewGles_ : public native::HImageView_, public impl::TextureView_
{
public:
	TextureViewGles_(const TextureStoreGles& texture,
	                 const types::ImageSubresourceRange& range = types::ImageSubresourceRange(),
	                 const types::SwizzleChannels swizzleChannels = types::SwizzleChannels()) :
		impl::TextureView_(texture), m_subResourceRange(range), m_swizzleChannels(swizzleChannels) {}

	const types::ImageSubresourceRange& getSubResourceRange()const {	return m_subResourceRange;	}
    const types::SwizzleChannels getSwizzleChannel()const { return m_swizzleChannels; }
private:
	types::ImageSubresourceRange m_subResourceRange;
	types::SwizzleChannels m_swizzleChannels;
};
typedef RefCountedResource<gles::TextureViewGles_> TextureViewGles;

}
}
namespace native {
/*!*********************************************************************************************************************
\brief Get the OpenGL ES texture object underlying a PVRApi Texture object.
\return A smart pointer wrapper containing the OpenGL ES Texture
\description The smart pointer returned by this function will keep alive the underlying OpenGL ES object even if all other
references to the texture (including the one that was passed to this function) are released.
***********************************************************************************************************************/
inline HTexture createNativeHandle(const RefCountedResource<api::impl::TextureStore_>& texture)
{
	return static_cast<api::gles::TextureStoreGles>(texture);
}


}
}

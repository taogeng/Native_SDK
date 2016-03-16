#include "PVRApi/Vulkan/SyncVk.h"
#include "PVRApi/Vulkan/BufferVk.h"
#include "PVRApi/Vulkan/TextureVk.h"


namespace pvr {
namespace api {

namespace vulkan {

FenceVk_::~FenceVk_()
{
	destroy();
}
bool FenceVk_::init(bool createSignaled)
{
	VkFenceCreateInfo nfo;
	nfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	nfo.pNext = 0;
	nfo.flags = ((createSignaled != 0) * VK_FENCE_CREATE_SIGNALED_BIT);
	VkResult res = vk::CreateFence(native_cast(*context).getDevice(), &nfo, NULL, &*native_cast(*this));
	vkThrowIfFailed(res, "FenceVk_::init: Failed to create Fence object");
	return res == VK_SUCCESS;
}
void FenceVk_::destroy()
{
	if (context.isNull())
	{
		Log(Log.Warning, "Attempted to destroy Fence object after context was released.");
	}
	else
	{
		vk::DestroyFence(native_cast(*context).getDevice(), handle, NULL);
		handle  = VK_NULL_HANDLE;
	}
}
SemaphoreVk_::~SemaphoreVk_()
{
	destroy();
}
bool SemaphoreVk_::init()
{
	VkSemaphoreCreateInfo nfo;
	nfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	nfo.pNext = 0;
	nfo.flags = 0;
	VkResult res = vk::CreateSemaphore(native_cast(*context).getDevice(), &nfo, NULL, &*native_cast(*this));
	vkThrowIfFailed(res, "SemaphoreVk_::init: Failed to create Semaphore object");
	return res == VK_SUCCESS;
}
void SemaphoreVk_::destroy()
{
	if (context.isNull())
	{
		Log(Log.Warning, "Attempted to destroy Semaphore object after context was released.");
	}
	else
	{
		vk::DestroySemaphore(native_cast(*context).getDevice(), handle, NULL);
		handle  = VK_NULL_HANDLE;
	}
}
EventVk_::~EventVk_()
{
	destroy();
}
bool EventVk_::init()
{
	VkEventCreateInfo nfo;
	nfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	nfo.pNext = 0;
	nfo.flags = 0;
	VkResult res = vk::CreateEvent(native_cast(*context).getDevice(), &nfo, NULL, &*native_cast(*this));
	vkThrowIfFailed(res, "EventVk_::init: Failed to create Semaphore object");
	return res == VK_SUCCESS;
}
void EventVk_::destroy()
{
	if (context.isNull())
	{
		Log(Log.Warning, "Attempted to destroy Semaphore object after context was released.");
	}
	else
	{
		vk::DestroyEvent(native_cast(*context).getDevice(), handle, NULL);
		handle  = VK_NULL_HANDLE;
	}
}
}

namespace impl {
bool Fence_::wait(uint64 timeoutNanos)
{
	VkResult res;
	res = vk::WaitForFences(native_cast(*context).getDevice(), 1, &native_cast(*this).handle, true, timeoutNanos);
	vkThrowIfFailed(res, "Fence::wait returned an error");
	if (res == VK_SUCCESS) { return true; }
	else if (res == VK_TIMEOUT) { return false; }
	return false;
}
void Fence_::reset()
{
	vkThrowIfFailed(vk::ResetFences(native_cast(*context).getDevice(), 1, &*native_cast(*this)), "Fence::reset returned an error");
}
bool Fence_::isSignalled()
{
	VkResult res = vk::GetFenceStatus(native_cast(*context).getDevice(), native_cast(*this).handle);
	vkThrowIfFailed(res, "Fence::wait returned an error");
	return (res == VK_SUCCESS ? true : false);
}

void Event_::set()
{
	vkThrowIfFailed(vk::SetEvent(native_cast(*context).getDevice(), *native_cast(*this)), "Event::set returned an error");
}
void Event_::reset()
{
	vkThrowIfFailed(vk::ResetEvent(native_cast(*context).getDevice(), *native_cast(*this)), "Event::reset returned an error");
}
bool Event_::isSet()
{
	VkResult res = vk::GetEventStatus(native_cast(*context).getDevice(), *native_cast(*this));
	if (res == VK_EVENT_SET)
	{
		return true;
	}
	else if (res == VK_EVENT_RESET)
	{
		return false;
	}
	vkThrowIfFailed(res, "Event::set returned an error");
	return false;
}



template <typename T_, typename VkT_, typename TVk_>
class MultiContainer
{
public:
	typedef T_ ContainedType;
	typedef VkT_ VulkanType;
	typedef TVk_ ApiType;
	std::vector<ContainedType> itemsvk;
	VulkanType* cookedItems;
	uint16 cookedItemsSize;
	bool isCooked;
	MultiContainer() : cookedItems(NULL), cookedItemsSize(0), isCooked(false) {}
	void add(const ContainedType& item)
	{
		isCooked = false;
		itemsvk.push_back(static_cast<ApiType>(item));
	}
	void add(ContainedType* items, uint32 numItems)
	{
		isCooked = false;
		itemsvk.reserve(itemsvk.size() + numItems);
		for (uint32 i = 0; i < numItems; ++i)
		{
			itemsvk.push_back(static_cast<ApiType>(items[i]));
		}

	}
	void assign(ContainedType* items, uint32 numItems)
	{
		itemsvk.clear();
		add(items, numItems);
	}
	void clear()
	{
		isCooked = false;
		itemsvk.clear();
	}
	void cook()
	{
		if (isCooked) { return; }
		if (cookedItemsSize != itemsvk.size())
		{
			delete cookedItems;
			if (itemsvk.size())
			{
				cookedItems = new VulkanType[itemsvk.size()];
			}
			else
			{
				cookedItems = 0;
			}
			cookedItemsSize = (uint16)itemsvk.size();
		}
		for (uint32 i = 0; i < itemsvk.size(); ++i)
		{
			cookedItems[i] = *static_cast<typename ApiType::ElementType&>(*itemsvk[i]);
		}
		isCooked = true;
	}
	VulkanType* getVulkanArray()
	{
		cook();
		return cookedItems;
	}
	uint32 size() { return (uint32)itemsvk.size(); }
	virtual ~MultiContainer() { delete[] cookedItems; }
};


class FenceSetImpl_ : public MultiContainer<Fence, VkFence, vulkan::FenceVk>
{
public:
	bool wait(uint64 timeoutNanos, bool waitAll)
	{
		cook();
		if (cookedItemsSize == 0) { return true; }
		VkResult res;
		res = vk::WaitForFences(native_cast(*itemsvk[0]->getContext()).getDevice(), cookedItemsSize, getVulkanArray(), waitAll, timeoutNanos);
		if (res == VK_SUCCESS) { return true; }
		else if (res == VK_TIMEOUT) { return false; }
		vkThrowIfFailed(res, "FenceSet::wait returned an error");
		return false;
	}
	void resetAll()
	{
		cook();
		if (cookedItemsSize == 0) { return; }
		VkDevice device = native_cast(*itemsvk[0]->getContext()).getDevice();
		vk::ResetFences(device, cookedItemsSize, cookedItems);
	}
};

FenceSet_::FenceSet_() { pimpl.reset(new FenceSetImpl_()); }
FenceSet_::FenceSet_(Fence* fences, uint32 numFences)
{
	pimpl.reset(new FenceSetImpl_());
	add(fences, numFences);
}
FenceSet_::~FenceSet_() { }
void FenceSet_::add(const Fence& fence) {pimpl->add(fence); }
void FenceSet_::add(Fence* fences, uint32 numFences) {pimpl->add(fences, numFences); }
void FenceSet_::assign(Fence* fences, uint32 numFences) {pimpl->assign(fences, numFences); }
bool FenceSet_::waitOne(uint64 timeout) { return pimpl->wait(timeout, false); }
bool FenceSet_::waitAll(uint64 timeout) { return pimpl->wait(timeout, true); }
void FenceSet_::clear() { pimpl->clear(); }
void FenceSet_::resetAll() { pimpl->resetAll(); }
const Fence& FenceSet_::operator[](uint32 index)const { return pimpl->itemsvk[index]; }
Fence& FenceSet_::operator[](uint32 index) { return pimpl->itemsvk[index]; }

const void* FenceSet_::getNativeFences()const { return pimpl->getVulkanArray(); }
uint32 FenceSet_::getNativeFencesCount() const { return pimpl->size(); }


class SemaphoreSetImpl_ : public MultiContainer<Semaphore, VkSemaphore, vulkan::SemaphoreVk>
{
};
SemaphoreSet_::SemaphoreSet_() { pimpl.reset(new SemaphoreSetImpl_()); }
SemaphoreSet_::SemaphoreSet_(Semaphore* semaphores, uint32 numSemaphores)
{
	pimpl.reset(new SemaphoreSetImpl_());
	add(semaphores, numSemaphores);
}
SemaphoreSet_::~SemaphoreSet_() { }
void SemaphoreSet_::add(const Semaphore& semaphore) { pimpl->add(semaphore); }
void SemaphoreSet_::add(Semaphore* semaphores, uint32 numSemaphores) { pimpl->add(semaphores, numSemaphores); }
void SemaphoreSet_::assign(Semaphore* semaphores, uint32 numSemaphores) { pimpl->assign(semaphores, numSemaphores); }
void SemaphoreSet_::clear() {pimpl->clear(); }
const Semaphore& SemaphoreSet_::operator[](uint32 index)const { return pimpl->itemsvk[index]; }
Semaphore& SemaphoreSet_::operator[](uint32 index) { return pimpl->itemsvk[index]; }

const void* SemaphoreSet_::getNativeSemaphores()const { return pimpl->getVulkanArray(); }
uint32 SemaphoreSet_::getNativeSemaphoresCount() const { return pimpl->size(); }


class EventSetImpl_ : public MultiContainer<Event, VkEvent, vulkan::EventVk>
{
};

EventSet_::EventSet_() { pimpl.reset(new EventSetImpl_()); }
EventSet_::EventSet_(Event* events, uint32 numEvents)
{
	pimpl.reset(new EventSetImpl_());
	add(events, numEvents);
}
EventSet_::~EventSet_() { }
void EventSet_::add(const Event& Event) { pimpl->add(Event); }
void EventSet_::add(Event* Events, uint32 numEvents) { pimpl->add(Events, numEvents); }
void EventSet_::assign(Event* Events, uint32 numEvents) { pimpl->assign(Events, numEvents); }
void EventSet_::clear() { pimpl->clear(); }
const Event& EventSet_::operator[](uint32 index)const { return pimpl->itemsvk[index]; }
Event& EventSet_::operator[](uint32 index) { return pimpl->itemsvk[index]; }

const void* EventSet_::getNativeEvents()const { return pimpl->getVulkanArray(); }
uint32 EventSet_::getNativeEventsCount() const { return pimpl->size(); }

void EventSet_::setAll()
{
	auto it = pimpl->itemsvk.begin();
	auto end = pimpl->itemsvk.end();
	while (it != end)
	{
		(*it)->set();
	}
}
void EventSet_::resetAll()
{
	auto it = pimpl->itemsvk.begin();
	auto end = pimpl->itemsvk.end();
	while (it != end)
	{
		(*it)->reset();
	}
}
bool EventSet_::any()
{
	auto it = pimpl->itemsvk.begin();
	auto end = pimpl->itemsvk.end();
	while (it != end)
	{
		if ((*it)->isSet()) { return true; }
	}
	return false;
}
bool EventSet_::all()
{
	auto it = pimpl->itemsvk.begin();
	auto end = pimpl->itemsvk.end();
	while (it != end)
	{
		if (!(*it)->isSet()) { return false; }
	}
	return true;
}


class MemoryBarrierSetImpl
{
public:
	std::vector<VkMemoryBarrier> memBarriers;
	std::vector<VkImageMemoryBarrier> imgBarriers;
	std::vector<VkBufferMemoryBarrier> bufBarriers;
};


}
MemoryBarrierSet::MemoryBarrierSet() : pimpl(new impl::MemoryBarrierSetImpl()) { }
MemoryBarrierSet::~MemoryBarrierSet() {}
MemoryBarrierSet& MemoryBarrierSet::addBarrier(MemoryBarrier memBarrier)
{
	VkMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.pNext = 0;
	barrier.srcAccessMask = ConvertToVk::accessFlags(memBarrier.srcMask);
	barrier.dstAccessMask = ConvertToVk::accessFlags(memBarrier.dstMask);
	pimpl->memBarriers.push_back(barrier);
	return *this;
}
MemoryBarrierSet& MemoryBarrierSet::addBarrier(const BufferRangeBarrier& buffBarrier)
{
	VkBufferMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.pNext = 0;
	barrier.srcAccessMask = ConvertToVk::accessFlags(buffBarrier.srcMask);
	barrier.dstAccessMask = ConvertToVk::accessFlags(buffBarrier.dstMask);

	barrier.dstQueueFamilyIndex = 0;
	barrier.srcQueueFamilyIndex = 0;

	barrier.buffer = native_cast(*buffBarrier.buffer).buffer;
	barrier.offset = buffBarrier.offset;
	barrier.size = buffBarrier.range;
	pimpl->bufBarriers.push_back(barrier);
	return *this;
}
MemoryBarrierSet& MemoryBarrierSet::addBarrier(const ImageAreaBarrier& imgBarrier)
{
	VkImageMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = 0;
	barrier.srcAccessMask = ConvertToVk::accessFlags(imgBarrier.srcMask);
	barrier.dstAccessMask = ConvertToVk::accessFlags(imgBarrier.dstMask);

	barrier.dstQueueFamilyIndex = 0;
	barrier.srcQueueFamilyIndex = 0;

	barrier.image = native_cast(*imgBarrier.texture).image;
	barrier.newLayout = ConvertToVk::imageLayout(imgBarrier.newLayout);
	barrier.oldLayout = ConvertToVk::imageLayout(imgBarrier.oldLayout);
	barrier.subresourceRange = ConvertToVk::imageSubResourceRange(imgBarrier.area);
	pimpl->imgBarriers.push_back(barrier);
	return *this;
}

uint32 MemoryBarrierSet::getNativeMemoryBarriersCount() const { return (uint32)pimpl->memBarriers.size(); }
const void* MemoryBarrierSet::getNativeMemoryBarriers() const { return pimpl->memBarriers.data(); }
uint32 MemoryBarrierSet::getNativeImageBarriersCount() const { return (uint32)pimpl->imgBarriers.size(); }
const void* MemoryBarrierSet::getNativeImageBarriers() const { return pimpl->imgBarriers.data(); }
uint32 MemoryBarrierSet::getNativeBufferBarriersCount() const { return (uint32)pimpl->bufBarriers.size(); }
const void* MemoryBarrierSet::getNativeBufferBarriers() const { return pimpl->bufBarriers.data(); }

}
}

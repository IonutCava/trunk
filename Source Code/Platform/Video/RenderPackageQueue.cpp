#include "stdafx.h"

#include "Headers/RenderPackageQueue.h"

namespace Divide {

RenderPackageQueue::RenderPackageQueue(GFXDevice& context, size_t reserveSize)
    : _context(context),
      _locked(false),
      _currentCount(0),
      _packages(reserveSize, nullptr)
{
}

void RenderPackageQueue::clear() {
    for (U32 idx = 0; idx < _currentCount; ++idx) {
        _packages[idx]->clear();
    }
    _currentCount = 0;
}

U32 RenderPackageQueue::size() const {
    return _currentCount;
}

U32 RenderPackageQueue::size(RenderPackage::MinQuality qualityRequirement) const {
    U32 count = 0;
    for (U32 idx = 0; idx < _currentCount; ++idx) {
        if (_packages[idx]->qualityRequirement() == qualityRequirement) {
            ++count;
        }
    }

    return count;
}

bool RenderPackageQueue::locked() const {
    return _locked;
}

bool RenderPackageQueue::empty() const {
    return _currentCount == 0;
}

const GFX::CommandBuffer& RenderPackageQueue::getCommandBuffer(U32 idx) {
    assert(idx < _currentCount);
    RenderPackage& pkg = *_packages[idx];
    bool cacheMiss = false;
    return Attorney::RenderPackageRenderPackageQueue::buildAndGetCommandBuffer(pkg, cacheMiss);
}

const GFX::CommandBuffer& RenderPackageQueue::getCommandBuffer(RenderPackage::MinQuality qualityRequirement, U32 idx) {
    U32 idxInternal = 0;

    U8 i = 0;
    for (; i < _currentCount; ++i) {
        if (_packages[i]->qualityRequirement() == qualityRequirement) {
            if (idx == idxInternal) {
                break;
            }
            ++idxInternal;
        }
    }

    RenderPackage& pkg = *_packages[i];
    bool cacheMiss = false;
    return Attorney::RenderPackageRenderPackageQueue::buildAndGetCommandBuffer(pkg, cacheMiss);
}

RenderPackage& RenderPackageQueue::back() {
    assert(_currentCount > 0);

    return *_packages[std::max(to_I32(_currentCount) - 1, 0)];
}

bool RenderPackageQueue::push_back(const RenderPackage& package) {
    if (_currentCount <= Config::MAX_VISIBLE_NODES) {
        std::shared_ptr<RenderPackage>& pkg = _packages[_currentCount++];
        if (!pkg) {
            pkg = std::make_shared<RenderPackage>(_context, true);
        }
        pkg->set(package);
        return true;
    }
    return false;
}

bool RenderPackageQueue::pop_back() {
    if (!empty()) {
        _packages.pop_back();
        --_currentCount;
        return true;
    }

    return false;
}

void RenderPackageQueue::lock() {
    _locked = true;
}

void RenderPackageQueue::unlock() {
    _locked = false;
}

}; //namespace Divide

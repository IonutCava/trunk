#include "stdafx.h"

#include "Headers/RenderPackage.h"

namespace Divide {

RenderPackageQueue::RenderPackageQueue(GFXDevice& context, size_t queueSize)
    : _context(context),
      _locked(false),
      _currentCount(0),
      _packages(queueSize, nullptr)
{
    std::generate(
        std::begin(_packages),
        std::end(_packages),
        [&context]() { return std::make_shared<RenderPackage>(context, false); }
    );
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

bool RenderPackageQueue::locked() const {
    return _locked;
}

bool RenderPackageQueue::empty() const {
    return _currentCount == 0;
}

const RenderPackage& RenderPackageQueue::getPackage(U32 idx) const {
    assert(idx < Config::MAX_VISIBLE_NODES);
    return *_packages[idx];
}

RenderPackage& RenderPackageQueue::getPackage(U32 idx) {
    assert(idx < Config::MAX_VISIBLE_NODES);
    return *_packages[idx];
}

RenderPackage& RenderPackageQueue::back() {
    return *_packages[std::max(to_I32(_currentCount) - 1, 0)];
}

bool RenderPackageQueue::push_back(const RenderPackage& package) {
    if (_currentCount <= Config::MAX_VISIBLE_NODES) {
        _packages[_currentCount++]->set(package);
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

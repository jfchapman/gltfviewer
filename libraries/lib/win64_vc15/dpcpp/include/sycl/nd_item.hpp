//==--------- nd_item.hpp --- SYCL iteration nd_item -----------------------==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include <CL/__spirv/spirv_ops.hpp>
#include <sycl/access/access.hpp>
#include <sycl/detail/defines.hpp>
#include <sycl/detail/helpers.hpp>
#include <sycl/group.hpp>
#include <sycl/id.hpp>
#include <sycl/item.hpp>
#include <sycl/nd_range.hpp>
#include <sycl/range.hpp>
#include <sycl/sub_group.hpp>

#include <cstddef>
#include <stdexcept>
#include <type_traits>

namespace sycl {
__SYCL_INLINE_VER_NAMESPACE(_V1) {
namespace detail {
class Builder;
}

/// Identifies an instance of the function object executing at each point in an
/// nd_range.
///
/// \ingroup sycl_api
template <int dimensions = 1> class nd_item {
public:
  nd_item() = delete;

  id<dimensions> get_global_id() const { return globalItem.get_id(); }

  size_t __SYCL_ALWAYS_INLINE get_global_id(int dimension) const {
    size_t Id = globalItem.get_id(dimension);
    __SYCL_ASSUME_INT(Id);
    return Id;
  }

  size_t __SYCL_ALWAYS_INLINE get_global_linear_id() const {
    size_t Id = globalItem.get_linear_id();
    __SYCL_ASSUME_INT(Id);
    return Id;
  }

  id<dimensions> get_local_id() const { return localItem.get_id(); }

  size_t __SYCL_ALWAYS_INLINE get_local_id(int dimension) const {
    size_t Id = localItem.get_id(dimension);
    __SYCL_ASSUME_INT(Id);
    return Id;
  }

  size_t get_local_linear_id() const {
    size_t Id = localItem.get_linear_id();
    __SYCL_ASSUME_INT(Id);
    return Id;
  }

  group<dimensions> get_group() const { return Group; }

  sub_group get_sub_group() const { return sub_group(); }

  size_t __SYCL_ALWAYS_INLINE get_group(int dimension) const {
    size_t Size = Group[dimension];
    __SYCL_ASSUME_INT(Size);
    return Size;
  }

  size_t __SYCL_ALWAYS_INLINE get_group_linear_id() const {
    size_t Id = Group.get_linear_id();
    __SYCL_ASSUME_INT(Id);
    return Id;
  }

  range<dimensions> get_group_range() const { return Group.get_group_range(); }

  size_t __SYCL_ALWAYS_INLINE get_group_range(int dimension) const {
    size_t Range = Group.get_group_range(dimension);
    __SYCL_ASSUME_INT(Range);
    return Range;
  }

  range<dimensions> get_global_range() const { return globalItem.get_range(); }

  size_t get_global_range(int dimension) const {
    return globalItem.get_range(dimension);
  }

  range<dimensions> get_local_range() const { return localItem.get_range(); }

  size_t get_local_range(int dimension) const {
    return localItem.get_range(dimension);
  }

  __SYCL2020_DEPRECATED("offsets are deprecated in SYCL 2020")
  id<dimensions> get_offset() const { return globalItem.get_offset(); }

  nd_range<dimensions> get_nd_range() const {
    return nd_range<dimensions>(get_global_range(), get_local_range(),
                                get_offset());
  }

  void barrier(access::fence_space accessSpace =
                   access::fence_space::global_and_local) const {
    uint32_t flags = detail::getSPIRVMemorySemanticsMask(accessSpace);
    __spirv_ControlBarrier(__spv::Scope::Workgroup, __spv::Scope::Workgroup,
                           flags);
  }

  /// Executes a work-group mem-fence with memory ordering on the local address
  /// space, global address space or both based on the value of \p accessSpace.
  template <access::mode accessMode = access::mode::read_write>
  __SYCL2020_DEPRECATED("use sycl::atomic_fence() free function instead")
  void mem_fence(
      typename detail::enable_if_t<accessMode == access::mode::read ||
                                       accessMode == access::mode::write ||
                                       accessMode == access::mode::read_write,
                                   access::fence_space>
          accessSpace = access::fence_space::global_and_local) const {
    (void)accessSpace;
    Group.mem_fence();
  }

  template <typename dataT>
  device_event async_work_group_copy(local_ptr<dataT> dest,
                                     global_ptr<dataT> src,
                                     size_t numElements) const {
    return Group.async_work_group_copy(dest, src, numElements);
  }

  template <typename dataT>
  device_event async_work_group_copy(global_ptr<dataT> dest,
                                     local_ptr<dataT> src,
                                     size_t numElements) const {
    return Group.async_work_group_copy(dest, src, numElements);
  }

  template <typename dataT>
  device_event async_work_group_copy(local_ptr<dataT> dest,
                                     global_ptr<dataT> src, size_t numElements,
                                     size_t srcStride) const {

    return Group.async_work_group_copy(dest, src, numElements, srcStride);
  }

  template <typename dataT>
  device_event async_work_group_copy(global_ptr<dataT> dest,
                                     local_ptr<dataT> src, size_t numElements,
                                     size_t destStride) const {
    return Group.async_work_group_copy(dest, src, numElements, destStride);
  }

  template <typename... eventTN> void wait_for(eventTN... events) const {
    Group.wait_for(events...);
  }

  nd_item(const nd_item &rhs) = default;

  nd_item(nd_item &&rhs) = default;

  nd_item &operator=(const nd_item &rhs) = default;

  nd_item &operator=(nd_item &&rhs) = default;

  bool operator==(const nd_item &rhs) const {
    return (rhs.localItem == this->localItem) &&
           (rhs.globalItem == this->globalItem) && (rhs.Group == this->Group);
  }

  bool operator!=(const nd_item &rhs) const { return !((*this) == rhs); }

protected:
  friend class detail::Builder;
  nd_item(const item<dimensions, true> &GL, const item<dimensions, false> &L,
          const group<dimensions> &GR)
      : globalItem(GL), localItem(L), Group(GR) {}

private:
  item<dimensions, true> globalItem;
  item<dimensions, false> localItem;
  group<dimensions> Group;
};

template <int Dims>
__SYCL_DEPRECATED("use sycl::ext::oneapi::experimental::this_nd_item() instead")
nd_item<Dims> this_nd_item() {
#ifdef __SYCL_DEVICE_ONLY__
  return detail::Builder::getElement(detail::declptr<nd_item<Dims>>());
#else
  throw sycl::exception(
      sycl::make_error_code(sycl::errc::feature_not_supported),
      "Free function calls are not supported on host device");
#endif
}

namespace ext {
namespace oneapi {
namespace experimental {
template <int Dims> nd_item<Dims> this_nd_item() {
#ifdef __SYCL_DEVICE_ONLY__
  return sycl::detail::Builder::getElement(
      sycl::detail::declptr<nd_item<Dims>>());
#else
  throw sycl::exception(
      sycl::make_error_code(sycl::errc::feature_not_supported),
      "Free function calls are not supported on host device");
#endif
}
} // namespace experimental
} // namespace oneapi
} // namespace ext
} // __SYCL_INLINE_VER_NAMESPACE(_V1)
} // namespace sycl

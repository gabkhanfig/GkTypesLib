#pragma once

#include "../BasicTypes.h"
#include <type_traits>
#include "JobFuture.h"

namespace gk {
	namespace job {
		namespace internal {
			struct ALIGN_AS(64) JobContainer {

			private:

				template<typename ReturnT>
				using WithinJobFuture = gk::job::internal::WithinJobFuture<ReturnT>;

				//template<typename... T>
				//using TupleWithRemovedRefs = std::tuple<typename std::remove_reference<T>::type...>;

				struct BaseFunc {
					virtual void invoke() = 0;
					virtual bool isObject(const void* obj) const = 0;
					virtual void destruct() = 0;
				};


				template<typename ObjT, typename FuncClassT, typename ReturnT, typename... Args>
				struct ObjectFuncImplInPlace : public BaseFunc {

					typedef ReturnT(FuncClassT::* MemberFunc)(Args...);

					ObjectFuncImplInPlace(ObjT* inObject, MemberFunc inFunc, WithinJobFuture<ReturnT>&& inFuture, std::tuple<Args...>&& inArgs)
						: obj(inObject), func(inFunc), future(std::move(inFuture)), args(std::move(inArgs)) {}

					~ObjectFuncImplInPlace() = default;

					virtual void invoke() override {
						callFunc(std::index_sequence_for<Args...>{});
					}

					virtual bool isObject(const void* inObj) const {
						return obj == inObj;
					}

					virtual void destruct() override {
						this->~ObjectFuncImplInPlace();
					}

					template<size_t... Is>
					inline void callFunc(std::index_sequence<Is...>) {
						if constexpr (std::is_same<ReturnT, void>::value) {
							((FuncClassT*)obj->*func)(std::get<Is>(args)...);
							future.set(true);
							return;
						}
						else {
							ReturnT temp = ((FuncClassT*)obj->*func)(std::get<Is>(args)...);
							future.set(std::move(temp));
							return;
						}
					}

					ObjT* obj;
					MemberFunc func;
					WithinJobFuture<ReturnT> future;
					std::tuple<Args...> args;
				};

			public:

				JobContainer() : internalBuffer{ 0 } {}

				JobContainer(const JobContainer&) = delete;
				JobContainer(JobContainer && other) noexcept {
					memcpy(this->internalBuffer, other.internalBuffer, sizeof(JobContainer::internalBuffer));
					memset(other.internalBuffer, 0, sizeof(JobContainer::internalBuffer));
				}

				JobContainer& operator = (const JobContainer&) = delete;
				JobContainer& operator = (JobContainer && other) noexcept {
					if (internalBuffer[0] != 0) {
						BaseFunc* asBaseFunc = (BaseFunc*)(internalBuffer);
						asBaseFunc->destruct();
					}

					memcpy(this->internalBuffer, other.internalBuffer, sizeof(JobContainer::internalBuffer));
					memset(other.internalBuffer, 0, sizeof(JobContainer::internalBuffer));
					return *this;
				}

				~JobContainer() {
					if (internalBuffer[0] != 0) {
						BaseFunc* asBaseFunc = (BaseFunc*)(internalBuffer);
						asBaseFunc->destruct();
					}
					memset(internalBuffer, 0, sizeof(JobContainer::internalBuffer));
				}

				template<typename ObjT, typename FuncClassT, typename ReturnT, typename... Args>
				static JobContainer bindMember(ObjT * inObject, ReturnT(FuncClassT:: * inFunc)(Args...), WithinJobFuture<ReturnT> && inFuture, Args&&... inArgs) {
					auto tuple = std::make_tuple(inArgs...);
					static_assert(sizeof(tuple) < 32);
					JobContainer out;
					new (out.internalBuffer) ObjectFuncImplInPlace(inObject, inFunc, std::move(inFuture), std::move(tuple));
					return out;
				}

				void invoke() {
					BaseFunc* asBaseFunc = (BaseFunc*)(internalBuffer);
					gk_assertm(isBound(), "Cannot invoke non-bound job");
					asBaseFunc->invoke();
				}

				bool isBound() const {
					const BaseFunc* asBaseFunc = (const BaseFunc*)(internalBuffer);
					return asBaseFunc != nullptr;
				}

			private:

				size_t internalBuffer[8];

			};
		} // namespace internal
	} // namespace job
} // namespace gk
#pragma once

#include "../BasicTypes.h"
#include <type_traits>
#include "JobFuture.h"
#include <functional>

namespace gk {
	namespace job {
		namespace internal {
			struct ALIGN_AS(64) JobContainer {

			private:

				template<typename ReturnT>
				using WithinJobFuture = gk::job::internal::WithinJobFuture<ReturnT>;

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

				template<typename ObjT, typename FuncClassT, typename ReturnT, typename... Args>
				struct ObjectFuncImplOnHeap : public BaseFunc {

					typedef ReturnT(FuncClassT::* MemberFunc)(Args...);

					ObjectFuncImplOnHeap(ObjT* inObject, MemberFunc inFunc, WithinJobFuture<ReturnT>&& inFuture, std::tuple<Args...>&& inArgs)
						: obj(inObject), func(inFunc), future(std::move(inFuture)), args(new std::tuple<Args...>(std::move(inArgs))) {}

					~ObjectFuncImplOnHeap() = default;

					virtual void invoke() override {
						callFunc(std::index_sequence_for<Args...>{});
					}

					virtual bool isObject(const void* inObj) const {
						return obj == inObj;
					}

					virtual void destruct() override {
						this->~ObjectFuncImplOnHeap();
					}

					template<size_t... Is>
					inline void callFunc(std::index_sequence<Is...>) {
						if constexpr (std::is_same<ReturnT, void>::value) {
							((FuncClassT*)obj->*func)(std::get<Is>(*args)...); // IMPORTANT TO DEREF
							future.set(true);
							return;
						}
						else {
							ReturnT temp = ((FuncClassT*)obj->*func)(std::get<Is>(*args)...); // IMPORTANT TO DEREF
							future.set(std::move(temp));
							return;
						}
					}

					ObjT* obj;
					MemberFunc func;
					WithinJobFuture<ReturnT> future;
					std::tuple<Args...>* args;
				};

				template<typename ObjT, typename FuncClassT, typename ReturnT, typename... Args>
				struct ConstObjectFuncImplInPlace : public BaseFunc {

					typedef ReturnT(FuncClassT::* MemberFunc)(Args...) const;

					ConstObjectFuncImplInPlace(const ObjT* inObject, MemberFunc inFunc, WithinJobFuture<ReturnT>&& inFuture, std::tuple<Args...>&& inArgs)
						: obj(inObject), func(inFunc), future(std::move(inFuture)), args(std::move(inArgs)) {}

					~ConstObjectFuncImplInPlace() = default;

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
							((const FuncClassT*)obj->*func)(std::get<Is>(args)...);
							future.set(true);
							return;
						}
						else {
							ReturnT temp = ((const FuncClassT*)obj->*func)(std::get<Is>(args)...);
							future.set(std::move(temp));
							return;
						}
					}

					const ObjT* obj;
					MemberFunc func;
					WithinJobFuture<ReturnT> future;
					std::tuple<Args...> args;
				};

				template<typename ObjT, typename FuncClassT, typename ReturnT, typename... Args>
				struct ConstObjectFuncImplOnHeap : public BaseFunc {

					typedef ReturnT(FuncClassT::* MemberFunc)(Args...) const;

					ConstObjectFuncImplOnHeap(const ObjT* inObject, MemberFunc inFunc, WithinJobFuture<ReturnT>&& inFuture, std::tuple<Args...>&& inArgs)
						: obj(inObject), func(inFunc), future(std::move(inFuture)), args(new std::tuple<Args...>(std::move(inArgs))) {}

					~ConstObjectFuncImplOnHeap() = default;

					virtual void invoke() override {
						callFunc(std::index_sequence_for<Args...>{});
					}

					virtual bool isObject(const void* inObj) const {
						return obj == inObj;
					}

					virtual void destruct() override {
						this->~ObjectFuncImplOnHeap();
					}

					template<size_t... Is>
					inline void callFunc(std::index_sequence<Is...>) {
						if constexpr (std::is_same<ReturnT, void>::value) {
							((const FuncClassT*)obj->*func)(std::get<Is>(*args)...); // IMPORTANT TO DEREF
							future.set(true);
							return;
						}
						else {
							ReturnT temp = ((const FuncClassT*)obj->*func)(std::get<Is>(*args)...); // IMPORTANT TO DEREF
							future.set(std::move(temp));
							return;
						}
					}

					const ObjT* obj;
					MemberFunc func;
					WithinJobFuture<ReturnT> future;
					std::tuple<Args...>* args;
				};

				template<typename ReturnT, typename... Args>
				struct FreeFunctionInPlace : public BaseFunc {
					typedef ReturnT(*FuncT)(Args...);

					FreeFunctionInPlace(FuncT inFunc, WithinJobFuture<ReturnT>&& inFuture, std::tuple<Args...>&& inArgs)
						: func(inFunc), future(std::move(inFuture)), args(std::move(inArgs)) {}

					~FreeFunctionInPlace() = default;

					virtual void invoke() override {
						callFunc(std::index_sequence_for<Args...>{});
					}

					virtual bool isObject(const void* inObj) const {
						return false;
					}

					virtual void destruct() override {
						this->~ObjectFuncImplInPlace();
					}

					template<size_t... Is>
					inline void callFunc(std::index_sequence<Is...>) {
						if constexpr (std::is_same<ReturnT, void>::value) {
							func(std::get<Is>(args)...);
							future.set(true);
							return;
						}
						else {
							ReturnT temp = func(std::get<Is>(args)...);
							future.set(std::move(temp));
							return;
						}
					}

					FuncT func;
					WithinJobFuture<ReturnT> future;
					std::tuple<Args...> args;
				};

				template<typename ReturnT, typename... Args>
				struct FreeFunctionOnHeap : public BaseFunc {
					typedef ReturnT(*FuncT)(Args...);

					FreeFunctionOnHeap(FuncT inFunc, WithinJobFuture<ReturnT>&& inFuture, std::tuple<Args...>&& inArgs)
						: func(inFunc), future(std::move(inFuture)), args(new std::tuple<Args...>(std::move(inArgs))) {}

					~FreeFunctionOnHeap() = default;

					virtual void invoke() override {
						callFunc(std::index_sequence_for<Args...>{});
					}

					virtual bool isObject(const void* inObj) const {
						return false;
					}

					virtual void destruct() override {
						this->~ObjectFuncImplInPlace();
					}

					template<size_t... Is>
					inline void callFunc(std::index_sequence<Is...>) {
						if constexpr (std::is_same<ReturnT, void>::value) {
							func(std::get<Is>(*args)...); // IMPORTANT TO DEREF
							future.set(true);
							return;
						}
						else {
							ReturnT temp = func(std::get<Is>(*args)...); // IMPORTANT TO DEREF
							future.set(std::move(temp));
							return;
						}
					}

					FuncT func;
					WithinJobFuture<ReturnT> future;
					std::tuple<Args...>* args;
				};

				template<typename ReturnT>
				struct StdFunction : public BaseFunc {
					using FuncT = std::function<ReturnT()>;

					StdFunction(FuncT&& inStdFunction, WithinJobFuture<ReturnT>&& inFuture)
						: func(std::move(inStdFunction)), future(std::move(inFuture)) {}

					~StdFunction() = default;

					virtual void invoke() override {
						if constexpr (std::is_same<ReturnT, void>::value) {
							//func(std::get<Is>(*args)...); // IMPORTANT TO DEREF
							func();
							future.set(true);
							return;
						}
						else {
							//ReturnT temp = func(std::get<Is>(*args)...); // IMPORTANT TO DEREF
							ReturnT temp = func();
							future.set(std::move(temp));
							return;
						}



						//callFunc(std::index_sequence_for<Args...>{});
					}

					virtual bool isObject(const void* inObj) const {
						return false;
					}

					virtual void destruct() override {
						this->~ObjectFuncImplInPlace();
					}

					FuncT func;
					WithinJobFuture<ReturnT> future;
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

#pragma region Bind

				template<typename ObjT, typename FuncClassT, typename ReturnT, typename... Args>
				static JobContainer bindMember(ObjT * inObject, ReturnT(FuncClassT:: * inFunc)(Args...), WithinJobFuture<ReturnT> && inFuture, Args&&... inArgs) {
					auto tuple = std::make_tuple(inArgs...);
					JobContainer out;
					if constexpr (sizeof(tuple) <= 32) {
						std::cout << "bind member job container stack\n";
						std::cout << "sizeof(tuple): " << sizeof(tuple) << '\n';
						new (out.internalBuffer) ObjectFuncImplInPlace(inObject, inFunc, std::move(inFuture), std::move(tuple));
					}
					else {
						std::cout << "bind member job container heap\n";
						std::cout << "sizeof(tuple): " << sizeof(tuple) << '\n';
						new (out.internalBuffer) ObjectFuncImplOnHeap(inObject, inFunc, std::move(inFuture), std::move(tuple));
					}				
					return out;
				}

				template<typename ObjT, typename FuncClassT, typename ReturnT, typename... Args>
				static JobContainer bindConstMember(const ObjT * inObject, ReturnT(FuncClassT:: * inFunc)(Args...) const, WithinJobFuture<ReturnT> && inFuture, Args&&... inArgs) {
					auto tuple = std::make_tuple(inArgs...);
					JobContainer out;
					if constexpr (sizeof(tuple) <= 32) {
						std::cout << "bind const member job container stack\n";
						std::cout << "sizeof(tuple): " << sizeof(tuple) << '\n';
						new (out.internalBuffer) ConstObjectFuncImplInPlace(inObject, inFunc, std::move(inFuture), std::move(tuple));
					}
					else {
						std::cout << "bind const member job container heap\n";
						std::cout << "sizeof(tuple): " << sizeof(tuple) << '\n';
						new (out.internalBuffer) ConstObjectFuncImplOnHeap(inObject, inFunc, std::move(inFuture), std::move(tuple));
					}				
					return out;
				}

				template<typename ReturnT, typename... Args>
				static JobContainer bindFreeFunction(ReturnT(*inFunc)(Args...), WithinJobFuture<ReturnT>&& inFuture, std::tuple<Args...>&& inArgs) {
					auto tuple = std::make_tuple(inArgs...);
					JobContainer out;
					if constexpr (sizeof(tuple) <= 40) {
						std::cout << "bind free job container stack\n";
						std::cout << "sizeof(tuple): " << sizeof(tuple) << '\n';
						new (out.internalBuffer) FreeFunctionInPlace(inFunc, std::move(inFuture), std::move(tuple));
					}
					else {
						std::cout << "bind free job container heap\n";
						std::cout << "sizeof(tuple): " << sizeof(tuple) << '\n';
						new (out.internalBuffer) FreeFunctionOnHeap(inFunc, std::move(inFuture), std::move(tuple));
					}
				}

				template<typename ReturnT>
				static JobContainer bindStdFunction(std::function<ReturnT()>&& inFunc, WithinJobFuture<ReturnT>&& inFuture) {
					JobContainer out; 
					new (out.internalBuffer) StdFunction(std::move(inFunc), std::move(inFuture));
					return out;
				}

#pragma endregion

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
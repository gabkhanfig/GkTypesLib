#pragma once

#include "../String/GlobalString.h"

namespace gk 
{
	/* Handle using other classes as variables and easy runtime object creation. 
	See macro CLASS_REF_STATIC_BODY().  */
	class ClassRef
	{
	private:

		class ClassRefInternal
		{
		public:

			virtual void* NewObject() const = 0;
		};

		template <class T>
		class ClassRefInternalImpl : public ClassRefInternal
		{
		public:

			virtual void* NewObject() const override
			{
				return (void*)new T();
			}
		};

	private:

		ClassRefInternal* classReference;

		GlobalString className;

	private:

		ClassRef()
			: classReference(nullptr)
		{}

		~ClassRef() {
			if (classReference) {
				delete classReference;
			}
		}

	public:

		template<class T>
		static ClassRef* CreateClassReference(GlobalString inClassName) {
			ClassRef* newClassReference = new ClassRef();
			newClassReference->classReference = new ClassRefInternalImpl<T>();
			newClassReference->className = inClassName;
			return newClassReference;
		}

		/* Constructs a new instance of the templated class object. Make sure to cast to the correct (base) class. */
		void* NewObject() const {
			return classReference->NewObject();
		}

		/* Get the name of the contained class. */
		GlobalString GetName() const {
			return className;
		}

	};

}

/* Static body function for a class to generate a static class reference object to create instances of this class.
Use T::GetStaticClassRef() to get the class reference object to get the class reference object to instantiate more.
@param T: Class typename */
#define CLASS_REF_STATIC_BODY(T) \
public:	\
static gk::ClassRef* GetStaticClassRef() { \
	static gk::ClassRef* classReference = gk::ClassRef::CreateClassReference<T>(#T); \
	return classReference; \
}

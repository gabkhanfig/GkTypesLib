#include "../pch.h"
#include "../../GkTypesLib/GkTypes/ClassRef/ClassRef.h"

class ClassRefTestClass1 {
public:
  int a;
  int b;
  int c;

  ClassRefTestClass1() {
    a = 1;
    b = 2;
    c = 3;
  }

  virtual int GetNumber() { return 10; }
};

class ClassRefTestClass2 : public ClassRefTestClass1 {
public:

  ClassRefTestClass2() {
    a = 4;
    b = 5;
    c = 6;
  }

  virtual int GetNumber() override { return 20; }

};

namespace UnitTests {
  TEST(ClassRef, Name) {
    gk::ClassRef* ref = gk::ClassRef::CreateClassReference<ClassRefTestClass1>(gk::GlobalString::create("TestClass1"_str));
    EXPECT_EQ(ref->GetName().toString(), "TestClass1"_str);
  }

  TEST(ClassRef, NewCorrectClassType) {
    gk::ClassRef* ref = gk::ClassRef::CreateClassReference<ClassRefTestClass1>(gk::GlobalString::create("TestClass1"_str));
    ClassRefTestClass1* instance = (ClassRefTestClass1*)ref->NewObject();
    EXPECT_EQ(instance->a, 1);
    EXPECT_EQ(instance->b, 2);
    EXPECT_EQ(instance->c, 3);
  }

  TEST(ClassRef, SubclassConstructorValues) {
    gk::ClassRef* ref = gk::ClassRef::CreateClassReference<ClassRefTestClass2>(gk::GlobalString::create("TestClass2"_str));
    ClassRefTestClass1* instance = (ClassRefTestClass1*)ref->NewObject();
    EXPECT_EQ(instance->a, 4);
    EXPECT_EQ(instance->b, 5);
    EXPECT_EQ(instance->c, 6);
  }

  TEST(ClassRef, BaseClassVirtualCall) {
    gk::ClassRef* ref = gk::ClassRef::CreateClassReference<ClassRefTestClass1>(gk::GlobalString::create("TestClass1"_str));
    ClassRefTestClass1* instance = (ClassRefTestClass1*)ref->NewObject();
    EXPECT_EQ(instance->GetNumber(), 10);
  }

  TEST(ClassRef, ChildClassVirtualCall) {
    gk::ClassRef* ref = gk::ClassRef::CreateClassReference<ClassRefTestClass2>(gk::GlobalString::create("TestClass2"_str));
    ClassRefTestClass1* instance = (ClassRefTestClass1*)ref->NewObject();
    EXPECT_EQ(instance->GetNumber(), 20);
  }
}
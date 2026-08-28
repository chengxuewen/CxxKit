/***********************************************************************************************************************
**
** Library: CxxKit
**
** Copyright (C) 2025~Present ChengXueWen.
** Copyright 2016 The WebRTC Project Authors.
**
** License: MIT License
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
** and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#include <cxxkit/memory/shared_data.hpp>

#include <thread>
#include <memory>
#include <utility>

#include <gtest/gtest.h>

CXXKIT_BEGIN_NAMESPACE

namespace
{
class MyClass : public SharedData
{
public:
    MyClass(int id = 0, const std::string &name = "")
        : mId(id)
        , mName(name)
    {
    }
    MyClass(const MyClass &other)
        : SharedData(other)
        , mId(other.mId)
        , mName(other.mName)
    {
    }

    int id() const { return mId; }
    void setId(int id) { mId = id; }

    std::string name() const { return mName; }
    void set_name(const std::string &name) { mName = name; }

    void mutating() { }

    void notMutating() const { }

    MyClass &operator=(const MyClass &) { return *this; }

private:
    int mId;
    std::string mName;
};
class MyClassHandler
{
public:
    MyClassHandler()
        : d(new MyClass)
    {
    }
    MyClassHandler(int id, const std::string &name)
        : d(new MyClass(id, name))
    {
    }

    int id() const { return d->id(); }
    std::string name() const { return d->name(); }

    void setId(int id) { d->setId(id); }
    void set_name(const std::string &name) { d->set_name(name); }

    int ref_count() const { return d->ref_count(); }
    bool isShared() const { return ref_count() > 1; }

private:
    ImplicitlySharedDataPointer<MyClass> d;
};

class Base : public SharedData
{
public:
    virtual ~Base() { }
    virtual Base *clone() { return new Base(*this); }
    virtual bool isBase() const { return true; }
};
class Derived : public Base
{
public:
    virtual Base *clone() { return new Derived(*this); }
    virtual bool isBase() const { return false; }
};
} // namespace


TEST(SharedDataTest, ImplicitlyConstructor)
{
    MyClassHandler obj(100, "test");
    EXPECT_EQ(obj.id(), 100);
    EXPECT_EQ(obj.name(), std::string("test"));
    EXPECT_EQ(obj.ref_count(), 1);
}

TEST(SharedDataTest, ImplicitlySharing)
{
    MyClassHandler obj1(1, "Original");

    // copy constructor-sharing
    MyClassHandler obj2 = obj1;
    EXPECT_EQ(obj1.ref_count(), 2);
    EXPECT_EQ(obj2.ref_count(), 2);
    EXPECT_TRUE(obj1.isShared());
    EXPECT_TRUE(obj2.isShared());

    // obj2 modify-detach
    obj2.set_name("Modified");
    EXPECT_EQ(obj1.ref_count(), 1);
    EXPECT_EQ(obj2.ref_count(), 1);
    EXPECT_TRUE(!obj1.isShared());
    EXPECT_TRUE(!obj2.isShared());

    EXPECT_EQ(obj1.name(), std::string("Original"));
    EXPECT_EQ(obj2.name(), std::string("Modified"));
}

TEST(SharedDataTest, ImplicitlyAssignment)
{
    MyClassHandler obj1(1, "Alice");
    MyClassHandler obj2(2, "Bob");

    EXPECT_EQ(obj2.name(), std::string("Bob"));

    // copy sharing
    obj2 = obj1;
    EXPECT_EQ(obj1.ref_count(), 2);
    EXPECT_EQ(obj2.ref_count(), 2);
    EXPECT_EQ(obj2.name(), std::string("Alice"));
}

TEST(SharedDataTest, ImplicitlyThreadSafety)
{
    MyClassHandler sharedObj(42, "Shared");

    std::atomic<int> successCount{0};
    const int THREAD_COUNT = 10;
    std::thread *threads[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i] = new std::thread(
            [&sharedObj, &successCount]()
            {
                // create local copy in thread
                MyClassHandler localCopy = sharedObj;
                // verify data consistency
                if (localCopy.id() == 42 && localCopy.name() == "Shared")
                {
                    successCount.fetch_add(1, std::memory_order_relaxed);
                }
                // localCopy destructor will decrease ref count
            });
    }

    // join thread wait finish
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        threads[i]->join();
        delete threads[i];
    }

    // verify all threads success read correct data
    EXPECT_EQ(successCount.load(), THREAD_COUNT);
    // finally ref count should be 1 (only sharedObj holds)
    EXPECT_EQ(sharedObj.ref_count(), 1);
}

TEST(SharedDataTest, ImplicitlyEdgeCases)
{
    // test default constructor
    MyClassHandler empty;
    EXPECT_EQ(empty.id(), 0);
    EXPECT_EQ(empty.name(), std::string());

    // test self-assignment
    MyClassHandler obj(5, "Self");
    obj = obj; // should not crash or change state
    EXPECT_EQ(obj.id(), 5);
    EXPECT_EQ(obj.ref_count(), 1);
}

TEST(SharedDataTest, ExplicitlyPointerOperatorOnConst)
{
    /* Pointer itself is const. */
    {
        const ExplicitlySharedDataPointer<const MyClass> pointer(new MyClass());
        pointer->notMutating();
    }

    /* Pointer itself is mutable. */
    {
        ExplicitlySharedDataPointer<const MyClass> pointer(new MyClass());
        pointer->notMutating();
    }
}

TEST(SharedDataTest, ExplicitlyPointerOperatorOnMutable)
{
    /* Pointer itself is const. */
    {
        const ExplicitlySharedDataPointer<MyClass> pointer(new MyClass());
        pointer->notMutating();
        pointer->mutating();
        *pointer = MyClass();
    }

    /* Pointer itself is mutable. */
    {
        const ExplicitlySharedDataPointer<MyClass> pointer(new MyClass());
        pointer->notMutating();
        pointer->mutating();
        *pointer = MyClass();
    }
}

TEST(SharedDataTest, ExplicitlyPointerCopyConstructor)
{
    const ExplicitlySharedDataPointer<const MyClass> pointer(new MyClass());
    const ExplicitlySharedDataPointer<const MyClass> copy(pointer);
}

TEST(SharedDataTest, ExplicitlyPointerClone)
{
    /* holding a base element */
    {
        ExplicitlySharedDataPointer<Base> pointer(new Base);
        EXPECT_TRUE(pointer->isBase());

        ExplicitlySharedDataPointer<Base> copy(pointer);
        pointer.detach();
        EXPECT_TRUE(pointer->isBase());
    }

    /* holding a derived element */
    {
        ExplicitlySharedDataPointer<Derived> pointer(new Derived);
        EXPECT_TRUE(!pointer->isBase());

        ExplicitlySharedDataPointer<Derived> copy(pointer);
        pointer.detach();
        EXPECT_TRUE(!pointer->isBase());
    }
}

TEST(SharedDataTest, ExplicitlyPointerData)
{
    /* check default value. */
    {
        ExplicitlySharedDataPointer<const MyClass> pointer;
        EXPECT_EQ(pointer.data(), static_cast<const MyClass *>(0));
        EXPECT_TRUE(pointer == nullptr);
        EXPECT_TRUE(nullptr == pointer);
    }

    /* On const pointer. Must not mutate the pointer. */
    {
        const ExplicitlySharedDataPointer<const MyClass> pointer(new MyClass());
        pointer.data();

        /* check that this cast is possible. */
        CXXKIT_UNUSED(static_cast<const MyClass *>(pointer.data()));

        EXPECT_TRUE(!(pointer == nullptr));
        EXPECT_TRUE(!(nullptr == pointer));
    }

    /* On mutatable pointer. Must not mutate the pointer. */
    {
        ExplicitlySharedDataPointer<const MyClass> pointer(new MyClass());
        pointer.data();

        /* check that this cast is possible. */
        CXXKIT_UNUSED(static_cast<const MyClass *>(pointer.data()));
    }

    /* Must not mutate the pointer. */
    {
        const ExplicitlySharedDataPointer<MyClass> pointer(new MyClass());
        pointer.data();

        /* check that these casts are possible. */
        CXXKIT_UNUSED(static_cast<MyClass *>(pointer.data()));
        CXXKIT_UNUSED(static_cast<const MyClass *>(pointer.data()));
    }

    /* Must not mutate the pointer. */
    {
        ExplicitlySharedDataPointer<MyClass> pointer(new MyClass());
        pointer.data();

        /* check that these casts are possible. */
        CXXKIT_UNUSED(static_cast<MyClass *>(pointer.data()));
        CXXKIT_UNUSED(static_cast<const MyClass *>(pointer.data()));
    }
}

TEST(SharedDataTest, ExplicitlyPointerReset)
{
    /* Do reset on a single ref count. */
    {
        ExplicitlySharedDataPointer<MyClass> pointer(new MyClass());
        EXPECT_TRUE(pointer.data() != 0);

        pointer.reset();
        EXPECT_EQ(pointer.data(), static_cast<MyClass *>(0));
    }

    /* Do reset on a default constructed object. */
    {
        ExplicitlySharedDataPointer<MyClass> pointer;
        EXPECT_EQ(pointer.data(), static_cast<MyClass *>(0));

        pointer.reset();
        EXPECT_EQ(pointer.data(), static_cast<MyClass *>(0));
    }
}

TEST(SharedDataTest, ExplicitlyPointerSwap)
{
    ExplicitlySharedDataPointer<MyClass> p1(0), p2(new MyClass());
    EXPECT_TRUE(!p1.data());
    EXPECT_TRUE(p2.data());

    p1.swap(p2);
    EXPECT_TRUE(p1.data());
    EXPECT_TRUE(!p2.data());

    p1.swap(p2);
    EXPECT_TRUE(!p1.data());
    EXPECT_TRUE(p2.data());

    std::swap(p1, p2);
    EXPECT_TRUE(p1.data());
    EXPECT_TRUE(!p2.data());
}

CXXKIT_END_NAMESPACE

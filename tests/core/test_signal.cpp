#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include "dolbot/core/signal.hpp"

using namespace dolbot::core;

TEST(SignalTest, ConnectAndFire) {
    Signal<int> signal;
    int received = 0;
    
    auto handle = signal.connect([&received](int val) {
        received = val;
    });
    
    signal.fire(42);
    
    EXPECT_EQ(received, 42);
}

TEST(SignalTest, MultipleListeners) {
    Signal<int> signal;
    int count = 0;
    
    auto h1 = signal.connect([&count](int) { count++; });
    auto h2 = signal.connect([&count](int) { count++; });
    auto h3 = signal.connect([&count](int) { count++; });
    
    signal.fire(1);
    
    EXPECT_EQ(count, 3);
}

TEST(SignalTest, DisconnectStopsReceiving) {
    Signal<int> signal;
    int count = 0;
    
    auto handle = signal.connect([&count](int) { count++; });
    
    signal.fire(1);
    EXPECT_EQ(count, 1);
    
    handle.disconnect();
    
    signal.fire(2);
    EXPECT_EQ(count, 1);  // Should still be 1
}

TEST(SignalTest, ConnectionCountAccurate) {
    Signal<> signal;
    
    EXPECT_EQ(signal.connection_count(), 0);
    
    auto h1 = signal.connect([]() {});
    EXPECT_EQ(signal.connection_count(), 1);
    
    auto h2 = signal.connect([]() {});
    EXPECT_EQ(signal.connection_count(), 2);
    
    h1.disconnect();
    EXPECT_EQ(signal.connection_count(), 1);
}

TEST(SignalTest, FireWithNoListeners) {
    Signal<int, double> signal;
    
    signal.fire(1, 2.0);
    EXPECT_TRUE(true);
}

TEST(SignalTest, OperatorCallWorks) {
    Signal<std::string> signal;
    std::string received;
    
    auto handle = signal.connect([&received](const std::string& s) {
        received = s;
    });
    
    signal("hello");
    
    EXPECT_EQ(received, "hello");
}

TEST(SignalTest, MultipleArguments) {
    Signal<int, double, std::string> signal;
    int a = 0;
    double b = 0.0;
    std::string c;
    
    auto handle = signal.connect([&](int x, double y, const std::string& z) {
        a = x; b = y; c = z;
    });
    
    signal.fire(1, 2.5, "test");
    
    EXPECT_EQ(a, 1);
    EXPECT_DOUBLE_EQ(b, 2.5);
    EXPECT_EQ(c, "test");
}

TEST(SignalTest, VoidSignal) {
    Signal<> signal;
    bool called = false;
    
    auto handle = signal.connect([&called]() {
        called = true;
    });
    
    signal.fire();
    
    EXPECT_TRUE(called);
}

TEST(ConnectionHandleTest, MoveSemantics) {
    Signal<int> signal;
    int count = 0;
    
    auto handle1 = signal.connect([&count](int) { count++; });
    auto handle2 = std::move(handle1);
    
    signal.fire(1);
    EXPECT_EQ(count, 1);
    
    handle2.disconnect();
    signal.fire(2);
    EXPECT_EQ(count, 1); 
}

TEST(ConnectionHandleTest, AutoDisconnectOnDestruction) {
    Signal<int> signal;
    int count = 0;
    
    {
        auto handle = signal.connect([&count](int) { count++; });
        signal.fire(1);
        EXPECT_EQ(count, 1);
    }  
    
    signal.fire(2);
    EXPECT_EQ(count, 1);
}

TEST(ConnectionHandleTest, DisconnectTwiceIsSafe) {
    Signal<> signal;
    
    auto handle = signal.connect([]() {});
    
    handle.disconnect();
    handle.disconnect(); 
    
    EXPECT_TRUE(true);
}

TEST(ConnectionHandleTest, DefaultConstructorIsSafe) {
    ConnectionHandle handle;
    
    handle.disconnect();  
    
    EXPECT_TRUE(true);
}

TEST(ObservableValueTest, GetInitialValue) {
    ObservableValue<int> value(42);
    
    EXPECT_EQ(value.get(), 42);
}

TEST(ObservableValueTest, SetUpdatesValue) {
    ObservableValue<int> value(0);
    
    value.set(100);
    
    EXPECT_EQ(value.get(), 100);
}

TEST(ObservableValueTest, OnChangeCallback) {
    ObservableValue<int> value(0);
    int received = 0;
    
    auto handle = value.on_change([&received](const int& val) {
        received = val;
    });
    
    value.set(42);
    
    EXPECT_EQ(received, 42);
}

TEST(ObservableValueTest, NoCallbackOnSameValue) {
    ObservableValue<int> value(5);
    int callback_count = 0;
    
    auto handle = value.on_change([&callback_count](const int&) {
        callback_count++;
    });
    
    value.set(5);  
    
    EXPECT_EQ(callback_count, 0);
}

TEST(ObservableValueTest, ImplicitConversion) {
    ObservableValue<int> value(42);
    
    int x = value; 
    
    EXPECT_EQ(x, 42);
}

TEST(ObservableValueTest, AssignmentOperator) {
    ObservableValue<int> value(0);
    
    value = 100;
    
    EXPECT_EQ(value.get(), 100);
}

TEST(ObservableValueTest, StringValue) {
    ObservableValue<std::string> value("initial");
    std::string received;
    
    auto handle = value.on_change([&received](const std::string& s) {
        received = s;
    });
    
    value.set("updated");
    
    EXPECT_EQ(received, "updated");
    EXPECT_EQ(value.get(), "updated");
}

TEST(ObservableValueTest, MultipleObservers) {
    ObservableValue<int> value(0);
    int count = 0;
    
    auto h1 = value.on_change([&count](const int&) { count++; });
    auto h2 = value.on_change([&count](const int&) { count++; });
    
    value.set(1);
    
    EXPECT_EQ(count, 2);
}

TEST(SignalThreadTest, ConcurrentFire) {
    Signal<int> signal;
    std::atomic<int> total{0};
    
    auto handle = signal.connect([&total](int val) {
        total += val;
    });
    
    std::thread t1([&signal]() {
        for (int i = 0; i < 100; ++i) {
            signal.fire(1);
        }
    });
    
    std::thread t2([&signal]() {
        for (int i = 0; i < 100; ++i) {
            signal.fire(1);
        }
    });
    
    t1.join();
    t2.join();
    
    EXPECT_EQ(total.load(), 200);
}

TEST(ObservableValueThreadTest, ConcurrentSet) {
    ObservableValue<int> value(0);
    std::atomic<int> callback_count{0};
    
    auto handle = value.on_change([&callback_count](const int&) {
        callback_count++;
    });
    
    std::thread t1([&value]() {
        for (int i = 0; i < 50; ++i) {
            value.set(i);
        }
    });
    
    std::thread t2([&value]() {
        for (int i = 50; i < 100; ++i) {
            value.set(i);
        }
    });
    
    t1.join();
    t2.join();
    
    EXPECT_GE(callback_count.load(), 0);
}

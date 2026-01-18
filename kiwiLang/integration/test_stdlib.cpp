
#include "kiwiLang/StdLib/Runtime.h"
#include "kiwiLang/StdLib/IO.h"
#include "kiwiLang/StdLib/Math.h"
#include "kiwiLang/StdLib/String.h"
#include "kiwiLang/StdLib/Array.h"
#include "kiwiLang/StdLib/Map.h"
#include "kiwiLang/StdLib/Time.h"
#include "kiwiLang/StdLib/Thread.h"
#include "kiwiLang/StdLib/File.h"
#include "kiwiLang/StdLib/Network.h"
#include "kiwiLang/StdLib/Regex.h"
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <chrono>
#include <fstream>

using namespace kiwiLang;

class StdLibTest : public ::testing::Test {
protected:
    void SetUp() override {
        runtime_ = std::make_unique<Runtime>();
        runtime_->initialize();
    }
    
    void TearDown() override {
        runtime_->shutdown();
    }
    
    std::unique_ptr<Runtime> runtime_;
};

TEST_F(StdLibTest, IOBasicOperations) {
    IO io;
    
    // Test string output
    testing::internal::CaptureStdout();
    io.print("Hello");
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "Hello");
    
    testing::internal::CaptureStdout();
    io.println("World");
    output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "World\n");
    
    // Test formatted output
    testing::internal::CaptureStdout();
    io.printf("Value: %d, String: %s\n", 42, "test");
    output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, "Value: 42, String: test\n");
}

TEST_F(StdLibTest, StringOperations) {
    String str1("Hello");
    String str2("World");
    
    // Test concatenation
    String result = str1 + " " + str2;
    EXPECT_EQ(result, "Hello World");
    
    // Test substring
    String substr = str1.substring(1, 3);
    EXPECT_EQ(substr, "el");
    
    // Test find
    std::size_t pos = str1.find("ell");
    EXPECT_EQ(pos, 1);
    
    pos = str1.find("xyz");
    EXPECT_EQ(pos, String::npos);
    
    // Test replace
    String replaced = str1.replace("ll", "LL");
    EXPECT_EQ(replaced, "HeLLo");
    
    // Test split
    String csv("a,b,c,d");
    auto parts = csv.split(",");
    EXPECT_EQ(parts.size(), 4);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[3], "d");
    
    // Test trim
    String spaced("  test  ");
    spaced.trim();
    EXPECT_EQ(spaced, "test");
    
    // Test case conversion
    String mixed("Hello World");
    EXPECT_EQ(mixed.toUpper(), "HELLO WORLD");
    EXPECT_EQ(mixed.toLower(), "hello world");
}

TEST_F(StdLibTest, MathOperations) {
    Math math;
    
    // Test basic functions
    EXPECT_DOUBLE_EQ(math.abs(-5.0), 5.0);
    EXPECT_DOUBLE_EQ(math.sqrt(16.0), 4.0);
    EXPECT_DOUBLE_EQ(math.pow(2.0, 3.0), 8.0);
    EXPECT_DOUBLE_EQ(math.exp(1.0), M_E);
    EXPECT_DOUBLE_EQ(math.log(M_E), 1.0);
    
    // Test trigonometric functions
    EXPECT_DOUBLE_EQ(math.sin(0.0), 0.0);
    EXPECT_DOUBLE_EQ(math.cos(0.0), 1.0);
    EXPECT_DOUBLE_EQ(math.tan(0.0), 0.0);
    
    // Test rounding
    EXPECT_DOUBLE_EQ(math.floor(3.7), 3.0);
    EXPECT_DOUBLE_EQ(math.ceil(3.2), 4.0);
    EXPECT_DOUBLE_EQ(math.round(3.5), 4.0);
    
    // Test min/max
    EXPECT_EQ(math.min(5, 3), 3);
    EXPECT_EQ(math.max(5, 3), 5);
    
    // Test random
    for (int i = 0; i < 100; ++i) {
        double r = math.random();
        EXPECT_GE(r, 0.0);
        EXPECT_LE(r, 1.0);
    }
    
    // Test random range
    for (int i = 0; i < 100; ++i) {
        int r = math.randomInt(10, 20);
        EXPECT_GE(r, 10);
        EXPECT_LE(r, 20);
    }
}

TEST_F(StdLibTest, ArrayOperations) {
    Array<int> arr;
    
    // Test push/pop
    arr.push(1);
    arr.push(2);
    arr.push(3);
    
    EXPECT_EQ(arr.size(), 3);
    EXPECT_EQ(arr[0], 1);
    EXPECT_EQ(arr[2], 3);
    
    EXPECT_EQ(arr.pop(), 3);
    EXPECT_EQ(arr.size(), 2);
    
    // Test insert/remove
    arr.insert(1, 99);
    EXPECT_EQ(arr[1], 99);
    EXPECT_EQ(arr.size(), 3);
    
    arr.remove(1);
    EXPECT_EQ(arr[1], 2);
    EXPECT_EQ(arr.size(), 2);
    
    // Test find
    arr.push(42);
    std::size_t pos = arr.find(42);
    EXPECT_EQ(pos, 2);
    
    pos = arr.find(999);
    EXPECT_EQ(pos, Array<int>::npos);
    
    // Test iteration
    int sum = 0;
    for (auto& val : arr) {
        sum += val;
    }
    EXPECT_EQ(sum, 1 + 2 + 42);
    
    // Test sort
    Array<int> unsorted = {5, 2, 8, 1, 9};
    unsorted.sort();
    
    EXPECT_EQ(unsorted[0], 1);
    EXPECT_EQ(unsorted[1], 2);
    EXPECT_EQ(unsorted[2], 5);
    EXPECT_EQ(unsorted[3], 8);
    EXPECT_EQ(unsorted[4], 9);
    
    // Test reverse
    Array<int> forward = {1, 2, 3, 4, 5};
    forward.reverse();
    
    EXPECT_EQ(forward[0], 5);
    EXPECT_EQ(forward[4], 1);
}

TEST_F(StdLibTest, MapOperations) {
    Map<std::string, int> map;
    
    // Test insert
    map.insert("one", 1);
    map.insert("two", 2);
    map.insert("three", 3);
    
    EXPECT_EQ(map.size(), 3);
    EXPECT_TRUE(map.contains("one"));
    EXPECT_FALSE(map.contains("four"));
    
    // Test access
    EXPECT_EQ(map["one"], 1);
    EXPECT_EQ(map.get("two"), 2);
    
    // Test update
    map["one"] = 11;
    EXPECT_EQ(map["one"], 11);
    
    // Test remove
    map.remove("two");
    EXPECT_EQ(map.size(), 2);
    EXPECT_FALSE(map.contains("two"));
    
    // Test iteration
    int sum = 0;
    for (const auto& [key, value] : map) {
        sum += value;
    }
    EXPECT_EQ(sum, 11 + 3);
    
    // Test clear
    map.clear();
    EXPECT_EQ(map.size(), 0);
    EXPECT_TRUE(map.empty());
}

TEST_F(StdLibTest, TimeOperations) {
    Time time;
    
    // Test current time
    auto now = time.now();
    EXPECT_GT(now, 0);
    
    // Test sleep
    auto start = time.now();
    time.sleep(100); // 100ms
    auto end = time.now();
    EXPECT_GE(end - start, 100);
    
    // Test date formatting
    DateTime dt = time.fromTimestamp(now);
    std::string formatted = time.format(dt, "%Y-%m-%d");
    EXPECT_EQ(formatted.length(), 10);
    EXPECT_EQ(formatted[4], '-');
    EXPECT_EQ(formatted[7], '-');
    
    // Test parsing
    DateTime parsed = time.parse("2023-01-15", "%Y-%m-%d");
    EXPECT_EQ(parsed.year, 2023);
    EXPECT_EQ(parsed.month, 1);
    EXPECT_EQ(parsed.day, 15);
}

TEST_F(StdLibTest, ThreadOperations) {
    Thread thread;
    
    std::atomic<int> counter{0};
    
    // Test thread creation and join
    auto func = [&counter]() {
        for (int i = 0; i < 1000; ++i) {
            ++counter;
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(func);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(counter, 10000);
    
    // Test mutex
    std::mutex mtx;
    int shared = 0;
    
    auto mutexFunc = [&mtx, &shared]() {
        std::lock_guard<std::mutex> lock(mtx);
        ++shared;
    };
    
    threads.clear();
    for (int i = 0; i < 1000; ++i) {
        threads.emplace_back(mutexFunc);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(shared, 1000);
    
    // Test condition variable
    std::condition_variable cv;
    bool ready = false;
    
    auto waiter = [&mtx, &cv, &ready]() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&ready]() { return ready; });
    };
    
    auto notifier = [&mtx, &cv, &ready]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        {
            std::lock_guard<std::mutex> lock(mtx);
            ready = true;
        }
        cv.notify_all();
    };
    
    auto startTime = std::chrono::steady_clock::now();
    
    std::thread t1(waiter);
    std::thread t2(notifier);
    
    t1.join();
    t2.join();
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_GE(duration.count(), 100);
}

TEST_F(StdLibTest, FileOperations) {
    File file;
    
    std::string testContent = "Hello, World!\nThis is a test file.\n";
    std::string filename = "test_file.txt";
    
    // Test write
    EXPECT_TRUE(file.writeAll(filename, testContent));
    
    // Test read
    std::string readContent;
    EXPECT_TRUE(file.readAll(filename, readContent));
    EXPECT_EQ(readContent, testContent);
    
    // Test append
    std::string appendContent = "Appended line.\n";
    EXPECT_TRUE(file.append(filename, appendContent));
    
    std::string fullContent;
    EXPECT_TRUE(file.readAll(filename, fullContent));
    EXPECT_EQ(fullContent, testContent + appendContent);
    
    // Test exists
    EXPECT_TRUE(file.exists(filename));
    EXPECT_FALSE(file.exists("non_existent_file.txt"));
    
    // Test size
    auto size = file.size(filename);
    EXPECT_EQ(size, fullContent.size());
    
    // Test copy
    std::string copyName = "test_file_copy.txt";
    EXPECT_TRUE(file.copy(filename, copyName));
    EXPECT_TRUE(file.exists(copyName));
    
    // Test move
    std::string moveName = "test_file_moved.txt";
    EXPECT_TRUE(file.move(copyName, moveName));
    EXPECT_FALSE(file.exists(copyName));
    EXPECT_TRUE(file.exists(moveName));
    
    // Test delete
    EXPECT_TRUE(file.remove(filename));
    EXPECT_TRUE(file.remove(moveName));
    EXPECT_FALSE(file.exists(filename));
    EXPECT_FALSE(file.exists(moveName));
    
    // Test directory operations
    std::string dirName = "test_dir";
    EXPECT_TRUE(file.createDirectory(dirName));
    EXPECT_TRUE(file.isDirectory(dirName));
    
    std::string nestedDir = dirName + "/subdir";
    EXPECT_TRUE(file.createDirectory(nestedDir));
    
    // Test list directory
    auto entries = file.listDirectory(dirName);
    EXPECT_GE(entries.size(), 1); // Should at least contain "subdir"
    
    EXPECT_TRUE(file.removeDirectory(dirName, true));
    EXPECT_FALSE(file.exists(dirName));
}

TEST_F(StdLibTest, NetworkOperations) {
    Network network;
    
    // Test DNS resolution
    auto addresses = network.resolve("localhost");
    EXPECT_FALSE(addresses.empty());
    
    // Test URL parsing
    URL url = network.parseURL("http://example.com:8080/path?query=value#fragment");
    
    EXPECT_EQ(url.scheme, "http");
    EXPECT_EQ(url.host, "example.com");
    EXPECT_EQ(url.port, 8080);
    EXPECT_EQ(url.path, "/path");
    EXPECT_EQ(url.query, "query=value");
    EXPECT_EQ(url.fragment, "fragment");
    
    // Test HTTP client (requires network, may fail in CI)
    HttpClient client;
    
    // Note: This test requires network access and may fail in isolated environments
    // Uncomment to test actual HTTP requests
    
    /*
    auto response = client.get("http://httpbin.org/get");
    if (response) {
        EXPECT_EQ(response->status, 200);
        EXPECT_TRUE(response->body.find("httpbin.org") != std::string::npos);
    }
    
    // Test POST request
    auto postResponse = client.post("http://httpbin.org/post", 
                                   "test=data", 
                                   {{"Content-Type", "application/x-www-form-urlencoded"}});
    if (postResponse) {
        EXPECT_EQ(postResponse->status, 200);
    }
    */
    
    // Test socket operations
    Socket socket;
    
    // Try to create a simple server socket (won't actually connect in test)
    bool created = socket.create(AF_INET, SOCK_STREAM, 0);
    EXPECT_TRUE(created);
    
    if (created) {
        // Set socket options
        EXPECT_TRUE(socket.setOption(SOL_SOCKET, SO_REUSEADDR, 1));
        
        // Bind to localhost:0 (let OS choose port)
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        
        bool bound = socket.bind((sockaddr*)&addr, sizeof(addr));
        EXPECT_TRUE(bound);
        
        socket.close();
    }
}

TEST_F(StdLibTest, RegexOperations) {
    Regex regex;
    
    // Test matching
    EXPECT_TRUE(regex.match("hello", "h.*o"));
    EXPECT_FALSE(regex.match("hello", "H.*o"));
    
    // Test case-insensitive matching
    Regex ciRegex("H.*O", Regex::CASE_INSENSITIVE);
    EXPECT_TRUE(ciRegex.match("hello"));
    
    // Test search
    std::string text = "The quick brown fox jumps over the lazy dog";
    auto match = regex.search("brown.*fox", text);
    EXPECT_TRUE(match);
    EXPECT_EQ(match->str(), "brown fox");
    
    // Test find all
    text = "cat bat sat fat";
    auto allMatches = regex.findall("[cb]at", text);
    EXPECT_EQ(allMatches.size(), 2);
    EXPECT_EQ(allMatches[0], "cat");
    EXPECT_EQ(allMatches[1], "bat");
    
    // Test replace
    text = "hello world";
    std::string replaced = regex.replace("world", "universe", text);
    EXPECT_EQ(replaced, "hello universe");
    
    // Test replace all
    text = "a1b2c3d4";
    replaced = regex.replaceAll("\\d", "", text);
    EXPECT_EQ(replaced, "abcd");
    
    // Test split
    text = "one,two,three,four";
    auto parts = regex.split(",", text);
    EXPECT_EQ(parts.size(), 4);
    EXPECT_EQ(parts[0], "one");
    EXPECT_EQ(parts[3], "four");
    
    // Test groups
    text = "John Doe, Age: 30";
    Regex groupRegex("(\\w+) (\\w+), Age: (\\d+)");
    auto groups = groupRegex.groups(text);
    
    EXPECT_EQ(groups.size(), 3);
    EXPECT_EQ(groups[0], "John");
    EXPECT_EQ(groups[1], "Doe");
    EXPECT_EQ(groups[2], "30");
}

TEST_F(StdLibTest, StdLibIntegration) {
    // Test integration between different stdlib components
    
    // String + File
    File file;
    String content = "Hello from String class!\nSecond line.";
    
    EXPECT_TRUE(file.writeAll("integration_test.txt", content.c_str()));
    
    String readContent;
    EXPECT_TRUE(file.readAll("integration_test.txt", readContent));
    EXPECT_EQ(readContent, content);
    
    // Array + String
    String csv = "apple,banana,cherry,date";
    auto fruits = csv.split(",");
    
    Array<String> fruitArray;
    for (const auto& fruit : fruits) {
        fruitArray.push(fruit.trim());
    }
    
    EXPECT_EQ(fruitArray.size(), 4);
    EXPECT_EQ(fruitArray[0], "apple");
    EXPECT_EQ(fruitArray[3], "date");
    
    // Map + Time
    Map<String, DateTime> schedule;
    
    Time time;
    DateTime now = time.nowAsDateTime();
    
    schedule.insert("meeting", now);
    schedule.insert("deadline", time.addDays(now, 7));
    
    EXPECT_TRUE(schedule.contains("meeting"));
    EXPECT_TRUE(schedule.contains("deadline"));
    
    // Thread + Network (simulated)
    std::atomic<int> completed{0};
    auto worker = [&completed]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++completed;
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(completed, 5);
    
    // Cleanup
    EXPECT_TRUE(file.remove("integration_test.txt"));
}

TEST_F(StdLibTest, PerformanceTest) {
    // Performance tests for critical operations
    
    const int ITERATIONS = 10000;
    
    // String concatenation performance
    auto start = std::chrono::high_resolution_clock::now();
    
    String result;
    for (int i = 0; i < ITERATIONS; ++i) {
        result += "test";
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 1000); // Should complete in under 1 second
    
    // Array push performance
    Array<int> arr;
    arr.reserve(ITERATIONS);
    
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        arr.push(i);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 100); // Should be very fast
    
    // Map insert performance
    Map<int, String> map;
    
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        map.insert(i, String("value_") + std::to_string(i));
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 500); // Should complete reasonably fast
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
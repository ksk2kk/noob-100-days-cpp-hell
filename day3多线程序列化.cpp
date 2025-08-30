#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <cstring>
#include <iomanip>
#include <atomic>
#include <thread>
#include <future>
#include <mutex>
#include <cassert>
#include <immintrin.h>  // AVX2  SIMD 
#include <chrono>       // 计算时间

//缓冲区 装满了就要搬走了 很满就会使用avx2 满了就要爆炸了哥们
class BufferedStream {
public:
    BufferedStream(size_t buffer_size = 8192)
        : buffer_(new char[buffer_size]), buffer_size_(buffer_size), pos_(0) {
    }//pos这个地方放了多少数据

    ~BufferedStream() {
        if (buffer_) delete[] buffer_;
        //buffer_放数据(实际内存）
    }

    // 将数据写入缓冲区，批量写入  AVX2
    void write(const void* data, size_t size) {
        size_t remaining = size;//剩余数量
        const char* p = static_cast<const char*>(data);//static_cast用于编译时类型转换，节约运行开销

        // 使用 AVX2 或其他SIMD指令加速写
        while (remaining > 0) {
            size_t space_left = buffer_size_ - pos_;//剩余空闲位置 pos被初始化为0 防止坠机
            size_t to_copy = std::min(remaining, space_left);//左边剩余数量 右边剩余空间  stdmin会肘击最小值出来比较使用

            if (to_copy >= 32) {  // 使用 AVX2 就像卡车一样创
                write_avx2(p, to_copy);
            }
            else {
                std::memcpy(buffer_ + pos_, p, to_copy);//手动//memcpy把内存块从一个地方复制到另一个地方
                pos_ += to_copy;
            }

            remaining -= to_copy;
            p += to_copy;

            if (pos_ == buffer_size_) {//满了就搬走
                flush();//清空
            }
        }
    }

    // 刷新缓冲区到控制台
    void flush() {
        for (size_t i = 0; i < pos_; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (0xFF & buffer_[i]) << " ";
        }
        std::cout << std::dec << std::endl;
        pos_ = 0;
    }

    // 利用 AVX2 批量
    void write_avx2(const char* data, size_t size) {
        // 对齐到 32 字节
        size_t aligned_size = (size / 32) * 32;  // 取出最接近32的倍数

        if (aligned_size > 0) {
            __m256i* p = reinterpret_cast<__m256i*>(buffer_ + pos_);
            const __m256i* source = reinterpret_cast<const __m256i*>(data);

            size_t num_chunks = aligned_size / 32;
            for (size_t i = 0; i < num_chunks; ++i) {
                p[i] = source[i];//搬到这
            }
            pos_ += aligned_size;
        }

        // 处理剩余不足32字节的数据
        size_t remaining = size - aligned_size;
        if (remaining > 0) {
            std::memcpy(buffer_ + pos_, data + aligned_size, remaining);
            pos_ += remaining;
        }
    }

private:
    char* buffer_;
    size_t buffer_size_;
    size_t pos_;
};

// 序列化接口
class ISerializable {
public:
    virtual void serialize(BufferedStream& out) const = 0;
};

//序列化工具
class Serializer {
public:
    // 基本类型序列化方法
    static void serialize(BufferedStream& out, const int& value) {
        out.write(&value, sizeof(value));//整数就放进去
    }

    static void serialize(BufferedStream& out, const double& value) {
        out.write(&value, sizeof(value));//放进去
    }

    static void serialize(BufferedStream& out, const std::string& value) {
        size_t len = value.size();
        out.write(&len, sizeof(len));  // 写下字符串长度
        out.write(value.data(), len);  // 写入字符串内容
    }

    // 自定义类型的序列化
    template <typename T>
    static void serialize(BufferedStream& out, const T& value) {
        value.serialize(out);//复杂的数据
    }

    // 注册类型的方法，不再使用模板递归调用，用了就不吭声，不知道编译器抽风还是怎么地，判断为两个参数就不给过
    template <typename T>
    static void register_type() {
        // 注册每种类型的序列化函数
    }

private:
    static std::unordered_map<size_t, std::function<void(BufferedStream&, const void*)>> registry_;
};

//  实现用户
struct User : public ISerializable {
    int id;
    double score;

    // 提供一个接受 int 和 double 的构造函数
    User(int id, double score) : id(id), score(score) {}

    // 自定义序列
    void serialize(BufferedStream& out) const override {
        out.write(&id, sizeof(id));
        out.write(&score, sizeof(score));
    }
};

// 并发写入
std::mutex write_mutex;

// 多线程执行序列化，提高吞吐
void parallel_serialize(BufferedStream& out, const std::vector<std::string>& data) {
    std::vector<std::future<void>> futures;

    for (const auto& item : data) {
        //多线程
        futures.push_back(std::async(std::launch::async, [&out, &item]() {
            Serializer::serialize(out, item);
            }));
    }

    // 等待所有任务
    for (auto& future : futures) {
        future.get();
    }
}

// 性能测试
void run_performance_test() {
    // 创建缓冲区
    BufferedStream out(1024 * 1024);  // 1MB 

    // 生成大量数据用于测试
    std::vector<std::string> large_data;
    for (size_t i = 0; i < 10000000; ++i) {  // 大约 10MB
        large_data.push_back("wocaonihaobang.");//
    }

    // 性能计时
    auto start_time = std::chrono::high_resolution_clock::now();

    // 并发序列化测试
    parallel_serialize(out, large_data);

    // 获取结束时间
    auto end_time = std::chrono::high_resolution_clock::now();//开始计算时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // 计算吞吐量
    size_t total_data_size = large_data.size() * sizeof(std::string::value_type) * 42;  // 每条字符串大约42字节
    double throughput = (total_data_size / 1024.0 / 1024.0 / 1024.0) / (duration / 1000.0);  // GB/s

    std::cout << "处理的总数据量: " << total_data_size / 1024.0 / 1024.0 / 1024.0 << " GB\n";
    std::cout << "你小子用了这么多时间: " << duration << " ms\n";
    std::cout << "吞吐: " << throughput << " GB/s\n";
}

//主函数
int main() {
    std::cout << "准备测试了...\n";
    run_performance_test();
    return 0;
}

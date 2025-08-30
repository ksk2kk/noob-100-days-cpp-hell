#include <iostream>
#include <vector>
#include <memory>
#include <functional>
#include <type_traits>
#include <algorithm>
#include <chrono>
#include <thread>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <any>
#include <variant>
#include <optional>
#include <span>
#include <ranges>
#include <concepts>
#include <new>
#include <cstring>

namespace banana_jungle {
	//concept 只有能唱歌的香蕉才能当主角
	//concept声明 编译的门禁 只有满足条件的类型才可以通过
    //static_assert(Singable<NormalBanana>);//通过
	//static_assert(!Singable<int>);//不能通过
	template<typename T>
	concept Singable = requires(T t)
		//声明一个叫 Singable 的门禁卡
	{
		{ t.sing() }->std::convertible_to<int>;//要求返回值可以转换为int
		//{}是要求 只要t有成员函数sing 并返回值可以转换为int

	};
	//空类 占位测试sizeof
	//std::cout<<sizeof(Empty)<<std::endl;

	class EmptyBanana {};

	//CRTP 基类用派生类型做模板参数 实现静态堕胎
	//NormalBanana nb;nb.whoami();//编译期
	template<typename Derived>
	//derived是泛型形参 待会就会替换成具体的香蕉
	class BananaBase {//香蕉是公共接口 是一个基类 香蕉要带上这个接口
	public:
		void whoami() const {
			static_cast<const Derived*>(this)->sing();//自我介绍函数

		}
	};
	//继承+explicit 单参构造
	//禁用隐式转换
	//NormalBanana nb{5};//ok
	//NormalBanana nb=5;//error

	class NormalBanana :public BananaBase<NormalBanana>, public EmptyBanana {
	public:
		int price = 0;
		NormalBanana() = default;//补充默认构造。下面要用
		explicit NormalBanana(int p) :price(p) {}//explict禁止隐式转换 比如偷偷变成了对象
		int sing()const {
			return price;

		}
	};
	//继承和数组 测试对齐大小
	//std::cout<<sizeof(FANCYBANANA)<<std::endl; //字节40+

	class FancyBanana :public BananaBase<FancyBanana> {
	public:
		int price = 0;
		char name[32] = "Fancy";
		explicit FancyBanana(int p) :price(p) {}
		int sing()const
		{
			return price * 2;
		}
	};

	//闭包大小 静态反射对齐位阈
	//alignas强制对齐
	//alignas(64)int x;//x的地址必须是64背书

	struct alignas(32) BigBanana {
		int price;
		int flag;
		char : 0;//位阈占位

	};
	//sizeof /alignof 编译 量大小和对齐
	//std::cout<<sizeof(bigbanana)<<alignof(bigbanana);
	template<typename T>
	constexpr void print_size() {
		std::cout << "笼子大小" << sizeof(T) << "对齐" << alignof(T) << std::endl;//编译的适合算
	}
	//悬挂
	//lambda[&]捕获引用 悬挂
	//引用外部变量 离开作用域引用失效 
	//int x =42;auto l=[&x]{return x;}; x=0; l();//读到0
	void dangling_trap()
	{
		int temp = 42;
		//如果只拿引用 temp离开作用域 引用失效
		//auto bad=[&] {return temp;};//挂了
		auto good = [value = temp] {return value; };//值捕获
		std::cout <<"正确捕获" << good() << std::endl;


	}
	//lambda [=]拷贝
	//深拷贝外部变量
	//std::vector<int> v{1,2,3}; auto l [v]{return v.size();};
	void copy_move_trap()
	{
		std::vector<int> big_vec(1000, 7);
		//给箱子里做1000个大香蕉 每根都是七块钱
		//1000个int深拷贝
		auto copy_lambda = [big_vec]() {return big_vec.size(); };
		//猴子把1000给大香蕉抄了一份放进自己的闭包里
		//包里有1000个大香蕉 和仓库那份不共享
		
		auto move_lambda = [v = std::move(big_vec)]()mutable
			{
				v.push_back(42);
				return v.size();
			};
		//不抄香蕉 直接把仓库打包带走
		//mutable允许修改包内成员 所以猴子可以塞进去一个42元香蕉 现在包里只有1001个
		std::cout << "拷贝后大小:" << copy_lambda() << "\n";
		std::cout << "移动后大小：" << move_lambda() << '\n';
		
	}
	//lambda mutable
	// 允许修改包内成员
	// 
	//int x=5;auto l=[x]()mutable{++x;return x;};
	void mutable_trap()
	{
		int price = 5;
		//仓库里面有一根五元香蕉
		auto const_lambda = [price]() {return price; };
		//猴子把五元香蕉抄了一份放进包里 不允许涨价
		auto mutable_lambda = [price]()mutable {
			++price; return price;
			};
		//允许涨价的猴子 问一次贵一块钱
		std::cout << "第一次" << mutable_lambda() << "\n";
		std::cout << "第二次" << mutable_lambda() << "\n";
		std::cout << "原始值" << price << "\n";

	}
	//lambda 泛型 转发
	//decltype(auto)完美返回 保持值类别
	//示例
	// decltype(auto)f(int&& x){return std::forward<int>(x);}
	template<typename T>
	decltype(auto)forward_banana(T&& t)
	{
		return [t = std::forward<T>(t)]()mutable {
			return t.sing();
			};
			
	}

	//折叠表达式
	//ex return (0+...+vec);
	template<typename...Bananas>
	constexpr int total_price(Bananas&&...bs) {
		return (0 + ... + bs.sing());
	}
	//并发 
	//std::thread std::mutex
	//多线程 互斥
	//std::mutex m;std::lock_guard lg(m);
	void concurrency_trap()
	{
		std::mutex mtx;//立一把锁
		int shard_price = 0;//共享价格 现在是0
		auto worker = [&](int add) {//万能猴子拿到了这个add 准备写在价格上

			std::lock_guard<std::mutex>lock(mtx);//猴子进门先拿钥匙 lock_guard会自动解锁，当你离开作用域就自动上锁
			//不会忘记锁 也不死锁
			shard_price += add;

			};
		std::thread t1(worker, 10);//同时跑
		std::thread t2(worker, 20);
		t1.join(); t2.join();//等猴子干活
		std::cout << "并发价格" << shard_price << std::endl;
	}
	//可变模板加任意lambda
	//(fs(),...)折叠
	//顺序执行任意可调用
	//cook_all([]{std::cout<<"a";},[]{std::cout<<"b";});
	template<typename...F>
	void cook_all(F&&...fs)
	{
		(fs(), ...);
	}
	//sizeof (decltype(lambda))
	//编译期间量闭包
	//sizeof([]{})
	void closure_size_experiment()
	{
		int a = 1;
		double b = 2.0;
		char c = 3;
		//仓库里放一根一元香蕉Int 1根两元香蕉double 一根三元香蕉 char

		auto e1 = [] {};//空书包猴子
		auto e2 = [a] {//塞进了一根香蕉的猴子
			return a;
			};
		auto e3 = [a, b] {};//塞进了两根香蕉的猴子
		auto e4 = [a, b, c] {}; //三根香蕉猴子
		auto e5 = [&, a, b, c] {};//混合捕获 引用捕获外部变量  和显示拷贝三根香蕉
		auto e6 = [vec = std::vector<int>(100)] {};//不变胖 但是把100个大香蕉打包带走 香蕉在堆
		
		print_size<decltype(e1)>();
		print_size<decltype(e2)>();
		print_size<decltype(e3)>();
		print_size<decltype(e4)>();
		print_size<decltype(e5)>();
		print_size<decltype(e6)>();


	}
	//ranges过滤lambda
	//std::views::filter+transform
	//管道算法
	//auto v=vec|views::filter(...)|views::transform(...);
	void ranges_lambda()
	{
		std::vector<NormalBanana>bananas{NormalBanana{1},NormalBanana{2},NormalBanana{3},NormalBanana{4},NormalBanana{5} };
		//告诉编译器用单参构造 因为我这里坠机了

		//std::vector<NormalBanana>bananas{1,2,3,4,5};//error

		//过滤价格大于2的香蕉 价格*10
		auto big = bananas
			| std::views::filter([](const auto& b) {return b.price > 2; })
			| std::views::transform([](const auto& b) {return b.price * 10; });
		for (int v:big)std::cout<<"大香蕉价值"<<v;

	}
}//namespace banana_jungle

int main()
{
	using namespace banana_jungle;
	std::cout << "悬挂引用坠机测试" << std::endl;
	dangling_trap();
	//坠机了孩子们

	std::cout << "拷贝和移动" << std::endl;
	copy_move_trap();

	std::cout << "mutable坠机" << std::endl;
	mutable_trap();

	std::cout << "折叠" << std::endl;
	std::cout << "总价" << total_price(NormalBanana{ 1 }, NormalBanana{ 2 }, FancyBanana{ 3 }) << "\n";

	std::cout << "并发" << std::endl;
	concurrency_trap();

	std::cout << "万能" << std::endl;
	cook_all([] {std::cout << "锅1\n"; },
		[] {std::cout << "锅2\n"; },
		[] {std::cout << "锅3\n"; });

	printf("闭包大小\n");
	closure_size_experiment();

	printf("ranges\n");
	ranges_lambda();
	return 0;
}

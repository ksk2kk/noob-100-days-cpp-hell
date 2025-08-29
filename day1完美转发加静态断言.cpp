#include <iostream>
#include <tuple>
#include <type_traits>
#include <utility>
#include <sstream>
namespace monkey {
	//编译器反射
	template <typename T>
	struct type_info {
		static constexpr const char* name = "神秘香蕉";
		//类模板定义 泛型形参T
		//提供一个默认的实现 ，所有没有特化的类型都输出神秘香蕉
		//充当兜底

	};
	template <>struct type_info<int> { static constexpr const char* name = "整形香蕉"; };
	//全特化 当T精准匹配Int 编译器使用这个
	//让int在编译器就使用整形香蕉这个名字 而不是神秘香蕉

	template<>struct type_info<double> { static constexpr const char* name = "小数香蕉"; };
	//double使用小数香蕉 专属昵称
	//type_name_v<double>  --> 小数香蕉
	
	template<>struct type_info<const char*> { static constexpr const char* name = "字符香蕉"; };
	//tuple_to_string const char*  --> 字符香蕉

	template<typename T>
	inline constexpr const char* type_name_v = type_info<T>::name;
	//变量模板 + constexpr
	//constexpr 让变量在编译期就能计算出来
	//语法糖 省了写typeinfo<T>::name
	//直接写type_name_v<int>  整形香蕉

	//检查类型是否能被cout
	//template<typename T, typename = void> struct can_print : std::false_type {}
	//类模板 第二个形参有默认实参数 void
	//默认情况认为不能打印
	//这是SFINAE的失败分支

	template <typename T, typename = void>struct can_print :std::false_type {};

	//主模板特化 
	//再次声明同名模板 但是第二个形参使用std::void_t 做SFINAE
	//当表达式std::cout<<std::declval<T>（）合法的时候 才会被选中
	//借助void_t的只关心是否存在 不关心返回值特性实现编译检查

	template<typename T> 
	struct can_print<T,std::void_t<decltype(std::cout<<std::declval<T>())>>:std::true_type{};

	//变量模板再包装
	//省了写can_print<T>::value
	//编译期布尔值 直接用于static_assert 和if constexpr

	template<typename T>
	inline constexpr bool can_print_v = can_print<T>::value;

	//元组转字符串 折叠表达式
	template<typename Tuple,std::size_t...I>
	auto tuple_to_string_impl(const Tuple& tp, std::index_sequence<I...>) {
		std::ostringstream oss;//字符串流  拼接
		//折叠表达式 逗号运算符和参数包展开
		//对每个I执行oss<<(I?",":"")<<type_name_v<...>
		//I==0的时候 不加逗号 其余加逗号 最后得到 整形香蕉 小数香蕉 字符串香蕉
		((oss << (I ? "," : "") << type_name_v< std::tuple_element_t<I, Tuple>>), ...);
		return oss.str();

	}
	//用户层包装 省得我每次手写 index_sequence
	template<typename Tuple>
	auto tuple_to_string(const Tuple& tp) {
		return tuple_to_string_impl(
			tp, std::make_index_sequence<std::tuple_size_v<Tuple>>{}
		//0~N-1
		);
	}

}//namespace monkey

//打印任意参数包
//可变参数模板和完美转发形参
//接收任意数量和类型的实参 全部转发给cout
//折叠表达式加静态断言
template<typename...Ts>
//
void show_pack(Ts&& ...args) {
	//静态断言 折叠表达式展开成为canprintv<T0>.....
	//只要有一个类型不支持operator<<就报错
	//提前编译器发现错误给出信息
	static_assert((monkey::can_print_v<std::remove_reference_t<Ts>>&&...), "所有参数必须支持operator");

	//折叠表达式加分隔符
	//对每个实参执行cout<<forward<t>(arg)<<
	//实战 参数包展开从左到右 保证输出顺序一致

	((std::cout << std::forward<Ts>(args) << (sizeof...(Ts) > 1 ? "|" : "")), ...);

}
int main() {
	//类型别名声明
	//定义一个含三个类型的tuple

	using MyTuple = std::tuple<int, double, const char*>;

	//static_assert 
	//编译期断言 tuple长度必须等于3
	//防止以后手滑改tuple 导致逻辑错误

	static_assert(std::tuple_size_v <MyTuple> == 3, "必须是三个类型");

	//tuple实例
	MyTuple tp{ 42,3.14,"hello" };

	//打印类型族谱
	std::cout << "tuple fimaly" << monkey::tuple_to_string(tp) << std::endl;
	//打印实际数值
	show_pack(100, 2.718, "world");


	//搞完了 睡觉

}

# 高并发内存池(tcmalloc-mini)

原项目：[google-tcmalloc](https://github.com/google/tcmalloc)

---

# 项目介绍

> 这是一个CPP实现的高并发内存池，原型是谷歌的tcmalloc开源项目（Thread-Caching Malloc），既线程缓存的malloc，实现了高效的多线程内存管理，用于替代系统的内存分配相关函数（malloc、free）


# 前置需求

+ C/C++
+ 数据结构（链表、哈希桶）
+ 操作系统内存管理
+ 单例模式
+ 多线程
+ 互斥锁
+ ....


# 项目文档

[高并发内存池序章](https://atong.run/2023/01/29/%E9%A1%B9%E7%9B%AE/%E9%AB%98%E5%B9%B6%E5%8F%91%E5%86%85%E5%AD%98%E6%B1%A0/%E9%AB%98%E5%B9%B6%E5%8F%91%E5%86%85%E5%AD%98%E6%B1%A0-%E5%BA%8F%E7%AB%A0/)
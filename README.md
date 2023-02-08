# 高并发内存池(tcmalloc-mini)

原项目：[google-tcmalloc](https://github.com/google/tcmalloc)

---

# 项目介绍

> 这是一个CPP实现的高并发内存池，原型是谷歌的tcmalloc开源项目（Thread-Caching Malloc），既线程缓存的malloc，实现了高效的多线程内存管理，用于替代系统的内存分配相关函数（malloc、free）, 在多线程高并发的场景下更胜一筹。


# 前置需求

+ C/C++
+ 数据结构（链表、哈希桶）
+ 操作系统内存管理
+ 单例模式
+ 多线程
+ 互斥锁
+ ....


# 项目文档

> tcmalloc-mini文档，访问速度可能有点慢

[一、高并发内存池-序章](https://atong.run/posts/4062989894/)

[二、高并发内存池-预备](https://atong.run/posts/594127029/)

[三、高并发内存池-框架](https://atong.run/posts/1235732513/)

[四、高并发内存池-ThreadCache](https://atong.run/posts/1785806469/)

[五、高并发内存池-CentralCache](https://atong.run/posts/1806624746/)
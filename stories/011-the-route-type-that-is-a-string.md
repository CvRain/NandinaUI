# 分组的类型是一个字符串：拼错就多一段

> 状态：分组已落地并测试；`type` 的类型安全待决，见
> [待决设计草案](../docs/references/reference_draft.md)。

侧边栏要按 `RouteOptions::type` 分段（作者给的参考图是 BUTTON / DATA / PANEL 那种小标题分组）。
作者的初始思路是 `unordered_map`，key 是 type、value 是条目数组，分块构造再拼接。我没按这个写，
因为它在**顺序**上有个不容易看出来的坑。

## 一、为什么不用 unordered_map

哈希表的迭代顺序是实现细节、未指定。导航顺序是用户直接看得见的，用它分组再拼接等于把顺序
交给标准库实现：换一次实现、加一条路由，段的顺序就可能变。

我改成有序单遍扫描，一次同时得到"分组"和"顺序"：

```cpp
for (const auto* entry: nav_entries()) {
    const std::string_view type {entry->options.type};
    const auto found = std::ranges::find(sections, type, &NavSection::type);  // 投影查找
    if (found != sections.end()) {
        found->entries.push_back(entry);
        continue;
    }
    sections.push_back(NavSection {.type = type, .entries = {entry}});
}
```

**分段顺序 = 各 type 首次出现的顺序**，也就是路由表的声明顺序；要调顺序就把同一 type 的路由
写在声明处。段数是个位数，线性扫描比哈希表更快，而且读者一眼能看出顺序由什么决定。

这个判断没有停在"我觉得"上 —— 我按作者原本的思路做了故障注入：

```text
注入 unordered_map 版本 → router_tests 立刻红：
  REQUIRE( sections[0].type == "面板" )
  with expansion:  "default" == "面板"      ← 顺序真的变了
```

测试还锁了四件容易做错的事：交错的同类条目要合并回同一段、段内保持路由表顺序、每段的条目
恰好是 `nav_entries()` 的一次划分（不重不漏）、同一份表两次分组结果一致（顺序不能是"这次恰好
这样"）。空表和"全都不可导航"的表都得到空分段，而不是一段空标题。

## 二、放在框架而不是 showcase

分组逻辑进了 `Routes::nav_sections()`，紧挨已有的 `nav_entries()`。理由和那个方法当初进去时
一样：`nav_entries()` 已经收掉了"哪些条目进导航"的规则（`show_in_nav` + `activate != nullptr`
两个条件），"怎么分组"是同一类规则，收在框架里才有测试可写、也不会被每个消费者各写一遍。

## 三、留下的是类型问题

`RouteOptions::type` 是作者加的 `std::string type{"default"}`。它能用，但拼错一个字母不会报错 ——
只会在导航里**多出一个带错字的分组**。按项目的分类这属于"静默失效"的近亲：可见，但没有任何
东西告诉你写错了（对比：如果换成枚举，`"buton"` 直接编译不过）。

当前的分组实现对这个行为是**中立**的：它如实按字符串分组，不猜、不归一化。要收紧就得改类型，
而那会影响路由表的作者 API，所以留成待决项。

## 四、待决

1. 换成枚举（编译期拦住拼写错误），还是保留字符串 + 增加校验/白名单；
2. 若是枚举，第三方或扩展分组怎么容纳（枚举 + 自定义标签？固定集合？）；
3. 分组顺序是否要一个显式声明处（现在是"首次出现顺序"，靠声明位置隐含表达）。

细节见 [待决设计草案](../docs/references/reference_draft.md)。

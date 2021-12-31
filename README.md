# OmniSketch

## Getting Started

获取源代码：

```shell
git clone git@dl1:Ferric/SketchLab_CPP.git
```

配置并编译：

```shell
cd OmniSketch
cd build
cmake .. && make
```

为执行对算法的准确性 /
性能测试，需要使用已精简的数据集，可于 [百度网盘](https://pan.baidu.com/s/1PU2OwcZ0IBdxPWW2W7eeNA)
（提取码 `bwj4`
）或 [北大网盘](https://disk.pku.edu.cn:443/link/FFB1DD2A4DD3921B80E5502BA0B06036)
下载并置于 `test/data/` 目录下，重命名为 `records.bin` 或修改 `test/CMakeLists.txt` 中测试定义。

在根目录下执行（替换 `<sketch>` 为需要测试的 Sketch 名称）：

```shell
ctest -R test_<sketch> -VV
```

以获取当前 Sketch 实现在该数据集上的相关结果。

## Code Structure

### 主体算法

- 所有算法代码置于 `SketchLab` 命名空间中
- 抽取多个 Sketch 算法可能共用的 哈希函数、工具函数等 置于 `common/` 目录下，并分类分置于对应文件、对应命名空间中
- Sketch 算法实现分文件置于 `sketch/` 目录下
- 鉴于单个算法实现不复杂、算法间不耦合且可能定义模板类，均实现为 Header-Only 库
  - 如需闭源对外提供实现，需要特化模板并分离实现，编译为（静态）库文件

### 测试

- `test/SketchTest.h(.cpp)` 中定义了 `SketchTest` 类并实现了通用的测试数据接口
- 对于每种已实现的 Sketch 算法，应于 `test/sketch/` 目录下创建对应的派生类，并实现特有的 `run` 方法，于工厂注册（参考 `test/sketch/TestCMSketch.h`）且包含入 `test/sketch/AllSketchTest.h` 中
- `test/driver.cpp` 作为测试入口，驱动执行各算法测试类的 `run` 方法并依需求打印结果
- 应为每种 Sketch 算法在 `test/CMakeLists.txt` 中创建 CTest 项目

## Style Guides

- 本仓库中的实现使用 C++11 标准以兼顾代码通用性与清晰简洁
  - 合理编写 C++ 风格的代码，但也应考虑 Sketch 的算法实现性能
- 类、函数、变量定义应尽量表意且避免歧义，暂制定下列规则
  - 类、全局函数、全局变量定义使用首字母大写的 `PascalCase`
  - 类方法定义使用首字母小写的 `camelCase`
  - 类成员变量、临时变量定义使用 `snake_case`
  - 全局常量定义使用全大写的 `SNAKE_CASE`
- 定义**位长相关**变量时，使用 `cstdint` 中提供的类型（如 `uint64_t` 等）以保证满足预期的一致性
- 使用 Clang Format 进行代码格式化，直接继承 LLVM 配置
  - commit 前请先格式化代码，现代文本编辑器 / IDE 均应有内置支持

## 如何debug

- 用cmake的dubug模式
- 找到`driver`文件(一般在`test/driver`),gdb它
- 输入参数。输入的参数形式和`test/CMakeList.txt`下的形式相同。ctest只是用于简化输入参数
- 打断点，调试

例如，你想调试CountingBloomFilterTest.h文件

``` shell
$ cmake -DCMAKE_BUILD_TYPE=Debug .
$ make
$ gdb ./test/driver
# 进入了gdb
$ set args -s CountingBloomFilter -r ./test/data/records.bin -c ./test/config.ini
# 然后打断点，run
```

## Implementation Status

已有实现状态：**h**ave, **c**hecked, checked but with **d**oubt, **t**ested

|                         | C   | C++ |
| ----------------------- | --- | --- |
| cm sketch               | c   | t   |
| count sketch            | c   | t   |
| cu sketch               | h   | t   |
| bloom filter            | c   | t   |
| counting bloom filter   | c   | t   |
| LD-sketch               | h   | t   |
| MV-sketch               | h   | t   |
| hashpipe                | h   | t   |
| FM-sketch(PCSA)         | h   | t   |
| Linear Counting         | h   |     |
| Kmin(KMV)               | h   |     |
| Deltoid                 | h   | t   |
| flow radar              | h   | t   |
| sketch learn            |     |     |
| elastic sketch          | h   | t   |
| univmon                 | h   |     |
| nitro sketch            | h   | t   |
| reversible sketch       | h   |     |
| Mrac                    | h   | t   |
| k-ary sketch            | h   | t   |
| seqHash                 |     |     |
| TwoLevel                |     | t   |
| multi-resolution bitmap |     |     |
| lossy count             |     | t   |
| space saving            |     | t   |
| HyperLogLog             |     | t   |
| Misra-Gries             |     | t   |
| Fast Sketch             |     | t   |
| CounterBraids           |     | t   |

## 论文

CM Sketch

- Cormode, Graham, and Shan Muthukrishnan. "An improved data stream summary: the count-min sketch and its applications." Journal of Algorithms 55.1 (2005): 58-75.

Count Sketch

- M. Charikar, K. Chen, and M. Farach-Colton. Finding frequent items in data streams. Automata, languages and programming, pages 784–784, 2002.

CU Sketch

- C. Estan and G. Varghese. New directions in traffic measurement and accounting: Focusing on the elephants, ignoring the mice. ACM Transactions on Computer Systems (TOCS), 21(3):270–313, 2003.

Bloom Filter

- Bloom, Burton H. "Space/time trade-offs in hash coding with allowable errors." Communications of the ACM 13.7 (1970): 422-426.

Counting Bloom Filter

- L. Fan, P. Cao, J. Almeida, A.Z. Broder
Summary cache: a scalable wide-area web cache sharing protocol
Proc. of the ACM SIGCOMM (Sep. 1998), pp. 254-265

LD-Sketch

- Huang, Qun, and Patrick PC Lee. "Ld-sketch: A distributed sketching design for accurate and scalable anomaly detection in network data streams." IEEE INFOCOM 2014-IEEE Conference on Computer Communications. IEEE, 2014.

MV-Sketch

- Tang, Lu, Qun Huang, and Patrick PC Lee. "Mv-sketch: A fast and compact invertible sketch for heavy flow detection in network data streams." IEEE INFOCOM 2019-IEEE Conference on Computer Communications. IEEE, 2019.

HashPipe

- Sivaraman, Vibhaalakshmi, et al. "Heavy-hitter detection entirely in the data plane." Proceedings of the Symposium on SDN Research. 2017.

FM Sketch

- P. Flajolet and G. N. Martin, “Probabilistic counting algorithms for database applications,” Journal of computer and system sciences, vol. 31, no. 2, pp. 182–209, 1985.(但是找不到电子版，下面这个论文也有介绍FM Sketch)
- Wang, Lun, et al. "Fine-grained probability counting: Refined loglog algorithm." 2018 IEEE International Conference on Big Data and Smart Computing (BigComp). IEEE, 2018.

Linear Counting

- Whang, Kyu-Young, Brad T. Vander-Zanden, and Howard M. Taylor. "A linear-time probabilistic counting algorithm for database applications." ACM Transactions on Database Systems (TODS) 15.2 (1990): 208-229.

Kmin

- <https://agkn.wordpress.com/2012/07/09/sketch-of-the-day-k-minimum-values/>

Deltoid

- Cormode, Graham, and Shanmugavelayutham Muthukrishnan. "What's new: Finding significant differences in network data streams." IEEE/ACM Transactions on Networking 13.6 (2005): 1219-1232.

Flow Radar

- Li, Yuliang, et al. "Flowradar: A better netflow for data centers." 13th {USENIX} Symposium on Networked Systems Design and Implementation ({NSDI} 16). 2016.

Sketch Learn

- Huang, Qun, Patrick PC Lee, and Yungang Bao. "Sketchlearn: relieving user burdens in approximate measurement with automated statistical inference." Proceedings of the 2018 Conference of the ACM Special Interest Group on Data Communication. 2018.

Elastic Sketch

- Yang, Tong, et al. "Elastic sketch: Adaptive and fast network-wide measurements." Proceedings of the 2018 Conference of the ACM Special Interest Group on Data Communication. 2018.

univmon

- Liu, Zaoxing, et al. "One sketch to rule them all: Rethinking network flow monitoring with univmon." Proceedings of the 2016 ACM SIGCOMM Conference. 2016.

nitro sketch

- Liu, Zaoxing, et al. "Nitrosketch: Robust and general sketch-based monitoring in software switches." Proceedings of the ACM Special Interest Group on Data Communication. 2019. 334-350.

Reversible Sketch

- Schweller, Robert, et al. "Reversible sketches: enabling monitoring and analysis over high-speed data streams." IEEE/ACM Transactions on Networking 15.5 (2007): 1059-1072.

Mrac

- Abhishek Kumar, Minho Sung, Jun (Jim) Xu, and Jia Wang. 2004. "Data streaming algorithms for efficient and accurate estimation of flow size distribution". In Proceedings of the joint international conference on Measurement and modeling of computer systems
  - 这个论文的算法是有问题的，复杂度非常高。所以只实现了它数据面的部分。

k-ary sketch

- Krishnamurthy, Balachander, et al. "Sketch-based change detection: Methods, evaluation, and applications." Proceedings of the 3rd ACM SIGCOMM conference on Internet measurement. 2003.

SeqHash

- Bu, Tian, et al. "Sequential hashing: A flexible approach for unveiling significant patterns in high speed networks." Computer Networks 54.18 (2010): 3309-3326.

Two level

- VENKATARAMAN, S., SONG, D., GIBBONS, P. B., AND BLUM, A. New streaming algorithms for fast detection of superspreaders. In Network and Distributed System Security Symposium (2005).

multi-resolution bitmap

- ESTAN, C., VARGHESE, G., AND FISK, M. Bitmap algorithms for counting active flows on high speed links. In IMC (2003).

lossy count

- Gurmeet Singh Manku. 2002. Approximate Frequency Counts over Data Streams. In Proc. of VLDB.

spacesaving

- Ahmed Metwally, Divyakant Agrawal, and Amr El Abbadi. 2005. Effi- cient Computation of Frequent and Top-k Elements in Data Streams. In Proc. of ICDT.

HyperLoglog

- Philippe Flajolet, Éric Fusy, Olivier Gandouet, and Frédéric Meunier. 2007. HyperLogLog: The Analysis of A Near-optimal Cardinality Estimation Algorithm. In Proc. of AOFA. 127–146.

Misra-Gries

- <https://en.wikipedia.org/wiki/Misra%E2%80%93Gries_summary>
- J. Misra and David Gries. 1982. Finding repeated elements. Science of Computer Programming 2, 2 (1982), 143–152.

Fast Sketch

- Liu, Y., Chen, W., Guan, Y.: A fast sketch for aggregate queries over high-speed network traffic. In: Proceedings of the IEEE International Conference on Computer Communications, pp. 2741–2745 (2012)

CounterBraids

- Yi Lu, Andrea Montanari, Balaji Prabhakar, Sarang Dharmapurikar, and Abdul Kabbani. 2008. Counter braids: a novel counter architecture for per-flow measurement. In Proceedings of the 2008 ACM SIGMETRICS international conference on Measurement and modeling of computer systems (SIGMETRICS '08). Association for Computing Machinery, New York, NY, USA, 121–132.

原 SketchLab 仓库：[Networking/SketchLab](http://47.105.117.154:8805/Networking/SketchLab)

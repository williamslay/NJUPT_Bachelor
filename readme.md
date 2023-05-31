<style type="text/css">
    h1 { counter-reset: h2counter; }
    h2 { counter-reset: h3counter; }
    h3 { counter-reset: h4counter; }
    h4 { counter-reset: h5counter; }
    h5 { counter-reset: h6counter; }
    h6 { }
    h2:before {
      counter-increment: h2counter;
      content: counter(h2counter) ".\0000a0\0000a0";
    }
    h3:before {
      counter-increment: h3counter;
      content: counter(h2counter) "."
                counter(h3counter) ".\0000a0\0000a0";
    }
    h4:before {
      counter-increment: h4counter;
      content: counter(h2counter) "."
                counter(h3counter) "."
                counter(h4counter) ".\0000a0\0000a0";
    }
    h5:before {
      counter-increment: h5counter;
      content: counter(h2counter) "."
                counter(h3counter) "."
                counter(h4counter) "."
                counter(h5counter) ".\0000a0\0000a0";
    }
    h6:before {
      counter-increment: h6counter;
      content: counter(h2counter) "."
                counter(h3counter) "."
                counter(h4counter) "."
                counter(h5counter) "."
                counter(h6counter) ".\0000a0\0000a0";
    }
</style>

# 定向灰盒模糊工具软件说明书

## 软件概述
本软件是基于AFLGo为基础架构实现的定向灰盒模糊测试工具（Directed Grey-box Fuzzing, 下称DGF）。为区别于AFLGo，下称本软件为AFLGo $^\prime$。

### 目标
AFLGo $^\prime$ 是一种基于模糊测试的自动化软件测试工具，其主要的目的是通过生成大量随机输入数据以发现程序中可能存在的漏洞和错误。定向灰盒模糊测试的应用场景主要为：

-  补丁测试（Patch Testing）
-  崩溃重现（Crash Reproduction）
-  静态分析报告验证（Static Analysis Report Verification）
-  数据流检测（Information Flow Detection）

在AFLGo $^\prime$ 的主要目标场景为补丁测试。即源码中由行号指定的代码区域是否会导致程序的错误。
### 功能
用户给出重点测试被测试程序中需要重点检测的目标代码区域，该工具会通过生成测试用例来全面检测被测试程序是否会因为目标代码的引入导致崩溃，从而辅助测试人员检测确定代码中的漏洞。

## 运行环境
### 硬件
本软件支持大多数现代 PC 平台和服务器硬件，包括 x86、x86-64、ARM 和 PowerPC 等架构。内存推荐4GB及以上。
### 系统
本软件主要运行在 Linux 操作系统环境。具体推荐Ubuntu 14.04及以上，Debian 8及以上，CentOS 7及以上。
## 使用说明
### 安装和初始化
1. 本工具初始提供AFLGo $^\prime$ 源码。
2. 在对源码编译之前需要确保已经安装相应的依赖库。可以使用以下命令安装。
```
sudo apt-get update
sudo apt-get install python3
sudo apt-get install python3-dev
sudo apt-get install python3-pip
sudo apt-get install libboost-all-dev 
sudo pip3 install --upgrade pip
sudo pip3 install networkx
sudo pip3 install pydot
sudo pip3 install pydotplus
```
3. 在编译之前需要确保系统中已经包含安装有Gold-plugin的LLVM 11.0.0。
4. 对工具源码进行编译。可以使用以下命令。
```
export AFLGO=$PWD/aflgo

# Compile source code
pushd $AFLGO
make clean all 
cd llvm_mode
make clean all
cd ..
cd distance_calculator/
cmake -G Ninja ./
cmake --build ./
popd
```
### 被测试程序预处理
将以Libxml2为被测试程序介绍如何使用本工具。
1. 首先需要准备好被测试程序。
```
# Clone subject repository
git clone https://gitlab.gnome.org/GNOME/libxml2
export SUBJECT=$PWD/libxml2
```
2. 准备包含被测试程序的目标区域文件。
```
# Setup directory containing all temporary files
mkdir temp
export TMP_DIR=$PWD/temp

# Download commit-analysis tool
wget https://raw.githubusercontent.com/jay/showlinenum/develop/showlinenum.awk
chmod +x showlinenum.awk
mv showlinenum.awk $TMP_DIR

# Generate BBtargets from commit ef709ce2
pushd $SUBJECT
  git checkout ef709ce2
  git diff -U0 HEAD^ HEAD > $TMP_DIR/commit.diff
popd
cat $TMP_DIR/commit.diff |  $TMP_DIR/showlinenum.awk show_header=0 path=1 | grep -e "\.[ch]:[0-9]*:+" -e "\.cpp:[0-9]*:+" -e "\.cc:[0-9]*:+" | cut -d+ -f1 | rev | cut -c2- | rev > $TMP_DIR/BBtargets.txt

# Print extracted targets. 
echo "Targets:"
cat $TMP_DIR/BBtargets.txt
```
3. 将被测试程序的目标区域文件作为辅助参数`-targets`的输入值，对被测试程序进行第一次编译处理。
```
# Set aflgo-instrumenter
export CC=$AFLGO/afl-clang-fast
export CXX=$AFLGO/afl-clang-fast++

# Set aflgo-instrumentation flags
export COPY_CFLAGS=$CFLAGS
export COPY_CXXFLAGS=$CXXFLAGS
export ADDITIONAL="-targets=$TMP_DIR/BBtargets.txt -outdir=$TMP_DIR -flto -fuse-ld=gold -Wl,-plugin-opt=save-temps"
export CFLAGS="$CFLAGS $ADDITIONAL"
export CXXFLAGS="$CXXFLAGS $ADDITIONAL"

# Build libxml2 (in order to generate CG and CFGs).
export LDFLAGS=-lpthread
pushd $SUBJECT
  ./autogen.sh
  ./configure --disable-shared
  make clean
  make xmllint
popd
```
4. 检查第一次编译处理是否正确运行，并计算生成基本块级目标距离文件。
```
# Test whether CG/CFG extraction was successful
$SUBJECT/xmllint --valid --recover $SUBJECT/test/dtd3
ls $TMP_DIR/dot-files
echo "Function targets"
cat $TMP_DIR/Ftargets.txt

# Clean up
cat $TMP_DIR/BBnames.txt | grep -v "^$"| rev | cut -d: -f2- | rev | sort | uniq > $TMP_DIR/BBnames2.txt && mv $TMP_DIR/BBnames2.txt $TMP_DIR/BBnames.txt
cat $TMP_DIR/BBcalls.txt | grep -Ev "^[^,]*$|^([^,]*,){2,}[^,]*$"| sort | uniq > $TMP_DIR/BBcalls2.txt && mv $TMP_DIR/BBcalls2.txt $TMP_DIR/BBcalls.txt
cut -d',' -f1,3 $TMP_DIR/BBcallcounts.txt | sort | uniq -c  |awk '{print $2","$1}' > $TMP_DIR/BBcallcounts2.txt
sort -u $TMP_DIR/BBcallcounts.txt | cut -d',' -f1,3 | >  $TMP_DIR/BBcallcounts3.txt
sort $TMP_DIR/BBcallcounts3.txt | uniq -c | awk '{print $2","$1}' > $TMP_DIR/BBcallcounts4.txt && mv $TMP_DIR/BBcallcounts4.txt $TMP_DIR/BBcallcounts3.txt
awk -F ',' 'FNR==NR{a[$1,$2]=$3;next}{print $0","a[$1,$2]}' $TMP_DIR/BBcallcounts3.txt $TMP_DIR/BBcallcounts2.txt  > $TMP_DIR/BBcallcounts.txt


cd $TMP_DIR 
ls | grep -P '^BBcallcounts(?!\.txt$)[\w-]+\.txt$' | xargs rm
popd

# Generate distance
$AFLGO/scripts/gen_distance_fast.py $SUBJECT $TMP_DIR xmllint

# Check distance file
echo "Distance values:"
head -n5 $TMP_DIR/distance.cfg.txt
echo "..."
tail -n5 $TMP_DIR/distance.cfg.txt
```
5. 将被测试程序的基本块级目标距离文件`distance.cfg.txt`作为辅助参数`-distance`的输入值，对被测试程序进行第二次编译处理插桩。
```
export CFLAGS="$COPY_CFLAGS -distance=$TMP_DIR/distance.cfg.txt"
export CXXFLAGS="$COPY_CXXFLAGS -distance=$TMP_DIR/distance.cfg.txt"

# Clean and build subject with distance instrumentation ☕️
pushd $SUBJECT
  make clean
  ./configure --disable-shared
  make xmllint
popd

sed -i 's/[{}]//g'  $TMP_DIR/target.trace.txt
``` 

正常结束上述预处理过程生成的temp文件夹中应该包含以下文件:
- dot-files/：该目录包含被测试程序的所有函数的控制流图（CFG）。
- BBcallcounts.txt：该文件包含函数间调用模式因子。
- BBcalls.txt：该文件包含被测试程序的基本块对函数的调用情况
- BBnames.txt：该文件包含被测试程序所有基本块
- BBtargets.txt：该文件为最初始由用户提供的目标文件
- callgraph.distance.txt：该文件为预处理得到的函数级目标距离文件
- distance.cfg.txt：该文件为预处理得到的基本块级目标距离文件
- Fnames.txt：该文件包含被测试程序中所有的函数名
- Ftargets.txt： 该文件包含目标函数名
- target.trace.txt：该文件包含可达目标函集中所有函数的函数名
### 对被测试程序进行模糊测试
1. 准备初始种子。这一步采用的是AFLGo $^\prime$ 提供的预备种子。
```
# Construct seed corpus
mkdir in
cp -r $SUBJECT/test/dtd* in
cp $SUBJECT/test/dtds/* in
```
2. 模糊测试
```
$AFLGO/afl-fuzz -S ef709ce2 -z exp -c 45m -i in -o out $SUBJECT/xmllint --valid --recover @@
```
### 检查测试结果
AFLGo $^\prime$ 的输出文件夹结构如下：
- queue/：该目录包含待测试的输入文件，初始时需要向该目录中添加一些样本文件。
- crashes/：该目录包含导致程序崩溃的输入文件，这些文件有助于开发人员进行漏洞修复。
- hangs/：该目录包含导致程序挂起或停止响应的输入文件，这些文件也可能指示程序中存在潜在的漏洞。
- fuzz_bitmap：该文件记录了已经执行过的代码块，以帮助 AFLGo $^\prime$ 选择新的测试用例。
- fuzzer_stats：该文件提供有关 AFLGo $^\prime$ 运行期间的统计信息，例如已经执行的测试用例数量、平均执行时间等等。
- plot_data/：该目录包含生成 plot 统计图所需的数据文件。
- sync_dir/：如果使用多个 AFLGo $^\prime$ 实例进行并行测试，该目录包含各个实例之间同步的数据。
- README.txt：该文件包含有关 AFLGo $^\prime$ 输出文件夹结构的简要说明。


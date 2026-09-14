实验二 xv6 文件管理与用户程序
========================================

本次实验继续使用实验一配置好的 ``xv6-riscv`` 环境。你将编写 ``sixfive`` 和 ``find``
两个用户程序，练习通过系统调用读取普通文件和目录，并理解用户程序如何被编译、写入
``fs.img``，最终在 xv6 shell 中运行。

本次实验不要求修改 xv6 内核。实验内容参考 `MIT 6.1810 Fall 2026: Xv6 and Unix utilities
<https://pdos.csail.mit.edu/6.828/2026/labs/util.html>`_，并根据本课程使用的
``xv6-riscv`` 仓库进行了调整。

.. raw:: html

   <nav class="lab-progress" aria-label="实验流程">
     <a href="#lab2-goals"><span>01</span>了解目标</a>
     <a href="#lab2-prepare"><span>02</span>准备源码</a>
     <a href="#lab2-makefile"><span>03</span>修改构建</a>
     <a href="#lab2-sixfive"><span>04</span>读取文件</a>
     <a href="#lab2-find"><span>05</span>递归查找</a>
     <a href="#lab2-exec"><span>06</span>执行命令</a>
     <a href="#lab2-report"><span>07</span>检查提交</a>
   </nav>

.. _lab2-goals:

实验目标
~~~~~~~~

完成本次实验后，你应该能够：

* 说明 xv6 用户程序从源代码到 ``fs.img`` 的构建过程；
* 区分 Makefile 中 ``UPROGS`` 和 ``UEXTRA`` 的作用；
* 使用 ``open``、``read`` 和 ``close`` 读取普通文件；
* 使用 ``fstat`` 判断文件类型，并读取 xv6 目录项；
* 使用 ``fork``、``exec`` 和 ``wait`` 对查找到的文件执行指定命令；
* 使用 C 字符串处理文本、文件名和路径；
* 使用递归遍历多层目录；
* 对系统调用失败、参数错误和路径过长等情况进行基本处理。

本次实验包含两个必做任务：

#. ``sixfive``：读取文本文件，输出其中能被 5 或 6 整除的数字；
#. ``find``：从指定路径开始，递归查找具有指定名称的文件。

.. raw:: html

   <div class="admonition mycaution">
     <p class="admonition-title">实验边界</p>
     <p>本次实验只编写用户态程序，不添加新的系统调用，也不修改
     <code>kernel/</code> 中的内核代码。程序必须使用 xv6 已经提供的接口完成。</p>
   </div>

.. _lab2-report:

实验报告与提交要求
~~~~~~~~~~~~~~~~~~

实验报告至少应包含以下内容：

#. 实验使用的 xv6-riscv Git 提交短哈希；
#. ``UPROGS``、``UEXTRA`` 和 ``fs.img`` 构建规则的修改；
#. xv6 中执行 ``ls`` 后显示 ``sixfive``、``find`` 和 ``sixfive.txt`` 的截图；
#. ``sixfive`` 的文本解析思路和关键代码；
#. ``sixfive`` 正常输入、边界输入和错误输入的测试结果；
#. ``find`` 的递归遍历思路和关键代码；
#. ``find -exec`` 的参数构造过程以及 ``fork``、``exec``、``wait`` 的作用；
#. 普通 ``find`` 和 ``find -exec`` 两种模式的运行结果；
#. 至少三层目录的创建过程及 ``find`` 输出；
#. 实验中遇到的问题、定位过程和解决方法；
#. 本页思考题的答案。

需要提交的代码文件为：

.. code-block:: text

   Makefile
   user/sixfive.c
   user/sixfive.txt
   user/find.c

如果实验一的 ``user/hello.c`` 仍在仓库中，可以保留，无需删除。

提交前应在 Ubuntu shell 中检查：

.. code-block:: bash

   git status
   git diff
   make clean
   make qemu

确认两个程序都能够从干净状态编译并运行后，再按照课程通知提交代码和实验报告。

.. raw:: html

   <div class="admonition mycaution">
     <p class="admonition-title">提交前检查</p>
     <p>代码必须能够在课程提供的 xv6-riscv 环境中从干净状态编译。截图应同时包含命令和输出，
     文字应清晰可辨；不要只放运行截图而不解释程序的设计和结果。</p>
   </div>


.. _lab2-prepare:

实验准备
~~~~~~~~

进入实验一使用的 xv6-riscv 仓库：

.. code-block:: bash

   cd ~/os-lab/xv6-riscv
   git status
   git rev-parse --short HEAD

如果课程另行指定了仓库、分支或起始代码，应以课程通知为准。开始修改前，应确认实验一的
代码已经妥善保存，并记录当前仓库版本，避免在多个 xv6-riscv 副本中混淆修改。

复习源码目录
------------

本次实验主要使用以下文件：

``user/ls.c``
  xv6 的 ``ls`` 用户程序。阅读它可以了解如何调用 ``open``、``fstat`` 和 ``read``，
  以及如何处理 ``struct dirent``。

``user/cat.c``
  xv6 的 ``cat`` 用户程序。阅读它可以了解如何循环调用 ``read`` 读取普通文件。

``user/ulib.c``
  xv6 用户库的一部分，可以查看 ``atoi``、``strchr``、``strcmp`` 和 ``strlen`` 等函数。

``user/user.h``
  用户程序可调用的系统调用和用户库函数声明。

``kernel/fs.h``
  文件系统相关数据结构，例如 ``struct dirent`` 和 ``DIRSIZ``。

``kernel/stat.h``
  ``struct stat`` 以及 ``T_FILE``、``T_DIR`` 等文件类型定义。

``kernel/fcntl.h``
  ``O_RDONLY``、``O_WRONLY`` 等文件打开标志。

``Makefile``
  控制用户程序的编译以及 ``fs.img`` 的生成。

先在 Ubuntu shell 中阅读相关代码：

.. code-block:: bash

   sed -n '1,200p' user/cat.c
   sed -n '1,240p' user/ls.c
   sed -n '1,200p' user/user.h
   sed -n '1,160p' kernel/fs.h
   sed -n '1,160p' kernel/stat.h

.. raw:: html

   <div class="admonition myquestion">
     <p class="admonition-title">用户程序为什么能够读取文件？</p>
     <p><code>sixfive.c</code> 和 <code>find.c</code> 都运行在用户态，不能直接操作磁盘。
     阅读 <code>user/user.h</code>，找出本次实验使用的系统调用，并思考这些调用如何把请求交给 xv6 内核。</p>
   </div>

用户程序的构建过程
------------------

用户程序不会因为放入 ``user/`` 目录就自动出现在 xv6 中。以实验一的 ``hello`` 为例，
其构建过程可以概括为：

.. code-block:: text

   user/hello.c
         ↓ 由 Makefile 交叉编译和链接
   user/_hello
         ↓ 由 mkfs 写入文件系统镜像
   fs.img
         ↓ QEMU 启动 xv6
   在 xv6 shell 中执行 hello

本次实验的 ``sixfive.c`` 和 ``find.c`` 也需要加入 ``UPROGS``。测试数据
``sixfive.txt`` 是普通文件，需要通过 ``UEXTRA`` 写入 ``fs.img``。

.. _lab2-makefile:

修改 Makefile
~~~~~~~~~~~~~

添加用户程序
------------

在 xv6-riscv 根目录的 ``Makefile`` 中找到 ``UPROGS``。保留其中已有的所有程序，
在程序列表中加入：

.. code-block:: make

      $U/_sixfive\
      $U/_find\

如果实验一的 ``hello`` 仍然保留，相应部分可以写成：

.. code-block:: make

      $U/_hello\
      $U/_sixfive\
      $U/_find\

``user/sixfive.c`` 会被编译为主机文件系统中的 ``user/_sixfive``，但是进入 xv6 后，
运行命令仍然是 ``sixfive``，不需要输入下划线。

添加测试文件
------------

在 ``UPROGS`` 定义之后、``fs.img`` 构建规则之前加入：

.. code-block:: make

   UEXTRA += user/sixfive.txt

``UPROGS`` 保存需要交叉编译的用户程序；``UEXTRA`` 保存直接写入文件系统镜像的普通文件。
不要把 ``sixfive.txt`` 加入 ``UPROGS``。

修改文件系统镜像构建规则
------------------------

找到 ``fs.img`` 构建规则，将 ``$(UEXTRA)`` 同时加入依赖列表和 ``mkfs`` 命令：

.. code-block:: make

   fs.img: mkfs/mkfs README $(UEXTRA) $(UPROGS)
      mkfs/mkfs fs.img README $(UEXTRA) $(UPROGS)

第二行 ``mkfs/mkfs`` 前面必须是一个 Tab，不能使用普通空格。否则 Make 可能报告：

.. code-block:: text

   Makefile:...: *** missing separator.  Stop.

如果当前 Makefile 已经定义了 ``UEXTRA`` 或者已经在 ``fs.img`` 规则中使用了
``$(UEXTRA)``，只需把 ``user/sixfive.txt`` 加入现有配置，不要重复覆盖其他文件。

创建测试数据
------------

在 Ubuntu shell 中创建并编辑 ``user/sixfive.txt``：

.. code-block:: bash

   nano user/sixfive.txt

写入以下内容：

.. code-block:: text

   5
   3
   127
   100
   18-4
   06


保存后检查文件：

.. code-block:: bash

   sed -n '1,20p' user/sixfive.txt

现在可以先创建两个程序的空文件：

.. code-block:: bash

   touch user/sixfive.c user/find.c

完成程序后，使用下面的命令重新生成文件系统镜像：

.. code-block:: bash

   make clean
   make qemu

进入 xv6 后执行 ``ls``，应当能够看到 ``sixfive``、``find`` 和 ``sixfive.txt``。

.. raw:: html

   <div class="admonition mytodo">
     <p class="admonition-title">检查 Makefile 和 fs.img</p>
     <p>完成 <code>UPROGS</code>、<code>UEXTRA</code> 和 <code>fs.img</code> 规则的修改。
     重新编译并进入 xv6，执行 <code>ls</code>，确认两个用户程序和测试文件均已进入文件系统镜像。
     在实验报告中附上 Makefile 修改和 xv6 中 <code>ls</code> 输出的截图。</p>
   </div>

.. _lab2-sixfive:

任务一：sixfive
~~~~~~~~~~~~~~~

问题描述：筛选文本中的数字
--------------------------

在 ``user/sixfive.c`` 中实现 ``sixfive``。程序接收一个或多个文本文件作为命令行参数，
按照参数顺序读取文件，并输出其中所有能被 5 或 6 整除的数字。

运行方式：

.. code-block:: sh

   sixfive file1 [file2 ...]

数字由一个或多个连续的十进制数字组成。数字之间只能使用下列字符分隔：

.. code-block:: c

   " -\r\t\n./,"

这些字符依次包括空格、连字符、回车、制表符、换行符、句点、斜杠和逗号。
文件开头和文件结尾也视为分隔符。

下表给出了一些边界情况，其中 ``␠`` 表示空格：

.. list-table:: sixfive 输入识别规则
   :widths: 25 20 55
   :header-rows: 1

   * - 输入片段
     - 是否识别数字
     - 原因
   * - ``6``
     - 是
     - 文件开头和结尾是隐式分隔符
   * - ``/6,``
     - 是
     - ``6`` 两侧都是合法分隔符
   * - ``xv6``
     - 否
     - ``6`` 与字母相连，不是独立数字
   * - ``25.36``
     - 是
     - 句点是分隔符，得到 ``25`` 和 ``36``
   * - ``a30␠``
     - 否
     - ``30`` 左侧不是合法分隔符
   * - ``␠30b``
     - 否
     - ``30`` 右侧不是合法分隔符

本实验提供的 ``sixfive.txt`` 内容如下：

.. code-block:: text

   5
   3
   127
   100
   18-4
   06

运行程序：

.. code-block:: sh

   $ sixfive sixfive.txt
   5
   100
   18
   6
   $

在该输入文件中：

* ``5`` 和 ``100`` 能被 5 整除，因此需要输出；
* ``3`` 和 ``127`` 不能被 5 或 6 整除，因此不输出；
* 连字符是合法分隔符，因此 ``18-4`` 被识别为数字 ``18`` 和 ``4``。其中只有 ``18`` 能被 6 整除；
* ``06`` 是合法的十进制数字，转换为整数后数值为 ``6``，因此程序输出 ``6``。

实现要求
--------

``sixfive`` 必须满足以下要求：

#. 使用 ``open`` 打开输入文件；
#. 使用 ``read`` 读取文件，建议每次读取一个字符；
#. 使用 ``strchr`` 判断一个字符是否属于分隔符集合；
#. 正确识别由连续十进制数字构成的数；
#. 只有当数字左右两侧均为合法分隔符、文件开头或文件结尾时，才把它视为有效数字；
#. 对有效数字判断 ``number % 5 == 0 || number % 6 == 0``；
#. 每个满足条件的数字单独输出一行；
#. 按顺序处理命令行给出的所有输入文件；
#. 文件处理结束后调用 ``close``；
#. 缺少输入文件或者文件无法打开时，输出清楚的错误信息；
#. 程序结束前调用 ``exit``；
#. 不得硬编码测试数据或预期输出。

输入中的数字均处于普通 C ``int`` 的表示范围内，不要求处理任意精度整数。

实现提示
--------

可以把文本解析设计成一个简单的状态机：

#. 文件开始时允许读取一个新数字；
#. 遇到数字字符时，记录当前数字；
#. 遇到合法分隔符时，结束并检查当前的有效数字，然后准备读取下一个字段；
#. 遇到字母等非数字、非分隔符字符时，将当前字段标记为无效；
#. 无效字段只有在遇到下一个合法分隔符后才能结束；
#. 文件结束时执行一次与遇到分隔符相同的收尾检查。

你可以在 ``user/ulib.c`` 中查看 ``atoi`` 和 ``strchr`` 的实现与使用方法。不要包含 Linux
的 ``stdio.h``，也不要使用 xv6 未提供的 ``fopen``、``fscanf`` 或 ``strtol``。

根据当前 xv6-riscv 版本，可以参考以下头文件：

.. code-block:: c

   #include "kernel/types.h"
   #include "kernel/stat.h"
   #include "kernel/fcntl.h"
   #include "user/user.h"

测试要求
--------

至少完成以下测试：

.. code-block:: sh

   $ sixfive sixfive.txt
   5
   100
   18
   6

还需要测试缺少参数和文件不存在的情况：

.. code-block:: sh

   $ sixfive
   sixfive: missing input file
   $ sixfive nofile.txt
   sixfive: cannot open nofile.txt

错误信息不要求与示例逐字相同，但必须说明错误原因，程序不能崩溃或无限循环。

.. raw:: html

   <div class="admonition mytodo">
     <p class="admonition-title">完成 sixfive</p>
     <p>实现 <code>user/sixfive.c</code>，验证示例输出，并额外测试参数缺失、文件不存在、
     数字位于文件边界以及数字与字母相连等情况。实验报告应说明你如何判断一个数字是否具有合法边界。</p>
   </div>

.. _lab2-find:

任务二：find
~~~~~~~~~~~

任务描述
--------

在 ``user/find.c`` 中实现一个简化版 UNIX ``find`` 程序。程序从指定路径开始递归查找，
输出目录树中名称与目标名称完全相同的路径。

运行方式：

.. code-block:: sh

   find path target

例如，``find . b`` 表示从当前目录开始，递归查找名称为 ``b`` 的文件。

准备测试目录
------------

进入 xv6 后执行：

.. code-block:: sh

   echo > b
   mkdir a
   echo > a/b
   mkdir a/aa
   echo > a/aa/b

得到的目录结构为：

.. code-block:: text

   .
   |-- b
   `-- a
       |-- b
       `-- aa
           `-- b

运行：

.. code-block:: sh

   $ find . b
   ./b
   ./a/b
   ./a/aa/b
   $

程序输出顺序可以遵循目录项的读取顺序，但每条匹配路径必须正确、完整且只输出一次。

实现要求
--------

``find`` 必须满足以下要求：

#. 接收起始路径和目标名称两个参数；
#. 使用 ``open`` 打开当前路径；
#. 使用 ``fstat`` 判断当前路径是普通文件还是目录；
#. 使用 ``read`` 逐个读取目录中的 ``struct dirent``；
#. 跳过 inode 编号为 0 的无效目录项；
#. 不得递归进入 ``.`` 和 ``..``；
#. 使用递归遍历任意深度的子目录；
#. 使用 ``strcmp`` 进行完整文件名比较，不能使用 ``==`` 比较字符串；
#. 输出从起始路径出发的完整路径；
#. 在拼接路径前检查缓冲区长度，不能发生数组越界；
#. 路径处理完成后调用 ``close``；
#. 参数数量错误或者路径无法打开时，输出清楚的错误信息；
#. 不得硬编码目录层数、测试路径、文件名或者预期结果；
#. 不得调用 Ubuntu/Linux 中的 ``find`` 命令代替程序实现。

实现提示
--------

先阅读 ``user/ls.c``，重点观察它如何：

* 使用 ``open`` 打开路径；
* 使用 ``fstat`` 获取文件类型；
* 读取 ``struct dirent``；
* 根据 ``DIRSIZ`` 处理目录项名称；
* 拼接父目录路径和子目录项名称。

可以将递归部分组织成：

.. code-block:: c

   void find(char *path, char *target);

其执行过程可以概括为：

.. code-block:: text

   打开 path
       |
       v
   使用 fstat 判断类型
       |-- 普通文件：比较名称，匹配则输出路径
       `-- 目录：依次读取目录项
                       |-- 跳过空目录项
                       |-- 跳过 . 和 ..
                       |-- 拼接新路径
                       `-- 递归处理新路径

``struct dirent`` 中的名称字段长度固定，不应假设它一定以 ``\0`` 结尾。复制目录项名称时，
需要为 C 字符串预留结尾的空字符。路径拼接时还要为 ``/`` 和 ``\0`` 预留空间。

.. raw:: html

   <div class="admonition mydanger">
     <p class="admonition-title">避免无限递归</p>
     <p>每个目录都包含指向自身的 <code>.</code> 和指向父目录的 <code>..</code>。
     如果递归时不跳过它们，<code>find</code> 会不断访问相同目录，最终耗尽进程栈或文件描述符。</p>
   </div>

测试要求
--------

完成基本示例后，还应测试以下情况。

查找不存在的名称：

.. code-block:: sh

   $ find . not_exist
   $

从子目录开始查找：

.. code-block:: sh

   $ find a b
   a/b
   a/aa/b

查找其他名称：

.. code-block:: sh

   $ echo > a/test
   $ find . test
   ./a/test

参数数量错误：

.. code-block:: sh

   $ find .
   find: usage: find path target

错误信息不要求与示例逐字相同，但程序必须正常退出。

.. raw:: html

   <div class="admonition mytodo">
     <p class="admonition-title">完成 find</p>
     <p>实现 <code>user/find.c</code>，创建至少三层测试目录，验证从当前目录和子目录开始查找的结果。
     实验报告中应展示目录结构、运行命令和完整输出，并说明为什么递归时必须跳过
     <code>.</code> 和 <code>..</code>。</p>
   </div>

编译与调试
~~~~~~~~~~

修改源代码后，退出 QEMU，在 xv6-riscv 根目录重新编译：

.. code-block:: bash

   make qemu

如果新增程序或 ``sixfive.txt`` 没有出现在 xv6 中，执行：

.. code-block:: bash

   make clean
   make qemu

``make clean`` 会重新生成 ``fs.img``。此前在 xv6 shell 中临时创建的 ``a``、``b`` 等文件可能
随之消失，需要重新执行测试目录的创建命令。通过 ``UEXTRA`` 加入的 ``sixfive.txt`` 会在重新生成
``fs.img`` 时再次写入。

如果课程仓库提供 ``make qemu-fs`` 目标，可以使用它在不重新生成文件系统镜像的情况下启动；
是否支持该命令以当前仓库的 Makefile 为准。

建议在每完成一个小步骤后查看修改：

.. code-block:: bash

   git status
   git diff -- Makefile user/sixfive.c user/find.c

不要在 QEMU 仍在运行时执行 ``make clean``。退出 QEMU 时，先按 ``Ctrl+a``，松开后再按 ``x``。

.. _lab2-exec:

任务三：为 find 添加 -exec
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

问题描述：对查找到的文件执行命令
------------------------------

在任务二实现的 ``find`` 基础上增加 ``-exec`` 功能。

普通模式下，``find`` 输出所有名称匹配的文件路径：

.. code-block:: sh

   find path target

增加 ``-exec`` 后，运行形式为：

.. code-block:: sh

   find path target -exec command [arg ...]

对于每个查找到的文件，程序不再直接输出文件路径，而是执行：

.. code-block:: text

   command [arg ...] matched_path

例如：

.. code-block:: sh

   $ find . wc -exec echo hi
   hi ./wc
   $

这里需要查找名称为 ``wc`` 的文件。找到 ``./wc`` 后，程序实际执行的命令为：

.. code-block:: sh

   echo hi ./wc

因此输出：

.. code-block:: text

   hi ./wc

本任务需要使用 ``fork``、``exec`` 和 ``wait``。每找到一个匹配文件，父进程创建一个子进程，
由子进程执行指定命令，父进程等待子进程结束后继续查找。

执行步骤：扩展 find
-------------------

第 1 步：保留原有 find 功能
^^^^^^^^^^^^^^^^^^^^^^^^^^^

修改 ``user/find.c``，使程序同时支持以下两种运行方式：

.. code-block:: sh

   find path target
   find path target -exec command [arg ...]

当没有 ``-exec`` 时，程序应保持任务二的行为，直接输出匹配路径。

当给出 ``-exec`` 时，程序应对每个匹配路径执行指定命令，不再直接输出该路径。

第 2 步：解析命令行参数
^^^^^^^^^^^^^^^^^^^^^^^

程序需要判断第四个参数是否为 ``-exec``。

例如：

.. code-block:: sh

   find . wc -exec echo hi

对应的命令行参数为：

.. list-table:: find -exec 参数
   :widths: 20 30
   :header-rows: 1

   * - 参数
     - 内容
   * - ``argv[0]``
     - ``find``
   * - ``argv[1]``
     - ``.``
   * - ``argv[2]``
     - ``wc``
   * - ``argv[3]``
     - ``-exec``
   * - ``argv[4]``
     - ``echo``
   * - ``argv[5]``
     - ``hi``

因此：

* ``argv[1]`` 是起始路径；
* ``argv[2]`` 是需要查找的文件名；
* ``argv[4]`` 及之后的参数组成需要执行的命令；
* 查找到的文件路径需要追加到命令参数末尾。

如果参数格式不正确，应输出用法提示：

.. code-block:: text

   usage: find path target [-exec command [arg ...]]

第 3 步：构造 exec 参数
^^^^^^^^^^^^^^^^^^^^^^^

xv6 的 ``exec`` 接收程序路径和参数数组：

.. code-block:: c

   exec(command, argv);

参数数组最后必须以空指针 ``0`` 结束。

假设用户执行：

.. code-block:: sh

   find . wc -exec echo hi

并且找到文件 ``./wc``，传递给 ``exec`` 的参数数组应当相当于：

.. code-block:: c

   exec_argv[0] = "echo";
   exec_argv[1] = "hi";
   exec_argv[2] = "./wc";
   exec_argv[3] = 0;

然后调用：

.. code-block:: c

   exec(exec_argv[0], exec_argv);

可以包含 ``kernel/param.h``，使用其中定义的 ``MAXARG`` 限制参数数量：

.. code-block:: c

   #include "kernel/param.h"

构造参数数组时，必须为匹配文件路径和末尾的空指针预留位置，不能超过 ``MAXARG``。

第 4 步：创建子进程
^^^^^^^^^^^^^^^^^^^

每找到一个匹配文件，调用 ``fork`` 创建子进程。

可以将执行命令的操作组织成单独的函数：

.. code-block:: c

   void
   run_command(char *file, char **command, int command_argc)
   {
     // 构造 exec 使用的参数数组
     // 调用 fork
     // 子进程调用 exec
     // 父进程调用 wait
   }

``fork`` 的返回值含义如下：

* 返回值小于 0：创建子进程失败；
* 返回值等于 0：当前处于子进程；
* 返回值大于 0：当前处于父进程。

第 5 步：子进程调用 exec
^^^^^^^^^^^^^^^^^^^^^^^^

子进程构造好参数数组后调用：

.. code-block:: c

   exec(exec_argv[0], exec_argv);

如果 ``exec`` 成功，当前子进程将被指定程序替换，不会继续执行 ``exec`` 后面的代码。

如果 ``exec`` 返回，说明执行失败。此时应输出错误信息并退出：

.. code-block:: c

   fprintf(2, "find: exec %s failed\n", exec_argv[0]);
   exit(1);

第 6 步：父进程等待
^^^^^^^^^^^^^^^^^^^

父进程调用：

.. code-block:: c

   wait(0);

等待当前子进程执行结束，然后继续遍历目录。

这样可以避免产生僵尸进程，同时使多个匹配文件按照查找顺序依次执行命令。

实验要求：find -exec
--------------------

扩展后的 ``find`` 必须满足以下要求：

#. 保留任务二原有的递归查找功能；
#. 支持 ``find path target``，直接输出匹配路径；
#. 支持 ``find path target -exec command [arg ...]``；
#. 对每个匹配路径创建一个子进程；
#. 子进程使用 ``exec`` 执行指定命令；
#. 将匹配文件路径追加到命令参数末尾；
#. 父进程使用 ``wait`` 等待子进程结束；
#. ``-exec`` 模式下不再由 ``find`` 直接输出匹配路径；
#. 参数数组必须以空指针 ``0`` 结束；
#. 参数数量不能超过 ``MAXARG``；
#. 正确处理 ``fork`` 或 ``exec`` 失败的情况；
#. 不得硬编码命令名称、文件名或运行结果。

实现提示：find -exec
--------------------

建议在 ``find`` 的递归函数中增加执行模式和命令参数，例如：

.. code-block:: c

   void
   find(char *path, char *target,
        int exec_mode, char **command, int command_argc);

找到匹配路径后，根据 ``exec_mode`` 决定下一步操作：

.. code-block:: text

   找到匹配路径
       |
       |-- 普通模式：printf 输出路径
       |
       `-- -exec 模式：fork 创建子进程
                            |
                            |-- 子进程：exec 执行命令
                            |
                            `-- 父进程：wait 等待结束

命令参数已经由 xv6 shell 分割完成，不需要自行解析整条命令字符串。只需要将 ``-exec`` 后面的
参数复制到新的参数数组，并在末尾追加匹配文件路径和空指针。

测试 find -exec
---------------

首先确认任务二的普通查找功能没有被破坏：

.. code-block:: sh

   $ find . b
   ./b
   ./a/b
   ./a/aa/b

然后测试 ``-exec``：

.. code-block:: sh

   $ find . wc -exec echo hi
   hi ./wc
   $

继续使用任务二创建的测试目录：

.. code-block:: sh

   $ find . b -exec echo found
   found ./b
   found ./a/b
   found ./a/aa/b
   $

还应测试不存在的命令：

.. code-block:: sh

   $ find . b -exec no_such_command
   find: exec no_such_command failed

程序应输出错误信息，不能导致 xv6 崩溃。

.. raw:: html

   <div class="admonition mytodo">
     <p class="admonition-title">完成任务三</p>
     <p>在任务二的 <code>find</code> 基础上实现 <code>-exec</code>，
     使用 <code>fork</code>、<code>exec</code> 和 <code>wait</code>
     对每个匹配文件执行指定命令。实验报告中应展示普通查找模式和
     <code>-exec</code> 模式的运行结果，并说明父进程和子进程分别完成了什么工作。</p>
   </div>


常见问题
~~~~~~~~

``make: *** No rule to make target 'user/sixfive.txt', needed by 'fs.img'``
  检查文件是否确实位于 ``user/sixfive.txt``，并确认文件名大小写与 Makefile 完全一致。

``make: *** missing separator``
  检查 ``mkfs/mkfs`` 命令前是否使用了 Tab。Makefile 中命令行不能使用普通空格缩进。

进入 xv6 后找不到 ``sixfive`` 或 ``find``
  检查程序是否已加入 ``UPROGS``，然后退出 QEMU，执行 ``make clean`` 和 ``make qemu``。

进入 xv6 后找不到 ``sixfive.txt``
  检查 ``UEXTRA`` 和 ``fs.img`` 构建规则是否正确，并确认执行过 ``make clean`` 重新生成镜像。

编译提示 ``implicit declaration of function``
  说明调用了没有声明的函数，或者使用了 xv6 用户库中不存在的 Linux C 库函数。检查
  ``user/user.h`` 和所包含的头文件。

``find`` 输出乱码或者文件名异常
  ``struct dirent`` 的名称长度固定，可能没有 ``\0`` 结尾。复制名称后需要正确构造 C 字符串。

``find`` 一直运行或反复输出相同目录
  检查是否跳过了 ``.`` 和 ``..``，以及递归调用使用的路径是否正确。

``find`` 运行一段时间后提示无法打开文件
  检查每次 ``open`` 后是否在所有返回路径上执行了 ``close``，避免耗尽文件描述符。

思考题
------

#. ``UPROGS`` 与 ``UEXTRA`` 的作用有什么区别？
#. 为什么 ``xv6`` 中的 ``6`` 不能被 ``sixfive`` 识别，而 ``/6,`` 中的 ``6`` 可以？
#. 为什么文件结尾需要被当作一个隐式分隔符处理？
#. 为什么 C 语言中不能使用 ``==`` 比较两个文件名？
#. 为什么 ``find`` 递归读取目录时必须跳过 ``.`` 和 ``..``？
#. ``sixfive`` 和 ``find`` 分别使用了哪些系统调用？这些系统调用完成了什么工作？
#. ``find.c`` 位于用户态，为什么仍然能够读取 xv6 文件系统中的目录信息？

扩展阅读
~~~~~~~~

* `MIT 6.1810 Fall 2026: Xv6 and Unix utilities
  <https://pdos.csail.mit.edu/6.828/2026/labs/util.html>`_
* `xv6-riscv 官方源码 <https://github.com/mit-pdos/xv6-riscv>`_
* `xv6: a simple, Unix-like teaching operating system
  <https://pdos.csail.mit.edu/6.1810/2024/xv6/book-riscv-rev4.pdf>`_

阅读程序时，应优先查看 xv6 自带的 ``user/cat.c``、``user/ls.c`` 和 ``user/ulib.c``。
本次实验的目标不是记忆接口，而是通过编写和调试程序，理解用户程序如何通过系统调用使用文件系统。

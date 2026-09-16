实验二：xv6 文件管理与用户程序
====================================

本次实验继续使用实验一配置好的 ``xv6-riscv`` 环境。主要任务是编写用户程序 ``find``，
递归查找文件，并使用 ``fork``、``exec`` 和 ``wait`` 对查找结果执行命令。

本实验只修改用户态程序和 ``Makefile``，不添加系统调用，也不修改 ``kernel/`` 中的代码。

.. raw:: html

   <nav class="lab-progress" aria-label="实验流程">
     <a href="#lab2-goals"><span>01</span>实验目标</a>
     <a href="#lab2-prepare"><span>02</span>实验准备</a>
     <a href="#lab2-makefile"><span>03</span>添加程序</a>
     <a href="#lab2-find"><span>04</span>递归查找</a>
     <a href="#lab2-exec"><span>05</span>执行命令</a>
     <a href="#lab2-faq"><span>06</span>常见问题</a>
   </nav>

.. _lab2-goals:

实验目标
~~~~~~~~~~~~~~~~~~~~~

完成本次实验后，应能够：

* 使用 ``open``、``read``、``fstat`` 和 ``close`` 访问 xv6 文件系统；
* 读取 ``struct dirent`` 并递归遍历目录；
* 正确处理文件名、路径和 C 字符串；
* 使用 ``fork``、``exec`` 和 ``wait`` 运行其他用户程序；
* 处理参数错误、系统调用失败和路径过长等情况。

.. _lab2-report:

实验报告与提交要求
------------------------

实验报告至少应包含以下内容：

#. ``Makefile`` 中 ``UPROGS`` 的修改；
#. xv6 中执行 ``ls`` 后显示 ``find`` 的截图；
#. 递归遍历目录的设计和关键代码；
#. 至少三层测试目录的结构、命令和运行结果；
#. ``find -exec`` 的参数构造过程，以及 ``fork``、 ``exec``、 ``wait`` 的作用；
#. 普通查找和 ``-exec`` 模式的测试结果；
#. 实验中遇到的问题及解决方法；
#. 本页思考题的答案。

需要提交的代码文件为：

.. code-block:: text

   Makefile
   user/find.c

.. raw:: html

   <div class="admonition mycaution">
      <p class="admonition-title">提交前检查</p>
      <p>执行 <code>git status</code> 和 <code>git diff</code> 检查修改，并确认代码能够从干净状态编译。
      截图应同时包含命令和输出，每幅截图均应配有简要说明。</p>
   </div>

.. _lab2-prepare:

实验准备
~~~~~~~~~~~~~~~~~~~~~

进入 xv6-riscv 仓库，将 ``.git*`` 删除，并初始化新的 Git 仓库：

.. code-block:: bash

   # cd xv6-riscv
   rm -rf .git*
   git init
   git branch -M main
   git add .
   git commit -m "Initial commit"
   git checkout -b lab2

本实验主要参考以下文件：

``user/ls.c``
  展示如何打开路径、判断文件类型、读取目录项和拼接路径。

``user/ulib.c``
  提供 ``strcmp``、``strlen`` 等字符串函数。

``user/user.h``
  声明用户程序可调用的系统调用和库函数。

``kernel/fs.h``
  定义 ``struct dirent`` 和 ``DIRSIZ``。

``kernel/stat.h``
  定义 ``struct stat``、``T_FILE`` 和 ``T_DIR``。

``kernel/fcntl.h``
  定义 ``O_RDONLY`` 等文件打开标志。

``kernel/param.h``
  定义命令参数数量上限 ``MAXARG``。

用户程序需要经过 Makefile 编译和链接，再由 ``mkfs`` 写入 ``fs.img``：

.. code-block:: text

   user/find.c  →  user/_find  →  fs.img  →  xv6 shell 中的 find

.. _lab2-makefile:

将 find 加入 xv6
~~~~~~~~~~~~~~~~~~~~~

创建源文件：

.. code-block:: bash

   touch user/find.c

在 xv6-riscv 根目录的 ``Makefile`` 中找到 ``UPROGS``，保留已有内容并加入：

.. code-block:: text

   $U/_find\

``user/find.c`` 会被编译为 ``user/_find``；进入 xv6 后，命令名称仍然是 ``find``。
完成程序后，在 Ubuntu shell 中执行：

.. code-block:: bash

   make qemu

进入 xv6 后执行 ``ls``，确认列表中包含 ``find``。若程序没有更新，退出 QEMU 后执行：

.. code-block:: bash

   make clean
   make qemu

``make clean`` 会重新生成 ``fs.img``，因此此前在 xv6 中临时创建的测试文件也会被删除。

.. raw:: html

   <div class="admonition mytodo">
      <p class="admonition-title">将 find 写入文件系统镜像</p>
      <p>把 <code>$U/_find\</code> 加入 <code>UPROGS</code>，重新编译并进入 xv6，
      执行 <code>ls</code> 确认 <code>find</code> 已经存在。</p>
   </div>

.. _lab2-find:

任务一：递归查找文件
~~~~~~~~~~~~~~~~~~~~~~~~

在 ``user/find.c`` 中实现简化版 ``find``：从指定路径开始递归查找，输出名称与目标名称
完全相同的文件或目录路径。

运行格式为：

.. code-block:: sh

   find path target

例如，``find . b`` 表示从当前目录开始查找名称为 ``b`` 的项目。

递归查找要求
---------------------

程序应满足以下要求：

#. 接收起始路径和目标名称两个参数；
#. 使用 ``open`` 和 ``fstat`` 判断当前路径是普通文件还是目录；
#. 使用 ``read`` 逐个读取目录中的 ``struct dirent``；
#. 跳过 inode 编号为 0 的目录项，以及 ``.`` 和 ``..``；
#. 递归遍历任意深度的子目录；
#. 使用 ``strcmp`` 完整比较文件名；
#. 输出从起始路径开始的完整匹配路径；
#. 拼接路径前检查长度，并为 ``/`` 和 ``\0`` 预留空间；
#. 在所有返回路径上正确关闭文件描述符；
#. 参数或系统调用出错时输出错误信息并正常退出。

不得硬编码目录层数、文件名或预期输出，也不得调用 Ubuntu/Linux 的 ``find`` 代替实现。

实现提示
---------------------

先阅读 ``user/ls.c``，重点关注 ``open``、``fstat``、``read``、``DIRSIZ`` 和
``struct dirent`` 的用法。递归函数可以组织为：

.. code-block:: c

   void find(char *path, char *target);

处理流程如下：

.. code-block:: text

   打开 path 并判断类型
       ├─ 普通文件：比较名称，匹配则输出路径
       └─ 目录：读取目录项
                  ├─ 跳过空目录项、. 和 ..
                  ├─ 拼接子路径
                  └─ 递归处理子路径

``struct dirent`` 的名称字段长度固定，不一定以 ``\0`` 结尾。复制名称后必须手动构造合法的
C 字符串，否则可能出现乱码或比较错误。

.. raw:: html

   <div class="admonition mydanger">
      <p class="admonition-title">避免无限递归</p>
      <p>每个目录都包含指向自身的 <code>.</code> 和指向父目录的 <code>..</code>。
      递归时必须跳过二者，否则路径中会不断出现 <code>/.</code> 或 <code>/..</code>，程序反复访问
      相同目录。随着递归层数增加，调用栈和未关闭的文件描述符会持续增长，最终可能出现
      <code>open</code> 失败、用户进程异常退出或控制台被重复输出占满；在 <code>-exec</code> 模式下，
      还会反复创建子进程并执行命令。通常受影响的是当前用户进程，但终端可能表现得像“卡死”。</p>
   </div>

测试任务一
---------------------

在 xv6 中创建三层测试目录：

.. code-block:: sh

   echo > b
   mkdir a
   echo > a/b
   mkdir a/aa
   echo > a/aa/b

目录结构如下：

.. code-block:: text

   .
   ├── b
   └── a
       ├── b
       └── aa
           └── b

预期结果为：

.. code-block:: sh

   $ find . b
   ./b
   ./a/b
   ./a/aa/b
   $

输出顺序可以随目录项读取顺序变化，但路径必须完整，且每个结果只输出一次。还应测试：

.. code-block:: sh

   $ find a b
   a/b
   a/aa/b
   $ find . not_exist
   $ find .
   find: usage: find path target

错误信息不要求与示例完全相同，但必须说明原因，程序不能崩溃或无限循环。

.. raw:: html

   <div class="admonition mytodo">
      <p class="admonition-title">完成递归查找</p>
      <p>实现 <code>find path target</code>，使用至少三层目录测试从当前目录和子目录开始的查找，
      并在实验报告中说明为什么必须跳过 <code>.</code> 和 <code>..</code>。</p>
   </div>

.. _lab2-exec:

任务二：为 find 添加 -exec
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

在任务一的基础上增加以下运行方式：

.. code-block:: sh

   find path target -exec command [arg ...]

每找到一个匹配路径，程序执行：

.. code-block:: text

   command [arg ...] matched_path

例如：

.. code-block:: sh

   $ find . wc -exec echo hi
   hi ./wc
   $

找到 ``./wc`` 后，程序实际执行的是 ``echo hi ./wc``。在 ``-exec`` 模式下，``find``
不再直接输出路径，而是将路径追加到命令参数末尾。

``-exec`` 要求
---------------------

扩展后的程序应满足以下要求：

#. 保留 ``find path target`` 的原有行为；
#. 检查第四个参数是否为 ``-exec``，并确保其后至少有一个命令；
#. 将 ``-exec`` 后的参数和匹配路径组成新的参数数组；
#. 参数数组以空指针 ``0`` 结束，且参数数量不超过 ``MAXARG``；
#. 每个匹配结果由一个子进程调用 ``exec`` 执行；
#. 父进程调用 ``wait``，等待子进程结束后继续查找；
#. 正确处理 ``fork`` 和 ``exec`` 失败。

不得硬编码命令名称、文件名或运行结果。

参数与进程
---------------------

执行 ``find . wc -exec echo hi`` 时，原始参数为：

.. code-block:: text

   argv[0] = "find"
   argv[1] = "."
   argv[2] = "wc"
   argv[3] = "-exec"
   argv[4] = "echo"
   argv[5] = "hi"

若匹配路径为 ``./wc``，传给 ``exec`` 的参数数组应相当于：

.. code-block:: c

   exec_argv[0] = "echo";
   exec_argv[1] = "hi";
   exec_argv[2] = "./wc";
   exec_argv[3] = 0;

   exec(exec_argv[0], exec_argv);

可包含 ``kernel/param.h`` 并使用 ``MAXARG`` 检查参数数量。命令参数已经由 xv6 shell
分割完成，不需要再次解析整条命令字符串。

每次找到匹配路径后：

.. code-block:: text

   fork
    ├─ 子进程：构造参数并调用 exec
    └─ 父进程：调用 wait，随后继续查找

``exec`` 成功后不会返回；若它返回，则说明执行失败，子进程应输出错误信息并退出。

测试任务二
---------------------

先确认普通模式仍然正确，再测试 ``-exec``：

.. code-block:: sh

   $ find . b
   ./b
   ./a/b
   ./a/aa/b
   $ find . b -exec echo found
   found ./b
   found ./a/b
   found ./a/aa/b
   $ find . wc -exec echo hi
   hi ./wc

还应测试不存在的命令和错误参数：

.. code-block:: sh

   $ find . b -exec no_such_command
   find: exec no_such_command failed
   $ find . b -exec
   find: usage: find path target [-exec command [arg ...]]

.. raw:: html

   <div class="admonition mytodo">
      <p class="admonition-title">完成 find -exec</p>
      <p>使用 <code>fork</code>、<code>exec</code> 和 <code>wait</code> 对每个匹配路径执行命令。
      在实验报告中展示两种运行模式，并说明父进程和子进程各自完成的工作。</p>
   </div>

编译与调试
~~~~~~~~~~~~~~~~~~~~~

修改代码后，退出 QEMU 并在 xv6-riscv 根目录重新执行 ``make qemu``。调试过程中可随时检查修改：

.. code-block:: bash

   git status
   git diff -- Makefile user/find.c

不要在 QEMU 运行时执行 ``make clean``。退出 QEMU 时，先按 ``Ctrl+a``，松开后再按 ``x``。

使用打印追踪执行路径
---------------------

xv6 用户程序可以直接使用 ``printf`` 或 ``fprintf(2, ...)`` 输出调试信息。建议先用只有一、两层的
目录复现问题，再在递归函数的关键位置打印状态：

.. code-block:: c

   fprintf(2, "[find] enter path=%s\n", path);
   fprintf(2, "[find] type=%d path=%s\n", st.type, path);
   fprintf(2, "[find] recurse child=%s\n", child_path);
   fprintf(2, "[find] leave path=%s\n", path);

应重点检查：

* 同一路径是否反复出现，或路径中是否不断增加 ``/.``、``/..``；
* ``read`` 返回值和目录项的 inode 编号是否正确；
* 目录项名称复制后是否补上了 ``\0``；不要直接用 ``%s`` 打印未终止的 ``de.name``；
* 每次 ``open`` 是否都有对应的 ``close``，包括错误返回路径；
* 拼接后的路径长度是否超过缓冲区；
* ``fork`` 的返回值、传给 ``exec`` 的每个参数，以及参数数组末尾是否为 ``0``。

如果输出不断重复，可以临时给递归函数增加 ``depth`` 参数并打印递归深度。调试时也可以设置一个较小的
深度上限，使程序在超过上限时输出路径并返回，从而找到形成环的位置；定位完成后应删除该限制，正式程序
仍须支持任意合理深度的目录。

判断程序卡在哪里
---------------------

可以根据最后一条调试信息缩小范围：

* 只有 ``enter``、没有 ``type``：检查 ``open`` 或 ``fstat``；
* 反复输出相同的 ``recurse``：检查 ``.``、``..`` 的比较和路径拼接；
* 只有 ``enter``、没有对应的 ``leave``：检查递归调用、错误分支和 ``close``；
* 停在 ``fork`` 之后：分别打印父、子进程分支，检查子进程是否退出、父进程是否调用 ``wait``；
* ``exec`` 后仍出现下一行调试输出：``exec`` 已失败，应检查命令名称和参数数组。

程序运行时按 ``Ctrl+p``，xv6 会打印进程列表和状态。这可以帮助判断 ``find`` 是仍在运行、等待子进程，
还是已经退出。``Ctrl+p`` 只显示状态，不会终止程序；若重复输出使控制台无法操作，可退出 QEMU，修正代码后
重新启动。

使用 VS Code + GDB 调试
-------------------------

本节沿用实验一的 VS Code 调试环境。``launch.json`` 仍以 ``kernel/kernel`` 启动调试器，
并额外加载 ``user/_find`` 的符号，使 VS Code 能够识别 ``user/find.c`` 中的函数、变量和源代码行。

添加 find 调试配置
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

保留实验一的 ``.vscode/tasks.json``，其中 ``xv6build`` 任务使用
``make qemu-gdb CPUS=1`` 启动 QEMU。在 ``.vscode/launch.json`` 的
``configurations`` 数组中增加一个用户程序配置；也可以复制实验一的 ``xv6debug`` 配置后修改：

.. code-block:: json
   :caption: launch.json 中用于调试 find 的配置
   :emphasize-lines: 8, 21-25

   {
      "name": "xv6 user: find",
      "type": "cppdbg",
      "request": "launch",
      "program": "${workspaceFolder}/kernel/kernel",
      "cwd": "${workspaceFolder}",
      "MIMode": "gdb",
      "miDebuggerPath": "你自己的riscv64-unknown-elf-gdb路径",
      "preLaunchTask": "xv6build",
      "setupCommands": [
         {
            "description": "切换到 xv6 根目录",
            "text": "cd ${workspaceFolder}",
            "ignoreFailures": false
         },
         {
            "description": "连接 QEMU",
            "text": "source ${workspaceFolder}/.gdbinit",
            "ignoreFailures": false
         },
         {
            "description": "加载 find 用户程序符号",
            "text": "add-symbol-file ${workspaceFolder}/user/_find 0x0",
            "ignoreFailures": false
         }
      ]
   }

执行 ``which riscv64-unknown-elf-gdb``，并将 ``miDebuggerPath`` 改为实际路径。如果环境使用
``riscv64-linux-gnu-gdb``，也应填写对应程序的路径。当前课程版本的用户程序从 ``0x0`` 开始，
因此通过 ``add-symbol-file user/_find 0x0`` 加载 ``find`` 的调试符号。

设置断点并运行 find
^^^^^^^^^^^^^^^^^^^^^

#. 在 ``user/find.c`` 的递归函数中选择一条可执行语句设置断点，例如 ``open(path, 0)`` 所在行；
#. 在 VS Code 的“运行和调试”视图中选择 ``xv6 user: find``，按 ``F5``；
#. 等待 QEMU 中出现 xv6 shell 的 ``$`` 提示符；
#. 在 QEMU 控制台创建测试目录，然后执行 ``find . b``；
#. 命中断点后，在 VS Code 中使用 ``F10`` 单步跳过、``F11`` 单步进入、
   ``Shift+F11`` 单步跳出、``F5`` 继续运行。

不建议最初就在 ``main`` 设置断点。xv6 的 ``sh``、``find``、``echo`` 等用户程序拥有各自的
地址空间，却可能使用相同的低虚拟地址。QEMU GDB 不理解 xv6 的进程语义，断点可能在其他用户程序的
同一地址提前命中。优先选择 ``find`` 递归函数中具有代表性的代码行；若仍提前暂停，检查当前源码位置和
调用栈，确认不是 ``find`` 后再继续运行。

观察递归查找
^^^^^^^^^^^^^^^^^^^^^

命中断点后，建议在 ``WATCH`` 中加入：

.. code-block:: text

   path
   target
   fd
   st.type
   de.inum
   de.name
   buf
   p

变量名称应按自己的实现调整。每次 ``read`` 后观察 ``de.inum`` 和 ``de.name``，并在递归调用前检查
拼接后的路径。进入多层目录后，``CALL STACK`` 应显示多层递归，例如：

.. code-block:: text

   find("./a/aa", "b")
   find("./a", "b")
   find(".", "b")
   main(...)

这可以直接判断程序是在正常向下遍历，还是因为没有跳过 ``.``、``..`` 而反复进入相同目录。
``de.name`` 是固定长度数组，可能没有 ``\0`` 结尾；如果变量窗口显示的字符串异常，应结合字节内容判断，
不要仅依赖字符串预览。

调试 fork、exec 和 wait
^^^^^^^^^^^^^^^^^^^^^^^^^

调试 ``find -exec`` 时，建议在用户程序的以下位置设置断点：

* 调用 ``fork`` 之前；
* ``fork`` 返回后的父、子进程分支；
* 子进程调用 ``exec`` 之前及其失败处理代码；
* 父进程调用 ``wait`` 之前和之后。

在 ``WATCH`` 中观察 ``fork`` 返回值、匹配路径、命令参数数量和传给 ``exec`` 的参数数组。
父进程中 ``fork`` 返回子进程 PID，子进程中返回 0；``exec`` 成功后不会执行其下一行代码，
只有失败时才会进入错误处理分支。

GDB 控制的是 QEMU CPU，而不是绑定某个 xv6 PID。``fork`` 后，调度器可能先运行父进程，也可能先运行
子进程；断点也可能在两者中分别命中。使用 ``CPUS=1`` 可以减少多核交错，但不会固定进程调度顺序。
应结合 ``fork`` 返回值、当前调用栈和 QEMU 控制台输出判断正在观察哪个分支。

子进程成功执行 ``exec`` 后，其地址空间会替换为目标程序，例如 ``echo``。此时已经加载的
``user/_find`` 符号不再描述子进程中的用户代码；本实验只需观察到 ``find`` 发起 ``exec`` 即可。

断点未命中或显示为空心圆
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

在 VS Code 的 ``DEBUG CONSOLE`` 中可以执行：

.. code-block:: text

   -exec info files
   -exec info functions find
   -exec info address find
   -exec break find
   -exec bt

按以下顺序排查：

#. 确认 ``user/_find`` 已经重新编译，且时间戳晚于 ``user/find.c``；
#. 确认 ``add-symbol-file`` 使用的是 ``user/_find``，路径和加载地址正确；
#. 用 ``info functions find`` 检查 GDB 是否已经识别 ``find`` 函数；
#. 将断点移到确实会执行的语句，而不是声明、空行或被编译器优化掉的代码；
#. 确认已经在 xv6 shell 中实际运行 ``find``，而不是宿主机上的同名命令；
#. 若断点在其他用户程序中提前命中，继续执行并结合调用栈判断当前地址空间；
#. 若变量显示 ``<optimized out>`` 或源码行发生跳跃，参照实验一关于编译优化的说明。

打印调试与 GDB 并不冲突。可以先用少量日志确定问题发生在哪次递归或哪个进程分支，再在相应位置设置
断点观察变量，这通常比从 ``main`` 开始逐行单步更高效。

调试完成后删除临时打印和深度限制，并重新执行所有正常输入、错误输入和 ``-exec`` 测试。

.. _lab2-faq:

常见问题
~~~~~~~~~~~~~~~~~~~~~

进入 xv6 后找不到 ``find``
  检查 ``find`` 是否已加入 ``UPROGS``。退出 QEMU 后执行 ``make clean`` 和 ``make qemu``。

编译提示 ``implicit declaration of function``
  检查函数是否已在 ``user/user.h`` 或相应头文件中声明。xv6 不提供完整的 Linux C 标准库。

``find`` 输出乱码或文件名异常
  ``struct dirent`` 的名称可能没有 ``\0`` 结尾。复制名称后应正确构造 C 字符串。

``find`` 一直运行或重复访问相同目录
  检查是否跳过了 ``.`` 和 ``..``，以及递归时拼接的路径是否正确。

运行一段时间后无法打开文件
  检查每次 ``open`` 后是否在所有返回路径上调用了 ``close``，避免耗尽文件描述符。

``exec`` 执行失败
  检查命令是否存在、参数数组是否以 ``0`` 结束，以及参数数量是否超过 ``MAXARG``。

思考题
---------------------

#. 为什么递归读取目录时必须跳过 ``.`` 和 ``..``？
#. ``find`` 使用了哪些系统调用？它们分别完成什么工作？
#. 用户态的 ``find`` 为什么能够读取 xv6 文件系统中的目录信息？
#. 为什么 ``find`` 需要使用 ``fork`` 创建子进程，而不是直接在父进程中调用 ``exec``？
#. 为什么 ``exec`` 成功后不会返回？父进程为什么需要调用 ``wait``？

扩展阅读
~~~~~~~~~~~~~~~~~~~~~

* `MIT 6.1810: Xv6 and Unix utilities <https://pdos.csail.mit.edu/6.828/2026/labs/util.html>`_
* `xv6-riscv 官方源码 <https://github.com/mit-pdos/xv6-riscv>`_
* `xv6: a simple, Unix-like teaching operating system
  <https://pdos.csail.mit.edu/6.1810/2024/xv6/book-riscv-rev4.pdf>`_

阅读程序时，应优先查看 xv6 自带的 ``user/ls.c`` 和 ``user/ulib.c``。实验重点是理解用户程序
如何通过系统调用访问文件系统和管理进程，而不是记忆接口。

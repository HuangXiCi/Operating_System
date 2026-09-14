实验一：搭建实验环境
============================

本课程以 `xv6-riscv <https://github.com/mit-pdos/xv6-riscv>`_ 为教学平台，用于学习操作系统的基本原理。
xv6 是一个面向教学的类 Unix 小型操作系统。它保留了进程、虚拟内存、系统调用、
文件系统和设备驱动等操作系统的核心概念，同时代码规模远小于 Linux，适合阅读、修改和开展实验。

本次实验不要求修改 xv6 内核，主要任务包括搭建 Linux 实验环境、安装 RISC-V 交叉编译工具链和
QEMU、编译并启动 xv6，以及在 xv6 shell 中完成若干基本操作。

.. raw:: html

   <nav class="lab-progress" aria-label="实验流程">
     <a href="#lab1-goals"><span>01</span>实验目标</a>
     <a href="#lab1-tools"><span>02</span>实验工具</a>
     <a href="#lab1-source"><span>03</span>获取源码</a>
     <a href="#lab1-run"><span>04</span>启动 xv6</a>
     <a href="#lab1-gdb"><span>05</span>调试 xv6</a>
     <a href="#lab1-faq"><span>06</span>常见问题</a>
   </nav>

.. _lab1-goals:

实验目标
~~~~~~~~~~~~~~~~~~~~~

完成本次实验后，应能够：

* 使用 ``riscv64-unknown-elf`` 工具链交叉编译 RISC-V 程序；
* 使用 ``qemu-system-riscv64`` 启动 xv6；
* 识别 xv6 源码仓库的主要目录；
* 在 xv6 shell 中运行程序并进行简单的文件操作。

.. _lab1-report:

实验报告要求
----------------------

实验报告至少应包含以下内容：

#. 实验环境类型（Linux 真机、WSL 2 或虚拟机）、Ubuntu 版本和宿主机体系结构；
#. RISC-V 交叉编译器与 QEMU 的版本信息；
#. xv6 成功编译、启动并进入 shell 的截图；
#. xv6 shell 基础操作的命令、输出截图和对管道、重定向的解释；
#. 实验中遇到的问题、定位过程和解决方法；如果没有遇到问题，也应简要总结本次实验的收获。

.. raw:: html

   <div class="admonition mycaution">
      <p class="admonition-title">提交前检查</p>
      <p>橙色提示框是实验必做内容。截图应包含命令和输出，文字应清晰可辨；
      每幅截图均应配有操作目的和结果说明。</p>
   </div>


.. _lab1-tools:

实验工具
~~~~~~~~~~~~~~~~~~~~~

系统环境
----------------------

本课程建议优先使用 **Ubuntu 22.04 LTS 或 Ubuntu 24.04 LTS**。实验环境可部署在 Linux 真机、
Windows Subsystem for Linux（WSL 2）或 VMware 等虚拟机中，三种方式均可满足实验要求。

首次配置 Ubuntu 时，可根据网络状况选择合适的软件镜像源。

.. raw:: html

   <div class="admonition mydanger">
      <p class="admonition-title">正确配置软件镜像源</p>
      <p>合适的软件镜像源能够提高软件包的下载速度。配置前必须核对 Ubuntu 版本、代号及宿主机架构；
      使用与当前系统不匹配的软件源可能造成依赖冲突，强制安装不兼容的软件包还可能破坏系统环境。</p>
   </div>


完成软件镜像源配置后，在 Ubuntu 终端中执行：

.. code-block:: bash

   sudo apt update
   sudo apt install git build-essential


安装 RISC-V GNU 工具链
----------------------------

打开 `riscv-gnu-toolchain Releases
<https://github.com/riscv-collab/riscv-gnu-toolchain/releases>`_，选择适用于当前系统的正式 Release 并展开
``Assets``。根据所使用的 Ubuntu 版本下载下列文件之一：

* Ubuntu 22.04：``riscv64-elf-ubuntu-22.04-gcc.tar.xz``
* Ubuntu 24.04：``riscv64-elf-ubuntu-24.04-gcc.tar.xz``

必须选择 ``riscv64``、``elf`` 和 ``gcc`` 的组合。``elf`` 表示该工具链面向裸机环境，符合 xv6
内核和用户程序的构建需求。``riscv32``、``glibc``、``musl``、``picolibc`` 或 ``llvm`` 版本
不适用于本实验。Release 页面同时提供文件的 SHA-256 校验值，可用于检查下载文件的完整性：

.. code-block:: bash

   cd ~/Downloads
   sha256sum riscv64-elf-ubuntu-24.04-gcc.tar.xz

上述命令以 Ubuntu 24.04 为例；使用 Ubuntu 22.04 时应替换相应文件名。将终端输出与发布页中对应文件的
``sha256`` 值比较，两者应完全相同。

创建安装目录，并将压缩包解压到 ``~/.local/riscv``：

.. code-block:: bash

   mkdir -p ~/.local/riscv
   tar -xJf ~/Downloads/riscv64-elf-ubuntu-24.04-gcc.tar.xz \
       -C ~/.local/riscv --strip-components=1
   ls ~/.local/riscv/bin/riscv64-unknown-elf-gcc

使用 Ubuntu 22.04 时，将命令中的 ``24.04`` 改为 ``22.04``。``-xJf`` 表示解压 ``.tar.xz``；
``--strip-components=1`` 去掉压缩包最外层的 ``riscv`` 目录，使编译器最终位于
``~/.local/riscv/bin``。


安装 xPack QEMU RISC-V
----------------------------

打开 `xPack QEMU RISC-V Releases
<https://github.com/xpack-dev-tools/qemu-riscv-xpack/releases>`_，选择较新的正式版本，在 ``Assets``
中下载与 **宿主机架构** 对应的 GNU/Linux 压缩包。

发布页中的版本号可能已经更新，应以下载页面实际显示的最新正式版本为准。以下以
``9.2.4-1-linux-x64`` 为例，将压缩包解压到固定目录 ``~/.local/qemu-riscv``：

.. code-block:: bash

   mkdir -p ~/.local/qemu-riscv
   tar -xzf ~/Downloads/xpack-qemu-riscv-9.2.4-1-linux-x64.tar.gz \
       -C ~/.local/qemu-riscv --strip-components=1
   ls ~/.local/qemu-riscv/bin/qemu-system-riscv64


添加 PATH 环境变量
----------------------------

Linux shell 按照 ``PATH`` 中的目录顺序查找命令。使用文本编辑器打开 Bash 配置文件：

.. code-block:: bash

   vim ~/.bashrc

在文件末尾添加以下内容：

.. code-block:: bash

   export PATH="$HOME/.local/riscv/bin:$HOME/.local/qemu-riscv/bin:$PATH"

保存配置文件后，执行以下命令使配置在当前终端立即生效：

.. code-block:: bash

   source ~/.bashrc

该配置将在每次启动 Bash 时自动生效，无须重复添加。使用以下命令检查实际调用的程序：

.. code-block:: bash

   which riscv64-unknown-elf-gcc
   which qemu-system-riscv64

输出路径应分别位于 ``/home/<用户名>/.local/riscv/bin`` 和
``/home/<用户名>/.local/qemu-riscv/bin``。如果系统中已经安装其他同名工具，``which`` 的输出还可用于
确认当前优先调用的是否为本实验安装的版本。

验证安装结果
----------------------------

安装完成后执行：

.. code-block:: bash

   git --version
   make --version
   riscv64-unknown-elf-gcc --version
   riscv64-unknown-elf-ld --version
   qemu-system-riscv64 --version

若每条命令均能输出版本信息，且未出现 ``command not found``，则说明基本工具已经就绪。
具体版本号可能随发布版本更新而变化，无须与讲义示例完全一致。

.. raw:: html

   <div class="admonition mytodo">
      <p class="admonition-title">检查并记录实验环境</p>
      <p>依次执行 <code>cat /etc/os-release</code>、<code>uname -m</code>、
      <code>which riscv64-unknown-elf-gcc</code>、<code>which qemu-system-riscv64</code>、
      <code>riscv64-unknown-elf-gcc --version</code> 和 <code>qemu-system-riscv64 --version</code>。
      在实验报告中说明所采用的环境类型（Linux 真机、WSL 2 或虚拟机），并附上上述信息清晰可辨的终端截图。</p>
   </div>

.. _lab1-source:

获取 xv6-riscv
~~~~~~~~~~~~~~~~~~~~~

获取 MIT 官方 xv6-riscv 源码：

.. code-block:: bash

   git clone https://github.com/mit-pdos/xv6-riscv.git
   cd xv6-riscv


初识源码目录
-----------------------

在 xv6-riscv 仓库中执行 ``ls``，可查看以下主要目录和文件：

``kernel/``
  xv6 内核源代码，包括进程管理、虚拟内存、文件系统、系统调用、陷阱处理和设备驱动等。

``user/``
  xv6 用户态程序及其支持库，例如 shell、``ls``、``cat`` 和 ``echo``。这些程序运行在 xv6 中，
  不是 Ubuntu 中的同名程序。

``mkfs/``
  构造 xv6 文件系统镜像的工具。

``Makefile``
  描述如何编译内核和用户程序、生成文件系统镜像，以及如何启动 QEMU。

``README``
  xv6 项目简介和构建说明。


.. _lab1-run:

编译并启动 xv6
~~~~~~~~~~~~~~~~~~~~~

确认当前目录是 xv6-riscv 仓库根目录，然后执行：

.. code-block:: bash

   make qemu

Make 将调用 ``riscv64-unknown-elf`` 工具链编译内核和用户程序，生成文件系统镜像，再启动
``qemu-system-riscv64``。首次编译会输出较多构建命令。成功启动后，终端将显示类似以下内容：

.. code-block:: text

   xv6 kernel is booting

   hart 1 starting
   hart 2 starting
   init: starting sh
   $

``hart`` 是 RISC-V 对硬件线程的称呼。最后的 ``$`` 是 xv6 shell 的提示符，表示 xv6 已成功启动，
正在等待命令。不同源码版本的启动信息可能略有差异。

.. raw:: html

   <div class="admonition mytodo">
      <p class="admonition-title">首次启动 xv6</p>
      <p>执行 <code>make qemu</code>，等待 xv6 启动并出现 <code>$</code> 提示符。
      将包含 <code>xv6 kernel is booting</code> 和 shell 提示符的终端截图放入实验报告。</p>
   </div>

退出 xv6 和 QEMU
-----------------------

xv6 默认不提供 ``shutdown`` 命令。退出 QEMU 时，应先按 ``Ctrl+a``，松开后再按 ``x``。
这两个按键操作需要依次完成，而非同时按下三个按键。退出后将返回宿主机 shell。

.. raw:: html

   <div class="admonition mydanger">
      <p class="admonition-title">避免强制关闭正在写入磁盘镜像的 QEMU</p>
      <p>强制关闭 QEMU 可能导致文件系统镜像处于不一致状态，因此应按照上述按键顺序正常退出。
      如果终端未响应，应先确认当前环境是 xv6 shell、QEMU 控制台还是 Ubuntu shell。</p>
   </div>


初识 xv6 shell
-----------------------------

重新执行 ``make qemu`` 进入 xv6，并在 shell 提示符后执行：

.. code-block:: sh

   ls

该命令显示 xv6 文件系统根目录的内容。目录中的 ``cat``、``echo``、``grep``、``ls``、``mkdir``、
``rm``、``sh`` 和 ``wc`` 等都是编译进 xv6 文件系统镜像的用户程序。

命令、参数与进程
-----------------------

在提示符后执行：

.. code-block:: sh

   echo hello xv6
   echo one two three | wc

第一条命令启动 ``echo`` 程序并传入两个参数。第二条命令使用管道 ``|``；shell 将创建管道并启动
``echo`` 与 ``wc`` 两个进程，将前一个程序的输出连接至后一个程序的输入。

文件与目录
-----------------------

继续执行以下操作：

.. code-block:: sh

   echo hello > hello.txt
   cat hello.txt
   cat README
   grep xv6 README
   cat README | grep riscv
   wc README


``>`` 将程序的标准输出重定向到文件；``cat`` 读取并显示文件；``grep`` 查找包含指定字符串的行；
``wc`` 统计行数、单词数和字节数。xv6 工具仅实现教学所需的基本功能，其参数和选项少于
Ubuntu 中的 GNU 工具，不能替代日常 Linux 环境中的完整命令集。

.. raw:: html

   <div class="admonition mytodo">
      <p class="admonition-title">完成 xv6 shell 基础操作</p>
      <p>按顺序完成本节的目录创建、输出重定向、文件查看、字符串查找、统计和删除操作。
      在实验报告中附上完整操作及输出的截图，并结合命令执行结果说明
      <code>|</code> 与 <code>&gt;</code> 的作用。</p>
   </div>

.. raw:: html

   <div class="admonition myquestion">
      <p class="admonition-title">思考：Ubuntu shell 与 xv6 shell 是否相同？</p>
      <p>Ubuntu 和 xv6 中都能输入 <code>ls</code>、<code>echo</code> 等命令。
      请比较两种环境中的可执行文件，并说明它们所对应的指令集。</p>
   </div>

宿主机与 xv6 文件系统
-----------------------

在 xv6 中创建的 ``hello.txt`` 不会直接出现在 Ubuntu 当前目录中。xv6 使用的是 QEMU 挂载的
``fs.img`` 文件系统镜像。Ubuntu 负责保存这个镜像文件，而 xv6 内核负责解释镜像内部的目录、
文件和数据块。后续文件系统实验将进一步分析其实现机制。

修改 ``user/`` 中的程序并重新执行 ``make qemu`` 时，Makefile 会将相应程序重新编译并写入
文件系统镜像。修改 ``kernel/`` 中的代码则会改变下一次启动的 xv6 内核。

.. _lab1-gdb:

VS Code 调试 xv6
~~~~~~~~~~~~~~~~~~~~~

使用命令行 GDB 调试 xv6 内核和用户程序具有一定复杂度。本节采用 VS Code 组织调试流程，以便直观地
观察源代码、断点、变量、寄存器和调用栈。

安装插件
-----------------------

.. container:: tutorial-step

   .. rubric:: 1. 安装 C/C++ 扩展

   在 VS Code 的扩展视图中搜索并安装由 Microsoft 发布的 ``C/C++`` 扩展。

   .. figure:: ../picture/lab1/extension.png
      :alt: VS Code 中由 Microsoft 发布的 C/C++ 扩展详情页
      :align: center
      :width: 92%
      :figclass: tutorial-figure tutorial-figure--wide

      确认发布者为 Microsoft，并完成扩展安装。

.. admonition:: 远程环境安装
   :class: mycomment tutorial-note

   通过 VS Code 远程连接 WSL 2 或虚拟机时，应在 **远程环境** 中安装扩展；仅在本地环境中安装并不能
   为远程工作区提供相应功能。

   .. figure:: ../picture/lab1/ssh-extension.png
      :alt: VS Code 远程环境中的 C/C++ 扩展安装入口
      :align: center
      :width: 48%
      :figclass: tutorial-figure tutorial-figure--compact

      扩展面板中显示远程主机名称和 ``Installed``，表示扩展已安装到当前远程环境。


添加调试配置
-----------------------

在 VS Code 中打开 xv6-riscv 仓库根目录，创建 ``.vscode`` 目录，再创建 ``launch.json`` 和
``tasks.json`` 文件，分别写入以下配置。

.. code-block:: json
   :caption: launch.json
   :emphasize-lines: 13

   {
      "version": "0.2.0",
      "configurations": [
         {
               "name": "xv6debug",
               "type": "cppdbg",
               "request": "launch",

               "program": "${workspaceFolder}/kernel/kernel",
               "cwd": "${workspaceFolder}",

               "MIMode": "gdb",
               "miDebuggerPath": "/opt/riscv/bin/riscv64-unknown-elf-gdb",

               "preLaunchTask": "xv6build",

               "setupCommands": [
                  {
                     "description": "切换到 xv6 根目录",
                     "text": "cd ${workspaceFolder}",
                     "ignoreFailures": false
                  },
                  {
                     "description": "加载 xv6 自动生成的 GDB 配置",
                     "text": "source ${workspaceFolder}/.gdbinit",
                     "ignoreFailures": false
                  }
               ],

               "logging": {
                  "engineLogging": true,
                  "trace": true,
                  "traceResponse": true
               }
         }
      ]
   }


``miDebuggerPath`` 应根据 RISC-V 工具链的实际安装位置进行修改。执行
``which riscv64-unknown-elf-gdb`` 可查询 GDB 的实际路径。

.. code-block:: json
   :caption: tasks.json

   {
      "version": "2.0.0",
      "tasks": [
         {
            "label": "xv6build",
            "type": "shell",
            "isBackground": true,
            "command": "make qemu-gdb CPUS=1",
            "problemMatcher": [
               {
                  "pattern": [
                        {
                           "regexp": ".",
                           "file": 1,
                           "location": 2,
                           "message": 3
                        }
                  ],
                  "background": {
                        "beginsPattern": ".*Now run 'gdb' in another window.",
                        "endsPattern": "."
                  }
               }
            ]
         }
      ]
   }


.. admonition:: 调试时建议使用单核配置
   :class: myhint

   xv6 默认可以启动多个 RISC-V hart。多核并发执行时，不同 hart 可能交替命中断点，调用栈和寄存器
   视图也会随当前选中的执行线程发生变化，从而增加初次调试时的观察难度。因此，调试阶段建议通过
   ``CPUS=1`` 将 QEMU 配置为单核模式：

   .. code-block:: bash

      make qemu-gdb CPUS=1

   上述 ``tasks.json`` 已采用该配置。单核模式仅用于降低调试过程的复杂度，不改变 xv6 的主要功能；
   分析多核启动、调度或锁机制时，应再恢复多核配置。


.. container:: tutorial-step

   .. rubric:: 3. 允许在任意文件中设置断点

   为了在汇编代码中设置断点，在 VS Code 设置中搜索 ``breakpoint``，然后勾选
   ``Allow setting breakpoints in any file.``。启用该选项后，即可在汇编源文件中设置断点。

   .. figure:: ../picture/lab1/breakpoint.png
      :alt: VS Code 中允许在任意文件内设置断点的选项
      :align: center
      :width: 72%
      :figclass: tutorial-figure tutorial-figure--medium

      启用允许在任意文件中设置断点的选项。


启动调试与设置断点
-----------------------

调试前先在编辑器中打开 ``kernel/main.c``。单击代码行号左侧的空白区域，出现红色圆点即表示已经设置断点；
再次单击可以删除断点，也可以按 ``F9`` 切换当前行的断点。首次调试时，可在 ``main`` 函数内的第一条
可执行语句处设置断点。

可按 ``F5`` 启动调试，也可打开左侧的 **运行和调试** 视图并选择 ``xv6debug``。VS Code 将依次完成以下操作：

#. 执行 ``preLaunchTask``，编译 xv6 并以 ``make qemu-gdb CPUS=1`` 启动单核 QEMU；
#. 启动 RISC-V GDB，加载 ``kernel/kernel`` 中的符号和源代码行信息；
#. 根据生成的 ``.gdbinit`` 连接 QEMU GDB 服务器；
#. 恢复 xv6 的执行，并在命中断点时暂停。

程序暂停后，VS Code 会用黄色箭头标出 **下一条即将执行的语句**。黄色箭头所在行尚未执行；完成单步操作后，
箭头将移动到下一处暂停位置。如果断点显示为空心灰色圆点，通常表示符号尚未加载、文件路径不匹配，
或者该行没有可以设置断点的机器指令。

.. admonition:: xv6 使用优化编译
   :class: myhint

   xv6 默认启用编译优化。调试时可能出现源代码行跳跃、执行顺序与源码不一致，或者变量显示
   ``<optimized out>``。这通常并非调试器故障，而是源代码语句被合并、重排或优化所致。

调试工具栏按钮
-----------------------

调试开始后，窗口上方会出现调试工具栏。该工具栏用于控制 **被调试的 xv6** 的执行状态。

.. list-table:: VS Code 调试工具栏
   :header-rows: 1
   :widths: 20 18 62

   * - 按钮
     - 快捷键
     - 作用与区别
   * - 继续 / 暂停（▶ / ‖）
     - ``F5``
     - 暂停时，**继续** 将使程序运行至下一个断点、异常或手动暂停位置。在部分 QEMU/GDB 组合中，暂停请求可能无法立即生效，此时可通过预先设置断点使程序暂停。
   * - 单步跳过（Step Over）
     - ``F10``
     - 执行当前源代码行。如果该行包含函数调用，则将该函数调用作为一个整体执行，不进入函数内部。
   * - 单步进入（Step Into）
     - ``F11``
     - 执行当前行，并在可能时进入被调用函数内部，用于跟踪函数的具体实现。
   * - 单步跳出（Step Out）
     - ``Shift+F11``
     - 继续执行当前函数剩余部分，在当前函数返回到调用者后再次暂停。
   * - 重启（Restart）
     - ``Ctrl+Shift+F5``
     - 结束当前调试会话并使用相同配置重新启动。对 xv6 而言通常也需要重新启动 QEMU。
   * - 停止（Stop，■）
     - ``Shift+F5``
     - 结束 GDB 调试会话。它与 **暂停** 不同：暂停后仍可继续调试，停止后需要重新启动调试会话。

``F10`` 与 ``F11`` 的主要区别在于是否进入被调用函数。例如，当前行为 ``p = allocproc();`` 时，
``F10`` 会完整执行 ``allocproc`` 并停在调用者的下一行；``F11`` 则会进入 ``allocproc``，以便继续观察
其内部语句。若进入无须分析的函数，可按 ``Shift+F11`` 返回调用者。

.. admonition:: 停止调试与退出 QEMU
   :class: mycaution

   VS Code 的停止按钮主要用于结束 GDB 会话。xv6 正在运行时直接停止，QEMU 可能继续占用
   ``fs.img`` 和 GDB 端口。建议先按 **暂停**，等待程序停下后再按 **停止**。如果 QEMU 仍未退出，
   应在 QEMU 终端中依次按 ``Ctrl+a``、``x``，并确认 QEMU 已正常退出。

认识调试侧栏
-----------------------

程序暂停后，左侧 **运行和调试** 视图中的主要区域如下：

``VARIABLES（变量）``
  显示当前栈帧中可见的局部变量、函数参数和部分全局变量。展开结构体或指针可以继续查看成员。

``WATCH（监视）``
  用于持续观察表达式。单击 ``+`` 后可以添加 ``p->pid``、``mycpu()->noff`` 或 ``ticks`` 等表达式；
  每次程序暂停时，VS Code 都会重新计算其值。表达式必须在当前栈帧中有效。

``CALL STACK（调用堆栈）``
  显示函数调用路径。选择不同栈帧，可以查看对应调用层级中的参数和局部变量。变量值、表达式求值
  和部分寄存器显示都与当前选中的栈帧有关。

``BREAKPOINTS（断点）``
  集中启用、禁用或删除断点。右键单击代码行左侧的断点并选择 **编辑断点**，还可以设置条件断点；例如
  ``p->pid == 2`` 表示仅在条件成立时暂停。

``DEBUG CONSOLE（调试控制台）``
  可以计算 C 表达式，也可以向底层 GDB 发送命令。直接输入 ``p->pid`` 会求表达式的值；执行原生
  GDB 命令时需要写成 ``-exec <GDB 命令>``，例如 ``-exec info registers``。变量、寄存器和内存的
  检查应在程序暂停状态下进行。

查看变量值
-----------------------

程序暂停后，可将鼠标悬停在源代码中的变量上以查看其值。对于需要持续观察的表达式，应将其添加到
``WATCH``。此外，还可以在 ``DEBUG CONSOLE`` 中输入表达式，或者使用 GDB 的 ``print`` 命令：

.. code-block:: text
   :caption: 在 VS Code 的 DEBUG CONSOLE 中查看变量

   p->pid
   -exec print p->pid
   -exec print *p
   -exec p/x p
   -exec ptype struct proc

其中，``print``（可简写为 ``p``）按照变量类型打印值；``p/x`` 用十六进制显示；``print *p`` 对指针
解引用并打印所指向的对象；``ptype`` 查看类型定义。如果 GDB 提示 ``No symbol ... in current context``，
应先在 ``CALL STACK`` 中确认当前选中的栈帧是否包含该变量。

地址和值是两个不同概念。以下命令分别打印变量 ``ticks`` 的地址、当前值，以及该地址处保存的原始内存：

.. code-block:: text

   -exec p/x &ticks
   -exec p/x ticks
   -exec x/wx &ticks

查看 RISC-V 寄存器
-----------------------

部分版本的 C/C++ 扩展会在变量区域中显示 ``REGISTERS``。若当前界面未显示该区域，可以在
``DEBUG CONSOLE`` 中使用以下命令：

.. code-block:: text
   :caption: 常用寄存器查看命令

   -exec info registers
   -exec info registers pc sp ra s0 a0 a1 a7
   -exec p/x $pc
   -exec p/x $sp
   -exec x/i $pc

常用 RISC-V 寄存器含义如下：

.. list-table:: xv6 调试中常见的 RISC-V 寄存器
   :header-rows: 1
   :widths: 18 82

   * - 寄存器
     - 含义
   * - ``pc``
     - 程序计数器，保存下一条将要执行的机器指令地址。
   * - ``sp``
     - 栈指针，指向当前执行上下文的栈顶附近。
   * - ``ra``
     - 返回地址寄存器，函数返回时通常跳转到该地址。
   * - ``s0`` / ``fp``
     - 保存寄存器，通常也被用作当前栈帧的帧指针。
   * - ``a0``～``a7``
     - 参数/返回值寄存器；``a0``、``a1`` 等传递参数，``a0`` 通常也保存函数返回值，用户态系统调用号通常放在 ``a7``。

``$pc`` 中的 ``$`` 表示 GDB 寄存器变量。``x/i $pc`` 会反汇编并显示 ``pc`` 指向的下一条指令，
这在调试 ``entry.S``、``swtch.S`` 或陷阱入口代码时尤其有用。

查看内存地址处的值
-----------------------

GDB 使用 ``x/nfu address`` 检查原始内存，其中 ``x`` 表示 examine。各字段含义如下：

* ``n`` 是显示数量；
* ``f`` 是显示格式，例如 ``x`` 为十六进制、``d`` 为有符号十进制、``u`` 为无符号十进制、
  ``i`` 为机器指令、``s`` 为字符串；
* ``u`` 是每个单元的大小：``b`` 为 1 字节、``h`` 为 2 字节、``w`` 为 4 字节、``g`` 为 8 字节。

在 64 位 RISC-V 中，常用命令如下：

.. code-block:: text
   :caption: 内存与指令查看示例

   -exec x/16gx $sp
   -exec x/16wx $sp
   -exec x/10i $pc
   -exec x/gx 0x80000000
   -exec x/s path

``x/16gx $sp`` 从栈指针开始，以十六进制显示 16 个 8 字节单元；``x/16wx $sp`` 显示 16 个
4 字节单元；``x/10i $pc`` 显示接下来的 10 条机器指令；``x/gx 0x80000000`` 读取指定地址处的
一个 8 字节值；如果 ``path`` 是有效的字符指针，``x/s path`` 会将其指向的内存解释为字符串。

.. admonition:: print 与 x 的区别
   :class: myquestion

   ``print`` 根据 C 类型解释表达式，适合查看变量、结构体和指针；``x`` 不依赖变量类型，直接从指定
   地址读取原始内存。``p/x p`` 显示指针 ``p`` 本身保存的地址，而 ``x/gx p`` 显示该地址处的
   8 字节内容。调试时应注意区分变量地址与该地址处保存的值。

相关内容可参阅 `VS Code 官方调试文档 <https://code.visualstudio.com/docs/editor/debugging>`_、
`VS Code C/C++ 调试文档 <https://code.visualstudio.com/docs/cpp/cpp-debug>`_ 和
`GDB 内存检查文档 <https://sourceware.org/gdb/current/onlinedocs/gdb.html/Memory.html>`_。


.. _lab1-faq:

常见问题
~~~~~~~~~~~~~~~~~~~~~

``Unable to locate package``
  确认系统版本为 Ubuntu 22.04/24.04，并确认 ``sudo apt update`` 已成功执行。如果无法访问软件源，
  应检查 Linux 环境的网络连接。

``tar: ... Cannot open: No such file or directory``
  压缩包不在命令指定的位置，或者版本号、Ubuntu 版本、宿主机架构与文件名不一致。执行
  ``ls ~/Downloads`` 查看实际文件名和位置，再修改解压命令；命令中的文件名必须与实际下载文件一致。

``riscv64-unknown-elf-gcc: command not found``
  执行 ``ls ~/.local/riscv/bin`` 检查工具链是否正确解压；确认 ``~/.bashrc`` 中的 ``PATH`` 没有
  拼写错误，然后执行 ``source ~/.bashrc`` 和 ``which riscv64-unknown-elf-gcc``。

``qemu-system-riscv64: command not found``
  执行 ``ls ~/.local/qemu-riscv/bin`` 检查 QEMU 是否正确解压；确认 ``~/.bashrc`` 中的 ``PATH``
  配置已生效，再执行 ``which qemu-system-riscv64``。

``cannot execute binary file: Exec format error``
  通常表示下载的预编译程序与宿主机架构不匹配。重新执行 ``uname -m``，并核对下载文件是
  ``linux-x64`` 还是 ``linux-arm64`` 版本；RISC-V 工具链同样必须能够在当前宿主机架构上运行。

执行 ``make qemu`` 后仅显示 ``$``
  ``$`` 是 xv6 shell 提示符，表示 xv6 已成功启动并正在等待命令输入，并非程序阻塞。

无法判断当前 shell 环境
  xv6 shell 通常仅显示简单的 ``$`` 提示符，且可用命令较少；Ubuntu shell 的提示符通常包含用户名、
  主机名和当前路径。可执行 ``uname -a`` 进行判断：Ubuntu 提供该命令，而标准 xv6 通常不包含该程序。

重新编译后行为异常
  退出 QEMU，在 xv6-riscv 根目录执行 ``make clean``，再执行 ``make qemu``。QEMU 运行期间不应
  删除构建产物。

编程作业
~~~~~~~~~~~~~~~~~~~~~

.. raw:: html

   <div class="admonition mytodo">
      <p class="admonition-title">printf</p>
      <p>使用printf编写最简单的Hello World程序
      <code>strace -o trace.log ./your_program</code> 使用strace找到与程序行为对应的系统调用，并尝试理解：</p>
      <p>1. 程序中的 printf() 最终对应了哪个系统调用？</p>
      <p>2. 文件描述符 0、1、2 分别通常代表什么？</p>
      <p>3. 修改程序，打印 10 次字符串，观察 write() 调用了几次，如果printf时不加入'\n'，而是加入空格进行分隔，现在write() 调用了几次？为什么呢？查阅 stdout的缓冲机制。</p>
      <p>4. 为什么printf函数支持有任意个参数，实现机制是怎样的？</p>
   </div>


.. raw:: html

   <div class="admonition mytodo">
      <p class="admonition-title">fork</p>
      <p>使用 fork() 创建两个进程，让父进程和子进程分别对同一个变量 counter 执行100000次 ++ 操作，父进程使用 wait() 等待子进程结束，最后分别打印父进程和子进程中 counter 的值</p>
      <p>1. 最终两个进程打印出的 counter 分别是多少？</p>
      <p>2. 将本实验与之前的两个线程同时执行 counter++ 作业3进行比较。为什么多线程程序会产生数据竞争，而这里父进程和子进程却不会因为 counter++ 而互相干扰？</p>
      <p>3. 如果两个进程确实需要共享数据，应该怎么办？自行查阅“进程间通信 IPC”，列举至少两种方法</p>
   </div>


扩展阅读
~~~~~~~~~~~~~~~~~~~~~

* `xv6-riscv 官方源码 <https://github.com/mit-pdos/xv6-riscv>`_
* `MIT 6.1810 操作系统工程课程 <https://pdos.csail.mit.edu/6.1810/>`_
* `xv6: a simple, Unix-like teaching operating system
  <https://pdos.csail.mit.edu/6.1810/2024/xv6/book-riscv-rev4.pdf>`_

阅读源码时，如遇不熟悉的函数或机制，应优先查阅相关手册和对应源文件。操作系统实验需要逐步形成
RTFM（阅读手册）和 RTFSC（阅读源代码）的习惯。

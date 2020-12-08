# MIT-6.828: OS Engineering

[good looking notes in Notion](https://www.notion.so/MIT-6-828-OS-Engineering-48dc7bf0f820401bafc6ae821185a503)

# How to construct an OS?

---

这一节既是笔者对整个课程Lab的总结，也是笔者认为，面对一个操作系统，怎么看待其组成的一个维度。课程最终是实现了一个操作系统，那么操作系统应该提供哪些功能，这些功能之间是如何相互配合的？希望这个脉络有助于读者更好地了解此课程，也希望能够帮助笔者更好地复习。

操作系统需要提供**内存管理**功能：

- 从Lab 2中，读者应知晓CPU访问内存（忽略TLB），需要经过MMU参与的地址转换：`逻辑地址→线性地址→物理地址`，在地址总线上设定物理地址后，传输给RAM，RAM再通过数据总线将内存中存储的值返回给CPU。其中，逻辑地址→线性地址的转换，需要段表的参与，线性地址→物理地址的转换，需要页表的参与。除了提供地址转换作用以外，段表提供了特权级的功能（用于后续中断机制的特权管理），页表提供了访问RAM的权限功能（PTE_P、PTE_W、PTE_U等）。**完成段表和页表的设计，是完成内存管理的一部分。**
- 操作系统**以页为粒度管理内存**，好处是减少外部碎片。根据第一步，操作系统需要提供，创建、修改和删除`虚拟地址→物理地址映射关系`的功能。

操作系统需要提供**进程管理**功能：

- 操作系统需要支持运行用户程序。**每一个程序都运行在，拥有独有的虚拟地址空间的进程中**（独有的页表），这样做的好处是，用户程序之间实现了地址的隔离（保障了安全，也让程序编写无需考虑其他程序是否占用了某些地址）。**支持进程的调度，是进程管理的一部分。**
- **进程的权限需要被限制**，防止恶意进程破坏操作系统（破坏内核数据结构、操纵硬件）。通过段表，赋予用户进程对应的特权，并将内核所在页表项设置不许用户访问，那么用户进程在用户态无法访问内核数据结构。同样的，用户态下，用户进程也无法修改寄存器（EFLAG中的IOPL）和页表，无法使用PIO访问硬件，无法修改MMIO机制。
- **进程需要请求内核服务，内核通过系统调用的方式提供服务。**通过系统调用的方式，内核只提供内核定制的，良构的服务，避免将特权赋予用户进程，防止恶意进程破坏。

操作系统需要支持**SMP**（symmetric multiprocessor）：

- 操作系统需要支持多CPU，让多个进程分别在不同的CPU上同时运行，实现加速的目的。从Lab 4中，读者应知晓启动SMP的过程，需要硬件PIC、LAPIC和IOAPIC的相互配合。BSP（Bootstrap Processor）通过LAPIC向其他的AP（Assitant Processor）发送处理器间中断，使AP执行重启并执行指定程序，参与到进程调度中来。**了解PCI、LAPIC、IOAPLIC以及AP执行的指定程序，是支持SMP的一部分。**
- **多个CPU同时运行内核程序，需要对内核中的数据结构进行并发控制。**JOS采用了最简单的Big Kernel Lock来保护整个内核，也采用了为每个CPU配置内核栈的方式来保护内核栈。

操作系统需要支持**文件系统**：

- 采用外核的设计架构，让文件系统服务运行在一个用户进程——File Server中，进程被赋予执行IO指令的权限（EFLAG中的IOPL）。**File Server需要访问硬盘，需要编写硬盘的驱动程序。**
- 文件打开、修改和删除功能。这些功能都离不开一套逻辑自洽的文件系统。从Lab 5中，知晓文件是由元数据和数据组成，目录是包含文件的文件。课程设计的文件系统，主要由超级块（提供文件系统的元数据）、bitmap（标记硬盘块占用状态）和数据区组成。**设计并实现一套文件系统（还需要理解struct Fd、struct File和struct OpenFile之间的关联）。**
- **其他进程需要File Server提供文件IO服务，因此需要一套进程间通信IPC机制。**课程设计的IPC机制，支持传输一个32位整数和大小为内存页的数据。接收方在内核管理进程的数据结构中，告知其处于接受IPC消息的状态，告知其用于接受数据的地址。调用方负责将数据映射到接收方所告知的地址中。

操作系统需要支持**网络服务**：

- 操作系统需要至少支持TCP/IP的网络协议栈，这一部分可以使用开源的lwIP，但数据链路层和物理层，是由硬件实现的。同样的，采用微内核的设计架构（这里不说外核，是因为笔者发现，网卡驱动是编写在内核里的），网络服务运行在一个用户进程——Network Server中。**需要编写网卡驱动，使操作系统支持数据链路层和物理层协议。**
- 操作系统实现BSD socket接口，使用文件系统中的struct Fd来管理socket，支持用户程序使用文件IO的方式，将应用层数据发送到网络上的逻辑对端。**为此，用户进程需要将应用层数据通过IPC机制发送给Network Server，Network Server将通过lwIP和网卡驱动，为应用层数据添加头部，使其在网络上传输。**
- 网络协议中，不乏支持全双工的网络协议。因此，Network Server既要处于能够接收来自其他用户进程发送数据包的状态，同时也要处于能够接受来自网卡数据包的状态。遗憾的是，前者的行为软件可以定义，通过IPC机制实现；后者的行为由网卡硬件定义，无法让网卡实现IPC机制。因此，**需要额外创建Input进程处于接收来自网卡数据包的状态，并在接收数据包后向Network Server发送IPC消息。这样，Network Server只需要处于能够接受其他用户进程发送数据包的状态即可。同理，引入Input进程也是必须的。**

# Lab Part

---

[Lab 1: Booting a PC](https://www.notion.so/Lab-1-Booting-a-PC-1f17fbd7e1ea4645a124419f89276fa3)

[Lab 2: Memory Management](https://www.notion.so/Lab-2-Memory-Management-7cea2ce8c3c7482aab080ceb4cccae81)

[Lab 3: User Environments](https://www.notion.so/Lab-3-User-Environments-e58ee6fc4c3d41a0bf4ab5cc91941ea2)

[Lab 4: Preemptive Multitasking](https://www.notion.so/Lab-4-Preemptive-Multitasking-48df5bed262d419296619fde90d19607)

[Lab 5: File system, Spawn and Shell](https://www.notion.so/Lab-5-File-system-Spawn-and-Shell-1b8a651141fc49f19267b4416074a60e)

[Lab 6: Network Driver](https://www.notion.so/Lab-6-Network-Driver-0adb6574c7e045369bb95d9f09cd0d4f)

# HomeWork Part

---

[Homework: boot xv6](https://www.notion.so/Homework-boot-xv6-44375c3949624750922a94b593e157f5)

[Homework: shell](https://www.notion.so/Homework-shell-8e29506897524d24ac3862cef5aa428b)

[Homework: xv6 system calls](https://www.notion.so/Homework-xv6-system-calls-c8e92eac3aa4465d85f893f4ecea5b6f)

[Homework: xv6 lazy page allocation](https://www.notion.so/Homework-xv6-lazy-page-allocation-60f9d26f646545968b33e706bbf8d883)

[Homework: xv6 CPU alarm](https://www.notion.so/Homework-xv6-CPU-alarm-f3d2bb0973c94500b19075ff1fb1bebd)

[Homework: Threads and Locking](https://www.notion.so/Homework-Threads-and-Locking-bcb6406590544e7e8c120ddc1601ff27)

[Homework: xv6 locking](https://www.notion.so/Homework-xv6-locking-b66ce8f27f3f4607bc2a66e21bb4aa88)

[Homework: User-level threads](https://www.notion.so/Homework-User-level-threads-2177213b3ebd45b3ad7dd7bfb2834293)

[Homework: Barriers](https://www.notion.so/Homework-Barriers-ae9887dc99d64e15a9016f3e7d7faaf3)

[Homework: bigger files for xv6](https://www.notion.so/Homework-bigger-files-for-xv6-f7f471c1b22141bdbe4e46cedc2cabd3)

[Homework: xv6 log](https://www.notion.so/Homework-xv6-log-8c2c428f0c1e4e7195c5b304ce96367f)

# 后记

完成这个课程的所有Lab和部分作业，花了笔者近3个月的时间。一方面是笔者第一次接触系统编程和硬件驱动程序的编写，另一方面是笔者自认博客内容还是写的比较丰富的，有利于以后回头看这系列博客时，可以尽量多地回忆起课程的内容。

**同样不错的参考资料：**

[MIT6.828-神级OS课程-要是早遇到，我还会是这种 five 系列](https://zhuanlan.zhihu.com/p/74028717)

[操作系统_bysui的博客-CSDN博客](https://blog.csdn.net/bysui/category_6232831.html)
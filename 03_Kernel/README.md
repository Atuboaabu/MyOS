# 内核编写
## 保护模式&实模式
### 1、实模式和保护模式概述
在 x86 处理器架构中，保护模式和实模式是两种不同的运行模式，各自有不同的特性和应用场景。  
#### 实模式
实模式是 x86 处理器在启动时的默认模式，它模拟了早期的 8086 处理器环境，主要用于简单的操作系统和早期的 DOS 程序。  

**特点：**  
- 内存寻址：使用 20 位地址总线，可访问 1 MB 的内存（2^20 = 1MB）。  
- 分段寻址：使用段寄存器（如 CS, DS, SS, ES）进行分段寻址。内存地址由段寄存器和偏移量计算得出，段地址左移 4 位加上偏移量形成物理地址。  
- 无内存保护：程序可以直接访问全部内存，没有权限控制或内存隔离，可能导致不同程序相互干扰。  
- 简单环境：不支持虚拟内存、硬件级别的进程隔离、特权级等功能，只能运行单任务，适合小型或早期的系统。  
**用途：**  
- 主要用于引导程序、BIOS、DOS 系统以及一些早期的 16 位程序。  
- 引导加载器通常在实模式下启动操作系统（如 MBR 的执行）。  

#### 保护模式
保护模式是 80286 及以上处理器支持的一种模式，提供了更多的内存管理和保护机制。现代操作系统（如 Windows、Linux）都在保护模式下运行。  

**特点：**  
- 32 位地址空间：支持 32 位地址总线，可访问 4 GB 内存（2^32 = 4GB）。
- 段选择器和页表：使用段选择器和页表进行内存管理。段寄存器在保护模式下不直接存储段地址，而是作为段选择器，指向段描述符表，进一步划分内存。
- 内存保护：每个段都有访问权限和限长设置，可以有效保护内存区域，防止非法访问。
- 多任务和特权级：支持硬件级别的进程隔离和多任务调度。引入特权级（0-3 级），内核程序一般在 0 级，应用程序在 3 级，从而提供安全隔离。
- 虚拟内存：通过页表支持虚拟内存管理，使物理内存可以被映射到更大的虚拟地址空间，提高内存管理的灵活性。
**用途:**  
- 现代操作系统内核都在保护模式下运行，以实现多任务、安全隔离和虚拟内存管理。
- 支持复杂系统的软件执行，包括现代应用程序和服务。

#### 对比
![实模式&保护模式对比](mode_comparison.png)  
实模式主要用于系统启动阶段，保护模式则是现代系统运行的主要模式，能够充分利用处理器的高级功能。  
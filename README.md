# 操作系统编程作业

## 一、设计一个按照高响应比优先调度算法实现作业与进程调度的程序
1.当作业到达系统后，被调度进程调入后备队列。在后备队列中按照高响应比优先调度算法排序等待被调入就绪队列。

2.高响应比计算公式：优先权 = $\frac{等待时间 + 要求服务时间}{要求服务时间}$

3.规定就绪队列中只能容纳5个进程，且为单处理机，即在同一时刻只能将一个进程进入运行队列。也就是每次将就绪队列队首进程调入运行队列。

4.基于轮转调度算法，每一个时间片只能执行一个进程。

5.每个作业有到达系统的时间、要求执行时间。现求出每个作业的开始运行时间和结束时间，以及在每个时间片内如有进程执行打印每个时间片及其执行进程。
## 二、银行家算法实现
1.假设系统此时有5个进程，每个进程有其获得的资源情况。
2.每个进程有Max数组表示资源数量满足Max数组才能执行，有Allocation数组表示系统已经分配此数量资源，有Need数组表示还需此数量资源即可执行，有Available数组表示此时系统只有此数量资源可被分配。
3.现在有某进程发出资源请求，Request数组表示资源数量，请求系统分配该
数量资源。
4.要求实现银行家算法，求出满足进程的请求后，是否存在一条安全执行序列使得这些进程不会发生死锁。
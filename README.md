Memory Monitor
================

`mem_monitor` is self contained header file containing a C++11 memory monitor. This is a modified version obtained from 
https://github.com/mpetri/mem_monitor, that uses a multi-platform procedure for obtaining the current memory usage.

This version, however, has a limited amount of information that can be fetched:

   Metric |Description
----------------------|----------------------------
         pid | The process ID
         VmPeak | Peak virtual memory size
         VmRSS | Resident set size

## Usage

Here an example of how the class can be used:

```c++
#include "mem_monitor.hpp"

int main(int argc, char const *argv[])
{
	mem_monitor mm("mem-mon-out.csv"); // poll every 50 milliseconds

	mm.event("vector init"); // optional event
	std::vector<uint64_t> vec(50000);
	/* do more stuff */
	mm.event("sort vector"); // optional event
	std::sort(vec.begin(),vec.end());
}
```

the information are periodically written to the output file and flushed once
the object is destroyed.

Additionally, the granularity with which the monitoring thread polls

```C++
mem_monitor mm("mem-mon-out.csv",std::chrono::milliseconds(5));
```

to use the memory monitor, your program has to be linked to the `pthread`
and `rt` library and the `c++11` flags:

```bash
g++ -std=c++11 -o a.out example.cpp -l pthread -l rt
```
Note: The object does not need to be created at the beginning of `main()`. The
monitor can also be used to track only certain aspects of the executing program.

## Output

The output produced by the class consist of a CSV file:

time_ms|pid|VmPeakVmRSS|event
-------|---|-----------|-----
1|12382|15995289664116|"vector init"
51|12382|159952896682142|"vector init"
101|12382|15995289848445|"vector init"
151|12382|16044032744676|"sort vector"
201|12382|19127910976845|"sort vector"
251|12382|20180992027217|"sort vector"
301|12382|22019276446411|"sort vector"

where `time_ms` corresponds to the number of milliseconds since the creation of
the object.


## Visualization

The output can be visualized, for example, with the following `R` script:

```R
library(ggplot2)
library(reshape2)
library(sitools)
library(scales)

data <- read.csv(file="res-mon.csv",sep=";",header=TRUE);
tdata <- melt(data,id=c("time_ms","event"))
tdata <- subset(tdata,variable!="pid")
dup <-tdata[!duplicated(tdata$event),]

plot <- ggplot(tdata,aes(time_ms,value,color=variable))
plot <- plot + geom_line(size=1)
#plot <- plot + scale_y_log10(expand=c(0,0),labels=f2si,breaks = trans_breaks("log10", function(x) 10^x),"Memory Usage [Byte]")
plot <- plot + scale_y_continuous(expand=c(0,0),labels=f2si,"Memory Usage [Byte]")
#plot <- plot + annotation_logticks(sides = "lr")
plot <- plot + scale_x_continuous(expand=c(0,0),labels=f2si,"Time [Millseconds]")
plot <- plot + scale_linetype(name="Events") + scale_color_discrete(name="Metric")
plot <- plot + geom_vline(data=dup,aes(xintercept = time_ms,linetype=event),show_guide=TRUE)
plot <- plot + theme_grey() + ggtitle("Memory Usage over Time")
ggsave(plot,file="memory_usage.png")
```

## License

LGPLv3

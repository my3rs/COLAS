package main

import (
	"./utils"
	"fmt"
)

var (
	sLogRootPath     = "D:\\Users\\Cyril\\Desktop\\logs\\"
	arrayExperiments = []string{
		"Reader固定-增加Writer", // 0
		"Writer固定-增加Reader", // 1
		"服务器数量及读写比例",        // 2
		"文件大小变化",            // 3
		"写速率对读的影响",          // 4
		"只有读-Reader数量变化",    // 5
		"只有写-Writer数量变化",    // 6
		"只有写-写速率变化",
	}
	nReader   = "30"
	nWriter   = "0"
	nServer   = "5"
	fileSize  = "10240"
	readDist  = "100"
	writeDist = "100"
	sExp      = arrayExperiments[5]
	sConfig   = "R" + nReader + "W" + nWriter + "S" + nServer + "F" + fileSize + "RD" + readDist + "WD" + writeDist
	sLogPath  = sLogRootPath + sExp + "\\" + sConfig + "\\"
)

func main() {
	fmt.Println("Experiment: ", sExp, "\t", "Configure: ", sConfig)
	fmt.Println()

	utils.ParseSodawRead(sLogPath, "reader_sodaw.log")

	utils.ParseSodawWrite(sLogPath, "writer_sodaw.log")

	utils.ParseStats(sLogPath, "stats")

	fmt.Println()
	fmt.Println("read tasks/min")
	utils.TaskThroughput(sLogPath, "reader_sodaw.log")
	fmt.Println()

	fmt.Println("write tasks/min")
	utils.TaskThroughput(sLogPath, "writer_sodaw.log")
}

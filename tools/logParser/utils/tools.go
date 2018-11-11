package utils

import (
	"fmt"
	"os"
)

func writeToFile(content string, file *os.File) {
	if display {
		fmt.Println(content)
	}
	if storeToFile {
		_, err := file.Write([]byte(content))
		checkError(err)
	}
}

func WriteToFile(content string, file *os.File) {
	writeToFile(content, file)
}

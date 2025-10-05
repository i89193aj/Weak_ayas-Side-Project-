#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>

using namespace std;

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        cerr << "fork 失敗！" << endl;
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) {
        // 子進程
        cout << "[Main] 啟動 server.out ..." << endl;
        #if defined(DEBUG_MODE)
        execl("./build_debug/Weak_ayas_server", "Weak_ayas_server", nullptr);
        #else
        execl("./build_release/Weak_ayas_server", "Weak_ayas_server", nullptr);
        #endif
        perror("exec 失敗");
        exit(EXIT_FAILURE);
        
    } 
    else {
        // 父進程
        

        cout << "[Main] 啟動 client.out ..." << endl;

        // 等待 server ready，可以加 retry 或 sleep
        sleep(1);
        #if defined(DEBUG_MODE)
        execl("./build_debug/Weak_ayas_client", "Weak_ayas_client", nullptr);
        #else
        execl("./build_release/Weak_ayas_client", "Weak_ayas_client", nullptr);
        #endif

        // 若 exec 執行失敗
        perror("exec 失敗");
        exit(EXIT_FAILURE);
    }

    return 0;
}
/* 保留父進程，創兩個子進程
int main(){
	pid_t pid_server = fork();
	if (pid_server == 0) {
		execl("./Weak_ayas_server.out", "Weak_ayas_server.out", nullptr);
		perror("exec server 失敗");
		exit(EXIT_FAILURE);
	}

	pid_t pid_client = fork();
	if (pid_client == 0) {
		// 等 server ready
		sleep(1);
		execl("./Weak_ayas_client", "Weak_ayas_client", nullptr);
		perror("exec client 失敗");
		exit(EXIT_FAILURE);
	}

	// 父 process 可以等待子 process 結束
	int status;
	waitpid(pid_server, &status, 0); //0 阻塞等待子進程
	waitpid(pid_client, &status, 0);
}*/

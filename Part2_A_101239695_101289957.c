

#include <stdio.h>
#include <unistd.h>  // Required for fork()


int TA() {
    //hejhduiedhjk
    return 0;
}


//5 processes base code
int main(int argc, char* argv[]) {
    // Create 4 child processes in total
    int pid0 = fork();  // First fork
    if (pid0 == 0) {
        // This is the first child process
        printf("I am childc process 1! My PID is %d, Parent PID is %d\n", getpid(), getppid());
    } else {
        int pid1 = fork();  // Second fork
        if (pid1 == 0) {
            // This is the second child process
            printf("I am child process 2! My PID is %d, Parent PID is %d\n", getpid(), getppid());
        } else {
            int pid2 = fork();  // Third fork
            if (pid2 == 0) {
                // This is the third child process
                printf("I am child process 3! My PID is %d, Parent PID is %d\n", getpid(), getppid());
            } else {
                int pid3 = fork();  // Fourth fork
                if (pid3 == 0) {
                    // This is the fourth child process
                    printf("I am child process 4! My PID is %d, Parent PID is %d\n", getpid(), getppid());
                } else {
                    // This is the parent process, which has created 4 child processes
                    printf("I am the parent process! My PID is %d\n", getpid());
                }
            }
        }
    }

    return 0;
}


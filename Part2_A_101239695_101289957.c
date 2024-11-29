



int TA() {
    //hejhduiedhjk
    return 0;
}



int main(int argc, char* argv[]) {
    int pid0 = fork();
    int pid1 = fork();
    int pid2 = fork();
    int pid3 = fork();
    int pid4 = fork();


    if (pid0 == 0) {
        printf("I am the child process 0!\n");
    } else if (pid1 == 1) {
        printf("child 1")
    } else {
        printf("I am the parent process!\n");
    }

    return 0;

}


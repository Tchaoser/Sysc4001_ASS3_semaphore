



int TA() {
    //hejhduiedhjk
    return 0;
}



int main(int argc, char* argv[]) {
    int pid = fork();

    if (pid == 0) {
        printf("I am the child process!\n");
    } else {
        printf("I am the parent process!\n");
    }

    return 0;

}


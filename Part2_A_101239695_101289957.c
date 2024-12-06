
//how to run the code
//   gcc -pthread Part2_A_101239695_101289957.c -o Part2_A_101239695_101289957
//   ./Part2_A_101239695_101289957

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#define NUM_TAS 5
#define NUM_STUDENTS 20
#define MAX_REPEATS 3

sem_t *semaphores[NUM_TAS];
char *database_file = "database.txt";
char *ta_files[NUM_TAS] = {"TA1.txt", "TA2.txt", "TA3.txt", "TA4.txt", "TA5.txt"};

void mark_student(int ta_id, int repeat_count) {
    FILE *db_fp, *ta_fp;
    char student[5];
    int delay, mark;

    while (repeat_count < MAX_REPEATS) {
        // Access database
        sem_wait(semaphores[ta_id]);
        sem_wait(semaphores[(ta_id + 1) % NUM_TAS]);

        printf("TA %d: Accessing the database.\n", ta_id + 1);

        db_fp = fopen(database_file, "r");
        ta_fp = fopen(ta_files[ta_id], "a");

        if (!db_fp || !ta_fp) {
            perror("File open error");
            exit(1);
        }

        // Read next student number
        while (fscanf(db_fp, "%s", student) != EOF) {
            if (strcmp(student, "9999") == 0) {
                repeat_count++;
                if (repeat_count >= MAX_REPEATS) {
                    fclose(db_fp);
                    fclose(ta_fp);
                    sem_post(semaphores[(ta_id + 1) % NUM_TAS]);
                    sem_post(semaphores[ta_id]);
                    printf("TA %d: Completed marking.\n", ta_id + 1);
                    exit(0);
                }
                continue;
            }

            delay = rand() % 4 + 1; // Random delay (1-4 seconds)
            printf("TA %d: Marking student %s.\n", ta_id + 1, student);
            sleep(delay);

            // Mark student
            mark = rand() % 11; // Random mark (0-10)
            fprintf(ta_fp, "Student %s: Mark %d\n", student, mark);

            break;
        }

        fclose(db_fp);
        fclose(ta_fp);

        // Release semaphores
        sem_post(semaphores[(ta_id + 1) % NUM_TAS]);
        sem_post(semaphores[ta_id]);

        delay = rand() % 10 + 1; // Random delay (1-10 seconds)
        sleep(delay);
    }
}

int main() {
    pid_t pids[NUM_TAS];
    int i;

    // Initialize semaphores
    for (i = 0; i < NUM_TAS; i++) {
        char sem_name[10];
        sprintf(sem_name, "sem%d", i);
        semaphores[i] = sem_open(sem_name, O_CREAT, 0644, 1);
        if (semaphores[i] == SEM_FAILED) {
            perror("Semaphore initialization failed");
            exit(1);
        }
    }

    // Seed random number generator
    srand(time(NULL));

    // Create TA processes
    for (i = 0; i < NUM_TAS; i++) {
        if ((pids[i] = fork()) == 0) {
            mark_student(i, 0);
            exit(0);
        }
    }

    // Wait for all TAs to finish
    for (i = 0; i < NUM_TAS; i++) {
        wait(NULL);
    }

    // Cleanup semaphores
    for (i = 0; i < NUM_TAS; i++) {
        sem_close(semaphores[i]);
        char sem_name[10];
        sprintf(sem_name, "sem%d", i);
        sem_unlink(sem_name);
    }

    printf("All TAs have completed marking.\n");
    return 0;
}

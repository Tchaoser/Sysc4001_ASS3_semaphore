#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>

#define NUM_STUDENTS 20
#define NUM_CYCLES 3
#define NUM_TAS 5

// We'll read 20 students. The last one is "9999".
static char students[NUM_STUDENTS][5]; 

struct shared_data {
    sem_t sem[NUM_TAS];
};

int main() {
    srand(time(NULL));

    // Read the database file:
    FILE* fin = fopen("database.txt", "r");
    if (!fin) {
        perror("fopen database.txt");
        return 1;
    }

    for (int i = 0; i < NUM_STUDENTS; i++) {
        if (fscanf(fin, "%4s", students[i]) != 1) {
            fprintf(stderr, "Error reading line %d from database.txt\n", i+1);
            fclose(fin);
            return 1;
        }
    }
    fclose(fin);

    // Create shared memory for semaphores
    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_SHARED | MAP_ANONYMOUS;
    struct shared_data* shm = (struct shared_data*) mmap(NULL, sizeof(struct shared_data), prot, flags, -1, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // Initialize semaphores
    for (int i = 0; i < NUM_TAS; i++) {
        if (sem_init(&shm->sem[i], 1, 1) < 0) {
            perror("sem_init");
            return 1;
        }
    }

    // Fork 5 TAs
    for (int ta_id = 1; ta_id <= NUM_TAS; ta_id++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return 1;
        } else if (pid == 0) {
            // Child: TA process
            char filename[20];
            snprintf(filename, sizeof(filename), "TA%d.txt", ta_id);
            FILE* fout = fopen(filename, "w");
            if (!fout) {
                fprintf(stderr, "TA %d: Could not open output file.\n", ta_id);
                _exit(1);
            }

            int cycle_count = 0;
            int current_index = 0;

            while (cycle_count < NUM_CYCLES) {
                // Access database phase
                int left_sem = ta_id - 1;             // zero-based index for left semaphore
                int right_sem = (ta_id % NUM_TAS);    // zero-based index for right semaphore

                // Wait (lock) on both semaphores
                sem_wait(&shm->sem[left_sem]);
                sem_wait(&shm->sem[right_sem]);

                // TA accessing database
                printf("TA %d is accessing the database.\n", ta_id);
                fflush(stdout);

                // Simulate database access delay 1-4 sec
                int delay_db = 1 + rand() % 4;
                sleep(delay_db);

                // Read next student id
                const char* student_id = students[current_index];

                // Release semaphores after accessing database
                sem_post(&shm->sem[right_sem]);
                sem_post(&shm->sem[left_sem]);

                // Marking phase
                printf("TA %d is marking student %s.\n", ta_id, student_id);
                fflush(stdout);

                // Check if we reached "9999"
                if (strcmp(student_id, "9999") == 0) {
                    cycle_count++;
                    current_index = 0; // start from beginning next cycle
                } else {
                    current_index++;
                    if (current_index >= NUM_STUDENTS) {
                        // Should not happen if last is "9999", but just in case
                        current_index = 0;
                    }
                }

                // Random mark between 0 and 10
                int mark = rand() % 11;
                // Marking delay 1-10 sec
                int delay_mark = 1 + rand() % 10;
                sleep(delay_mark);

                // Write result to TAj.txt
                fprintf(fout, "%s %d\n", student_id, mark);
                fflush(fout);
            }

            fclose(fout);
            printf("TA %d finished all cycles.\n", ta_id);
            fflush(stdout);
            _exit(0);
        }
        // Parent continues to spawn the other TAs
    }

    // Parent: wait for all TAs to finish
    for (int i = 0; i < NUM_TAS; i++) {
        wait(NULL);
    }

    // Destroy semaphores
    for (int i = 0; i < NUM_TAS; i++) {
        sem_destroy(&shm->sem[i]);
    }

    // Unmap shared memory
    munmap(shm, sizeof(*shm));

    printf("All TAs have finished.\n");
    return 0;
}
